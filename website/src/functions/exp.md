---
title: "Vector Arithmetic Exp (EXP)"
description: "Element-wise base-e exponential of the input series."
---

## Summary

Element-wise base-e exponential of the input series.

## Formula

outReal[i] = exp(inReal[i]) = e^{inReal[i]}

## Inputs

- `inReal` — Input values

## Outputs

- `outReal` — e raised to each input value

## Properties

**Numerical Stability:** [Start-Independent](/functions/stability.md#start-independent)

<div class="flag-table">

|  |
| :-- |
| <span class="flag-box">☐</span> <span style="opacity:0.5">Overlap Input</span> |
| <span class="flag-box">✅</span> **Independent Y-Axis** <span class="flag-tip" tabindex="0" role="note" aria-label="Output is on its own scale, drawn in a separate pane below the price chart." data-tip="Output is on its own scale, drawn in a separate pane below the price chart.">i</span> |
| <span class="flag-box">☐</span> <span style="opacity:0.5">Candlestick</span> |
| <span class="flag-box">☐</span> <span style="opacity:0.5">Can Output NaN or ±Inf</span> |
| <span class="flag-box">☐</span> <span style="opacity:0.5">Identity at Period 1</span> |

</div>

## Implementation

TA-Lib Definition: [`exp.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/exp/exp.c) · [`exp.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/exp/exp.yaml)

| Native | File |
|--------|------|
| C | [`ta_EXP.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_EXP.c) |
| Rust | [`exp.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/library/src/ta_func/exp.rs) |
| Java | [`Core_EXP.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/fragments/Core_EXP.java) |

TA-Lib is also available for Python, R and more using a [wrapper](/install/#wrappers).

## Aliases

exponential, e^x

## See Also

[LN](/functions/ln.md) · [SQRT](/functions/sqrt.md)

## References

- Wikipedia, *Exponential function*: [en.wikipedia.org/wiki/Exponential_function](https://en.wikipedia.org/wiki/Exponential_function)
