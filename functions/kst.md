---
url: 'https://ta-lib.org/functions/kst.md'
description: >-
  Know Sure Thing: Martin J. Pring's momentum oscillator, a weighted sum of four
  smoothed rates of change with a moving-average signal line.
---
# Know Sure Thing (Pring) (KST)

## Summary

Know Sure Thing: Martin J. Pring's momentum oscillator, a weighted sum of four smoothed rates of change with a moving-average signal line.

Each leg smooths a rate of change over a different span, and the longer legs carry the larger weights, so the line follows the dominant trend while the short legs make it turn early. Readings above zero mean the combined momentum is positive. The usual signals are the line crossing its signal line and the line changing direction. The level is unbounded, and the usual comparison is against the line's own history.

## Chart

<ta-chart src="/functions/kst.svg" ratio="720:416" label="KST over IBM, daily, 1999" alt="Drawn on the price pane over the close as a line; below it, a separate indicator pane with a zero line showing outKST as a solid line and outKSTSignal as a dashed line."></ta-chart>

::: details Chart metadata <span class="flag-tip" tabindex="0" role="note" aria-label="The chart is drawn from the function's metadata: the flags and attributes the abstract API reports for it and for each output, and the input series its reference run declares." data-tip="The chart is drawn from the function's metadata: the flags and attributes the abstract API reports for it and for each output, and the input series its reference run declares.">i</span>

Drawn on the price pane over the close as a line; below it, a separate indicator pane with a zero line showing outKST as a solid line and outKSTSignal as a dashed line.

Metadata flags used: `outKST: line, positive, negative` · `outKSTSignal: dash_line, positive, negative`

:::

[Reference data (CSV)](/functions/kst.csv) · [Chart (SVG)](/functions/kst.svg)

## Formula

ROC_k\[t] = (price\[t] / price\[t - X_k] - 1) × 100

RCMA_k = SMA(ROC_k, A_k), for k = 1..4

KST = 1 × RCMA_1 + 2 × RCMA_2 + 3 × RCMA_3 + 4 × RCMA_4

Signal = SMA(KST, S)

X_k = optInROC{k}Period, A_k = optInSMA{k}Period, S = optInSignalPeriod

## Notes

* **The input is assumed positive.** The price each rate of change looks back to is its divisor. Near zero the divisor is small and the output very large; no error is returned. A series that reaches or crosses zero, such as an oscillator or a spread, has no meaningful result here.
* The weights are 1 to 4 and nothing divides by their total. Pring's daily table prints each leg's SMA period times its weight (a total of 120) only to show how much the longest leg dominates.
* Both outputs start at the first bar where the signal line exists, so the first few bars where the KST line alone is defined are not emitted. Setting the signal period to 1 returns the line from its own first bar, and the signal output is then a copy of it.
* Each rate of change follows `ROC`: a zero price in the denominator makes that term 0.
* The legs can be in any order. The weights go with leg position, not with the length of the rate of change.
* Pring's daily page uses a 10-day signal. The default signal period follows the charting platforms instead.

## Inputs

* `inReal` — Source price series (canonically the close)

## Outputs

* `outKST` — Know Sure Thing line
* `outKSTSignal` — Simple moving average of the line

## Parameters

| Parameter | Type | Default | Accepted values | Description |
| --- | --- | --- | --- | --- |
| `optInROC1Period` | integer | 10 | 1–100000 | Rate-of-change period of leg 1 (weight 1) |
| `optInROC2Period` | integer | 15 | 1–100000 | Rate-of-change period of leg 2 (weight 2) |
| `optInROC3Period` | integer | 20 | 1–100000 | Rate-of-change period of leg 3 (weight 3) |
| `optInROC4Period` | integer | 30 | 1–100000 | Rate-of-change period of leg 4 (weight 4) |
| `optInSMA1Period` | integer | 10 | 1–100000 | Simple-moving-average period smoothing leg 1 |
| `optInSMA2Period` | integer | 10 | 1–100000 | Simple-moving-average period smoothing leg 2 |
| `optInSMA3Period` | integer | 10 | 1–100000 | Simple-moving-average period smoothing leg 3 |
| `optInSMA4Period` | integer | 15 | 1–100000 | Simple-moving-average period smoothing leg 4 |
| `optInSignalPeriod` | integer | 9 | 1–100000 | Simple-moving-average period of the signal line |

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
double kst[252], kstSignal[252];
int outBegIdx, outNBElement;

TA_Initialize();
TA_KST( 0, n - 1, series, 10, 15, 20, 30, 10, 10, 10, 15, 9,
        &outBegIdx, &outNBElement, kst, kstSignal );
printf( "%d values from bar %d; last kst = %f\n", outNBElement, outBegIdx, kst[outNBElement - 1] );
```

@tab Rust

```rust
use ta_lib::Core;

let series = vec![0.0; 252]; // your data, oldest first
let mut kst = vec![0.0; 252];
let mut kst_signal = vec![0.0; 252];

let core = Core::new();
let range = core.kst(0, series.len() - 1, &series, 10, 15, 20, 30, 10, 10, 10, 15, 9,
                     &mut kst, &mut kst_signal)?;
println!("{} values from bar {}; last kst = {}", range.count, range.beg_idx, kst[range.count - 1]);
```

@tab Java

```java
import io.github.talib.Core;
import io.github.talib.OutRange;

double[] series = new double[252]; // your data, oldest first
double[] kst = new double[252];
double[] kstSignal = new double[252];

Core core = Core.DEFAULT;
OutRange r = core.kst(0, series.length - 1, series, 10, 15, 20, 30, 10, 10, 10, 15, 9,
                      kst, kstSignal);
System.out.println(r.count() + " values from bar " + r.begIdx() + "; last kst = " + kst[r.count() - 1]);
```

@tab C#

```csharp
using System;
using TALib;

var series = new double[252]; // your data, oldest first
var kst = new double[252];
var kstSignal = new double[252];

var core = Core.Default;
OutRange r = core.Kst(0, series.Length - 1, series, 10, 15, 20, 30, 10, 10, 10, 15, 9,
                      kst, kstSignal);
Console.WriteLine($"{r.Count} values from bar {r.BegIdx}; last kst = {kst[r.Count - 1]}");
```

@tab TypeScript

```ts
import { kst } from 'ta-lib';

const series = new Float64Array(252); // your data, oldest first

const { begIdx, outKst, outKstSignal } = kst(series, {
  roc1Period: 10,
  roc2Period: 15,
  roc3Period: 20,
  roc4Period: 30,
  sma1Period: 10,
  sma2Period: 10,
  sma3Period: 10,
  sma4Period: 15,
  signalPeriod: 9,
});
console.log(outKst.length + ' values from bar ' + begIdx + '; last outKst = ' + outKst[outKst.length - 1]);
```

:::

## Implementation

Added in 0.8.3 (ta-lib-python 0.8.3).

TA-Lib Definition: [`kst.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/kst/kst.c) · [`kst.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/kst/kst.yaml)

| Native | File |
|--------|------|
| C | [`ta_KST.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_KST.c) |
| Rust | [`kst.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/library/src/ta_func/kst.rs) |
| Java | [`Core_KST.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/fragments/Core_KST.java) |
| C# | [`Core_KST.cs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/csharp/library/src/Core_KST.cs) |
| TypeScript | [`kst.ts`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/ts/library/src/func/kst.ts) |

Python: `talib.KST` (ta-lib-python 0.8.3 or later), see [ta-lib-python](https://ta-lib.github.io/ta-lib-python/func_groups/momentum_indicators.html).

TA-Lib is also available for R and more using a [wrapper](/install/#wrappers).

## Aliases

Know Sure Thing, Pring KST, Summed Rate of Change

## See Also

[KSTEXT](/functions/kstext.md) · [ROC](/functions/roc.md) · [SMA](/functions/sma.md) · [COPPOCK](/functions/coppock.md) · [MACD](/functions/macd.md)

## References

* Martin J. Pring, "Summed Rate Of Change (KST)", *Technical Analysis of Stocks & Commodities*, V.10:9, September 1992, pp. 365-369.
* Martin J. Pring, "Identifying Trends With The KST Indicator", *Technical Analysis of Stocks & Commodities*, V.10:10, October 1992, pp. 420-424.
* Pring Research, [Introducing the Daily KST](https://web.archive.org/web/20100505073742/www.pring.com/movieweb/daily_kst.htm).
* StockCharts ChartSchool, [Pring's Know Sure Thing (KST)](https://chartschool.stockcharts.com/table-of-contents/technical-indicators-and-overlays/technical-indicators/prings-know-sure-thing-kst).
