---
title: ADXR
description: "Smoothed variant of ADX: the average of the current ADX value and the ADX value from (period-1) bars earlier. Further damps ADX to gauge trend strength. Higher values mean a stronger trend; smoother and more lagging than ADX."
---

# ADXR

## Summary

Smoothed variant of ADX: the average of the current ADX value and the ADX value from (period-1) bars earlier. Further damps ADX to gauge trend strength. Higher values mean a stronger trend; smoother and more lagging than ADX.

## Formula

ADXR[i] = (ADX[i] + ADX[i-(period-1)]) / 2

## Notes

- Wilder's original integer rounding is not applied (unreliable when values are near 1).

## Inputs

- `inPriceHLC` — High, low, close price series

## Outputs

- `outReal` — ADXR line (averaged ADX)

## Parameters

- `optInTimePeriod` — Smoothing period, also the bar gap between the two averaged ADX values

## Implementation

TA-Lib Definition: [`adxr.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/adxr/adxr.c) · [`adxr.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/adxr/adxr.yaml)

| Native | File |
|--------|------|
| C | [`ta_ADXR.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_ADXR.c) |
| Rust | [`adxr.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/src/ta_func/adxr.rs) |
| Java | [`Core_ADXR.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/Core_ADXR.java) |

TA-Lib is also available for Python, R and more using a [wrapper](https://ta-lib.org/wrappers/).

## Aliases

Average Directional Movement Index Rating

## See Also

[ADX](/functions/adx.md) · [DX](/functions/dx.md) · [PLUS_DI](/functions/plus_di.md) · [MINUS_DI](/functions/minus_di.md)

## References

- J. Welles Wilder, *New Concepts in Technical Trading Systems*, Trend Research (ISBN 0894590278)
