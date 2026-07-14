---
title: MINUS_DI
description: "Wilder's Minus Directional Indicator: the Wilder-smoothed downward directional movement (-DM) normalized by smoothed True Range. Measures the strength of downward price movement. Higher -DI indicates a stronger downtrend; compared against +DI to gauge directional dominance."
---

# MINUS_DI

## Summary

Wilder's Minus Directional Indicator: the Wilder-smoothed downward directional movement (-DM) normalized by smoothed True Range. Measures the strength of downward price movement. Higher -DI indicates a stronger downtrend; compared against +DI to gauge directional dominance.

## Formula

-DM1 = (prevLow - low) if (prevLow-low)>0 and (high-prevHigh)<(prevLow-low), else 0. Seed -DM/TR = sum of first (period-1) -DM1/TR1, then Wilder-smooth each: X = X - X/period + today. -DI = 100 * (-DM / TR); TR from ta_true_range. If period<=1: -DI1 = -DM1/TR1 (no ×100).

## Notes

- Wilder's original integer rounding is not applied (it was removed as unreliable when values are near 1).

## Inputs

- `inPriceHLC` — High, low, close price series

## Outputs

- `outReal` — The Minus Directional Indicator (-DI) line

## Parameters

- `optInTimePeriod` — Smoothing/lookback period for -DM and TR

## Implementation

TA-Lib Definition: [`minus_di.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/minus_di/minus_di.c) · [`minus_di.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/minus_di/minus_di.yaml)

| Native | File |
|--------|------|
| C | [`ta_MINUS_DI.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_MINUS_DI.c) |
| Rust | [`minus_di.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/src/ta_func/minus_di.rs) |
| Java | [`Core_MINUS_DI.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/Core_MINUS_DI.java) |

TA-Lib is also available for Python, R and more using a [wrapper](https://ta-lib.org/wrappers/).

## Aliases

-DI, Negative Directional Indicator

## See Also

[PLUS_DI](/functions/plus_di.md) · [MINUS_DM](/functions/minus_dm.md) · [DX](/functions/dx.md) · [ADX](/functions/adx.md) · [ADXR](/functions/adxr.md) · [TRANGE](/functions/trange.md)

## References

- J. Welles Wilder, *New Concepts in Technical Trading Systems*, Trend Research (ISBN 0894590278)
