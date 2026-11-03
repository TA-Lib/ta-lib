---
title: "Chandelier Exit (CE)"
description: "Le Beau's Chandelier Exit, a two-line volatility stop."
---

## Summary

Le Beau's Chandelier Exit, a two-line volatility stop. The highest high of the window is offset down, and the lowest low up, by a multiple of the Average True Range, so each line hangs from the recent extreme at a distance that widens as the true range grows. The long line is the exit for a long position and hangs below the recent highs; the short line is the mirror for a short position, above the recent lows.

## Formula

Long = MAX(High, TimePeriod) - Multiplier * ATR(TimePeriod)
Short = MIN(Low, TimePeriod) + Multiplier * ATR(TimePeriod)

## Notes

- The extremes are taken over the last TimePeriod bars. Le Beau anchors the highest high at the entry of the trade, which a function over a price series cannot know; the rolling window is what charting platforms plot under this name.
- The Average True Range is this library's, whose first value is the average of the first full period of true ranges that have a previous close. Platforms that start the true range on the very first bar give slightly different values on early bars; the difference decays as the average warms up.
- Both lines are emitted on every bar, each from the highs or the lows. A popular variant takes the extremes of the closes and keeps a single stop that only moves toward price until the trend flips; that variant is not this function.
- Neither line is always on its side of the other: with a small multiplier the long line sits above the short one.
- Both lines inherit the Average True Range's warm-up, so a caller who wants them converged sets `TA_FUNC_UNST_ATR`, exactly as when calling that function directly.
- The two lines are the Chande Kroll Stop with a stop period of 1, `outRealLong` being CKSP's `outHighStop`.

## Inputs

- `inHigh` — High price of each bar
- `inLow` — Low price of each bar
- `inClose` — Close price of each bar

## Outputs

- `outRealLong` — Highest high less the multiple of the Average True Range; the stop for a long position
- `outRealShort` — Lowest low plus the multiple of the Average True Range; the stop for a short position

## Parameters

| Parameter | Type | Default | Accepted values | Description |
| --- | --- | --- | --- | --- |
| `optInTimePeriod` | integer | 22 | 2–100000 | Window of the highest high and lowest low, and smoothing period of the Average True Range |
| `optInMultiplier` | real | 3 | ≥ 0 | Multiplier applied to the Average True Range to offset the lines |

## Properties

**Numerical Stability:** [Initial Unstable Period](/functions/stability.md#initial-unstable-period) — Inherited from ATR, which CE computes internally; tunable via ATR's unstable period.

<div class="flag-table">

|  |
| :-- |
| <span class="flag-box">✅</span> **Overlap Input** <span class="flag-tip" tabindex="0" role="note" aria-label="Output is on the same scale as the input price, so it is drawn over the price chart." data-tip="Output is on the same scale as the input price, so it is drawn over the price chart.">i</span> |
| <span class="flag-box">☐</span> <span style="opacity:0.5">Independent Y-Axis</span> |
| <span class="flag-box">☐</span> <span style="opacity:0.5">Candlestick</span> |
| <span class="flag-box">☐</span> <span style="opacity:0.5">Can Output NaN or ±Inf</span> |
| <span class="flag-box">☐</span> <span style="opacity:0.5">Identity at Period 1</span> |
| <span class="flag-box">☐</span> <span style="opacity:0.5">Display Shift</span> |
| <span class="flag-box">☐</span> <span style="opacity:0.5">Uses Transcendental</span> |
| <span class="flag-box">☐</span> <span style="opacity:0.5">Input Must Be Positive</span> |

</div>

## Implementation

TA-Lib Definition: [`ce.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/ce/ce.c) · [`ce.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/ce/ce.yaml)

| Native | File |
|--------|------|
| C | [`ta_CE.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_CE.c) |
| Rust | [`ce.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/library/src/ta_func/ce.rs) |
| Java | [`Core_CE.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/fragments/Core_CE.java) |
| C# | [`Core_CE.cs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/csharp/library/src/Core_CE.cs) |
| TypeScript | [`ce.ts`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/ts/library/src/func/ce.ts) |

TA-Lib is also available for Python, R and more using a [wrapper](/install/#wrappers).

## Aliases

Chandelier Exit, Chandelier Stop, Le Beau Chandelier Exit

## See Also

[ATR](/functions/atr.md) · [MAX](/functions/max.md) · [MIN](/functions/min.md) · [CKSP](/functions/cksp.md) · [SUPERTREND](/functions/supertrend.md) · [KC](/functions/kc.md) · [SAR](/functions/sar.md)

## References

- Alexander Elder, *Come Into My Trading Room*, John Wiley & Sons, 2002, where Charles Le Beau's method is described
- [Chandelier Exit (StockCharts ChartSchool)](https://chartschool.stockcharts.com/table-of-contents/technical-indicators-and-overlays/technical-overlays/chandelier-exit)
