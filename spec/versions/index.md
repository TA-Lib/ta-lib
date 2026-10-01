---
url: 'https://ta-lib.org/spec/versions/index.md'
description: >-
  Which TA-Lib results are bit-identical across languages, machines, C builds
  and equivalent calls, and what a release keeps and may add.
---
# Versions and Determinism

*Part of TA-Lib's exhaustive [specifications](/spec/), intended for precise AI-agent-driven integration with TA-Lib, to minimize errors.*

D1 to D4 say where the same call gives the same bits across languages, machines and C builds; the table below also indexes the equivalences other pages own. V1 to V4 say what a release keeps and what it may add.

## Determinism {#determinism}

On this page, **bit-identical** means the same return code, the same output range, and every output element with the same bits, except that a NaN matches any NaN: no math library specifies a NaN's payload. **The same call** means the same function, input values, `startIdx` and `endIdx`, optional parameters and settings. **The same settings** means the same unstable periods and candle settings; in C that includes having called `TA_Initialize` ([T1](/spec/settings-threads/#t1)).

| Compared | Result | Rule |
|---|---|---|
| the same call in C and Rust, one machine | bit-identical | [D1](/spec/versions/#d1) |
| the same call in Java or C# and in C, one machine | bit-identical, except a call that evaluates a transcendental function | [D2](/spec/versions/#d2) |
| the same call in one backend on two machines | bit-identical, except a call that evaluates a transcendental function | [D3](/spec/versions/#d3) |
| C built from source | bit-identical only under D4's conditions | [D4](/spec/versions/#d4) |
| the same bar from batch calls with different `startIdx` | not bit-identical in general | [Lookback](/spec/lookback/#start) |
| a stream and the batch call over the same bars | bit-identical but for a zero's sign, under H1's conditions | [H1](/spec/streaming/#h1) |
| `OpenAndFill` and the batch call over its history | bit-identical but for a zero's sign | [H3](/spec/streaming/#h3) |
| `Peek` and the next `Update` with the same bar | bit-identical | [N7](/spec/streaming/#n7) |
| a `Clone` and its original | the same state, value and range | [H7](/spec/streaming/#h7) |
| a default sentinel and the explicit default | bit-identical | [N3](/spec/inputs-outputs/#n3) |
| an output in place on its input, and in a separate buffer | bit-identical | [N4](/spec/inputs-outputs/#n4) |
| a declinable output declined, and supplied | the other outputs bit-identical | [O5](/spec/inputs-outputs/#o5) |
| a `float` input, and the `double` call on its widened values | bit-identical | [I6](/spec/inputs-outputs/#i6) |

### Transcendental functions {#transcendental}

A **transcendental function** here is `exp`, `log`, `log10`, or a trigonometric, inverse trigonometric or hyperbolic function. C and Rust take it from the platform's C math library, Java from the JVM, C# from the .NET runtime, and none of them is required to round it correctly. A call evaluates one when its function does, or when an MA-type parameter selects an average that does. **Current behaviour**: the functions that do are ACOS, ALMA, ASIN, ATAN, CHOP, CHOPTR, COS, COSH, EXP, FRAMA, HT_DCPERIOD, HT_DCPHASE, HT_PHASOR, HT_SINE, HT_TRENDLINE, HT_TRENDMODE, LINEARREG_ANGLE, LN, LOG10, MAMA, SIN, SINH, TAN and TANH, and the averages that do are `MAMA` and `ALMA`.

Where D2 or D3 lets such a call differ, the difference starts in the last bit of one result and can grow through the function's arithmetic. It can also jump: HT_DCPHASE, HT_SINE, HT_TRENDLINE and HT_TRENDMODE round a period computed with `atan` to the nearest whole number of bars, and on a constant series HT_DCPHASE and HT_SINE take `atan` of a ratio of rounding residues, where a last-bit difference can move the phase by whole degrees. Compare these calls with a tolerance, and expect the HT\_\* outputs to differ outright on some inputs.

<a id="d1"></a>**D1** On one machine, C built as [D4](/spec/versions/#d4) requires and Rust are bit-identical, transcendental functions included.

<a id="d2"></a>**D2** On one machine, Java and C# are bit-identical to C built as [D4](/spec/versions/#d4) requires, for every call that evaluates no transcendental function. A call that evaluates one may differ from C's; whether it does depends on the runtime and the host.

<a id="d3"></a>**D3** Between machines (operating system, math library, CPU), a call that evaluates no transcendental function is bit-identical in every backend, C built as [D4](/spec/versions/#d4) requires. Every fused multiply-add is explicit in the source (C `fma`, Rust `mul_add`, Java `Math.fma`, C# `Math.FusedMultiplyAdd`), so a CPU with or without an FMA unit gives the same bits. A call that evaluates a transcendental function may differ between machines.

<a id="d4"></a>**D4** The C sources give [D1](/spec/versions/#d1) and [D3](/spec/versions/#d3) only when the compiler evaluates every `double` operation as written. Stated for GCC and Clang:

| Condition | GCC and Clang | Build files in the source tree |
|---|---|---|
| no contraction of `a*b+c` into a fused multiply-add | `-ffp-contract=off` | CMake passes it to every compiler but MSVC and those taking MSVC's command line (clang-cl); autotools passes it when the compiler accepts it |
| no value-changing optimization | no `-ffast-math` or `-Ofast`, nor any part of them but `-fno-math-errno` | none uses them |
| no extended-precision intermediates | on 32-bit x86, `-msse2 -mfpmath=sse` | only CMake's i386 cross-build toolchain, `cmake/toolchain-linux-i386.cmake`; a native 32-bit x86 build with CMake or autotools does not pass it, so add it yourself |

Neither the optimization level, `-march`, nor `-fno-math-errno` (which CMake and autotools also pass to a compiler that accepts it) changes a value. Under MSVC, or clang-cl, CMake passes no floating-point option, so the compiler's defaults apply; D4 names no condition for them, and D1 and D3 are not promised for C built with them. **Current behaviour**: no `/arch:AVX2` is passed there, so the compiler has no FMA instruction to contract into. D4 concerns C only.

## Releases {#releases}

<a id="v1"></a>**V1** Within one ABI generation N (the N of the soname `libta-lib.so.N`; the table below shows it on each platform), no function or callback signature, struct layout, typedef, enum value or `TA_` constant that a release shipped in the installed C headers is removed or changed, except that `TA_MATYPE_MAX` and `TA_FUNC_UNST_COUNT` follow their enums ([V3](/spec/versions/#v3)). A release that removes or changes one starts a new N; additions keep it. Deprecated names are part of that surface and keep compiling and linking. V1 covers declarations, not values ([not covered](/spec/versions/#not-covered)). `TA_LIB_SOURCES_DIGEST` is outside V1; it changes whenever the sources do. Rust, Java and C# have no ABI generation, and V1 does not apply to them.

| Platform | Carrier | A program built against an earlier release with the same N |
|---|---|---|
| Linux | soname `libta-lib.so.N` | links and runs against a later one without rebuilding |
| macOS | install name `libta-lib.N.dylib` | links and runs against a later one without rebuilding |
| Windows | none: the DLL's name carries no N (`ta-lib.dll` under MSVC) | gets no signal at link or load time when N changes; rebuild against the headers of the DLL you ship |

<a id="v2"></a>**V2** No enum member is renumbered, in any backend. A retired member keeps its number under a reserved name (`TA_FUNC_UNST_UNUSED_1` and the like) instead of being deleted. `TA_AllCandleSettings` is pinned at 11 and `TA_FUNC_UNST_ALL` at 65535; neither tracks the number of members. A Rust, Java or C# enum may omit C members; each member it has carries C's number. Reading the number: for `RetCode`, see the [hub](/spec/#failures); for the other enums, Rust `as i32`, Java `value()` on `FuncUnstId` and `ordinal()` on `MAType`, `RangeType` and `CandleSettingType`, C# an `(int)` cast.

**Current behaviour**: new members have been appended. A reserved `UNUSED_n` slot is documented as reusable, so a later release may give its number to a new member; in C that removes the reserved name, which V1 counts as a change, so it comes with a new N.

<a id="v3"></a>**V3** A release may add functions, MA types, unstable-period ids, candle settings and return codes. Enumerate functions through the abstraction layer (`TA_ForEachFunc` in C; each language's catalog is in the hub's [abstraction layer](/spec/#abstraction) table), never from a list fixed when your code was written. `TA_MATYPE_MAX` and `TA_FUNC_UNST_COUNT` grow when a member is appended to their enum; how to bound an MA-type value is [I3](/spec/inputs-outputs/#i3). Rust marks `RetCode`, `FuncUnstId`, `MAType`, `RangeType` and `CandleSettingType` `#[non_exhaustive]`, so a `match` on one needs a wildcard arm; give a Java or C# `switch` over them a default branch.

<a id="v4"></a>**V4** In the function tier, the id a C internal error carries ([B8](/spec/errors/#b8)) is never reassigned: a later release keeps it on the same guard or retires it, so a reported number identifies the guard whichever release produced it.

### Not covered {#not-covered}

* **Output values from one release to the next.** No rule is published, and a release can change a function's values behind an unchanged declaration. Validate against the release you ship.
* **API compatibility of the Rust, Java and C# packages between releases**, beyond V2 and V3. No rule is published.
* **Which versions and packages exist**: [Install](/install/).
