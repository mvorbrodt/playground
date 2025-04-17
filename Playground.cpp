/*#include <iostream>
#include <sstream>
#include <iomanip>
#include <locale>
#include <codecvt>
#include <string>
#include <iterator>
#include <algorithm>
#include <stdexcept>
#include <utility>
#include <thread>
#include <chrono>
#define NOMINMAX
#include <Windows.h>
#include <ShellScalingApi.h>
#include <dxgi1_6.h>
#include <wil/com.h>
#include <wil/registry.h>
#include <wil/registry_helpers.h>

bool IsMainMonitor(HMONITOR monitor)
{
    static auto GetMyMainWindow = []() -> HWND
    {
        using WindowInfo = std::pair<HWND, DWORD>;
        using WindowInfoPtr = WindowInfo*;

        static auto window_proc = [](HWND window, LPARAM param) -> BOOL
        {
            auto window_info = reinterpret_cast<WindowInfoPtr>(param);
            auto pid = DWORD{};
            auto result = GetWindowThreadProcessId(window, &pid);
            if (result && pid == window_info->second && GetWindow(window, GW_OWNER) == 0 && IsWindowVisible(window))
            {
                SetLastError(ERROR_EVENT_DONE);
                window_info->first = window;

                return FALSE;
            }

            return TRUE;
        };

        auto params = WindowInfo{ {}, GetCurrentProcessId() };
        auto result = EnumWindows(window_proc,(LPARAM)&params);
        if (!result && GetLastError() == ERROR_EVENT_DONE && params.first)
        {
            return params.first;
        }

        return HWND{};
    };

    auto main_window = GetMyMainWindow();
    auto main_monitor = MonitorFromWindow(main_window, MONITOR_DEFAULTTONEAREST);

    return monitor == main_monitor;
}

void CollectMonitors()
{
    static auto WStringToString = [](const std::wstring & wstr, const std::locale & loc = std::locale("en_US.UTF-8"))
    {
        static auto transform_if = [](auto first, auto last, auto d_first, auto pred, auto op)
        {
            using ValT = std::iterator_traits<decltype(first)>::value_type;
            using VecT = std::vector<ValT>;
            auto temp = VecT{};
            temp.reserve(std::distance(first, last));
            std::copy_if(first, last, std::back_inserter(temp), pred);
            return std::transform(temp.begin(), temp.end(), d_first, op);
        };

        auto str = std::string{};
        str.reserve(wstr.length());
        transform_if(std::begin(wstr), std::end(wstr), std::back_inserter(str),
            [](auto wc) { return wc != L'\0'; },
            [&loc](auto wc) { return std::use_facet<std::ctype<wchar_t>>(loc).narrow(wc, 0); });
        return str;
    };

    wil::com_ptr_t<IDXGIFactory6> pFactory = nullptr;
    auto hr = CreateDXGIFactory(IID_PPV_ARGS(&pFactory));
    if (FAILED(hr))
    {
        auto ss = std::stringstream{};
        ss << __FUNCTION__ << " CreateDXGIFactory failed! HRESULT = 0x" << std::hex << std::setfill('0') << std::setw(8) << hr;
        throw std::runtime_error(ss.str());
    }

    wil::com_ptr_t<IDXGIAdapter4> pAdapter = nullptr;
    for (UINT i = 0; pFactory->EnumAdapterByGpuPreference(i, DXGI_GPU_PREFERENCE_UNSPECIFIED, IID_PPV_ARGS(&pAdapter)) != DXGI_ERROR_NOT_FOUND; ++i)
    {
        auto adapter_desc = DXGI_ADAPTER_DESC3{};
        pAdapter->GetDesc3(&adapter_desc);

        //if (adapter_desc.Flags & DXGI_ADAPTER_FLAG3_SOFTWARE) continue;

        wil::com_ptr_t<IDXGIOutput6> pOutput = nullptr;
        for (UINT j = 0; SUCCEEDED(pAdapter->EnumOutputs(j, (IDXGIOutput**)&pOutput)); ++j)
        {
            auto output_desc = DXGI_OUTPUT_DESC1{};
            pOutput->GetDesc1(&output_desc);

            auto dpiX = UINT{};
            auto dpiY = UINT{};
            auto result = GetDpiForMonitor(output_desc.Monitor, MDT_EFFECTIVE_DPI, &dpiX, &dpiY);

            auto resolutionX = static_cast<std::size_t>(output_desc.DesktopCoordinates.right - output_desc.DesktopCoordinates.left);
            auto resolutionY = static_cast<std::size_t>(output_desc.DesktopCoordinates.bottom - output_desc.DesktopCoordinates.top);
            auto main_monitor = IsMainMonitor(output_desc.Monitor);
            auto dpi = static_cast<std::size_t>((SUCCEEDED(result) ? std::max(dpiX, dpiY) : 0) / 96.f * 100.f);
            auto hdr = bool{ output_desc.ColorSpace == DXGI_COLOR_SPACE_RGB_FULL_G2084_NONE_P2020 };
            auto bpc = std::size_t{ output_desc.BitsPerColor };
            auto vendor = WStringToString(adapter_desc.Description);

            std::cout << WStringToString(output_desc.DeviceName) << ", " << vendor << ", RES: " << resolutionX << "x" << resolutionY << ", main: " << main_monitor << ", BPC: " << bpc << ", HDR: " << hdr << ", DPI: " << dpi << std::endl;
        }
    }

    std::cout << std::endl;
}

void CollectCodecs()
{
    using namespace std::string_literals;

    static const auto kPath = L"SOFTWARE\\Citrix\\ICA Client\\Engine\\Configuration\\Advanced\\Modules\\GfxRender\\DecoderCaps"s;
    static const auto kFPS = L"FramePerSecond"s;
    static const auto kSupported = L"Supported"s;

    auto key1 = wil::reg::open_unique_key(HKEY_CURRENT_USER, kPath.c_str());
    auto adapters = wil::make_range(wil::reg::key_iterator{ key1.get() }, wil::reg::key_iterator{});
    for (const auto& adapter : adapters)
    {
        auto key2 = wil::reg::open_unique_key(HKEY_CURRENT_USER, (kPath + L"\\" + adapter.name).c_str());
        auto codecs = wil::make_range(wil::reg::key_iterator{ key2.get() }, wil::reg::key_iterator{});
        for (const auto& codec : codecs)
        {
            auto key3 = wil::reg::open_unique_key(HKEY_CURRENT_USER, (kPath + L"\\" + adapter.name + L"\\" + codec.name).c_str());
            auto colors = wil::make_range(wil::reg::key_iterator{ key3.get() }, wil::reg::key_iterator{});
            for (const auto& color : colors)
            {
                auto key4 = wil::reg::open_unique_key(HKEY_CURRENT_USER, (kPath + L"\\" + adapter.name + L"\\" + codec.name + L"\\" + color.name).c_str());
                auto fps = wil::reg::get_value_dword(key4.get(), kFPS.c_str());
                auto supported = (wil::reg::get_value_dword(key4.get(), kSupported.c_str()) == 1);
                if (supported)
                {
                    std::wcout << adapter.name << " -> " << codec.name << " -> " << color.name << " fps: " << fps << std::endl;
                }
            }
        }
    }

    std::cout << std::endl;
}

int main()
{
    using namespace std::chrono_literals;

    SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);

    while (true)
    {
        system("cls");

        CollectMonitors();
        CollectCodecs();

        std::this_thread::sleep_for(1s);
    }

    return 0;
}*/