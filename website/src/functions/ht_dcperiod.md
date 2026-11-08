---
title: "Hilbert Transform - Dominant Cycle Period (HT_DCPERIOD)"
description: "Hilbert Transform estimate of the dominant cycle period (in bars) of the price series. Outputs the smoothed instantaneous cycle period."
---

## Summary

Hilbert Transform estimate of the dominant cycle period (in bars) of the price series. Outputs the smoothed instantaneous cycle period. Output is the estimated dominant cycle length in bars (clamped to 6-50).

## Chart

<ta-chart src="/functions/ht_dcperiod.svg" ratio="720:416" label="HT_DCPERIOD over IBM, daily, 1999" alt="Drawn on the price pane over the close as a line; below it, a separate indicator pane showing outReal as a solid line."></ta-chart>

Drawn on the price pane over the close as a line; below it, a separate indicator pane showing outReal as a solid line. [Reference data (CSV)](/functions/ht_dcperiod.csv) · [Chart (SVG)](/functions/ht_dcperiod.svg)

## Inputs

- `inReal` — Source price/value series

## Outputs

- `outReal` — Smoothed dominant cycle period in bars

## Properties

**Numerical Stability:** [Initial Unstable Period](/functions/stability.md#initial-unstable-period)

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
TA_HT_DCPERIOD( 0, n - 1, series, &outBegIdx, &outNBElement, out );
printf( "%d values from bar %d; last out = %f\n", outNBElement, outBegIdx, out[outNBElement - 1] );
```

@tab Rust

```rust
use ta_lib::Core;

let series = vec![0.0; 252]; // your data, oldest first
let mut out = vec![0.0; 252];

let core = Core::new();
let range = core.ht_dcperiod(0, series.len() - 1, &series, &mut out)?;
println!("{} values from bar {}; last out = {}", range.count, range.beg_idx, out[range.count - 1]);
```

@tab Java

```java
import io.github.talib.Core;
import io.github.talib.OutRange;

double[] series = new double[252]; // your data, oldest first
double[] out = new double[252];

Core core = Core.DEFAULT;
OutRange r = core.htDcperiod(0, series.length - 1, series, out);
System.out.println(r.count() + " values from bar " + r.begIdx() + "; last out = " + out[r.count() - 1]);
```

@tab C\#

```csharp
using System;
using TALib;

var series = new double[252]; // your data, oldest first
var outReal = new double[252];

var core = Core.Default;
OutRange r = core.HtDcperiod(0, series.Length - 1, series, outReal);
Console.WriteLine($"{r.Count} values from bar {r.BegIdx}; last outReal = {outReal[r.Count - 1]}");
```

@tab TypeScript

```ts
import { htDcperiod } from 'ta-lib';

const series = new Float64Array(252); // your data, oldest first

const { begIdx, values } = htDcperiod(series);
console.log(values.length + ' values from bar ' + begIdx + '; last values = ' + values[values.length - 1]);
```

:::

## Implementation

Added in 0.4.0.

TA-Lib Definition: [`ht_dcperiod.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/ht_dcperiod/ht_dcperiod.c) · [`ht_dcperiod.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/ht_dcperiod/ht_dcperiod.yaml)

| Native | File |
|--------|------|
| C | [`ta_HT_DCPERIOD.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_HT_DCPERIOD.c) |
| Rust | [`ht_dcperiod.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/library/src/ta_func/ht_dcperiod.rs) |
| Java | [`Core_HT_DCPERIOD.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/fragments/Core_HT_DCPERIOD.java) |
| C# | [`Core_HT_DCPERIOD.cs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/csharp/library/src/Core_HT_DCPERIOD.cs) |
| TypeScript | [`htDcperiod.ts`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/ts/library/src/func/htDcperiod.ts) |

Python: `talib.HT_DCPERIOD`, see [ta-lib-python](https://ta-lib.github.io/ta-lib-python/func_groups/cycle_indicators.html).

TA-Lib is also available for R and more using a [wrapper](/install/#wrappers).

## Aliases

Hilbert Transform Dominant Cycle Period, Dominant Cycle Period

## See Also

[HT_DCPHASE](/functions/ht_dcphase.md) · [HT_PHASOR](/functions/ht_phasor.md) · [HT_SINE](/functions/ht_sine.md) · [HT_TRENDMODE](/functions/ht_trendmode.md) · [MAMA](/functions/mama.md) · [WMA](/functions/wma.md)

## References

- John F. Ehlers, *Rocket Science for Traders: Digital Signal Processing Applications*, John Wiley & Sons (ISBN 0471405671)
