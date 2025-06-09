## Fork of TA-Lib with iOS cmake toolchain support

Build for iOS/Simulator/macOS:

```bash
cmake .

cmake -B build-ios -DCMAKE_TOOLCHAIN_FILE=cmake/ios.toolchain.cmake -DPLATFORM=OS64 -DDEPLOYMENT_TARGET=18.0
cmake -B build-ios-sim -DCMAKE_TOOLCHAIN_FILE=cmake/ios.toolchain.cmake -DPLATFORM=SIMULATORARM64 -DDEPLOYMENT_TARGET=18.0
cmake -B build-macos -DCMAKE_SYSTEM_NAME=Darwin

cmake --build build-ios --config Release
cmake --build build-ios-sim --config Release
cmake --build build-macos --config Release
```

---
[![Discord chat](https://img.shields.io/discord/1038616996062953554.svg?logo=discord&style=flat-square)](https://discord.gg/Erb6SwsVbH)

[![main nightly tests](https://github.com/TA-Lib/ta-lib/actions/workflows/main-nightly-tests.yml/badge.svg)](https://github.com/TA-Lib/ta-lib/actions/workflows/main-nightly-tests.yml) [![dev nightly tests](https://github.com/TA-Lib/ta-lib/actions/workflows/dev-nightly-tests.yml/badge.svg)](https://github.com/TA-Lib/ta-lib/actions/workflows/dev-nightly-tests.yml)

# TA-Lib - Technical Analysis Library
This is now the official home for C/C++ TA-Lib (instead of SourceForge).

More info [https://ta-lib.org](https://ta-lib.org)

# You want a new TA Function implemented?
First step is to document the algorithm, with a sample of input/output in the [ta-lib-proposal-drafts]( https://github.com/TA-Lib/ta-lib-proposal-drafts ) repos.
