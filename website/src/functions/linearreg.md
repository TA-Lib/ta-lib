---
title: "Linear Regression (LINEARREG)"
description: "Least-squares straight-line fit over the last optInTimePeriod bars, reported as the fitted line value at the window endpoint (b + m*(period-1))."
---

## Summary

Least-squares straight-line fit over the last optInTimePeriod bars, reported as the fitted line value at the window endpoint (b + m*(period-1)).

## Inputs

- `inReal` — Series to fit

## Outputs

- `outReal` — Regression line value at the window endpoint

## Parameters

| Parameter | Type | Default | Accepted values | Description |
| --- | --- | --- | --- | --- |
| `optInTimePeriod` | integer | 14 | 2–100000 | Number of bars in each regression window |

## Properties

**Numerical Stability:** [Start-Independent](/functions/stability.md#start-independent)

<div class="flag-table">

|  |
| :-- |
| <span class="flag-box">✅</span> **Overlap Input** <span class="flag-tip" tabindex="0" role="note" aria-label="Output is on the same scale as the input price, so it is drawn over the price chart." data-tip="Output is on the same scale as the input price, so it is drawn over the price chart.">i</span> |
| <span class="flag-box">☐</span> <span style="opacity:0.5">Independent Y-Axis</span> |
| <span class="flag-box">☐</span> <span style="opacity:0.5">Candlestick</span> |
| <span class="flag-box">☐</span> <span style="opacity:0.5">Can Output NaN or ±Inf</span> |
| <span class="flag-box">☐</span> <span style="opacity:0.5">Identity at Period 1</span> |

</div>

## Implementation

TA-Lib Definition: [`linearreg.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/linearreg/linearreg.c) · [`linearreg.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/linearreg/linearreg.yaml)

| Native | File |
|--------|------|
| C | [`ta_LINEARREG.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_LINEARREG.c) |
| Rust | [`linearreg.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/library/src/ta_func/linearreg.rs) |
| Java | [`Core_LINEARREG.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/fragments/Core_LINEARREG.java) |

TA-Lib is also available for Python, R and more using a [wrapper](/install/#wrappers).

## Aliases

Linear Regression, Least Squares, Best Fit Line

## See Also

[LINEARREG_SLOPE](/functions/linearreg_slope.md) · [LINEARREG_ANGLE](/functions/linearreg_angle.md) · [LINEARREG_INTERCEPT](/functions/linearreg_intercept.md) · [TSF](/functions/tsf.md)
