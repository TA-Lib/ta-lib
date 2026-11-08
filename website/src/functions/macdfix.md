---
title: "Moving Average Convergence/Divergence Fix 12/26 (MACDFIX)"
description: "MACD with the fast/slow EMAs fixed to the classic 12/26 periods (with the classic fixed smoothing factors 0.15 and 0.075), exposing only the signal period."
---

## Summary

MACD with the fast/slow EMAs fixed to the classic 12/26 periods (with the classic fixed smoothing factors 0.15 and 0.075), exposing only the signal period. Signal-line crossovers and histogram sign flag momentum shifts.

## Chart

<ta-chart src="/functions/macdfix.svg" ratio="720:416" label="MACDFIX over IBM, daily, 1999" alt="Drawn on the price pane over the close as a line; below it, a separate indicator pane with a zero line showing outMACD as a solid line, outMACDSignal as a dashed line and outMACDHist as a sign-coloured histogram."></ta-chart>

::: details Chart metadata <span class="flag-tip" tabindex="0" role="note" aria-label="The chart is drawn from the function's metadata: the flags and attributes the abstract API reports for it and for each output, and the input series its reference run declares." data-tip="The chart is drawn from the function's metadata: the flags and attributes the abstract API reports for it and for each output, and the input series its reference run declares.">i</span>

Drawn on the price pane over the close as a line; below it, a separate indicator pane with a zero line showing outMACD as a solid line, outMACDSignal as a dashed line and outMACDHist as a sign-coloured histogram.

Metadata flags used: `outMACD: line` · `outMACDSignal: dash_line` · `outMACDHist: histogram`

:::

[Reference data (CSV)](/functions/macdfix.csv) · [Chart (SVG)](/functions/macdfix.svg)

## Formula

MACD = EMA_12 - EMA_26   (fixed k: 0.15 for 12, 0.075 for 26)
Signal = EMA(MACD, signalPeriod),  k = 2/(signalPeriod+1)
Hist = MACD - Signal

## Notes

- A signal period of 1 disables signal-line smoothing: the signal equals the MACD line and the histogram is zero. Before 0.6.5 this parameter value produced misaligned output (issues #48/#59).

## Inputs

- `inReal` — Source series (typically close)

## Outputs

- `outMACD` — Fixed EMA12 minus EMA26
- `outMACDSignal` — EMA of the MACD line
- `outMACDHist` — MACD minus signal

## Parameters

| Parameter | Type | Default | Accepted values | Description |
| --- | --- | --- | --- | --- |
| `optInSignalPeriod` | integer | 9 | 1–100000 | Smoothing period for the signal line |

## Properties

**Numerical Stability:** [Initial Unstable Period](/functions/stability.md#initial-unstable-period) — Inherited from EMA, which MACDFIX computes internally; tunable via EMA's unstable period.

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
double macd[252], macdSignal[252], macdHist[252];
int outBegIdx, outNBElement;

TA_Initialize();
TA_MACDFIX( 0, n - 1, series, 9, &outBegIdx, &outNBElement, macd, macdSignal, macdHist );
printf( "%d values from bar %d; last macd = %f\n", outNBElement, outBegIdx, macd[outNBElement - 1] );
```

@tab Rust

```rust
use ta_lib::Core;

let series = vec![0.0; 252]; // your data, oldest first
let mut macd = vec![0.0; 252];
let mut macd_signal = vec![0.0; 252];
let mut macd_hist = vec![0.0; 252];

let core = Core::new();
let range = core.macdfix(0, series.len() - 1, &series, 9,
                         &mut macd, &mut macd_signal, &mut macd_hist)?;
println!("{} values from bar {}; last macd = {}", range.count, range.beg_idx, macd[range.count - 1]);
```

@tab Java

```java
import io.github.talib.Core;
import io.github.talib.OutRange;

double[] series = new double[252]; // your data, oldest first
double[] macd = new double[252];
double[] macdSignal = new double[252];
double[] macdHist = new double[252];

Core core = Core.DEFAULT;
OutRange r = core.macdfix(0, series.length - 1, series, 9, macd, macdSignal, macdHist);
System.out.println(r.count() + " values from bar " + r.begIdx() + "; last macd = " + macd[r.count() - 1]);
```

@tab C\#

```csharp
using System;
using TALib;

var series = new double[252]; // your data, oldest first
var macd = new double[252];
var macdSignal = new double[252];
var macdHist = new double[252];

var core = Core.Default;
OutRange r = core.Macdfix(0, series.Length - 1, series, 9, macd, macdSignal, macdHist);
Console.WriteLine($"{r.Count} values from bar {r.BegIdx}; last macd = {macd[r.Count - 1]}");
```

@tab TypeScript

```ts
import { macdfix } from 'ta-lib';

const series = new Float64Array(252); // your data, oldest first

const {
  begIdx,
  outMacd,
  outMacdSignal,
  outMacdHist,
} = macdfix(series, { signalPeriod: 9 });
console.log(outMacd.length + ' values from bar ' + begIdx + '; last outMacd = ' + outMacd[outMacd.length - 1]);
```

:::

## Implementation

Added in 0.4.0.

TA-Lib Definition: [`macdfix.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/macdfix/macdfix.c) · [`macdfix.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/macdfix/macdfix.yaml)

| Native | File |
|--------|------|
| C | [`ta_MACDFIX.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_MACDFIX.c) |
| Rust | [`macdfix.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/library/src/ta_func/macdfix.rs) |
| Java | [`Core_MACDFIX.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/fragments/Core_MACDFIX.java) |
| C# | [`Core_MACDFIX.cs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/csharp/library/src/Core_MACDFIX.cs) |
| TypeScript | [`macdfix.ts`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/ts/library/src/func/macdfix.ts) |

Python: `talib.MACDFIX`, see [ta-lib-python](https://ta-lib.github.io/ta-lib-python/func_groups/momentum_indicators.html).

TA-Lib is also available for R and more using a [wrapper](/install/#wrappers).

## Aliases

Moving Average Convergence/Divergence Fix

## See Also

[MACD](/functions/macd.md) · [MACDEXT](/functions/macdext.md) · [EMA](/functions/ema.md) · [APO](/functions/apo.md)
