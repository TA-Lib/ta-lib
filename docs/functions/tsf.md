---
title: TSF
description: "Time Series Forecast: fits a least-squares linear regression line over the last N bars and projects it one x-step beyond LINEARREG. Same regression as LINEARREG but evaluated at x=period instead of x=period-1."
---

# TSF

## Summary

Time Series Forecast: fits a least-squares linear regression line over the last N bars and projects it one x-step beyond LINEARREG. Same regression as LINEARREG but evaluated at x=period instead of x=period-1.

## Formula

Fit y=b+m*x over window (x=0..N-1): m = (N*SumXY - SumX*SumY)/(SumX^2 - N*SumXSqr), b = (SumY - m*SumX)/N; output = b + m*N. With SumX=N(N-1)/2, SumXSqr=N(N-1)(2N-1)/6.

## Inputs

- `inReal` — Input series to regress and forecast

## Outputs

- `outReal` — Regression line value projected to x=period (one step past LINEARREG)

## Parameters

- `optInTimePeriod` — Number of bars in the regression window

## Implementation

TA-Lib Definition: [`tsf.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/tsf/tsf.c) · [`tsf.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/tsf/tsf.yaml)

| Native | File |
|--------|------|
| C | [`ta_TSF.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_TSF.c) |
| Rust | [`tsf.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/src/ta_func/tsf.rs) |
| Java | [`Core_TSF.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/Core_TSF.java) |

TA-Lib is also available for Python, R and more using a [wrapper](https://ta-lib.org/wrappers/).

## Aliases

Time Series Forecast

## See Also

[LINEARREG](linearreg.md) · [LINEARREG_SLOPE](linearreg_slope.md) · [LINEARREG_INTERCEPT](linearreg_intercept.md) · [LINEARREG_ANGLE](linearreg_angle.md)
