---
title: "Vector Trigonometric Tanh (TANH)"
description: "Element-wise hyperbolic tangent of the input series."
---

## Summary

Element-wise hyperbolic tangent of the input series.

## Chart

<ta-chart src="/functions/tanh.svg" ratio="720:416" alt="Drawn on the price pane over the input, close->[-3,3], as a line; below it, a separate indicator pane with a zero line showing outReal as a solid line."></ta-chart>

Drawn on the price pane over the input, close->[-3,3], as a line; below it, a separate indicator pane with a zero line showing outReal as a solid line. [Reference data (CSV)](/functions/tanh.csv) · [Chart (SVG)](/functions/tanh.svg)

## Formula

outReal[i] = tanh(inReal[i])

## Inputs

- `inReal` — Input value series

## Outputs

- `outReal` — Hyperbolic tangent of each input

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
| <span class="flag-box">☐</span> <span style="opacity:0.5">Input Must Be Positive</span> |

</div>

## Implementation

Added in 0.4.0.

TA-Lib Definition: [`tanh.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/tanh/tanh.c) · [`tanh.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/tanh/tanh.yaml)

| Native | File |
|--------|------|
| C | [`ta_TANH.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_TANH.c) |
| Rust | [`tanh.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/library/src/ta_func/tanh.rs) |
| Java | [`Core_TANH.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/fragments/Core_TANH.java) |
| C# | [`Core_TANH.cs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/csharp/library/src/Core_TANH.cs) |
| TypeScript | [`tanh.ts`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/ts/library/src/func/tanh.ts) |

Python: `talib.TANH`, see [ta-lib-python](https://ta-lib.github.io/ta-lib-python/func_groups/math_transform.html).

TA-Lib is also available for R and more using a [wrapper](/install/#wrappers).

## Aliases

Hyperbolic Tangent

## See Also

[SINH](/functions/sinh.md) · [COSH](/functions/cosh.md) · [TAN](/functions/tan.md)

## References

- Wikipedia, *Hyperbolic functions*: [en.wikipedia.org/wiki/Hyperbolic_functions](https://en.wikipedia.org/wiki/Hyperbolic_functions)
