---
title: "Vector Trigonometric ACos (ACOS)"
description: "Element-wise arc cosine of the input series."
---

## Summary

Element-wise arc cosine of the input series.

## Formula

outReal[i] = acos(inReal[i])

## Notes

- Outside [-1, 1] there is no angle whose cosine is that value, so those elements come out NaN.

## Inputs

- `inReal` — input values (expected in [-1, 1])

## Outputs

- `outReal` — arc cosine of each input, in radians

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

TA-Lib Definition: [`acos.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/acos/acos.c) · [`acos.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/acos/acos.yaml)

| Native | File |
|--------|------|
| C | [`ta_ACOS.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_ACOS.c) |
| Rust | [`acos.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/library/src/ta_func/acos.rs) |
| Java | [`Core_ACOS.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/fragments/Core_ACOS.java) |

TA-Lib is also available for Python, R and more using a [wrapper](/install/#wrappers).

## Aliases

Arc Cosine, Inverse Cosine, arccos

## See Also

[COS](/functions/cos.md) · [ASIN](/functions/asin.md) · [ATAN](/functions/atan.md)

## References

- Wikipedia, *Inverse trigonometric functions*: [en.wikipedia.org/wiki/Inverse_trigonometric_functions](https://en.wikipedia.org/wiki/Inverse_trigonometric_functions)
