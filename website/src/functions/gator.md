---
title: "Gator Oscillator (GATOR)"
description: "Bill Williams' Gator Oscillator: how far apart the three Alligator lines are, as two histograms."
---

## Summary

Bill Williams' Gator Oscillator: how far apart the three Alligator lines are, as two histograms. The upper bars are the gap between Jaw and Teeth, the lower bars the gap between Teeth and Lips drawn below zero. Both growing is the alligator eating, a trend under way; both shrinking toward zero is it falling asleep, the lines converging in a range. One side growing while the other shrinks is it waking or sated, the turn from one state into the other.

It is read under the Alligator ([`ALLIGATOR`](/functions/alligator.md)), the Profitunity leg it measures.

## Chart

<ta-chart src="/functions/gator.svg" ratio="720:416" label="GATOR over IBM, daily, 1999" alt="Drawn on the price pane over candles; below it, a separate indicator pane with a zero line showing outUpper as a sign-coloured histogram moved 5 bars forward and outLower as a sign-coloured histogram moved 3 bars forward."></ta-chart>

::: details Chart metadata <span class="flag-tip" tabindex="0" role="note" aria-label="The chart is drawn from the function's metadata: the flags and attributes the abstract API reports for it and for each output, and the input series its reference run declares." data-tip="The chart is drawn from the function's metadata: the flags and attributes the abstract API reports for it and for each output, and the input series its reference run declares.">i</span>

Drawn on the price pane over candles; below it, a separate indicator pane with a zero line showing outUpper as a sign-coloured histogram moved 5 bars forward and outLower as a sign-coloured histogram moved 3 bars forward.

Metadata flags used: `inPriceHL: high, low` · `outUpper: histogram, display_shift (+5)` · `outLower: histogram, display_shift (+3)`

:::

[Reference data (CSV)](/functions/gator.csv) · [Chart (SVG)](/functions/gator.svg)

## Formula

Jaw, Teeth and Lips are the three [`ALLIGATOR`](/functions/alligator.md) lines, each drawn its own shift ahead. The histograms compare them where a chart draws them:

Upper = | Jaw − Teeth |  
Lower = −| Teeth − Lips |

## Notes

- Each line is drawn ahead by its own shift, so the line drawn further ahead is read back by the difference of the two shifts: at the default shifts Upper_t = | Jaw_t−3 − Teeth_t | and Lower_t = −| Teeth_t−2 − Lips_t |. `TA_GATOR_DisplayShift` reports where a chart draws each histogram, the smaller of its two lines' shifts; it changes nothing about the values or the returned range.
- Each histogram computes its own lines, each seeded over the bars its own lookback reaches back, so the Teeth an output reads can differ from the `TA_ALLIGATOR` Teeth by the seed until it has decayed; setting the `RMA` unstable period closes the gap.
- A zero Lower is `+0.0`.

## Inputs

- `inHigh` — High price of each bar
- `inLow` — Low price of each bar

## Outputs

- `outUpper` — Gap between the Jaw and the Teeth, zero or above
- `outLower` — Gap between the Teeth and the Lips, zero or below

## Parameters

| Parameter | Type | Default | Accepted values | Description |
| --- | --- | --- | --- | --- |
| `optInJawPeriod` | integer | 13 | 1–100000 | Number of bars in the jaw's smoothed moving average. |
| `optInJawShift` | integer | 8 | 0–100000 | Bars ahead of the computing bar at which a chart draws the jaw. |
| `optInTeethPeriod` | integer | 8 | 1–100000 | Number of bars in the teeth's smoothed moving average. |
| `optInTeethShift` | integer | 5 | 0–100000 | Bars ahead of the computing bar at which a chart draws the teeth. |
| `optInLipsPeriod` | integer | 5 | 1–100000 | Number of bars in the lips' smoothed moving average. |
| `optInLipsShift` | integer | 3 | 0–100000 | Bars ahead of the computing bar at which a chart draws the lips. |

## Properties

**Numerical Stability:** [Initial Unstable Period](/functions/stability.md#initial-unstable-period) — Inherited from RMA, which GATOR computes internally; tunable via RMA's unstable period.

<div class="flag-table">

|  |
| :-- |
| <span class="flag-box">☐</span> <span style="opacity:0.5">Overlap Input</span> |
| <span class="flag-box">✅</span> **Independent Y-Axis** <span class="flag-tip" tabindex="0" role="note" aria-label="Output is on its own scale, drawn in a separate pane below the price chart." data-tip="Output is on its own scale, drawn in a separate pane below the price chart.">i</span> |
| <span class="flag-box">☐</span> <span style="opacity:0.5">Candlestick</span> |
| <span class="flag-box">☐</span> <span style="opacity:0.5">Can Output NaN or ±Inf</span> |
| <span class="flag-box">☐</span> <span style="opacity:0.5">Identity at Period 1</span> |
| <span class="flag-box">✅</span> **Display Shift** <span class="flag-tip" tabindex="0" role="note" aria-label="A chart draws at least one output ahead of or behind the bar that computed it. The display-shift query gives the number of bars; the values are not shifted." data-tip="A chart draws at least one output ahead of or behind the bar that computed it. The display-shift query gives the number of bars; the values are not shifted.">i</span> |
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
double upper[252], lower[252];
int outBegIdx, outNBElement;

TA_Initialize();
TA_GATOR( 0, n - 1, high, low, 13, 8, 8, 5, 5, 3, &outBegIdx, &outNBElement, upper, lower );
printf( "%d values from bar %d; last upper = %f\n", outNBElement, outBegIdx, upper[outNBElement - 1] );
```

@tab Rust

```rust
use ta_lib::Core;

let high = vec![0.0; 252]; // your data, oldest first
let low = vec![0.0; 252]; // your data, oldest first
let mut upper = vec![0.0; 252];
let mut lower = vec![0.0; 252];

let core = Core::new();
let range = core.gator(0, high.len() - 1, &high, &low, 13, 8, 8, 5, 5, 3,
                       &mut upper, &mut lower)?;
println!("{} values from bar {}; last upper = {}", range.count, range.beg_idx, upper[range.count - 1]);
```

@tab Java

```java
import io.github.talib.Core;
import io.github.talib.OutRange;

double[] high = new double[252]; // your data, oldest first
double[] low = new double[252]; // your data, oldest first
double[] upper = new double[252];
double[] lower = new double[252];

Core core = Core.DEFAULT;
OutRange r = core.gator(0, high.length - 1, high, low, 13, 8, 8, 5, 5, 3, upper, lower);
System.out.println(r.count() + " values from bar " + r.begIdx() + "; last upper = " + upper[r.count() - 1]);
```

@tab C\#

```csharp
using System;
using TALib;

var high = new double[252]; // your data, oldest first
var low = new double[252]; // your data, oldest first
var upper = new double[252];
var lower = new double[252];

var core = Core.Default;
OutRange r = core.Gator(0, high.Length - 1, high, low, 13, 8, 8, 5, 5, 3, upper, lower);
Console.WriteLine($"{r.Count} values from bar {r.BegIdx}; last upper = {upper[r.Count - 1]}");
```

@tab TypeScript

```ts
import { gator } from 'ta-lib';

const high = new Float64Array(252); // your data, oldest first
const low = new Float64Array(252); // your data, oldest first

const { begIdx, outUpper, outLower } = gator({ high, low }, {
  jawPeriod: 13,
  jawShift: 8,
  teethPeriod: 8,
  teethShift: 5,
  lipsPeriod: 5,
  lipsShift: 3,
});
console.log(outUpper.length + ' values from bar ' + begIdx + '; last outUpper = ' + outUpper[outUpper.length - 1]);
```

:::

## Implementation

Added in 0.8.4.

TA-Lib Definition: [`gator.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/gator/gator.c) · [`gator.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/gator/gator.yaml)

| Native | File |
|--------|------|
| C | [`ta_GATOR.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_GATOR.c) |
| Rust | [`gator.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/library/src/ta_func/gator.rs) |
| Java | [`Core_GATOR.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/fragments/Core_GATOR.java) |
| C# | [`Core_GATOR.cs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/csharp/library/src/Core_GATOR.cs) |
| TypeScript | [`gator.ts`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/ts/library/src/func/gator.ts) |

Python: `talib.GATOR`, see [ta-lib-python](https://ta-lib.github.io/ta-lib-python/func_groups/momentum_indicators.html).

TA-Lib is also available for R and more using a [wrapper](/install/#wrappers).

## Aliases

gator oscillator, williams gator

## See Also

[ALLIGATOR](/functions/alligator.md) · [AO](/functions/ao.md) · [AC](/functions/ac.md) · [RMA](/functions/rma.md)

## References

- Bill Williams, *New Trading Dimensions*, 1998
