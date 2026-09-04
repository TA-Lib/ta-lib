---
title: "Moving average with variable period (MAVP)"
description: "Moving average whose period varies per bar, driven by a companion period series."
---

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

*`MAType` values: 0 SMA · 1 EMA · 2 WMA · 3 DEMA · 4 TEMA · 5 TRIMA · 6 KAMA · 7 MAMA · 8 T3 · 9 HMA · 10 DISABLED · 11 DEFAULT · 12 ZLEMA*

## Properties

**Numerical Stability:** [Depends on MA Type](/functions/stability.md#depends-on-ma-type) — This function's default, SMA, is start-independent.

<div class="flag-table">

|  |
| :-- |
| <span class="flag-box">✅</span> **Overlap Input** <span class="flag-tip" tabindex="0" role="note" aria-label="Output is on the same scale as the input price, so it is drawn over the price chart." data-tip="Output is on the same scale as the input price, so it is drawn over the price chart.">i</span> |
| <span class="flag-box">☐</span> <span style="opacity:0.5">Independent Y-Axis</span> |
| <span class="flag-box">☐</span> <span style="opacity:0.5">Candlestick</span> |
| <span class="flag-box">☐</span> <span style="opacity:0.5">Can Output NaN or ±Inf</span> |
| <span class="flag-box">☐</span> <span style="opacity:0.5">Identity at Period 1</span> |

</div>

## Implementation

TA-Lib Definition: [`mavp.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/mavp/mavp.c) · [`mavp.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/mavp/mavp.yaml)

| Native | File |
|--------|------|
| C | [`ta_MAVP.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_MAVP.c) |
| Rust | [`mavp.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/library/src/ta_func/mavp.rs) |
| Java | [`Core_MAVP.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/fragments/Core_MAVP.java) |

TA-Lib is also available for Python, R and more using a [wrapper](/install/#wrappers).

## Aliases

Moving Average Variable Period, Variable Period Moving Average

## See Also

[MA](/functions/ma.md) · [SMA](/functions/sma.md) · [MAMA](/functions/mama.md) · [T3](/functions/t3.md)
