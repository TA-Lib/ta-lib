---
title: "Vector Trigonometric ASin (ASIN)"
description: "Element-wise arcsine (inverse sine) of each input value. A vector math transform, not a market indicator."
---

## Summary

Element-wise arcsine (inverse sine) of each input value. A vector math transform, not a market indicator.

## Formula

outReal[i] = asin(inReal[i])

## Notes

- Outside [-1, 1] there is no angle whose sine is that value, so those elements come out NaN.

## Inputs

- `inReal` — Input values (domain [-1,1] for a real result)

## Outputs

- `outReal` — Arcsine of each input, in radians

## Properties

**Numerical Stability:** [Start-Independent](/functions/stability.md#start-independent)

<div class="flag-table">

|  |
| :-- |
| <span class="flag-box">☐</span> <span style="opacity:0.5">Overlap Input</span> |
| <span class="flag-box">✅</span> **Independent Y-Axis** <span class="flag-tip" tabindex="0" role="note" aria-label="Output is on its own scale, drawn in a separate pane below the price chart." data-tip="Output is on its own scale, drawn in a separate pane below the price chart.">i</span> |
| <span class="flag-box">☐</span> <span style="opacity:0.5">Candlestick</span> |
| <span class="flag-box">✅</span> **Can Output NaN or ±Inf** <span class="flag-tip" tabindex="0" role="note" aria-label="Some inputs have no finite result, so a successful call can return NaN or ±Inf — a gap with nothing to plot. See Notes for when." data-tip="Some inputs have no finite result, so a successful call can return NaN or ±Inf — a gap with nothing to plot. See Notes for when.">i</span> |
| <span class="flag-box">☐</span> <span style="opacity:0.5">Identity at Period 1</span> |

</div>

## Implementation

TA-Lib Definition: [`asin.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/asin/asin.c) · [`asin.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/asin/asin.yaml)

| Native | File |
|--------|------|
| C | [`ta_ASIN.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_ASIN.c) |
| Rust | [`asin.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/library/src/ta_func/asin.rs) |
| Java | [`Core_ASIN.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/fragments/Core_ASIN.java) |

TA-Lib is also available for Python, R and more using a [wrapper](/install/#wrappers).

## Aliases

arcsine, inverse sine

## See Also

[ACOS](/functions/acos.md) · [ATAN](/functions/atan.md) · [SIN](/functions/sin.md) · [COS](/functions/cos.md)
