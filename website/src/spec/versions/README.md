---
title: Versions and Determinism
description: "Which TA-Lib results are bit-identical across languages and equivalent calls, what a C build from source must pass to keep that, and what a release keeps and may add."
---

*Part of TA-Lib's [specification](/spec/), a reference for AI-agent-driven integration.*

Where the same call gives the same bits, and what a release keeps and may add.

## The caller must {#caller}

- <a id="tolerance"></a>**Compare a transcendental call with a tolerance** unless both results come from C or Rust on the same machine ([transcendental functions](/spec/versions/#transcendental)). Their difference starts in the last bit of one math-library result and can grow through the function's arithmetic; the HT_* outputs that round a computed period to whole bars can differ outright on some inputs.
- <a id="build-flags"></a>**Build C from source with the project's floating-point flags.** [rD1](/spec/versions/#rd1) and [rD2](/spec/versions/#rd2) hold for C only when the compiler evaluates every `double` operation as written:

  | Condition | GCC and Clang | Build files in the source tree |
  |---|---|---|
  | no contraction of `a*b+c` into a fused multiply-add | `-ffp-contract=off` | CMake passes it to every compiler but MSVC and those taking MSVC's command line (clang-cl); autotools passes it when the compiler accepts it |
  | no value-changing optimization | no `-ffast-math` or `-Ofast`, nor any part of them but `-fno-math-errno` | none uses them |
  | no extended-precision intermediates | on 32-bit x86, `-msse2 -mfpmath=sse` | only CMake's i386 cross-build toolchain, `cmake/toolchain-linux-i386.cmake`; a native 32-bit x86 build does not pass it, so add it yourself |

  Under MSVC or clang-cl, CMake passes no floating-point option; no condition is stated for them, and rD1 and rD2 are not promised for C built with them.
- <a id="enumerate"></a>**Enumerate, never hard-code a list.** A release may add functions, MA types, unstable-period ids, candle settings and return codes ([rV3](/spec/versions/#rv3)). Enumerate functions through the [abstraction layer](/spec/#abstraction), bound an MA-type value with the enum your code was built with ([rP2](/spec/inputs-outputs/#rp2)), and give every `switch` or `match` over a TA-Lib enum a default branch.
- <a id="pin"></a>**Validate against the release you ship.** No rule covers output values from one release to the next, nor API compatibility of the Rust, Java and C# packages between releases beyond rV2 and rV3. Which versions and packages exist: [Install](/install/).

## Determinism {#determinism}

On this page, **bit-identical** means the same return code, the same output range, and every output element with the same bits, except that a NaN matches any NaN: no math library specifies a NaN's payload. **The same call** means the same function, input values, `startIdx` and `endIdx`, optional parameters and settings.

<a id="rd1"></a>**rD1** On one machine, C and Rust are bit-identical for the same call, transcendental functions included.

<a id="rd2"></a>**rD2** On one machine, Java and C# are bit-identical to C for every call that evaluates no transcendental function. A call that evaluates one may differ from C's: Java's and .NET's math libraries can round a transcendental result differently from the C library in the last bit, which is beyond TA-Lib's control.

### Transcendental functions {#transcendental}

A **transcendental function** here is `exp`, `log`, `log10`, or a trigonometric, inverse trigonometric or hyperbolic function. C and Rust take it from the platform's C math library, Java from the JVM, C# from the .NET runtime, and none of them is required to round it correctly. A call evaluates one when its function does, or when an MA-type parameter selects an average that does. The functions that do are ACOS, ALMA, ASIN, ATAN, CHOP, CHOPTR, COS, COSH, EXP, FRAMA, HT_DCPERIOD, HT_DCPHASE, HT_PHASOR, HT_SINE, HT_TRENDLINE, HT_TRENDMODE, LINEARREG_ANGLE, LN, LOG10, MAMA, SIN, SINH, TAN and TANH, and the averages that do are `MAMA` and `ALMA`.

### Across machines {#machines}

A difference between two machines' math libraries can change the result of a call that evaluates a transcendental function, as it can between languages.

Functions are tested on several CPUs and compilers against the same golden values. The comparison is bit-exact or within a tolerance, depending on whether the algorithm is sensitive to how floating-point operations are ordered or fused (fused multiply-add, for example) and on whether it uses a transcendental function.

### Equivalent calls {#equivalent}

| Compared | Result | Rule |
|---|---|---|
| a stream and the batch call over the same bars | bit-identical, as rH1 defines | [rH1](/spec/streaming/#rh1) |
| `OpenAndFill` and the batch call over its history | bit-identical, as rH3 defines | [rH3](/spec/streaming/#rh3) |
| `Peek` and the next `Update` with the same bar | bit-identical | [rH4](/spec/streaming/#rh4) |
| a `Clone` and its original | the same value and range | [rH8](/spec/streaming/#rh8) |
| a default sentinel and the explicit default | bit-identical | [rP3](/spec/inputs-outputs/#rp3) |
| an output in place on its input, and in a separate buffer | bit-identical | [rW7](/spec/inputs-outputs/#rw7) |
| a declinable output declined, and supplied | the other outputs bit-identical | [rW5](/spec/inputs-outputs/#rw5) |
| a C `float` input, and the `double` call on its widened values | bit-identical | [rP4](/spec/inputs-outputs/#rp4) |
| the same call on two machines | as [across machines](/spec/versions/#machines) describes | |
| the same bar from batch calls with different `startIdx` | not bit-identical in general | [different starts](/spec/lookback/#start) |

## Releases {#releases}

<a id="rv1"></a>**rV1** Within one ABI generation N (the N of the soname `libta-lib.so.N`; the table below shows it on each platform), no function or callback signature, struct layout, typedef, enum value or `TA_` constant that a release shipped in the installed C headers is removed or changed, except that `TA_MATYPE_MAX` and `TA_FUNC_UNST_COUNT` follow their enums ([rV3](/spec/versions/#rv3)). A release that removes or changes one starts a new N; additions keep it. Deprecated names are part of that surface. rV1 covers declarations, not values. `TA_LIB_SOURCES_DIGEST` is outside it; it changes whenever the sources do. Rust, Java and C# have no ABI generation, and rV1 does not apply to them.

| Platform | Carrier | A program built against an earlier release with the same N |
|---|---|---|
| Linux | soname `libta-lib.so.N` | links and runs against a later one without rebuilding |
| macOS | install name `libta-lib.N.dylib` | links and runs against a later one without rebuilding |
| Windows | none: the DLL's name carries no N (`ta-lib.dll` under MSVC) | gets no signal at link or load time when N changes; rebuild against the headers of the DLL you ship |

<a id="rv2"></a>**rV2** No enum member is renumbered, in any language. A retired member keeps its number under a reserved name (`TA_FUNC_UNST_UNUSED_1` and the like) instead of being deleted. `TA_AllCandleSettings` is pinned at 11 and `TA_FUNC_UNST_ALL` at 65535; neither tracks the number of members. A Rust, Java or C# enum may omit C members; each member it has carries C's number. Reading the number: for `RetCode`, see the [hub](/spec/#failures); for the other enums, Rust `as i32`, Java `value()` on `FuncUnstId` and `ordinal()` on `MAType`, `RangeType` and `CandleSettingType`, C# an `(int)` cast.

<a id="rv3"></a>**rV3** A release may add functions, MA types, unstable-period ids, candle settings and return codes, and adding one starts no new N. `TA_MATYPE_MAX` and `TA_FUNC_UNST_COUNT` grow when a member is appended to their enum. Rust marks `RetCode`, `FuncUnstId`, `MAType`, `RangeType` and `CandleSettingType` `#[non_exhaustive]`, so a `match` on one needs a wildcard arm.
