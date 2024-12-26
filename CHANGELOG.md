# Changelog

Check https://ta-lib.org/install/ for latest installation instructions.

Only notable changes are documented here. See GitHub commits for all changes.

## [0.6.2] 2024-12-26
### Fixed
- (#51) Allow for Debian 11 and Ubuntu 22.04 LTS support with lower version of CMake
- (#43) Windows - Fix 64 bits DLL install location to C:\Program Files\TA-Lib
- x86 Debian package renamed to i386 (as per Debian convention)

### Added
- Windows - New 32 bits zip and msi packages.

## [0.6.1] 2024-12-24
### Added
- Packaging automation for various platforms, notably Windows 64 bits.

### Fixed
- Autotools and CMakeLists.txt have been modernized.
- Fix for very small inputs to TA functions (floating point epsilon problem).

### Changed

- Static/Shared lib file names uses hyphen instead of underscore. This was needed for some package naming convention.
  In other word, look for "ta-lib" instead of "ta_lib".

  Example: when linking you now specify "-lta-lib" instead of "-lta_lib".

- C/C++ headers are now under a "ta-lib" subdirectory. You may have to change your code accordingly.

  Best way to handle this is to add the headers path to your compiler (e.g. `-I/usr/local/include/ta-lib` for gcc).

  Alternatively, you can modify your code to `#include <ta-lib/ta_libc.h>` instead of `#include <ta_libc.h>`

  This change is for namespace best-practice for when TA-Lib is installed at the system level.

- Moving forward, autotools and CMake are the only two supported build systems. Consequently:
    - All xcode/Visual Studio projects (.sln) are not maintained anymore.
    - There is no "cdd", "cdr" etc... library variants anymore. This is an outdated way of doing.
    - The ide/ and make/ directories from 0.4.0 have been removed.

  Recommendation: VSCode+CMake works consistently on most platforms.

- TA_GetVersionBuild() is deprecated. Use TA_GetVersionPatch() instead.

