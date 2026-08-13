---
title: "Index of highest value over a specified period (MAXINDEX)"
description: "Returns the index of the highest input value within a rolling window of optInTimePeriod bars. Same as MAX but outputs the location instead of the value."
---

## Summary

Returns the index of the highest input value within a rolling window of optInTimePeriod bars. Same as MAX but outputs the location instead of the value.

## Formula

outInteger[i] = argmax_{j in [i-optInTimePeriod+1, i]} inReal[j]

## Notes

- When several bars in a window share the highest value, which bar's index is returned is not guaranteed to be a specific one of the tied bars.

## Inputs

- `inReal` — Input series to scan

## Outputs

- `outInteger` — Absolute index (into inReal) of the highest value in each window

## Parameters

| Parameter | Type | Default | Accepted values | Description |
| --- | --- | --- | --- | --- |
| `optInTimePeriod` | integer | 30 | 2–100000 | Window length over which the max is located |

## Properties

**Numerical Stability:** [Start-Independent](/functions/stability.md#start-independent)

<div class="flag-table">

|  |
| :-- |
| <span class="flag-box">☐</span> <span style="opacity:0.5">Overlap Input</span> |
| <span class="flag-box">✅</span> **Independent Y-Axis** <span class="flag-tip" tabindex="0" role="note" aria-label="Output is on its own scale, drawn in a separate pane below the price chart." data-tip="Output is on its own scale, drawn in a separate pane below the price chart.">i</span> |
| <span class="flag-box">☐</span> <span style="opacity:0.5">Candlestick</span> |
| <span class="flag-box">☐</span> <span style="opacity:0.5">Can Output NaN or ±Inf</span> |
| <span class="flag-box">☐</span> <span style="opacity:0.5">Identity at Period 1</span> |

</div>

## Implementation

TA-Lib Definition: [`maxindex.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/maxindex/maxindex.c) · [`maxindex.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/maxindex/maxindex.yaml)

| Native | File |
|--------|------|
| C | [`ta_MAXINDEX.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_MAXINDEX.c) |
| Rust | [`maxindex.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/library/src/ta_func/maxindex.rs) |
| Java | [`Core_MAXINDEX.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/fragments/Core_MAXINDEX.java) |

TA-Lib is also available for Python, R and more using a [wrapper](/install/#wrappers).

## Aliases

Index of Highest Value, Highest Value Index, argmax

## See Also

[MAX](/functions/max.md) · [MININDEX](/functions/minindex.md) · [MIN](/functions/min.md) · [MINMAXINDEX](/functions/minmaxindex.md)
