---
title: "Linear Regression (LINEARREG)"
description: "Least-squares straight-line fit over the last optInTimePeriod bars, reported as the fitted line value at the window endpoint (b + m*(period-1))."
---

## Summary

Least-squares straight-line fit over the last optInTimePeriod bars, reported as the fitted line value at the window endpoint (b + m*(period-1)).

## Chart

<ta-chart src="/functions/linearreg.svg" ratio="720:274" label="LINEARREG over IBM, daily, 1999" alt="Drawn on the price pane over the close as a line, with outReal as a solid line."></ta-chart>

::: details Chart metadata <span class="flag-tip" tabindex="0" role="note" aria-label="The chart is drawn from the function's metadata alone: the flags the abstract API reports for it and for each output." data-tip="The chart is drawn from the function's metadata alone: the flags the abstract API reports for it and for each output.">i</span>

Drawn on the price pane over the close as a line, with outReal as a solid line.

Metadata flags used: `overlap` · `outReal: line`

:::

[Reference data (CSV)](/functions/linearreg.csv) · [Chart (SVG)](/functions/linearreg.svg)

## Inputs

- `inReal` — Series to fit

## Outputs

- `outReal` — Regression line value at the window endpoint

## Parameters

| Parameter | Type | Default | Accepted values | Description |
| --- | --- | --- | --- | --- |
| `optInTimePeriod` | integer | 14 | 2–100000 | Number of bars in each regression window |

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
TA_LINEARREG( 0, n - 1, series, 14, &outBegIdx, &outNBElement, out );
printf( "%d values from bar %d; last out = %f\n", outNBElement, outBegIdx, out[outNBElement - 1] );
```

@tab Rust

```rust
use ta_lib::Core;

let series = vec![0.0; 252]; // your data, oldest first
let mut out = vec![0.0; 252];

let core = Core::new();
let range = core.linearreg(0, series.len() - 1, &series, 14, &mut out)?;
println!("{} values from bar {}; last out = {}", range.count, range.beg_idx, out[range.count - 1]);
```

@tab Java

```java
import io.github.talib.Core;
import io.github.talib.OutRange;

double[] series = new double[252]; // your data, oldest first
double[] out = new double[252];

Core core = Core.DEFAULT;
OutRange r = core.linearreg(0, series.length - 1, series, 14, out);
System.out.println(r.count() + " values from bar " + r.begIdx() + "; last out = " + out[r.count() - 1]);
```

@tab C\#

```csharp
using System;
using TALib;

var series = new double[252]; // your data, oldest first
var outReal = new double[252];

var core = Core.Default;
OutRange r = core.Linearreg(0, series.Length - 1, series, 14, outReal);
Console.WriteLine($"{r.Count} values from bar {r.BegIdx}; last outReal = {outReal[r.Count - 1]}");
```

@tab TypeScript

```ts
import { linearreg } from 'ta-lib';

const series = new Float64Array(252); // your data, oldest first

const { begIdx, values } = linearreg(series, { timePeriod: 14 });
console.log(values.length + ' values from bar ' + begIdx + '; last values = ' + values[values.length - 1]);
```

:::

## Implementation

Added in 0.4.0.

TA-Lib Definition: [`linearreg.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/linearreg/linearreg.c) · [`linearreg.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/linearreg/linearreg.yaml)

| Native | File |
|--------|------|
| C | [`ta_LINEARREG.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_LINEARREG.c) |
| Rust | [`linearreg.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/library/src/ta_func/linearreg.rs) |
| Java | [`Core_LINEARREG.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/fragments/Core_LINEARREG.java) |
| C# | [`Core_LINEARREG.cs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/csharp/library/src/Core_LINEARREG.cs) |
| TypeScript | [`linearreg.ts`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/ts/library/src/func/linearreg.ts) |

Python: `talib.LINEARREG`, see [ta-lib-python](https://ta-lib.github.io/ta-lib-python/func_groups/statistic_functions.html).

TA-Lib is also available for R and more using a [wrapper](/install/#wrappers).

## Aliases

Linear Regression, Least Squares, Best Fit Line

## See Also

[LINEARREG_SLOPE](/functions/linearreg_slope.md) · [LINEARREG_ANGLE](/functions/linearreg_angle.md) · [LINEARREG_INTERCEPT](/functions/linearreg_intercept.md) · [TSF](/functions/tsf.md)
