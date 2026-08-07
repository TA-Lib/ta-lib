---
title: "Vector Arithmetic Subtraction (SUB)"
description: "Element-wise vector subtraction of two input series. Outputs inReal0 minus inReal1 at each index."
---

## Summary

Element-wise vector subtraction of two input series. Outputs inReal0 minus inReal1 at each index.

## Formula

outReal[i] = inReal0[i] - inReal1[i]

## Inputs

- `inReal0` — Minuend series
- `inReal1` — Subtrahend series

## Outputs

- `outReal` — Per-element difference inReal0 - inReal1

## Properties

**Numerical Stability:** [Start-Independent](/functions/stability.md#start-independent)

| Display<br>Flags |
| :-- |
| <span class="flag-box">☐</span> <span style="opacity:0.5">Overlap Input</span> |
| <span class="flag-box">✅</span> **Independent Y-Axis** <span class="flag-tip" tabindex="0" role="note" aria-label="Output is on its own scale, drawn in a separate pane below the price chart." data-tip="Output is on its own scale, drawn in a separate pane below the price chart.">i</span> |
| <span class="flag-box">☐</span> <span style="opacity:0.5">Candlestick</span> |

## Implementation

TA-Lib Definition: [`sub.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/sub/sub.c) · [`sub.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/sub/sub.yaml)

| Native | File |
|--------|------|
| C | [`ta_SUB.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_SUB.c) |
| Rust | [`sub.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/library/src/ta_func/sub.rs) |
| Java | [`Core_SUB.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/fragments/Core_SUB.java) |

TA-Lib is also available for Python, R and more using a [wrapper](/install/#wrappers).

## Aliases

Subtract, Vector Subtraction

## See Also

[ADD](/functions/add.md) · [MULT](/functions/mult.md) · [DIV](/functions/div.md)
