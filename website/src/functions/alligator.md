---
title: "Williams Alligator (ALLIGATOR)"
description: "Bill Williams' Alligator (Trading Chaos, 1995): three smoothed moving averages of the median price."
---

## Summary

Bill Williams' Alligator (*Trading Chaos*, 1995): three smoothed moving averages of the median price. The Jaw, Teeth and Lips are each drawn a few bars ahead of the bar that computed it. When the three lines are intertwined the alligator sleeps and the market has no trend; when they fan out in order, Lips above Teeth above Jaw or the reverse, it is feeding and a trend is under way. Price crossing back into the lines is read as the trend tiring.

The Alligator is a leg of Williams' Profitunity system, alongside the Awesome Oscillator ([`AO`](/functions/ao.md)) and the Accelerator/Decelerator ([`AC`](/functions/ac.md)).

## Chart

<ta-chart src="/functions/alligator.svg" ratio="720:274" label="ALLIGATOR over IBM, daily, 1999" alt="Drawn on the price pane over candles, with outJaw as a solid line moved 8 bars forward, outTeeth as a solid line moved 5 bars forward and outLips as a solid line moved 3 bars forward."></ta-chart>

Drawn on the price pane over candles, with outJaw as a solid line moved 8 bars forward, outTeeth as a solid line moved 5 bars forward and outLips as a solid line moved 3 bars forward. [Reference data (CSV)](/functions/alligator.csv) · [Chart (SVG)](/functions/alligator.svg)

## Formula

median_t = ( high_t + low_t ) / 2  
Jaw_t = SMMA(median, jawPeriod)_t  
Teeth_t = SMMA(median, teethPeriod)_t  
Lips_t = SMMA(median, lipsPeriod)_t

SMMA is Wilder's smoothed moving average ([`RMA`](/functions/rma.md)): seeded with the simple average of its first `n` values, then SMMA_t = SMMA_t−1 + ( x_t − SMMA_t−1 ) / n.

## Notes

- Each line is `TA_RMA` of `TA_MEDPRICE` over its own period. Williams draws the lines ahead of price; that is a display shift: `TA_ALLIGATOR_DisplayShift` reports each line's shift, and it changes nothing about the values, the lookback or the returned range. Every output is written at the bar that computed it, so the last `shift` values of a line are the part a chart draws past the last bar.
- All three lines start at the bar of the longest period, and each shorter line seeds over the bars its own lookback reaches back from there, not at the first bar of the series. A chart that seeds every line at the first bar draws different short lines until the seed has decayed; setting the `RMA` unstable period closes the gap.

## Inputs

- `inHigh` — High price of each bar
- `inLow` — Low price of each bar

## Outputs

- `outJaw` — Jaw: the slowest line, drawn ahead by the jaw shift
- `outTeeth` — Teeth: the middle line, drawn ahead by the teeth shift
- `outLips` — Lips: the fastest line, drawn ahead by the lips shift

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

**Numerical Stability:** [Initial Unstable Period](/functions/stability.md#initial-unstable-period) — Inherited from RMA, which ALLIGATOR computes internally; tunable via RMA's unstable period.

<div class="flag-table">

|  |
| :-- |
| <span class="flag-box">✅</span> **Overlap Input** <span class="flag-tip" tabindex="0" role="note" aria-label="Output is on the same scale as the input price, so it is drawn over the price chart." data-tip="Output is on the same scale as the input price, so it is drawn over the price chart.">i</span> |
| <span class="flag-box">☐</span> <span style="opacity:0.5">Independent Y-Axis</span> |
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
double jaw[252], teeth[252], lips[252];
int outBegIdx, outNBElement;

TA_Initialize();
TA_ALLIGATOR( 0, n - 1, high, low, 13, 8, 8, 5, 5, 3, &outBegIdx, &outNBElement, jaw, teeth, lips );
printf( "%d values from bar %d; last jaw = %f\n", outNBElement, outBegIdx, jaw[outNBElement - 1] );
```

@tab Rust

```rust
use ta_lib::Core;

let high = vec![0.0; 252]; // your data, oldest first
let low = vec![0.0; 252]; // your data, oldest first
let mut jaw = vec![0.0; 252];
let mut teeth = vec![0.0; 252];
let mut lips = vec![0.0; 252];

let core = Core::new();
let range = core.alligator(0, high.len() - 1, &high, &low, 13, 8, 8, 5, 5, 3,
                           &mut jaw, &mut teeth, &mut lips)?;
println!("{} values from bar {}; last jaw = {}", range.count, range.beg_idx, jaw[range.count - 1]);
```

@tab Java

```java
import io.github.talib.Core;
import io.github.talib.OutRange;

double[] high = new double[252]; // your data, oldest first
double[] low = new double[252]; // your data, oldest first
double[] jaw = new double[252];
double[] teeth = new double[252];
double[] lips = new double[252];

Core core = Core.DEFAULT;
OutRange r = core.alligator(0, high.length - 1, high, low, 13, 8, 8, 5, 5, 3, jaw, teeth, lips);
System.out.println(r.count() + " values from bar " + r.begIdx() + "; last jaw = " + jaw[r.count() - 1]);
```

@tab C\#

```csharp
using System;
using TALib;

var high = new double[252]; // your data, oldest first
var low = new double[252]; // your data, oldest first
var jaw = new double[252];
var teeth = new double[252];
var lips = new double[252];

var core = Core.Default;
OutRange r = core.Alligator(0, high.Length - 1, high, low, 13, 8, 8, 5, 5, 3, jaw, teeth, lips);
Console.WriteLine($"{r.Count} values from bar {r.BegIdx}; last jaw = {jaw[r.Count - 1]}");
```

@tab TypeScript

```ts
import { alligator } from 'ta-lib';

const high = new Float64Array(252); // your data, oldest first
const low = new Float64Array(252); // your data, oldest first

const { begIdx, outJaw, outTeeth, outLips } = alligator({ high, low }, {
  jawPeriod: 13,
  jawShift: 8,
  teethPeriod: 8,
  teethShift: 5,
  lipsPeriod: 5,
  lipsShift: 3,
});
console.log(outJaw.length + ' values from bar ' + begIdx + '; last outJaw = ' + outJaw[outJaw.length - 1]);
```

:::

## Implementation

Added in 0.8.4.

TA-Lib Definition: [`alligator.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/alligator/alligator.c) · [`alligator.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/alligator/alligator.yaml)

| Native | File |
|--------|------|
| C | [`ta_ALLIGATOR.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_ALLIGATOR.c) |
| Rust | [`alligator.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/library/src/ta_func/alligator.rs) |
| Java | [`Core_ALLIGATOR.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/fragments/Core_ALLIGATOR.java) |
| C# | [`Core_ALLIGATOR.cs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/csharp/library/src/Core_ALLIGATOR.cs) |
| TypeScript | [`alligator.ts`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/ts/library/src/func/alligator.ts) |

Python: `talib.ALLIGATOR` (ta-lib-python 0.8.4 or later), see [ta-lib-python](https://ta-lib.github.io/ta-lib-python/func_groups/overlap_studies.html).

TA-Lib is also available for R and more using a [wrapper](/install/#wrappers).

## Aliases

williams alligator, bill williams alligator, jaw teeth lips

## See Also

[RMA](/functions/rma.md) · [MEDPRICE](/functions/medprice.md) · [AO](/functions/ao.md) · [AC](/functions/ac.md) · [ICHIMOKU](/functions/ichimoku.md)

## References

- Bill Williams, *Trading Chaos*, 1995
- Bill Williams, *New Trading Dimensions*, 1998
- [MetaTrader 5 — Alligator](https://www.metatrader5.com/en/terminal/help/indicators/bw_indicators/alligator)
