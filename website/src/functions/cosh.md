---
title: "Vector Trigonometric Cosh (COSH)"
description: "Element-wise hyperbolic cosine of the input series."
---

## Summary

Element-wise hyperbolic cosine of the input series.

## Formula

outReal[i] = cosh(inReal[i]) = (e^{inReal[i]} + e^{-inReal[i]}) / 2

## Inputs

- `inReal` — Input values to transform

## Outputs

- `outReal` — Hyperbolic cosine of each input

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

TA-Lib Definition: [`cosh.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/cosh/cosh.c) · [`cosh.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/cosh/cosh.yaml)

| Native | File |
|--------|------|
| C | [`ta_COSH.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_COSH.c) |
| Rust | [`cosh.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/library/src/ta_func/cosh.rs) |
| Java | [`Core_COSH.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/fragments/Core_COSH.java) |

TA-Lib is also available for Python, R and more using a [wrapper](/install/#wrappers).

## Aliases

Hyperbolic Cosine

## See Also

[SINH](/functions/sinh.md) · [TANH](/functions/tanh.md) · [COS](/functions/cos.md)

## References

- Wikipedia, *Hyperbolic functions*: [en.wikipedia.org/wiki/Hyperbolic_functions](https://en.wikipedia.org/wiki/Hyperbolic_functions)
