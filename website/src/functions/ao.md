---
title: "Awesome Oscillator (AO)"
description: "Bill Williams' Awesome Oscillator (New Trading Dimensions, 1998): market momentum read as the spread between a short and a long simple moving average of…"
---

## Summary

Bill Williams' Awesome Oscillator (*New Trading Dimensions*, 1998): market momentum read as the spread between a short and a long simple moving average of the median price. It contrasts what the recent bars have done against a longer stretch of the same market, using the bar midpoint rather than the close so that intrabar range, not the settle, drives the reading.

Above zero the short window sits higher than the long one and momentum is with the bulls; below zero it is with the bears. It is drawn as a zero-centred histogram, and the readings that get traded are the zero-line crossings, the twin-peaks divergence, and the run of consecutive same-side bars — which is why the sign and the bar-to-bar change matter more than the level.

The oscillator is the first leg of Williams' Profitunity system, alongside the Alligator and the Accelerator/Decelerator ([`AC`](/functions/ac.md)).

## Chart

<ta-chart src="/functions/ao.svg" ratio="720:416" label="AO over IBM, daily, 1999" alt="Drawn on the price pane over candles; below it, a separate indicator pane with a zero line showing outReal as a sign-coloured histogram."></ta-chart>

::: details Chart metadata <span class="flag-tip" tabindex="0" role="note" aria-label="The chart is drawn automatically from metadata TA-Lib provides. Your app can get the same through the Abstract API." data-tip="The chart is drawn automatically from metadata TA-Lib provides. Your app can get the same through the Abstract API.">i</span>

Drawn on the price pane over candles; below it, a separate indicator pane with a zero line showing outReal as a sign-coloured histogram.

Metadata flags used: `inPriceHL: high, low` · `outReal: histogram, positive, negative`

:::

[Reference data (CSV)](/functions/ao.csv) · [Chart (SVG)](/functions/ao.svg)

## Formula

median_t = ( high_t + low_t ) / 2  
AO_t = SMA(median, fast)_t − SMA(median, slow)_t

An inverted pair is not swapped: passing a fast period longer than the slow one is well defined and simply yields −AO.

## Inputs

- `inHigh` — High price of each bar
- `inLow` — Low price of each bar

## Outputs

- `outReal` — Spread between the two moving averages, centred on zero

## Parameters

| Parameter | Type | Default | Accepted values | Description |
| --- | --- | --- | --- | --- |
| `optInFastPeriod` | integer | 5 | 2–100000 | Number of bars in the short moving average. |
| `optInSlowPeriod` | integer | 34 | 2–100000 | Number of bars in the long moving average. |

## Properties

**Numerical Stability:** [Start-Independent](/functions/stability.md#start-independent)

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
double out[252];
int outBegIdx, outNBElement;

TA_Initialize();
TA_AO( 0, n - 1, high, low, 5, 34, &outBegIdx, &outNBElement, out );
printf( "%d values from bar %d; last out = %f\n", outNBElement, outBegIdx, out[outNBElement - 1] );
```

@tab Rust

```rust
use ta_lib::Core;

let high = vec![0.0; 252]; // your data, oldest first
let low = vec![0.0; 252]; // your data, oldest first
let mut out = vec![0.0; 252];

let core = Core::new();
let range = core.ao(0, high.len() - 1, &high, &low, 5, 34, &mut out)?;
println!("{} values from bar {}; last out = {}", range.count, range.beg_idx, out[range.count - 1]);
```

@tab Java

```java
import io.github.talib.Core;
import io.github.talib.OutRange;

double[] high = new double[252]; // your data, oldest first
double[] low = new double[252]; // your data, oldest first
double[] out = new double[252];

Core core = Core.DEFAULT;
OutRange r = core.ao(0, high.length - 1, high, low, 5, 34, out);
System.out.println(r.count() + " values from bar " + r.begIdx() + "; last out = " + out[r.count() - 1]);
```

@tab C\#

```csharp
using System;
using TALib;

var high = new double[252]; // your data, oldest first
var low = new double[252]; // your data, oldest first
var outReal = new double[252];

var core = Core.Default;
OutRange r = core.Ao(0, high.Length - 1, high, low, 5, 34, outReal);
Console.WriteLine($"{r.Count} values from bar {r.BegIdx}; last outReal = {outReal[r.Count - 1]}");
```

@tab TypeScript

```ts
import { ao } from 'ta-lib';

const high = new Float64Array(252); // your data, oldest first
const low = new Float64Array(252); // your data, oldest first

const { begIdx, values } = ao({ high, low }, { fastPeriod: 5, slowPeriod: 34 });
console.log(values.length + ' values from bar ' + begIdx + '; last values = ' + values[values.length - 1]);
```

:::

## Implementation

Added in 0.8.1 (ta-lib-python 0.8.1).

TA-Lib Definition: [`ao.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/ao/ao.c) · [`ao.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/ao/ao.yaml)

| Native | File |
|--------|------|
| C | [`ta_AO.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_AO.c) |
| Rust | [`ao.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/library/src/ta_func/ao.rs) |
| Java | [`Core_AO.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/fragments/Core_AO.java) |
| C# | [`Core_AO.cs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/csharp/library/src/Core_AO.cs) |
| TypeScript | [`ao.ts`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/ts/library/src/func/ao.ts) |

Python: `talib.AO` (ta-lib-python 0.8.1 or later), see [ta-lib-python](https://ta-lib.github.io/ta-lib-python/func_groups/momentum_indicators.html).

TA-Lib is also available for R and more using a [wrapper](/install/#wrappers).

## Aliases

Awesome Oscillator, Bill Williams Awesome Oscillator, BW AO

## See Also

[APO](/functions/apo.md) · [MACD](/functions/macd.md) · [MEDPRICE](/functions/medprice.md) · [PPO](/functions/ppo.md) · [ULTOSC](/functions/ultosc.md)

## References

- Bill Williams, *New Trading Dimensions*, Wiley, 1998, and *Trading Chaos*, define the Awesome Oscillator as the 5-period less the 34-period simple moving average of the median price.
- Tulip Indicators `ti_ao` and pandas-ta-classic `ao` compute the same form on the same inputs, and both report the first value at the same bar.
- pandas-ta-classic swaps an inverted pair before computing, so it answers AO(slow, fast) where this answers its negation.
