---
url: 'https://ta-lib.org/functions/donchian.md'
description: 'Donchian Channels: three overlap lines built from rolling price extrema.'
---
# Donchian Channels (DONCHIAN)

## Summary

Donchian Channels: three overlap lines built from rolling price extrema. The upper band is the highest high and the lower band the lowest low over the period; the middle line is their midpoint. Richard Donchian's original four-week rule — generally credited as the first published systematic trend-following system — buys a break above the high of the preceding weeks and sells a break below their low.

## Chart

<ta-chart src="/functions/donchian.svg" ratio="720:274" label="DONCHIAN over IBM, daily, 1999" alt="Drawn on the price pane over candles, with outRealMiddleBand as a solid line and a band between outRealUpperBand and outRealLowerBand with a soft fill."></ta-chart>

<div class="ta-chart-links">

[Reference data (CSV)](/functions/donchian.csv) · [Chart (SVG)](/functions/donchian.svg)

</div>

::: details Chart metadata <span class="flag-tip" tabindex="0" role="note" aria-label="The chart is drawn automatically from metadata TA-Lib provides. Your app can get the same through the Abstract API." data-tip="The chart is drawn automatically from metadata TA-Lib provides. Your app can get the same through the Abstract API.">i</span>

**Outputs**

* `outRealUpperBand`: `upper_limit`
* `outRealMiddleBand`: `line`
* `outRealLowerBand`: `lower_limit`

:::

## Formula

Window = the optInTimePeriod bars ending at the current bar

Upper  = Highest High of Window
Lower  = Lowest  Low  of Window
Middle = (Upper + Lower) / 2

## Notes

* The window includes the current bar, matching TradingView (`ta.highest`/`ta.lowest`), NinjaTrader, ta4j, pandas-ta and every other library that ships Donchian Channels.
* A breakout rule compares the current bar against the **previous** bar's band — `High[t] > Upper[t-1]` — which is where the one-bar offset belongs. Reading `Upper[t]` against `High[t]` can never signal, because `High[t]` is inside the window that produced it.
* Upper, Middle and Lower are bit-identical to `MAX(high, N)`, `MIDPRICE(N)` and `MIN(low, N)`. DONCHIAN computes all three in one pass under the name users look for.
* The middle line is the channel midpoint, not a moving average of price.
* No smoothing or recursion is involved, so there is no unstable period: outputs are exact from the first bar.

## Inputs

* `inHigh` — High price of each bar
* `inLow` — Low price of each bar

## Outputs

* `outRealUpperBand` — Highest high of the window
* `outRealMiddleBand` — Midpoint of the upper and lower bands
* `outRealLowerBand` — Lowest low of the window

## Parameters

| Parameter | Type | Default | Accepted values | Description |
| --- | --- | --- | --- | --- |
| `optInTimePeriod` | integer | 20 | 2–100000 | Number of bars in the extrema window |

## Properties

**Numerical Stability:** [Start-Independent](/functions/stability.md#start-independent)

<div class="flag-cols">

<div class="flag-table">

|  |
| :-- |
| <span class="flag-box">✅</span> **Overlap Input** <span class="flag-tip" tabindex="0" role="note" aria-label="Output is on the same scale as the input price, so it is drawn over the price chart." data-tip="Output is on the same scale as the input price, so it is drawn over the price chart.">i</span> |
| <span class="flag-box">☐</span> <span style="opacity:0.5">Independent Y-Axis</span> |
| <span class="flag-box">☐</span> <span style="opacity:0.5">Candlestick</span> |
| <span class="flag-box">☐</span> <span style="opacity:0.5">Display Shift</span> |

</div>

<div class="flag-table">

|  |
| :-- |
| <span class="flag-box">☐</span> <span style="opacity:0.5">Can Output NaN or ±Inf</span> |
| <span class="flag-box">☐</span> <span style="opacity:0.5">Identity at Period 1</span> |
| <span class="flag-box">☐</span> <span style="opacity:0.5">Uses Transcendental</span> |
| <span class="flag-box">☐</span> <span style="opacity:0.5">Input Must Be Positive</span> |

</div>

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
double upperBand[252], middleBand[252], lowerBand[252];
int outBegIdx, outNBElement;

TA_Initialize();
TA_DONCHIAN( 0, n - 1, high, low, 20, &outBegIdx, &outNBElement, upperBand, middleBand, lowerBand );
printf( "%d values from bar %d; last upperBand = %f\n", outNBElement, outBegIdx, upperBand[outNBElement - 1] );
```

@tab Rust

```rust
use ta_lib::Core;

let high = vec![0.0; 252]; // your data, oldest first
let low = vec![0.0; 252]; // your data, oldest first
let mut upper_band = vec![0.0; 252];
let mut middle_band = vec![0.0; 252];
let mut lower_band = vec![0.0; 252];

let core = Core::new();
let range = core.donchian(0, high.len() - 1, &high, &low, 20,
                          &mut upper_band, &mut middle_band, &mut lower_band)?;
println!("{} values from bar {}; last upper_band = {}", range.count, range.beg_idx, upper_band[range.count - 1]);
```

@tab Java

```java
import io.github.talib.Core;
import io.github.talib.OutRange;

double[] high = new double[252]; // your data, oldest first
double[] low = new double[252]; // your data, oldest first
double[] upperBand = new double[252];
double[] middleBand = new double[252];
double[] lowerBand = new double[252];

Core core = Core.DEFAULT;
OutRange r = core.donchian(0, high.length - 1, high, low, 20, upperBand, middleBand, lowerBand);
System.out.println(r.count() + " values from bar " + r.begIdx() + "; last upperBand = " + upperBand[r.count() - 1]);
```

@tab C#

```csharp
using System;
using TALib;

var high = new double[252]; // your data, oldest first
var low = new double[252]; // your data, oldest first
var upperBand = new double[252];
var middleBand = new double[252];
var lowerBand = new double[252];

var core = Core.Default;
OutRange r = core.Donchian(0, high.Length - 1, high, low, 20, upperBand, middleBand, lowerBand);
Console.WriteLine($"{r.Count} values from bar {r.BegIdx}; last upperBand = {upperBand[r.Count - 1]}");
```

@tab TypeScript

```ts
import { donchian } from 'ta-lib';

const high = new Float64Array(252); // your data, oldest first
const low = new Float64Array(252); // your data, oldest first

const {
  begIdx,
  outUpperBand,
  outMiddleBand,
  outLowerBand,
} = donchian({ high, low }, { timePeriod: 20 });
console.log(outUpperBand.length + ' values from bar ' + begIdx + '; last outUpperBand = ' + outUpperBand[outUpperBand.length - 1]);
```

:::

## Implementation

Added in 0.8.1 (ta-lib-python 0.8.1).

TA-Lib Definition: [`donchian.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/donchian/donchian.c) · [`donchian.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/donchian/donchian.yaml)

| Native | File |
|--------|------|
| C | [`ta_DONCHIAN.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_DONCHIAN.c) |
| Rust | [`donchian.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/library/src/ta_func/donchian.rs) |
| Java | [`Core_DONCHIAN.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/fragments/Core_DONCHIAN.java) |
| C# | [`Core_DONCHIAN.cs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/csharp/library/src/Core_DONCHIAN.cs) |
| TypeScript | [`donchian.ts`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/ts/library/src/func/donchian.ts) |

Python: `talib.DONCHIAN` (ta-lib-python 0.8.1 or later), see [ta-lib-python](https://ta-lib.github.io/ta-lib-python/func_groups/overlap_studies.html).

TA-Lib is also available for R and more using a [wrapper](/install/#wrappers).
