---
title: "Vector Log10 (LOG10)"
description: "Element-wise base-10 logarithm of the input series."
---

## Summary

Element-wise base-10 logarithm of the input series.

## Chart

<ta-chart src="/functions/log10.svg" ratio="720:416" label="LOG10 over a ramp from 0.1 to 10" alt="Drawn on the price pane over a ramp from 0.1 to 10 as a line; below it, a separate indicator pane with a zero line showing outReal as a solid line."></ta-chart>

::: details Chart metadata <span class="flag-tip" tabindex="0" role="note" aria-label="The chart is drawn from the function's metadata: the flags and attributes the abstract API reports for it and for each output, and the input series its reference run declares." data-tip="The chart is drawn from the function's metadata: the flags and attributes the abstract API reports for it and for each output, and the input series its reference run declares.">i</span>

Drawn on the price pane over a ramp from 0.1 to 10 as a line; below it, a separate indicator pane with a zero line showing outReal as a solid line.

Metadata flags used: `inReal: series ramp [0.1, 10]` · `outReal: line`

:::

[Reference data (CSV)](/functions/log10.csv) · [Chart (SVG)](/functions/log10.svg)

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
| <span class="flag-box">☐</span> <span style="opacity:0.5">Display Shift</span> |
| <span class="flag-box">✅</span> **Uses Transcendental** <span class="flag-tip" tabindex="0" role="note" aria-label="Calls a transcendental math function (exp, log, log10, trigonometric, inverse trigonometric or hyperbolic), so a value may differ slightly between languages, between a stream and a batch call, and between machines. TA-Lib targets a difference below 1e-9." data-tip="Calls a transcendental math function (exp, log, log10, trigonometric, inverse trigonometric or hyperbolic), so a value may differ slightly between languages, between a stream and a batch call, and between machines. TA-Lib targets a difference below 1e-9.">i</span> |
| <span class="flag-box">✅</span> **Input Must Be Positive** <span class="flag-tip" tabindex="0" role="note" aria-label="Assumes the input is positive, as a price is. A series that reaches or crosses zero, such as an oscillator or a spread, returns success and an output that may have no meaning. See Notes for what a zero or a negative value does." data-tip="Assumes the input is positive, as a price is. A series that reaches or crosses zero, such as an oscillator or a spread, returns success and an output that may have no meaning. See Notes for what a zero or a negative value does.">i</span> |

</div>

## Example

::: code-tabs#lang

@tab C

```c
#include <stdio.h>
#include <ta_libc.h>

int n = 252;
double series[252]; /* your data, oldest first */
double out[252];
int outBegIdx, outNBElement;

TA_Initialize();
TA_LOG10( 0, n - 1, series, &outBegIdx, &outNBElement, out );
printf( "%d values from bar %d; last out = %f\n", outNBElement, outBegIdx, out[outNBElement - 1] );
```

@tab Rust

```rust
use ta_lib::Core;

let series = vec![0.0; 252]; // your data, oldest first
let mut out = vec![0.0; 252];

let core = Core::new();
let range = core.log10(0, series.len() - 1, &series, &mut out)?;
println!("{} values from bar {}; last out = {}", range.count, range.beg_idx, out[range.count - 1]);
```

@tab Java

```java
import io.github.talib.Core;
import io.github.talib.OutRange;

double[] series = new double[252]; // your data, oldest first
double[] out = new double[252];

Core core = Core.DEFAULT;
OutRange r = core.log10(0, series.length - 1, series, out);
System.out.println(r.count() + " values from bar " + r.begIdx() + "; last out = " + out[r.count() - 1]);
```

@tab C\#

```csharp
using System;
using TALib;

var series = new double[252]; // your data, oldest first
var outReal = new double[252];

var core = Core.Default;
OutRange r = core.Log10(0, series.Length - 1, series, outReal);
Console.WriteLine($"{r.Count} values from bar {r.BegIdx}; last outReal = {outReal[r.Count - 1]}");
```

@tab TypeScript

```ts
import { log10 } from 'ta-lib';

const series = new Float64Array(252); // your data, oldest first

const { begIdx, values } = log10(series);
console.log(values.length + ' values from bar ' + begIdx + '; last values = ' + values[values.length - 1]);
```

:::

## Implementation

Added in 0.4.0.

TA-Lib Definition: [`log10.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/log10/log10.c) · [`log10.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/log10/log10.yaml)

| Native | File |
|--------|------|
| C | [`ta_LOG10.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_LOG10.c) |
| Rust | [`log10.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/library/src/ta_func/log10.rs) |
| Java | [`Core_LOG10.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/fragments/Core_LOG10.java) |
| C# | [`Core_LOG10.cs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/csharp/library/src/Core_LOG10.cs) |
| TypeScript | [`log10.ts`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/ts/library/src/func/log10.ts) |

Python: `talib.LOG10`, see [ta-lib-python](https://ta-lib.github.io/ta-lib-python/func_groups/math_transform.html).

TA-Lib is also available for R and more using a [wrapper](/install/#wrappers).

## Aliases

Log Base 10, Common Logarithm

## See Also

[LN](/functions/ln.md) · [EXP](/functions/exp.md)

## References

- Wikipedia, *Common logarithm*: [en.wikipedia.org/wiki/Common_logarithm](https://en.wikipedia.org/wiki/Common_logarithm)
