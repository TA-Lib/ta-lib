---
title: "Derivative Oscillator (DOSC)"
description: "Derivative Oscillator: Wilder's RSI, smoothed by two exponential averages in series, with a simple average of that smoothed line subtracted from it."
---

## Summary

Derivative Oscillator: Wilder's RSI, smoothed by two exponential averages in series, with a simple average of that smoothed line subtracted from it. Constance Brown's reading is that the RSI's own swings are too noisy to time with, so she smooths it twice and then plots the distance from its own average as a histogram — MACD's histogram construction, applied to a smoothed RSI rather than to price. The result is in RSI points, centred on zero: crossings of the zero line mark the turn, and the height measures how far the smoothed RSI has run from its mean.

## Chart

<ta-chart src="/functions/dosc.svg" ratio="720:416" label="DOSC over IBM, daily, 1999" alt="Drawn on the price pane over the close as a line; below it, a separate indicator pane with a zero line showing outReal as a sign-coloured histogram."></ta-chart>

::: details Chart metadata <span class="flag-tip" tabindex="0" role="note" aria-label="The chart is drawn from the function's metadata: the flags and attributes the abstract API reports for it and for each output, and the input series its reference run declares." data-tip="The chart is drawn from the function's metadata: the flags and attributes the abstract API reports for it and for each output, and the input series its reference run declares.">i</span>

Drawn on the price pane over the close as a line; below it, a separate indicator pane with a zero line showing outReal as a sign-coloured histogram.

Metadata flags used: `outReal: histogram, positive, negative`

:::

[Reference data (CSV)](/functions/dosc.csv) · [Chart (SVG)](/functions/dosc.svg)

## Formula

RSI = RSI(inReal, timePeriod)   (Wilder, SMA-seeded)

S1 = EMA(RSI, firstPeriod)

DS = EMA(S1, secondPeriod)

DOSC = DS - SMA(DS, signalPeriod)

## Notes

- Every stage is a function TA-Lib already ships, and the output is identical, bit for bit, to `TA_RSI` followed by two `TA_EMA` calls, a `TA_SMA` and a `TA_SUB`. That chain takes five calls and their intermediate buffers; this computes it in one pass without materialising them.
- Each exponential average is seeded with a simple average of its own first inputs, the same seeding TA-Lib's EMA uses, and the second seeds on what the first publishes. `TA_SetUnstablePeriod` on either `TA_FUNC_UNST_RSI` or `TA_FUNC_UNST_EMA` discards more of that warm-up, and the EMA setting counts twice because there are two exponential stages. Implementations seeding each stage from a single first sample differ over the transient and agree once it decays.
- The degenerate reading is whatever `TA_RSI` answers when neither a gain nor a loss has been seen since the seed. Some other implementations answer 100 there and will disagree over that stretch.
- The periods are independent: no ordering between the two smoothing periods is required or checked. Swapping them changes the outputs, most at the start: each average is seeded on its own inputs, and that difference decays to rounding level. A signal period of 1 would make the output identically zero, so the minimum is 2.

## Inputs

- `inReal` — Input series, usually the close

## Outputs

- `outReal` — Derivative Oscillator, in RSI points, centred on zero

## Parameters

| Parameter | Type | Default | Accepted values | Description |
| --- | --- | --- | --- | --- |
| `optInTimePeriod` | integer | 14 | 2–100000 | Period of the RSI |
| `optInFirstPeriod` | integer | 5 | 2–100000 | Period of the first smoothing, applied to the RSI |
| `optInSecondPeriod` | integer | 3 | 2–100000 | Period of the second smoothing, applied to the first |
| `optInSignalPeriod` | integer | 9 | 2–100000 | Period of the simple average subtracted from the smoothed line |

## Properties

**Numerical Stability:** [Initial Unstable Period](/functions/stability.md#initial-unstable-period) — Inherited from EMA and RSI, which DOSC computes internally; tunable via EMA and RSI's unstable period.

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
double out[252];
int outBegIdx, outNBElement;

TA_Initialize();
TA_DOSC( 0, n - 1, series, 14, 5, 3, 9, &outBegIdx, &outNBElement, out );
printf( "%d values from bar %d; last out = %f\n", outNBElement, outBegIdx, out[outNBElement - 1] );
```

@tab Rust

```rust
use ta_lib::Core;

let series = vec![0.0; 252]; // your data, oldest first
let mut out = vec![0.0; 252];

let core = Core::new();
let range = core.dosc(0, series.len() - 1, &series, 14, 5, 3, 9, &mut out)?;
println!("{} values from bar {}; last out = {}", range.count, range.beg_idx, out[range.count - 1]);
```

@tab Java

```java
import io.github.talib.Core;
import io.github.talib.OutRange;

double[] series = new double[252]; // your data, oldest first
double[] out = new double[252];

Core core = Core.DEFAULT;
OutRange r = core.dosc(0, series.length - 1, series, 14, 5, 3, 9, out);
System.out.println(r.count() + " values from bar " + r.begIdx() + "; last out = " + out[r.count() - 1]);
```

@tab C\#

```csharp
using System;
using TALib;

var series = new double[252]; // your data, oldest first
var outReal = new double[252];

var core = Core.Default;
OutRange r = core.Dosc(0, series.Length - 1, series, 14, 5, 3, 9, outReal);
Console.WriteLine($"{r.Count} values from bar {r.BegIdx}; last outReal = {outReal[r.Count - 1]}");
```

@tab TypeScript

```ts
import { dosc } from 'ta-lib';

const series = new Float64Array(252); // your data, oldest first

const { begIdx, values } = dosc(series, {
  timePeriod: 14,
  firstPeriod: 5,
  secondPeriod: 3,
  signalPeriod: 9,
});
console.log(values.length + ' values from bar ' + begIdx + '; last values = ' + values[values.length - 1]);
```

:::

## Implementation

Added in 0.8.3 (ta-lib-python 0.8.3).

TA-Lib Definition: [`dosc.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/dosc/dosc.c) · [`dosc.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/dosc/dosc.yaml)

| Native | File |
|--------|------|
| C | [`ta_DOSC.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_DOSC.c) |
| Rust | [`dosc.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/library/src/ta_func/dosc.rs) |
| Java | [`Core_DOSC.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/fragments/Core_DOSC.java) |
| C# | [`Core_DOSC.cs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/csharp/library/src/Core_DOSC.cs) |
| TypeScript | [`dosc.ts`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/ts/library/src/func/dosc.ts) |

Python: `talib.DOSC` (ta-lib-python 0.8.3 or later), see [ta-lib-python](https://ta-lib.github.io/ta-lib-python/func_groups/momentum_indicators.html).

TA-Lib is also available for R and more using a [wrapper](/install/#wrappers).

## Aliases

derivative oscillator

## See Also

[RSI](/functions/rsi.md) · [STOCHRSI](/functions/stochrsi.md) · [MACD](/functions/macd.md) · [AC](/functions/ac.md)

## References

- Constance M. Brown, "The Derivative Oscillator: A New Approach for an Old Problem", *MTA Journal* (Market Technicians Association), Issue 42, Winter 1993-Spring 1994, pp. 45-55 and 61
