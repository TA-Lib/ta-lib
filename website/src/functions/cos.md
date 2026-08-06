---
title: COS
description: "Element-wise trigonometric cosine of the input series. Applies the C library cos() to each sample."
---

# COS

## Summary

Element-wise trigonometric cosine of the input series. Applies the C library cos() to each sample.

## Formula

outReal[i] = cos(inReal[i])

## Inputs

- `inReal` — Input values, treated as angles in radians

## Outputs

- `outReal` — Cosine of each input value

## Properties

**Numerical Stability:** [Start-Independent](/functions/stability#start-independent)

| Display<br>Flags |
| :-- |
| <span class="flag-box">☐</span> <span style="opacity:0.5">Overlap Input</span> |
| <span class="flag-box">✅</span> **Independent Y-Axis** <span class="flag-tip" tabindex="0" role="note" aria-label="Output is on its own scale, drawn in a separate pane below the price chart." data-tip="Output is on its own scale, drawn in a separate pane below the price chart.">i</span> |
| <span class="flag-box">☐</span> <span style="opacity:0.5">Candlestick</span> |

## Implementation

TA-Lib Definition: [`cos.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/cos/cos.c) · [`cos.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/cos/cos.yaml)

| Native | File |
|--------|------|
| C | [`ta_COS.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_COS.c) |
| Rust | [`cos.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/library/src/ta_func/cos.rs) |
| Java | [`Core_COS.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/fragments/Core_COS.java) |

TA-Lib is also available for Python, R and more using a [wrapper](https://ta-lib.org/wrappers/).

## Aliases

Cosine, Vector Trigonometric Cos

## See Also

[ACOS](/functions/acos) · [SIN](/functions/sin) · [TAN](/functions/tan) · [COSH](/functions/cosh)
