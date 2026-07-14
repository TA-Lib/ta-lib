---
title: MAVP
description: "Moving average whose period varies per bar, driven by a companion period series. For each bar it computes an MA of the selected type over the (clamped) period given by inPeriods."
---

# MAVP

## Summary

Moving average whose period varies per bar, driven by a companion period series. For each bar it computes an MA of the selected type over the (clamped) period given by inPeriods.

## Formula

p_i = clamp((int)inPeriods[startIdx+i], optInMinPeriod, optInMaxPeriod); outReal[i] = MA(inReal, p_i, optInMAType) at bar startIdx+i

## Notes

- Fractional per-bar periods are truncated to whole numbers before being clamped to the minimum and maximum period.
- Period values of 1 perform no smoothing (the bar's output equals its input); the minimum allowed period is 1 since 0.6.5.

## Inputs

- `inReal` — series to be averaged
- `inPeriods` — per-bar desired MA period

## Outputs

- `outReal` — variable-period moving average

## Parameters

- `optInMinPeriod` — lower clamp for per-bar period
- `optInMaxPeriod` — upper clamp for per-bar period
- `optInMAType` — moving-average type applied

## Implementation

TA-Lib Definition: [`mavp.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/mavp/mavp.c) · [`mavp.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/mavp/mavp.yaml)

| Native | File |
|--------|------|
| C | [`ta_MAVP.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_MAVP.c) |
| Rust | [`mavp.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/src/ta_func/mavp.rs) |
| Java | [`Core_MAVP.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/Core_MAVP.java) |

TA-Lib is also available for Python, R and more using a [wrapper](https://ta-lib.org/wrappers/).

## Aliases

Moving Average Variable Period, Variable Period Moving Average

## See Also

[MA](/functions/ma) · [SMA](/functions/sma) · [MAMA](/functions/mama) · [T3](/functions/t3)
