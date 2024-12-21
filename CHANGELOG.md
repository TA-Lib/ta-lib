# Changelog

Check https://ta-lib.org/install/ for latest installation instructions.

Only notable changes are documented here. See GitHub commits for all changes.

## [0.6.0] 2024-12-24
### Added
- Packaging automation for various platforms, notably Windows 64 bits.
- Fix for very small inputs to TA functions (floating point epsilon problem).

### Fixed
- Autotools and CMakeLists.txt have been modernized.

### Changed

- Static/Shared lib file names uses hyphen instead of underscore. This was needed for some package naming convention.
  In other word, look for "ta-lib" instead of "ta_lib".

  Example: when linking you now use "-lta-lib" instead of "-lta_lib".

- C/C++ headers are now under a "ta-lib" subdirectory. You may have to change your code accordingly.

  Example: `#include <ta-lib/ta_libc.h>` instead of `#include <ta_libc.h>`

  This change is for namespace best-practice for when TA-Lib is installed at the system level.

- Moving forward, autotools and CMake are the only two supported build systems. Consequently:
    - All xcode/Visual Studio projects (.sln) are not maintained anymore.
    - There is no "cdd", "cdr" etc... library variants anymore. This is an outdated way of doing.
    - The ide/ and make/ directories from 0.4.0 have been removed.

  Recommendation: VSCode+CMake works consistently on most platforms.

- TA_GetVersionBuild() is deprecated. Use TA_GetVersionPatch() instead.

