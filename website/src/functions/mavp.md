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

| Parameter | Type | Default | Accepted values | Description |
| --- | --- | --- | --- | --- |
| `optInMinPeriod` | integer | 2 | 1–100000 | Lower clamp for the per-bar period |
| `optInMaxPeriod` | integer | 30 | 1–100000 | Upper clamp for the per-bar period |
| `optInMAType` | MAType | SMA (0) | any MAType | Moving-average type applied |

*`MAType` values: 0 SMA · 1 EMA · 2 WMA · 3 DEMA · 4 TEMA · 5 TRIMA · 6 KAMA · 7 MAMA · 8 T3*

## Implementation

TA-Lib Definition: [`mavp.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/mavp/mavp.c) · [`mavp.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/mavp/mavp.yaml)

| Native | File |
|--------|------|
| C | [`ta_MAVP.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_MAVP.c) |
| Rust | [`mavp.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/library/src/ta_func/mavp.rs) |
| Java | [`Core_MAVP.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/library/fragments/Core_MAVP.java) |

TA-Lib is also available for Python, R and more using a [wrapper](https://ta-lib.org/wrappers/).

## Aliases

Moving Average Variable Period, Variable Period Moving Average

## See Also

[MA](/functions/ma) · [SMA](/functions/sma) · [MAMA](/functions/mama) · [T3](/functions/t3)
