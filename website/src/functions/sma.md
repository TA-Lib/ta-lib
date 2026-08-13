---
title: "Simple Moving Average (SMA)"
description: "Simple Moving Average: the unweighted arithmetic mean of the last N input values. Used to smooth a series."
---

## Summary

Simple Moving Average: the unweighted arithmetic mean of the last N input values. Used to smooth a series.

## Formula

SMA_t = (1/N) * sum_{i=t-N+1}^{t} inReal_i

## Notes

- A period of 1 performs no smoothing: the output is a copy of the input. Allowed since 0.6.5 (issues #48/#59).

## Inputs

- `inReal` — Source series to average

## Outputs

- `outReal` — Simple moving average of the input

## Parameters

| Parameter | Type | Default | Accepted values | Description |
| --- | --- | --- | --- | --- |
| `optInTimePeriod` | integer | 30 | 1–100000 | Number of bars in the averaging window |

## Properties

**Numerical Stability:** [Start-Independent](/functions/stability.md#start-independent)

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

TA-Lib Definition: [`sma.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/sma/sma.c) · [`sma.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/sma/sma.yaml)

| Native | File |
|--------|------|
| C | [`ta_SMA.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_SMA.c) |
| Rust | [`sma.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/library/src/ta_func/sma.rs) |
| Java | [`Core_SMA.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/fragments/Core_SMA.java) |

TA-Lib is also available for Python, R and more using a [wrapper](/install/#wrappers).

## Aliases

simple moving average

## See Also

[EMA](/functions/ema.md) · [WMA](/functions/wma.md) · [MA](/functions/ma.md) · [DEMA](/functions/dema.md) · [TEMA](/functions/tema.md)
