---
title: Specification
description: "The calling contract of TA-Lib's C, Rust, Java and C# APIs, stated rule by rule as a reference for AI-agent-driven integration: how a failure reaches the caller, how names fold, and one page per topic."
---

**TA-Lib's calling contract, stated rule by rule for a robust integration. Every rule is enforced by the test suite, so your code can rely on it.**

Written as a reference for AI agents integrating TA-Lib. For a lighter introduction written for people, start with the API pages for [C/C++](/api/), [Rust](/api/rust/), [Java](/api/java/) or [C#](/api/csharp/).

The rules cover the four native APIs (C, Rust, Java, C#). Each rule has one home page, states what the code does, and is written in C's spelling. This page maps that shared vocabulary onto Rust, Java and C#; a rule that introduces a language-specific name gives all four spellings itself.

## Scope {#scope}

- The four native APIs: batch, lookback, display shift, streams, settings, and the [Abstract API](/spec/abstract/).
- A wrapper keeps its own conventions. ta-lib-python aligns outputs to the input and fills the warm-up with NaN; the native APIs do not ([rW3](/spec/inputs-outputs/#rw3)).
- Published packages and their versions: [Install](/install/).
- Owned by other pages: [Unstable Period](/api/unstable-period/), [Candlestick Settings](/api/candle-settings/) (model and defaults), streaming in [C](/api/stream/), [Rust](/api/rust/stream/), [Java](/api/java/stream/) and [C#](/api/csharp/stream/), [Numerical Stability](/functions/stability), and the [function pages](/functions/): inputs, outputs in order, parameters (type, default, accepted values), stability category, flags.
- Every page except the per-function pages in one file: [/llms-full.txt](/llms-full.txt), whose [function index](/functions/) lists and links each function's page; [/llms.txt](/llms.txt) indexes them all. Each page has a Markdown twin, `/spec/index.md` for this one.

## Rule pages {#reading}

In the order a caller meets them:

| Page | Covers | Rule families |
|---|---|---|
| [Batch Inputs and Outputs](/spec/inputs-outputs/) | what a call takes, what a successful call writes, pattern outputs, computing in place | `rP` parameters, `rW` writes |
| [Lookback and Shift](/spec/lookback/) | lookback call, what enters it, display shift, stability from metadata | `rL` lookback |
| [Streaming](/spec/streaming/) | bit-identity with batch, opening, advancing, accessors | `rH` handle behaviour, `rS` stream opening, `rU` update |
| [Abstract API and Metadata](/spec/abstract/) | what the Abstract API provides, its rules and return codes, the catalog of metadata flags | `rA` Abstract API |
| [Settings and Threads](/spec/settings-threads/) | C lifecycle and settings, `Core`, setting validation, threads | `rT` settings and threads |
| [Errors](/spec/errors/) | return codes, batch conditions | `rE` errors, `rB` batch conditions |
| [Versions and Determinism](/spec/versions/) | bit-identity across languages and equivalent calls; releases | `rD` determinism, `rV` versions |

Each page has two kinds of statement:

- **The caller must**: what the calling code has to ensure. Most of it is not checked by the library, so it comes first on every page, and the next section collects the items that fail silently.
- **A rule**: what TA-Lib does. Every rule is enforced by the test suite. Its id is `r`, the letter of its family and a number, such as `rB5`, and its anchor is the id in lower case: [/spec/errors/#rb5](/spec/errors/#rb5). An id's meaning can change from one release to the next.

`<N>` stands for a function's canonical name: `TA_<N>_Open` is `TA_SMA_Open`.

## What is not detected {#not-detected}

None of these is reported. The caller avoids them, or treats what follows as undefined.

- [A buffer too short, in C](/spec/inputs-outputs/#input-length): read or written past its end. Rust, Java and C# reject it.
- [A buffer bound to a C parameter holder](/spec/abstract/#c-buffers) and no longer valid when the holder is called.
- [A C parameter holder or table](/spec/abstract/#c-release) never released, or released twice.
- [NaN or infinity inside an input series](/spec/inputs-outputs/#finite-inputs), or inside the history a stream opens on.
- [A real input outside ±3e37](/spec/inputs-outputs/#input-domain).
- [Intermediate overflow](/spec/inputs-outputs/#overflow) on finite input.
- [Buffers that partially overlap](/spec/inputs-outputs/#no-overlap), in C.
- [C used before `TA_Initialize`](/spec/settings-threads/#initialize) or after `TA_Shutdown`, or `TA_Initialize` called twice.
- [A C setting changed](/spec/settings-threads/#idle-settings) while a TA function is running or a stream is open.
- [A writer and any other call at once on one stream handle](/spec/settings-threads/#one-writer), or two threads on one parameter holder or builder (in Rust, the compiler rejects it).
- [C's unstable-period getter](/spec/settings-threads/#rt4) given a wildcard or unknown id: it returns 0.
- [The state after an allocation failure](/spec/errors/#stop-on-alloc).

## Names in each language {#names}

A function's canonical name is its name on [/functions/](/functions/). A fold changes only case and `_`; it adds no word boundary inside a segment:

| Fold | Rule | SMA | HT_TRENDLINE | CDL3BLACKCROWS |
|---|---|---|---|---|
| C | `TA_` + canonical | `TA_SMA` | `TA_HT_TRENDLINE` | `TA_CDL3BLACKCROWS` |
| snake | lower case | `sma` | `ht_trendline` | `cdl3blackcrows` |
| UpperCamel | lower case, upper-case each `_` segment's first letter, drop `_` | `Sma` | `HtTrendline` | `Cdl3blackcrows` |
| lowerCamel | UpperCamel, first letter lower case | `sma` | `htTrendline` | `cdl3blackcrows` |

Rust uses snake for functions, UpperCamel for types; Java lowerCamel for methods, UpperCamel for types; C# UpperCamel. Messages and the Abstract API use the canonical name. Indicator parameters keep C's names everywhere (`startIdx`, `inReal`, `optInTimePeriod`, `outReal`).

Every function has this surface; in Rust, Java and C# the calls are methods of a `Core`:

| Surface | C | Rust | Java | C# |
|---|---|---|---|---|
| batch | `TA_SMA` | `sma` | `sma` | `Sma` |
| lookback | `int TA_SMA_Lookback` | `sma_lookback` returns `Result<usize, RetCode>` | `int smaLookback` | `int SmaLookback` |
| display shift of one output | `int TA_SMA_DisplayShift` | `sma_display_shift` returns `Result<i32, RetCode>` | `int smaDisplayShift` | `int SmaDisplayShift` |
| open, open and fill | `TA_SMA_Open`, `TA_SMA_OpenAndFill` | `sma_open`, `sma_open_and_fill` | `smaOpen`, `smaOpenAndFill` | `SmaOpen`, `SmaOpenAndFill` |
| handle | `TA_SMA_Stream *` | `SmaStream` | `Core.SmaStream` | `Core.SmaStream` |
| handle calls | `TA_SMA_Update`, `_Peek`, `_Value`, `_OutRange`, `_Advance`, `_Clone`, `_Close` | `update`, `peek`, `value`, `out_range`, `advance`, `clone`; drop to release | `update`, `peek`, `value`, `outRange`, `advance`, `clone` | `Update`, `Peek`, `Advance`, `Clone`; properties `Value`, `OutRange` |
| one bar of MACD | three out-pointers | `(f64, f64, f64)` | caller-owned `Core.MacdOut` sink | returned `Core.MacdValue` |

| Argument | C | Rust | Java | C# |
|---|---|---|---|---|
| index | `int` | `usize` | `int` | `int` |
| real input, output | `const double[]`, `double[]` | `&[f64]`, `&mut [f64]` | `double[]` | `ReadOnlySpan<double>`, `Span<double>` |
| integer output | `int[]` | `&mut [i32]` | `int[]` | `Span<int>` |
| integer, real parameter | `int`, `double` | `i32`, `f64` | `int`, `double` | `int`, `double` |
| MA-type parameter | `TA_MAType` | `MAType` | `MAType` | `MAType` |
| absent ([rB4](/spec/errors/#rb4)) | `NULL` | an empty slice | `null` or an empty array | an empty span, which a `null` array becomes |

| Type | C | Rust | Java | C# |
|---|---|---|---|---|
| import | `ta_libc.h` | crate `ta_lib` | `io.github.talib` | `TALib` |
| settings: default, builder | process-wide ([settings](/spec/settings-threads/#idle-settings)) | `Core::new()`, `Core::builder()` | `Core.DEFAULT`, `Core.builder()` | `Core.Default`, `Core.Builder()` |
| output range | `outBegIdx`, `outNBElement` | `OutRange { beg_idx, count }`, `EMPTY`, `is_empty()` | `OutRange(begIdx, count)`, `EMPTY`, `isEmpty()` | `OutRange(BegIdx, Count)`, `Empty`, `IsEmpty` |

Setters per language: [Unstable Period](/api/unstable-period/), [Candlestick Settings](/api/candle-settings/). C has no public candle-setting type: a setting is the four arguments of `TA_SetCandleSettings`.

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
| `REAL_DEFAULT` | -4e37, selects a real parameter's default ([rP3](/spec/inputs-outputs/#rp3)) |
| `INTEGER_DEFAULT` | `INT_MIN`, selects an integer parameter's default |
| `REAL_MIN`, `REAL_MAX` | -3e37, 3e37 ([input domain](/spec/inputs-outputs/#input-domain)) |

## How a failure reaches the caller {#failures}

| Success | C, returning `TA_SUCCESS` | Rust | Java | C# |
|---|---|---|---|---|
| batch | range in `*outBegIdx`, `*outNBElement` | `Ok(OutRange)` | `OutRange` | `OutRange` |
| open | handle in `*stream`, last value in the out-pointers | `Ok((handle, value))` | handle | handle |
| open and fill | handle and range in out-pointers | `Ok((handle, OutRange))` | handle; range from `outRange()` | handle; range from `OutRange` |
| update, peek | value in the out-pointers | `Ok(value)` | value, or written into the sink | value |

| Failure | C | Rust | Java | C# |
|---|---|---|---|---|
| carrier, code | returned `TA_RetCode` | `Err(RetCode)` | an exception implementing `TALibFailure`; `retCode()` | an exception implementing `ITALibFailure`; `RetCode` |
| C's number | the value | `as_c_int()` | `asCInt()` | `(int)` cast |
| lookback ([rL2](/spec/lookback/#rl2)) | `-1` | `Err(RetCode::BadParam)` | `-1` | `-1` |
| display shift ([rL11](/spec/lookback/#rl11)) | `INT_MIN` | `Err(RetCode::BadParam)` | `Integer.MIN_VALUE` | `int.MinValue` |

One row per code a function call can report; `A : B` means `A` extends the platform type `B`, so a `catch` of `B` works.

| Code | Rust | Java | C# |
|---|---|---|---|
| `TA_BAD_PARAM` (2) | `BadParam` | `TALibArgumentException : IllegalArgumentException` | `TALibArgumentException : ArgumentException` |
| `TA_OUT_OF_RANGE_START_INDEX` (12) | `OutOfRangeStartIndex` | `TALibIndexException : IndexOutOfBoundsException` | `TALibArgumentOutOfRangeException : ArgumentOutOfRangeException` |
| `TA_OUT_OF_RANGE_END_INDEX` (13) | `OutOfRangeEndIndex` | `TALibIndexException` | `TALibArgumentOutOfRangeException`; `TALibArgumentException` from `Update`, `Advance` ([rU4](/spec/streaming/#ru4)) |
| `TA_INSUFFICIENT_HISTORY` (17) | `InsufficientHistory` | `InsufficientHistoryException : TALibArgumentException` | `InsufficientHistoryException : TALibArgumentException` |
| `TA_INTERNAL_ERROR` (5000, [rB9](/spec/errors/#rb9)) | `InternalError` | `TALibStateException : IllegalStateException` | `TALibInvalidOperationException : InvalidOperationException` |
| `TA_ALLOC_ERR` (3) | `AllocErr`, never returned ([rB8](/spec/errors/#rb8)) | never thrown | never thrown |

Numbering: [rV2](/spec/versions/#rv2).

A refused setting:

| Refusal | C | Rust | Java | C# |
|---|---|---|---|---|
| setter ([rT2](/spec/settings-threads/#rt2)) | returns `TA_BAD_PARAM` | `build()` returns `Err(RetCode::BadParam)` ([rT8](/spec/settings-threads/#rt8)) | throws `IllegalArgumentException` (`NullPointerException` on null), no code | throws `ArgumentOutOfRangeException`, no code |
| getter ([rT4](/spec/settings-threads/#rt4)) | cannot refuse | returns `Err(RetCode::BadParam)` | as the setter | as the setter |
