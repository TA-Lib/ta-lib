---
title: SQRT
description: "Vector square root: applies the square-root function element-wise to each input value."
---

# SQRT

## Summary

Vector square root: applies the square-root function element-wise to each input value.

## Formula

outReal[i] = sqrt(inReal[i])

## Inputs

- `inReal` — Input values

## Outputs

- `outReal` — Square root of each input value

## Properties

**Numerical Stability:** [Start-Independent](/functions/stability#start-independent)

| Display<br>Flags |
| :-- |
| <span class="flag-box">☐</span> <span style="opacity:0.5">Overlap Input</span> |
| <span class="flag-box">✅</span> **Independent Y-Axis** <span class="flag-tip" tabindex="0" role="note" aria-label="Output is on its own scale, drawn in a separate pane below the price chart." data-tip="Output is on its own scale, drawn in a separate pane below the price chart.">i</span> |
| <span class="flag-box">☐</span> <span style="opacity:0.5">Candlestick</span> |

## Implementation

TA-Lib Definition: [`sqrt.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/sqrt/sqrt.c) · [`sqrt.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/sqrt/sqrt.yaml)

| Native | File |
|--------|------|
| C | [`ta_SQRT.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_SQRT.c) |
| Rust | [`sqrt.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/library/src/ta_func/sqrt.rs) |
| Java | [`Core_SQRT.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/fragments/Core_SQRT.java) |

TA-Lib is also available for Python, R and more using a [wrapper](https://ta-lib.org/wrappers/).

## Aliases

Square Root
