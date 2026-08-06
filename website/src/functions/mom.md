---
title: MOM
description: "Momentum: current price minus the price optInTimePeriod bars ago. The absolute (unnormalized) rate of change. Positive = price rose over the period, negative = fell; centered at zero."
---

# MOM

## Summary

Momentum: current price minus the price optInTimePeriod bars ago. The absolute (unnormalized) rate of change. Positive = price rose over the period, negative = fell; centered at zero.

## Formula

MOM[i] = inReal[i] - inReal[i - optInTimePeriod]

## Inputs

- `inReal` — Input price series

## Outputs

- `outReal` — Momentum (current minus value optInTimePeriod bars ago)

## Parameters

| Parameter | Type | Default | Accepted values | Description |
| --- | --- | --- | --- | --- |
| `optInTimePeriod` | integer | 10 | 1–100000 | Lookback distance in bars |

## Properties

**Numerical Stability:** [Start-Independent](/functions/stability#start-independent)

| Display<br>Flags |
| :-- |
| <span class="flag-box">☐</span> <span style="opacity:0.5">Overlap Input</span> |
| <span class="flag-box">✅</span> **Independent Y-Axis** <span class="flag-tip" tabindex="0" role="note" aria-label="Output is on its own scale, drawn in a separate pane below the price chart." data-tip="Output is on its own scale, drawn in a separate pane below the price chart.">i</span> |
| <span class="flag-box">☐</span> <span style="opacity:0.5">Candlestick</span> |

## Implementation

TA-Lib Definition: [`mom.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/mom/mom.c) · [`mom.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/mom/mom.yaml)

| Native | File |
|--------|------|
| C | [`ta_MOM.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_MOM.c) |
| Rust | [`mom.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/library/src/ta_func/mom.rs) |
| Java | [`Core_MOM.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/fragments/Core_MOM.java) |

TA-Lib is also available for Python, R and more using a [wrapper](https://ta-lib.org/wrappers/).

## Aliases

Momentum

## See Also

[ROC](/functions/roc) · [ROCP](/functions/rocp) · [ROCR](/functions/rocr) · [ROCR100](/functions/rocr100)
