---
title: "Vector Arithmetic Add (ADD)"
description: "Element-wise addition of two input series."
---

## Summary

Element-wise addition of two input series.

## Chart

<ta-chart src="/functions/add.svg" ratio="720:416" alt="Drawn on the price pane over the close as a line; below it, a separate indicator pane showing outReal as a solid line."></ta-chart>

Drawn on the price pane over the close as a line; below it, a separate indicator pane showing outReal as a solid line. [Reference data (CSV)](/functions/add.csv) · [Chart (SVG)](/functions/add.svg)

## Formula

outReal[i] = inReal0[i] + inReal1[i]

## Inputs

- `inReal0` — First operand series
- `inReal1` — Second operand series

## Outputs

- `outReal` — Element-wise sum of the two inputs

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
| <span class="flag-box">☐</span> <span style="opacity:0.5">Uses Transcendental</span> |
| <span class="flag-box">☐</span> <span style="opacity:0.5">Input Must Be Positive</span> |

</div>

## Implementation

Added in 0.4.0.

TA-Lib Definition: [`add.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/add/add.c) · [`add.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/add/add.yaml)

| Native | File |
|--------|------|
| C | [`ta_ADD.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_ADD.c) |
| Rust | [`add.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/library/src/ta_func/add.rs) |
| Java | [`Core_ADD.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/fragments/Core_ADD.java) |
| C# | [`Core_ADD.cs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/csharp/library/src/Core_ADD.cs) |
| TypeScript | [`add.ts`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/ts/library/src/func/add.ts) |

Python: `talib.ADD`, see [ta-lib-python](https://ta-lib.github.io/ta-lib-python/func_groups/math_operators.html).

TA-Lib is also available for R and more using a [wrapper](/install/#wrappers).

## Aliases

Vector Add, Vector Arithmetic Add

## See Also

[SUB](/functions/sub.md) · [MULT](/functions/mult.md) · [DIV](/functions/div.md)
