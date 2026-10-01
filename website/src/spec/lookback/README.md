---
title: Lookback
description: "How the lookback is defined and queried, the display shift a chart applies to an output, how the unstable period, candle averaging and a period of 1 enter the lookback, and how to tell from metadata whether the start of the series matters."
---

*Part of TA-Lib's exhaustive [specifications](/spec/), intended for precise AI-agent-driven integration with TA-Lib, to minimize errors.*

The lookback is how many input bars a function consumes before its first output, set by the optional parameters and the settings in effect, never by the input values. This page owns the lookback call, the display-shift call, how the unstable period, candle averaging and a period of 1 enter it, and how to tell whether a value depends on where the series starts.

## Definition {#definition}

<a id="l5"></a>**L5** The lookback is the number of input bars a function consumes before its first output: over a long enough series read from bar 0, the first output is at bar `lookback` (SMA at period 10 has lookback 9). Where a batch call's output starts, and how many values it writes: [O1](/spec/inputs-outputs/#o1), [N1](/spec/inputs-outputs/#n1), [O2](/spec/inputs-outputs/#o2). How much history a stream needs: [S7](/spec/streaming/#s7).

## Lookback calls {#calls}

Every function has a lookback call. It takes exactly the batch call's optional parameters, in the same order, and no input series. A default sentinel selects the default, as in the batch call ([N3](/spec/inputs-outputs/#n3)). Its name and return type in each language: [names](/spec/#names).

| Rule | Statement |
|---|---|
| <a id="l1"></a>**L1** | An optional parameter outside its accepted values, or a combination of parameters the function rejects ([I3](/spec/inputs-outputs/#i3)), returns the lookback rejection signal ([per language](/spec/#failures)). |
| <a id="l2"></a>**L2** | The signal is returned exactly when the batch call, `Open` or `OpenAndFill` would reject the same parameters (batch [B3](/spec/errors/#b3), streams [S3](/spec/streaming/#s3)). |
| <a id="l3"></a>**L3** | Rust, Java and C# reach the same accept or reject decision as C for the same parameters and settings, and wherever both accept, return the same lookback. Whether their outputs match C's: [D1](/spec/versions/#d1) (Rust), [D2](/spec/versions/#d2) (Java, C#). |
| <a id="l4"></a>**L4** | Nothing else in this tier fails. |

**[Current behaviour](/spec/#reading)**: Java's lookback calls break L1, L2 and L4 for a null MA type. They return a lookback when every null-typed stage runs at period 1 and throw `NullPointerException` otherwise; the batch call rejects every such call under [B3](/spec/errors/#b3).

<a id="l6"></a>**L6** A lookback depends only on the optional parameters and on the settings in effect at the call: the unstable periods ([L7](/spec/lookback/#l7)) and the candle averaging periods ([L8](/spec/lookback/#l8)). C reads them from the process globals ([T2](/spec/settings-threads/#t2)); Rust, Java and C# from the `Core` the call is made on ([T3](/spec/settings-threads/#t3)). It never depends on an input value.

**Current behaviour**: a lookback other than the rejection signal is in `[0, INT_MAX]`, settings at `TA_INDEX_MAX` included. One above `TA_INDEX_MAX` leaves no call able to produce a value.

## Display shift {#display-shift}

Every function has a display-shift call. It takes the lookback call's parameters followed by the index of one output, counted from 0 over every output in the batch call's output order. Its name and return type in each language: [names](/spec/#names). The abstraction layer's holder answers the same query ([abstraction layer](/spec/#abstraction)); in C that call returns `TA_SUCCESS` and carries a rejection in the value, as `TA_GetLookback` does.

| Rule | Statement |
|---|---|
| <a id="l10"></a>**L10** | A display shift of `s` means a chart draws the value computed at bar `i` at bar `i + s`. It describes drawing only: every output value is written at the bar that computed it, and no value, lookback, `outBegIdx` or `outNBElement` depends on it. It depends only on the optional parameters and the output index, never on a setting or an input value. |
| <a id="l11"></a>**L11** | An output without `TA_OUT_DISPLAY_SHIFT` (Rust `OutputFlags::DISPLAY_SHIFT`, Java `OutputFlags.DISPLAY_SHIFT`, C# `OutputFlags.DisplayShift`) has a display shift of 0; a flagged output can report 0 for some parameters. A function carries `TA_FUNC_FLG_DISPLAY_SHIFT` (Rust `FuncFlags::DISPLAY_SHIFT`, Java `FuncFlags.DISPLAY_SHIFT`, C# `FuncFlags.DisplayShift`) exactly when at least one of its outputs carries the output flag. |
| <a id="l12"></a>**L12** | The call returns its rejection signal ([per language](/spec/#failures)) exactly when the lookback call rejects the same parameters ([L1](/spec/lookback/#l1)) or the index names no output. Rust, Java and C# reach the same decision as C and, wherever both accept, return the same shift. Nothing else in this tier fails. |

**[Current behaviour](/spec/#reading)**: Java's display-shift calls throw `NullPointerException` for a null MA type wherever the lookback call does ([L4](/spec/lookback/#l4)).

## Unstable period {#unstable-period}

<a id="l7"></a>**L7** An unstable period enters the lookback of the function that owns its id, adding exactly that many bars; MINUS_DI, MINUS_DM, PLUS_DI and PLUS_DM at period 1 are the exception, with a lookback of 1 whatever their unstable period. It also enters the lookback of every function computed through the owner, directly or through an MA type the caller selects. There it can count more than once (through EMA, DEMA counts it twice and TEMA three times), or count only where its path is the longest (KC takes the longer of its EMA and ATR paths). No unstable period enters through an MA-type stage at period 1, which copies its input whatever the type (MA itself at period 1, STOCH's slow stages at period 1), nor through the `DISABLED` type, a copy at any period. Its bound is [G2](/spec/settings-threads/#g2). One changed while a stream is open: [T7](/spec/settings-threads/#t7).

**Current behaviour** (every function, default parameters), for a batch call reading from bar 0 (`startIdx` at most the lookback before the raise): raising an id's unstable period removes leading outputs from the owner and leaves every value it still reports unchanged, bit for bit. A function computed through the owner can also change the values it still reports (DEMA, MACD, KC), because each inner stage then starts later.

The functions that own an id, and how to set one: [Unstable Period](/api/unstable-period/). An inheriting function names its source in the Numerical Stability line of its [function page](/functions/).

## Candle averaging {#candle-averaging}

<a id="l8"></a>**L8** A candle setting's `avgPeriod` enters the lookback of every CDL function that reads that setting. CDL functions carry `TA_FUNC_FLG_CANDLESTICK` (Rust `FuncFlags::CANDLESTICK`, Java `FuncFlags.CANDLESTICK`, C# `FuncFlags.Candlestick`). Its bound is [G5](/spec/settings-threads/#g5). The model and the defaults: [Candlestick Settings](/api/candle-settings/). A C setting changed while a stream is open: [T7](/spec/settings-threads/#t7).

**Current behaviour**: a CDL function's lookback moves with the `avgPeriod` of exactly the settings it reads, never with a range type or a factor; no other function's lookback reads a candle setting. Raising one setting's `avgPeriod` above every other moves a pattern's lookback exactly when the pattern reads that setting.

## Period-1 identity {#period-1-identity}

<a id="l9"></a>**L9** A function flagged `TA_FUNC_FLG_PERIOD1_IDENTITY` (Rust `FuncFlags::PERIOD1_IDENTITY`, Java `FuncFlags.PERIOD1_IDENTITY`, C# `FuncFlags.Period1Identity`), called with `optInTimePeriod` at 1, writes every output value as a bit-for-bit copy of its input at the same bar (VWMA copies the close). When no unstable period reaches it, its lookback at period 1 is 0. Which functions carry the flag: the "Identity at Period 1" row of each [function page](/functions/) (its tooltip omits the unstable-period case; L9 governs).

**Current behaviour**: an unstable period that reaches the function still enters its lookback at period 1, and the values stay copies. With every unstable period at 7, EMA's lookback at period 1 is 7, DEMA's 14 and TEMA's 21. MA at period 1: [L7](/spec/lookback/#l7).

## Start of the series {#start}

Whether the value at a bar depends on where the series starts is a function's numerical stability: the categories are on [Numerical Stability](/functions/stability), and each [function page](/functions/) names its own.

**Current behaviour** (every function, default parameters):

- A batch call reads no bar before `max(startIdx, lookback) - lookback`: changing an earlier bar changes no output. That bar is where the call's series starts, so for a function that is not start-independent, a call with a later `startIdx` is not a slice of a call from 0.
- A start-independent function gives the same value at a bar for any `startIdx` only up to rounding error, as much as about 1e-10 of `max(|value|, 1)` (LINEARREG_ANGLE). Never compare such values bit for bit.

Detecting each property from metadata:

| Property | C | Rust | Java | C# |
|---|---|---|---|---|
| Owns an unstable-period id | `TA_FUNC_FLG_UNST_PER` | `FuncFlags::UNSTABLE_PERIOD` | `FuncFlags.UNSTABLE_PERIOD` | `FuncFlags.UnstablePeriod` |
| Path-dependent | `TA_FUNC_FLG_PATH_DEP` | `FuncFlags::PATH_DEPENDENT` | `FuncFlags.PATH_DEPENDENT` | `FuncFlags.PathDependent` |

- **Depends on the MA type**: an optional parameter whose name ends in `MAType`. The abstraction layer describes it as an integer list of the MA types.
- **Inherits an unstable period**: no flag marks it (DEMA through EMA). For one call, compare its lookback with every id at 0 and with every id set above that first lookback (the `ALL` wildcard): an unstable period reaches the call, through its own id, a function it computes through or the MA type selected, exactly when the two differ.

A stream opened later in a series raises the same question: [H2](/spec/streaming/#h2).
