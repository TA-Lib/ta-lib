---
title: "True Strength Index (TSI)"
description: "True Strength Index: William Blau's double-smoothed momentum oscillator."
---

## Summary

True Strength Index: William Blau's double-smoothed momentum oscillator. The one-bar price change is smoothed twice with exponential averages, and the same pair of averages is applied to the magnitude of that change; the ratio of the two is scaled by 100. Dividing the smoothed signed momentum by the smoothed absolute momentum normalises the reading, so the result is bounded by -100 and +100 and comparable across instruments. The double smoothing is what separates it from a raw momentum plot: the curve is smooth enough to read while keeping far less lag than a single average of the same total length.

Zero is the reference line — positive means the smoothed momentum is net upward, negative net downward — and its crossings are the usual trade trigger. Extreme readings mark overbought and oversold conditions, and divergence against price is the classic Blau reading. A signal line is not part of the output; apply `EMA` to `outReal` to obtain one, since no source agrees on its period.

## Formula

m = close - previous close

TSI = 100 * EMA(EMA(m, firstPeriod), secondPeriod) / EMA(EMA(|m|, firstPeriod), secondPeriod)

The first period is applied first, to the raw change; the second smooths its result. The order matters: the two averages do not commute, because each is seeded from a simple average of its own inputs.

## Notes

- An input whose every change is exactly zero leaves both the numerator and the denominator at zero. Rather than divide, TSI emits 0 there — the same convention as CCI and IMI. Some implementations divide unguarded and return a non-finite value.
- Each exponential average is seeded with a simple average of its own first inputs, the same seeding TA-Lib's EMA uses, so the first published values converge toward an unlimited-history result rather than reproducing it exactly. `TA_SetUnstablePeriod(TA_FUNC_UNST_EMA, ...)` discards more of that warm-up. Implementations seeding from a single first sample — trading-signals among them — differ over the transient and agree once it decays.
- The parameters are named by the order they are applied in, not fast and slow. Blau's published pair applies the longer average first, the inverse of the differenced fast/slow pairs elsewhere in the library, so swapping them silently returns a different indicator with the same lookback.

## Inputs

- `inReal` — Source price/value series, canonically the close

## Outputs

- `outReal` — True Strength Index, -100 to +100

## Parameters

| Parameter | Type | Default | Accepted values | Description |
| --- | --- | --- | --- | --- |
| `optInFirstPeriod` | integer | 25 | 2–100000 | Period of the first smoothing, applied to the raw momentum |
| `optInSecondPeriod` | integer | 13 | 2–100000 | Period of the second smoothing, applied to the first |

## Properties

**Numerical Stability:** [Initial Unstable Period](/functions/stability.md#initial-unstable-period) — Inherited from EMA, which TSI computes internally; tunable via EMA's unstable period.

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

TA-Lib Definition: [`tsi.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/tsi/tsi.c) · [`tsi.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/tsi/tsi.yaml)

| Native | File |
|--------|------|
| C | [`ta_TSI.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_TSI.c) |
| Rust | [`tsi.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/library/src/ta_func/tsi.rs) |
| Java | [`Core_TSI.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/fragments/Core_TSI.java) |

TA-Lib is also available for Python, R and more using a [wrapper](/install/#wrappers).

## Aliases

true strength index, Blau true strength index

## See Also

[SMI](/functions/smi.md) · [MACD](/functions/macd.md) · [CMO](/functions/cmo.md) · [RSI](/functions/rsi.md)

## References

- William Blau, "True Strength Index", *Technical Analysis of Stocks & Commodities*, v9:11 (November 1991), pp. 438-446
- William Blau, *Momentum, Direction and Divergence*, Wiley 1995 (ISBN 0471027294)
- [StockCharts: True Strength Index](https://school.stockcharts.com/doku.php?id=technical_indicators:true_strength_index)
