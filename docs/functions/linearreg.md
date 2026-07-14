---
title: LINEARREG
description: "Least-squares straight-line fit over the last optInTimePeriod bars, reported as the fitted line value at the window endpoint (b + m*(period-1))."
---

# LINEARREG

## Summary

Least-squares straight-line fit over the last optInTimePeriod bars, reported as the fitted line value at the window endpoint (b + m*(period-1)).

## Inputs

- `inReal` — Series to fit

## Outputs

- `outReal` — Regression line value at the window endpoint

## Parameters

- `optInTimePeriod` — Number of bars in each regression window

## Implementation

TA-Lib Definition: [`linearreg.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/linearreg/linearreg.c) · [`linearreg.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/linearreg/linearreg.yaml)

| Native | File |
|--------|------|
| C | [`ta_LINEARREG.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_LINEARREG.c) |
| Rust | [`linearreg.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/src/ta_func/linearreg.rs) |
| Java | [`Core_LINEARREG.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/Core_LINEARREG.java) |

TA-Lib is also available for Python, R and more using a [wrapper](https://ta-lib.org/wrappers/).

## Aliases

Linear Regression, Least Squares, Best Fit Line

## See Also

[LINEARREG_SLOPE](/functions/linearreg_slope) · [LINEARREG_ANGLE](/functions/linearreg_angle) · [LINEARREG_INTERCEPT](/functions/linearreg_intercept) · [TSF](/functions/tsf)
