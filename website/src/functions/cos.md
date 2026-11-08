---
title: "Vector Trigonometric Cos (COS)"
description: "Element-wise cosine of the input series."
---

## Summary

Element-wise cosine of the input series.

## Chart

<ta-chart src="/functions/cos.svg" ratio="720:416" alt="Drawn on the price pane over the close as a line; below it, a separate indicator pane with a zero line showing outReal as a solid line."></ta-chart>

Drawn on the price pane over the close as a line; below it, a separate indicator pane with a zero line showing outReal as a solid line. [Reference data (CSV)](/functions/cos.csv) · [Chart (SVG)](/functions/cos.svg)

## Formula

outReal[i] = cos(inReal[i])

## Inputs

- `inReal` — Input values, treated as angles in radians

## Outputs

- `outReal` — Cosine of each input value

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

TA-Lib Definition: [`cos.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/cos/cos.c) · [`cos.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/cos/cos.yaml)

| Native | File |
|--------|------|
| C | [`ta_COS.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_COS.c) |
| Rust | [`cos.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/library/src/ta_func/cos.rs) |
| Java | [`Core_COS.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/fragments/Core_COS.java) |
| C# | [`Core_COS.cs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/csharp/library/src/Core_COS.cs) |
| TypeScript | [`cos.ts`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/ts/library/src/func/cos.ts) |

Python: `talib.COS`, see [ta-lib-python](https://ta-lib.github.io/ta-lib-python/func_groups/math_transform.html).

TA-Lib is also available for R and more using a [wrapper](/install/#wrappers).

## Aliases

Cosine, Vector Trigonometric Cos

## See Also

[ACOS](/functions/acos.md) · [SIN](/functions/sin.md) · [TAN](/functions/tan.md) · [COSH](/functions/cosh.md)

## References

- Wikipedia, *Trigonometric functions*: [en.wikipedia.org/wiki/Trigonometric_functions](https://en.wikipedia.org/wiki/Trigonometric_functions)
