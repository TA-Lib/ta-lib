---
title: "Rate of change ratio 100 scale: (price/prevPrice)*100 (ROCR100)"
description: "Rate-of-change ratio scaled by 100: current price as a percentage of the price optInTimePeriod bars ago."
---

## Summary

Rate-of-change ratio scaled by 100: current price as a percentage of the price optInTimePeriod bars ago. Momentum measure centered at 100 and always positive. Above 100 = price rose vs n bars ago; below 100 = price fell.

## Formula

$ROCR100_t = \dfrac{price_t}{price_{t-n}} \times 100$, where $n$ = optInTimePeriod

## Inputs

- `inReal` — Input price/data series

## Outputs

- `outReal` — Rate-of-change ratio times 100

## Parameters

| Parameter | Type | Default | Accepted values | Description |
| --- | --- | --- | --- | --- |
| `optInTimePeriod` | integer | 10 | 1–100000 | Lookback distance (bars back) for the reference price |

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

TA-Lib Definition: [`rocr100.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/rocr100/rocr100.c) · [`rocr100.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/rocr100/rocr100.yaml)

| Native | File |
|--------|------|
| C | [`ta_ROCR100.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_ROCR100.c) |
| Rust | [`rocr100.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/library/src/ta_func/rocr100.rs) |
| Java | [`Core_ROCR100.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/fragments/Core_ROCR100.java) |

TA-Lib is also available for Python, R and more using a [wrapper](/install/#wrappers).

## Aliases

Rate of Change Ratio 100 Scale, MO

## See Also

[ROCR](/functions/rocr.md) · [ROC](/functions/roc.md) · [ROCP](/functions/rocp.md) · [MOM](/functions/mom.md)
