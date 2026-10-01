---
title: Specification
description: "The exhaustive contract of TA-Lib's C, Rust, Java and C# APIs, for precise AI-agent-driven integration: how a failure reaches the caller, how names fold, and one page per topic."
---

**These are TA-Lib's exhaustive specifications, intended for precise AI-agent-driven integration with TA-Lib, to minimize errors.** They cover the four native APIs (C, Rust, Java, C#). Each rule has one home page, states what the code does, and is written in C's spelling.

This page maps that shared vocabulary onto Rust, Java and C#; a rule that introduces a language-specific name gives all four spellings itself. For a first contact, start with the Core API page of your language: [C/C++](/api/), [Rust](/api/rust/), [Java](/api/java/), [C#](/api/csharp/).

## Scope {#scope}

- The four native APIs: batch, lookback, streams, settings, and the abstraction layer, which is specified only in part ([M1](/spec/errors/#m1)).
- A wrapper keeps its own conventions. ta-lib-python aligns outputs to the input and fills the warm-up with NaN; the native APIs do not ([N2](/spec/inputs-outputs/#n2)).
- Published packages and their versions: [Install](/install/).
- Owned by other pages: [Unstable Period](/api/unstable-period/), [Candlestick Settings](/api/candle-settings/) (model and defaults), streaming in [C](/api/stream/), [Rust](/api/rust/stream/), [Java](/api/java/stream/) and [C#](/api/csharp/stream/), [Numerical Stability](/functions/stability), and the [function pages](/functions/): inputs, outputs in order, parameters (type, default, accepted values), stability category, flags.
- Every page except the per-function pages in one file: [/llms-full.txt](/llms-full.txt), whose [function index](/functions/) lists and links each function's page; [/llms.txt](/llms.txt) indexes them all. Each page has a Markdown twin, `/spec/index.md` for this one.

## Rule pages {#reading}

| Page | Covers | Ids |
|---|---|---|
| [Errors](/spec/errors/) | return codes, evaluation order, batch tier, messages, abstraction layer | R1 to R5, B1 to B8, B6a, M1, M2 |
| [Inputs and Outputs](/spec/inputs-outputs/) | index range, inputs, parameters, outputs, aliasing | I1 to I6, O1 to O7, N1 to N4, N8 |
| [Lookback](/spec/lookback/) | lookback call, unstable period, candle averaging, period 1, start dependence | L1 to L9 |
| [Streaming](/spec/streaming/) | bit-identity with batch, every stream call, lifetime | S1 to S7, S6a, U1 to U4, U6a, X1, H1 to H10, N7 |
| [Settings and Threads](/spec/settings-threads/) | C lifecycle, `Core`, setting validation, threads, settings under a live stream | G1 to G7, T1 to T7, N5, N6 |
| [Versions and Determinism](/spec/versions/) | bit-identity across languages and machines; releases | D1 to D4, V1 to V4 |

- `<N>` stands for a function's canonical name: `TA_<N>_Open` is `TA_SMA_Open`.
- An id's anchor is the id in lower case: [/spec/errors/#b5](/spec/errors/#b5). Its number is not its evaluation order (U4 precedes U2): [R2](/spec/errors/#r2). Its meaning is not promised to stay the same across releases.
- **Current behaviour** marks what today's code does, verified, where no rule is decided. It promises nothing.

## Names in each language {#names}

A function's canonical name is its name on [/functions/](/functions/). A fold changes only case and `_`; it adds no word boundary inside a segment:

| Fold | Rule | SMA | HT_TRENDLINE | CDL3BLACKCROWS |
|---|---|---|---|---|
| C | `TA_` + canonical | `TA_SMA` | `TA_HT_TRENDLINE` | `TA_CDL3BLACKCROWS` |
| snake | lower case | `sma` | `ht_trendline` | `cdl3blackcrows` |
| UpperCamel | lower case, upper-case each `_` segment's first letter, drop `_` | `Sma` | `HtTrendline` | `Cdl3blackcrows` |
| lowerCamel | UpperCamel, first letter lower case | `sma` | `htTrendline` | `cdl3blackcrows` |

Rust uses snake for functions, UpperCamel for types; Java lowerCamel for methods, UpperCamel for types; C# UpperCamel. Messages and the abstraction layer use the canonical name. Indicator parameters keep C's names everywhere (`startIdx`, `inReal`, `optInTimePeriod`, `outReal`).

Every function has this surface; in Rust, Java and C# the calls are methods of a `Core`:

| Surface | C | Rust | Java | C# |
|---|---|---|---|---|
| batch | `TA_SMA` | `sma` | `sma` | `Sma` |
| lookback | `int TA_SMA_Lookback` | `sma_lookback` returns `Result<usize, RetCode>` | `int smaLookback` | `int SmaLookback` |
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
| absent ([B4](/spec/errors/#b4)) | `NULL` | not expressible | `null` | a `null` array becomes an empty span |

| Type | C | Rust | Java | C# |
|---|---|---|---|---|
| import | `ta_libc.h` | crate `ta_lib` | `io.github.talib` | `TALib` |
| settings: default, builder | process globals ([T2](/spec/settings-threads/#t2)) | `Core::new()`, `Core::builder()` | `Core.DEFAULT`, `Core.builder()` | `Core.Default`, `Core.Builder()` |
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
| `REAL_DEFAULT` | -4e37, selects a real parameter's default ([N3](/spec/inputs-outputs/#n3)) |
| `INTEGER_DEFAULT` | `INT_MIN`, selects an integer parameter's default |
| `REAL_MIN`, `REAL_MAX` | -3e37, 3e37 ([I4](/spec/inputs-outputs/#i4)) |

### Abstraction layer {#abstraction}

It describes every function at run time (inputs, outputs, each optional parameter's default and range, flags) and runs its double-precision batch call. Its flags do not give the stability category: [Lookback](/spec/lookback/#start).

| | C `ta_abstract.h` | Rust `ta_lib::abstract_api` | Java `io.github.talib.metadata` | C# `TALib.Metadata` |
|---|---|---|---|---|
| look up, enumerate | `TA_GetFuncHandle`, `TA_ForEachFunc` | `get_func_handle`, `FUNCS` | `Functions.byName`, `Functions.all()` | `FunctionCatalog.Default[name]`, `FunctionCatalog.Default` |
| call | `TA_ParamHolderAlloc`, `TA_CallFunc` | `FuncId::new_call`, `call` | `FuncInfo.newCall`, `call` | `FuncInfo.CreateCall`, `Call`, `TryCall` |
| all as one XML document | `TA_FunctionDescriptionXML()` | `function_description_xml()` | `FunctionDescription.xml()` | `FunctionDescription.Xml` |

## How a failure reaches the caller {#failures}

| Success | C, returning `TA_SUCCESS` | Rust | Java | C# |
|---|---|---|---|---|
| batch | range in `*outBegIdx`, `*outNBElement` | `Ok(OutRange)` | `OutRange` | `OutRange` |
| open | handle in `*stream`, last value in the out-pointers | `Ok((handle, value))` | handle | handle |
| open and fill | handle and range in out-pointers | `Ok((handle, OutRange))` | handle; range from `outRange()` | handle; range from `OutRange` |
| update, peek | value in the out-pointers | `Ok(value)` | value, or written into the sink | value |

| Failure | C | Rust | Java | C# |
|---|---|---|---|---|
| carrier, code (messages: [R5](/spec/errors/#r5)) | returned `TA_RetCode` | `Err(RetCode)` | an exception implementing `TALibFailure`; `retCode()` | an exception implementing `ITALibFailure`; `RetCode` |
| C's number | the value | `as_c_int()` | `asCInt()` | `(int)` cast |
| lookback ([L1](/spec/lookback/#l1)) | `-1` | `Err(RetCode::BadParam)` | `-1` | `-1` |

One row per function-tier code; `A : B` means `A` extends the platform type `B`, so a `catch` of `B` works.

| Code | Rust | Java | C# |
|---|---|---|---|
| `TA_BAD_PARAM` (2) | `BadParam` | `TALibArgumentException : IllegalArgumentException` | `TALibArgumentException : ArgumentException` |
| `TA_OUT_OF_RANGE_START_INDEX` (12) | `OutOfRangeStartIndex` | `TALibIndexException : IndexOutOfBoundsException` | `TALibArgumentOutOfRangeException : ArgumentOutOfRangeException` |
| `TA_OUT_OF_RANGE_END_INDEX` (13) | `OutOfRangeEndIndex` | `TALibIndexException` | `TALibArgumentOutOfRangeException`; `TALibArgumentException` from `Update`, `Advance` ([U4](/spec/streaming/#u4)) |
| `TA_INSUFFICIENT_HISTORY` (17) | `InsufficientHistory` | `InsufficientHistoryException : TALibArgumentException` | `InsufficientHistoryException : TALibArgumentException` |
| `TA_INTERNAL_ERROR` (5000, [B8](/spec/errors/#b8)) | `InternalError` | `TALibStateException : IllegalStateException` | `TALibInvalidOperationException : InvalidOperationException` |
| `TA_ALLOC_ERR` (3) | `AllocErr`, never returned ([B7](/spec/errors/#b7)) | never thrown | never thrown |

C# sets `ParamName` of an index exception to `startIdx` or `endIdx` in batch, and to the first input series in an opener.

Rust's and Java's `RetCode` hold exactly `TA_SUCCESS` and these codes; C#'s also holds `InputNotAllInitialize` (10) and `OutputNotAllInitialize` (11). Numbering: [V2](/spec/versions/#v2).

Refusals outside the function tier:

| Refusal | C | Rust | Java | C# |
|---|---|---|---|---|
| setter ([G1](/spec/settings-threads/#g1)) | returns `TA_BAD_PARAM` | `build()` returns `Err(RetCode::BadParam)` ([G7](/spec/settings-threads/#g7)) | throws `IllegalArgumentException` (`NullPointerException` on null), no code | throws `ArgumentOutOfRangeException`, no code |
| getter ([G3](/spec/settings-threads/#g3)) | cannot refuse | returns `Err(RetCode::BadParam)` | as the setter | as the setter |
| abstraction layer's own, current behaviour | a returned [code](/spec/errors/#return-codes) | `Err(RetCode::BadParam)`; lookup returns `None` | `IllegalArgumentException`, no code; lookup returns `null` | .NET exceptions, no code; `TryCall` returns a code instead |
