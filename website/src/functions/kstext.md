---
title: "Know Sure Thing with controllable MA type (KSTEXT)"
description: "Know Sure Thing with selectable moving averages: Pring's weighted sum of four smoothed rates of change, with a signal line."
---

## Summary

Know Sure Thing with selectable moving averages: Pring's weighted sum of four smoothed rates of change, with a signal line. One MA type smooths the four legs and another the signal line.

`KST` is Pring's definition, with simple averages throughout. Formulas published since then smooth the legs exponentially, and Pring allows a simple or an exponential signal line. The reading is the same as `KST`'s: above zero the combined momentum is positive, and the usual signals are the line crossing its signal line and the line changing direction.

## Chart

<ta-chart src="/functions/kstext.svg" ratio="720:416" label="KSTEXT over IBM, daily, 1999" alt="Drawn on the price pane over the close as a line; below it, a separate indicator pane with a zero line showing outKST as a solid line and outKSTSignal as a dashed line."></ta-chart>

::: details Chart metadata <span class="flag-tip" tabindex="0" role="note" aria-label="The chart is drawn automatically from metadata TA-Lib provides. Your app can get the same through the Abstract API." data-tip="The chart is drawn automatically from metadata TA-Lib provides. Your app can get the same through the Abstract API.">i</span>

Drawn on the price pane over the close as a line; below it, a separate indicator pane with a zero line showing outKST as a solid line and outKSTSignal as a dashed line.

Metadata flags used: `outKST: line, positive, negative` · `outKSTSignal: dash_line, positive, negative`

:::

[Reference data (CSV)](/functions/kstext.csv) · [Chart (SVG)](/functions/kstext.svg)

## Formula

ROC_k[t] = (price[t] / price[t - X_k] - 1) × 100

RCMA_k = MA_roc(ROC_k, A_k), for k = 1..4

KST = 1 × RCMA_1 + 2 × RCMA_2 + 3 × RCMA_3 + 4 × RCMA_4

Signal = MA_signal(KST, S)

X_k = optInROC{k}Period, A_k = optInMA{k}Period, S = optInSignalPeriod

MA_roc uses optInROCMAType, MA_signal uses optInSignalMAType

## Notes

- **The input is assumed positive.** The price each rate of change looks back to is its divisor. Near zero the divisor is small and the output very large; no error is returned. A series that reaches or crosses zero, such as an oscillator or a spread, has no meaningful result here.
- With both MA types set to `TA_MAType_SMA` the outputs are those of `KST`.
- Both outputs start at the first bar where the signal line exists. A signal period of 1 disables signal-line smoothing for every signal MAType: the signal is then a copy of the line.
- The unstable period of a selected MA type lengthens the lookback, for the legs and for the signal line separately.
- `TA_MAType_MAMA` ignores its period argument, so where it is selected every period above 1 gives the same average.
- Each rate of change follows `ROC`: a zero price in the denominator makes that term 0.
- The weights go with leg position, not with the length of the rate of change.

## Inputs

- `inReal` — Source price series (canonically the close)

## Outputs

- `outKST` — Know Sure Thing line
- `outKSTSignal` — Signal line: MA of the line

## Parameters

| Parameter | Type | Default | Accepted values | Description |
| --- | --- | --- | --- | --- |
| `optInROC1Period` | integer | 10 | 1–100000 | Rate-of-change period of leg 1 (weight 1) |
| `optInROC2Period` | integer | 15 | 1–100000 | Rate-of-change period of leg 2 (weight 2) |
| `optInROC3Period` | integer | 20 | 1–100000 | Rate-of-change period of leg 3 (weight 3) |
| `optInROC4Period` | integer | 30 | 1–100000 | Rate-of-change period of leg 4 (weight 4) |
| `optInMA1Period` | integer | 10 | 1–100000 | Period of the MA smoothing leg 1 |
| `optInMA2Period` | integer | 10 | 1–100000 | Period of the MA smoothing leg 2 |
| `optInMA3Period` | integer | 10 | 1–100000 | Period of the MA smoothing leg 3 |
| `optInMA4Period` | integer | 15 | 1–100000 | Period of the MA smoothing leg 4 |
| `optInSignalPeriod` | integer | 9 | 1–100000 | Period of the signal-line MA |
| `optInROCMAType` | MAType | SMA (0) | any MAType | MA type smoothing the four legs |
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
TA_KSTEXT( 0, n - 1, series, 10, 15, 20, 30, 10, 10, 10, 15, 9, TA_MAType_SMA, TA_MAType_SMA,
           &outBegIdx, &outNBElement, kst, kstSignal );
printf( "%d values from bar %d; last kst = %f\n", outNBElement, outBegIdx, kst[outNBElement - 1] );
```

@tab Rust

```rust
use ta_lib::{Core, MAType};

let series = vec![0.0; 252]; // your data, oldest first
let mut kst = vec![0.0; 252];
let mut kst_signal = vec![0.0; 252];

let core = Core::new();
let range = core.kstext(0, series.len() - 1, &series, 10, 15, 20, 30, 10, 10, 10, 15, 9, MAType::SMA, MAType::SMA,
                        &mut kst, &mut kst_signal)?;
println!("{} values from bar {}; last kst = {}", range.count, range.beg_idx, kst[range.count - 1]);
```

@tab Java

```java
import io.github.talib.Core;
import io.github.talib.MAType;
import io.github.talib.OutRange;

double[] series = new double[252]; // your data, oldest first
double[] kst = new double[252];
double[] kstSignal = new double[252];

Core core = Core.DEFAULT;
OutRange r = core.kstext(0, series.length - 1, series, 10, 15, 20, 30, 10, 10, 10, 15, 9, MAType.SMA, MAType.SMA,
                         kst, kstSignal);
System.out.println(r.count() + " values from bar " + r.begIdx() + "; last kst = " + kst[r.count() - 1]);
```

@tab C\#

```csharp
using System;
using TALib;

var series = new double[252]; // your data, oldest first
var kst = new double[252];
var kstSignal = new double[252];

var core = Core.Default;
OutRange r = core.Kstext(0, series.Length - 1, series, 10, 15, 20, 30, 10, 10, 10, 15, 9, MAType.SMA, MAType.SMA,
                         kst, kstSignal);
Console.WriteLine($"{r.Count} values from bar {r.BegIdx}; last kst = {kst[r.Count - 1]}");
```

@tab TypeScript

```ts
import { MAType, kstext } from 'ta-lib';

const series = new Float64Array(252); // your data, oldest first

const { begIdx, outKst, outKstSignal } = kstext(series, {
  roc1Period: 10,
  roc2Period: 15,
  roc3Period: 20,
  roc4Period: 30,
  ma1Period: 10,
  ma2Period: 10,
  ma3Period: 10,
  ma4Period: 15,
  signalPeriod: 9,
  rocMaType: MAType.SMA,
  signalMaType: MAType.SMA,
});
console.log(outKst.length + ' values from bar ' + begIdx + '; last outKst = ' + outKst[outKst.length - 1]);
```

:::

## Implementation

Added in 0.8.3 (ta-lib-python 0.8.3).

TA-Lib Definition: [`kstext.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/kstext/kstext.c) · [`kstext.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/kstext/kstext.yaml)

| Native | File |
|--------|------|
| C | [`ta_KSTEXT.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_KSTEXT.c) |
| Rust | [`kstext.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/library/src/ta_func/kstext.rs) |
| Java | [`Core_KSTEXT.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/fragments/Core_KSTEXT.java) |
| C# | [`Core_KSTEXT.cs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/csharp/library/src/Core_KSTEXT.cs) |
| TypeScript | [`kstext.ts`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/ts/library/src/func/kstext.ts) |

Python: `talib.KSTEXT` (ta-lib-python 0.8.3 or later), see [ta-lib-python](https://ta-lib.github.io/ta-lib-python/func_groups/momentum_indicators.html).

TA-Lib is also available for R and more using a [wrapper](/install/#wrappers).

## Aliases

Know Sure Thing Extended, KST with controllable MA type

## See Also

[KST](/functions/kst.md) · [ROC](/functions/roc.md) · [MA](/functions/ma.md) · [MACDEXT](/functions/macdext.md) · [COPPOCK](/functions/coppock.md)

## References

- Martin J. Pring, "Summed Rate Of Change (KST)", *Technical Analysis of Stocks & Commodities*, V.10:9, September 1992, pp. 365-369.
- Martin J. Pring, "Identifying Trends With The KST Indicator", *Technical Analysis of Stocks & Commodities*, V.10:10, October 1992, pp. 420-424.
