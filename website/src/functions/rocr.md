---
title: "Rate of change ratio: (price/prevPrice) (ROCR)"
description: "Rate of Change Ratio: the ratio of the current price to the price optInTimePeriod bars ago. A momentum measure centered at 1."
---

## Summary

Rate of Change Ratio: the ratio of the current price to the price optInTimePeriod bars ago. A momentum measure centered at 1. Always positive, centered at 1: >1 rising, <1 falling.

## Formula

ROCR = price / price[t - optInTimePeriod]

## Inputs

- `inReal` — Price series

## Outputs

- `outReal` — Ratio of current price to prior price

## Parameters

| Parameter | Type | Default | Accepted values | Description |
| --- | --- | --- | --- | --- |
| `optInTimePeriod` | integer | 10 | 1–100000 | Lookback distance in bars for the prior price |

## Properties

**Numerical Stability:** [Start-Independent](/functions/stability.md#start-independent)

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

TA-Lib Definition: [`rocr.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/rocr/rocr.c) · [`rocr.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/rocr/rocr.yaml)

| Native | File |
|--------|------|
| C | [`ta_ROCR.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_ROCR.c) |
| Rust | [`rocr.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/library/src/ta_func/rocr.rs) |
| Java | [`Core_ROCR.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/fragments/Core_ROCR.java) |

TA-Lib is also available for Python, R and more using a [wrapper](/install/#wrappers).

## Aliases

Rate of Change Ratio

## See Also

[ROC](/functions/roc.md) · [ROCP](/functions/rocp.md) · [ROCR100](/functions/rocr100.md) · [MOM](/functions/mom.md)
