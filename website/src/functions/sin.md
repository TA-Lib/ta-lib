---
title: SIN
description: "Vector trigonometric sine: applies sin() element-wise to each input value. Part of the Math Transform group."
---

# SIN

## Summary

Vector trigonometric sine: applies sin() element-wise to each input value. Part of the Math Transform group.

## Formula

outReal[i] = sin(inReal[i])

## Inputs

- `inReal` — Input values (radians)

## Outputs

- `outReal` — Sine of each input

## Properties

**Numerical Stability:** [Start-Independent](/functions/stability#start-independent)

| Display<br>Flags |
| :-- |
| <span class="flag-box">☐</span> <span style="opacity:0.5">Overlap Input</span> |
| <span class="flag-box">✅</span> **Independent Y-Axis** <span class="flag-tip" tabindex="0" role="note" aria-label="Output is on its own scale, drawn in a separate pane below the price chart." data-tip="Output is on its own scale, drawn in a separate pane below the price chart.">i</span> |
| <span class="flag-box">☐</span> <span style="opacity:0.5">Candlestick</span> |

## Implementation

TA-Lib Definition: [`sin.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/sin/sin.c) · [`sin.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/sin/sin.yaml)

| Native | File |
|--------|------|
| C | [`ta_SIN.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_SIN.c) |
| Rust | [`sin.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/library/src/ta_func/sin.rs) |
| Java | [`Core_SIN.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/library/fragments/Core_SIN.java) |

TA-Lib is also available for Python, R and more using a [wrapper](https://ta-lib.org/wrappers/).

## Aliases

sine

## See Also

[COS](/functions/cos) · [TAN](/functions/tan) · [ASIN](/functions/asin)
