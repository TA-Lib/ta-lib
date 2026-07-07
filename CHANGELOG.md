# Changelog

Installation docs: https://ta-lib.org/install/

Just re-install to upgrade. Older versions are automatically removed.

See [github commits](https://github.com/TA-Lib/ta-lib/commits) for complete list of changes

## [0.7.2] Not Released Yet
### Added
- (#81) Microsoft VCPKG support. Thanks @greenTableWork !
- (#78) CMake can now build only the shared or only the static library. Thanks @BwL1289 !

### Changed
- Algo Optimisation: DEMA, TEMA and TRIX are 3x to 7x faster.
- Algo Optimisation: MACD and MACDFIX are ~8x faster, and MACDEXT as well when all three MA types are EMA.
- Algo Optimisation: ATR and NATR compute the True Range inline (single pass, no temporary buffer), ~10-15% faster.

### Fixed
- (#7) CCI returned a spurious value (~66.67) instead of 0.0 when all prices over the period were identical; the near-zero guard now uses an epsilon tolerance (TA_IS_ZERO) rather than an exact `!= 0.0` comparison. Thanks @trufanov-nok for identifying and resolving this!
- (#57) Missing TA_GetVersionString function in Windows DLL. Thanks @Youngv !
- (#98) TRIX and NATR returned wrong values when startIdx > lookback, and a non-zero unstable period changed IMI's summation window.
- (#99) BBANDS with `TA_MAType_MAMA` and a period >= 34 returned a misaligned middle band.
- (#77) The CMake shared library was not linked against libm, leaving math symbols unresolved for consumers. Thanks @BwL1289 !

## [0.7.1] 2026-07-03
### Added
- (#79) TA-Lib is now available as a GitHub Action: [setup-ta-lib](https://github.com/TA-Lib/setup-ta-lib). Thanks @mrjbq7 !
- (#86) Conan package manager support. Thanks @CaptainTrunky !

### Welcome new wrappers
- Go: [ta-lib-cgo](https://github.com/TA-Lib/ta-lib-cgo). Thanks @bradleypeabody !
- PHP: [ext-ta-lib](https://github.com/TA-Lib/ext-ta-lib). Thanks @rernesto !
- PostgreSQL: [ta_pg](https://github.com/TA-Lib/ta_pg). Thanks @tuxmonteiro !
- R: [ta-lib-R](https://github.com/serkor1/ta-lib-R). Thanks @serkor1 !
- Ruby: [ta-lib-ruby](https://github.com/TA-Lib/ta-lib-ruby). Thanks @Youngv !
- Zig: [ta-lib-zig](https://github.com/TA-Lib/ta-lib-zig). Thanks @mrjbq7 !

### Changed
- Major simplification of how TA functions are written and generated: the new ta_codegen (Rust) replaces the outdated gen_code — a big win for maintainability and the coming Rust-native release. Thanks @chadfurman !
- The `TA_FUNC_NO_RANGE_CHECK` compile flag is gone: parameter validation is always on in the public functions. Callers that used the flag for speed should call the new exported `TA_*_Unguarded` variants instead (no validation, same results for valid inputs).
- Algo Optimization: MIDPOINT and MIDPRICE now cache the rolling min/max instead of rescanning the window each bar, reducing typical cost from O(n*period) toward O(n) (largest gains at bigger periods).
- Removed outdated ta-lib/make directory. Use CMake and Autotools instead.
- (#70) Documentation index updates. Thanks @kennethjor !

### Fixed
- (#48, #59) Fixed period=1 handling. MACD/MACDFIX with `signalPeriod=1`, TRIX and ULTOSC with period 1 used to produce misaligned output. A period of 1 now consistently means "no smoothing": SMA, EMA, WMA, DEMA, TEMA, TRIMA, KAMA, T3 and MAVP accept a minimum period of 1 and return the input unchanged, MACD-family signal lines with `signalPeriod=1` equal the MACD line (histogram). A new `PERIOD1/BOUNDARY` regression-test group pins all of this, for every backend. Thanks @trufanov-nok for the original analysis and fix in ta-lib-rt!
- (#62) Fixed an out-of-bounds access in the regression test tooling. Thanks @Lqingyu !
- (#68) Corrected a spelling error in the TA_LIB_NOT_INITIALIZE return-code message. Thanks @alteholz !
- (#88) HT_TRENDLINE: removed an internal buffer that was written on every bar but never read (small speed-up, no output change). Thanks to Jake Arkinstall (from SourceForge) and @731315163 !

## [0.6.4] 2025-01-11
### Fixed
- (#54): Fix gen_code compilation on Windows
- RPM packaging: Fix ta-lib.spec.in with Github URL instead of sourceforge


## [0.6.3] 2025-01-06
### Fixed
- (#52) Add missing export to import lib for Windows DLL.


## [0.6.2] 2024-12-26
### Added
- Windows - New 32 bits zip and msi packages.

### Fixed
- (#51) Allow for Debian 11 and Ubuntu 22.04 LTS support with lower version of CMake
- (#43) Windows - Fix 64 bits DLL install location to C:\Program Files\TA-Lib
- x86 Debian package renamed to i386 (as per Debian convention)


## [0.6.1] 2024-12-23
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

