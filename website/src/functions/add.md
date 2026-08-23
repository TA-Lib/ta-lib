---
title: "Vector Arithmetic Add (ADD)"
description: "Element-wise addition of two input series."
---

## Summary

Element-wise addition of two input series.

## Formula

outReal[i] = inReal0[i] + inReal1[i]

## Inputs

- `inReal0` — First operand series
- `inReal1` — Second operand series

## Outputs

- `outReal` — Element-wise sum of the two inputs

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

TA-Lib Definition: [`add.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/add/add.c) · [`add.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/add/add.yaml)

| Native | File |
|--------|------|
| C | [`ta_ADD.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_ADD.c) |
| Rust | [`add.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/library/src/ta_func/add.rs) |
| Java | [`Core_ADD.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/fragments/Core_ADD.java) |

TA-Lib is also available for Python, R and more using a [wrapper](/install/#wrappers).

## Aliases

Vector Add, Vector Arithmetic Add

## See Also

[SUB](/functions/sub.md) · [MULT](/functions/mult.md) · [DIV](/functions/div.md)
