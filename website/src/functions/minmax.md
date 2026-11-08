---
title: "Lowest and highest values over a specified period (MINMAX)"
description: "Returns both the lowest and highest values of the input over a rolling window of the last optInTimePeriod bars."
---

## Summary

Returns both the lowest and highest values of the input over a rolling window of the last optInTimePeriod bars. An overlap-study companion to MIN and MAX that computes both extrema in one pass.

## Chart

<ta-chart src="/functions/minmax.svg" ratio="720:274" label="MINMAX over IBM, daily, 1999" alt="Drawn on the price pane over the close as a line, with outMin as a solid line and outMax as a solid line."></ta-chart>

Drawn on the price pane over the close as a line, with outMin as a solid line and outMax as a solid line. [Reference data (CSV)](/functions/minmax.csv) · [Chart (SVG)](/functions/minmax.svg)

## Formula

outMin[i] = min(inReal[i-optInTimePeriod+1 .. i])  
outMax[i] = max(inReal[i-optInTimePeriod+1 .. i])

## Inputs

- `inReal` — Values scanned for the window min and max

## Outputs

- `outMin` — Lowest value in each rolling window
- `outMax` — Highest value in each rolling window

## Parameters

| Parameter | Type | Default | Accepted values | Description |
| --- | --- | --- | --- | --- |
| `optInTimePeriod` | integer | 30 | 2–100000 | Rolling window length |

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
double min[252], max[252];
int outBegIdx, outNBElement;

TA_Initialize();
TA_MINMAX( 0, n - 1, series, 30, &outBegIdx, &outNBElement, min, max );
printf( "%d values from bar %d; last min = %f\n", outNBElement, outBegIdx, min[outNBElement - 1] );
```

@tab Rust

```rust
use ta_lib::Core;

let series = vec![0.0; 252]; // your data, oldest first
let mut min = vec![0.0; 252];
let mut max = vec![0.0; 252];

let core = Core::new();
let range = core.minmax(0, series.len() - 1, &series, 30, &mut min, &mut max)?;
println!("{} values from bar {}; last min = {}", range.count, range.beg_idx, min[range.count - 1]);
```

@tab Java

```java
import io.github.talib.Core;
import io.github.talib.OutRange;

double[] series = new double[252]; // your data, oldest first
double[] min = new double[252];
double[] max = new double[252];

Core core = Core.DEFAULT;
OutRange r = core.minmax(0, series.length - 1, series, 30, min, max);
System.out.println(r.count() + " values from bar " + r.begIdx() + "; last min = " + min[r.count() - 1]);
```

@tab C\#

```csharp
using System;
using TALib;

var series = new double[252]; // your data, oldest first
var min = new double[252];
var max = new double[252];

var core = Core.Default;
OutRange r = core.Minmax(0, series.Length - 1, series, 30, min, max);
Console.WriteLine($"{r.Count} values from bar {r.BegIdx}; last min = {min[r.Count - 1]}");
```

@tab TypeScript

```ts
import { minmax } from 'ta-lib';

const series = new Float64Array(252); // your data, oldest first

const { begIdx, outMin, outMax } = minmax(series, { timePeriod: 30 });
console.log(outMin.length + ' values from bar ' + begIdx + '; last outMin = ' + outMin[outMin.length - 1]);
```

:::

## Implementation

Added in 0.4.0.

TA-Lib Definition: [`minmax.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/minmax/minmax.c) · [`minmax.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/minmax/minmax.yaml)

| Native | File |
|--------|------|
| C | [`ta_MINMAX.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_MINMAX.c) |
| Rust | [`minmax.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/library/src/ta_func/minmax.rs) |
| Java | [`Core_MINMAX.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/fragments/Core_MINMAX.java) |
| C# | [`Core_MINMAX.cs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/csharp/library/src/Core_MINMAX.cs) |
| TypeScript | [`minmax.ts`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/ts/library/src/func/minmax.ts) |

Python: `talib.MINMAX`, see [ta-lib-python](https://ta-lib.github.io/ta-lib-python/func_groups/math_operators.html).

TA-Lib is also available for R and more using a [wrapper](/install/#wrappers).

## Aliases

Highest Lowest

## See Also

[MIN](/functions/min.md) · [MAX](/functions/max.md) · [MINMAXINDEX](/functions/minmaxindex.md) · [MININDEX](/functions/minindex.md) · [MAXINDEX](/functions/maxindex.md)
