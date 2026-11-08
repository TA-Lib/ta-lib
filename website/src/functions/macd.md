---
title: "Moving Average Convergence/Divergence (MACD)"
description: "Moving Average Convergence/Divergence: the difference between a fast and a slow EMA of the input, plus an EMA-smoothed signal line and their histogram."
---

## Summary

Moving Average Convergence/Divergence: the difference between a fast and a slow EMA of the input, plus an EMA-smoothed signal line and their histogram. MACD crossing its signal line and histogram sign changes flag momentum shifts.

## Chart

<ta-chart src="/functions/macd.svg" ratio="720:416" label="MACD over IBM, daily, 1999" alt="Drawn on the price pane over the close as a line; below it, a separate indicator pane with a zero line showing outMACD as a solid line, outMACDSignal as a dashed line and outMACDHist as a sign-coloured histogram."></ta-chart>

::: details Chart metadata <span class="flag-tip" tabindex="0" role="note" aria-label="The chart is drawn from the function's metadata: the flags the abstract API reports for it and for each output, and the input series its reference run declares." data-tip="The chart is drawn from the function's metadata: the flags the abstract API reports for it and for each output, and the input series its reference run declares.">i</span>

Drawn on the price pane over the close as a line; below it, a separate indicator pane with a zero line showing outMACD as a solid line, outMACDSignal as a dashed line and outMACDHist as a sign-coloured histogram.

Metadata flags used: `outMACD: line` · `outMACDSignal: dash_line` · `outMACDHist: histogram`

:::

[Reference data (CSV)](/functions/macd.csv) · [Chart (SVG)](/functions/macd.svg)

## Formula

MACD = EMA_fast - EMA_slow;  Signal = EMA(MACD, signalPeriod);  Hist = MACD - Signal

## Notes

- If the slow period is set smaller than the fast period, the two are swapped so the slow EMA is always the longer one.
- A signal period of 1 disables signal-line smoothing: the signal equals the MACD line and the histogram is zero. Before 0.6.5 this parameter value produced misaligned output (issues #48/#59).

## Inputs

- `inReal` — Input series (typically close)

## Outputs

- `outMACD` — Fast EMA minus slow EMA
- `outMACDSignal` — EMA of the MACD line
- `outMACDHist` — MACD minus signal line

## Parameters

| Parameter | Type | Default | Accepted values | Description |
| --- | --- | --- | --- | --- |
| `optInFastPeriod` | integer | 12 | 2–100000 | Period of the fast EMA |
| `optInSlowPeriod` | integer | 26 | 2–100000 | Period of the slow EMA |
| `optInSignalPeriod` | integer | 9 | 1–100000 | Smoothing period of the signal line |

## Properties

**Numerical Stability:** [Initial Unstable Period](/functions/stability.md#initial-unstable-period) — Inherited from EMA, which MACD computes internally; tunable via EMA's unstable period.

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
TA_MACD( 0, n - 1, series, 12, 26, 9, &outBegIdx, &outNBElement, macd, macdSignal, macdHist );
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
let range = core.macd(0, series.len() - 1, &series, 12, 26, 9,
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
OutRange r = core.macd(0, series.length - 1, series, 12, 26, 9, macd, macdSignal, macdHist);
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
OutRange r = core.Macd(0, series.Length - 1, series, 12, 26, 9, macd, macdSignal, macdHist);
Console.WriteLine($"{r.Count} values from bar {r.BegIdx}; last macd = {macd[r.Count - 1]}");
```

@tab TypeScript

```ts
import { macd } from 'ta-lib';

const series = new Float64Array(252); // your data, oldest first

const {
  begIdx,
  outMacd,
  outMacdSignal,
  outMacdHist,
} = macd(series, { fastPeriod: 12, slowPeriod: 26, signalPeriod: 9 });
console.log(outMacd.length + ' values from bar ' + begIdx + '; last outMacd = ' + outMacd[outMacd.length - 1]);
```

:::

## Implementation

Added in 0.4.0.

TA-Lib Definition: [`macd.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/macd/macd.c) · [`macd.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/macd/macd.yaml)

| Native | File |
|--------|------|
| C | [`ta_MACD.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_MACD.c) |
| Rust | [`macd.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/library/src/ta_func/macd.rs) |
| Java | [`Core_MACD.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/fragments/Core_MACD.java) |
| C# | [`Core_MACD.cs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/csharp/library/src/Core_MACD.cs) |
| TypeScript | [`macd.ts`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/ts/library/src/func/macd.ts) |

Python: `talib.MACD`, see [ta-lib-python](https://ta-lib.github.io/ta-lib-python/func_groups/momentum_indicators.html).

TA-Lib is also available for R and more using a [wrapper](/install/#wrappers).

## Aliases

moving average convergence divergence, moving average convergence/divergence

## See Also

[MACDEXT](/functions/macdext.md) · [MACDFIX](/functions/macdfix.md) · [EMA](/functions/ema.md) · [APO](/functions/apo.md)

## References

- Gerald Appel, *Stock Market Trading Systems*, Traders Pr (ISBN 0934380163)
