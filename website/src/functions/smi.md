---
title: "Stochastic Momentum Index (SMI)"
description: "Stochastic Momentum Index: where the close sits relative to the midpoint of the recent high/low range, double-smoothed."
---

## Summary

Stochastic Momentum Index: where the close sits relative to the **midpoint** of the recent high/low range, double-smoothed. Lane's stochastic measures the close against the bottom of the range; Blau measures it against the middle, then smooths numerator and denominator separately with two exponential averages before dividing, which is what buys the low-lag, smooth-contoured curve. The result runs -100 to +100 rather than 0 to 100, so the zero line is the reference: positive means the close is above the midpoint of its range, negative below. Extreme readings mark overbought and oversold conditions, and crossings of the signal line are the usual trade trigger.

## Chart

<ta-chart src="/functions/smi.svg" ratio="720:416" label="SMI over IBM, daily, 1999" alt="Drawn on the price pane over candles; below it, a separate indicator pane with a zero line showing outSMI as a solid line with dashed rules at -40 and 40 and outSMISignal as a dashed line."></ta-chart>

<div class="ta-chart-links">

[Reference data (CSV)](/functions/smi.csv) · [Chart (SVG)](/functions/smi.svg)

</div>

::: details Chart metadata <span class="flag-tip" tabindex="0" role="note" aria-label="The chart is drawn automatically from metadata TA-Lib provides. Your app can get the same through the Abstract API." data-tip="The chart is drawn automatically from metadata TA-Lib provides. Your app can get the same through the Abstract API.">i</span>

**Outputs**

- `outSMI`: `line` · `zero` · `positive` · `negative` · `levels: -40, 40`
- `outSMISignal`: `dash_line` · `zero` · `positive` · `negative`

:::

## Formula

HH = MAX(high, timePeriod);  LL = MIN(low, timePeriod)

num = close - 0.5 * (HH + LL);  den = HH - LL

SMI = 100 * EMA(EMA(num, slowPeriod), fastPeriod) / (0.5 * EMA(EMA(den, slowPeriod), fastPeriod))

Signal = EMA(SMI, signalPeriod)

## Notes

- A window whose bars are all flat (every high equal to its low) leaves both the numerator and the denominator at zero. Rather than divide, SMI emits 0 there, its neutral value, as CCI and IMI emit theirs. Some implementations divide unguarded and return a non-finite value.
- Each exponential average is seeded with a simple average of its own first inputs, the same seeding TA-Lib's EMA uses, so the first published values converge toward an unlimited-history result rather than reproducing it exactly. `TA_SetUnstablePeriod(TA_FUNC_UNST_EMA, ...)` discards more of that warm-up. Implementations seeding from a single first sample — Tulip and TradingView among them — differ over the transient and agree once it decays.
- One output range covers both outputs, so the SMI values consumed by the signal line's own warm-up are not published.

## Inputs

- `inHigh` — High price series
- `inLow` — Low price series
- `inClose` — Close price series

## Outputs

- `outSMI` — Stochastic Momentum Index, -100 to +100
- `outSMISignal` — Exponential average of the SMI line

## Parameters

| Parameter | Type | Default | Accepted values | Description |
| --- | --- | --- | --- | --- |
| `optInTimePeriod` | integer | 13 | 2–100000 | Period of the high/low range |
| `optInFastPeriod` | integer | 2 | 2–100000 | Period of the second smoothing, applied to the first |
| `optInSlowPeriod` | integer | 25 | 2–100000 | Period of the first smoothing, applied to the raw momentum |
| `optInSignalPeriod` | integer | 9 | 2–100000 | Smoothing period of the signal line |

## Properties

**Numerical Stability:** [Initial Unstable Period](/functions/stability.md#initial-unstable-period) — Inherited from EMA, which SMI computes internally; tunable via EMA's unstable period.

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
double smi[252], smiSignal[252];
int outBegIdx, outNBElement;

TA_Initialize();
TA_SMI( 0, n - 1, high, low, close, 13, 2, 25, 9, &outBegIdx, &outNBElement, smi, smiSignal );
printf( "%d values from bar %d; last smi = %f\n", outNBElement, outBegIdx, smi[outNBElement - 1] );
```

@tab Rust

```rust
use ta_lib::Core;

let high = vec![0.0; 252]; // your data, oldest first
let low = vec![0.0; 252]; // your data, oldest first
let close = vec![0.0; 252]; // your data, oldest first
let mut smi = vec![0.0; 252];
let mut smi_signal = vec![0.0; 252];

let core = Core::new();
let range = core.smi(0, high.len() - 1, &high, &low, &close, 13, 2, 25, 9,
                     &mut smi, &mut smi_signal)?;
println!("{} values from bar {}; last smi = {}", range.count, range.beg_idx, smi[range.count - 1]);
```

@tab Java

```java
import io.github.talib.Core;
import io.github.talib.OutRange;

double[] high = new double[252]; // your data, oldest first
double[] low = new double[252]; // your data, oldest first
double[] close = new double[252]; // your data, oldest first
double[] smi = new double[252];
double[] smiSignal = new double[252];

Core core = Core.DEFAULT;
OutRange r = core.smi(0, high.length - 1, high, low, close, 13, 2, 25, 9, smi, smiSignal);
System.out.println(r.count() + " values from bar " + r.begIdx() + "; last smi = " + smi[r.count() - 1]);
```

@tab C\#

```csharp
using System;
using TALib;

var high = new double[252]; // your data, oldest first
var low = new double[252]; // your data, oldest first
var close = new double[252]; // your data, oldest first
var smi = new double[252];
var smiSignal = new double[252];

var core = Core.Default;
OutRange r = core.Smi(0, high.Length - 1, high, low, close, 13, 2, 25, 9, smi, smiSignal);
Console.WriteLine($"{r.Count} values from bar {r.BegIdx}; last smi = {smi[r.Count - 1]}");
```

@tab TypeScript

```ts
import { smi } from 'ta-lib';

const high = new Float64Array(252); // your data, oldest first
const low = new Float64Array(252); // your data, oldest first
const close = new Float64Array(252); // your data, oldest first

const { begIdx, outSmi, outSmiSignal } = smi({ high, low, close }, {
  timePeriod: 13,
  fastPeriod: 2,
  slowPeriod: 25,
  signalPeriod: 9,
});
console.log(outSmi.length + ' values from bar ' + begIdx + '; last outSmi = ' + outSmi[outSmi.length - 1]);
```

:::

## Implementation

Added in 0.8.1 (ta-lib-python 0.8.1).

TA-Lib Definition: [`smi.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/smi/smi.c) · [`smi.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/smi/smi.yaml)

| Native | File |
|--------|------|
| C | [`ta_SMI.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_SMI.c) |
| Rust | [`smi.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/library/src/ta_func/smi.rs) |
| Java | [`Core_SMI.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/fragments/Core_SMI.java) |
| C# | [`Core_SMI.cs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/csharp/library/src/Core_SMI.cs) |
| TypeScript | [`smi.ts`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/ts/library/src/func/smi.ts) |

Python: `talib.SMI` (ta-lib-python 0.8.1 or later), see [ta-lib-python](https://ta-lib.github.io/ta-lib-python/func_groups/momentum_indicators.html).

TA-Lib is also available for R and more using a [wrapper](/install/#wrappers).

## Aliases

stochastic momentum index, Blau stochastic momentum

## See Also

[STOCH](/functions/stoch.md) · [STOCHRSI](/functions/stochrsi.md) · [WILLR](/functions/willr.md) · [MACD](/functions/macd.md)

## References

- William Blau, "Stochastic Momentum", *Technical Analysis of Stocks & Commodities*, v11:1 (January 1993), pp. 11-18
- William Blau, *Momentum, Direction and Divergence*, Wiley 1995 (ISBN 0471027294)
