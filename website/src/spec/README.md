---
title: Specification
description: "TA-Lib's calling contract in C, Rust, Java, C# and TypeScript, for AI-agent-driven integration: what your code can rely on, in rules the test suite checks; what it must ensure; and how names and failures map across the five languages."
---

**TA-Lib's calling contract, stated rule by rule for a robust integration. The test suite checks every rule, so your code can rely on them.**

Written as a reference for AI agents integrating TA-Lib. For a lighter introduction written for people, start with the API pages for [C/C++](/api/), [Rust](/api/rust/), [Java](/api/java/), [C#](/api/csharp/) or [TypeScript](/api/typescript/).

An AI agent is expected to read the code for exact signatures and names. The specification states the invariants: behaviour an integrator can rely on across languages and, as the [release rules](/spec/versions/#releases) define, across releases.

The rules use C's spelling, and this page maps it onto the other languages. Every function's C, Rust, Java, C# and TypeScript code is generated from one definition, so its name converts by a fixed rule ([names](/spec/#names)) and its batch call takes its arguments in the same order in each language ([rP5](/spec/inputs-outputs/#rp5); TypeScript Caveat: [rJ2](/spec/#rj2)).

## Scope {#scope}

- The native APIs (C, Rust, Java, C#, TypeScript).
- A wrapper keeps its own conventions. ta-lib-python aligns outputs to the input and fills the warm-up with NaN; the native APIs do not ([rW3](/spec/inputs-outputs/#rw3)).
- Owned by other pages: [Install](/install/) (packages and versions), [Unstable Period](/api/unstable-period/), [Candlestick Settings](/api/candle-settings/) (model and defaults), streaming in [C](/api/stream/), [Rust](/api/rust/stream/), [Java](/api/java/stream/), [C#](/api/csharp/stream/) and [TypeScript](/api/typescript/stream/), [Numerical Stability](/functions/stability), and the [function pages](/functions/): inputs, outputs, parameters, stability category, flags.
- The pages other than the per-function pages are in [/llms-full.txt](/llms-full.txt); its [function index](/functions/) links each function page. [/llms.txt](/llms.txt) indexes every page. Each page has a Markdown twin; this page's is `/spec/index.md`.

## Rule pages {#reading}

In the order a caller meets them:

| Page | Covers | Rule families |
|---|---|---|
| [Batch Inputs and Outputs](/spec/inputs-outputs/) | what a call takes, what a successful call writes, pattern outputs, computing in place | `rP` parameters, `rW` writes |
| [Lookback and Shift](/spec/lookback/) | lookback call, what enters it, display shift, stability from metadata | `rL` lookback |
| [Auto-Stabilization](/spec/auto-stabilization/) | the bars an Auto level adds for each id, what a level targets, the known exceptions | `rZ` Auto-Stabilization |
| [Streaming](/spec/streaming/) | bit-identity with batch, opening, advancing, accessors | `rH` handle behaviour, `rS` stream opening, `rU` update |
| [Abstract API and Metadata](/spec/abstract/) | what the Abstract API provides, its rules and return codes, run-time information, the catalog of metadata flags | `rA` Abstract API |
| [Settings and Threads](/spec/settings-threads/) | C lifecycle and settings, `Core`, setting validation, threads | `rT` settings and threads |
| [Errors](/spec/errors/) | return codes, batch conditions | `rE` errors, `rB` batch conditions |
| [Versions and Determinism](/spec/versions/) | bit-identity across languages and equivalent calls; releases | `rD` determinism, `rV` versions |

TypeScript departs from a few rules, and each of those says so with a link to its [TypeScript Caveat](/spec/#typescript) below, family `rJ`.

Each page has two kinds of statement:

- **The caller must**: what the calling code has to ensure. Most of it is not checked; nothing is promised after a violation that goes unreported.
- **A rule**: what TA-Lib does. Its id is `r`, the letter of its family and a number, such as `rB5`, and its anchor is the id in lower case: [/spec/errors/#rb5](/spec/errors/#rb5). An id's meaning can change from one release to the next.

`<N>` stands for a function's [canonical name](/spec/#names): `TA_<N>_Open` is `TA_SMA_Open`.

## Names in each language {#names}

A function's canonical name is its name on [/functions/](/functions/). Every spelling derives from it:

| Style | Rule | SMA | HT_TRENDLINE | CDL3BLACKCROWS |
|---|---|---|---|---|
| C | `TA_` + canonical | `TA_SMA` | `TA_HT_TRENDLINE` | `TA_CDL3BLACKCROWS` |
| snake | lower case | `sma` | `ht_trendline` | `cdl3blackcrows` |
| UpperCamel | lower case, upper-case the first letter of each `_`-separated part, drop `_` | `Sma` | `HtTrendline` | `Cdl3blackcrows` |
| lowerCamel | UpperCamel, first letter lower case | `sma` | `htTrendline` | `cdl3blackcrows` |

Rust uses snake for functions, UpperCamel for types; Java lowerCamel for methods, UpperCamel for types; C# UpperCamel; TypeScript lowerCamel for functions, UpperCamel for types; a TypeScript batch function whose name is a reserved word takes a trailing `_` (`var_` for VAR, while `varLookback`, `varOpen` and `VarStream` keep the rule). The Abstract API uses the canonical name. Indicator parameters keep C's names everywhere (`startIdx`, `inReal`, `optInTimePeriod`, `outReal`). TypeScript Caveat: [rJ1](/spec/#rj1).

In Rust, Java and C#, a function's calls are methods of a `Core`. In TypeScript they are functions exported by the package, each taking a `Core` as the optional `core` key of its options:

| Surface | C | Rust | Java | C# | TypeScript |
|---|---|---|---|---|---|
| batch | `TA_SMA` | `sma` | `sma` | `Sma` | `sma` returns `{ begIdx, values }` |
| lookback | `int TA_SMA_Lookback` | `sma_lookback` returns `Result<usize, RetCode>` | `int smaLookback` | `int SmaLookback` | `smaLookback` returns `number` |
| display shift of one output | `int TA_SMA_DisplayShift` | `sma_display_shift` returns `Result<i32, RetCode>` | `int smaDisplayShift` | `int SmaDisplayShift` | `smaDisplayShift` returns `number` |
| open, open and fill | `TA_SMA_Open`, `TA_SMA_OpenAndFill` | `sma_open`, `sma_open_and_fill` | `smaOpen`, `smaOpenAndFill` | `SmaOpen`, `SmaOpenAndFill` | `smaOpen`; `smaOpenAndFill` returns `{ stream, begIdx, values }` |
| handle | `TA_SMA_Stream *` | `SmaStream` | `Core.SmaStream` | `Core.SmaStream` | `SmaStream` |
| handle calls | `TA_SMA_Update`, `_Peek`, `_Value`, `_OutRange`, `_Advance`, `_Clone`, `_Close` | `update`, `peek`, `value`, `out_range`, `advance`, `clone`; drop to release | `update`, `peek`, `value`, `outRange`, `advance`, `clone` | `Update`, `Peek`, `Advance`, `Clone`; properties `Value`, `OutRange` | `update`, `peek`, `value`, `outRange`, `advance`, `clone` |
| one bar of MACD | three out-pointers | `(f64, f64, f64)` | caller-owned `Core.MacdOut` sink | returned `Core.MacdValue` | returned `MacdOut`: the sink passed, or a new one |

| Type | C | Rust | Java | C# | TypeScript |
|---|---|---|---|---|---|
| import | `ta_libc.h` | crate `ta_lib` | `io.github.talib` | `TALib` | package `ta-lib` |
| settings: default, builder | process-wide ([settings](/spec/settings-threads/#idle-settings)) | `Core::new()`, `Core::builder()` | `Core.DEFAULT`, `Core.builder()` | `Core.Default`, `Core.Builder()` | `Core.DEFAULT`, `Core.builder()` |
| output range | `outBegIdx`, `outNBElement` | `OutRange { beg_idx, count }`, `EMPTY`, `is_empty()` | `OutRange(begIdx, count)`, `EMPTY`, `isEmpty()` | `OutRange(BegIdx, Count)`, `Empty`, `IsEmpty` | `OutRange(begIdx, count)`, `EMPTY`, `isEmpty()` |

Setters per language: [Unstable Period](/api/unstable-period/), [Candlestick Settings](/api/candle-settings/). C has no public candle-setting type: a setting is the arguments of `TA_SetCandleSettings`.

Enum types drop C's `TA_`. Members:

| Enum | C | Rust, C# | Java, TypeScript |
|---|---|---|---|
| `RetCode` | `TA_BAD_PARAM` | `BadParam` | `BAD_PARAM` |
| `CandleSettingType` | `TA_BodyLong` | `BodyLong` | `BODY_LONG` |
| `RangeType` | `TA_RangeType_RealBody` | `RealBody` | `REAL_BODY` |
| `MAType` | `TA_MAType_SMA` | `SMA` | `SMA` |
| `FuncUnstId` | `TA_FUNC_UNST_HT_DCPERIOD` | `HT_DCPERIOD` | `HT_DCPERIOD` |

Constants: C prefixes `TA_` (`TA_INDEX_MAX`); Rust, Java, C# and TypeScript hold them on `Core`, C# in UpperCamel (`Core.IndexMax`).

| Constant | Value |
|---|---|
| `INDEX_MAX` | 100000000, the largest index |
| `UNSTABLE_AUTO_PREC_4`, `UNSTABLE_AUTO_PREC_8` | `INDEX_MAX + 4`, `INDEX_MAX + 8`: the Auto levels ([rT3](/spec/settings-threads/#rt3), [rZ1](/spec/auto-stabilization/#rz1)) |
| `REAL_DEFAULT`, `INTEGER_DEFAULT` | -4e37, `INT_MIN`: the default sentinels ([rP3](/spec/inputs-outputs/#rp3)) |
| `REAL_MIN`, `REAL_MAX` | -3e37, 3e37 ([input domain](/spec/inputs-outputs/#input-domain)) |
| `LOOKBACK_INVALID`, `DISPLAY_SHIFT_INVALID` | -1, `INT_MIN`: a failed lookback and display shift ([failures](/spec/#failures)); TypeScript only |

### Metadata flags {#flag-names}

What each flag means: [flags](/spec/abstract/#flags).

| Flags of | C prefix | Rust, Java, TypeScript | C# |
|---|---|---|---|
| a function | `TA_FUNC_FLG_` | `FuncFlags` | `FuncFlags` |
| a price input | `TA_IN_PRICE_` | `InputFlags` | `PriceComponents` |
| an optional parameter | `TA_OPTIN_` | `OptInputFlags` | `OptInputFlags` |
| an output | `TA_OUT_` | `OutputFlags` | `OutputFlags` |

Members are upper case in Rust, Java and TypeScript and UpperCamel in C#, and can use other words than C's (`FuncFlags.STREAMING` in Java and TypeScript is `TA_FUNC_FLG_STREAM`): match a member to its C name by its bit value ([rA7](/spec/abstract/#ra7)). Each C# flag enum also has `None`: no flag set.

## How a failure reaches the caller {#failures}

| Success | C, returning `TA_SUCCESS` | Rust | Java | C# | TypeScript |
|---|---|---|---|---|---|
| batch | range in the out-pointers | `Ok(OutRange)` | `OutRange` | `OutRange` | `{ begIdx, values }`, or one array per output |
| open | handle in `*stream`, last value in the out-pointers | `Ok((handle, value))` | handle | handle | handle |
| open and fill | handle and range in out-pointers | `Ok((handle, OutRange))` | handle; range from `outRange()` | handle; range from `OutRange` | `{ stream, begIdx, values }`; range also from `outRange()` |
| update, peek | value in the out-pointers | `Ok(value)` | value, or written into the sink | value | value, or the outputs in the sink passed or a new one |

| Failure | C | Rust | Java | C# | TypeScript |
|---|---|---|---|---|---|
| reported as; its code | returned `TA_RetCode` | `Err(RetCode)` | an exception implementing `TALibFailure`; `retCode()` | an exception implementing `ITALibFailure`; `RetCode` | a thrown `TALibError`; `code`, C's name as a string (`'TA_BAD_PARAM'`) |
| C's number ([rV2](/spec/versions/#rv2)) | the value | `as_c_int()` | `asCInt()` | `(int)` cast | `retCode` |
| lookback ([rL2](/spec/lookback/#rl2)) | `-1` | `Err(RetCode::BadParam)` | `-1` | `-1` | `Core.LOOKBACK_INVALID` (-1) |
| display shift ([rL11](/spec/lookback/#rl11)) | `INT_MIN` | `Err(RetCode::BadParam)` | `Integer.MIN_VALUE` | `int.MinValue` | `Core.DISPLAY_SHIFT_INVALID` (`INT_MIN`) |

One row per [code a function call can report](/spec/errors/#return-codes). `A : B` means `A` extends the platform type `B`, so a `catch` of `B` works. TypeScript throws `TALibError : Error` for every code, its `code` being the code's C name.

| Code | Rust | Java | C# | TypeScript |
|---|---|---|---|---|
| `TA_BAD_PARAM` | `BadParam` | `TALibArgumentException : IllegalArgumentException` | `TALibArgumentException : ArgumentException` | `'TA_BAD_PARAM'` |
| `TA_OUT_OF_RANGE_START_INDEX` | `OutOfRangeStartIndex` | `TALibIndexException : IndexOutOfBoundsException` | `TALibArgumentOutOfRangeException : ArgumentOutOfRangeException` | `'TA_OUT_OF_RANGE_START_INDEX'` |
| `TA_OUT_OF_RANGE_END_INDEX` | `OutOfRangeEndIndex` | `TALibIndexException` | `TALibArgumentOutOfRangeException`; `TALibArgumentException` from `Update`, `Advance` ([rU4](/spec/streaming/#ru4)) | `'TA_OUT_OF_RANGE_END_INDEX'` |
| `TA_INSUFFICIENT_HISTORY` | `InsufficientHistory` | `InsufficientHistoryException : TALibArgumentException` | `InsufficientHistoryException : TALibArgumentException` | `'TA_INSUFFICIENT_HISTORY'` |
| `TA_INTERNAL_ERROR` ([rB8](/spec/errors/#rb8)) | `InternalError` | `TALibStateException : IllegalStateException` | `TALibInvalidOperationException : InvalidOperationException` | `'TA_INTERNAL_ERROR'` |
| `TA_ALLOC_ERR` | `AllocErr`, never returned ([rB7](/spec/errors/#rb7)) | never thrown | never thrown | never thrown |

Failed settings:

| Failure | C | Rust | Java | C# | TypeScript |
|---|---|---|---|---|---|
| setter ([rT2](/spec/settings-threads/#rt2)) | returns `TA_BAD_PARAM` | `build()` returns `Err(RetCode::BadParam)` ([rT8](/spec/settings-threads/#rt8)) | throws `IllegalArgumentException` (`NullPointerException` on null), no code | throws `ArgumentOutOfRangeException`, no code | throws `TALibError` `'TA_BAD_PARAM'` |
| getter ([rT4](/spec/settings-threads/#rt4)) | cannot fail | returns `Err(RetCode::BadParam)` | as the setter | as the setter | as the setter |

## TypeScript Caveats {#typescript}

Where the TypeScript API departs from a rule. Each amended rule links here.

| Rule | Statement |
|---|---|
| <a id="rj1"></a>**rJ1** | Amends [names](/spec/#names). An option key is C's parameter name without `optIn`, split into words at `_` and where C's case changes, an acronym being one word and an `MA` that ends one before `Type` a word of its own, then joined with the first word lower case and each later one capitalized: `timePeriod`, `maType`, `fastDMaType`, `rocMaType`. An output keeps `out`, drops C's type word and is joined the same way: `outMACDSignal` is `outMacdSignal`, `outRealUpperBand` is `outUpperBand`. A function with one output returns it as `values`, its buffer key being `outReal` or `outInteger`. Inputs are positional, and a positional input's parameter name drops `in` (`real`, `real0`, `periods`; a price input is `prices`) in the batch call and the openers; a stream's `update` and `peek` take one number per component, named by the price object's keys (`update(high, low, close)`); the components of a price input are one object keyed `open`, `high`, `low`, `close`, `volume`, `openInterest`. |
| <a id="rj2"></a>**rJ2** | Amends [rP5](/spec/inputs-outputs/#rp5). A batch call takes its inputs in the order of the Inputs list, then one optional object holding the optional parameters, `startIdx`, `endIdx` ([rP1](/spec/inputs-outputs/#rp1)), `core` and output buffers. The call returns `begIdx` and every output, each exactly as long as the count; an output passed as a buffer is returned as a view of it. |
| <a id="rj3"></a>**rJ3** | Amends [rB4](/spec/errors/#rb4) and [size every output](/spec/inputs-outputs/#output-size). An output buffer omitted or `undefined` is allocated, not rejected; `null` or an empty array is still absent, and a buffer passed is sized as in any other language. |
| <a id="rj4"></a>**rJ4** | Amends [lookback calls](/spec/lookback/#calls), [rL2](/spec/lookback/#rl2) and [rL11](/spec/lookback/#rl11). The lookback and display-shift calls take the batch call's options object; keys they do not use are ignored. Options that are not a plain object, or a `core` that is not a `Core`, throw `TALibError` `'TA_BAD_PARAM'`; a rejected parameter still returns the call's invalid constant. |
| <a id="rj5"></a>**rJ5** | Amends [display shift](/spec/lookback/#display-shift). The display-shift call takes the output index first, then the options object. |
| <a id="rj6"></a>**rJ6** | Amends [slots](/spec/abstract/#provides). A metadata row carries each input's, parameter's and output's typed-call key ([rJ1](/spec/#rj1)) beside C's name, and `setOptInput` takes a parameter's key as well as its position. |
