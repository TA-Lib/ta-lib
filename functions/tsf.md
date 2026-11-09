---
url: 'https://ta-lib.org/functions/tsf.md'
description: >-
  Time Series Forecast: fits a least-squares linear regression line over the
  last N bars and projects it one x-step beyond LINEARREG.
---
# Time Series Forecast (TSF)

## Summary

Time Series Forecast: fits a least-squares linear regression line over the last N bars and projects it one x-step beyond LINEARREG. Same regression as LINEARREG but evaluated at x=period instead of x=period-1.

## Chart

<ta-chart src="/functions/tsf.svg" ratio="720:274" label="TSF over IBM, daily, 1999" alt="Drawn on the price pane over the close as a line, with outReal as a solid line."></ta-chart>

::: details Chart metadata <span class="flag-tip" tabindex="0" role="note" aria-label="The chart is drawn from the function's metadata: the flags and attributes the abstract API reports for it and for each output, and the input series its reference run declares." data-tip="The chart is drawn from the function's metadata: the flags and attributes the abstract API reports for it and for each output, and the input series its reference run declares.">i</span>

Drawn on the price pane over the close as a line, with outReal as a solid line.

Metadata flags used: `overlap` · `outReal: line`

:::

[Reference data (CSV)](/functions/tsf.csv) · [Chart (SVG)](/functions/tsf.svg)

## Formula

Fit y=b+m*x over window (x=0..N-1): m = (N*SumXY - SumX*SumY)/(SumX^2 - N*SumXSqr), b = (SumY - m*SumX)/N; output = b + m*N. With SumX=N(N-1)/2, SumXSqr=N(N-1)(2N-1)/6.

## Inputs

* `inReal` — Input series to regress and forecast

## Outputs

* `outReal` — Regression line value projected to x=period (one step past LINEARREG)

## Parameters

| Parameter | Type | Default | Accepted values | Description |
| --- | --- | --- | --- | --- |
| `optInTimePeriod` | integer | 14 | 2–100000 | Number of bars in the regression window |

## Properties

**Numerical Stability:** [Start-Independent](/functions/stability.md#start-independent)

<div class="flag-table">

|  |
| :-- |
| <span class="flag-box">✅</span> **Overlap Input** <span class="flag-tip" tabindex="0" role="note" aria-label="Output is on the same scale as the input price, so it is drawn over the price chart." data-tip="Output is on the same scale as the input price, so it is drawn over the price chart.">i</span> |
| <span class="flag-box">☐</span> <span style="opacity:0.5">Independent Y-Axis</span> |
| <span class="flag-box">☐</span> <span style="opacity:0.5">Candlestick</span> |
| <span class="flag-box">☐</span> <span style="opacity:0.5">Can Output NaN or ±Inf</span> |
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
double series[252]; /* your data, oldest first */
double out[252];
int outBegIdx, outNBElement;

TA_Initialize();
TA_TSF( 0, n - 1, series, 14, &outBegIdx, &outNBElement, out );
printf( "%d values from bar %d; last out = %f\n", outNBElement, outBegIdx, out[outNBElement - 1] );
```

@tab Rust

```rust
use ta_lib::Core;

let series = vec![0.0; 252]; // your data, oldest first
let mut out = vec![0.0; 252];

let core = Core::new();
let range = core.tsf(0, series.len() - 1, &series, 14, &mut out)?;
println!("{} values from bar {}; last out = {}", range.count, range.beg_idx, out[range.count - 1]);
```

@tab Java

```java
import io.github.talib.Core;
import io.github.talib.OutRange;

double[] series = new double[252]; // your data, oldest first
double[] out = new double[252];

Core core = Core.DEFAULT;
OutRange r = core.tsf(0, series.length - 1, series, 14, out);
System.out.println(r.count() + " values from bar " + r.begIdx() + "; last out = " + out[r.count() - 1]);
```

@tab C#

```csharp
using System;
using TALib;

var series = new double[252]; // your data, oldest first
var outReal = new double[252];

var core = Core.Default;
OutRange r = core.Tsf(0, series.Length - 1, series, 14, outReal);
Console.WriteLine($"{r.Count} values from bar {r.BegIdx}; last outReal = {outReal[r.Count - 1]}");
```

@tab TypeScript

```ts
import { tsf } from 'ta-lib';

const series = new Float64Array(252); // your data, oldest first

const { begIdx, values } = tsf(series, { timePeriod: 14 });
console.log(values.length + ' values from bar ' + begIdx + '; last values = ' + values[values.length - 1]);
```

:::

## Implementation

Added in 0.4.0.

TA-Lib Definition: [`tsf.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/tsf/tsf.c) · [`tsf.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/tsf/tsf.yaml)

| Native | File |
|--------|------|
| C | [`ta_TSF.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_TSF.c) |
| Rust | [`tsf.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/library/src/ta_func/tsf.rs) |
| Java | [`Core_TSF.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/fragments/Core_TSF.java) |
| C# | [`Core_TSF.cs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/csharp/library/src/Core_TSF.cs) |
| TypeScript | [`tsf.ts`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/ts/library/src/func/tsf.ts) |

Python: `talib.TSF`, see [ta-lib-python](https://ta-lib.github.io/ta-lib-python/func_groups/statistic_functions.html).

TA-Lib is also available for R and more using a [wrapper](/install/#wrappers).

## Aliases

Time Series Forecast

## See Also

[LINEARREG](/functions/linearreg.md) · [LINEARREG_SLOPE](/functions/linearreg_slope.md) · [LINEARREG_INTERCEPT](/functions/linearreg_intercept.md) · [LINEARREG_ANGLE](/functions/linearreg_angle.md)
