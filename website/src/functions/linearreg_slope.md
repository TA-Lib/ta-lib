---
title: LINEARREG_SLOPE
description: "Slope 'm' of the least-squares best-fit line (y = b + m*x) over the last optInTimePeriod bars. Reports the per-bar rate of change of the fitted trend line. Positive slope = rising trend, negative = falling; magnitude is price change per bar."
---

# LINEARREG_SLOPE

## Summary

Slope 'm' of the least-squares best-fit line (y = b + m*x) over the last optInTimePeriod bars. Reports the per-bar rate of change of the fitted trend line. Positive slope = rising trend, negative = falling; magnitude is price change per bar.

## Formula

m = (n·SumXY − SumX·SumY) / Divisor
SumX = n(n−1)/2,  SumXSqr = n(n−1)(2n−1)/6,  Divisor = SumX² − n·SumXSqr
SumXY = Σ i·y[today−i],  SumY = Σ y[today−i],  i=0..n−1,  n=period,  y=inReal

## Inputs

- `inReal` — Data series to fit

## Outputs

- `outReal` — Slope m of the fitted line

## Parameters

- `optInTimePeriod` — Number of bars in the regression window

## Implementation

TA-Lib Definition: [`linearreg_slope.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/linearreg_slope/linearreg_slope.c) · [`linearreg_slope.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/linearreg_slope/linearreg_slope.yaml)

| Native | File |
|--------|------|
| C | [`ta_LINEARREG_SLOPE.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_LINEARREG_SLOPE.c) |
| Rust | [`linearreg_slope.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/library/src/ta_func/linearreg_slope.rs) |
| Java | [`Core_LINEARREG_SLOPE.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/library/fragments/Core_LINEARREG_SLOPE.java) |

TA-Lib is also available for Python, R and more using a [wrapper](https://ta-lib.org/wrappers/).

## Aliases

Linear Regression Slope, LSMA slope, least squares slope

## See Also

[LINEARREG](/functions/linearreg) · [LINEARREG_INTERCEPT](/functions/linearreg_intercept) · [LINEARREG_ANGLE](/functions/linearreg_angle) · [TSF](/functions/tsf)
