---
title: "Lowest and highest values over a specified period (MINMAX)"
description: "Returns both the lowest and highest values of the input over a rolling window of the last optInTimePeriod bars."
---

## Summary

Returns both the lowest and highest values of the input over a rolling window of the last optInTimePeriod bars. An overlap-study companion to MIN and MAX that computes both extrema in one pass.

## Inputs

- `inReal` — Values scanned for the window min and max

## Outputs

- `outMin` — Lowest value in each rolling window
- `outMax` — Highest value in each rolling window

## Parameters

| Parameter | Type | Default | Accepted values | Description |
| --- | --- | --- | --- | --- |
| `optInTimePeriod` | integer | 30 | 2–100000 | Rolling window length |

## Properties

**Numerical Stability:** [Start-Independent](/functions/stability.md#start-independent)

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

TA-Lib Definition: [`minmax.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/minmax/minmax.c) · [`minmax.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/minmax/minmax.yaml)

| Native | File |
|--------|------|
| C | [`ta_MINMAX.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_MINMAX.c) |
| Rust | [`minmax.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/library/src/ta_func/minmax.rs) |
| Java | [`Core_MINMAX.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/fragments/Core_MINMAX.java) |

TA-Lib is also available for Python, R and more using a [wrapper](/install/#wrappers).

## Aliases

Highest Lowest

## See Also

[MIN](/functions/min.md) · [MAX](/functions/max.md) · [MINMAXINDEX](/functions/minmaxindex.md) · [MININDEX](/functions/minindex.md) · [MAXINDEX](/functions/maxindex.md)
