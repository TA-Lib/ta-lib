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
  - EFI: Elder's Force Index (#206)
  - HMA: Hull Moving Average (#139)
  - MARKETFI: Market Facilitation Index (#230)
  - NVI: Negative Volume Index (#126)
  - PVI: Positive Volume Index (#126)
  - PVO: Percentage Volume Oscillator (#119)
  - QSTICK: Qstick (#226)
  - SMI: Stochastic Momentum Index (#238)
  - VWMA: Volume Weighted Moving Average (#131)
  - WAD: Williams' Accumulation/Distribution (#200)
- (#236) `TA_INSUFFICIENT_HISTORY` (17), a new `TA_RetCode`: a streaming
  `Open`/`OpenAndFill` given fewer than `lookback + 1` bars reports it. That is the
  library's one recoverable failure — accumulate more bars and retry — so it is worth
  telling apart from `TA_BAD_PARAM`, which always means the call itself is wrong.
  Appended, so no existing code's value moved. The batch tier is unaffected: a range
  shorter than the lookback is still `TA_SUCCESS` with a zero count.
- (#236) Java and C#: every exception the library raises now carries the
  `TA_RetCode` it corresponds to — `TaLibFailure.retCode()` in Java,
  `ITaLibFailure.RetCode` in C#. The exception types are unchanged (the new classes
  subclass the ones already documented, so existing `catch` blocks keep working);
  what is new is that a caller can tell apart the conditions one exception type
  covers — `startIdx` from `endIdx`, an allocation failure from an internal error.
  Java's `RetCode` enum is public for this, with `asCInt()` giving C's number. It
  covers every indicator call, batch and streaming; the settings builder and the
  metadata binder are not indicator calls and still raise plain types.
- New MAType (for MA, BBANDS, STOCH etc...):
  - TA_MAType_HMA (#139)
  - TA_MAType_DISABLED — no smoothing at any period; the output is a copy of the input (#93)
  - TA_MAType_DEFAULT — selects that parameter's documented MA type (#182)

### Faster
- ~3x to 7x: DEMA, TEMA and TRIX
- ~8x: MACD and MACDFIX
- ~8x: MACDEXT when MA types are EMA.
- ~2.4x: ACCBANDS
- ~2x: SQRT (#192). Thanks @kevinlincg !
- ~1.6x to 15x: MIN, MAX, MINMAX, MIDPOINT, MIDPRICE and WILLR (#147). Thanks @kevinlincg !
- ~40%: ULTOSC (#154). Thanks @dexhunter !
- ~30%: MAVP (#143). Thanks @dexhunter !
- ~27% Apple, ~8% GCC: MIN, MAX, MINMAX, MININDEX, MAXINDEX, MINMAXINDEX, MIDPOINT, MIDPRICE, AROON, AROONOSC and WILLR (#128). Thanks @dexhunter !
- ~20%: VAR, STDDEV, BBANDS
- ~10%: ATR and NATR

### Changed
- (#236) The cross-language test harness now drives each language's public API for
  every correctness comparison, so the surface a caller actually touches — its
  argument checks, its exception mapping, the range it reports — is compared against
  the C reference on every case in the cross-language corpus. It was comparing an
  internal tier before. No behaviour changed: the return code of every case is
  identical either way, which is how the switch was verified.
- (#236) Java and C#: the internal `RetCode`-returning tier is gone. `Core` now has
  one entry point per indicator — the `OutRange`-returning method that throws on a
  rejection — instead of that beside a second, near-identical method taking two
  out-parameters. Nothing in the public API changed: the tier was package-private in
  Java and `internal` in C#, so no caller outside the library could name it.
- (#236) Java and C#: when one indicator is built from another (APO from MA, BBANDS
  from MA and STDDEV, …) the inner call now goes through the same public API you
  would call yourself. One consequence is visible: if the inner call is the one that
  rejects, the exception names the inner function — `MA: bad parameter` from a call
  you made to `MACDEXT`. Reaching it needs a fault the outer function does not screen
  for first, so it is rare; the outer function's own rejections are unchanged.
- (#236) Java: an out-of-range `startIdx`/`endIdx` is now reported ahead of a null
  array argument, matching C's order — previously a null buffer pre-empted the index
  complaint. And a null enum parameter (e.g. `MAType`) is rejected naming the function
  and the parameter instead of raising a bare `NullPointerException` from inside the
  lookback.
- (#133) BBANDS default `optInTimePeriod` changed from 5 to 20, as intended by John Bollinger.
- (#120) PPO and APO now default `optInMAType` to EMA (was SMA), matching Gerald Appel's original PPO/MACD definition. Pass `TA_MAType_SMA` explicitly to keep the previous behavior.
- (#96) Fused multiply-add and other floating-point re-ordering produce minor output differences; an intentional modernization.
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
- (#118) VAR, STDDEV and BBANDS more precise and faster.
- (#33) Float overflow in the single-precision (`TA_S_*`) functions. Thanks @iglesias !
- (#64) Website docs mixing up CDL3LINESTRIKE with CDL3OUTSIDE's description. Thanks @mw66 !
- (#7) CCI returned a spurious value when all prices over the period were identical; Thanks @trufanov-nok for identifying and resolving this!
- (#57) Missing TA_GetVersionString function in Windows DLL. Thanks @Youngv !
- (#98) TRIX and NATR returned wrong values when startIdx > lookback. NATR additionally left output slots unwritten for any bar with a zero close.
- (#98) A non-zero unstable period changed IMI's summation window.
- (#107) MFI and STOCHRSI could return a wrong value when floating-point rounding left a near-zero result that was then compared exactly against zero. Thanks @Caleblgx, @trufanov-nok and @mrjbq7 !
- (#4,#14) MFI and IMI are no longer flagged as having an unstable period. Thanks @mw66 and @wony-zheng !
- (#99) BBANDS with `TA_MAType_MAMA` and a period >= 34 returned a misaligned middle band.
- (#77) CMake shared library now links libm directly, so it declares its own math-library dependency instead of relying on the consuming program to provide it. Thanks @BwL1289 !
- (#102) Fixed ULTOSC and CDL3INSIDE performance regression (only in 0.7.1)
- (#112) IMI returned NaN on an all-flat window (every bar `close == open`); now returns 50.0.
- (#202) VAR no longer returns a tiny negative variance on a flat stretch, where the calculation cancels to either side of zero; it now returns 0.0 instead.

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

