---
title: "MACD with controllable MA type (MACDEXT)"
description: "MACD variant where the fast, slow, and signal moving averages each use a user-selectable MA type."
---

## Summary

MACD variant where the fast, slow, and signal moving averages each use a user-selectable MA type. Outputs the MACD line, its signal line, and their difference (histogram). Hist sign change (MACD crossing its signal line) flags momentum shifts.

## Chart

<ta-chart src="/functions/macdext.svg" ratio="720:416" label="MACDEXT over IBM, daily, 1999" alt="Drawn on the price pane over the close as a line; below it, a separate indicator pane with a zero line showing outMACD as a solid line, outMACDSignal as a dashed line and outMACDHist as a sign-coloured histogram."></ta-chart>

Drawn on the price pane over the close as a line; below it, a separate indicator pane with a zero line showing outMACD as a solid line, outMACDSignal as a dashed line and outMACDHist as a sign-coloured histogram. [Reference data (CSV)](/functions/macdext.csv) · [Chart (SVG)](/functions/macdext.svg)

## Formula

MACD = MA_fast(inReal) - MA_slow(inReal)
Signal = MA_signal(MACD)
Hist = MACD - Signal
(each MA_* uses its own MA type and period)

## Notes

- If the slow period is set smaller than the fast period, the fast and slow periods and their MA types are swapped so the slow moving average is always the longer one.
- A signal period of 1 disables signal-line smoothing for every signal MAType: the signal equals the MACD line and the histogram is zero.
- `TA_MAType_MAMA` ignores its period argument, so it always produces the same series regardless of the period requested. If both `optInFastMAType` and `optInSlowMAType` are set to MAMA, the fast and slow lines are therefore identical and MACD, Signal, and Hist are all zero at every bar. Select MAMA for only one side to get a meaningful spread.

## Inputs

- `inReal` — Source series

## Outputs

- `outMACD` — MACD line: fast MA minus slow MA
- `outMACDSignal` — Signal line: MA of the MACD line
- `outMACDHist` — Histogram: MACD minus signal

## Parameters

| Parameter | Type | Default | Accepted values | Description |
| --- | --- | --- | --- | --- |
| `optInFastPeriod` | integer | 12 | 2–100000 | Period of the fast MA |
| `optInFastMAType` | MAType | SMA (0) | any MAType | MA type for the fast MA |
| `optInSlowPeriod` | integer | 26 | 2–100000 | Period of the slow MA |
| `optInSlowMAType` | MAType | SMA (0) | any MAType | MA type for the slow MA |
| `optInSignalPeriod` | integer | 9 | 1–100000 | Period of the signal-line MA |
| `optInSignalMAType` | MAType | SMA (0) | any MAType | MA type for the signal line |

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
double series[252]; /* your data, oldest first */
double macd[252], macdSignal[252], macdHist[252];
int outBegIdx, outNBElement;

TA_Initialize();
TA_MACDEXT( 0, n - 1, series, 12, TA_MAType_SMA, 26, TA_MAType_SMA, 9, TA_MAType_SMA,
            &outBegIdx, &outNBElement, macd, macdSignal, macdHist );
printf( "%d values from bar %d; last macd = %f\n", outNBElement, outBegIdx, macd[outNBElement - 1] );
```

@tab Rust

```rust
use ta_lib::{Core, MAType};

let series = vec![0.0; 252]; // your data, oldest first
let mut macd = vec![0.0; 252];
let mut macd_signal = vec![0.0; 252];
let mut macd_hist = vec![0.0; 252];

let core = Core::new();
let range = core.macdext(0, series.len() - 1, &series, 12, MAType::SMA, 26, MAType::SMA, 9, MAType::SMA,
                         &mut macd, &mut macd_signal, &mut macd_hist)?;
println!("{} values from bar {}; last macd = {}", range.count, range.beg_idx, macd[range.count - 1]);
```

@tab Java

```java
import io.github.talib.Core;
import io.github.talib.MAType;
import io.github.talib.OutRange;

double[] series = new double[252]; // your data, oldest first
double[] macd = new double[252];
double[] macdSignal = new double[252];
double[] macdHist = new double[252];

Core core = Core.DEFAULT;
OutRange r = core.macdext(0, series.length - 1, series, 12, MAType.SMA, 26, MAType.SMA, 9, MAType.SMA,
                          macd, macdSignal, macdHist);
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
OutRange r = core.Macdext(0, series.Length - 1, series, 12, MAType.SMA, 26, MAType.SMA, 9, MAType.SMA,
                          macd, macdSignal, macdHist);
Console.WriteLine($"{r.Count} values from bar {r.BegIdx}; last macd = {macd[r.Count - 1]}");
```

@tab TypeScript

```ts
import { MAType, macdext } from 'ta-lib';

const series = new Float64Array(252); // your data, oldest first

const {
  begIdx,
  outMacd,
  outMacdSignal,
  outMacdHist,
} = macdext(series, {
  fastPeriod: 12,
  fastMaType: MAType.SMA,
  slowPeriod: 26,
  slowMaType: MAType.SMA,
  signalPeriod: 9,
  signalMaType: MAType.SMA,
});
console.log(outMacd.length + ' values from bar ' + begIdx + '; last outMacd = ' + outMacd[outMacd.length - 1]);
```

:::

## Implementation

Added in 0.4.0.

TA-Lib Definition: [`macdext.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/macdext/macdext.c) · [`macdext.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/macdext/macdext.yaml)

| Native | File |
|--------|------|
| C | [`ta_MACDEXT.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_MACDEXT.c) |
| Rust | [`macdext.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/library/src/ta_func/macdext.rs) |
| Java | [`Core_MACDEXT.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/fragments/Core_MACDEXT.java) |
| C# | [`Core_MACDEXT.cs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/csharp/library/src/Core_MACDEXT.cs) |
| TypeScript | [`macdext.ts`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/ts/library/src/func/macdext.ts) |

Python: `talib.MACDEXT`, see [ta-lib-python](https://ta-lib.github.io/ta-lib-python/func_groups/momentum_indicators.html).

TA-Lib is also available for R and more using a [wrapper](/install/#wrappers).

## Aliases

MACD Extended, MACD with controllable MA type

## See Also

[MACD](/functions/macd.md) · [MACDFIX](/functions/macdfix.md) · [MA](/functions/ma.md) · [EMA](/functions/ema.md) · [APO](/functions/apo.md) · [PPO](/functions/ppo.md)
