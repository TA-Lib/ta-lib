---
title: "Wilder's Smoothed Moving Average (RMA)"
description: "Wilder's smoothed moving average: an exponential average whose smoothing factor is the reciprocal of the period rather than the 2/(n+1) of a classic EMA…"
---

## Summary

Wilder's smoothed moving average: an exponential average whose smoothing factor is the reciprocal of the period rather than the `2/(n+1)` of a classic EMA, seeded with a simple average of the first window. J. Welles Wilder Jr. introduced it in 1978 as the smoothing inside RSI, ATR and the directional-movement family; this exposes it as a moving average in its own right.

Because the smoothing factor is smaller than an EMA's at the same period, RMA reacts more slowly and gives noticeably more weight to old data: it takes about twice the period to shed the influence of a bar. Read it as a slow trend line — direction and slope matter, individual crossings much less than on a faster average.

It travels under five names for one object: RMA (TradingView, pandas-ta), SMMA (MetaTrader), Wilder's Smoothing or Wilder's Average (thinkorswim), `wilders` (Tulip), WilderMA (Wealth-Lab).

## Formula

alpha = 1 / N,  beta = 1 - alpha,  N = optInTimePeriod

seed at bar N-1:  RMA = ( x[0] + x[1] + ... + x[N-1] ) / N

for i >= N:       RMA[i] = alpha * x[i] + beta * RMA[i-1]

## Notes

- Wilder's own writing uses a period of 14, and pandas-ta defaults to 10. The default here is the one the rest of the moving-average family carries, so a call that swaps one MA for another keeps its period.
- The smoothing factor being `1/N` is sometimes quoted as "an RMA of N is an EMA of 2N-1". The factors really are identical, since `2/((2N-1)+1)` is `1/N`, but the two seed over different windows: the series differ through the warm-up and only converge as the seed's influence decays.
- `TA_RMA` over `TA_TRANGE` is `TA_ATR`, bit for bit. The recurrence is spelled here exactly as ATR spells it.
- The recurrence is the `alpha * x + (1 - alpha) * prev` form, which is TradingView Pine's `ta.rma` and pandas' `ewm(adjust=False)` kernel. Implementations that spell it `prev + (x - prev) * alpha` or `(prev * (N-1) + x) / N` are algebraically the same average and differ from this one only at the last bits.
- Being recursive, an output depends on how much history precedes it: the same bar computed from an earlier start differs while the seed still carries weight, and that difference decays by a factor of `1 - 1/N` per bar. The unstable period is how much of the warm-up to discard.

## Inputs

- `inReal` — Data on which to compute the average

## Outputs

- `outReal` — Wilder's smoothed moving average of the input

## Parameters

| Parameter | Type | Default | Accepted values | Description |
| --- | --- | --- | --- | --- |
| `optInTimePeriod` | integer | 30 | 1–100000 | Number of bars in the seed window, and the reciprocal of the smoothing factor |

## Properties

**Numerical Stability:** [Initial Unstable Period](/functions/stability.md#initial-unstable-period)

<div class="flag-table">

|  |
| :-- |
| <span class="flag-box">✅</span> **Overlap Input** <span class="flag-tip" tabindex="0" role="note" aria-label="Output is on the same scale as the input price, so it is drawn over the price chart." data-tip="Output is on the same scale as the input price, so it is drawn over the price chart.">i</span> |
| <span class="flag-box">☐</span> <span style="opacity:0.5">Independent Y-Axis</span> |
| <span class="flag-box">☐</span> <span style="opacity:0.5">Candlestick</span> |
| <span class="flag-box">☐</span> <span style="opacity:0.5">Can Output NaN or ±Inf</span> |
| <span class="flag-box">✅</span> **Identity at Period 1** <span class="flag-tip" tabindex="0" role="note" aria-label="A period of 1 performs no smoothing: the lookback is 0 and every output value is a bit-exact copy of its input value." data-tip="A period of 1 performs no smoothing: the lookback is 0 and every output value is a bit-exact copy of its input value.">i</span> |

</div>

## Implementation

TA-Lib Definition: [`rma.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/rma/rma.c) · [`rma.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/rma/rma.yaml)

| Native | File |
|--------|------|
| C | [`ta_RMA.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_RMA.c) |
| Rust | [`rma.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/library/src/ta_func/rma.rs) |
| Java | [`Core_RMA.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/fragments/Core_RMA.java) |

TA-Lib is also available for Python, R and more using a [wrapper](/install/#wrappers).

## Aliases

Wilder's Smoothed Moving Average, Smoothed Moving Average, SMMA, Wilder's Smoothing, WilderMA

## See Also

[EMA](/functions/ema.md) · [SMA](/functions/sma.md) · [ATR](/functions/atr.md) · [RSI](/functions/rsi.md) · [DEMA](/functions/dema.md)

## References

- **J. Welles Wilder Jr., _New Concepts in Technical Trading Systems_, Trend Research, 1978.** The original definition, given as the smoothing inside RSI, ATR and the directional-movement family.
- Steven B. Achelis, _Technical Analysis from A to Z_, page 366 — a worked Wilder-smoothing series.
- [thinkorswim, *WildersSmoothing*](https://tlc.thinkorswim.com/center/reference/Tech-Indicators/studies-library/V-Z/WildersSmoothing) — "smoothing factor of 1/length"; "The value for the first period is an SMA".
- [Incredible Charts, *Wilder Moving Average*](https://www.incrediblecharts.com/indicators/wilder_moving_average.php) — the `1/n` versus `2/(n+1)` comparison.
- [TradingView Pine Script, `ta.rma`](https://www.tradingview.com/pine-script-reference/v5/#fun_ta.rma) — the same recurrence, SMA-seeded.
