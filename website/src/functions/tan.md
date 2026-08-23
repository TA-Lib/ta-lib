---
title: "Vector Trigonometric Tan (TAN)"
description: "Element-wise tangent of the input series."
---

## Summary

Element-wise tangent of the input series.

## Formula

outReal[i] = tan(inReal[i])

## Inputs

- `inReal` — input values

## Outputs

- `outReal` — tangent of each input

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

TA-Lib Definition: [`tan.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/tan/tan.c) · [`tan.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/tan/tan.yaml)

| Native | File |
|--------|------|
| C | [`ta_TAN.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_TAN.c) |
| Rust | [`tan.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/library/src/ta_func/tan.rs) |
| Java | [`Core_TAN.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/fragments/Core_TAN.java) |

TA-Lib is also available for Python, R and more using a [wrapper](/install/#wrappers).

## Aliases

tangent

## See Also

[ATAN](/functions/atan.md) · [SIN](/functions/sin.md) · [COS](/functions/cos.md) · [TANH](/functions/tanh.md)

## References

- Wikipedia, *Trigonometric functions*: [en.wikipedia.org/wiki/Trigonometric_functions](https://en.wikipedia.org/wiki/Trigonometric_functions)
