---
url: 'https://ta-lib.org/functions/tanh.md'
description: Element-wise hyperbolic tangent of the input series.
---
# Vector Trigonometric Tanh (TANH)

## Summary

Element-wise hyperbolic tangent of the input series.

## Formula

outReal\[i] = tanh(inReal\[i])

## Inputs

* `inReal` — Input value series

## Outputs

* `outReal` — Hyperbolic tangent of each input

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
| <span class="flag-box">☐</span> <span style="opacity:0.5">Display Shift</span> |
| <span class="flag-box">✅</span> **Uses Transcendental** <span class="flag-tip" tabindex="0" role="note" aria-label="Calls a transcendental math function (exp, log, log10, trigonometric, inverse trigonometric or hyperbolic), so a value may differ slightly between languages, between a stream and a batch call, and between machines. TA-Lib targets a difference below 1e-9." data-tip="Calls a transcendental math function (exp, log, log10, trigonometric, inverse trigonometric or hyperbolic), so a value may differ slightly between languages, between a stream and a batch call, and between machines. TA-Lib targets a difference below 1e-9.">i</span> |

</div>

## Implementation

TA-Lib Definition: [`tanh.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/tanh/tanh.c) · [`tanh.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/tanh/tanh.yaml)

| Native | File |
|--------|------|
| C | [`ta_TANH.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_TANH.c) |
| Rust | [`tanh.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/library/src/ta_func/tanh.rs) |
| Java | [`Core_TANH.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/fragments/Core_TANH.java) |
| C# | [`Core_TANH.cs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/csharp/library/src/Core_TANH.cs) |

TA-Lib is also available for Python, R and more using a [wrapper](/install/#wrappers).

## Aliases

Hyperbolic Tangent

## See Also

[SINH](/functions/sinh.md) · [COSH](/functions/cosh.md) · [TAN](/functions/tan.md)

## References

* Wikipedia, *Hyperbolic functions*: [en.wikipedia.org/wiki/Hyperbolic_functions](https://en.wikipedia.org/wiki/Hyperbolic_functions)
