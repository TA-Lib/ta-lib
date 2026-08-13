---
title: "Highest value over a specified period (MAX)"
description: "Highest input value over a rolling window of the last optInTimePeriod bars. A moving-window maximum."
---

## Summary

Highest input value over a rolling window of the last optInTimePeriod bars. A moving-window maximum.

## Formula

outReal[i] = max(inReal[i-optInTimePeriod+1 .. i])

## Inputs

- `inReal` — Series to take the rolling maximum of

## Outputs

- `outReal` — Highest value within each trailing window

## Parameters

| Parameter | Type | Default | Accepted values | Description |
| --- | --- | --- | --- | --- |
| `optInTimePeriod` | integer | 30 | 2–100000 | Window length in bars |

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

TA-Lib Definition: [`max.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/max/max.c) · [`max.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/max/max.yaml)

| Native | File |
|--------|------|
| C | [`ta_MAX.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_MAX.c) |
| Rust | [`max.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/library/src/ta_func/max.rs) |
| Java | [`Core_MAX.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/fragments/Core_MAX.java) |

TA-Lib is also available for Python, R and more using a [wrapper](/install/#wrappers).

## Aliases

Highest, Highest High, Rolling Maximum

## See Also

[MIN](/functions/min.md) · [MAXINDEX](/functions/maxindex.md) · [MINMAX](/functions/minmax.md)
