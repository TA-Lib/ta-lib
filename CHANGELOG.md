# Changelog

Installation docs: https://ta-lib.org/install/

Just re-install to upgrade. Older versions are automatically removed.

See [github commits](https://github.com/TA-Lib/ta-lib/commits) for complete list of changes

## [0.8.1] Not Released Yet
### Added
- New Streaming API. See https://ta-lib.org/api/stream/
- (#81) Microsoft VCPKG support. Thanks @greenTableWork !
- (#78) CMake can now opt out of building the static or the shared library (both built by default). Thanks @BwL1289 !
- (#75) More docs for DEMA, TEMA, T3, MFI, ULTOSC, KAMA and TRIX. Thanks @nehemiah888 !
- New TA Functions:
  - AC: Accelerator/Decelerator Oscillator (#228)
  - AO: Awesome Oscillator (#227)
  - CMF: Chaikin Money Flow (#134)
  - CMOU: Chande Momentum Oscillator, Unsmoothed (#124)
  - DONCHIAN: Donchian Channels, the rolling extrema bands (#342)
  - EFI: Elder's Force Index (#206)
  - HMA: Hull Moving Average (#139)
  - KC: Keltner Channels (#273)
  - MARKETFI: Market Facilitation Index (#230)
  - NVI: Negative Volume Index (#126)
  - PVI: Positive Volume Index (#126)
  - PVO: Percentage Volume Oscillator (#119)
  - QSTICK: Qstick (#226)
  - RMA: Wilder's Smoothed Moving Average (#348)
  - SMI: Stochastic Momentum Index (#238)
  - SUPERTREND: SuperTrend, an ATR-scaled trailing band with a trend flag (#272)
  - VWAP: Volume Weighted Average Price (#237)
  - VWMA: Volume Weighted Moving Average (#131)
  - WAD: Williams' Accumulation/Distribution (#200)
  - ZLEMA: Zero-Lag Exponential Moving Average (#347)
- New MAType (for MA, BBANDS, STOCH etc...):
  - TA_MAType_HMA (#139)
  - TA_MAType_DISABLED — no smoothing at any period; the output is a copy of the input (#93)
  - TA_MAType_DEFAULT — selects that parameter's documented MA type (#182)
  - TA_MAType_ZLEMA (#347)
  - TA_MAType_RMA (#348)

### Faster
- ~8x: MACD, MACDFIX and MACDEXT (when MA type is EMA).
- ~3x to 7x: DEMA, TEMA and TRIX
- ~2x: ACCBANDS, MFI (#244), SQRT (#192). Thanks @kevinlincg !
- ~1.6x to 15x: MIN, MAX, MINMAX, MIDPOINT, MIDPRICE and WILLR (#147). Thanks @kevinlincg !
- ~40%: ULTOSC (#154). Thanks @dexhunter !
- ~30%: MAVP (#143). Thanks @dexhunter !
- ~27% Apple, ~8% GCC: MIN, MAX, MINMAX, MININDEX, MAXINDEX, MINMAXINDEX, MIDPOINT, MIDPRICE, AROON, AROONOSC and WILLR (#128). Thanks @dexhunter !
- ~20%: VAR, STDDEV, BBANDS
- ~3x to 4.7x: ATR and NATR (#338), and ~1.4x SUPERTREND / ~1.3x KC with them. The upper end needs the gcc/glibc/x86_64 hardware-FMA clone; elsewhere it is ~2x.

### Changed
- (#133) BBANDS default `optInTimePeriod` changed from 5 to 20, as intended by John Bollinger.
- (#120) PPO and APO now default `optInMAType` to EMA (was SMA), matching Gerald Appel's original PPO/MACD definition. Pass `TA_MAType_SMA` explicitly to keep the previous behavior.
- (#96) Fused multiply-add and other floating-point re-ordering produce minor output differences; an intentional modernization.
- (#338) ATR, NATR and SUPERTREND smooth the true range with a fused two-coefficient step
  instead of multiply, add, divide, which takes the divide out of the loop-carried chain.
  Values move by at most 1.3e-15 relative from the reference series, and KC inherits the
  same shift through its ATR. Period 1 and 2 are unchanged.
- (#183) EMA now uses a fused multiply-add in its recursion, as the EMA cascades inside
  DEMA, TEMA, TRIX, MACD and MACDFIX already did. Values move by at most 2.8e-16 relative
  from the reference series, and the same shift reaches MA, BBANDS, APO, PPO, PVO, MAVP,
  STOCH, STOCHF and STOCHRSI when the MAtype is EMA.
- (#4,#14) API: `TA_FUNC_UNST_MFI` and `TA_FUNC_UNST_IMI` enum constants removed
- (#129) API: `TA_FUNC_UNST_ADXR` and `TA_FUNC_UNST_STOCHRSI` enum constants removed.
- (#180) API: `startIdx` and `endIdx` are now capped at the new `TA_MAX_INDEX` (100,000,000);
  above it a call returns `TA_OUT_OF_RANGE_START_INDEX` / `TA_OUT_OF_RANGE_END_INDEX`.
- (#144) API: `TA_FUNC_UNST_NONE` enum constant removed. It could not be passed in
  (it is rejected) and was never returned, so it had no use in the public API.
- (#122) Removed the `ide/` directory (Visual Studio/Xcode/MSVC project files). Use autotools, CMake and vcpkg instead.

### Deprecated
- `TA_SetCompatibility()` and `TA_GetCompatibility()`. The notion of variant (e.g. MetaStock compatibility) is not actively maintained and will be removed in a future release. Default behavior is unaffected. Moving forward TA-Lib will create separate TA functions for distinct behaviors.

### Fixed
- (#130) In-place calls (same buffer as input and output) returned wrong values for STOCH, STOCHF and MAVP. Regular (separate-buffer) calls were always correct.
- (#118,#242) VAR, CORREL, STDDEV and BBANDS more precise and faster.
- (#33) Float overflow in the single-precision (`TA_S_*`) functions. Thanks @iglesias !
- (#64) Website docs mixing up CDL3LINESTRIKE with CDL3OUTSIDE's description. Thanks @mw66 !
- (#7) CCI returned a spurious value when all prices over the period were identical; Thanks @trufanov-nok for identifying and resolving this!
- (#57) Missing TA_GetVersionString function in Windows DLL. Thanks @Youngv !
- (#98) TRIX and NATR returned wrong values when startIdx > lookback. NATR additionally left output slots unwritten for any bar with a zero close.
- (#98) A non-zero unstable period settings changed IMI's summation window. Fine with default settings.
- (#107) MFI and STOCHRSI could return a wrong value when floating-point rounding left a near-zero result that was then compared exactly against zero. Thanks @Caleblgx, @trufanov-nok and @mrjbq7 !
- (#4,#14) MFI and IMI are no longer flagged as having an unstable period. Thanks @mw66 and @wony-zheng !
- (#99) BBANDS with `TA_MAType_MAMA` and a period >= 34 returned a misaligned middle band.
- (#77) CMake shared library now links libm directly, so it declares its own math-library dependency instead of relying on the consuming program to provide it. Thanks @BwL1289 !
- (#102) Fixed ULTOSC and CDL3INSIDE performance (regression only in 0.7.1)
- (#112) IMI returned NaN on an all-flat window (every bar `close == open`); now returns 50.0.
- (#202) VAR no longer returns a tiny negative variance on a flat stretch, where the calculation cancels to either side of zero; it now returns 0.0 instead.
- (#243) STDDEV and BBANDS returned exactly 0 for a standard deviation that was small but non-zero. In rare cases, was making the bands "collapse" on the middle line.
- (#244) MFI returned 0 instead of the index whenever the window summed to less than 1.0. Also, no longer returns values slightly outside 0-100 (clamps the epsilon errors).
- (#253) Fix many TA_IS_ZERO vs TA_IS_ZERO_SCALED choices. Numerically better for edge cases, like very small inputs (<10e-8) or mostly flat input prices.

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

