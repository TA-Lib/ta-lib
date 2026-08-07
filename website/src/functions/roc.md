---
title: "Rate of change : ((price/prevPrice)-1)*100 (ROC)"
description: "Rate-of-change momentum oscillator: the percent change of price versus the price optInTimePeriod bars earlier."
---

## Summary

Rate-of-change momentum oscillator: the percent change of price versus the price optInTimePeriod bars earlier. Centered at zero with positive and negative values. Positive when price rose over the period, negative when it fell; magnitude scales the move.

## Formula

ROC = ((price / prevPrice) - 1) * 100, where prevPrice = inReal[i - optInTimePeriod]

## Inputs

- `inReal` — Input price series

## Outputs

- `outReal` — Percent rate of change

## Parameters

| Parameter | Type | Default | Accepted values | Description |
| --- | --- | --- | --- | --- |
| `optInTimePeriod` | integer | 10 | 1–100000 | Lookback distance to the prior price |

## Properties

**Numerical Stability:** [Start-Independent](/functions/stability.md#start-independent)

| Display<br>Flags |
| :-- |
| <span class="flag-box">☐</span> <span style="opacity:0.5">Overlap Input</span> |
| <span class="flag-box">✅</span> **Independent Y-Axis** <span class="flag-tip" tabindex="0" role="note" aria-label="Output is on its own scale, drawn in a separate pane below the price chart." data-tip="Output is on its own scale, drawn in a separate pane below the price chart.">i</span> |
| <span class="flag-box">☐</span> <span style="opacity:0.5">Candlestick</span> |

## Implementation

TA-Lib Definition: [`roc.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/roc/roc.c) · [`roc.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/roc/roc.yaml)

| Native | File |
|--------|------|
| C | [`ta_ROC.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_ROC.c) |
| Rust | [`roc.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/library/src/ta_func/roc.rs) |
| Java | [`Core_ROC.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/fragments/Core_ROC.java) |

TA-Lib is also available for Python, R and more using a [wrapper](/install/#wrappers).

## Aliases

Rate of Change, Price Rate of Change

## See Also

[MOM](/functions/mom.md) · [ROCP](/functions/rocp.md) · [ROCR](/functions/rocr.md) · [ROCR100](/functions/rocr100.md)
