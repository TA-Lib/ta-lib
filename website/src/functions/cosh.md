---
title: "Vector Trigonometric Cosh (COSH)"
description: "Vector hyperbolic cosine: applies cosh element-wise to each input value. A Math Transform primitive with no lookback."
---

## Summary

Vector hyperbolic cosine: applies cosh element-wise to each input value. A Math Transform primitive with no lookback.

## Formula

outReal[i] = cosh(inReal[i]) = (e^{inReal[i]} + e^{-inReal[i]}) / 2

## Inputs

- `inReal` — Input values to transform

## Outputs

- `outReal` — Hyperbolic cosine of each input

## Properties

**Numerical Stability:** [Start-Independent](/functions/stability.md#start-independent)

| Display<br>Flags |
| :-- |
| <span class="flag-box">☐</span> <span style="opacity:0.5">Overlap Input</span> |
| <span class="flag-box">✅</span> **Independent Y-Axis** <span class="flag-tip" tabindex="0" role="note" aria-label="Output is on its own scale, drawn in a separate pane below the price chart." data-tip="Output is on its own scale, drawn in a separate pane below the price chart.">i</span> |
| <span class="flag-box">☐</span> <span style="opacity:0.5">Candlestick</span> |

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
