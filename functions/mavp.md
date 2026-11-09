---
url: 'https://ta-lib.org/functions/mavp.md'
description: >-
  Moving average whose period varies per bar, driven by a companion period
  series.
---
# Moving average with variable period (MAVP)

## Summary

Moving average whose period varies per bar, driven by a companion period series. For each bar it computes an MA of the selected type over the (clamped) period given by inPeriods.

## Chart

<ta-chart src="/functions/mavp.svg" ratio="720:274" label="MAVP over IBM, daily, 1999" alt="Drawn on the price pane over the close as a line, with outReal as a solid line."></ta-chart>

::: details Chart metadata <span class="flag-tip" tabindex="0" role="note" aria-label="The chart is drawn from the function's metadata: the flags and attributes the abstract API reports for it and for each output, and the input series its reference run declares." data-tip="The chart is drawn from the function's metadata: the flags and attributes the abstract API reports for it and for each output, and the input series its reference run declares.">i</span>

Drawn on the price pane over the close as a line, with outReal as a solid line.

Metadata flags used: `overlap` · `outReal: line`

:::

[Reference data (CSV)](/functions/mavp.csv) · [Chart (SVG)](/functions/mavp.svg)

## Formula

p_i = clamp((int)inPeriods\[startIdx+i], optInMinPeriod, optInMaxPeriod); outReal\[i] = MA(inReal, p_i, optInMAType) at bar startIdx+i

## Notes

* Fractional per-bar periods are truncated to whole numbers before being clamped to the minimum and maximum period.
* Period values of 1 perform no smoothing (the bar's output equals its input); the minimum allowed period is 1 since 0.6.5.
* `optInMinPeriod` must not exceed `optInMaxPeriod`; a larger minimum is rejected.

## Inputs

* `inReal` — series to be averaged
* `inPeriods` — per-bar desired MA period

## Outputs

* `outReal` — variable-period moving average

## Parameters

| Parameter | Type | Default | Accepted values | Description |
| --- | --- | --- | --- | --- |
| `optInMinPeriod` | integer | 2 | 1–10000 | Lower clamp for the per-bar period |
| `optInMaxPeriod` | integer | 30 | 1–10000 | Upper clamp for the per-bar period |
| `optInMAType` | MAType | SMA (0) | any MAType | Moving-average type applied |

*`MAType` values: 0 SMA · 1 EMA · 2 WMA · 3 DEMA · 4 TEMA · 5 TRIMA · 6 KAMA · 7 MAMA · 8 T3 · 9 HMA · 10 DISABLED · 11 DEFAULT · 12 ZLEMA · 13 RMA · 14 VIDYA · 15 ALMA*

## Properties

**Numerical Stability:** [Depends on MA Type](/functions/stability.md#depends-on-ma-type) — This function's default, SMA, is start-independent.

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
double periods[252]; /* one period per bar */
double out[252];
int outBegIdx, outNBElement;

TA_Initialize();
TA_MAVP( 0, n - 1, series, periods, 2, 30, TA_MAType_SMA, &outBegIdx, &outNBElement, out );
printf( "%d values from bar %d; last out = %f\n", outNBElement, outBegIdx, out[outNBElement - 1] );
```

@tab Rust

```rust
use ta_lib::{Core, MAType};

let series = vec![0.0; 252]; // your data, oldest first
let periods = vec![0.0; 252]; // one period per bar
let mut out = vec![0.0; 252];

let core = Core::new();
let range = core.mavp(0, series.len() - 1, &series, &periods, 2, 30, MAType::SMA,
                      &mut out)?;
println!("{} values from bar {}; last out = {}", range.count, range.beg_idx, out[range.count - 1]);
```

@tab Java

```java
import io.github.talib.Core;
import io.github.talib.MAType;
import io.github.talib.OutRange;

double[] series = new double[252]; // your data, oldest first
double[] periods = new double[252]; // one period per bar
double[] out = new double[252];

Core core = Core.DEFAULT;
OutRange r = core.mavp(0, series.length - 1, series, periods, 2, 30, MAType.SMA, out);
System.out.println(r.count() + " values from bar " + r.begIdx() + "; last out = " + out[r.count() - 1]);
```

@tab C#

```csharp
using System;
using TALib;

var series = new double[252]; // your data, oldest first
var periods = new double[252]; // one period per bar
var outReal = new double[252];

var core = Core.Default;
OutRange r = core.Mavp(0, series.Length - 1, series, periods, 2, 30, MAType.SMA, outReal);
Console.WriteLine($"{r.Count} values from bar {r.BegIdx}; last outReal = {outReal[r.Count - 1]}");
```

@tab TypeScript

```ts
import { MAType, mavp } from 'ta-lib';

const series = new Float64Array(252); // your data, oldest first
const periods = new Float64Array(252); // one period per bar

const { begIdx, values } = mavp(series, periods, {
  minPeriod: 2,
  maxPeriod: 30,
  maType: MAType.SMA,
});
console.log(values.length + ' values from bar ' + begIdx + '; last values = ' + values[values.length - 1]);
```

:::

## Implementation

Added in 0.4.0.

TA-Lib Definition: [`mavp.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/mavp/mavp.c) · [`mavp.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/mavp/mavp.yaml)

| Native | File |
|--------|------|
| C | [`ta_MAVP.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_MAVP.c) |
| Rust | [`mavp.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/library/src/ta_func/mavp.rs) |
| Java | [`Core_MAVP.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/fragments/Core_MAVP.java) |
| C# | [`Core_MAVP.cs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/csharp/library/src/Core_MAVP.cs) |
| TypeScript | [`mavp.ts`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/ts/library/src/func/mavp.ts) |

Python: `talib.MAVP`, see [ta-lib-python](https://ta-lib.github.io/ta-lib-python/func_groups/overlap_studies.html).

TA-Lib is also available for R and more using a [wrapper](/install/#wrappers).

## Aliases

Moving Average Variable Period, Variable Period Moving Average

## See Also

[MA](/functions/ma.md) · [SMA](/functions/sma.md) · [MAMA](/functions/mama.md) · [T3](/functions/t3.md)
