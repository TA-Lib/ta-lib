---
title: Abstract API and Metadata
description: "What TA-Lib's Abstract API provides in C, Rust, Java and C#: the run-time description of every function, the call by name through a parameter holder, the codes it answers, and the catalog of every metadata flag."
---

*Part of TA-Lib's [specification](/spec/), a reference for AI-agent-driven integration.*

The Abstract API describes every function at run time and calls it by name. The description is the metadata: a function's group, inputs, optional parameters, outputs and flags. The call runs the function's batch call through a parameter holder. Examples in the four languages: [Abstract API](/api/abstract/).

## The caller must {#caller}

- <a id="bind-all"></a>**Bind every input and every output before the call.** That includes an output the typed call lets a caller decline ([rW5](/spec/inputs-outputs/#rw5)). An optional parameter may stay unbound ([rA5](/spec/abstract/#ra5)).
- <a id="range-at-call"></a>**Not take an accepted setter for an accepted value.** A setter checks the slot it names and the kind of the value. It may accept a value outside the parameter's accepted values: the call then rejects it ([rA1](/spec/abstract/#ra1)), and the holder's lookback and display shift return their rejection signal ([rL2](/spec/lookback/#rl2), [rL11](/spec/lookback/#rl11)). In C, `TA_GetLookback` and `TA_GetDisplayShift` return `TA_SUCCESS` and carry the signal in the value.
- <a id="by-name"></a>**Identify a function by its name, never by its position.** A release that adds a function moves the ones after it, and the order of the groups differs between languages. In Rust, the number of a `FuncId` is such a position.
- <a id="unknown-flag"></a>**Ignore a flag you do not know.** A release may add a flag ([rV3](/spec/versions/#rv3)).
- <a id="c-buffers"></a>**In C, keep every bound buffer valid and long enough.** A C setter stores the pointer and takes no length. The buffer must stay valid until the last call on the holder, an input must reach `endIdx` ([input length](/spec/inputs-outputs/#input-length)) and an output must hold the count the call produces ([output size](/spec/inputs-outputs/#output-size)). None of it is detected.
- <a id="c-release"></a>**In C, release what the API allocated, once, with its own call.** `TA_ParamHolderFree` releases a holder, `TA_GroupTableFree` a group table and `TA_FuncTableFree` a function table. A function handle and the descriptors it leads to are static: never free them.

Two more are on other pages: one thread per holder ([confine](/spec/settings-threads/#confine)), and enumerating the functions rather than hard-coding a list ([enumerate](/spec/versions/#enumerate)).

## What it provides {#provides}

It covers the double-precision batch call of every function ([rA6](/spec/abstract/#ra6)). It opens no stream ([rH9](/spec/streaming/#rh9)).

| | C `ta_abstract.h` | Rust `ta_lib::abstract_api` | Java `io.github.talib.metadata` | C# `TALib.Metadata` |
|---|---|---|---|---|
| look up by name | `TA_GetFuncHandle` | `get_func_handle`, `get_func_handle_rc` | `Functions.byName` | `FunctionCatalog.Default[name]`, `TryGet` |
| enumerate the functions | `TA_ForEachFunc` | `FUNCS`, `for_each_func` | `Functions.all()` | `FunctionCatalog.Default` |
| enumerate the groups | `TA_GroupTableAlloc` | `groups()` | `Functions.groups()` | `FunctionCatalog.Groups` |
| functions of one group | `TA_FuncTableAlloc` | `funcs_in_group` | filter `Functions.all()` on `group()` | `InGroup` |
| function descriptor | `TA_GetFuncInfo` | `FuncId::info` | `FuncInfo` | `FuncInfo` |
| input, optional parameter, output descriptors | `TA_GetInputParameterInfo`, `TA_GetOptInputParameterInfo`, `TA_GetOutputParameterInfo` | `inputs`, `opt_inputs`, `outputs` | `inputs()`, `optInputs()`, `outputs()` | `Inputs`, `OptInputs`, `Outputs` |
| make a parameter holder | `TA_ParamHolderAlloc` | `FuncId::new_call` | `FuncInfo.newCall` | `FuncInfo.CreateCall` |
| bind an input | `TA_SetInputParamRealPtr`, `TA_SetInputParamIntegerPtr`, `TA_SetInputParamPricePtr` | `set_input`, `set_int_input`, `set_price_input` | `setInput`, `setPriceInput` | `SetInput`, `SetPriceInput` |
| bind an optional parameter | `TA_SetOptInputParamInteger`, `TA_SetOptInputParamReal` | `set_opt_input` | `setOptInput` | `SetOptInput` |
| bind an output | `TA_SetOutputParamRealPtr`, `TA_SetOutputParamIntegerPtr` | `set_output`, `set_int_output` | `setOutput` | `SetOutput` |
| call | `TA_CallFunc` | `call` | `call` | `Call`, `TryCall` |
| lookback | `TA_GetLookback` | `lookback` | `lookback` | `Lookback` |
| display shift of one output | `TA_GetDisplayShift` | `display_shift` | `displayShift` | `DisplayShift` |
| the metadata as one XML document | `TA_FunctionDescriptionXML()` | `function_description_xml()` | `FunctionDescription.xml()` | `FunctionDescription.Xml` |
| release | `TA_ParamHolderFree`, `TA_GroupTableFree`, `TA_FuncTableFree` | none | none | none |

The calls from binding to the display shift are made on the holder: C's `TA_ParamHolder`, and `ParamHolder` in Rust, Java and C#. In Rust, Java and C# a holder is made on a `Core` and computes under its settings ([rT1](/spec/settings-threads/#rt1)); Java's and C#'s holder takes the default `Core` when none is given. C#'s `TryCall` returns the code where `Call` throws. The holder's lookback and display shift need no input or output bound.

What a descriptor holds:

| Descriptor | Holds |
|---|---|
| function | canonical name ([names](/spec/#names)), group, one-line hint, [flags](/spec/abstract/#flags) |
| input | name, kind, and for a price input the [series it consumes](/spec/abstract/#flags-price) |
| optional parameter | name, display name, hint, [flags](/spec/abstract/#flags-display), kind, default, and its accepted values ([rP2](/spec/inputs-outputs/#rp2)): for a range its bounds, a suggested sweep (start, end, increment) and, for a real range, a display precision; for a list its named values |
| output | name, kind, [flags](/spec/abstract/#flags) |

In Rust, Java and C# the function descriptor also holds the descriptors of its inputs, optional parameters and outputs. In C it holds their three counts and the function's handle, and each parameter descriptor comes from its getter, by index from 0. Rust's also holds `id`, the `FuncId` that `get_func_handle` returns.

A slot is one input, one optional parameter or one output. A setter names it by its position among the function's inputs, optional parameters or outputs, from 0. A price input is one input, whatever number of series it consumes. An MA-type parameter is an integer list of the MA types.

| Kind | C | Rust | Java | C# |
|---|---|---|---|---|
| input | `TA_Input_Price`, `TA_Input_Real`, `TA_Input_Integer` | `InputType::Price`, `Real`, `Integer` | `InputType.PRICE`, `REAL`, `INTEGER` | `InputKind.Price`, `Real`, `Integer` |
| optional parameter | `TA_OptInput_RealRange`, `TA_OptInput_RealList`, `TA_OptInput_IntegerRange`, `TA_OptInput_IntegerList` | `OptInputType::RealRange`, `RealList`, `IntegerRange`, `IntegerList` | `OptInputType.REAL_RANGE`, `REAL_LIST`, `INTEGER_RANGE`, `INTEGER_LIST` | `OptInputDomain.RealRange`, `RealList`, `IntegerRange`, `IntegerList` |
| output | `TA_Output_Real`, `TA_Output_Integer` | `OutputType::Real`, `Integer` | `OutputType.REAL`, `INTEGER` | `OutputKind.Real`, `Integer` |

## Rules {#rules}

<a id="ra1"></a>**rA1** A call through a parameter holder whose arguments are all bound and accepted by their setters is held to the [batch conditions](/spec/errors/#batch), with the same codes, and no output can be declined: an empty buffer bound to an output the typed call lets a caller decline ([rW5](/spec/inputs-outputs/#rw5)) is absent ([rB4](/spec/errors/#rb4)). The exceptions are in C. Its setters take bare pointers and no length: a bound series shorter than the range goes undetected, as in [rB5](/spec/errors/#rb5). And an absent range out-parameter answers `TA_INVALID_PARAM_HOLDER`, not [rB4](/spec/errors/#rb4)'s code.

<a id="ra2"></a>**rA2** A rejected setter leaves the parameter holder as it found it, so a rejected re-bind cannot leave the next call to succeed, silently, over a mix of old and new arguments.

<a id="ra3"></a>**rA3** A misuse of a parameter holder (an unbound or mistyped argument, a setter index that names no slot, a holder the Abstract API did not make) answers `TA_BAD_PARAM` or one of the [codes below](/spec/abstract/#codes), and so does a misuse of C's lookups and tables. Which one is not specified, and a release may make it more specific. When a call is refused for the state of its holder, the function did not run and the code is one of the codes below, so it cannot be mistaken for the function's own. In Java and C# the carrier is a `TALibArgumentException` ([failures](/spec/#failures)).

<a id="ra4"></a>**rA4** A lookup of a function by name ignores the case of ASCII letters. An unknown name answers `TA_FUNC_NOT_FOUND` in C and from Rust's `get_func_handle_rc`, `None` from Rust's `get_func_handle`, `null` in Java, and in C# a `KeyNotFoundException` from the indexer and `false` from `TryGet`. An empty name answers `TA_BAD_PARAM` in C and from `get_func_handle_rc`, as does an absent one in C.

<a id="ra5"></a>**rA5** An optional parameter left unbound takes the function's documented default, as the default sentinel does in the typed call ([rP3](/spec/inputs-outputs/#rp3)).

<a id="ra6"></a>**rA6** Every function is described and can be called through the Abstract API, in the four languages.

<a id="ra7"></a>**rA7** A [flag](/spec/abstract/#flags) has the same bit value in each language that has it.

## Return codes {#codes}

Besides the codes a function call answers ([return codes](/spec/errors/#return-codes)), the Abstract API can answer the codes below. No batch, lookback or stream call returns one.

| Code | `TA_RetCode` | Meaning |
|---:|---|---|
| 4 | `TA_GROUP_NOT_FOUND` | No group has that name. |
| 5 | `TA_FUNC_NOT_FOUND` | No function has that name. |
| 6 | `TA_INVALID_HANDLE` | Not a function handle the Abstract API gave out. |
| 7 | `TA_INVALID_PARAM_HOLDER` | Not a parameter holder it made, or `TA_CallFunc` given no holder or no range out-parameter. |
| 8 | `TA_INVALID_PARAM_HOLDER_TYPE` | A setter given a value of another kind than the slot it names. |
| 10 | `TA_INPUT_NOT_ALL_INITIALIZE` | A call with an input left unbound. |
| 11 | `TA_OUTPUT_NOT_ALL_INITIALIZE` | A call with an output left unbound. |
| 15 | `TA_BAD_OBJECT` | Not a table it made, or a table given to the other kind's free call. |

## Flags {#flags}

A function, an input, an optional parameter and an output each carry one word of flags. The four tables below list every flag. A rule that depends on a flag is on the page of its topic, linked from the flag's row.

### Capability {#flags-capability}

What a function or an output supports.

| C | Rust | Java | C# | Meaning |
|---|---|---|---|---|
| `TA_FUNC_FLG_STREAM` | `FuncFlags::STREAM` | `FuncFlags.STREAMING` | `FuncFlags.Stream` | The function has a streaming API. Every function carries it ([rH9](/spec/streaming/#rh9)). |
| `TA_FUNC_FLG_CANDLESTICK` | `FuncFlags::CANDLESTICK` | `FuncFlags.CANDLESTICK` | `FuncFlags.Candlestick` | A candlestick pattern function (`CDL*`): its lookback can depend on the candle settings ([rL7](/spec/lookback/#rl7)) and every output is an integer pattern output ([rW8](/spec/inputs-outputs/#rw8)). |
| `TA_OUT_NULLABLE` | `OutputFlags::NULLABLE` | `OutputFlags.NULLABLE` | `OutputFlags.Nullable` | The typed call lets the caller decline this output ([rW5](/spec/inputs-outputs/#rw5)). A holder still needs it bound ([bind all](/spec/abstract/#bind-all)). |

### Price components {#flags-price}

The series a price input consumes. The price setter requires each of them and ignores the others. C# also has a `SetPriceInput` that binds one series at a time: a consumed series left unbound is then refused at the call ([rA3](/spec/abstract/#ra3)).

| C | Rust | Java | C# | Meaning |
|---|---|---|---|---|
| `TA_IN_PRICE_OPEN` | `InputFlags::PRICE_OPEN` | `InputFlags.PRICE_OPEN` | `PriceComponents.Open` | The open. |
| `TA_IN_PRICE_HIGH` | `InputFlags::PRICE_HIGH` | `InputFlags.PRICE_HIGH` | `PriceComponents.High` | The high. |
| `TA_IN_PRICE_LOW` | `InputFlags::PRICE_LOW` | `InputFlags.PRICE_LOW` | `PriceComponents.Low` | The low. |
| `TA_IN_PRICE_CLOSE` | `InputFlags::PRICE_CLOSE` | `InputFlags.PRICE_CLOSE` | `PriceComponents.Close` | The close. |
| `TA_IN_PRICE_VOLUME` | `InputFlags::PRICE_VOLUME` | `InputFlags.PRICE_VOLUME` | `PriceComponents.Volume` | The volume. |
| `TA_IN_PRICE_OPENINTEREST` | `InputFlags::PRICE_OPENINTEREST` | `InputFlags.PRICE_OPENINTEREST` | `PriceComponents.OpenInterest` | The open interest. No function sets it. |
| `TA_IN_PRICE_TIMESTAMP` | `InputFlags::PRICE_TIMESTAMP` | none | none | A timestamp. No function sets it, and no setter takes one. |

### Numerical property {#flags-numerical}

What can be said of the values a function writes.

| C | Rust | Java | C# | Meaning |
|---|---|---|---|---|
| `TA_FUNC_FLG_UNST_PER` | `FuncFlags::UNSTABLE_PERIOD` | `FuncFlags.UNSTABLE_PERIOD` | `FuncFlags.UnstablePeriod` | The function owns an unstable-period id ([rL6](/spec/lookback/#rl6), [stability](/spec/lookback/#metadata)). |
| `TA_FUNC_FLG_PATH_DEP` | `FuncFlags::PATH_DEPENDENT` | `FuncFlags.PATH_DEPENDENT` | `FuncFlags.PathDependent` | Path-dependent: the value at a bar depends on where the range starts, and never converges ([stability](/spec/lookback/#metadata)). |
| `TA_FUNC_FLG_NAN_INF_OUT` | `FuncFlags::NAN_INF_OUTPUT` | `FuncFlags.NAN_INF_OUTPUT` | `FuncFlags.NanInfOutput` | A successful call can write NaN or ±Inf on ordinary finite input ([rW6](/spec/inputs-outputs/#rw6)). |
| `TA_FUNC_FLG_PERIOD1_IDENTITY` | `FuncFlags::PERIOD1_IDENTITY` | `FuncFlags.PERIOD1_IDENTITY` | `FuncFlags.Period1Identity` | At a period of 1 every output value is a bit-for-bit copy of its input ([rL8](/spec/lookback/#rl8)). |
| `TA_OUT_POSITIVE` | `OutputFlags::POSITIVE` | `OutputFlags.POSITIVE` | `OutputFlags.Positive` | Positive values occur. An output setting any of the three sign flags sets every sign it writes ([rW8](/spec/inputs-outputs/#rw8)). |
| `TA_OUT_NEGATIVE` | `OutputFlags::NEGATIVE` | `OutputFlags.NEGATIVE` | `OutputFlags.Negative` | Negative values occur ([rW8](/spec/inputs-outputs/#rw8)). |
| `TA_OUT_ZERO` | `OutputFlags::ZERO` | `OutputFlags.ZERO` | `OutputFlags.Zero` | 0 occurs; on a pattern output, no pattern on that bar ([rW8](/spec/inputs-outputs/#rw8)). |
| `TA_OUT_PATTERN_BOOL` | `OutputFlags::PATTERN_BOOL` | `OutputFlags.PATTERN_BOOL` | `OutputFlags.PatternBool` | A pattern output whose values are 0 and 100 ([rW8](/spec/inputs-outputs/#rw8)). |
| `TA_OUT_PATTERN_BULL_BEAR` | `OutputFlags::PATTERN_BULL_BEAR` | `OutputFlags.PATTERN_BULL_BEAR` | `OutputFlags.PatternBullBear` | A pattern output whose sign is a call: + bullish, - bearish ([rW8](/spec/inputs-outputs/#rw8)). |
| `TA_OUT_PATTERN_WEAK` | `OutputFlags::PATTERN_WEAK` | `OutputFlags.PATTERN_WEAK` | `OutputFlags.PatternWeak` | Adds level 80: a weaker form of the pattern, on the same bar ([rW8](/spec/inputs-outputs/#rw8)). |
| `TA_OUT_PATTERN_CONFIRM` | `OutputFlags::PATTERN_CONFIRM` | `OutputFlags.PATTERN_CONFIRM` | `OutputFlags.PatternConfirm` | Adds level 200: this bar confirms the output's live pattern ([rW8](/spec/inputs-outputs/#rw8)). |

### Display hint {#flags-display}

How a chart or a settings dialog should present a function, an output or an optional parameter. No value depends on one.

| C | Rust | Java | C# | Meaning |
|---|---|---|---|---|
| `TA_FUNC_FLG_OVERLAP` | `FuncFlags::OVERLAP` | `FuncFlags.OVERLAP_STUDY` | `FuncFlags.Overlap` | The output is on the scale of the input: draw it over the price series. |
| `TA_FUNC_FLG_VOLUME` | `FuncFlags::VOLUME` | `FuncFlags.VOLUME_USED` | `FuncFlags.VolumeUsed` | Draw the output over the volume data. No function sets it. |
| `TA_FUNC_FLG_DISPLAY_SHIFT` | `FuncFlags::DISPLAY_SHIFT` | `FuncFlags.DISPLAY_SHIFT` | `FuncFlags.DisplayShift` | At least one output carries `TA_OUT_DISPLAY_SHIFT` ([rL10](/spec/lookback/#rl10)). |
| `TA_OUT_DISPLAY_SHIFT` | `OutputFlags::DISPLAY_SHIFT` | `OutputFlags.DISPLAY_SHIFT` | `OutputFlags.DisplayShift` | A chart draws the output ahead of or behind the bar that computed it ([rL9](/spec/lookback/#rl9), [rL10](/spec/lookback/#rl10)). |
| `TA_OUT_LINE` | `OutputFlags::LINE` | `OutputFlags.LINE` | `OutputFlags.Line` | Draw as a connected line. |
| `TA_OUT_DOT_LINE` | `OutputFlags::DOT_LINE` | `OutputFlags.DOT_LINE` | `OutputFlags.DotLine` | Draw as a dotted line. No function sets it. |
| `TA_OUT_DASH_LINE` | `OutputFlags::DASH_LINE` | `OutputFlags.DASH_LINE` | `OutputFlags.DashLine` | Draw as a dashed line. |
| `TA_OUT_DOT` | `OutputFlags::DOT` | `OutputFlags.DOT` | `OutputFlags.Dot` | Draw as dots only. No function sets it. |
| `TA_OUT_HISTO` | `OutputFlags::HISTO` | `OutputFlags.HISTOGRAM` | `OutputFlags.Histogram` | Draw as a histogram. |
| `TA_OUT_UPPER_LIMIT` | `OutputFlags::UPPER_LIMIT` | `OutputFlags.UPPER_LIMIT` | `OutputFlags.UpperLimit` | The values are an upper limit, such as the upper line of a band. |
| `TA_OUT_LOWER_LIMIT` | `OutputFlags::LOWER_LIMIT` | `OutputFlags.LOWER_LIMIT` | `OutputFlags.LowerLimit` | The values are a lower limit, such as the lower line of a band. |
| `TA_OPTIN_IS_PERCENT` | `OptInputFlags::IS_PERCENT` | `OptInputFlags.IS_PERCENT` | `OptInputFlags.IsPercent` | The parameter is a percentage. |
| `TA_OPTIN_IS_DEGREE` | `OptInputFlags::IS_DEGREE` | `OptInputFlags.IS_DEGREE` | `OptInputFlags.IsDegree` | The parameter is an angle in degrees. No function sets it. |
| `TA_OPTIN_IS_CURRENCY` | `OptInputFlags::IS_CURRENCY` | `OptInputFlags.IS_CURRENCY` | `OptInputFlags.IsCurrency` | The parameter is a currency amount. No function sets it. |
| `TA_OPTIN_ADVANCED` | `OptInputFlags::ADVANCED` | `OptInputFlags.ADVANCED` | `OptInputFlags.Advanced` | The parameter is rarely changed: an application may hide it. No function sets it. |

C#'s four flag enums each also have `None`, the empty word.
