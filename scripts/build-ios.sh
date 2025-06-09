#!/bin/bash
set -euo pipefail

cmake -B build-ios -DCMAKE_TOOLCHAIN_FILE=cmake/ios.toolchain.cmake -DPLATFORM=OS64 -DDEPLOYMENT_TARGET=18.0

cmake -B build-ios-sim -DCMAKE_TOOLCHAIN_FILE=cmake/ios.toolchain.cmake -DPLATFORM=SIMULATORARM64 -DDEPLOYMENT_TARGET=18.0

cmake -B build-macos -DCMAKE_SYSTEM_NAME=Darwin

cmake --build build-ios --config Release
cmake --build build-ios-sim --config Release
cmake --build build-macos --config Release
