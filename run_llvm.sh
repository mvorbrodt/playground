#!/bin/sh

cd ~/Code/playground
ln -sf CMakeLists.txt.llvm CMakeLists.txt

source env_llvm.sh

rm -rf llvm 2> /dev/null
mkdir llvm 2> /dev/null
cd llvm
cmake -DCMAKE_OSX_SYSROOT="$(xcrun --show-sdk-path)" ..
