---
title: "Fisher Transform (FISHER)"
description: "Ehlers' Fisher Transform: an oscillator that reshapes the midpoint's position in its rolling channel into a near-normal distribution."
---

## Summary

Ehlers' Fisher Transform: an oscillator that reshapes the midpoint's position in its rolling channel into a near-normal distribution. Extreme values become rare and turning points sharp, where a plain channel position spends much of its time pinned near the edges.

The output is unbounded and centred on zero. A peak or trough marks a likely turn, and the Fisher line crossing its Trigger, the same line one bar later, is the author's entry signal.

## Chart

<ta-chart src="/functions/fisher.svg" ratio="720:416" label="FISHER over IBM, daily, 1999" alt="Drawn on the price pane over candles; below it, a separate indicator pane with a zero line showing outFisher as a solid line and outTrigger as a dashed line."></ta-chart>

::: details Chart metadata <span class="flag-tip" tabindex="0" role="note" aria-label="The chart is drawn from the function's metadata alone: the flags the abstract API reports for it and for each output." data-tip="The chart is drawn from the function's metadata alone: the flags the abstract API reports for it and for each output.">i</span>

Drawn on the price pane over candles; below it, a separate indicator pane with a zero line showing outFisher as a solid line and outTrigger as a dashed line.

Metadata flags used: `inPriceHL: high, low` · `outFisher: line` · `outTrigger: dash_line`

:::

[Reference data (CSV)](/functions/fisher.csv) · [Chart (SVG)](/functions/fisher.svg)

## Formula

With `P = (high + low) / 2` and `MaxP`, `MinP` its highest and lowest value over the last `optInTimePeriod` bars:

    r       = ( P - MinP ) / ( MaxP - MinP ), or 0.5 when MaxP = MinP
    v       = 0.66 * ( r - 0.5 ) + 0.67 * v[prev]
    v       = 0.999 if v > 0.99, -0.999 if v < -0.99
    Fisher  = 0.5 * ln( ( 1 + v ) / ( 1 - v ) ) + 0.5 * Fisher[prev]
    Trigger = Fisher[prev]

`v` and `Fisher` start from 0, so the first Trigger is 0, and the limited `v` is the one the next bar reads.

## Notes

- A window whose midpoints are all equal takes the neutral position, so a market that does not move decays toward 0. The original divides by the zero range there.

## Inputs

- `inHigh` — High price of each bar
- `inLow` — Low price of each bar

## Outputs

- `outFisher` — Fisher Transform value
- `outTrigger` — Fisher value of the previous bar

## Parameters

| Parameter | Type | Default | Accepted values | Description |
| --- | --- | --- | --- | --- |
| `optInTimePeriod` | integer | 10 | 2–100000 | Number of bars in the channel the midpoint is located in |

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
double high[252]; /* your data, oldest first */
double low[252]; /* your data, oldest first */
double fisher[252], trigger[252];
int outBegIdx, outNBElement;

TA_Initialize();
TA_FISHER( 0, n - 1, high, low, 10, &outBegIdx, &outNBElement, fisher, trigger );
printf( "%d values from bar %d; last fisher = %f\n", outNBElement, outBegIdx, fisher[outNBElement - 1] );
```

@tab Rust

```rust
use ta_lib::Core;

let high = vec![0.0; 252]; // your data, oldest first
let low = vec![0.0; 252]; // your data, oldest first
let mut fisher = vec![0.0; 252];
let mut trigger = vec![0.0; 252];

let core = Core::new();
let range = core.fisher(0, high.len() - 1, &high, &low, 10, &mut fisher, &mut trigger)?;
println!("{} values from bar {}; last fisher = {}", range.count, range.beg_idx, fisher[range.count - 1]);
```

@tab Java

```java
import io.github.talib.Core;
import io.github.talib.OutRange;

double[] high = new double[252]; // your data, oldest first
double[] low = new double[252]; // your data, oldest first
double[] fisher = new double[252];
double[] trigger = new double[252];

Core core = Core.DEFAULT;
OutRange r = core.fisher(0, high.length - 1, high, low, 10, fisher, trigger);
System.out.println(r.count() + " values from bar " + r.begIdx() + "; last fisher = " + fisher[r.count() - 1]);
```

@tab C\#

```csharp
using System;
using TALib;

var high = new double[252]; // your data, oldest first
var low = new double[252]; // your data, oldest first
var fisher = new double[252];
var trigger = new double[252];

var core = Core.Default;
OutRange r = core.Fisher(0, high.Length - 1, high, low, 10, fisher, trigger);
Console.WriteLine($"{r.Count} values from bar {r.BegIdx}; last fisher = {fisher[r.Count - 1]}");
```

@tab TypeScript

```ts
import { fisher } from 'ta-lib';

const high = new Float64Array(252); // your data, oldest first
const low = new Float64Array(252); // your data, oldest first

const { begIdx, outFisher, outTrigger } = fisher({ high, low }, {
  timePeriod: 10,
});
console.log(outFisher.length + ' values from bar ' + begIdx + '; last outFisher = ' + outFisher[outFisher.length - 1]);
```

:::

## Implementation

Added in 0.8.3 (ta-lib-python 0.8.3).

TA-Lib Definition: [`fisher.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/fisher/fisher.c) · [`fisher.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/fisher/fisher.yaml)

| Native | File |
|--------|------|
| C | [`ta_FISHER.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_FISHER.c) |
| Rust | [`fisher.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/library/src/ta_func/fisher.rs) |
| Java | [`Core_FISHER.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/fragments/Core_FISHER.java) |
| C# | [`Core_FISHER.cs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/csharp/library/src/Core_FISHER.cs) |
| TypeScript | [`fisher.ts`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/ts/library/src/func/fisher.ts) |

Python: `talib.FISHER` (ta-lib-python 0.8.3 or later), see [ta-lib-python](https://ta-lib.github.io/ta-lib-python/func_groups/momentum_indicators.html).

TA-Lib is also available for R and more using a [wrapper](/install/#wrappers).

## Aliases

Fisher Transform, Ehlers Fisher Transform

## See Also

[STOCHF](/functions/stochf.md) · [WILLR](/functions/willr.md) · [MIDPRICE](/functions/midprice.md) · [IBS](/functions/ibs.md)

## References

- John F. Ehlers, "Using The Fisher Transform", *Technical Analysis of Stocks & Commodities*, V.20:11 (November 2002), 40-42
- John F. Ehlers, *Cybernetic Analysis for Stocks and Futures*, Wiley, 2004
