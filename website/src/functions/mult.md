---
title: "Vector Arithmetic Mult (MULT)"
description: "Element-wise multiplication of two input series. Produces outReal[i] = inReal0[i] * inReal1[i]."
---

## Summary

Element-wise multiplication of two input series. Produces outReal[i] = inReal0[i] * inReal1[i].

## Formula

outReal[i] = inReal0[i] * inReal1[i]

## Inputs

- `inReal0` — First operand series
- `inReal1` — Second operand series

## Outputs

- `outReal` — Product of the two inputs at each index

## Properties

**Numerical Stability:** [Start-Independent](/functions/stability.md#start-independent)

| Display<br>Flags |
| :-- |
| <span class="flag-box">☐</span> <span style="opacity:0.5">Overlap Input</span> |
| <span class="flag-box">✅</span> **Independent Y-Axis** <span class="flag-tip" tabindex="0" role="note" aria-label="Output is on its own scale, drawn in a separate pane below the price chart." data-tip="Output is on its own scale, drawn in a separate pane below the price chart.">i</span> |
| <span class="flag-box">☐</span> <span style="opacity:0.5">Candlestick</span> |

## Implementation

TA-Lib Definition: [`mult.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/mult/mult.c) · [`mult.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/mult/mult.yaml)

| Native | File |
|--------|------|
| C | [`ta_MULT.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_MULT.c) |
| Rust | [`mult.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/library/src/ta_func/mult.rs) |
| Java | [`Core_MULT.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/fragments/Core_MULT.java) |

TA-Lib is also available for Python, R and more using a [wrapper](/install/#wrappers).

## Aliases

Vector Multiply, Vector Arithmetic Mult, Element-wise Product

## See Also

[ADD](/functions/add.md) · [SUB](/functions/sub.md) · [DIV](/functions/div.md)
