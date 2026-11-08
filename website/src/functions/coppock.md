---
title: "Coppock Curve (COPPOCK)"
description: "Coppock Curve: Edwin S. \"Sedge\" Coppock's long-term momentum oscillator (Barron's, originally published as the \"Trendex Model\"), computed as a weighted…"
---

## Summary

Coppock Curve: Edwin S. "Sedge" Coppock's long-term momentum oscillator (*Barron's*, originally published as the "Trendex Model"), computed as a weighted moving average of the **sum** of two rates of change.

Unbounded; positive turns from below zero are the signal the indicator was designed for (long-term buying opportunities on monthly index data).

## Chart

<ta-chart src="/functions/coppock.svg" ratio="720:416" label="COPPOCK over IBM, daily, 1999" alt="Drawn on the price pane over the close as a line; below it, a separate indicator pane with a zero line showing outReal as a solid line."></ta-chart>

::: details Chart metadata <span class="flag-tip" tabindex="0" role="note" aria-label="The chart is drawn from the function's metadata: the flags and attributes the abstract API reports for it and for each output, and the input series its reference run declares." data-tip="The chart is drawn from the function's metadata: the flags and attributes the abstract API reports for it and for each output, and the input series its reference run declares.">i</span>

Drawn on the price pane over the close as a line; below it, a separate indicator pane with a zero line showing outReal as a solid line.

Metadata flags used: `outReal: line`

:::

[Reference data (CSV)](/functions/coppock.csv) · [Chart (SVG)](/functions/coppock.svg)

## Formula

`COPPOCK = WMA(ROC(optInROC1Period) + ROC(optInROC2Period), optInWMAPeriod)`

Each ROC carries [`ROC`](/functions/roc.md)'s own zero guard — a zero price `optInROC*Period` bars back yields 0.0 for that term, never an infinity. The two ROCs are **summed**, not averaged: every published definition sums them. (Tulip's `copp` averages, so it reads at exactly half this amplitude — a clean 2.0x ratio against Tulip is Tulip's variant, not a defect.)

The formula is symmetric in the two ROC periods and the lookback keys off their max, so `optInROC1Period > optInROC2Period` is accepted rather than rejected.

The classic defaults are 11/14/10 on monthly data. Wikipedia's daily-scale variant (231/294-bar ROC, 210-bar WMA) is a parameter choice reachable through this API, not a competing formula.

## Inputs

- `inReal` — Source price/value series (canonically a monthly close)

## Outputs

- `outReal` — Coppock Curve value

## Parameters

| Parameter | Type | Default | Accepted values | Description |
| --- | --- | --- | --- | --- |
| `optInWMAPeriod` | integer | 10 | 1–100000 | Smoothing period for the ROC sum |
| `optInROC1Period` | integer | 11 | 1–100000 | Short rate-of-change period |
| `optInROC2Period` | integer | 14 | 1–100000 | Long rate-of-change period |

## Notes

- **The input is assumed positive.** The price each rate of change looks back to is its divisor. Near zero the divisor is small and the output very large; no error is returned. A series that reaches or crosses zero, such as an oscillator or a spread, has no meaningful result here.
- The single fused pass is bit-identical to running `ROC + ROC` into [`WMA`](/functions/wma.md).
- First output at `max(optInROC1Period, optInROC2Period) + optInWMAPeriod - 1`. Not start-dependent: each output depends only on its finite trailing window.

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
| <span class="flag-box">✅</span> **Input Must Be Positive** <span class="flag-tip" tabindex="0" role="note" aria-label="Assumes the input is positive, as a price is. A series that reaches or crosses zero, such as an oscillator or a spread, returns success and an output that may have no meaning. See Notes for what a zero or a negative value does." data-tip="Assumes the input is positive, as a price is. A series that reaches or crosses zero, such as an oscillator or a spread, returns success and an output that may have no meaning. See Notes for what a zero or a negative value does.">i</span> |

</div>

## Example

::: code-tabs#lang

@tab C

```c
#include <stdio.h>
#include <ta_libc.h>

int n = 252;
double series[252]; /* your data, oldest first */
double out[252];
int outBegIdx, outNBElement;

TA_Initialize();
TA_COPPOCK( 0, n - 1, series, 10, 11, 14, &outBegIdx, &outNBElement, out );
printf( "%d values from bar %d; last out = %f\n", outNBElement, outBegIdx, out[outNBElement - 1] );
```

@tab Rust

```rust
use ta_lib::Core;

let series = vec![0.0; 252]; // your data, oldest first
let mut out = vec![0.0; 252];

let core = Core::new();
let range = core.coppock(0, series.len() - 1, &series, 10, 11, 14, &mut out)?;
println!("{} values from bar {}; last out = {}", range.count, range.beg_idx, out[range.count - 1]);
```

@tab Java

```java
import io.github.talib.Core;
import io.github.talib.OutRange;

double[] series = new double[252]; // your data, oldest first
double[] out = new double[252];

Core core = Core.DEFAULT;
OutRange r = core.coppock(0, series.length - 1, series, 10, 11, 14, out);
System.out.println(r.count() + " values from bar " + r.begIdx() + "; last out = " + out[r.count() - 1]);
```

@tab C\#

```csharp
using System;
using TALib;

var series = new double[252]; // your data, oldest first
var outReal = new double[252];

var core = Core.Default;
OutRange r = core.Coppock(0, series.Length - 1, series, 10, 11, 14, outReal);
Console.WriteLine($"{r.Count} values from bar {r.BegIdx}; last outReal = {outReal[r.Count - 1]}");
```

@tab TypeScript

```ts
import { coppock } from 'ta-lib';

const series = new Float64Array(252); // your data, oldest first

const { begIdx, values } = coppock(series, {
  wmaPeriod: 10,
  roc1Period: 11,
  roc2Period: 14,
});
console.log(values.length + ' values from bar ' + begIdx + '; last values = ' + values[values.length - 1]);
```

:::

## Implementation

Added in 0.8.1 (ta-lib-python 0.8.1).

TA-Lib Definition: [`coppock.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/coppock/coppock.c) · [`coppock.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/coppock/coppock.yaml)

| Native | File |
|--------|------|
| C | [`ta_COPPOCK.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_COPPOCK.c) |
| Rust | [`coppock.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/library/src/ta_func/coppock.rs) |
| Java | [`Core_COPPOCK.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/fragments/Core_COPPOCK.java) |
| C# | [`Core_COPPOCK.cs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/csharp/library/src/Core_COPPOCK.cs) |
| TypeScript | [`coppock.ts`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/ts/library/src/func/coppock.ts) |

Python: `talib.COPPOCK` (ta-lib-python 0.8.1 or later), see [ta-lib-python](https://ta-lib.github.io/ta-lib-python/func_groups/momentum_indicators.html).

TA-Lib is also available for R and more using a [wrapper](/install/#wrappers).
