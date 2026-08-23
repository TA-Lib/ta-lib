---
title: "Vector Trigonometric Tanh (TANH)"
description: "Element-wise hyperbolic tangent of the input series."
---

## Summary

Element-wise hyperbolic tangent of the input series.

## Formula

outReal[i] = tanh(inReal[i])

## Inputs

- `inReal` — Input value series

## Outputs

- `outReal` — Hyperbolic tangent of each input

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

TA-Lib Definition: [`tanh.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/tanh/tanh.c) · [`tanh.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/tanh/tanh.yaml)

| Native | File |
|--------|------|
| C | [`ta_TANH.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_TANH.c) |
| Rust | [`tanh.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/library/src/ta_func/tanh.rs) |
| Java | [`Core_TANH.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/fragments/Core_TANH.java) |

TA-Lib is also available for Python, R and more using a [wrapper](/install/#wrappers).

## Aliases

Hyperbolic Tangent

## See Also

[SINH](/functions/sinh.md) · [COSH](/functions/cosh.md) · [TAN](/functions/tan.md)

## References

- Wikipedia, *Hyperbolic functions*: [en.wikipedia.org/wiki/Hyperbolic_functions](https://en.wikipedia.org/wiki/Hyperbolic_functions)
