---
title: Unstable Period
toc: false
---

# Unstable Period

Part of the [C/C++ Core API](/api/). The unstable period controls how much warm-up data a recursive indicator discards before its output is reported.

## Why it exists

Some indicators have "memory" — each output depends on the previous one, seeded from the start of the data. An Exponential Moving Average is the classic example: its value at a given bar depends, in principle, on every bar before it. In practice the influence of the earliest bars decays quickly, so the result becomes **stable** after enough bars.

The unstable period is the number of additional bars TA-Lib strips from the front of the output so that only stable values are reported. It is added on top of the function's normal lookback.

## API

```c
TA_RetCode   TA_SetUnstablePeriod( TA_FuncUnstId id, unsigned int unstablePeriod );
unsigned int TA_GetUnstablePeriod( TA_FuncUnstId id );
```

`id` selects which family of functions to affect. Setting it larger delays the first output but discards more of the warm-up transient; setting it to `0` (the default) reports every value the function can compute.

```c
/* Strip 30 extra bars from every EMA-based calculation: */
TA_SetUnstablePeriod( TA_FUNC_UNST_EMA, 30 );

/* Apply the same unstable period to ALL affected functions at once: */
TA_SetUnstablePeriod( TA_FUNC_UNST_ALL, 30 );
```

Like the other global settings, choose the unstable period **once, from a single thread**, before making concurrent calls (see [multi-threading](/api/#multithreading)).

## Functions with an unstable period

Pass one of these `TA_FuncUnstId` values (or `TA_FUNC_UNST_ALL` for all of them):

`TA_FUNC_UNST_ADX`, `ADXR`, `ATR`, `CMO`, `DX`, `EMA`, `HT_DCPERIOD`, `HT_DCPHASE`, `HT_PHASOR`, `HT_SINE`, `HT_TRENDLINE`, `HT_TRENDMODE`, `KAMA`, `MAMA`, `MINUS_DI`, `MINUS_DM`, `NATR`, `PLUS_DI`, `PLUS_DM`, `RSI`, `STOCHRSI`, `T3`.

The unstable period also flows through to functions built on these internally (for example, indicators that use an EMA inherit `TA_FUNC_UNST_EMA`'s setting). The full enumeration is in [ta_defs.h](https://github.com/TA-Lib/ta-lib/blob/main/include/ta_defs.h).

## See also

- [Initialize and Shutdown](/api/#init)
- [Candlestick Settings](/api/candle-settings/)
