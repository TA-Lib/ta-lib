---
title: "MidPoint over period (MIDPOINT)"
description: "Midpoint over a period: the average of the highest and lowest input values within the lookback window."
---

## Summary

Midpoint over a period: the average of the highest and lowest input values within the lookback window. A single-series overlap smoother (use MIDPRICE for separate high/low price bars).

## Chart

<ta-chart src="/functions/midpoint.svg" ratio="720:274" label="MIDPOINT over IBM, daily, 1999" alt="Drawn on the price pane over the close as a line, with outReal as a solid line."></ta-chart>

::: details Chart metadata <span class="flag-tip" tabindex="0" role="note" aria-label="The chart is drawn from the function's metadata alone: the flags the abstract API reports for it and for each output." data-tip="The chart is drawn from the function's metadata alone: the flags the abstract API reports for it and for each output.">i</span>

Drawn on the price pane over the close as a line, with outReal as a solid line.

Metadata flags used: `overlap` · `outReal: line`

:::

[Reference data (CSV)](/functions/midpoint.csv) · [Chart (SVG)](/functions/midpoint.svg)

## Formula

MIDPOINT = (Highest(inReal, period) + Lowest(inReal, period)) / 2

## Inputs

- `inReal` — Series to compute the midpoint over

## Outputs

- `outReal` — Midpoint of the period's high/low range

## Parameters

| Parameter | Type | Default | Accepted values | Description |
| --- | --- | --- | --- | --- |
| `optInTimePeriod` | integer | 14 | 2–100000 | Lookback window length |

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
TA_MIDPOINT( 0, n - 1, series, 14, &outBegIdx, &outNBElement, out );
printf( "%d values from bar %d; last out = %f\n", outNBElement, outBegIdx, out[outNBElement - 1] );
```

@tab Rust

```rust
use ta_lib::Core;

let series = vec![0.0; 252]; // your data, oldest first
let mut out = vec![0.0; 252];

let core = Core::new();
let range = core.midpoint(0, series.len() - 1, &series, 14, &mut out)?;
println!("{} values from bar {}; last out = {}", range.count, range.beg_idx, out[range.count - 1]);
```

@tab Java

```java
import io.github.talib.Core;
import io.github.talib.OutRange;

double[] series = new double[252]; // your data, oldest first
double[] out = new double[252];

Core core = Core.DEFAULT;
OutRange r = core.midpoint(0, series.length - 1, series, 14, out);
System.out.println(r.count() + " values from bar " + r.begIdx() + "; last out = " + out[r.count() - 1]);
```

@tab C\#

```csharp
using System;
using TALib;

var series = new double[252]; // your data, oldest first
var outReal = new double[252];

var core = Core.Default;
OutRange r = core.Midpoint(0, series.Length - 1, series, 14, outReal);
Console.WriteLine($"{r.Count} values from bar {r.BegIdx}; last outReal = {outReal[r.Count - 1]}");
```

@tab TypeScript

```ts
import { midpoint } from 'ta-lib';

const series = new Float64Array(252); // your data, oldest first

const { begIdx, values } = midpoint(series, { timePeriod: 14 });
console.log(values.length + ' values from bar ' + begIdx + '; last values = ' + values[values.length - 1]);
```

:::

## Implementation

Added in 0.4.0.

TA-Lib Definition: [`midpoint.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/midpoint/midpoint.c) · [`midpoint.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/midpoint/midpoint.yaml)

| Native | File |
|--------|------|
| C | [`ta_MIDPOINT.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_MIDPOINT.c) |
| Rust | [`midpoint.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/library/src/ta_func/midpoint.rs) |
| Java | [`Core_MIDPOINT.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/fragments/Core_MIDPOINT.java) |
| C# | [`Core_MIDPOINT.cs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/csharp/library/src/Core_MIDPOINT.cs) |
| TypeScript | [`midpoint.ts`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/ts/library/src/func/midpoint.ts) |

Python: `talib.MIDPOINT`, see [ta-lib-python](https://ta-lib.github.io/ta-lib-python/func_groups/overlap_studies.html).

TA-Lib is also available for R and more using a [wrapper](/install/#wrappers).

## See Also

[MIDPRICE](/functions/midprice.md) · [MAX](/functions/max.md) · [MIN](/functions/min.md)
