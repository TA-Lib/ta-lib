---
title: "WaveTrend Oscillator (WAVETREND)"
description: "WaveTrend Oscillator: how far the typical price sits from its own exponential average, divided by an exponential average of that distance and by…"
---

## Summary

WaveTrend Oscillator: how far the typical price sits from its own exponential average, divided by an exponential average of that distance and by Lambert's CCI constant, then smoothed. The reading is the plain stochastic's complaint answered a different way — rather than bounding the oscillator by construction, it scales it by how far price has recently been travelling, so the same numeric level means the same thing in a quiet market and a fast one. The oscillator line and its short simple average cross; the crossings that matter are the ones beyond the extremes, which the author draws at ±53 and ±60.

## Chart

<ta-chart src="/functions/wavetrend.svg" ratio="720:416" label="WAVETREND over IBM, daily, 1999" alt="Drawn on the price pane over candles; below it, a separate indicator pane with a zero line showing outWT1 as a solid line with dashed rules at -60, -53, 53 and 60 and outWT2 as a dashed line."></ta-chart>

::: details Chart metadata <span class="flag-tip" tabindex="0" role="note" aria-label="The chart is drawn from the function's metadata: the flags and attributes the abstract API reports for it and for each output, and the input series its reference run declares." data-tip="The chart is drawn from the function's metadata: the flags and attributes the abstract API reports for it and for each output, and the input series its reference run declares.">i</span>

Drawn on the price pane over candles; below it, a separate indicator pane with a zero line showing outWT1 as a solid line with dashed rules at -60, -53, 53 and 60 and outWT2 as a dashed line.

Metadata flags used: `inPriceHLC: high, low, close` · `outWT1: line, positive, negative, levels -60, -53, 53, 60` · `outWT2: dash_line, positive, negative`

:::

[Reference data (CSV)](/functions/wavetrend.csv) · [Chart (SVG)](/functions/wavetrend.svg)

## Formula

ap = (high + low + close) / 3

esa = EMA(ap, channelPeriod)

d = EMA(|ap - esa|, channelPeriod)

ci = (ap - esa) / (0.015 * d)

WT1 = EMA(ci, averagePeriod)

WT2 = SMA(WT1, signalPeriod)

## Notes

- The middle stage is an exponential CCI, not `TA_CCI`: CCI averages with a simple moving average and takes the mean deviation around that window's own average, where this uses two exponential averages.
- On a market that has stopped moving, the exponential average stops moving too — once its step falls under half an ulp it freezes, a few ulps away from the price. That frozen gap is a real non-zero distance, so dividing by its own average walks the oscillator to ±66.67, an extreme reading produced by nothing but rounding. This answers 0 instead, by taking the distance as zero whenever the price and its average both repeat. Implementations without that test drift to the extreme on flat data, at a bar that depends on their arithmetic.
- A zero divisor is tested on the scaled deviation, the quantity the division actually uses, and answers the neutral 0 rather than dividing.
- Each exponential average is seeded with a simple average of its own first inputs, the same seeding TA-Lib's EMA uses, and each seeds on what the stage before it publishes. `TA_SetUnstablePeriod(TA_FUNC_UNST_EMA, ...)` discards more of that warm-up, and it counts three times because there are three exponential stages.
- The difference the author also plots is `TA_SUB(outWT1, outWT2)`.

## Inputs

- `inHigh` — High price series
- `inLow` — Low price series
- `inClose` — Close price series

## Outputs

- `outWT1` — WaveTrend oscillator line
- `outWT2` — Simple average of the oscillator line

## Parameters

| Parameter | Type | Default | Accepted values | Description |
| --- | --- | --- | --- | --- |
| `optInChannelPeriod` | integer | 10 | 2–100000 | Period of the price channel, used by both the average and the deviation |
| `optInAveragePeriod` | integer | 21 | 1–100000 | Smoothing for the oscillator line |
| `optInSignalPeriod` | integer | 4 | 1–100000 | Period of the simple average making the signal line |

## Properties

**Numerical Stability:** [Initial Unstable Period](/functions/stability.md#initial-unstable-period) — Inherited from EMA, which WAVETREND computes internally; tunable via EMA's unstable period.

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
double high[252]; /* your data, oldest first */
double low[252]; /* your data, oldest first */
double close[252]; /* your data, oldest first */
double wt1[252], wt2[252];
int outBegIdx, outNBElement;

TA_Initialize();
TA_WAVETREND( 0, n - 1, high, low, close, 10, 21, 4, &outBegIdx, &outNBElement, wt1, wt2 );
printf( "%d values from bar %d; last wt1 = %f\n", outNBElement, outBegIdx, wt1[outNBElement - 1] );
```

@tab Rust

```rust
use ta_lib::Core;

let high = vec![0.0; 252]; // your data, oldest first
let low = vec![0.0; 252]; // your data, oldest first
let close = vec![0.0; 252]; // your data, oldest first
let mut wt1 = vec![0.0; 252];
let mut wt2 = vec![0.0; 252];

let core = Core::new();
let range = core.wavetrend(0, high.len() - 1, &high, &low, &close, 10, 21, 4,
                           &mut wt1, &mut wt2)?;
println!("{} values from bar {}; last wt1 = {}", range.count, range.beg_idx, wt1[range.count - 1]);
```

@tab Java

```java
import io.github.talib.Core;
import io.github.talib.OutRange;

double[] high = new double[252]; // your data, oldest first
double[] low = new double[252]; // your data, oldest first
double[] close = new double[252]; // your data, oldest first
double[] wt1 = new double[252];
double[] wt2 = new double[252];

Core core = Core.DEFAULT;
OutRange r = core.wavetrend(0, high.length - 1, high, low, close, 10, 21, 4, wt1, wt2);
System.out.println(r.count() + " values from bar " + r.begIdx() + "; last wt1 = " + wt1[r.count() - 1]);
```

@tab C\#

```csharp
using System;
using TALib;

var high = new double[252]; // your data, oldest first
var low = new double[252]; // your data, oldest first
var close = new double[252]; // your data, oldest first
var wt1 = new double[252];
var wt2 = new double[252];

var core = Core.Default;
OutRange r = core.Wavetrend(0, high.Length - 1, high, low, close, 10, 21, 4, wt1, wt2);
Console.WriteLine($"{r.Count} values from bar {r.BegIdx}; last wt1 = {wt1[r.Count - 1]}");
```

@tab TypeScript

```ts
import { wavetrend } from 'ta-lib';

const high = new Float64Array(252); // your data, oldest first
const low = new Float64Array(252); // your data, oldest first
const close = new Float64Array(252); // your data, oldest first

const { begIdx, outWt1, outWt2 } = wavetrend({ high, low, close }, {
  channelPeriod: 10,
  averagePeriod: 21,
  signalPeriod: 4,
});
console.log(outWt1.length + ' values from bar ' + begIdx + '; last outWt1 = ' + outWt1[outWt1.length - 1]);
```

:::

## Implementation

Added in 0.8.3 (ta-lib-python 0.8.3).

TA-Lib Definition: [`wavetrend.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/wavetrend/wavetrend.c) · [`wavetrend.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/wavetrend/wavetrend.yaml)

| Native | File |
|--------|------|
| C | [`ta_WAVETREND.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_WAVETREND.c) |
| Rust | [`wavetrend.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/library/src/ta_func/wavetrend.rs) |
| Java | [`Core_WAVETREND.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/fragments/Core_WAVETREND.java) |
| C# | [`Core_WAVETREND.cs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/csharp/library/src/Core_WAVETREND.cs) |
| TypeScript | [`wavetrend.ts`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/ts/library/src/func/wavetrend.ts) |

Python: `talib.WAVETREND` (ta-lib-python 0.8.3 or later), see [ta-lib-python](https://ta-lib.github.io/ta-lib-python/func_groups/momentum_indicators.html).

TA-Lib is also available for R and more using a [wrapper](/install/#wrappers).

## Aliases

wavetrend, wavetrend oscillator, WT

## See Also

[CCI](/functions/cci.md) · [SMI](/functions/smi.md) · [STOCHRSI](/functions/stochrsi.md) · [TSI](/functions/tsi.md)

## References

- LazyBear, "WaveTrend Oscillator [WT]", TradingView published script, 2014
