# API Naming Specification

This is what the tree spells today, and what a new backend or a new indicator
must follow.

One canonical name per function. Each backend spells that one name with its own
conventions. **C is frozen**: no row changes its column, and every other backend
is free to differ from it.

## How to read the tables

Each cell is the spelling that backend uses. Each backend's own convention
decides it, except where a row points at
[section 10](#10-exceptions-idiom-loses), which lists every place we keep a
non-idiomatic spelling on purpose.

## 1. The canonical name, and the four folds

The canonical name is the `ta_codegen/input/` directory name upper-cased: `SMA`,
`HT_TRENDLINE`, `CDL3BLACKCROWS`. It is data as well as an identifier: the
metadata tier stores it, `byName` resolves it case-insensitively, argument-error
messages quote it, and the website and generated docs print it.

| fold | rule | `SMA` | `HT_TRENDLINE` | `CDL3BLACKCROWS` | `MINUS_DM` | `T3` |
|---|---|---|---|---|---|---|
| canonical | as written | `SMA` | `HT_TRENDLINE` | `CDL3BLACKCROWS` | `MINUS_DM` | `T3` |
| C | `TA_` + canonical | `TA_SMA` | `TA_HT_TRENDLINE` | `TA_CDL3BLACKCROWS` | `TA_MINUS_DM` | `TA_T3` |
| snake | lower-case, keep `_` | `sma` | `ht_trendline` | `cdl3blackcrows` | `minus_dm` | `t3` |
| lowerCamel | split on `_`, capitalize every segment but the first | `sma` | `htTrendline` | `cdl3blackcrows` | `minusDm` | `t3` |
| UpperCamel | split on `_`, capitalize every segment | `Sma` | `HtTrendline` | `Cdl3blackcrows` | `MinusDm` | `T3` |

Each backend uses one fold per kind of identifier: Rust snake for functions and
UpperCamel for types, Java lowerCamel for methods and UpperCamel for types, C#
UpperCamel for both.

## 2. Per-function surface

| surface | C (frozen) | Rust | Java | C# |
|---|---|---|---|---|
| batch | `TA_SMA` | `sma` | `sma` | `Sma` |
| lookback | `TA_SMA_Lookback` | `sma_lookback` | `smaLookback` | `SmaLookback` |
| numerics tier (not public) | body of `TA_SMA` | `sma_impl` | `smaImpl` | `SmaImpl` |
| opener | `TA_SMA_Open` | `sma_open` | `smaOpen` | `SmaOpen` |
| filling opener | `TA_SMA_OpenAndFill` | `sma_open_and_fill` | `smaOpenAndFill` | `SmaOpenAndFill` |
| open seams (not public) | `TA_SMA_OpenImpl`, `TA_SMA_StepImpl`, `TA_SMA_ReleaseImpl` | `sma_open_impl`, `sma_step_impl` | `smaOpenImpl`, `smaOpenInternal`, `smaStepImpl` | `SmaOpenImpl`, `SmaOpenInternal`, `SmaStepImpl` |
| handle type | `TA_SMA_Stream` | `SmaStream` | `Core.SmaStream` | `Core.SmaStream` |
| handle methods | `TA_SMA_Update`, `_Peek`, `_Value`, `_OutRange`, `_Advance`, `_Clone`, `_Close` | `update`, `peek`, `value`, `out_range`, `advance`, derived `Clone` | `update`, `peek`, `value`, `outRange`, `advance`, `clone` | `Update`, `Peek`, `Value`, `OutRange`, `Advance`, `Clone` |
| multi-output carrier | out-parameters | `(f64, f64, f64)` | `AccbandsOut` (`realUpperBand`) | `AccbandsValue` (`RealUpperBand`) |

The carrier row looks inconsistent and is not. **The suffix names the construct,
not the function**: `<N>Out` is a caller-owned object the call writes into,
`<N>Value` is an immutable value the call returns. The shapes differ by design
(#310): Java's sink is mutable, has no `equals`/`hashCode`, and is valid only
until the next call, while C#'s is a `readonly record struct` the caller may
keep, compare and key on.

Only Java needs the sink: a returned object allocates on the heap there, while
C#'s struct and Rust's tuple do not.

## 3. Shared types

| type | C (frozen) | Rust | Java | C# |
|---|---|---|---|---|
| function holder | (globals) | `Core` | `Core` | `Core` |
| builder | `TA_SetUnstablePeriod`, `TA_SetCandleSettings` | `CoreBuilder`, `unstable_period`, `candle_setting`, `restore_candle_default`, `build` | `CoreBuilder`, `unstablePeriod`, `candleSetting`, `restoreCandleDefault`, `build` | `CoreBuilder`, `UnstablePeriod`, `CandleSetting`, `RestoreCandleDefault`, `Build` |
| output range | `outBegIdx`, `outNBElement` out-params | `OutRange { beg_idx, count }`, `EMPTY`, `is_empty` | `OutRange(begIdx, count)`, `EMPTY`, `isEmpty` | `OutRange { BegIdx, Count }`, `Empty`, `IsEmpty` |
| candle setting | `TA_CandleSetting` | `CandleSetting { range_type, avg_period, factor }`, `CandleSettings` | `CandleSetting`, `rangeType()`, `avgPeriod()`, `factor()` | `CandleSetting`, `RangeType`, `AvgPeriod`, `Factor` |
| enum type names | `TA_RetCode`, `TA_MAType`, `TA_FuncUnstId`, `TA_CandleSettingType`, `TA_RangeType` | `RetCode`, `MAType`, `FuncUnstId`, `CandleSettingType`, `RangeType` | same | same |

C's word carries over where it is idiomatic in the target and is replaced where
it is not: `outBegIdx` gives `begIdx`, `outNBElement` gives `count`.

`MAType` keeps its two capitals in all three managed backends; see E5.

## 4. Enum members

| enum | C (frozen) | Rust | Java | C# |
|---|---|---|---|---|
| `RetCode` | `TA_SUCCESS`, `TA_BAD_PARAM` | `Success`, `BadParam` | `SUCCESS`, `BAD_PARAM` | `Success`, `BadParam` |
| `CandleSettingType` | `TA_BodyLong` | `BodyLong` | `BODY_LONG` | `BodyLong` |
| `RangeType` | `TA_RangeType_RealBody` | `RealBody` | `REAL_BODY` | `RealBody` |
| `MAType` | `TA_MAType_SMA` | `SMA` (E2) | `SMA` | `SMA` (E2) |
| `FuncUnstId` | `TA_FUNC_UNST_HT_DCPERIOD` | `HT_DCPERIOD` (E2) | `HT_DCPERIOD` | `HT_DCPERIOD` (E2) |

The two function-keyed enums keep the canonical spelling everywhere, Rust's
`FuncId` with them: their members are the published metadata strings, down to
`MAType`'s `DISABLED` and `DEFAULT`, so a fold would leave each enum spelling its
own data differently. Only the underscored members (`HT_DCPERIOD`, `UNUSED_1`)
draw a lint, at the definition site and never in a consumer; raising the C#
project's analysis mode means suppressing CA1707 there with a justification.
`FuncUnstId`'s `UNUSED_1`, `UNUSED_12` and `ALL` name no function, and follow
their enum rather than splitting it.

## 5. Constants

| constant | C (frozen) | Rust | Java | C# |
|---|---|---|---|---|
| defaults and bounds | `TA_REAL_DEFAULT`, `TA_INTEGER_DEFAULT`, `TA_MAX_INDEX` | `Core::REAL_DEFAULT`, `INTEGER_DEFAULT`, `MAX_INDEX` | `Core.REAL_DEFAULT`, `INTEGER_DEFAULT`, `MAX_INDEX` | `RealDefault`, `IntegerDefault`, `MaxIndex` |

Screaming snake is the Java and Rust rule and the C inheritance; .NET spells
constants PascalCase, so C# takes `RealDefault` through `MaxIndex`.

## 6. Failures

| surface | C (frozen) | Rust | Java | C# |
|---|---|---|---|---|
| carrier | `TA_RetCode` return | `Result<_, RetCode>` | thrown | thrown |
| marker | n/a | n/a | `TALibFailure` | `ITALibFailure` |
| bad argument | `TA_BAD_PARAM` | `RetCode::BadParam` | `TALibArgumentException` | `TALibArgumentException` |
| bad index | `TA_OUT_OF_RANGE_*` | `RetCode::OutOfRange*` | `TALibIndexException` | `TALibArgumentOutOfRangeException` |
| bad state | n/a | n/a | `TALibStateException` | `TALibInvalidOperationException` |
| short history | `TA_INSUFFICIENT_HISTORY` | `RetCode::InsufficientHistory` | `InsufficientHistoryException` | `InsufficientHistoryException` |

Rule: an exception type is named after the platform base type it extends, so
Java's `TALibIndexException` and C#'s `TALibArgumentOutOfRangeException` are the
same rule applied twice, not a divergence. `InsufficientHistoryException` carries
no prefix in either, because it collides with no platform type.

The prefix is `TALib`, matching the C# namespace and assembly and both standard
libraries' own `IOException`: a two-letter acronym, in an exception type, inside
a namespace named by the same acronym.

## 7. Metadata tier

| surface | C (frozen) | Rust | Java | C# |
|---|---|---|---|---|
| lookup | `TA_GetFuncHandle` | `get_func_handle` | `Functions.byName` | `FunctionCatalog[name]` |
| enumerate | `TA_ForEachFunc` | `for_each_func`, `FUNCS` | `Functions.all`, `Functions.groups` | `FunctionCatalog` as `IReadOnlyList`, `InGroup` |
| holder | `TA_ParamHolder` | `ParamHolder` | `ParamHolder` | `ParamHolder` |
| set input | `TA_SetInputParamRealPtr` | `set_input` | `setInput` | `SetInput` |
| set parameter | `TA_SetOptInputParamInteger` | `set_opt_input` | `setOptInput` | `SetOptInput` |
| set output | `TA_SetOutputParamRealPtr` | `set_output` | `setOutput` | `SetOutput` |
| lookback | `TA_GetLookback` | `lookback` | `lookback` | `Lookback` |
| invoke | `TA_CallFunc` | `call` | `call` | `Call`, `TryCall` |
| descriptor | `TA_FuncInfo` | `FuncInfo` | `FuncInfo` | `FuncInfo` |
| flags | `TA_FuncFlags`, `TA_InputFlags` | `FuncFlags`, `InputFlags` | `FuncFlags`, `InputFlags` | `FuncFlags` |

This is the one area where the backends could disagree about *words*, not
casing. The tier exists to mirror C, so it takes C's words and recases them:
`ParamHolder`, `setInput` / `setOptInput` / `setOutput`, `lookback`, `call`,
`FuncInfo`. `TryCall` keeps .NET's `Try` prefix, because that is a pattern
rather than a word.

The lookup and enumerate rows have no C word to take: C has free functions where
the managed backends have a catalog object, so `Functions` and `FunctionCatalog`
keep their own accessors.

Out of scope here: C# models the parameter domains as a record hierarchy
(`OptInputDomain`, `InputKind`) where Rust and Java use enums (`OptInputType`,
`InputType`). That is a shape, and a naming specification does not decide it.

## 8. Parameter names

| surface | C (frozen) | Rust | Java | C# |
|---|---|---|---|---|
| index range | `startIdx`, `endIdx` | `startIdx`, `endIdx` (E1) | same | same |
| inputs | `inReal`, `inHigh`, `inClose` | same (E1) | same | same |
| parameters | `optInTimePeriod` | same (E1) | same | same |
| outputs | `outReal`, `outInteger`, `outBegIdx`, `outNBElement` | `outReal` (E1) | same | same |

## 9. Package identity

| backend | identity |
|---|---|
| C | `libta-lib`, `TA_` prefix |
| Rust | crate `ta-lib`, lib target `ta_lib` |
| Java | `io.github.ta-lib:ta-lib`, package `io.github.talib` |
| C# | namespace and assembly `TALib`, `TALib.Metadata` |

## 10. Exceptions: idiom loses

These keep a spelling their backend would not choose. Each is load-bearing;
none is inherited by default.

- **E1. Parameter names are verbatim C in every backend.** They are metadata,
  not only identifiers: `OptInputInfo::param_name` carries `optInTimePeriod` and
  documents itself as "the parameter's name in the generated signature", and the
  XML description publishes the same strings. Renaming them per backend makes
  that claim false in three backends out of four. Rust pays for this with
  `#![allow(non_snake_case)]`.
- **E2. `MAType`, `FuncUnstId` and `FuncId` members keep the canonical function
  spelling** (`SMA`, `HT_DCPERIOD`, `UNUSED_1`): the same tokens are the
  published metadata strings, which is E3 in identifier position. Rust and C#
  deviate from their conventions to do it; Java's agrees.
- **E3. The canonical name stays upper-case wherever it is data**, whatever the
  method is called: `byName("RSI")`, the metadata tables, the XML, the error
  messages, rustdoc and javadoc prose, the website. A rename of the methods must
  not touch any of it.
- **E4. One word per concept, in every backend**: `_Impl` is the numerics tier,
  `_Internal` is a `startIdx`-anchored variant of an entry point, `Open` /
  `OpenAndFill` / `Update` / `Peek` / `Value` / `OutRange` / `Advance` are the
  streaming verbs. Backends recase these; none of them invents a synonym.
- **E5. A two-letter acronym keeps both capitals in identifiers**, so `MAType`
  and `TALib` in all four backends, as `IOException` does in both standard
  libraries. Canonical function names are not covered: they fold by section 1,
  which is why the `MA` and `HA` functions are `Ma` and `Ha`.

Also deliberate, and cheaper to state than to re-litigate: file names track the
canonical name (`src/ta_func/ta_SMA.c`, `Core_SMA.cs`, `sma.rs`) and are not part
of the API.
