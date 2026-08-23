---
title: "Vector Log10 (LOG10)"
description: "Element-wise base-10 logarithm of the input series."
---

## Summary

Element-wise base-10 logarithm of the input series.

## Formula

outReal[i] = log10(inReal[i])

## Notes

- The logarithm is defined only for positive values: a negative input gives NaN, and a zero input gives negative infinity.

## Inputs

- `inReal` — Input values

## Outputs

- `outReal` — Base-10 logarithm of each input

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

TA-Lib Definition: [`log10.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/log10/log10.c) · [`log10.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/log10/log10.yaml)

| Native | File |
|--------|------|
| C | [`ta_LOG10.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_LOG10.c) |
| Rust | [`log10.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/library/src/ta_func/log10.rs) |
| Java | [`Core_LOG10.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/fragments/Core_LOG10.java) |

TA-Lib is also available for Python, R and more using a [wrapper](/install/#wrappers).

## Aliases

Log Base 10, Common Logarithm

## See Also

[LN](/functions/ln.md) · [EXP](/functions/exp.md)

## References

- Wikipedia, *Common logarithm*: [en.wikipedia.org/wiki/Common_logarithm](https://en.wikipedia.org/wiki/Common_logarithm)
