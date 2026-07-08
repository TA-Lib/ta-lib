---
title: STDDEV
description: "Rolling standard deviation of a series over a window, scaled by a deviations multiplier. Delegates to VAR, then takes the square root."
---

# STDDEV

## Summary

Rolling standard deviation of a series over a window, scaled by a deviations multiplier. Delegates to VAR, then takes the square root.

## Formula

$\sigma_i = \sqrt{\mathrm{VAR}_i}\cdot nbDev$, where $\mathrm{VAR}_i = \frac{1}{N}\sum x^2 - \left(\frac{1}{N}\sum x\right)^2$ (population variance, $N=$ timePeriod)

## Notes

- Uses population variance (divides by the period, not period minus one), so results differ slightly from the sample standard deviation used by some tools.

## Inputs

- `inReal` — Series to measure dispersion of

## Outputs

- `outReal` — Standard deviation at each bar, scaled by optInNbDev

## Parameters

- `optInTimePeriod` — Window length
- `optInNbDev` — Multiplier applied to the standard deviation

## Implementation

TA-Lib Definition: [`stddev.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/stddev/stddev.c) · [`stddev.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/stddev/stddev.yaml)

| Native | File |
|--------|------|
| C | [`ta_STDDEV.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_STDDEV.c) |
| Rust | [`stddev.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/src/ta_func/stddev.rs) |
| Java | [`Core_STDDEV.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/Core_STDDEV.java) |

TA-Lib is also available for Python, R and more using a [wrapper](https://ta-lib.org/wrappers/).

## Aliases

Standard Deviation, SD, sigma

## See Also

[VAR](var.md) · [BBANDS](bbands.md) · [SMA](sma.md)
