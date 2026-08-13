---
title: "Vector Ceil (CEIL)"
description: "Vector ceiling: element-wise ceiling of each input value (smallest integer >= input)."
---

## Summary

Vector ceiling: element-wise ceiling of each input value (smallest integer >= input).

## Formula

outReal[i] = ceil(inReal[i])

## Inputs

- `inReal` — Input values

## Outputs

- `outReal` — Each input rounded up to nearest integer

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

TA-Lib Definition: [`ceil.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/ceil/ceil.c) · [`ceil.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/ceil/ceil.yaml)

| Native | File |
|--------|------|
| C | [`ta_CEIL.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_CEIL.c) |
| Rust | [`ceil.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/library/src/ta_func/ceil.rs) |
| Java | [`Core_CEIL.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/fragments/Core_CEIL.java) |

TA-Lib is also available for Python, R and more using a [wrapper](/install/#wrappers).

## Aliases

Vector Ceil, Ceiling

## See Also

[FLOOR](/functions/floor.md)
