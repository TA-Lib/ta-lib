---
title: "Vector Trigonometric Sin (SIN)"
description: "Element-wise sine of the input series."
---

## Summary

Element-wise sine of the input series.

## Chart

<ta-chart src="/functions/sin.svg" ratio="720:416" label="SIN over a ramp from -6.3 to 6.3" alt="Drawn on the price pane over a ramp from -6.3 to 6.3 as a line; below it, a separate indicator pane with a zero line showing outReal as a solid line."></ta-chart>

::: details Chart metadata <span class="flag-tip" tabindex="0" role="note" aria-label="The chart is drawn from the function's metadata alone: the flags the abstract API reports for it and for each output." data-tip="The chart is drawn from the function's metadata alone: the flags the abstract API reports for it and for each output.">i</span>

Drawn on the price pane over a ramp from -6.3 to 6.3 as a line; below it, a separate indicator pane with a zero line showing outReal as a solid line.

Metadata flags used: `inReal: series ramp [-6.3, 6.3]` · `outReal: line`

:::

[Reference data (CSV)](/functions/sin.csv) · [Chart (SVG)](/functions/sin.svg)

## Formula

outReal[i] = sin(inReal[i])

## Inputs

- `inReal` — Input values (radians)

## Outputs

- `outReal` — Sine of each input

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
TA_SIN( 0, n - 1, series, &outBegIdx, &outNBElement, out );
printf( "%d values from bar %d; last out = %f\n", outNBElement, outBegIdx, out[outNBElement - 1] );
```

@tab Rust

```rust
use ta_lib::Core;

let series = vec![0.0; 252]; // your data, oldest first
let mut out = vec![0.0; 252];

let core = Core::new();
let range = core.sin(0, series.len() - 1, &series, &mut out)?;
println!("{} values from bar {}; last out = {}", range.count, range.beg_idx, out[range.count - 1]);
```

@tab Java

```java
import io.github.talib.Core;
import io.github.talib.OutRange;

double[] series = new double[252]; // your data, oldest first
double[] out = new double[252];

Core core = Core.DEFAULT;
OutRange r = core.sin(0, series.length - 1, series, out);
System.out.println(r.count() + " values from bar " + r.begIdx() + "; last out = " + out[r.count() - 1]);
```

@tab C\#

```csharp
using System;
using TALib;

var series = new double[252]; // your data, oldest first
var outReal = new double[252];

var core = Core.Default;
OutRange r = core.Sin(0, series.Length - 1, series, outReal);
Console.WriteLine($"{r.Count} values from bar {r.BegIdx}; last outReal = {outReal[r.Count - 1]}");
```

@tab TypeScript

```ts
import { sin } from 'ta-lib';

const series = new Float64Array(252); // your data, oldest first

const { begIdx, values } = sin(series);
console.log(values.length + ' values from bar ' + begIdx + '; last values = ' + values[values.length - 1]);
```

:::

## Implementation

Added in 0.4.0.

TA-Lib Definition: [`sin.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/sin/sin.c) · [`sin.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/sin/sin.yaml)

| Native | File |
|--------|------|
| C | [`ta_SIN.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_SIN.c) |
| Rust | [`sin.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/library/src/ta_func/sin.rs) |
| Java | [`Core_SIN.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/fragments/Core_SIN.java) |
| C# | [`Core_SIN.cs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/csharp/library/src/Core_SIN.cs) |
| TypeScript | [`sin.ts`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/ts/library/src/func/sin.ts) |

Python: `talib.SIN`, see [ta-lib-python](https://ta-lib.github.io/ta-lib-python/func_groups/math_transform.html).

TA-Lib is also available for R and more using a [wrapper](/install/#wrappers).

## Aliases

sine

## See Also

[COS](/functions/cos.md) · [TAN](/functions/tan.md) · [ASIN](/functions/asin.md)

## References

- Wikipedia, *Trigonometric functions*: [en.wikipedia.org/wiki/Trigonometric_functions](https://en.wikipedia.org/wiki/Trigonometric_functions)
