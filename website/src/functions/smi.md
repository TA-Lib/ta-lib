---
title: "Stochastic Momentum Index (SMI)"
description: "Stochastic Momentum Index: where the close sits relative to the midpoint of the recent high/low range, double-smoothed."
---

## Summary

Stochastic Momentum Index: where the close sits relative to the **midpoint** of the recent high/low range, double-smoothed. Lane's stochastic measures the close against the bottom of the range; Blau measures it against the middle, then smooths numerator and denominator separately with two exponential averages before dividing, which is what buys the low-lag, smooth-contoured curve. The result runs -100 to +100 rather than 0 to 100, so the zero line is the reference: positive means the close is above the midpoint of its range, negative below. Extreme readings mark overbought and oversold conditions, and crossings of the signal line are the usual trade trigger.

## Formula

HH = MAX(high, timePeriod);  LL = MIN(low, timePeriod)

num = close - 0.5 * (HH + LL);  den = HH - LL

SMI = 100 * EMA(EMA(num, slowPeriod), fastPeriod) / (0.5 * EMA(EMA(den, slowPeriod), fastPeriod))

Signal = EMA(SMI, signalPeriod)

## Notes

- A window whose bars are all flat (every high equal to its low) leaves both the numerator and the denominator at zero. Rather than divide, SMI emits 0 there — the same convention as CCI and IMI. Some implementations divide unguarded and return a non-finite value.
- Each exponential average is seeded with a simple average of its own first inputs, the same seeding TA-Lib's EMA uses, so the first published values converge toward an unlimited-history result rather than reproducing it exactly. `TA_SetUnstablePeriod(TA_FUNC_UNST_EMA, ...)` discards more of that warm-up. Implementations seeding from a single first sample — Tulip and TradingView among them — differ over the transient and agree once it decays.
- One output range covers both outputs, so the SMI values consumed by the signal line's own warm-up are not published.

## Inputs

- `inHigh` — High price series
- `inLow` — Low price series
- `inClose` — Close price series

## Outputs

- `outSMI` — Stochastic Momentum Index, -100 to +100
- `outSMISignal` — Exponential average of the SMI line

## Parameters

| Parameter | Type | Default | Accepted values | Description |
| --- | --- | --- | --- | --- |
| `optInTimePeriod` | integer | 13 | 2–100000 | Period of the high/low range |
| `optInFastPeriod` | integer | 2 | 2–100000 | Period of the second smoothing, applied to the first |
| `optInSlowPeriod` | integer | 25 | 2–100000 | Period of the first smoothing, applied to the raw momentum |
| `optInSignalPeriod` | integer | 9 | 2–100000 | Smoothing period of the signal line |

## Properties

**Numerical Stability:** [Initial Unstable Period](/functions/stability.md#initial-unstable-period) — Inherited from EMA, which SMI computes internally; tunable via EMA's unstable period.

<div class="flag-table">

|  |
| :-- |
| <span class="flag-box">☐</span> <span style="opacity:0.5">Overlap Input</span> |
| <span class="flag-box">✅</span> **Independent Y-Axis** <span class="flag-tip" tabindex="0" role="note" aria-label="Output is on its own scale, drawn in a separate pane below the price chart." data-tip="Output is on its own scale, drawn in a separate pane below the price chart.">i</span> |
| <span class="flag-box">☐</span> <span style="opacity:0.5">Candlestick</span> |
| <span class="flag-box">☐</span> <span style="opacity:0.5">Can Output NaN or ±Inf</span> |
| <span class="flag-box">☐</span> <span style="opacity:0.5">Identity at Period 1</span> |

</div>

## Implementation

TA-Lib Definition: [`smi.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/smi/smi.c) · [`smi.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/smi/smi.yaml)

| Native | File |
|--------|------|
| C | [`ta_SMI.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_SMI.c) |
| Rust | [`smi.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/library/src/ta_func/smi.rs) |
| Java | [`Core_SMI.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/fragments/Core_SMI.java) |

TA-Lib is also available for Python, R and more using a [wrapper](/install/#wrappers).

## Aliases

stochastic momentum index, Blau stochastic momentum

## See Also

[STOCH](/functions/stoch.md) · [STOCHRSI](/functions/stochrsi.md) · [WILLR](/functions/willr.md) · [MACD](/functions/macd.md)

## References

- William Blau, "Stochastic Momentum", *Technical Analysis of Stocks & Commodities*, v11:1 (January 1993), pp. 11-18
- William Blau, *Momentum, Direction and Divergence*, Wiley 1995 (ISBN 0471027294)
