---
title: "Stochastic Relative Strength Index (STOCHRSI)"
description: "Applies the Fast Stochastic (STOCHF) oscillator to an RSI series instead of price, measuring where RSI sits within its recent min/max range."
---

## Summary

Applies the Fast Stochastic (STOCHF) oscillator to an RSI series instead of price, measuring where RSI sits within its recent min/max range. Oscillates 0-100; high = RSI near its recent top, low = near its recent bottom.

## Chart

<ta-chart src="/functions/stochrsi.svg" ratio="720:416" label="STOCHRSI over IBM, daily, 1999" alt="Drawn on the price pane over the close as a line; below it, a separate indicator pane with a zero line showing outFastK as a solid line and outFastD as a dashed line."></ta-chart>

::: details Chart metadata <span class="flag-tip" tabindex="0" role="note" aria-label="The chart is drawn from the function's metadata: the flags and attributes the abstract API reports for it and for each output, and the input series its reference run declares." data-tip="The chart is drawn from the function's metadata: the flags and attributes the abstract API reports for it and for each output, and the input series its reference run declares.">i</span>

Drawn on the price pane over the close as a line; below it, a separate indicator pane with a zero line showing outFastK as a solid line and outFastD as a dashed line.

Metadata flags used: `outFastK: line` · `outFastD: dash_line, positive, negative`

:::

[Reference data (CSV)](/functions/stochrsi.csv) · [Chart (SVG)](/functions/stochrsi.svg)

## Formula

rsi = RSI(inReal, optInTimePeriod)
FastK = 100 * (rsi_t - min(rsi, FastK_Period)) / (max(rsi, FastK_Period) - min(rsi, FastK_Period))
FastD = MA(FastK, FastD_Period, FastD_MAType)

## Notes

- To reproduce the original article's unsmoothed Stochastic RSI, set the RSI period equal to the %K period and read the raw %K output.
- When the RSI's recent range is zero, %K is set to 0 instead of being undefined.

## Inputs

- `inReal` — Source series fed into the RSI calculation

## Outputs

- `outFastK` — Unsmoothed stochastic of the RSI (raw %K)
- `outFastD` — %K smoothed over FastD_Period (signal line)

## Parameters

| Parameter | Type | Default | Accepted values | Description |
| --- | --- | --- | --- | --- |
| `optInTimePeriod` | integer | 14 | 2–100000 | RSI period |
| `optInFastK_Period` | integer | 5 | 1–100000 | Lookback window for the RSI min/max stochastic |
| `optInFastD_Period` | integer | 3 | 1–100000 | Smoothing period for %D |
| `optInFastD_MAType` | MAType | SMA (0) | any MAType | MA type used to smooth %D |

*`MAType` values: 0 SMA · 1 EMA · 2 WMA · 3 DEMA · 4 TEMA · 5 TRIMA · 6 KAMA · 7 MAMA · 8 T3 · 9 HMA · 10 DISABLED · 11 DEFAULT · 12 ZLEMA · 13 RMA · 14 VIDYA · 15 ALMA*

## Properties

**Numerical Stability:** [Initial Unstable Period](/functions/stability.md#initial-unstable-period) — Inherited from RSI, which STOCHRSI computes internally; tunable via RSI's unstable period. The MA type selected may add one of its own.

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
double fastK[252], fastD[252];
int outBegIdx, outNBElement;

TA_Initialize();
TA_STOCHRSI( 0, n - 1, series, 14, 5, 3, TA_MAType_SMA, &outBegIdx, &outNBElement, fastK, fastD );
printf( "%d values from bar %d; last fastK = %f\n", outNBElement, outBegIdx, fastK[outNBElement - 1] );
```

@tab Rust

```rust
use ta_lib::{Core, MAType};

let series = vec![0.0; 252]; // your data, oldest first
let mut fast_k = vec![0.0; 252];
let mut fast_d = vec![0.0; 252];

let core = Core::new();
let range = core.stochrsi(0, series.len() - 1, &series, 14, 5, 3, MAType::SMA,
                          &mut fast_k, &mut fast_d)?;
println!("{} values from bar {}; last fast_k = {}", range.count, range.beg_idx, fast_k[range.count - 1]);
```

@tab Java

```java
import io.github.talib.Core;
import io.github.talib.MAType;
import io.github.talib.OutRange;

double[] series = new double[252]; // your data, oldest first
double[] fastK = new double[252];
double[] fastD = new double[252];

Core core = Core.DEFAULT;
OutRange r = core.stochrsi(0, series.length - 1, series, 14, 5, 3, MAType.SMA, fastK, fastD);
System.out.println(r.count() + " values from bar " + r.begIdx() + "; last fastK = " + fastK[r.count() - 1]);
```

@tab C\#

```csharp
using System;
using TALib;

var series = new double[252]; // your data, oldest first
var fastK = new double[252];
var fastD = new double[252];

var core = Core.Default;
OutRange r = core.Stochrsi(0, series.Length - 1, series, 14, 5, 3, MAType.SMA, fastK, fastD);
Console.WriteLine($"{r.Count} values from bar {r.BegIdx}; last fastK = {fastK[r.Count - 1]}");
```

@tab TypeScript

```ts
import { MAType, stochrsi } from 'ta-lib';

const series = new Float64Array(252); // your data, oldest first

const { begIdx, outFastK, outFastD } = stochrsi(series, {
  timePeriod: 14,
  fastKPeriod: 5,
  fastDPeriod: 3,
  fastDMaType: MAType.SMA,
});
console.log(outFastK.length + ' values from bar ' + begIdx + '; last outFastK = ' + outFastK[outFastK.length - 1]);
```

:::

## Implementation

Added in 0.4.0.

TA-Lib Definition: [`stochrsi.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/stochrsi/stochrsi.c) · [`stochrsi.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/stochrsi/stochrsi.yaml)

| Native | File |
|--------|------|
| C | [`ta_STOCHRSI.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_STOCHRSI.c) |
| Rust | [`stochrsi.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/library/src/ta_func/stochrsi.rs) |
| Java | [`Core_STOCHRSI.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/fragments/Core_STOCHRSI.java) |
| C# | [`Core_STOCHRSI.cs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/csharp/library/src/Core_STOCHRSI.cs) |
| TypeScript | [`stochrsi.ts`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/ts/library/src/func/stochrsi.ts) |

Python: `talib.STOCHRSI`, see [ta-lib-python](https://ta-lib.github.io/ta-lib-python/func_groups/momentum_indicators.html).

TA-Lib is also available for R and more using a [wrapper](/install/#wrappers).

## Aliases

Stochastic RSI

## See Also

[RSI](/functions/rsi.md) · [STOCHF](/functions/stochf.md) · [STOCH](/functions/stoch.md) · [MA](/functions/ma.md)

## References

- Tushar S. Chande, Stanley Kroll, *The New Technical Trader*, John Wiley & Sons (ISBN 0471597805)
