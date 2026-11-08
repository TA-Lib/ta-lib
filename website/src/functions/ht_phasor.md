---
title: "Hilbert Transform - Phasor Components (HT_PHASOR)"
description: "Hilbert Transform indicator that decomposes the price series into its in-phase (I) and quadrature (Q) phasor components."
---

## Summary

Hilbert Transform indicator that decomposes the price series into its in-phase (I) and quadrature (Q) phasor components. Shares the same detrend/Hilbert machinery as the other HT_* cycle functions.

This function is meant for building your own cycle analysis on top of the raw phasor, not as a ready-made signal.

## Chart

<ta-chart src="/functions/ht_phasor.svg" ratio="720:416" label="HT_PHASOR over IBM, daily, 1999" alt="Drawn on the price pane over the close as a line; below it, a separate indicator pane with a zero line showing outInPhase as a solid line and outQuadrature as a dashed line."></ta-chart>

::: details Chart metadata <span class="flag-tip" tabindex="0" role="note" aria-label="The chart is drawn from the function's metadata: the flags and attributes the abstract API reports for it and for each output, and the input series its reference run declares." data-tip="The chart is drawn from the function's metadata: the flags and attributes the abstract API reports for it and for each output, and the input series its reference run declares.">i</span>

Drawn on the price pane over the close as a line; below it, a separate indicator pane with a zero line showing outInPhase as a solid line and outQuadrature as a dashed line.

Metadata flags used: `outInPhase: line, positive, negative` · `outQuadrature: dash_line, positive, negative`

:::

[Reference data (CSV)](/functions/ht_phasor.csv) · [Chart (SVG)](/functions/ht_phasor.svg)

## Formula

Smooth price with a 4-bar WMA (weights 1,2,3,4 /10). Apply the Hilbert Transform (a=0.0962, b=0.5769, scaled per bar by adjustedPrevPeriod = 0.075*period + 0.54) to get detrender = HT(smoothed) and Q1 = HT(detrender). Output: outInPhase = detrender delayed 3 price bars; outQuadrature = Q1.

## Interpretation

I and Q are the Cartesian coordinates of the rotating cycle vector — the same phasor `HT_DCPHASE` reports as an angle (`atan(Q/I)`), exposed here raw and unsmoothed, one step before that estimate. There's no standalone I/Q trading rule from Ehlers; the practical signals sit downstream, in `HT_DCPERIOD`, `HT_DCPHASE`, `HT_SINE`, and `HT_TRENDMODE`, which each smooth this same decomposition into a usable period, phase, or trend/cycle read.

## Inputs

- `inReal` — Source price series

## Outputs

- `outInPhase` — In-phase component (detrender delayed 3 bars)
- `outQuadrature` — Quadrature component (Q1 of the Hilbert Transform)

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
double inPhase[252], quadrature[252];
int outBegIdx, outNBElement;

TA_Initialize();
TA_HT_PHASOR( 0, n - 1, series, &outBegIdx, &outNBElement, inPhase, quadrature );
printf( "%d values from bar %d; last inPhase = %f\n", outNBElement, outBegIdx, inPhase[outNBElement - 1] );
```

@tab Rust

```rust
use ta_lib::Core;

let series = vec![0.0; 252]; // your data, oldest first
let mut in_phase = vec![0.0; 252];
let mut quadrature = vec![0.0; 252];

let core = Core::new();
let range = core.ht_phasor(0, series.len() - 1, &series, &mut in_phase, &mut quadrature)?;
println!("{} values from bar {}; last in_phase = {}", range.count, range.beg_idx, in_phase[range.count - 1]);
```

@tab Java

```java
import io.github.talib.Core;
import io.github.talib.OutRange;

double[] series = new double[252]; // your data, oldest first
double[] inPhase = new double[252];
double[] quadrature = new double[252];

Core core = Core.DEFAULT;
OutRange r = core.htPhasor(0, series.length - 1, series, inPhase, quadrature);
System.out.println(r.count() + " values from bar " + r.begIdx() + "; last inPhase = " + inPhase[r.count() - 1]);
```

@tab C\#

```csharp
using System;
using TALib;

var series = new double[252]; // your data, oldest first
var inPhase = new double[252];
var quadrature = new double[252];

var core = Core.Default;
OutRange r = core.HtPhasor(0, series.Length - 1, series, inPhase, quadrature);
Console.WriteLine($"{r.Count} values from bar {r.BegIdx}; last inPhase = {inPhase[r.Count - 1]}");
```

@tab TypeScript

```ts
import { htPhasor } from 'ta-lib';

const series = new Float64Array(252); // your data, oldest first

const { begIdx, outInPhase, outQuadrature } = htPhasor(series);
console.log(outInPhase.length + ' values from bar ' + begIdx + '; last outInPhase = ' + outInPhase[outInPhase.length - 1]);
```

:::

## Implementation

Added in 0.4.0.

TA-Lib Definition: [`ht_phasor.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/ht_phasor/ht_phasor.c) · [`ht_phasor.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/ht_phasor/ht_phasor.yaml)

| Native | File |
|--------|------|
| C | [`ta_HT_PHASOR.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_HT_PHASOR.c) |
| Rust | [`ht_phasor.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/library/src/ta_func/ht_phasor.rs) |
| Java | [`Core_HT_PHASOR.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/fragments/Core_HT_PHASOR.java) |
| C# | [`Core_HT_PHASOR.cs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/csharp/library/src/Core_HT_PHASOR.cs) |
| TypeScript | [`htPhasor.ts`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/ts/library/src/func/htPhasor.ts) |

Python: `talib.HT_PHASOR`, see [ta-lib-python](https://ta-lib.github.io/ta-lib-python/func_groups/cycle_indicators.html).

TA-Lib is also available for R and more using a [wrapper](/install/#wrappers).

## Aliases

Hilbert Transform Phasor, InPhase Quadrature

## See Also

[HT_DCPERIOD](/functions/ht_dcperiod.md) · [HT_DCPHASE](/functions/ht_dcphase.md) · [HT_SINE](/functions/ht_sine.md) · [HT_TRENDMODE](/functions/ht_trendmode.md) · [MAMA](/functions/mama.md) · [WMA](/functions/wma.md)

## References

- John F. Ehlers, *Rocket Science for Traders: Digital Signal Processing Applications*, John Wiley & Sons (ISBN 0471405671)
