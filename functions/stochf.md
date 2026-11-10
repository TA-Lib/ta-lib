---
url: 'https://ta-lib.org/functions/stochf.md'
description: >-
  Fast Stochastic Oscillator: the raw %K line and its moving-average-smoothed %D
  line.
---
# Stochastic Fast (STOCHF)

## Summary

Fast Stochastic Oscillator: the raw %K line and its moving-average-smoothed %D line. Unlike STOCH (which slows both lines), STOCHF returns the unsmoothed FastK and FastD. Oscillates 0-100; >80 overbought, <20 oversold.

## Chart

<ta-chart src="/functions/stochf.svg" ratio="720:416" label="STOCHF over IBM, daily, 1999" alt="Drawn on the price pane over candles; below it, a separate indicator pane with a zero line showing outFastK as a solid line with dashed rules at 20 and 80 and outFastD as a dashed line."></ta-chart>

<div class="ta-chart-links">

[Reference data (CSV)](/functions/stochf.csv) · [Chart (SVG)](/functions/stochf.svg)

</div>

::: details Chart metadata <span class="flag-tip" tabindex="0" role="note" aria-label="The chart is drawn automatically from metadata TA-Lib provides. Your app can get the same through the Abstract API." data-tip="The chart is drawn automatically from metadata TA-Lib provides. Your app can get the same through the Abstract API.">i</span>

**Outputs**

* `outFastK`: `line` · `zero` · `positive` · `levels: 20, 80`
* `outFastD`: `dash_line` · `zero` · `positive` · `negative`

:::

## Formula

FastK = 100 \* (Close - LowestLow) / (HighestHigh - LowestLow), over the last FastK_Period bars (incl. today)
FastD = MA(FastK, FastD_Period, FastD_MAType)

## Notes

* When the high-low range over the window is zero, %K is set to 0 instead of being undefined.

## Inputs

* `inHigh` — High price of each bar
* `inLow` — Low price of each bar
* `inClose` — Close price of each bar

## Outputs

* `outFastK` — Raw %K stochastic line
* `outFastD` — MA-smoothed %K (signal line)

## Parameters

| Parameter | Type | Default | Accepted values | Description |
| --- | --- | --- | --- | --- |
| `optInFastK_Period` | integer | 5 | 1–100000 | Lookback window for the highest-high/lowest-low of Fast-K |
| `optInFastD_Period` | integer | 3 | 1–100000 | Smoothing period for the Fast-D line |
| `optInFastD_MAType` | MAType | SMA (0) | any MAType | Moving-average type used to smooth Fast-D |

*`MAType` values: 0 SMA · 1 EMA · 2 WMA · 3 DEMA · 4 TEMA · 5 TRIMA · 6 KAMA · 7 MAMA · 8 T3 · 9 HMA · 10 DISABLED · 11 DEFAULT · 12 ZLEMA · 13 RMA · 14 VIDYA · 15 ALMA*

## Properties

**Numerical Stability:** [Depends on MA Type](/functions/stability.md#depends-on-ma-type) — This function's default, SMA, is start-independent.

<div class="flag-cols">

<div class="flag-table">

|  |
| :-- |
| <span class="flag-box">☐</span> <span style="opacity:0.5">Overlap Input</span> |
| <span class="flag-box">✅</span> **Independent Y-Axis** <span class="flag-tip" tabindex="0" role="note" aria-label="Output is on its own scale, drawn in a separate pane below the price chart." data-tip="Output is on its own scale, drawn in a separate pane below the price chart.">i</span> |
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
double close[252]; /* your data, oldest first */
double fastK[252], fastD[252];
int outBegIdx, outNBElement;

TA_Initialize();
TA_STOCHF( 0, n - 1, high, low, close, 5, 3, TA_MAType_SMA,
           &outBegIdx, &outNBElement, fastK, fastD );
printf( "%d values from bar %d; last fastK = %f\n", outNBElement, outBegIdx, fastK[outNBElement - 1] );
```

@tab Rust

```rust
use ta_lib::{Core, MAType};

let high = vec![0.0; 252]; // your data, oldest first
let low = vec![0.0; 252]; // your data, oldest first
let close = vec![0.0; 252]; // your data, oldest first
let mut fast_k = vec![0.0; 252];
let mut fast_d = vec![0.0; 252];

let core = Core::new();
let range = core.stochf(0, high.len() - 1, &high, &low, &close, 5, 3, MAType::SMA,
                        &mut fast_k, &mut fast_d)?;
println!("{} values from bar {}; last fast_k = {}", range.count, range.beg_idx, fast_k[range.count - 1]);
```

@tab Java

```java
import io.github.talib.Core;
import io.github.talib.MAType;
import io.github.talib.OutRange;

double[] high = new double[252]; // your data, oldest first
double[] low = new double[252]; // your data, oldest first
double[] close = new double[252]; // your data, oldest first
double[] fastK = new double[252];
double[] fastD = new double[252];

Core core = Core.DEFAULT;
OutRange r = core.stochf(0, high.length - 1, high, low, close, 5, 3, MAType.SMA, fastK, fastD);
System.out.println(r.count() + " values from bar " + r.begIdx() + "; last fastK = " + fastK[r.count() - 1]);
```

@tab C#

```csharp
using System;
using TALib;

var high = new double[252]; // your data, oldest first
var low = new double[252]; // your data, oldest first
var close = new double[252]; // your data, oldest first
var fastK = new double[252];
var fastD = new double[252];

var core = Core.Default;
OutRange r = core.Stochf(0, high.Length - 1, high, low, close, 5, 3, MAType.SMA, fastK, fastD);
Console.WriteLine($"{r.Count} values from bar {r.BegIdx}; last fastK = {fastK[r.Count - 1]}");
```

@tab TypeScript

```ts
import { MAType, stochf } from 'ta-lib';

const high = new Float64Array(252); // your data, oldest first
const low = new Float64Array(252); // your data, oldest first
const close = new Float64Array(252); // your data, oldest first

const { begIdx, outFastK, outFastD } = stochf({ high, low, close }, {
  fastKPeriod: 5,
  fastDPeriod: 3,
  fastDMaType: MAType.SMA,
});
console.log(outFastK.length + ' values from bar ' + begIdx + '; last outFastK = ' + outFastK[outFastK.length - 1]);
```

:::

## Implementation

Added in 0.4.0.

TA-Lib Definition: [`stochf.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/stochf/stochf.c) · [`stochf.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/stochf/stochf.yaml)

| Native | File |
|--------|------|
| C | [`ta_STOCHF.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_STOCHF.c) |
| Rust | [`stochf.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/library/src/ta_func/stochf.rs) |
| Java | [`Core_STOCHF.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/fragments/Core_STOCHF.java) |
| C# | [`Core_STOCHF.cs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/csharp/library/src/Core_STOCHF.cs) |
| TypeScript | [`stochf.ts`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/ts/library/src/func/stochf.ts) |

Python: `talib.STOCHF`, see [ta-lib-python](https://ta-lib.github.io/ta-lib-python/func_groups/momentum_indicators.html).

TA-Lib is also available for R and more using a [wrapper](/install/#wrappers).

## Aliases

Stochastic Fast, Fast Stochastic Oscillator

## See Also

[STOCH](/functions/stoch.md) · [STOCHRSI](/functions/stochrsi.md) · [MA](/functions/ma.md)
