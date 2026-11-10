---
url: 'https://ta-lib.org/functions/minmaxindex.md'
description: >-
  Returns the absolute input indices of the lowest and highest values within
  each rolling window of optInTimePeriod bars. Index variant of MINMAX.
---
# Indexes of lowest and highest values over a specified period (MINMAXINDEX)

## Summary

Returns the absolute input indices of the lowest and highest values within each rolling window of optInTimePeriod bars. Index variant of MINMAX.

## Chart

<ta-chart src="/functions/minmaxindex.svg" ratio="720:438" label="MINMAXINDEX over IBM, daily, 1999" alt="Drawn on the price pane over the close as a line, with close at the bar outMinIdx names as a solid line and close at the bar outMaxIdx names as a solid line; below it, a step pane of its own for each of outMinIdx and outMaxIdx."></ta-chart>

<div class="ta-chart-links">

[Reference data (CSV)](/functions/minmaxindex.csv) · [Chart (SVG)](/functions/minmaxindex.svg)

</div>

::: details Chart metadata <span class="flag-tip" tabindex="0" role="note" aria-label="The chart is drawn automatically from metadata TA-Lib provides. Your app can get the same through the Abstract API." data-tip="The chart is drawn automatically from metadata TA-Lib provides. Your app can get the same through the Abstract API.">i</span>

**Outputs**

* `outMinIdx`: `line` · `bar_index`
* `outMaxIdx`: `line` · `bar_index`

:::

## Formula

outMinIdx\[i] = index of min(inReal\[i-optInTimePeriod+1 .. i])\
outMaxIdx\[i] = index of max(inReal\[i-optInTimePeriod+1 .. i])

## Notes

* When several bars in a window share the extreme value, the index of the most recent of them is returned.

## Inputs

* `inReal` — Input series scanned for extremes

## Outputs

* `outMinIdx` — Absolute index (into inReal) of the window minimum
* `outMaxIdx` — Absolute index (into inReal) of the window maximum

## Parameters

| Parameter | Type | Default | Accepted values | Description |
| --- | --- | --- | --- | --- |
| `optInTimePeriod` | integer | 30 | 2–100000 | Window length in bars |

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
| <span class="flag-box">☐</span> <span style="opacity:0.5">Uses Transcendental</span> |
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
int minIdx[252], maxIdx[252];
int outBegIdx, outNBElement;

TA_Initialize();
TA_MINMAXINDEX( 0, n - 1, series, 30, &outBegIdx, &outNBElement, minIdx, maxIdx );
printf( "%d values from bar %d; last minIdx = %d\n", outNBElement, outBegIdx, minIdx[outNBElement - 1] );
```

@tab Rust

```rust
use ta_lib::Core;

let series = vec![0.0; 252]; // your data, oldest first
let mut min_idx = vec![0i32; 252];
let mut max_idx = vec![0i32; 252];

let core = Core::new();
let range = core.minmaxindex(0, series.len() - 1, &series, 30, &mut min_idx, &mut max_idx)?;
println!("{} values from bar {}; last min_idx = {}", range.count, range.beg_idx, min_idx[range.count - 1]);
```

@tab Java

```java
import io.github.talib.Core;
import io.github.talib.OutRange;

double[] series = new double[252]; // your data, oldest first
int[] minIdx = new int[252];
int[] maxIdx = new int[252];

Core core = Core.DEFAULT;
OutRange r = core.minmaxindex(0, series.length - 1, series, 30, minIdx, maxIdx);
System.out.println(r.count() + " values from bar " + r.begIdx() + "; last minIdx = " + minIdx[r.count() - 1]);
```

@tab C#

```csharp
using System;
using TALib;

var series = new double[252]; // your data, oldest first
var minIdx = new int[252];
var maxIdx = new int[252];

var core = Core.Default;
OutRange r = core.Minmaxindex(0, series.Length - 1, series, 30, minIdx, maxIdx);
Console.WriteLine($"{r.Count} values from bar {r.BegIdx}; last minIdx = {minIdx[r.Count - 1]}");
```

@tab TypeScript

```ts
import { minmaxindex } from 'ta-lib';

const series = new Float64Array(252); // your data, oldest first

const { begIdx, outMinIdx, outMaxIdx } = minmaxindex(series, {
  timePeriod: 30,
});
console.log(outMinIdx.length + ' values from bar ' + begIdx + '; last outMinIdx = ' + outMinIdx[outMinIdx.length - 1]);
```

:::

## Implementation

Added in 0.4.0.

TA-Lib Definition: [`minmaxindex.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/minmaxindex/minmaxindex.c) · [`minmaxindex.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/minmaxindex/minmaxindex.yaml)

| Native | File |
|--------|------|
| C | [`ta_MINMAXINDEX.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_MINMAXINDEX.c) |
| Rust | [`minmaxindex.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/library/src/ta_func/minmaxindex.rs) |
| Java | [`Core_MINMAXINDEX.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/fragments/Core_MINMAXINDEX.java) |
| C# | [`Core_MINMAXINDEX.cs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/csharp/library/src/Core_MINMAXINDEX.cs) |
| TypeScript | [`minmaxindex.ts`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/ts/library/src/func/minmaxindex.ts) |

Python: `talib.MINMAXINDEX`, see [ta-lib-python](https://ta-lib.github.io/ta-lib-python/func_groups/math_operators.html).

TA-Lib is also available for R and more using a [wrapper](/install/#wrappers).

## Aliases

Lowest/Highest Index

## See Also

[MINMAX](/functions/minmax.md) · [MIN](/functions/min.md) · [MAX](/functions/max.md) · [MININDEX](/functions/minindex.md) · [MAXINDEX](/functions/maxindex.md)
