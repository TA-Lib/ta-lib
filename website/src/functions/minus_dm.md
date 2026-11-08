---
title: "Minus Directional Movement (MINUS_DM)"
description: "Minus Directional Movement, the downward component of Wilder's directional movement system. Measures Wilder-smoothed downward price motion over the period."
---

## Summary

Minus Directional Movement, the downward component of Wilder's directional movement system. Measures Wilder-smoothed downward price motion over the period. Higher -DM indicates stronger downward directional movement.

## Chart

<ta-chart src="/functions/minus_dm.svg" ratio="720:416" label="MINUS_DM over IBM, daily, 1999" alt="Drawn on the price pane over candles; below it, a separate indicator pane showing outReal as a solid line."></ta-chart>

Drawn on the price pane over candles; below it, a separate indicator pane showing outReal as a solid line. [Reference data (CSV)](/functions/minus_dm.csv) · [Chart (SVG)](/functions/minus_dm.svg)

## Formula

diffP = high - prevHigh; diffM = prevLow - low
-DM1 = diffM if (diffM > 0 and diffP < diffM) else 0
period<=1: output raw -DM1 per bar.
period>1: seed = sum of first (period-1) -DM1; then Wilder smooth each bar:
-DM = prevMinusDM - prevMinusDM/period (+ -DM1 when the bar qualifies)

## Inputs

- `inHigh` — High price of each bar
- `inLow` — Low price of each bar

## Outputs

- `outReal` — Smoothed minus directional movement

## Parameters

| Parameter | Type | Default | Accepted values | Description |
| --- | --- | --- | --- | --- |
| `optInTimePeriod` | integer | 14 | 1–100000 | Wilder smoothing period |

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
double out[252];
int outBegIdx, outNBElement;

TA_Initialize();
TA_MINUS_DM( 0, n - 1, high, low, 14, &outBegIdx, &outNBElement, out );
printf( "%d values from bar %d; last out = %f\n", outNBElement, outBegIdx, out[outNBElement - 1] );
```

@tab Rust

```rust
use ta_lib::Core;

let high = vec![0.0; 252]; // your data, oldest first
let low = vec![0.0; 252]; // your data, oldest first
let mut out = vec![0.0; 252];

let core = Core::new();
let range = core.minus_dm(0, high.len() - 1, &high, &low, 14, &mut out)?;
println!("{} values from bar {}; last out = {}", range.count, range.beg_idx, out[range.count - 1]);
```

@tab Java

```java
import io.github.talib.Core;
import io.github.talib.OutRange;

double[] high = new double[252]; // your data, oldest first
double[] low = new double[252]; // your data, oldest first
double[] out = new double[252];

Core core = Core.DEFAULT;
OutRange r = core.minusDm(0, high.length - 1, high, low, 14, out);
System.out.println(r.count() + " values from bar " + r.begIdx() + "; last out = " + out[r.count() - 1]);
```

@tab C\#

```csharp
using System;
using TALib;

var high = new double[252]; // your data, oldest first
var low = new double[252]; // your data, oldest first
var outReal = new double[252];

var core = Core.Default;
OutRange r = core.MinusDm(0, high.Length - 1, high, low, 14, outReal);
Console.WriteLine($"{r.Count} values from bar {r.BegIdx}; last outReal = {outReal[r.Count - 1]}");
```

@tab TypeScript

```ts
import { minusDm } from 'ta-lib';

const high = new Float64Array(252); // your data, oldest first
const low = new Float64Array(252); // your data, oldest first

const { begIdx, values } = minusDm({ high, low }, { timePeriod: 14 });
console.log(values.length + ' values from bar ' + begIdx + '; last values = ' + values[values.length - 1]);
```

:::

## Implementation

Added in 0.4.0.

TA-Lib Definition: [`minus_dm.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/minus_dm/minus_dm.c) · [`minus_dm.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/minus_dm/minus_dm.yaml)

| Native | File |
|--------|------|
| C | [`ta_MINUS_DM.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_MINUS_DM.c) |
| Rust | [`minus_dm.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/library/src/ta_func/minus_dm.rs) |
| Java | [`Core_MINUS_DM.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/fragments/Core_MINUS_DM.java) |
| C# | [`Core_MINUS_DM.cs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/csharp/library/src/Core_MINUS_DM.cs) |
| TypeScript | [`minusDm.ts`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/ts/library/src/func/minusDm.ts) |

Python: `talib.MINUS_DM`, see [ta-lib-python](https://ta-lib.github.io/ta-lib-python/func_groups/momentum_indicators.html).

TA-Lib is also available for R and more using a [wrapper](/install/#wrappers).

## Aliases

Minus Directional Movement, -DM

## See Also

[PLUS_DM](/functions/plus_dm.md) · [MINUS_DI](/functions/minus_di.md) · [PLUS_DI](/functions/plus_di.md) · [DX](/functions/dx.md) · [ADX](/functions/adx.md) · [ADXR](/functions/adxr.md)

## References

- J. Welles Wilder, *New Concepts in Technical Trading Systems*, Trend Research (ISBN 0894590278)
