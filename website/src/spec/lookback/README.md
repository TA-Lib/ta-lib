---
title: Lookback and Shift
description: "How the lookback is defined and queried, what enters it (unstable period, candle averaging, a period of 1), the display shift a chart applies to an output, and how to tell from metadata whether the start of the series matters."
---

*Part of TA-Lib's [specification](/spec/), a reference for AI-agent-driven integration.*

The lookback is how many input bars a function consumes before its first output. It is set by the optional parameters and the settings in effect, never by the input values.

## The caller must {#caller}

- <a id="ask"></a>**Ask for the lookback, never compute it.** Call the function's lookback call with the parameters of the batch call, under the same settings. A formula written from a function's description goes wrong as soon as an unstable period or an MA type enters ([rL6](/spec/lookback/#rl6)).
- <a id="start"></a>**Not compare values from different starts bit for bit.** Whether a value depends on where the series starts is the function's numerical stability; each [function page](/functions/) names its [category](/functions/stability). Only a start-independent function gives the same value at a bar for any `startIdx`, and then only up to rounding error; for any other, a call with a later `startIdx` is not a slice of a call from 0. A stream opened later in a series raises the same question: [rH2](/spec/streaming/#rh2).

## Definition {#definition}

<a id="rl1"></a>**rL1** The lookback is the number of input bars a function consumes before its first output: over a long enough series read from bar 0, the first output is at bar `lookback` (SMA at period 10 has lookback 9). A batch call reads no bar before `max(startIdx, lookback) - lookback`: changing an earlier bar changes nothing the call writes. Where a batch call's output starts, and how many values it writes: [rW1](/spec/inputs-outputs/#rw1), [rW2](/spec/inputs-outputs/#rw2). How much history a stream needs: [rS8](/spec/streaming/#rs8).

## Lookback calls {#calls}

Every function has a lookback call. It takes exactly the batch call's optional parameters, in the same order, and no input series. A default sentinel selects the default, as in the batch call ([rP3](/spec/inputs-outputs/#rp3)). Its name and return type in each language: [names](/spec/#names).

| Rule | Statement |
|---|---|
| <a id="rl2"></a>**rL2** | An optional parameter outside its accepted values, or a combination of parameters the function rejects ([rP2](/spec/inputs-outputs/#rp2)), fails ([per language](/spec/#failures)). It is the call's only failure. |
| <a id="rl3"></a>**rL3** | The call fails exactly when the batch call, `Open` or `OpenAndFill` would reject the same parameters (batch [rB3](/spec/errors/#rb3), streams [rS3](/spec/streaming/#rs3)). A lookback call is therefore a way to validate parameters before a series exists. |
| <a id="rl4"></a>**rL4** | Rust, Java and C# reach the same accept or reject decision as C for the same parameters and settings, and wherever both accept, return the same lookback or display shift. |
| <a id="rl5"></a>**rL5** | A lookback depends only on the optional parameters and on the settings in effect at the call: the unstable periods ([rL6](/spec/lookback/#rl6)) and the candle averaging periods ([rL7](/spec/lookback/#rl7)). C reads them from its process-wide settings; Rust, Java and C# from the `Core` the call is made on ([rT1](/spec/settings-threads/#rt1)). With every setting at its bound ([rT3](/spec/settings-threads/#rt3), [rT6](/spec/settings-threads/#rt6)) a lookback is still a count: it is negative only when the call fails. |

## What enters the lookback {#enters}

<a id="rl6"></a>**rL6** An unstable period adds bars to the lookback of the function that owns its id:

- A count adds exactly that many bars. MINUS_DI, MINUS_DM, PLUS_DI and PLUS_DM at period 1 take none and keep a lookback of 1.
- An Auto level adds the owner's count for that level ([rZ1](/spec/auto-stabilization/#rz1)), computed from the call's optional parameters. It can be 0.
- Either way, for a batch call from bar 0, the values the owner still reports are unchanged, bit for bit.
- A function computed through the owner, directly or through an MA type the caller selects, also takes the lengthening, and how much depends on the function. It can count more than once (through EMA, DEMA counts it twice and TEMA three times), count only where its path is the longest (KC takes the longer of its EMA and ATR paths), or not count at all (an MA stage at period 1 or of the `DISABLED` type takes none).
- Under an Auto level only, such a function may add bars of its own for how it uses the owner's value ([rZ2](/spec/auto-stabilization/#rz2)).

Accepted values: [rT3](/spec/settings-threads/#rt3). The functions that own an id, and how to set one: [Unstable Period](/api/unstable-period/). An inheriting function names its source in the Numerical Stability line of its [function page](/functions/).

<a id="rl7"></a>**rL7** A candle setting's `avgPeriod` enters the lookback of the CDL functions that read that setting. CDL functions carry the [metadata flag](/spec/abstract/#flags) `TA_FUNC_FLG_CANDLESTICK` (Rust `FuncFlags::CANDLESTICK`, Java `FuncFlags.CANDLESTICK`, C# `FuncFlags.Candlestick`). Only `avgPeriod` enters: a range type or a factor changes no lookback, and a candle setting changes the lookback of no function without the flag. Its bound is [rT6](/spec/settings-threads/#rt6). The model and the defaults: [Candlestick Settings](/api/candle-settings/).

<a id="rl8"></a>**rL8** A function whose [metadata flags](/spec/abstract/#flags) include `TA_FUNC_FLG_PERIOD1_IDENTITY` (Rust `FuncFlags::PERIOD1_IDENTITY`, Java `FuncFlags.PERIOD1_IDENTITY`, C# `FuncFlags.Period1Identity`), called with `optInTimePeriod` at 1, writes every output value as a bit-for-bit copy of its input at the same bar (VWMA copies the close). Its lookback at period 1 is 0 when every unstable period that reaches it is 0 or an Auto level. Which functions carry the flag: the "Identity at Period 1" row of each [function page](/functions/).

## Display shift {#display-shift}

Every function has a display-shift call. It takes the lookback call's parameters followed by the index of one output, counted from 0 over every output in the batch call's output order. Its name and return type in each language: [names](/spec/#names). A parameter holder of the [Abstract API](/spec/abstract/#range-at-call) returns the same value.

| Rule | Statement |
|---|---|
| <a id="rl9"></a>**rL9** | A display shift of `s` means a chart draws the value computed at bar `i` at bar `i + s`. It describes drawing only: every output value is written at the bar that computed it, and no value, lookback, `outBegIdx` or `outNBElement` depends on it. It depends only on the optional parameters and the output index. |
| <a id="rl10"></a>**rL10** | A function carries `TA_FUNC_FLG_DISPLAY_SHIFT` (Rust `FuncFlags::DISPLAY_SHIFT`, Java `FuncFlags.DISPLAY_SHIFT`, C# `FuncFlags.DisplayShift`) exactly when at least one of its outputs carries the [metadata flag](/spec/abstract/#flags) `TA_OUT_DISPLAY_SHIFT`. Without it, you can assume that every shift is 0. |
| <a id="rl11"></a>**rL11** | The call fails ([per language](/spec/#failures)) exactly when the lookback call rejects the same parameters ([rL2](/spec/lookback/#rl2)) or the index names no output. |
| <a id="rl12"></a>**rL12** | No unstable period or candle setting ([rL5](/spec/lookback/#rl5)) changes a display shift. |

## Stability from metadata {#metadata}

What the [metadata](/spec/abstract/) says about a function's numerical stability:

| Property | C | Rust | Java | C# |
|---|---|---|---|---|
| Owns an unstable-period id | `TA_FUNC_FLG_UNST_PER` | `FuncFlags::UNSTABLE_PERIOD` | `FuncFlags.UNSTABLE_PERIOD` | `FuncFlags.UnstablePeriod` |
| Path-dependent | `TA_FUNC_FLG_PATH_DEP` | `FuncFlags::PATH_DEPENDENT` | `FuncFlags.PATH_DEPENDENT` | `FuncFlags.PathDependent` |

- **Depends on the MA type**: an optional parameter whose name ends in `MAType`. The Abstract API describes it as an [integer list](/spec/abstract/#provides) of the MA types.
- **Inherits an unstable period**: no flag marks it (DEMA through EMA). The function page is the complete classification. To test one call: read its lookback with every unstable period at 0; then set them all (the `ALL` wildcard) to a count above that lookback and read it again. If the two are equal, no unstable period reaches the call. An Auto level cannot serve here: it can add no bar to a call it reaches.
