---
title: "Vector Trigonometric ATan (ATAN)"
description: "Element-wise arctangent of the input series."
---

## Summary

Element-wise arctangent of the input series.

## Chart

<ta-chart src="/functions/atan.svg" ratio="720:416" alt="Drawn on the price pane over the close as a line; below it, a separate indicator pane showing outReal as a solid line."></ta-chart>

Drawn on the price pane over the close as a line; below it, a separate indicator pane showing outReal as a solid line. [Reference data (CSV)](/functions/atan.csv) · [Chart (SVG)](/functions/atan.svg)

## Formula

outReal[i] = atan(inReal[i])  (radians, range (-pi/2, pi/2))

## Inputs

- `inReal` — Input values

## Outputs

- `outReal` — Arc tangent of each input, in radians

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

TA-Lib Definition: [`atan.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/atan/atan.c) · [`atan.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/atan/atan.yaml)

| Native | File |
|--------|------|
| C | [`ta_ATAN.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_ATAN.c) |
| Rust | [`atan.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/library/src/ta_func/atan.rs) |
| Java | [`Core_ATAN.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/fragments/Core_ATAN.java) |
| C# | [`Core_ATAN.cs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/csharp/library/src/Core_ATAN.cs) |
| TypeScript | [`atan.ts`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/ts/library/src/func/atan.ts) |

Python: `talib.ATAN`, see [ta-lib-python](https://ta-lib.github.io/ta-lib-python/func_groups/math_transform.html).

TA-Lib is also available for R and more using a [wrapper](/install/#wrappers).

## Aliases

arctangent, arctan, inverse tangent

## See Also

[TAN](/functions/tan.md) · [ACOS](/functions/acos.md) · [ASIN](/functions/asin.md)

## References

- Wikipedia, *Inverse trigonometric functions*: [en.wikipedia.org/wiki/Inverse_trigonometric_functions](https://en.wikipedia.org/wiki/Inverse_trigonometric_functions)
