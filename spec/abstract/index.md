---
url: 'https://ta-lib.org/spec/abstract/index.md'
description: >-
  The rules of the Abstract API in C, Rust, Java and C#: what each function's
  run-time description holds, the flags a program reads instead of hard-coding
  per function, the XML document, the Abstract API's own return codes and the
  run-time information keys.
---
# Abstract API and Metadata

*Part of TA-Lib's [specification](/spec/), a reference for AI-agent-driven integration.*

The Abstract API describes every function at run time and calls it by name ([rA6](/spec/abstract/#ra6)). The description is the metadata: a function's group, inputs, optional parameters, outputs and flags. The call runs the function's double-precision batch call through a parameter holder, and opens no stream ([rH9](/spec/streaming/#rh9)). From the metadata alone a program can list the functions, build a dialog for their parameters and call any of them, with no code written per function. Examples in the four languages: [Abstract API](/api/abstract/).

## The caller must {#caller}

* <a id="bind-all"></a>**Bind every input and every output before the call.** An optional parameter may stay unbound ([rA5](/spec/abstract/#ra5)).
* <a id="range-at-call"></a>**Do not assume a value a bind call accepts is valid for the call.** A value outside the parameter's accepted values fails the call ([rA1](/spec/abstract/#ra1)) and the holder's lookback and display shift ([rL2](/spec/lookback/#rl2), [rL11](/spec/lookback/#rl11)); in C, `TA_GetLookback` and `TA_GetDisplayShift` still return `TA_SUCCESS`: test the value they write.
* <a id="by-name"></a>**Identify a function by its name, never by its position.** A release that adds a function moves the ones after it, and the order of the groups differs between languages. In Rust, the number of a `FuncId` is such a position.
* <a id="unknown-flag"></a>**Ignore a flag you do not know.** A release may add a flag ([rV3](/spec/versions/#rv3)).
* <a id="c-buffers"></a>**In C, keep every bound buffer valid and long enough.** Each must stay valid until the last call on the holder, and be as long as the batch call needs ([input length](/spec/inputs-outputs/#input-length), [output size](/spec/inputs-outputs/#output-size)).
* <a id="c-release"></a>**In C, release what the API allocated, once, with its own call.** A function handle and the descriptors it leads to are static: never free them.

Two more are on other pages: one thread per holder ([confine](/spec/settings-threads/#confine)), and enumerating the functions rather than hard-coding a list ([enumerate](/spec/versions/#enumerate)).

## What it provides {#provides}

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

A slot is one input, one optional parameter or one output. A bind call names it by its position among the function's inputs, optional parameters or outputs, from 0. A price input is one input, whatever number of series it consumes. An MA-type parameter is an integer list of the MA types. The kind types are `TA_InputParameterType`, `TA_OptInputParameterType` and `TA_OutputParameterType` in C; `InputType`, `OptInputType` and `OutputType` in Rust and Java; `InputKind`, `OptInputDomain` and `OutputKind` in C#.

## Rules {#rules}

<a id="ra1"></a>**rA1** A call through a parameter holder whose arguments are all bound and accepted by their bind calls is held to the [batch conditions](/spec/errors/#batch), with the same codes. The exception is in C: an absent range out-parameter returns `TA_INVALID_PARAM_HOLDER`, not [rB4](/spec/errors/#rb4)'s code.

<a id="ra2"></a>**rA2** A failed bind call leaves the parameter holder as it found it.

<a id="ra3"></a>**rA3** A misuse of a parameter holder (an unbound or mistyped argument, a bind index that names no slot, a holder the Abstract API did not make), or of C's lookups and tables, returns `TA_BAD_PARAM` or a code below, unspecified which. A call that fails for the state of its holder did not run, and its code is one of the [codes below](/spec/abstract/#codes), so it cannot be mistaken for the function's own result. In Java and C# the exception is a `TALibArgumentException` ([failures](/spec/#failures)).

<a id="ra4"></a>**rA4** A lookup of a function by name ignores the case of ASCII letters. An unknown name returns `TA_FUNC_NOT_FOUND` in C and from Rust's `get_func_handle_rc`, `None` from Rust's `get_func_handle`, `null` in Java, and in C# a `KeyNotFoundException` from the indexer and `false` from `TryGet`. An empty name returns `TA_BAD_PARAM` in C and from `get_func_handle_rc`, as does an absent one in C.

<a id="ra5"></a>**rA5** An optional parameter left unbound takes the function's documented default, as the default sentinel does in the typed call ([rP3](/spec/inputs-outputs/#rp3)).

<a id="ra6"></a>**rA6** Every function is described and can be called through the Abstract API, in the four languages.

<a id="ra7"></a>**rA7** A [flag](/spec/abstract/#flags) has the same bit value in each language that has it.

## Return codes {#codes}

Besides the codes a function call returns ([return codes](/spec/errors/#return-codes)), the Abstract API can return the codes below.

| Code | `TA_RetCode` | Meaning |
|---:|---|---|
| 4 | `TA_GROUP_NOT_FOUND` | No group has that name. |
| 5 | `TA_FUNC_NOT_FOUND` | No function has that name. |
| 6 | `TA_INVALID_HANDLE` | Not a function handle the Abstract API gave out. |
| 7 | `TA_INVALID_PARAM_HOLDER` | Not a parameter holder it made, `TA_CallFunc` given no holder, or [rA1](/spec/abstract/#ra1)'s exception. |
| 8 | `TA_INVALID_PARAM_HOLDER_TYPE` | A bind call given a value of another kind than the slot it names. |
| 10 | `TA_INPUT_NOT_ALL_INITIALIZE` | A call with an input left unbound. |
| 11 | `TA_OUTPUT_NOT_ALL_INITIALIZE` | A call with an output left unbound. |
| 15 | `TA_BAD_OBJECT` | Not a table it made, or a table given to the other kind's free call. |

## Run-time information {#runtime-info}

One call answers a key with an integer that describes the library in this process: C's `TA_GetRuntimeInfo` (`ta_common.h`), Rust's `get_runtime_info`, Java's `RuntimeInfo.get`, C#'s `RuntimeInfo.Get` and `TryGet`. It reports state and promises no output value and no speed.

Every key is answered in the four languages, and the answer can differ with the language and the platform. An unknown key returns `TA_BAD_PARAM` ([failures](/spec/#failures)); C#'s `TryGet` returns `false`.

| Key | Meaning |
|---|---|
| `vmath.transcendental` | The vectorized math library behind the batch call of functions such as `SIN`, `EXP` and `LN`: 0 none, 1 Apple's vForce. |
| `count.initialize` | How many times `TA_Initialize` was called in this process. 0 in Rust, Java and C#. |
| `count.shutdown` | How many times `TA_Shutdown` was called in this process. 0 in Rust, Java and C#. |

## Flags {#flags}

A function, an input, an optional parameter and an output each carry one word of flags, a bitmask. From these a program reads what it would otherwise hard-code per function: whether a function can write NaN or ±Inf or assumes positive input, which series a price input consumes, how a chart draws an output, and which values a pattern output writes. The tables below list every flag under its C name; [Rust, Java and C# spell them](/spec/#flag-names) in their own flag types. A flag's row links any rule that depends on it.

### Capability {#flags-capability}

What a function or an output supports.

| Flag | Meaning |
|---|---|
| `TA_FUNC_FLG_STREAM` | The function has a streaming API ([rH9](/spec/streaming/#rh9)). |
| `TA_FUNC_FLG_CANDLESTICK` | A `CDL*` candlestick pattern function ([rL7](/spec/lookback/#rl7), [rW7](/spec/inputs-outputs/#rw7)). |
| `TA_OUT_NULLABLE` | Deprecated: no output is optional. No function sets it. |

### Price components {#flags-price}

The series a price input consumes. The price bind call requires each of them and ignores the others. C# also has a `SetPriceInput` that binds one series at a time: a consumed series left unbound then fails the call ([rA3](/spec/abstract/#ra3)).

| Flag | Meaning |
|---|---|
| `TA_IN_PRICE_OPEN` | The open. |
| `TA_IN_PRICE_HIGH` | The high. |
| `TA_IN_PRICE_LOW` | The low. |
| `TA_IN_PRICE_CLOSE` | The close. |
| `TA_IN_PRICE_VOLUME` | The volume. |
| `TA_IN_PRICE_OPENINTEREST` | The open interest. No function sets it. |
| `TA_IN_PRICE_TIMESTAMP` | A timestamp. No function sets it, and no setter takes one. Java and C# have no member for it. |

### Numerical property {#flags-numerical}

What a function assumes of its input, and what can be said of the values it writes.

| Flag | Meaning |
|---|---|
| `TA_FUNC_FLG_UNST_PER` | The function owns an unstable-period id ([rL6](/spec/lookback/#rl6), [stability](/spec/lookback/#metadata)). |
| `TA_FUNC_FLG_PATH_DEP` | The function is [path-dependent](/functions/stability#path-dependent) ([stability](/spec/lookback/#metadata)). |
| `TA_FUNC_FLG_NAN_INF_OUT` | Can output NaN or ±Inf ([rW5](/spec/inputs-outputs/#rw5)). |
| `TA_FUNC_FLG_PERIOD1_IDENTITY` | Identity at period 1 ([rL8](/spec/lookback/#rl8)). |
| `TA_FUNC_FLG_USES_TRANSCENDENTAL` | The function calls a transcendental function ([rD1](/spec/versions/#rd1), [transcendental functions](/spec/versions/#transcendental)). Not set on a function that reaches one only through an MA-type parameter. |
| `TA_FUNC_FLG_POSITIVE_INPUT` | The input must be positive, as a price is. The data is not checked: a series that reaches or crosses zero, such as an oscillator or a spread, gets a successful call and an output that may have no meaning. The function's page says what a zero or a negative value does. |
| `TA_OUT_POSITIVE` | Positive values occur ([rW7](/spec/inputs-outputs/#rw7)). |
| `TA_OUT_NEGATIVE` | Negative values occur ([rW7](/spec/inputs-outputs/#rw7)). |
| `TA_OUT_ZERO` | 0 occurs ([rW7](/spec/inputs-outputs/#rw7)). |
| `TA_OUT_PATTERN_BOOL` | A two-state pattern output: 0 or non-zero ([rW7](/spec/inputs-outputs/#rw7)). |
| `TA_OUT_PATTERN_BULL_BEAR` | A pattern output whose sign is the pattern's call ([rW7](/spec/inputs-outputs/#rw7)). |
| `TA_OUT_PATTERN_WEAK` | Level 80 occurs too ([rW7](/spec/inputs-outputs/#rw7)). |
| `TA_OUT_PATTERN_CONFIRM` | Level 200 occurs too ([rW7](/spec/inputs-outputs/#rw7)). |

### Display hint {#flags-display}

How a chart or a settings dialog should present a function, an output or an optional parameter. No value depends on one.

| Flag | Meaning |
|---|---|
| `TA_FUNC_FLG_OVERLAP` | The output is on the scale of the input: draw it over the price series. |
| `TA_FUNC_FLG_VOLUME` | Draw the output over the volume data. No function sets it. |
| `TA_FUNC_FLG_DISPLAY_SHIFT` | At least one output carries `TA_OUT_DISPLAY_SHIFT` ([rL10](/spec/lookback/#rl10)). |
| `TA_OUT_DISPLAY_SHIFT` | The output's display shift can be nonzero ([rL9](/spec/lookback/#rl9), [rL10](/spec/lookback/#rl10)). |
| `TA_OUT_LINE` | Draw as a connected line. |
| `TA_OUT_DOT_LINE` | Draw as a dotted line. No function sets it. |
| `TA_OUT_DASH_LINE` | Draw as a dashed line. |
| `TA_OUT_DOT` | Draw as dots only. No function sets it. |
| `TA_OUT_HISTO` | Draw as a histogram. |
| `TA_OUT_UPPER_LIMIT` | The values are an upper limit, such as the upper line of a band. |
| `TA_OUT_LOWER_LIMIT` | The values are a lower limit, such as the lower line of a band. |
| `TA_OPTIN_IS_PERCENT` | The parameter is a percentage. |
| `TA_OPTIN_IS_DEGREE` | The parameter is an angle in degrees. No function sets it. |
| `TA_OPTIN_IS_CURRENCY` | The parameter is a currency amount. No function sets it. |
| `TA_OPTIN_ADVANCED` | The parameter is rarely changed: an application may hide it. No function sets it. |
