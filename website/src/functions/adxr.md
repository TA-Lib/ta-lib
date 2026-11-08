---
title: "Average Directional Movement Index Rating (ADXR)"
description: "Smoothed variant of ADX: the average of the current ADX value and the ADX value from (period-1) bars earlier. Further damps ADX to gauge trend strength."
---

## Summary

Smoothed variant of ADX: the average of the current ADX value and the ADX value from (period-1) bars earlier. Further damps ADX to gauge trend strength. Higher values mean a stronger trend; smoother and more lagging than ADX.

## Chart

<ta-chart src="/functions/adxr.svg" ratio="720:416" alt="Drawn on the price pane over candles; below it, a separate indicator pane showing outReal as a solid line."></ta-chart>

Drawn on the price pane over candles; below it, a separate indicator pane showing outReal as a solid line. [Reference data (CSV)](/functions/adxr.csv) · [Chart (SVG)](/functions/adxr.svg)

## Formula

ADXR[i] = (ADX[i] + ADX[i-(period-1)]) / 2

## Notes

- Wilder's original integer rounding is not applied (unreliable when values are near 1).

## Inputs

- `inHigh` — High price of each bar
- `inLow` — Low price of each bar
- `inClose` — Close price of each bar

## Outputs

- `outReal` — ADXR line (averaged ADX)

## Parameters

| Parameter | Type | Default | Accepted values | Description |
| --- | --- | --- | --- | --- |
| `optInTimePeriod` | integer | 14 | 2–100000 | Smoothing period, also the bar gap between the two averaged ADX values |

## Properties

**Numerical Stability:** [Initial Unstable Period](/functions/stability.md#initial-unstable-period) — Inherited from ADX, which ADXR computes internally; tunable via ADX's unstable period.

<div class="flag-table">

|  |
| :-- |
| <span class="flag-box">☐</span> <span style="opacity:0.5">Overlap Input</span> |
| <span class="flag-box">✅</span> **Independent Y-Axis** <span class="flag-tip" tabindex="0" role="note" aria-label="Output is on its own scale, drawn in a separate pane below the price chart." data-tip="Output is on its own scale, drawn in a separate pane below the price chart.">i</span> |
| <span class="flag-box">☐</span> <span style="opacity:0.5">Candlestick</span> |
| <span class="flag-box">☐</span> <span style="opacity:0.5">Can Output NaN or ±Inf</span> |
| <span class="flag-box">☐</span> <span style="opacity:0.5">Identity at Period 1</span> |
| <span class="flag-box">☐</span> <span style="opacity:0.5">Display Shift</span> |
| <span class="flag-box">☐</span> <span style="opacity:0.5">Uses Transcendental</span> |
| <span class="flag-box">☐</span> <span style="opacity:0.5">Input Must Be Positive</span> |

</div>

## Implementation

Added in 0.4.0.

TA-Lib Definition: [`adxr.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/adxr/adxr.c) · [`adxr.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/adxr/adxr.yaml)

| Native | File |
|--------|------|
| C | [`ta_ADXR.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_ADXR.c) |
| Rust | [`adxr.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/library/src/ta_func/adxr.rs) |
| Java | [`Core_ADXR.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/fragments/Core_ADXR.java) |
| C# | [`Core_ADXR.cs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/csharp/library/src/Core_ADXR.cs) |
| TypeScript | [`adxr.ts`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/ts/library/src/func/adxr.ts) |

Python: `talib.ADXR`, see [ta-lib-python](https://ta-lib.github.io/ta-lib-python/func_groups/momentum_indicators.html).

TA-Lib is also available for R and more using a [wrapper](/install/#wrappers).

## Aliases

Average Directional Movement Index Rating

## See Also

[ADX](/functions/adx.md) · [DX](/functions/dx.md) · [PLUS_DI](/functions/plus_di.md) · [MINUS_DI](/functions/minus_di.md)

## References

- J. Welles Wilder, *New Concepts in Technical Trading Systems*, Trend Research (ISBN 0894590278)
