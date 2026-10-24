---
title: Versions and Determinism
description: "On one machine the same call gives bit-identical results in C, Rust, Java and C#, except where it evaluates a transcendental math function: the equivalent calls, the flags a C build from source must keep, and what a release keeps and may add."
---

*Part of TA-Lib's [specification](/spec/), a reference for AI-agent-driven integration.*

The rules for when [the same call](/spec/versions/#determinism) gives the same bits, and for what a release keeps and may add. On one machine, a result validated in one language holds bit for bit in the others, except for a transcendental call ([rD1](/spec/versions/#rd1)).

## The caller must {#caller}

- <a id="tolerance"></a>**Compare with a tolerance** any call [across machines](/spec/versions/#machines), and a [transcendental call](/spec/versions/#transcendental) across languages or between a stream and a batch call.
- <a id="build-flags"></a>**When you build C from source, keep the project's floating-point flags.** [rD1](/spec/versions/#rd1) holds for C only when the compiler evaluates every `double` operation as written:

  | Condition | GCC and Clang | Build files in the source tree |
  |---|---|---|
  | no contraction of `a*b+c` into a fused multiply-add | `-ffp-contract=off` | CMake passes it to every compiler but MSVC and those taking MSVC's command line (clang-cl); autotools passes it when the compiler accepts it |
  | no value-changing optimization | no `-ffast-math` or `-Ofast`, nor any part of them but `-fno-math-errno` | none uses them |
  | no extended-precision intermediates | on 32-bit x86, `-msse2 -mfpmath=sse` | only CMake's i386 cross-build toolchain, `cmake/toolchain-linux-i386.cmake`; add it yourself to a native 32-bit x86 build |

  rD1 is not promised for C built with MSVC or clang-cl.
- <a id="enumerate"></a>**Enumerate, never hard-code a list.** A release may add what [rV3](/spec/versions/#rv3) lists. Enumerate functions through the [Abstract API](/spec/abstract/#provides), check a raw MA-type integer ([rP2](/spec/inputs-outputs/#rp2)), give every `switch` or `match` over a TA-Lib enum a default branch, and ignore an [unknown metadata flag](/spec/abstract/#unknown-flag).
- <a id="pin"></a>**Validate against the release you ship.** No rule covers output values from one release to the next, nor API compatibility of the Rust, Java and C# packages between releases beyond rV2 and rV3. Which versions and packages exist: [Install](/install/).

## Determinism {#determinism}

In this spec, **bit-identical** means the same return code, the same output range, and every output element with the same bits, a NaN matching any NaN. **The same call** means the same function, input values, `startIdx` and `endIdx`, optional parameters and settings.

<a id="rd1"></a>**rD1** On one machine, the same call gives bit-identical results in C, Rust, Java and C#. The exception is a call that evaluates a [transcendental function](/spec/versions/#transcendental).

<a id="rd2"></a>**rD2** Where rD1 or [rH1](/spec/streaming/#rh1) lets two results differ, TA-Lib targets a difference below 1e-9: absolute for a value below 1 in magnitude, relative above it. CI verifies it across a wide range of series. It is not a promise for every input: a degenerate one, such as a flat series, can exceed it.

<a id="rd3"></a>**rD3** A difference caused by the unstable period falls outside rD2's target: values a function computes for the same bar from [different starts](/spec/lookback/#start) can differ by far more than 1e-9. Handling the [unstable period](/api/unstable-period/) is the caller's choice and responsibility, and TA-Lib provides a solution: numerical [Auto-Stabilization](/api/unstable-period/#auto), whose counts and limits are [specified](/spec/auto-stabilization/).

### Transcendental functions {#transcendental}

A **transcendental function** here is `exp`, `log`, `log10`, or a trigonometric, inverse trigonometric or hyperbolic function. C and Rust take it from the platform's C math library, Java from the JVM, C# from the .NET runtime, and none of them is required to round it correctly. A platform can also provide two routines for one function, one for a single value and one for several values at a time, and the two need not round alike: where a batch call takes the vectorized routine (the run-time key [`vmath.transcendental`](/spec/abstract/#runtime-info) tells whether one is in use), its result can differ from a stream's, which evaluates one value at a time, and from another language's. A call evaluates one when its function does, or when an MA-type parameter selects an average that does. The functions that do carry the [metadata flag](/spec/abstract/#flags-numerical) `TA_FUNC_FLG_USES_TRANSCENDENTAL`, and the averages that do are `MAMA` and `ALMA`. A transcendental call's difference starts in the last bits of one math-library result and can grow through the function's arithmetic; the HT_* outputs that round a computed period to whole bars can differ outright on some inputs.

### Across machines {#machines}

No rule promises the same bits on two machines: a different CPU, compiler or math library can change a value, and a release may compute a function differently on one CPU family than on another. TA-Lib targets the same 1e-9 there ([rD2](/spec/versions/#rd2)).

Each release is checked on every supported platform against the regression suite's reference values, some compared exactly and some within a tolerance.

### Equivalent calls {#equivalent}

| Compared | Rule |
|---|---|
| a stream and the batch call over the same bars | [rH1](/spec/streaming/#rh1) |
| `OpenAndFill` and the batch call over its history | [rH3](/spec/streaming/#rh3) |
| `Peek` and the next `Update` with the same bar | [rH4](/spec/streaming/#rh4) |
| a `Clone` and its original | [rH8](/spec/streaming/#rh8) |
| a default sentinel and the explicit default | [rP3](/spec/inputs-outputs/#rp3) |
| an output in place on its input, and in a separate buffer | [rW6](/spec/inputs-outputs/#rw6) |
| a `float` input, and the `double` call on its widened values | [rP4](/spec/inputs-outputs/#rp4) |

## Releases {#releases}

<a id="rv1"></a>**rV1** Within one ABI generation N (the N of `libta-lib.so.N`; the table below shows it per platform), no function or callback signature, struct layout, typedef, enum value or `TA_` constant that a release shipped in the installed C headers is removed or changed, except the counts that grow with their enums ([rV3](/spec/versions/#rv3)). Deprecated names are included. An addition starts no new N. `TA_LIB_SOURCES_DIGEST` is outside rV1: it changes whenever the sources do.

| Platform | Carrier | A program built against an earlier release with the same N |
|---|---|---|
| Linux | soname `libta-lib.so.N` | links and runs against a later one without rebuilding |
| macOS | install name `libta-lib.N.dylib` | links and runs against a later one without rebuilding |
| Windows | none: the DLL's name carries no N (`ta-lib.dll` under MSVC) | gets no signal at link or load time when N changes; rebuild against the headers of the DLL you ship |

<a id="rv2"></a>**rV2** No member of `RetCode`, `MAType`, `FuncUnstId`, `RangeType` or `CandleSettingType` is renumbered, in any language. `TA_AllCandleSettings` is pinned at 11 and `TA_FUNC_UNST_ALL` at 65535; neither tracks the number of members. A Rust, Java or C# enum may omit C members; each member it has carries C's number. Reading the number: for `RetCode`, see the [hub](/spec/#failures); for the other enums, Rust `as i32`, Java `value()` on `FuncUnstId` and `ordinal()` on `MAType`, `RangeType` and `CandleSettingType`, valid while the enum lists every member in order; C# an `(int)` cast.

<a id="rv3"></a>**rV3** A release may add functions, MA types, unstable-period ids, candle settings, return codes, [metadata flags](/spec/abstract/#flags) and [run-time information keys](/spec/abstract/#runtime-info). `TA_MATYPE_MAX` and `TA_FUNC_UNST_COUNT` grow when a member is appended to their enum. Rust marks `RetCode`, `FuncUnstId`, `MAType`, `RangeType` and `CandleSettingType` `#[non_exhaustive]`.
