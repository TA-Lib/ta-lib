---
title: "Indexes of lowest and highest values over a specified period (MINMAXINDEX)"
description: "Returns the absolute input indices of the lowest and highest values within each rolling window of optInTimePeriod bars. Index variant of MINMAX."
---

## Summary

Returns the absolute input indices of the lowest and highest values within each rolling window of optInTimePeriod bars. Index variant of MINMAX.

## Formula

outMinIdx[i] = index of min(inReal[i-optInTimePeriod+1 .. i])  
outMaxIdx[i] = index of max(inReal[i-optInTimePeriod+1 .. i])

## Notes

- When several bars in a window share the extreme value, the index of one of them is returned — not necessarily the first or the last.

## Inputs

- `inReal` — Input series scanned for extremes

## Outputs

- `outMinIdx` — Absolute index (into inReal) of the window minimum
- `outMaxIdx` — Absolute index (into inReal) of the window maximum

## Parameters

| Parameter | Type | Default | Accepted values | Description |
| --- | --- | --- | --- | --- |
| `optInTimePeriod` | integer | 30 | 2–100000 | Window length in bars |

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

TA-Lib Definition: [`minmaxindex.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/minmaxindex/minmaxindex.c) · [`minmaxindex.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/minmaxindex/minmaxindex.yaml)

| Native | File |
|--------|------|
| C | [`ta_MINMAXINDEX.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_MINMAXINDEX.c) |
| Rust | [`minmaxindex.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/library/src/ta_func/minmaxindex.rs) |
| Java | [`Core_MINMAXINDEX.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/fragments/Core_MINMAXINDEX.java) |

TA-Lib is also available for Python, R and more using a [wrapper](/install/#wrappers).

## Aliases

Lowest/Highest Index

## See Also

[MINMAX](/functions/minmax.md) · [MIN](/functions/min.md) · [MAX](/functions/max.md) · [MININDEX](/functions/minindex.md) · [MAXINDEX](/functions/maxindex.md)
