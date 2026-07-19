---
title: Unstable Period
toc: false
---

# Unstable Period

Some indicators need a warm-up before their output settles. The *unstable period* setting controls how many of those warm-up bars TA-Lib discards instead of reporting them.

## Why it exists

Some indicators have "memory" — each output depends on the previous one, seeded from the start of the data. An Exponential Moving Average is the classic example: its value at a given bar depends on every bar before it. In practice the influence of the earliest bars decays quickly, so the result becomes **stable** after enough bars.

This is inherent to the algorithms, not something specific to TA-Lib — every implementation has to seed the recursion somewhere, and the earliest outputs are distorted by that seed. What TA-Lib adds is the ability to scrub those early values on every function call, so unstable data never gets injected into your application.

## What to do

There are three distinct approaches, from the most common to the most rigorous:

1. **Ignore the problem.** This is what most users and most charting sites do, and it is relatively OK in practice: most people focus on the most recent bar, which by then has enough history behind it that the value has long since stabilized. The weakness is that nothing warns you when the assumption stops holding — a short series, or a back-test that acts on the earliest bars, will quietly use values that are off.

2. **Provide extra history.** Fetch more bars than you intend to use and treat the leading outputs as throwaway, so that even the first bar you actually act on has stabilized. This is application-level scrubbing: TA-Lib is left at its default (unstable period `0`) and returns everything it can compute, while your code chooses what to drop.

3. **Have TA-Lib drop the unstable data.** Set an unstable period and TA-Lib strips that many extra bars from the front of the output, on top of the function's normal lookback: `outBegIdx` moves forward and those unstable outputs never reach your code.

## API

```c
TA_RetCode   TA_SetUnstablePeriod( TA_FuncUnstId id, unsigned int unstablePeriod );
unsigned int TA_GetUnstablePeriod( TA_FuncUnstId id );
```

`id` selects which function to affect.

`unstablePeriod` sets how many warm-up bars that function discards — the larger the value, the later the first output. The default, `0`, discards nothing: you get every value the function can compute.

The setting follows the function wherever it runs: whether you call it directly, or another indicator uses it internally. `TA_FUNC_UNST_EMA` therefore affects `TA_EMA` and every indicator built on an EMA, such as `TA_MACD` and `TA_DEMA`.

```c
/* Strip 30 extra bars from every EMA-based calculation: */
TA_SetUnstablePeriod( TA_FUNC_UNST_EMA, 30 );

/* Apply the same unstable period to ALL affected functions at once: */
TA_SetUnstablePeriod( TA_FUNC_UNST_ALL, 30 );
```

Like the other global settings, choose the unstable period **once, from a single thread**, before making concurrent calls (see [multi-threading](/api/#multithreading)).

## Functions with an unstable period

Pass one of these `TA_FuncUnstId` values (or `TA_FUNC_UNST_ALL` for all of them):

<!-- ta_codegen:begin unstable-func-list -->
`TA_FUNC_UNST_ADX`, `TA_FUNC_UNST_ADXR`, `TA_FUNC_UNST_ATR`, `TA_FUNC_UNST_CMO`, `TA_FUNC_UNST_DX`, `TA_FUNC_UNST_EMA`, `TA_FUNC_UNST_HT_DCPERIOD`, `TA_FUNC_UNST_HT_DCPHASE`, `TA_FUNC_UNST_HT_PHASOR`, `TA_FUNC_UNST_HT_SINE`, `TA_FUNC_UNST_HT_TRENDLINE`, `TA_FUNC_UNST_HT_TRENDMODE`, `TA_FUNC_UNST_KAMA`, `TA_FUNC_UNST_MAMA`, `TA_FUNC_UNST_MINUS_DI`, `TA_FUNC_UNST_MINUS_DM`, `TA_FUNC_UNST_NATR`, `TA_FUNC_UNST_PLUS_DI`, `TA_FUNC_UNST_PLUS_DM`, `TA_FUNC_UNST_RSI`, `TA_FUNC_UNST_STOCHRSI`, `TA_FUNC_UNST_T3`.
<!-- ta_codegen:end unstable-func-list -->

The full enumeration is in [ta_defs.h](https://github.com/TA-Lib/ta-lib/blob/main/include/ta_defs.h).

## See also

- [Initialize and Shutdown](/api/#init)
- [Candlestick Settings](/api/candle-settings/)
