---
url: 'https://ta-lib.org/functions/stoch.md'
description: >-
  Slow Stochastic oscillator: locates the close within the high-low range over a
  lookback period, then double-smooths it.
---
# Stochastic (STOCH)

## Summary

Slow Stochastic oscillator: locates the close within the high-low range over a lookback period, then double-smooths it. Returns the Slow-%K and Slow-%D lines. SlowK/SlowD > 80 overbought, < 20 oversold; %K crossing %D signals momentum shifts.

## Chart

<ta-chart src="/functions/stoch.svg" ratio="720:416" label="STOCH over IBM, daily, 1999" alt="Drawn on the price pane over candles; below it, a separate indicator pane with a zero line showing outSlowK as a solid line with dashed rules at 20 and 80 and outSlowD as a dashed line."></ta-chart>

::: details Chart metadata <span class="flag-tip" tabindex="0" role="note" aria-label="The chart is drawn from the function's metadata: the flags and attributes the abstract API reports for it and for each output, and the input series its reference run declares." data-tip="The chart is drawn from the function's metadata: the flags and attributes the abstract API reports for it and for each output, and the input series its reference run declares.">i</span>

Drawn on the price pane over candles; below it, a separate indicator pane with a zero line showing outSlowK as a solid line with dashed rules at 20 and 80 and outSlowD as a dashed line.

Metadata flags used: `inPriceHLC: high, low, close` · `outSlowK: line, positive, negative, levels 20, 80` · `outSlowD: dash_line, positive, negative`

:::

[Reference data (CSV)](/functions/stoch.csv) · [Chart (SVG)](/functions/stoch.svg)

## Formula

FastK = 100\*(Close - LL_n)/(HH_n - LL_n), n = FastK_Period (LL/HH = lowest low / highest high over n)
SlowK = MA(FastK, SlowK_Period, SlowK_MAType)
SlowD = MA(SlowK, SlowD_Period, SlowD_MAType)

## Notes

* When the high-low range over the window is zero, the raw stochastic is set to 0 instead of being undefined.

## Inputs

* `inHigh` — High price of each bar
* `inLow` — Low price of each bar
* `inClose` — Close price of each bar

## Outputs

* `outSlowK` — Raw FastK smoothed by SlowK_Period MA
* `outSlowD` — Signal line: SlowK smoothed by SlowD_Period MA

## Parameters

| Parameter | Type | Default | Accepted values | Description |
| --- | --- | --- | --- | --- |
| `optInFastK_Period` | integer | 5 | 1–100000 | Lookback window for the raw %K high-low range |
| `optInSlowK_Period` | integer | 3 | 1–100000 | Smoothing period turning FastK into SlowK |
| `optInSlowK_MAType` | MAType | SMA (0) | any MAType | MA type used to smooth into SlowK |
| `optInSlowD_Period` | integer | 3 | 1–100000 | Smoothing period for the SlowD signal line |
| `optInSlowD_MAType` | MAType | SMA (0) | any MAType | MA type used for the SlowD line |

*`MAType` values: 0 SMA · 1 EMA · 2 WMA · 3 DEMA · 4 TEMA · 5 TRIMA · 6 KAMA · 7 MAMA · 8 T3 · 9 HMA · 10 DISABLED · 11 DEFAULT · 12 ZLEMA · 13 RMA · 14 VIDYA · 15 ALMA*

## Properties

**Numerical Stability:** [Depends on MA Type](/functions/stability.md#depends-on-ma-type) — This function's default, SMA, is start-independent.

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
double slowK[252], slowD[252];
int outBegIdx, outNBElement;

TA_Initialize();
TA_STOCH( 0, n - 1, high, low, close, 5, 3, TA_MAType_SMA, 3, TA_MAType_SMA,
          &outBegIdx, &outNBElement, slowK, slowD );
printf( "%d values from bar %d; last slowK = %f\n", outNBElement, outBegIdx, slowK[outNBElement - 1] );
```

@tab Rust

```rust
use ta_lib::{Core, MAType};

let high = vec![0.0; 252]; // your data, oldest first
let low = vec![0.0; 252]; // your data, oldest first
let close = vec![0.0; 252]; // your data, oldest first
let mut slow_k = vec![0.0; 252];
let mut slow_d = vec![0.0; 252];

let core = Core::new();
let range = core.stoch(0, high.len() - 1, &high, &low, &close, 5, 3, MAType::SMA, 3, MAType::SMA,
                       &mut slow_k, &mut slow_d)?;
println!("{} values from bar {}; last slow_k = {}", range.count, range.beg_idx, slow_k[range.count - 1]);
```

@tab Java

```java
import io.github.talib.Core;
import io.github.talib.MAType;
import io.github.talib.OutRange;

double[] high = new double[252]; // your data, oldest first
double[] low = new double[252]; // your data, oldest first
double[] close = new double[252]; // your data, oldest first
double[] slowK = new double[252];
double[] slowD = new double[252];

Core core = Core.DEFAULT;
OutRange r = core.stoch(0, high.length - 1, high, low, close, 5, 3, MAType.SMA, 3, MAType.SMA,
                        slowK, slowD);
System.out.println(r.count() + " values from bar " + r.begIdx() + "; last slowK = " + slowK[r.count() - 1]);
```

@tab C#

```csharp
using System;
using TALib;

var high = new double[252]; // your data, oldest first
var low = new double[252]; // your data, oldest first
var close = new double[252]; // your data, oldest first
var slowK = new double[252];
var slowD = new double[252];

var core = Core.Default;
OutRange r = core.Stoch(0, high.Length - 1, high, low, close, 5, 3, MAType.SMA, 3, MAType.SMA,
                        slowK, slowD);
Console.WriteLine($"{r.Count} values from bar {r.BegIdx}; last slowK = {slowK[r.Count - 1]}");
```

@tab TypeScript

```ts
import { MAType, stoch } from 'ta-lib';

const high = new Float64Array(252); // your data, oldest first
const low = new Float64Array(252); // your data, oldest first
const close = new Float64Array(252); // your data, oldest first

const { begIdx, outSlowK, outSlowD } = stoch({ high, low, close }, {
  fastKPeriod: 5,
  slowKPeriod: 3,
  slowKMaType: MAType.SMA,
  slowDPeriod: 3,
  slowDMaType: MAType.SMA,
});
console.log(outSlowK.length + ' values from bar ' + begIdx + '; last outSlowK = ' + outSlowK[outSlowK.length - 1]);
```

:::

## Implementation

Added in 0.4.0.

TA-Lib Definition: [`stoch.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/stoch/stoch.c) · [`stoch.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/stoch/stoch.yaml)

| Native | File |
|--------|------|
| C | [`ta_STOCH.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_STOCH.c) |
| Rust | [`stoch.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/library/src/ta_func/stoch.rs) |
| Java | [`Core_STOCH.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/fragments/Core_STOCH.java) |
| C# | [`Core_STOCH.cs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/csharp/library/src/Core_STOCH.cs) |
| TypeScript | [`stoch.ts`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/ts/library/src/func/stoch.ts) |

Python: `talib.STOCH`, see [ta-lib-python](https://ta-lib.github.io/ta-lib-python/func_groups/momentum_indicators.html).

TA-Lib is also available for R and more using a [wrapper](/install/#wrappers).

## Aliases

Stochastic, Stochastic Oscillator, Slow Stochastic

## See Also

[STOCHF](/functions/stochf.md) · [STOCHRSI](/functions/stochrsi.md) · [MA](/functions/ma.md)
