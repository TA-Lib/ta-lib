---
title: "Exponential Moving Average (EMA)"
description: "Exponential moving average that weights recent prices more heavily via a recursive smoothing factor."
---

## Summary

Exponential moving average that weights recent prices more heavily via a recursive smoothing factor. A core building block seeding or composing many other indicators. Reacts faster than SMA; price above/below EMA suggests up/down trend.

## Formula

k = 2 / (period + 1); EMA_t = (price_t - EMA_{t-1}) * k + EMA_{t-1}. Seed: EMA = SMA of first `period` bars.

## Notes

- A period of 1 performs no smoothing: the output is a copy of the input. Allowed since 0.6.5 (issues #48/#59).

## Inputs

- `inReal` — price/data series to smooth

## Outputs

- `outReal` — the exponential moving average

## Parameters

| Parameter | Type | Default | Accepted values | Description |
| --- | --- | --- | --- | --- |
| `optInTimePeriod` | integer | 30 | 1–100000 | Number of bars in the average; sets smoothing k = 2/(period+1) |

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

TA-Lib Definition: [`ema.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/ema/ema.c) · [`ema.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/ema/ema.yaml)

| Native | File |
|--------|------|
| C | [`ta_EMA.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_EMA.c) |
| Rust | [`ema.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/library/src/ta_func/ema.rs) |
| Java | [`Core_EMA.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/fragments/Core_EMA.java) |

TA-Lib is also available for Python, R and more using a [wrapper](/install/#wrappers).

## Aliases

Exponential Moving Average, Exponentially Weighted Moving Average, EWMA

## See Also

[SMA](/functions/sma.md) · [DEMA](/functions/dema.md) · [TEMA](/functions/tema.md) · [MA](/functions/ma.md) · [MACD](/functions/macd.md) · [T3](/functions/t3.md)
