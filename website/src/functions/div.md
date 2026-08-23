---
title: "Vector Arithmetic Div (DIV)"
description: "Element-wise division of two input series."
---

## Summary

Element-wise division of two input series.

## Formula

outReal[i] = inReal0[i] / inReal1[i]

## Notes

- Zero divided by zero gives NaN; anything else divided by zero gives positive or negative infinity. Neither is reported as an error.

## Inputs

- `inReal0` — Dividend (numerator) series
- `inReal1` — Divisor (denominator) series

## Outputs

- `outReal` — Per-element quotient inReal0/inReal1

## Properties

**Numerical Stability:** [Start-Independent](/functions/stability.md#start-independent)

<div class="flag-table">

|  |
| :-- |
| <span class="flag-box">☐</span> <span style="opacity:0.5">Overlap Input</span> |
| <span class="flag-box">✅</span> **Independent Y-Axis** <span class="flag-tip" tabindex="0" role="note" aria-label="Output is on its own scale, drawn in a separate pane below the price chart." data-tip="Output is on its own scale, drawn in a separate pane below the price chart.">i</span> |
| <span class="flag-box">☐</span> <span style="opacity:0.5">Candlestick</span> |
| <span class="flag-box">✅</span> **Can Output NaN or ±Inf** <span class="flag-tip" tabindex="0" role="note" aria-label="Some inputs have no finite result, so a successful call can return NaN or ±Inf — a gap with nothing to plot. See Notes for when." data-tip="Some inputs have no finite result, so a successful call can return NaN or ±Inf — a gap with nothing to plot. See Notes for when.">i</span> |
| <span class="flag-box">☐</span> <span style="opacity:0.5">Identity at Period 1</span> |

</div>

## Implementation

TA-Lib Definition: [`div.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/div/div.c) · [`div.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/div/div.yaml)

| Native | File |
|--------|------|
| C | [`ta_DIV.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_DIV.c) |
| Rust | [`div.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/library/src/ta_func/div.rs) |
| Java | [`Core_DIV.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/fragments/Core_DIV.java) |

TA-Lib is also available for Python, R and more using a [wrapper](/install/#wrappers).

## Aliases

Vector Arithmetic Divide, Divide

## See Also

[MULT](/functions/mult.md) · [ADD](/functions/add.md) · [SUB](/functions/sub.md)
