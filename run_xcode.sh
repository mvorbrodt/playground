#!/bin/sh

cd ~/Code/playground
ln -sf CMakeLists.txt.llvm CMakeLists.txt

mkdir xcode 2> /dev/null
cd xcode
rm CMakeCache.txt 2> /dev/null
cmake -DCMAKE_OSX_SYSROOT="$(xcrun --show-sdk-path)" -G Xcode ..
cd ..
