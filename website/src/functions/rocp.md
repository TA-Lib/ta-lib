---
title: ROCP
description: "Rate of change expressed as a fraction of the price optInTimePeriod bars ago. Normalized and centered at zero (positive or negative). >0 rising vs N bars ago, <0 falling; equals ROC/100."
---

# ROCP

## Summary

Rate of change expressed as a fraction of the price optInTimePeriod bars ago. Normalized and centered at zero (positive or negative). >0 rising vs N bars ago, <0 falling; equals ROC/100.

## Formula

ROCP = (price - prevPrice) / prevPrice, prevPrice = inReal[i - optInTimePeriod]

## Inputs

- `inReal` — Source price series

## Outputs

- `outReal` — Fractional rate of change vs the value optInTimePeriod bars earlier

## Parameters

| Parameter | Type | Default | Accepted values | Description |
| --- | --- | --- | --- | --- |
| `optInTimePeriod` | integer | 10 | 1–100000 | Lookback distance to the previous price |

## Properties

**Numerical Stability:** [Start-Independent](/functions/stability#start-independent)

| Display<br>Flags |
| :-- |
| <span class="flag-box">☐</span> <span style="opacity:0.5">Overlap Input</span> |
| <span class="flag-box">✅</span> **Independent Y-Axis** <span class="flag-tip" tabindex="0" role="note" aria-label="Output is on its own scale, drawn in a separate pane below the price chart." data-tip="Output is on its own scale, drawn in a separate pane below the price chart.">i</span> |
| <span class="flag-box">☐</span> <span style="opacity:0.5">Candlestick</span> |

## Implementation

TA-Lib Definition: [`rocp.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/rocp/rocp.c) · [`rocp.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/rocp/rocp.yaml)

| Native | File |
|--------|------|
| C | [`ta_ROCP.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_ROCP.c) |
| Rust | [`rocp.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/library/src/ta_func/rocp.rs) |
| Java | [`Core_ROCP.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/library/fragments/Core_ROCP.java) |

TA-Lib is also available for Python, R and more using a [wrapper](https://ta-lib.org/wrappers/).

## Aliases

Rate of Change Percentage, Percent Change

## See Also

[ROC](/functions/roc) · [ROCR](/functions/rocr) · [ROCR100](/functions/rocr100) · [MOM](/functions/mom)
