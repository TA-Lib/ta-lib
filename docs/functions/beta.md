---
title: BETA
description: "Beta: the slope of a least-squares linear regression of one series' percentage returns (y, from inReal1) against another's (x, from inReal0) over a rolling window. Measures how much a security moves relative to a market index. Beta = 1 moves with the index; < 1 less volatile, > 1 more volatile."
---

# BETA

## Summary

Beta: the slope of a least-squares linear regression of one series' percentage returns (y, from inReal1) against another's (x, from inReal0) over a rolling window. Measures how much a security moves relative to a market index. Beta = 1 moves with the index; < 1 less volatile, > 1 more volatile.

## Formula

Per-bar returns: $x_i=(p^0_i-p^0_{i-1})/p^0_{i-1}$ from inReal0, $y_i=(p^1_i-p^1_{i-1})/p^1_{i-1}$ from inReal1. With $n$=period over the window: $\beta = \dfrac{n\,S_{xy}-S_x S_y}{n\,S_{xx}-S_x^2}$, where $S_{xx}=\sum x^2,\ S_{xy}=\sum xy,\ S_x=\sum x,\ S_y=\sum y$.

## Inputs

- `inReal0` — Series whose returns are the regression x (market/index)
- `inReal1` — Series whose returns are the regression y (security)

## Outputs

- `outReal` — Beta: regression slope of inReal1-returns on inReal0-returns

## Parameters

- `optInTimePeriod` — Rolling window length (number of returns) for the regression sums

## Implementation

TA-Lib Definition: [`beta.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/beta/beta.c) · [`beta.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/beta/beta.yaml)

| Native | File |
|--------|------|
| C | [`ta_BETA.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_BETA.c) |
| Rust | [`beta.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/src/ta_func/beta.rs) |
| Java | [`Core_BETA.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/Core_BETA.java) |

TA-Lib is also available for Python, R and more using a [wrapper](https://ta-lib.org/wrappers/).

## Aliases

Beta coefficient

## See Also

[CORREL](correl.md) · [LINEARREG_SLOPE](linearreg_slope.md) · [VAR](var.md) · [STDDEV](stddev.md)
