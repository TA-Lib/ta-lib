---
title: "MESA Adaptive Moving Average (MAMA)"
description: "MESA Adaptive Moving Average: an adaptive EMA whose smoothing factor is driven by the dominant-cycle phase rate measured with a Hilbert transform."
---

## Summary

MESA Adaptive Moving Average: an adaptive EMA whose smoothing factor is driven by the dominant-cycle phase rate measured with a Hilbert transform. Emits two lines, MAMA and its slower follower FAMA. MAMA crossing above FAMA is bullish; crossing below is bearish.

## Chart

<ta-chart src="/functions/mama.svg" ratio="720:274" label="MAMA over IBM, daily, 1999" alt="Drawn on the price pane over the close as a line, with outMAMA as a solid line and outFAMA as a dashed line."></ta-chart>

::: details Chart metadata <span class="flag-tip" tabindex="0" role="note" aria-label="The chart is drawn automatically from metadata TA-Lib provides. Your app can get the same through the Abstract API." data-tip="The chart is drawn automatically from metadata TA-Lib provides. Your app can get the same through the Abstract API.">i</span>

Drawn on the price pane over the close as a line, with outMAMA as a solid line and outFAMA as a dashed line.

Metadata flags used: `overlap` · `outMAMA: line` · `outFAMA: dash_line`

:::

[Reference data (CSV)](/functions/mama.csv) · [Chart (SVG)](/functions/mama.svg)

## Formula

phase = atan(Q1/I1) in degrees; deltaPhase = max(1, prevPhase - phase)
alpha = max(fastLimit/deltaPhase, slowLimit) if deltaPhase>1 else fastLimit
MAMA = alpha*price + (1-alpha)*MAMA_prev
FAMA = (alpha/2)*MAMA + (1-alpha/2)*FAMA_prev

## Inputs

- `inReal` — Price series to smooth

## Outputs

- `outMAMA` — Adaptive moving average (fast line)
- `outFAMA` — Following adaptive moving average, using half the alpha (slow line)

## Parameters

| Parameter | Type | Default | Accepted values | Description |
| --- | --- | --- | --- | --- |
| `optInFastLimit` | real | 0.5 | 0.01–0.99 | Upper bound on the adaptive smoothing factor |
| `optInSlowLimit` | real | 0.05 | 0.01–0.99 | Lower bound on the adaptive smoothing factor |

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
double mama[252], fama[252];
int outBegIdx, outNBElement;

TA_Initialize();
TA_MAMA( 0, n - 1, series, 0.5, 0.05, &outBegIdx, &outNBElement, mama, fama );
printf( "%d values from bar %d; last mama = %f\n", outNBElement, outBegIdx, mama[outNBElement - 1] );
```

@tab Rust

```rust
use ta_lib::Core;

let series = vec![0.0; 252]; // your data, oldest first
let mut mama = vec![0.0; 252];
let mut fama = vec![0.0; 252];

let core = Core::new();
let range = core.mama(0, series.len() - 1, &series, 0.5, 0.05, &mut mama, &mut fama)?;
println!("{} values from bar {}; last mama = {}", range.count, range.beg_idx, mama[range.count - 1]);
```

@tab Java

```java
import io.github.talib.Core;
import io.github.talib.OutRange;

double[] series = new double[252]; // your data, oldest first
double[] mama = new double[252];
double[] fama = new double[252];

Core core = Core.DEFAULT;
OutRange r = core.mama(0, series.length - 1, series, 0.5, 0.05, mama, fama);
System.out.println(r.count() + " values from bar " + r.begIdx() + "; last mama = " + mama[r.count() - 1]);
```

@tab C\#

```csharp
using System;
using TALib;

var series = new double[252]; // your data, oldest first
var mama = new double[252];
var fama = new double[252];

var core = Core.Default;
OutRange r = core.Mama(0, series.Length - 1, series, 0.5, 0.05, mama, fama);
Console.WriteLine($"{r.Count} values from bar {r.BegIdx}; last mama = {mama[r.Count - 1]}");
```

@tab TypeScript

```ts
import { mama } from 'ta-lib';

const series = new Float64Array(252); // your data, oldest first

const { begIdx, outMama, outFama } = mama(series, {
  fastLimit: 0.5,
  slowLimit: 0.05,
});
console.log(outMama.length + ' values from bar ' + begIdx + '; last outMama = ' + outMama[outMama.length - 1]);
```

:::

## Implementation

Added in 0.4.0.

TA-Lib Definition: [`mama.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/mama/mama.c) · [`mama.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/mama/mama.yaml)

| Native | File |
|--------|------|
| C | [`ta_MAMA.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_MAMA.c) |
| Rust | [`mama.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/library/src/ta_func/mama.rs) |
| Java | [`Core_MAMA.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/fragments/Core_MAMA.java) |
| C# | [`Core_MAMA.cs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/csharp/library/src/Core_MAMA.cs) |
| TypeScript | [`mama.ts`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/ts/library/src/func/mama.ts) |

Python: `talib.MAMA`, see [ta-lib-python](https://ta-lib.github.io/ta-lib-python/func_groups/overlap_studies.html).

TA-Lib is also available for R and more using a [wrapper](/install/#wrappers).

## Aliases

MESA Adaptive Moving Average, Ehlers MAMA

## See Also

[MA](/functions/ma.md) · [WMA](/functions/wma.md) · [HT_DCPERIOD](/functions/ht_dcperiod.md)

## References

- John F. Ehlers, *Rocket Science for Traders: Digital Signal Processing Applications*, John Wiley & Sons (ISBN 0471405671)
