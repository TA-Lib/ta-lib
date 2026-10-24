---
title: Lookback and Shift
description: "The lookback call tells, from the parameters and settings before any data, whether a call accepts them and how many values it writes: what enters it, the display shift a chart applies without moving any value, and how to tell from metadata whether the start of the series matters."
---

*Part of TA-Lib's [specification](/spec/), a reference for AI-agent-driven integration.*

The lookback call tells a program, before any series exists, whether a call accepts its parameters ([rL3](/spec/lookback/#rl3)) and how many values the call will write ([rW1](/spec/inputs-outputs/#rw1)).

## The caller must {#caller}

- <a id="ask"></a>**Ask for the lookback, never compute it.** Call the function's lookback call with the batch call's parameters, under the same settings ([rL5](/spec/lookback/#rl5)). A formula written from a function's description goes wrong as soon as an unstable period or an MA type enters ([rL6](/spec/lookback/#rl6)).
- <a id="start"></a>**Not compare values from different starts bit for bit.** Only a start-independent function gives the same value at a bar for any `startIdx`, and then only up to rounding error; for any other, a call with a later `startIdx` is not a slice of a call from 0. Each [function page](/functions/) names its [numerical stability](/functions/stability) category. A stream opened later in a series: [rH2](/spec/streaming/#rh2).

## Definition {#definition}

<a id="rl1"></a>**rL1** The lookback is the number of input bars a function consumes before its first output (SMA at period 10 has lookback 9). A batch call reads no bar before `max(startIdx, lookback) - lookback`: changing an earlier bar changes nothing the call writes. Where a batch call's output starts, and how many values it writes: [rW1](/spec/inputs-outputs/#rw1), [rW2](/spec/inputs-outputs/#rw2). How much history a stream needs: [rS7](/spec/streaming/#rs7).

## Lookback calls {#calls}

Every function has a lookback call. It takes exactly the batch call's optional parameters, in the same order, and no input series. A default sentinel selects the default, as in the batch call ([rP3](/spec/inputs-outputs/#rp3)). Its name and return type in each language, and the display-shift call's: [names](/spec/#names).

| Rule | Statement |
|---|---|
| <a id="rl2"></a>**rL2** | The call fails ([per language](/spec/#failures)) only on rejected parameters ([rL3](/spec/lookback/#rl3)). |
| <a id="rl3"></a>**rL3** | The call rejects exactly the parameters the batch call, `Open` or `OpenAndFill` would reject (batch [rB3](/spec/errors/#rb3), streams [rS3](/spec/streaming/#rs3)). |
| <a id="rl4"></a>**rL4** | Rust, Java and C# reach the same accept or reject decision as C for the same parameters and settings, and wherever both accept, return the same lookback or display shift. |
| <a id="rl5"></a>**rL5** | A lookback depends only on the optional parameters and on the [settings](/spec/settings-threads/) in effect at the call: the unstable periods ([rL6](/spec/lookback/#rl6)) and the candle averaging periods ([rL7](/spec/lookback/#rl7)). With every setting at its bound ([rT3](/spec/settings-threads/#rt3), [rT6](/spec/settings-threads/#rt6)) a lookback is negative only when the call fails. |

## What enters the lookback {#enters}

<a id="rl6"></a>**rL6** An unstable period adds bars to the lookback of the function that owns its id:

- A count adds exactly that many bars. MINUS_DI, MINUS_DM, PLUS_DI and PLUS_DM at period 1 take none and keep a lookback of 1.
- An Auto level adds the owner's count for that level ([rZ1](/spec/auto-stabilization/#rz1)), which can be 0.
- Either way, for a batch call from bar 0, the values the owner still reports are unchanged, bit for bit.
- A function computed through the owner, directly or through an MA type the caller selects, also takes the lengthening, and how much depends on the function. It can count more than once (through EMA, DEMA counts it twice and TEMA three times), count only where its path is the longest (KC takes the longer of its EMA and ATR paths), or not count at all (an MA stage at period 1 or of the `DISABLED` type takes none).
- Under an Auto level only, such a function may add bars of its own for how it uses the owner's value ([rZ2](/spec/auto-stabilization/#rz2)).

Accepted values: [rT3](/spec/settings-threads/#rt3). The functions that own an id, and how to set one: [Unstable Period](/api/unstable-period/). An inheriting function names its source in the Numerical Stability line of its [function page](/functions/).

<a id="rl7"></a>**rL7** A candle setting's `avgPeriod`, bounded by [rT6](/spec/settings-threads/#rt6), enters the lookback of the CDL functions that read that setting, and of no function without the [metadata flag](/spec/abstract/#flags) `TA_FUNC_FLG_CANDLESTICK`. The model and the defaults: [Candlestick Settings](/api/candle-settings/).

<a id="rl8"></a>**rL8** A function whose [metadata flags](/spec/abstract/#flags) include `TA_FUNC_FLG_PERIOD1_IDENTITY`, called with `optInTimePeriod` at 1, writes every output value as a bit-for-bit copy of `inReal` at the same bar (VWMA ignores its volume there). Its lookback at period 1 is 0 when every unstable period that reaches it is 0 or an Auto level. Which functions carry the flag: the "Identity at Period 1" row of each [function page](/functions/).

## Display shift {#display-shift}

Every function has a display-shift call. It takes the lookback call's parameters followed by the index of one output, counted from 0 over every output in the batch call's output order. A parameter holder of the [Abstract API](/spec/abstract/#range-at-call) returns the same value.

| Rule | Statement |
|---|---|
| <a id="rl9"></a>**rL9** | A display shift of `s` means a chart draws the value computed at bar `i` at bar `i + s`. Every output value is written at the bar that computed it, and no value, lookback, `outBegIdx` or `outNBElement` depends on the shift. |
| <a id="rl10"></a>**rL10** | A function carries `TA_FUNC_FLG_DISPLAY_SHIFT` exactly when at least one of its outputs carries the [metadata flag](/spec/abstract/#flags) `TA_OUT_DISPLAY_SHIFT`. An output without `TA_OUT_DISPLAY_SHIFT` has a shift of 0 for any parameters. |
| <a id="rl11"></a>**rL11** | The call fails ([per language](/spec/#failures)) exactly when the lookback call rejects the same parameters ([rL3](/spec/lookback/#rl3)) or the index names no output. |
| <a id="rl12"></a>**rL12** | No unstable period or candle setting ([rL5](/spec/lookback/#rl5)) changes a display shift. |

## Stability from metadata {#metadata}

What the [metadata](/spec/abstract/) says about a function's numerical stability:

- **Owns an unstable-period id**: `TA_FUNC_FLG_UNST_PER`.
- **Path-dependent**: `TA_FUNC_FLG_PATH_DEP`.
- **Depends on the MA type**: an optional parameter whose name ends in `MAType`.
- **Inherits an unstable period**: no flag marks it (DEMA through EMA). To test one call: read its lookback with every unstable period at 0; then set them all (the `ALL` wildcard) to a count above that lookback and read it again. If the two are equal, no unstable period reaches the call. An Auto level cannot serve here: its count can be 0 ([rL6](/spec/lookback/#rl6)).
