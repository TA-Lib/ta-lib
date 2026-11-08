---
title: "Minus Directional Indicator (MINUS_DI)"
description: "Wilder's Minus Directional Indicator: the Wilder-smoothed downward directional movement (-DM) normalized by smoothed True Range."
---

## Summary

Wilder's Minus Directional Indicator: the Wilder-smoothed downward directional movement (-DM) normalized by smoothed True Range. Measures the strength of downward price movement. Higher -DI indicates a stronger downtrend; compared against +DI to gauge directional dominance.

## Chart

<ta-chart src="/functions/minus_di.svg" ratio="720:416" label="MINUS_DI over IBM, daily, 1999" alt="Drawn on the price pane over candles; below it, a separate indicator pane showing outReal as a solid line."></ta-chart>

::: details Chart metadata <span class="flag-tip" tabindex="0" role="note" aria-label="The chart is drawn from the function's metadata alone: the flags the abstract API reports for it and for each output." data-tip="The chart is drawn from the function's metadata alone: the flags the abstract API reports for it and for each output.">i</span>

Drawn on the price pane over candles; below it, a separate indicator pane showing outReal as a solid line.

Metadata flags used: `inPriceHLC: high, low, close` · `outReal: line`

:::

[Reference data (CSV)](/functions/minus_di.csv) · [Chart (SVG)](/functions/minus_di.svg)

## Formula

-DM1 = (prevLow - low) if (prevLow-low)>0 and (high-prevHigh)<(prevLow-low), else 0. Seed -DM/TR = sum of first (period-1) -DM1/TR1, then Wilder-smooth each: X = X - X/period + today. -DI = 100 * (-DM / TR); TR from ta_true_range. If period<=1: -DI1 = -DM1/TR1 (no ×100).

## Notes

- Wilder's original integer rounding is not applied (it was removed as unreliable when values are near 1).

## Inputs

- `inHigh` — High price of each bar
- `inLow` — Low price of each bar
- `inClose` — Close price of each bar

## Outputs

- `outReal` — The Minus Directional Indicator (-DI) line

## Parameters

| Parameter | Type | Default | Accepted values | Description |
| --- | --- | --- | --- | --- |
| `optInTimePeriod` | integer | 14 | 1–100000 | Smoothing/lookback period for -DM and TR |

## Properties

**Numerical Stability:** [Initial Unstable Period](/functions/stability.md#initial-unstable-period)

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
double out[252];
int outBegIdx, outNBElement;

TA_Initialize();
TA_MINUS_DI( 0, n - 1, high, low, close, 14, &outBegIdx, &outNBElement, out );
printf( "%d values from bar %d; last out = %f\n", outNBElement, outBegIdx, out[outNBElement - 1] );
```

@tab Rust

```rust
use ta_lib::Core;

let high = vec![0.0; 252]; // your data, oldest first
let low = vec![0.0; 252]; // your data, oldest first
let close = vec![0.0; 252]; // your data, oldest first
let mut out = vec![0.0; 252];

let core = Core::new();
let range = core.minus_di(0, high.len() - 1, &high, &low, &close, 14, &mut out)?;
println!("{} values from bar {}; last out = {}", range.count, range.beg_idx, out[range.count - 1]);
```

@tab Java

```java
import io.github.talib.Core;
import io.github.talib.OutRange;

double[] high = new double[252]; // your data, oldest first
double[] low = new double[252]; // your data, oldest first
double[] close = new double[252]; // your data, oldest first
double[] out = new double[252];

Core core = Core.DEFAULT;
OutRange r = core.minusDi(0, high.length - 1, high, low, close, 14, out);
System.out.println(r.count() + " values from bar " + r.begIdx() + "; last out = " + out[r.count() - 1]);
```

@tab C\#

```csharp
using System;
using TALib;

var high = new double[252]; // your data, oldest first
var low = new double[252]; // your data, oldest first
var close = new double[252]; // your data, oldest first
var outReal = new double[252];

var core = Core.Default;
OutRange r = core.MinusDi(0, high.Length - 1, high, low, close, 14, outReal);
Console.WriteLine($"{r.Count} values from bar {r.BegIdx}; last outReal = {outReal[r.Count - 1]}");
```

@tab TypeScript

```ts
import { minusDi } from 'ta-lib';

const high = new Float64Array(252); // your data, oldest first
const low = new Float64Array(252); // your data, oldest first
const close = new Float64Array(252); // your data, oldest first

const { begIdx, values } = minusDi({ high, low, close }, { timePeriod: 14 });
console.log(values.length + ' values from bar ' + begIdx + '; last values = ' + values[values.length - 1]);
```

:::

## Implementation

Added in 0.4.0.

TA-Lib Definition: [`minus_di.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/minus_di/minus_di.c) · [`minus_di.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/minus_di/minus_di.yaml)

| Native | File |
|--------|------|
| C | [`ta_MINUS_DI.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_MINUS_DI.c) |
| Rust | [`minus_di.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/library/src/ta_func/minus_di.rs) |
| Java | [`Core_MINUS_DI.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/fragments/Core_MINUS_DI.java) |
| C# | [`Core_MINUS_DI.cs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/csharp/library/src/Core_MINUS_DI.cs) |
| TypeScript | [`minusDi.ts`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/ts/library/src/func/minusDi.ts) |

Python: `talib.MINUS_DI`, see [ta-lib-python](https://ta-lib.github.io/ta-lib-python/func_groups/momentum_indicators.html).

TA-Lib is also available for R and more using a [wrapper](/install/#wrappers).

## Aliases

-DI, Negative Directional Indicator

## See Also

[PLUS_DI](/functions/plus_di.md) · [MINUS_DM](/functions/minus_dm.md) · [DX](/functions/dx.md) · [ADX](/functions/adx.md) · [ADXR](/functions/adxr.md) · [TRANGE](/functions/trange.md)

## References

- J. Welles Wilder, *New Concepts in Technical Trading Systems*, Trend Research (ISBN 0894590278)
