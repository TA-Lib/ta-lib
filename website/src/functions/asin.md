---
title: ASIN
description: "Element-wise arcsine (inverse sine) of each input value. A vector math transform, not a market indicator."
---

# ASIN

## Summary

Element-wise arcsine (inverse sine) of each input value. A vector math transform, not a market indicator.

## Formula

outReal[i] = asin(inReal[i])

## Inputs

- `inReal` — Input values (domain [-1,1] for a real result)

## Outputs

- `outReal` — Arcsine of each input, in radians

## Properties

**Numerical Stability:** [Start-Independent](/functions/stability#start-independent)

| Display<br>Flags |
| :-- |
| <span class="flag-box">☐</span> <span style="opacity:0.5">Overlap Input</span> |
| <span class="flag-box">✅</span> **Independent Y-Axis** <span class="flag-tip" tabindex="0" role="note" aria-label="Output is on its own scale, drawn in a separate pane below the price chart." data-tip="Output is on its own scale, drawn in a separate pane below the price chart.">i</span> |
| <span class="flag-box">☐</span> <span style="opacity:0.5">Candlestick</span> |

## Implementation

TA-Lib Definition: [`asin.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/asin/asin.c) · [`asin.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/asin/asin.yaml)

| Native | File |
|--------|------|
| C | [`ta_ASIN.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_ASIN.c) |
| Rust | [`asin.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/library/src/ta_func/asin.rs) |
| Java | [`Core_ASIN.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/library/fragments/Core_ASIN.java) |

TA-Lib is also available for Python, R and more using a [wrapper](https://ta-lib.org/wrappers/).

## Aliases

arcsine, inverse sine

## See Also

[ACOS](/functions/acos) · [ATAN](/functions/atan) · [SIN](/functions/sin) · [COS](/functions/cos)
