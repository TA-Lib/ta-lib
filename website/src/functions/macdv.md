---
title: "MACD-V, Volatility Normalised Momentum (MACDV)"
description: "MACD-V, Alex Spiroglou's volatility-normalised MACD: the spread of a fast and a slow EMA of the close over the Average True Range, times 100."
---

## Summary

MACD-V, Alex Spiroglou's volatility-normalised MACD: the spread of a fast and a slow EMA of the close over the Average True Range, times 100. Like MACD it comes with an EMA-smoothed signal line and their histogram. Dividing by the ATR expresses momentum in units of the instrument's own volatility, so one reading means the same thing across instruments, timeframes and volatility regimes, where the raw MACD spread does not. Spiroglou reads the line in ranges: beyond 150 or -150 is an overextended move, between -50 and 50 a ranging market, and the bands in between a trend.

## Chart

<ta-chart src="/functions/macdv.svg" ratio="720:416" label="MACDV over IBM, daily, 1999" alt="Drawn on the price pane over candles; below it, a separate indicator pane with a zero line showing outMACDV as a solid line with dashed rules at -150, -50, 50 and 150, outMACDVSignal as a dashed line and outMACDVHist as a sign-coloured histogram."></ta-chart>

::: details Chart metadata <span class="flag-tip" tabindex="0" role="note" aria-label="The chart is drawn from the function's metadata: the flags and attributes the abstract API reports for it and for each output, and the input series its reference run declares." data-tip="The chart is drawn from the function's metadata: the flags and attributes the abstract API reports for it and for each output, and the input series its reference run declares.">i</span>

Drawn on the price pane over candles; below it, a separate indicator pane with a zero line showing outMACDV as a solid line with dashed rules at -150, -50, 50 and 150, outMACDVSignal as a dashed line and outMACDVHist as a sign-coloured histogram.

Metadata flags used: `inPriceHLC: high, low, close` · `outMACDV: line, positive, negative, levels -150, -50, 50, 150` · `outMACDVSignal: dash_line, positive, negative` · `outMACDVHist: histogram, positive, negative`

:::

[Reference data (CSV)](/functions/macdv.csv) · [Chart (SVG)](/functions/macdv.svg)

## Formula

MACDV = (EMA(Close, fastPeriod) - EMA(Close, slowPeriod)) / ATR(slowPeriod) * 100
Signal = EMA(MACDV, signalPeriod)
Hist = MACDV - Signal

## Notes

- The Average True Range is Wilder's, over the slow period: the publication's `ATR(26)` beside the 26-bar slow EMA, so one parameter sets both. Without price movement the line falls to 0, its neutral point.
- The two price EMAs and the Average True Range each warm up as when calling `EMA` and `ATR` directly, so a caller who wants the line converged sets `TA_FUNC_UNST_EMA` and `TA_FUNC_UNST_ATR`, as for those functions.
- If the slow period is set smaller than the fast period, the two are swapped so the slow EMA is always the longer one, as `MACD` does; the ATR period follows the slow period after the swap.
- A signal period of 1 disables signal-line smoothing: the signal equals the MACD-V line and the histogram is zero.
- The publication's MACD section writes the histogram as the signal line minus the MACD line; its reading of the histogram ("above 40" is stretched to the upside) holds only for MACD-V minus the signal, which is what is emitted here, as for `MACD`.

## Inputs

- `inHigh` — High price of each bar
- `inLow` — Low price of each bar
- `inClose` — Close price of each bar

## Outputs

- `outMACDV` — Fast EMA minus slow EMA, divided by the Average True Range, times 100
- `outMACDVSignal` — EMA of the MACD-V line
- `outMACDVHist` — MACD-V minus signal line

## Parameters

| Parameter | Type | Default | Accepted values | Description |
| --- | --- | --- | --- | --- |
| `optInFastPeriod` | integer | 12 | 2–100000 | Period of the fast EMA |
| `optInSlowPeriod` | integer | 26 | 2–100000 | Period of the slow EMA and of the Average True Range |
| `optInSignalPeriod` | integer | 9 | 1–100000 | Smoothing period of the signal line |

## Properties

**Numerical Stability:** [Initial Unstable Period](/functions/stability.md#initial-unstable-period) — Inherited from ATR and EMA, which MACDV computes internally; tunable via ATR and EMA's unstable period.

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
double macdv[252], macdvSignal[252], macdvHist[252];
int outBegIdx, outNBElement;

TA_Initialize();
TA_MACDV( 0, n - 1, high, low, close, 12, 26, 9,
          &outBegIdx, &outNBElement, macdv, macdvSignal, macdvHist );
printf( "%d values from bar %d; last macdv = %f\n", outNBElement, outBegIdx, macdv[outNBElement - 1] );
```

@tab Rust

```rust
use ta_lib::Core;

let high = vec![0.0; 252]; // your data, oldest first
let low = vec![0.0; 252]; // your data, oldest first
let close = vec![0.0; 252]; // your data, oldest first
let mut macdv = vec![0.0; 252];
let mut macdv_signal = vec![0.0; 252];
let mut macdv_hist = vec![0.0; 252];

let core = Core::new();
let range = core.macdv(0, high.len() - 1, &high, &low, &close, 12, 26, 9,
                       &mut macdv, &mut macdv_signal, &mut macdv_hist)?;
println!("{} values from bar {}; last macdv = {}", range.count, range.beg_idx, macdv[range.count - 1]);
```

@tab Java

```java
import io.github.talib.Core;
import io.github.talib.OutRange;

double[] high = new double[252]; // your data, oldest first
double[] low = new double[252]; // your data, oldest first
double[] close = new double[252]; // your data, oldest first
double[] macdv = new double[252];
double[] macdvSignal = new double[252];
double[] macdvHist = new double[252];

Core core = Core.DEFAULT;
OutRange r = core.macdv(0, high.length - 1, high, low, close, 12, 26, 9,
                        macdv, macdvSignal, macdvHist);
System.out.println(r.count() + " values from bar " + r.begIdx() + "; last macdv = " + macdv[r.count() - 1]);
```

@tab C\#

```csharp
using System;
using TALib;

var high = new double[252]; // your data, oldest first
var low = new double[252]; // your data, oldest first
var close = new double[252]; // your data, oldest first
var macdv = new double[252];
var macdvSignal = new double[252];
var macdvHist = new double[252];

var core = Core.Default;
OutRange r = core.Macdv(0, high.Length - 1, high, low, close, 12, 26, 9,
                        macdv, macdvSignal, macdvHist);
Console.WriteLine($"{r.Count} values from bar {r.BegIdx}; last macdv = {macdv[r.Count - 1]}");
```

@tab TypeScript

```ts
import { macdv } from 'ta-lib';

const high = new Float64Array(252); // your data, oldest first
const low = new Float64Array(252); // your data, oldest first
const close = new Float64Array(252); // your data, oldest first

const {
  begIdx,
  outMacdv,
  outMacdvSignal,
  outMacdvHist,
} = macdv({ high, low, close }, {
  fastPeriod: 12,
  slowPeriod: 26,
  signalPeriod: 9,
});
console.log(outMacdv.length + ' values from bar ' + begIdx + '; last outMacdv = ' + outMacdv[outMacdv.length - 1]);
```

:::

## Implementation

Added in 0.8.4.

TA-Lib Definition: [`macdv.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/macdv/macdv.c) · [`macdv.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/macdv/macdv.yaml)

| Native | File |
|--------|------|
| C | [`ta_MACDV.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_MACDV.c) |
| Rust | [`macdv.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/library/src/ta_func/macdv.rs) |
| Java | [`Core_MACDV.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/fragments/Core_MACDV.java) |
| C# | [`Core_MACDV.cs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/csharp/library/src/Core_MACDV.cs) |
| TypeScript | [`macdv.ts`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/ts/library/src/func/macdv.ts) |

Python: `talib.MACDV`, see [ta-lib-python](https://ta-lib.github.io/ta-lib-python/func_groups/momentum_indicators.html).

TA-Lib is also available for R and more using a [wrapper](/install/#wrappers).

## Aliases

MACD-V, volatility normalised MACD, volatility normalized MACD, volatility normalised momentum

## See Also

[MACD](/functions/macd.md) · [ATR](/functions/atr.md) · [EMA](/functions/ema.md) · [APO](/functions/apo.md) · [PPO](/functions/ppo.md)

## References

- Alex Spiroglou, *MACD-V: Volatility Normalised Momentum*, 2022, [NAAIM Founders Award paper](https://www.naaim.org/wp-content/uploads/2022/05/MACD-V-Alex-Spiroglou-WEB.pdf)
- [MACD-V (StockCharts ChartSchool)](https://chartschool.stockcharts.com/table-of-contents/technical-indicators-and-overlays/technical-indicators/macd-v)
