---
title: "Hilbert Transform - Instantaneous Trendline (HT_TRENDLINE)"
description: "Ehlers' Hilbert Transform Instantaneous Trendline: a smoothed, low-lag overlay whose averaging window adapts to the dominant cycle period measured via…"
---

## Summary

Ehlers' Hilbert Transform Instantaneous Trendline: a smoothed, low-lag overlay whose averaging window adapts to the dominant cycle period measured via Hilbert-transform quadrature (I/Q) analysis of price.

## Chart

<ta-chart src="/functions/ht_trendline.svg" ratio="720:274" label="HT_TRENDLINE over IBM, daily, 1999" alt="Drawn on the price pane over the close as a line, with outReal as a solid line."></ta-chart>

::: details Chart metadata <span class="flag-tip" tabindex="0" role="note" aria-label="The chart is drawn from the function's metadata: the flags and attributes the abstract API reports for it and for each output, and the input series its reference run declares." data-tip="The chart is drawn from the function's metadata: the flags and attributes the abstract API reports for it and for each output, and the input series its reference run declares.">i</span>

Drawn on the price pane over the close as a line, with outReal as a solid line.

Metadata flags used: `overlap` · `outReal: line`

:::

[Reference data (CSV)](/functions/ht_trendline.csv) · [Chart (SVG)](/functions/ht_trendline.svg)

## Inputs

- `inReal` — Source price series

## Outputs

- `outReal` — Instantaneous trendline value

## Properties

**Numerical Stability:** [Initial Unstable Period](/functions/stability.md#initial-unstable-period)

<div class="flag-table">

|  |
| :-- |
| <span class="flag-box">✅</span> **Overlap Input** <span class="flag-tip" tabindex="0" role="note" aria-label="Output is on the same scale as the input price, so it is drawn over the price chart." data-tip="Output is on the same scale as the input price, so it is drawn over the price chart.">i</span> |
| <span class="flag-box">☐</span> <span style="opacity:0.5">Independent Y-Axis</span> |
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
TA_HT_TRENDLINE( 0, n - 1, series, &outBegIdx, &outNBElement, out );
printf( "%d values from bar %d; last out = %f\n", outNBElement, outBegIdx, out[outNBElement - 1] );
```

@tab Rust

```rust
use ta_lib::Core;

let series = vec![0.0; 252]; // your data, oldest first
let mut out = vec![0.0; 252];

let core = Core::new();
let range = core.ht_trendline(0, series.len() - 1, &series, &mut out)?;
println!("{} values from bar {}; last out = {}", range.count, range.beg_idx, out[range.count - 1]);
```

@tab Java

```java
import io.github.talib.Core;
import io.github.talib.OutRange;

double[] series = new double[252]; // your data, oldest first
double[] out = new double[252];

Core core = Core.DEFAULT;
OutRange r = core.htTrendline(0, series.length - 1, series, out);
System.out.println(r.count() + " values from bar " + r.begIdx() + "; last out = " + out[r.count() - 1]);
```

@tab C\#

```csharp
using System;
using TALib;

var series = new double[252]; // your data, oldest first
var outReal = new double[252];

var core = Core.Default;
OutRange r = core.HtTrendline(0, series.Length - 1, series, outReal);
Console.WriteLine($"{r.Count} values from bar {r.BegIdx}; last outReal = {outReal[r.Count - 1]}");
```

@tab TypeScript

```ts
import { htTrendline } from 'ta-lib';

const series = new Float64Array(252); // your data, oldest first

const { begIdx, values } = htTrendline(series);
console.log(values.length + ' values from bar ' + begIdx + '; last values = ' + values[values.length - 1]);
```

:::

## Implementation

Added in 0.4.0.

TA-Lib Definition: [`ht_trendline.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/ht_trendline/ht_trendline.c) · [`ht_trendline.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/ht_trendline/ht_trendline.yaml)

| Native | File |
|--------|------|
| C | [`ta_HT_TRENDLINE.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_HT_TRENDLINE.c) |
| Rust | [`ht_trendline.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/library/src/ta_func/ht_trendline.rs) |
| Java | [`Core_HT_TRENDLINE.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/fragments/Core_HT_TRENDLINE.java) |
| C# | [`Core_HT_TRENDLINE.cs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/csharp/library/src/Core_HT_TRENDLINE.cs) |
| TypeScript | [`htTrendline.ts`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/ts/library/src/func/htTrendline.ts) |

Python: `talib.HT_TRENDLINE`, see [ta-lib-python](https://ta-lib.github.io/ta-lib-python/func_groups/overlap_studies.html).

TA-Lib is also available for R and more using a [wrapper](/install/#wrappers).

## Aliases

Hilbert Transform Instantaneous Trendline, Instantaneous Trendline

## See Also

[HT_DCPERIOD](/functions/ht_dcperiod.md) · [HT_PHASOR](/functions/ht_phasor.md) · [MAMA](/functions/mama.md) · [WMA](/functions/wma.md)

## References

- John F. Ehlers, *Rocket Science for Traders: Digital Signal Processing Applications*, John Wiley & Sons (ISBN 0471405671)
