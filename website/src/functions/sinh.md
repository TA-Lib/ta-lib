---
title: SINH
description: "Element-wise hyperbolic sine of the input series. A vector math transform applying sinh() to each value."
---

# SINH

## Summary

Element-wise hyperbolic sine of the input series. A vector math transform applying sinh() to each value.

## Formula

outReal[i] = sinh(inReal[i])

## Inputs

- `inReal` — Input series

## Outputs

- `outReal` — Hyperbolic sine of each input value

## Properties

**Numerical Stability:** [Start-Independent](/functions/stability#start-independent)

| Display<br>Flags |
| :-- |
| <span class="flag-box">☐</span> <span style="opacity:0.5">Overlap Input</span> |
| <span class="flag-box">✅</span> **Independent Y-Axis** <span class="flag-tip" tabindex="0" role="note" aria-label="Output is on its own scale, drawn in a separate pane below the price chart." data-tip="Output is on its own scale, drawn in a separate pane below the price chart.">i</span> |
| <span class="flag-box">☐</span> <span style="opacity:0.5">Candlestick</span> |

## Implementation

TA-Lib Definition: [`sinh.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/sinh/sinh.c) · [`sinh.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/sinh/sinh.yaml)

| Native | File |
|--------|------|
| C | [`ta_SINH.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_SINH.c) |
| Rust | [`sinh.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/library/src/ta_func/sinh.rs) |
| Java | [`Core_SINH.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/fragments/Core_SINH.java) |

TA-Lib is also available for Python, R and more using a [wrapper](https://ta-lib.org/wrappers/).

## Aliases

Hyperbolic Sine

## See Also

[COSH](/functions/cosh) · [TANH](/functions/tanh)
