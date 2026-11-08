---
title: "Index of highest value over a specified period (MAXINDEX)"
description: "Returns the index of the highest input value within a rolling window of optInTimePeriod bars. Same as MAX but outputs the location instead of the value."
---

## Summary

Returns the index of the highest input value within a rolling window of optInTimePeriod bars. Same as MAX but outputs the location instead of the value.

## Chart

<ta-chart src="/functions/maxindex.svg" ratio="720:416" label="MAXINDEX over IBM, daily, 1999" alt="Drawn on the price pane over the close as a line; below it, a step pane of its own for the integer output outInteger."></ta-chart>

::: details Chart metadata <span class="flag-tip" tabindex="0" role="note" aria-label="The chart is drawn from the function's metadata: the flags and attributes the abstract API reports for it and for each output, and the input series its reference run declares." data-tip="The chart is drawn from the function's metadata: the flags and attributes the abstract API reports for it and for each output, and the input series its reference run declares.">i</span>

Drawn on the price pane over the close as a line; below it, a step pane of its own for the integer output outInteger.

Metadata flags used: `outInteger: integer`

:::

[Reference data (CSV)](/functions/maxindex.csv) · [Chart (SVG)](/functions/maxindex.svg)

## Formula

outInteger[i] = index of max(inReal[i-optInTimePeriod+1 .. i])

## Notes

- When several bars in a window share the highest value, the index of the most recent of them is returned.

## Inputs

- `inReal` — Input series to scan

## Outputs

- `outInteger` — Absolute index (into inReal) of the highest value in each window

## Parameters

| Parameter | Type | Default | Accepted values | Description |
| --- | --- | --- | --- | --- |
| `optInTimePeriod` | integer | 30 | 2–100000 | Window length over which the max is located |

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

## Example

::: code-tabs#lang

@tab C

```c
#include <stdio.h>
#include <ta_libc.h>

int n = 252;
double series[252]; /* your data, oldest first */
int out[252];
int outBegIdx, outNBElement;

TA_Initialize();
TA_MAXINDEX( 0, n - 1, series, 30, &outBegIdx, &outNBElement, out );
printf( "%d values from bar %d; last out = %d\n", outNBElement, outBegIdx, out[outNBElement - 1] );
```

@tab Rust

```rust
use ta_lib::Core;

let series = vec![0.0; 252]; // your data, oldest first
let mut out = vec![0i32; 252];

let core = Core::new();
let range = core.maxindex(0, series.len() - 1, &series, 30, &mut out)?;
println!("{} values from bar {}; last out = {}", range.count, range.beg_idx, out[range.count - 1]);
```

@tab Java

```java
import io.github.talib.Core;
import io.github.talib.OutRange;

double[] series = new double[252]; // your data, oldest first
int[] out = new int[252];

Core core = Core.DEFAULT;
OutRange r = core.maxindex(0, series.length - 1, series, 30, out);
System.out.println(r.count() + " values from bar " + r.begIdx() + "; last out = " + out[r.count() - 1]);
```

@tab C\#

```csharp
using System;
using TALib;

var series = new double[252]; // your data, oldest first
var outInteger = new int[252];

var core = Core.Default;
OutRange r = core.Maxindex(0, series.Length - 1, series, 30, outInteger);
Console.WriteLine($"{r.Count} values from bar {r.BegIdx}; last outInteger = {outInteger[r.Count - 1]}");
```

@tab TypeScript

```ts
import { maxindex } from 'ta-lib';

const series = new Float64Array(252); // your data, oldest first

const { begIdx, values } = maxindex(series, { timePeriod: 30 });
console.log(values.length + ' values from bar ' + begIdx + '; last values = ' + values[values.length - 1]);
```

:::

## Implementation

Added in 0.4.0.

TA-Lib Definition: [`maxindex.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/maxindex/maxindex.c) · [`maxindex.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/maxindex/maxindex.yaml)

| Native | File |
|--------|------|
| C | [`ta_MAXINDEX.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_MAXINDEX.c) |
| Rust | [`maxindex.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/library/src/ta_func/maxindex.rs) |
| Java | [`Core_MAXINDEX.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/fragments/Core_MAXINDEX.java) |
| C# | [`Core_MAXINDEX.cs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/csharp/library/src/Core_MAXINDEX.cs) |
| TypeScript | [`maxindex.ts`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/ts/library/src/func/maxindex.ts) |

Python: `talib.MAXINDEX`, see [ta-lib-python](https://ta-lib.github.io/ta-lib-python/func_groups/math_operators.html).

TA-Lib is also available for R and more using a [wrapper](/install/#wrappers).

## Aliases

Index of Highest Value, Highest Value Index, argmax

## See Also

[MAX](/functions/max.md) · [MININDEX](/functions/minindex.md) · [MIN](/functions/min.md) · [MINMAXINDEX](/functions/minmaxindex.md)
