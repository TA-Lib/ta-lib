---
title: "Vector Log Natural (LN)"
description: "Vector natural logarithm: applies the natural log (base e) elementwise to the input series."
---

## Summary

Vector natural logarithm: applies the natural log (base e) elementwise to the input series.

## Formula

outReal[i] = log(inReal[i])

## Inputs

- `inReal` — Input value series

## Outputs

- `outReal` — Natural log of each input value

## Properties

**Numerical Stability:** [Start-Independent](/functions/stability.md#start-independent)

| Display<br>Flags |
| :-- |
| <span class="flag-box">☐</span> <span style="opacity:0.5">Overlap Input</span> |
| <span class="flag-box">✅</span> **Independent Y-Axis** <span class="flag-tip" tabindex="0" role="note" aria-label="Output is on its own scale, drawn in a separate pane below the price chart." data-tip="Output is on its own scale, drawn in a separate pane below the price chart.">i</span> |
| <span class="flag-box">☐</span> <span style="opacity:0.5">Candlestick</span> |

## Implementation

TA-Lib Definition: [`ln.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/ln/ln.c) · [`ln.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/ln/ln.yaml)

| Native | File |
|--------|------|
| C | [`ta_LN.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_LN.c) |
| Rust | [`ln.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/library/src/ta_func/ln.rs) |
| Java | [`Core_LN.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/fragments/Core_LN.java) |

TA-Lib is also available for Python, R and more using a [wrapper](/install/#wrappers).

## Aliases

Natural Log, Vector Log Natural, Log

## See Also

[LOG10](/functions/log10.md) · [EXP](/functions/exp.md) · [SQRT](/functions/sqrt.md)
