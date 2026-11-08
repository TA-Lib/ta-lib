---
title: "Vector Arithmetic Div (DIV)"
description: "Element-wise division of two input series."
---

## Summary

Element-wise division of two input series.

## Chart

<ta-chart src="/functions/div.svg" ratio="720:416" label="DIV over a sine between 90 and 110 and a ramp from 95 to 105" alt="Drawn on the price pane over the inputs inReal0 and inReal1, a sine between 90 and 110 and a ramp from 95 to 105 as lines; below it, a separate indicator pane showing outReal as a solid line."></ta-chart>

::: details Chart metadata <span class="flag-tip" tabindex="0" role="note" aria-label="The chart is drawn from the function's metadata alone: the flags the abstract API reports for it and for each output." data-tip="The chart is drawn from the function's metadata alone: the flags the abstract API reports for it and for each output.">i</span>

Drawn on the price pane over the inputs inReal0 and inReal1, a sine between 90 and 110 and a ramp from 95 to 105 as lines; below it, a separate indicator pane showing outReal as a solid line.

Metadata flags used: `inReal0: series sine [90, 110]` · `inReal1: series ramp [95, 105]` · `outReal: line`

:::

[Reference data (CSV)](/functions/div.csv) · [Chart (SVG)](/functions/div.svg)

## Formula

outReal[i] = inReal0[i] / inReal1[i]

## Notes

- Zero divided by zero gives NaN; anything else divided by zero gives positive or negative infinity. Neither is reported as an error.

## Inputs

- `inReal0` — Dividend (numerator) series
- `inReal1` — Divisor (denominator) series

## Outputs

- `outReal` — Per-element quotient inReal0/inReal1

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
| <span class="flag-box">☐</span> <span style="opacity:0.5">Display Shift</span> |
| <span class="flag-box">☐</span> <span style="opacity:0.5">Uses Transcendental</span> |
| <span class="flag-box">☐</span> <span style="opacity:0.5">Input Must Be Positive</span> |

</div>

## Example

::: code-tabs#lang

@tab C

```c
#include <stdio.h>
#include <ta_libc.h>

int n = 252;
double series0[252]; /* your data, oldest first */
double series1[252]; /* your data, oldest first */
double out[252];
int outBegIdx, outNBElement;

TA_Initialize();
TA_DIV( 0, n - 1, series0, series1, &outBegIdx, &outNBElement, out );
printf( "%d values from bar %d; last out = %f\n", outNBElement, outBegIdx, out[outNBElement - 1] );
```

@tab Rust

```rust
use ta_lib::Core;

let series0 = vec![0.0; 252]; // your data, oldest first
let series1 = vec![0.0; 252]; // your data, oldest first
let mut out = vec![0.0; 252];

let core = Core::new();
let range = core.div(0, series0.len() - 1, &series0, &series1, &mut out)?;
println!("{} values from bar {}; last out = {}", range.count, range.beg_idx, out[range.count - 1]);
```

@tab Java

```java
import io.github.talib.Core;
import io.github.talib.OutRange;

double[] series0 = new double[252]; // your data, oldest first
double[] series1 = new double[252]; // your data, oldest first
double[] out = new double[252];

Core core = Core.DEFAULT;
OutRange r = core.div(0, series0.length - 1, series0, series1, out);
System.out.println(r.count() + " values from bar " + r.begIdx() + "; last out = " + out[r.count() - 1]);
```

@tab C\#

```csharp
using System;
using TALib;

var series0 = new double[252]; // your data, oldest first
var series1 = new double[252]; // your data, oldest first
var outReal = new double[252];

var core = Core.Default;
OutRange r = core.Div(0, series0.Length - 1, series0, series1, outReal);
Console.WriteLine($"{r.Count} values from bar {r.BegIdx}; last outReal = {outReal[r.Count - 1]}");
```

@tab TypeScript

```ts
import { div } from 'ta-lib';

const series0 = new Float64Array(252); // your data, oldest first
const series1 = new Float64Array(252); // your data, oldest first

const { begIdx, values } = div(series0, series1);
console.log(values.length + ' values from bar ' + begIdx + '; last values = ' + values[values.length - 1]);
```

:::

## Implementation

Added in 0.4.0.

TA-Lib Definition: [`div.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/div/div.c) · [`div.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/div/div.yaml)

| Native | File |
|--------|------|
| C | [`ta_DIV.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_DIV.c) |
| Rust | [`div.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/library/src/ta_func/div.rs) |
| Java | [`Core_DIV.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/fragments/Core_DIV.java) |
| C# | [`Core_DIV.cs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/csharp/library/src/Core_DIV.cs) |
| TypeScript | [`div.ts`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/ts/library/src/func/div.ts) |

Python: `talib.DIV`, see [ta-lib-python](https://ta-lib.github.io/ta-lib-python/func_groups/math_operators.html).

TA-Lib is also available for R and more using a [wrapper](/install/#wrappers).

## Aliases

Vector Arithmetic Divide, Divide

## See Also

[MULT](/functions/mult.md) · [ADD](/functions/add.md) · [SUB](/functions/sub.md)
