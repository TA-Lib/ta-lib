---
url: 'https://ta-lib.org/spec/index.md'
description: >-
  TA-Lib's calling contract in C, Rust, Java and C#, for AI-agent-driven
  integration: what your code can rely on, in rules the test suite checks; what
  it must ensure; and how names and failures map across the four languages.
---
# Specification

**TA-Lib's calling contract, stated rule by rule for a robust integration. The test suite checks every rule, so your code can rely on them.**

Written as a reference for AI agents integrating TA-Lib. For a lighter introduction written for people, start with the API pages for [C/C++](/api/), [Rust](/api/rust/), [Java](/api/java/) or [C#](/api/csharp/).

An AI agent is expected to read the code for exact signatures and names. The specification states the invariants: behaviour an integrator can rely on across languages and, as the [release rules](/spec/versions/#releases) define, across releases.

The rules use C's spelling, and this page maps it onto the other languages. Every function's C, Rust, Java and C# code is generated from one definition, so its name converts by a fixed rule ([names](/spec/#names)) and its batch call takes its arguments in the same order in each language ([rP5](/spec/inputs-outputs/#rp5)).

## Scope {#scope}

* The native APIs (C, Rust, Java, C#).
* A wrapper keeps its own conventions. ta-lib-python aligns outputs to the input and fills the warm-up with NaN; the native APIs do not ([rW3](/spec/inputs-outputs/#rw3)).
* Owned by other pages: [Install](/install/) (packages and versions), [Unstable Period](/api/unstable-period/), [Candlestick Settings](/api/candle-settings/) (model and defaults), streaming in [C](/api/stream/), [Rust](/api/rust/stream/), [Java](/api/java/stream/) and [C#](/api/csharp/stream/), [Numerical Stability](/functions/stability), and the [function pages](/functions/): inputs, outputs, parameters, stability category, flags.
* The pages other than the per-function pages are in [/llms-full.txt](/llms-full.txt); its [function index](/functions/) links each function page. [/llms.txt](/llms.txt) indexes every page. Each page has a Markdown twin; this page's is `/spec/index.md`.

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

Each page has two kinds of statement:

* **The caller must**: what the calling code has to ensure. Most of it is not checked; nothing is promised after a violation that goes unreported.
* **A rule**: what TA-Lib does. Its id is `r`, the letter of its family and a number, such as `rB5`, and its anchor is the id in lower case: [/spec/errors/#rb5](/spec/errors/#rb5). An id's meaning can change from one release to the next.

`<N>` stands for a function's [canonical name](/spec/#names): `TA_<N>_Open` is `TA_SMA_Open`.

## Names in each language {#names}

A function's canonical name is its name on [/functions/](/functions/). Every spelling derives from it:

| Style | Rule | SMA | HT_TRENDLINE | CDL3BLACKCROWS |
|---|---|---|---|---|
| C | `TA_` + canonical | `TA_SMA` | `TA_HT_TRENDLINE` | `TA_CDL3BLACKCROWS` |
| snake | lower case | `sma` | `ht_trendline` | `cdl3blackcrows` |
| UpperCamel | lower case, upper-case the first letter of each `_`-separated part, drop `_` | `Sma` | `HtTrendline` | `Cdl3blackcrows` |
| lowerCamel | UpperCamel, first letter lower case | `sma` | `htTrendline` | `cdl3blackcrows` |

Rust uses snake for functions, UpperCamel for types; Java lowerCamel for methods, UpperCamel for types; C# UpperCamel. The Abstract API uses the canonical name. Indicator parameters keep C's names everywhere (`startIdx`, `inReal`, `optInTimePeriod`, `outReal`).

In Rust, Java and C#, a function's calls are methods of a `Core`:

| Surface | C | Rust | Java | C# |
|---|---|---|---|---|
| batch | `TA_SMA` | `sma` | `sma` | `Sma` |
| lookback | `int TA_SMA_Lookback` | `sma_lookback` returns `Result<usize, RetCode>` | `int smaLookback` | `int SmaLookback` |
| display shift of one output | `int TA_SMA_DisplayShift` | `sma_display_shift` returns `Result<i32, RetCode>` | `int smaDisplayShift` | `int SmaDisplayShift` |
| open, open and fill | `TA_SMA_Open`, `TA_SMA_OpenAndFill` | `sma_open`, `sma_open_and_fill` | `smaOpen`, `smaOpenAndFill` | `SmaOpen`, `SmaOpenAndFill` |
| handle | `TA_SMA_Stream *` | `SmaStream` | `Core.SmaStream` | `Core.SmaStream` |
| handle calls | `TA_SMA_Update`, `_Peek`, `_Value`, `_OutRange`, `_Advance`, `_Clone`, `_Close` | `update`, `peek`, `value`, `out_range`, `advance`, `clone`; drop to release | `update`, `peek`, `value`, `outRange`, `advance`, `clone` | `Update`, `Peek`, `Advance`, `Clone`; properties `Value`, `OutRange` |
| one bar of MACD | three out-pointers | `(f64, f64, f64)` | caller-owned `Core.MacdOut` sink | returned `Core.MacdValue` |

| Type | C | Rust | Java | C# |
|---|---|---|---|---|
| import | `ta_libc.h` | crate `ta_lib` | `io.github.talib` | `TALib` |
| settings: default, builder | process-wide ([settings](/spec/settings-threads/#idle-settings)) | `Core::new()`, `Core::builder()` | `Core.DEFAULT`, `Core.builder()` | `Core.Default`, `Core.Builder()` |
| output range | `outBegIdx`, `outNBElement` | `OutRange { beg_idx, count }`, `EMPTY`, `is_empty()` | `OutRange(begIdx, count)`, `EMPTY`, `isEmpty()` | `OutRange(BegIdx, Count)`, `Empty`, `IsEmpty` |

Setters per language: [Unstable Period](/api/unstable-period/), [Candlestick Settings](/api/candle-settings/). C has no public candle-setting type: a setting is the arguments of `TA_SetCandleSettings`.

Enum types drop C's `TA_`. Members:

| Enum | C | Rust, C# | Java |
|---|---|---|---|
| `RetCode` | `TA_BAD_PARAM` | `BadParam` | `BAD_PARAM` |
| `CandleSettingType` | `TA_BodyLong` | `BodyLong` | `BODY_LONG` |
| `RangeType` | `TA_RangeType_RealBody` | `RealBody` | `REAL_BODY` |
| `MAType` | `TA_MAType_SMA` | `SMA` | `SMA` |
| `FuncUnstId` | `TA_FUNC_UNST_HT_DCPERIOD` | `HT_DCPERIOD` | `HT_DCPERIOD` |

Constants: C prefixes `TA_` (`TA_INDEX_MAX`); Rust, Java and C# hold them on `Core`, C# in UpperCamel (`Core.IndexMax`).

| Constant | Value |
|---|---|
| `INDEX_MAX` | 100000000, the largest index |
| `UNSTABLE_AUTO_PREC_4`, `UNSTABLE_AUTO_PREC_8` | `INDEX_MAX + 4`, `INDEX_MAX + 8`: the Auto levels ([rT3](/spec/settings-threads/#rt3), [rZ1](/spec/auto-stabilization/#rz1)) |
| `REAL_DEFAULT`, `INTEGER_DEFAULT` | -4e37, `INT_MIN`: the default sentinels ([rP3](/spec/inputs-outputs/#rp3)) |
| `REAL_MIN`, `REAL_MAX` | -3e37, 3e37 ([input domain](/spec/inputs-outputs/#input-domain)) |

### Metadata flags {#flag-names}

What each flag means: [flags](/spec/abstract/#flags).

| Flags of | C prefix | Rust, Java | C# |
|---|---|---|---|
| a function | `TA_FUNC_FLG_` | `FuncFlags` | `FuncFlags` |
| a price input | `TA_IN_PRICE_` | `InputFlags` | `PriceComponents` |
| an optional parameter | `TA_OPTIN_` | `OptInputFlags` | `OptInputFlags` |
| an output | `TA_OUT_` | `OutputFlags` | `OutputFlags` |

Members are upper case in Rust and Java and UpperCamel in C#, and can use other words than C's (Java `FuncFlags.STREAMING` is `TA_FUNC_FLG_STREAM`): match a member to its C name by its bit value ([rA7](/spec/abstract/#ra7)). Each C# flag enum also has `None`: no flag set.

## How a failure reaches the caller {#failures}

| Success | C, returning `TA_SUCCESS` | Rust | Java | C# |
|---|---|---|---|---|
| batch | range in the out-pointers | `Ok(OutRange)` | `OutRange` | `OutRange` |
| open | handle in `*stream`, last value in the out-pointers | `Ok((handle, value))` | handle | handle |
| open and fill | handle and range in out-pointers | `Ok((handle, OutRange))` | handle; range from `outRange()` | handle; range from `OutRange` |
| update, peek | value in the out-pointers | `Ok(value)` | value, or written into the sink | value |

| Failure | C | Rust | Java | C# |
|---|---|---|---|---|
| reported as; its code | returned `TA_RetCode` | `Err(RetCode)` | an exception implementing `TALibFailure`; `retCode()` | an exception implementing `ITALibFailure`; `RetCode` |
| C's number ([rV2](/spec/versions/#rv2)) | the value | `as_c_int()` | `asCInt()` | `(int)` cast |
| lookback ([rL2](/spec/lookback/#rl2)) | `-1` | `Err(RetCode::BadParam)` | `-1` | `-1` |
| display shift ([rL11](/spec/lookback/#rl11)) | `INT_MIN` | `Err(RetCode::BadParam)` | `Integer.MIN_VALUE` | `int.MinValue` |

One row per [code a function call can report](/spec/errors/#return-codes). `A : B` means `A` extends the platform type `B`, so a `catch` of `B` works.

| Code | Rust | Java | C# |
|---|---|---|---|
| `TA_BAD_PARAM` | `BadParam` | `TALibArgumentException : IllegalArgumentException` | `TALibArgumentException : ArgumentException` |
| `TA_OUT_OF_RANGE_START_INDEX` | `OutOfRangeStartIndex` | `TALibIndexException : IndexOutOfBoundsException` | `TALibArgumentOutOfRangeException : ArgumentOutOfRangeException` |
| `TA_OUT_OF_RANGE_END_INDEX` | `OutOfRangeEndIndex` | `TALibIndexException` | `TALibArgumentOutOfRangeException`; `TALibArgumentException` from `Update`, `Advance` ([rU4](/spec/streaming/#ru4)) |
| `TA_INSUFFICIENT_HISTORY` | `InsufficientHistory` | `InsufficientHistoryException : TALibArgumentException` | `InsufficientHistoryException : TALibArgumentException` |
| `TA_INTERNAL_ERROR` ([rB8](/spec/errors/#rb8)) | `InternalError` | `TALibStateException : IllegalStateException` | `TALibInvalidOperationException : InvalidOperationException` |
| `TA_ALLOC_ERR` | `AllocErr`, never returned ([rB7](/spec/errors/#rb7)) | never thrown | never thrown |

Failed settings:

| Failure | C | Rust | Java | C# |
|---|---|---|---|---|
| setter ([rT2](/spec/settings-threads/#rt2)) | returns `TA_BAD_PARAM` | `build()` returns `Err(RetCode::BadParam)` ([rT8](/spec/settings-threads/#rt8)) | throws `IllegalArgumentException` (`NullPointerException` on null), no code | throws `ArgumentOutOfRangeException`, no code |
| getter ([rT4](/spec/settings-threads/#rt4)) | cannot fail | returns `Err(RetCode::BadParam)` | as the setter | as the setter |
