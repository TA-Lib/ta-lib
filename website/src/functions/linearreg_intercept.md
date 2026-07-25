---
title: LINEARREG_INTERCEPT
description: "Returns the y-intercept (b) of the least-squares regression line fitted over the last optInTimePeriod values. Part of the linear-regression family (LINEARREG, SLOPE, ANGLE, TSF)."
---

# LINEARREG_INTERCEPT

## Summary

Returns the y-intercept (b) of the least-squares regression line fitted over the last optInTimePeriod values. Part of the linear-regression family (LINEARREG, SLOPE, ANGLE, TSF).

## Formula

Fit y = b + m·x over the window with x = bars-ago (x=0 is the current bar, x=period-1 the oldest). With SumX = period(period-1)/2, SumXSqr = period(period-1)(2·period-1)/6, Divisor = SumX² − period·SumXSqr:
m = (period·SumXY − SumX·SumY) / Divisor
b = (SumY − m·SumX) / period   ← output

## Inputs

- `inReal` — Input series to regress

## Outputs

- `outReal` — Intercept b of the fitted line at each bar

## Parameters

| Parameter | Type | Default | Accepted values | Description |
| --- | --- | --- | --- | --- |
| `optInTimePeriod` | integer | 14 | 2–100000 | Window length of the regression |

## Properties

**Numerical Stability:** [Start-Independent](/functions/stability#start-independent)

| Display<br>Flags |
| :-- |
| <span class="flag-box">✅</span> **Overlap Input** <span class="flag-tip" tabindex="0" role="note" aria-label="Output is on the same scale as the input price, so it is drawn over the price chart." data-tip="Output is on the same scale as the input price, so it is drawn over the price chart.">i</span> |
| <span class="flag-box">☐</span> <span style="opacity:0.5">Independent Y-Axis</span> |
| <span class="flag-box">☐</span> <span style="opacity:0.5">Candlestick</span> |

## Implementation

TA-Lib Definition: [`linearreg_intercept.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/linearreg_intercept/linearreg_intercept.c) · [`linearreg_intercept.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/linearreg_intercept/linearreg_intercept.yaml)

| Native | File |
|--------|------|
| C | [`ta_LINEARREG_INTERCEPT.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_LINEARREG_INTERCEPT.c) |
| Rust | [`linearreg_intercept.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/library/src/ta_func/linearreg_intercept.rs) |
| Java | [`Core_LINEARREG_INTERCEPT.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/library/fragments/Core_LINEARREG_INTERCEPT.java) |

TA-Lib is also available for Python, R and more using a [wrapper](https://ta-lib.org/wrappers/).

## Aliases

Linear Regression Intercept

## See Also

[LINEARREG](/functions/linearreg) · [LINEARREG_SLOPE](/functions/linearreg_slope) · [LINEARREG_ANGLE](/functions/linearreg_angle) · [TSF](/functions/tsf)
