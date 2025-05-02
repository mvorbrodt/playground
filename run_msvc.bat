@echo off

set VCPKG_TARGET_TRIPLET=x64-windows
vcpkg install tbb parallelstl boost range-v3

copy /Y CMakeLists.txt.msvc CMakeLists.txt
del /F /S /Q msvc
del /F /Q msvc
mkdir msvc
cd msvc

cmake -DCMAKE_TOOLCHAIN_FILE=C:/Code/vcpkg/scripts/buildsystems/vcpkg.cmake -DVCPKG_TARGET_TRIPLET=x64-windows ..

cd ..

REM msbuild -m msvc/playground.sln -t:Rebuild -p:Configuration=Debug
REM msbuild -m msvc/playground.sln -t:Rebuild -p:Configuration=Release
