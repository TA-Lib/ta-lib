---
title: "Directional Movement Index (DX)"
description: "Wilder's Directional Movement Index: the normalized spread between +DI and -DI."
---

## Summary

Wilder's Directional Movement Index: the normalized spread between +DI and -DI. Measures the strength of directional (trending) movement, irrespective of direction. Higher DX = stronger trend (either direction); low DX = ranging market.

## Chart

<ta-chart src="/functions/dx.svg" ratio="720:416" label="DX over IBM, daily, 1999" alt="Drawn on the price pane over candles; below it, a separate indicator pane with a zero line showing outReal as a solid line."></ta-chart>

::: details Chart metadata <span class="flag-tip" tabindex="0" role="note" aria-label="The chart is drawn from the function's metadata alone: the flags the abstract API reports for it and for each output." data-tip="The chart is drawn from the function's metadata alone: the flags the abstract API reports for it and for each output.">i</span>

Drawn on the price pane over candles; below it, a separate indicator pane with a zero line showing outReal as a solid line.

Metadata flags used: `inPriceHLC: high, low, close` · `outReal: line`

:::

[Reference data (CSV)](/functions/dx.csv) · [Chart (SVG)](/functions/dx.svg)

## Formula

Seed +DM14, -DM14, TR14 as sums of the first (period-1) one-period values, then Wilder-smooth each: X = X - X/period + today. +DI = 100*(+DM14/TR14), -DI = 100*(-DM14/TR14). DX = 100 * |(-DI) - (+DI)| / ((-DI) + (+DI)).

## Notes

- Wilder's original integer rounding is not applied (it can be unreliable when values are near 1).
- When +DI and -DI sum to zero the value is undefined; the previous bar's DX is carried forward instead (the first such bar outputs zero).

## Inputs

- `inHigh` — High price of each bar
- `inLow` — Low price of each bar
- `inClose` — Close price of each bar

## Outputs

- `outReal` — DX directional movement index value

## Parameters

| Parameter | Type | Default | Accepted values | Description |
| --- | --- | --- | --- | --- |
| `optInTimePeriod` | integer | 14 | 2–100000 | Smoothing period for the DM and TR sums |

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
TA_DX( 0, n - 1, high, low, close, 14, &outBegIdx, &outNBElement, out );
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
let range = core.dx(0, high.len() - 1, &high, &low, &close, 14, &mut out)?;
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
OutRange r = core.dx(0, high.length - 1, high, low, close, 14, out);
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
OutRange r = core.Dx(0, high.Length - 1, high, low, close, 14, outReal);
Console.WriteLine($"{r.Count} values from bar {r.BegIdx}; last outReal = {outReal[r.Count - 1]}");
```

@tab TypeScript

```ts
import { dx } from 'ta-lib';

const high = new Float64Array(252); // your data, oldest first
const low = new Float64Array(252); // your data, oldest first
const close = new Float64Array(252); // your data, oldest first

const { begIdx, values } = dx({ high, low, close }, { timePeriod: 14 });
console.log(values.length + ' values from bar ' + begIdx + '; last values = ' + values[values.length - 1]);
```

:::

## Implementation

Added in 0.4.0.

TA-Lib Definition: [`dx.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/dx/dx.c) · [`dx.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/dx/dx.yaml)

| Native | File |
|--------|------|
| C | [`ta_DX.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_DX.c) |
| Rust | [`dx.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/library/src/ta_func/dx.rs) |
| Java | [`Core_DX.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/fragments/Core_DX.java) |
| C# | [`Core_DX.cs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/csharp/library/src/Core_DX.cs) |
| TypeScript | [`dx.ts`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/ts/library/src/func/dx.ts) |

Python: `talib.DX`, see [ta-lib-python](https://ta-lib.github.io/ta-lib-python/func_groups/momentum_indicators.html).

TA-Lib is also available for R and more using a [wrapper](/install/#wrappers).

## Aliases

Directional Movement Index, DMI

## See Also

[ADX](/functions/adx.md) · [ADXR](/functions/adxr.md) · [PLUS_DI](/functions/plus_di.md) · [MINUS_DI](/functions/minus_di.md) · [PLUS_DM](/functions/plus_dm.md) · [MINUS_DM](/functions/minus_dm.md) · [TRANGE](/functions/trange.md)

## References

- J. Welles Wilder, *New Concepts in Technical Trading Systems*, Trend Research (ISBN 0894590278)
