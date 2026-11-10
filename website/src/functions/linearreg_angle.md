---
title: "Linear Regression Angle (LINEARREG_ANGLE)"
description: "The angle, in degrees, of the least-squares best-fit line over the last N points."
---

## Summary

The angle, in degrees, of the least-squares best-fit line over the last N points. It is the LINEARREG_SLOPE value passed through atan and converted to degrees. Positive angle = rising fit line, negative = falling; magnitude reflects steepness.

## Chart

<ta-chart src="/functions/linearreg_angle.svg" ratio="720:416" label="LINEARREG_ANGLE over IBM, daily, 1999" alt="Drawn on the price pane over the close as a line; below it, a separate indicator pane with a zero line showing outReal as a solid line."></ta-chart>

<div class="ta-chart-links">

[Reference data (CSV)](/functions/linearreg_angle.csv) · [Chart (SVG)](/functions/linearreg_angle.svg)

</div>

::: details Chart metadata <span class="flag-tip" tabindex="0" role="note" aria-label="The chart is drawn automatically from metadata TA-Lib provides. Your app can get the same through the Abstract API." data-tip="The chart is drawn automatically from metadata TA-Lib provides. Your app can get the same through the Abstract API.">i</span>

**Outputs**

- `outReal`: `line` · `zero` · `positive` · `negative`

:::

## Formula

m = (N·SumXY − SumX·SumY) / (SumX² − N·SumXSqr), with SumX=N(N−1)/2, SumXSqr=N(N−1)(2N−1)/6; angle = atan(m)·(180/π)

## Inputs

- `inReal` — Input series to regress

## Outputs

- `outReal` — Regression line slope expressed as an angle in degrees

## Parameters

| Parameter | Type | Default | Accepted values | Description |
| --- | --- | --- | --- | --- |
| `optInTimePeriod` | integer | 14 | 2–100000 | Number of points in the regression window |

## Properties

**Numerical Stability:** [Start-Independent](/functions/stability.md#start-independent)

<div class="flag-cols">

<div class="flag-table">

|  |
| :-- |
| <span class="flag-box">☐</span> <span style="opacity:0.5">Overlap Input</span> |
| <span class="flag-box">✅</span> **Independent Y-Axis** <span class="flag-tip" tabindex="0" role="note" aria-label="Output is on its own scale, drawn in a separate pane below the price chart." data-tip="Output is on its own scale, drawn in a separate pane below the price chart.">i</span> |
| <span class="flag-box">☐</span> <span style="opacity:0.5">Candlestick</span> |
| <span class="flag-box">☐</span> <span style="opacity:0.5">Display Shift</span> |

</div>

<div class="flag-table">

|  |
| :-- |
| <span class="flag-box">☐</span> <span style="opacity:0.5">Can Output NaN or ±Inf</span> |
| <span class="flag-box">☐</span> <span style="opacity:0.5">Identity at Period 1</span> |
| <span class="flag-box">✅</span> **Uses Transcendental** <span class="flag-tip" tabindex="0" role="note" aria-label="Calls a transcendental math function (exp, log, log10, trigonometric, inverse trigonometric or hyperbolic), so a value may differ slightly between languages, between a stream and a batch call, and between machines. TA-Lib targets a difference below 1e-9." data-tip="Calls a transcendental math function (exp, log, log10, trigonometric, inverse trigonometric or hyperbolic), so a value may differ slightly between languages, between a stream and a batch call, and between machines. TA-Lib targets a difference below 1e-9.">i</span> |
| <span class="flag-box">☐</span> <span style="opacity:0.5">Input Must Be Positive</span> |

</div>

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
TA_LINEARREG_ANGLE( 0, n - 1, series, 14, &outBegIdx, &outNBElement, out );
printf( "%d values from bar %d; last out = %f\n", outNBElement, outBegIdx, out[outNBElement - 1] );
```

@tab Rust

```rust
use ta_lib::Core;

let series = vec![0.0; 252]; // your data, oldest first
let mut out = vec![0.0; 252];

let core = Core::new();
let range = core.linearreg_angle(0, series.len() - 1, &series, 14, &mut out)?;
println!("{} values from bar {}; last out = {}", range.count, range.beg_idx, out[range.count - 1]);
```

@tab Java

```java
import io.github.talib.Core;
import io.github.talib.OutRange;

double[] series = new double[252]; // your data, oldest first
double[] out = new double[252];

Core core = Core.DEFAULT;
OutRange r = core.linearregAngle(0, series.length - 1, series, 14, out);
System.out.println(r.count() + " values from bar " + r.begIdx() + "; last out = " + out[r.count() - 1]);
```

@tab C\#

```csharp
using System;
using TALib;

var series = new double[252]; // your data, oldest first
var outReal = new double[252];

var core = Core.Default;
OutRange r = core.LinearregAngle(0, series.Length - 1, series, 14, outReal);
Console.WriteLine($"{r.Count} values from bar {r.BegIdx}; last outReal = {outReal[r.Count - 1]}");
```

@tab TypeScript

```ts
import { linearregAngle } from 'ta-lib';

const series = new Float64Array(252); // your data, oldest first

const { begIdx, values } = linearregAngle(series, { timePeriod: 14 });
console.log(values.length + ' values from bar ' + begIdx + '; last values = ' + values[values.length - 1]);
```

:::

## Implementation

Added in 0.4.0.

TA-Lib Definition: [`linearreg_angle.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/linearreg_angle/linearreg_angle.c) · [`linearreg_angle.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/linearreg_angle/linearreg_angle.yaml)

| Native | File |
|--------|------|
| C | [`ta_LINEARREG_ANGLE.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_LINEARREG_ANGLE.c) |
| Rust | [`linearreg_angle.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/library/src/ta_func/linearreg_angle.rs) |
| Java | [`Core_LINEARREG_ANGLE.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/fragments/Core_LINEARREG_ANGLE.java) |
| C# | [`Core_LINEARREG_ANGLE.cs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/csharp/library/src/Core_LINEARREG_ANGLE.cs) |
| TypeScript | [`linearregAngle.ts`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/ts/library/src/func/linearregAngle.ts) |

Python: `talib.LINEARREG_ANGLE`, see [ta-lib-python](https://ta-lib.github.io/ta-lib-python/func_groups/statistic_functions.html).

TA-Lib is also available for R and more using a [wrapper](/install/#wrappers).

## Aliases

Linear Regression Angle, Least Squares Angle

## See Also

[LINEARREG](/functions/linearreg.md) · [LINEARREG_SLOPE](/functions/linearreg_slope.md) · [LINEARREG_INTERCEPT](/functions/linearreg_intercept.md) · [TSF](/functions/tsf.md)
