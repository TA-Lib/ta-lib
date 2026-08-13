---
title: "Pearson's Correlation Coefficient (r) (CORREL)"
description: "Pearson's correlation coefficient (r) between two input series over a rolling window of optInTimePeriod bars."
---

## Summary

Pearson's correlation coefficient (r) between two input series over a rolling window of optInTimePeriod bars. Measures how linearly the two series move together. r near +1: strong positive co-movement; near -1: strong inverse; near 0: no linear relationship.

## Formula

r = (sumXY - sumX*sumY/n) / sqrt((sumX2 - sumX^2/n) * (sumY2 - sumY^2/n)),  n = optInTimePeriod, sums over the window

## Notes

- When the correlation is undefined for a window (for example a constant series), the output is 0 rather than an error or NaN.

## Inputs

- `inReal0` — First data series (X)
- `inReal1` — Second data series (Y)

## Outputs

- `outReal` — Correlation coefficient r in [-1, 1]

## Parameters

| Parameter | Type | Default | Accepted values | Description |
| --- | --- | --- | --- | --- |
| `optInTimePeriod` | integer | 30 | 1–100000 | Rolling window length |

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

TA-Lib Definition: [`correl.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/correl/correl.c) · [`correl.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/correl/correl.yaml)

| Native | File |
|--------|------|
| C | [`ta_CORREL.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_CORREL.c) |
| Rust | [`correl.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/library/src/ta_func/correl.rs) |
| Java | [`Core_CORREL.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/fragments/Core_CORREL.java) |

TA-Lib is also available for Python, R and more using a [wrapper](/install/#wrappers).

## Aliases

Pearson Correlation, Correlation Coefficient, r

## See Also

[BETA](/functions/beta.md) · [STDDEV](/functions/stddev.md) · [VAR](/functions/var.md)

## References

- Karl Pearson
