---
title: "Normalized Average True Range (NATR)"
description: "Average True Range expressed as a percentage of the current close, making volatility comparable across price levels and securities."
---

## Summary

Average True Range expressed as a percentage of the current close, making volatility comparable across price levels and securities. Same computation as ATR, then normalized by close. Higher values mean greater relative volatility; unit is percent of price.

## Chart

<ta-chart src="/functions/natr.svg" ratio="720:416" label="NATR over IBM, daily, 1999" alt="Drawn on the price pane over candles; below it, a separate indicator pane showing outReal as a solid line."></ta-chart>

::: details Chart metadata <span class="flag-tip" tabindex="0" role="note" aria-label="The chart is drawn from the function's metadata alone: the flags the abstract API reports for it and for each output." data-tip="The chart is drawn from the function's metadata alone: the flags the abstract API reports for it and for each output.">i</span>

Drawn on the price pane over candles; below it, a separate indicator pane showing outReal as a solid line.

Metadata flags used: `inPriceHLC: high, low, close` · `outReal: line`

:::

[Reference data (CSV)](/functions/natr.csv) · [Chart (SVG)](/functions/natr.svg)

## Formula

NATR = (ATR / Close) * 100
ATR: first value = SMA of TRANGE over period; then Wilder smoothing ATR_t = (ATR_{t-1}*(period-1) + TR_t) / period

## Notes

- **The input is assumed positive.** The close is the divisor. Where it is exactly 0 the output is 0. Near zero the divisor is small and the output very large; no error is returned. A series that reaches or crosses zero, such as an oscillator or a spread, has no meaningful result here.

## Inputs

- `inHigh` — High price of each bar
- `inLow` — Low price of each bar
- `inClose` — Close price of each bar

## Outputs

- `outReal` — ATR as a percentage of the close

## Parameters

| Parameter | Type | Default | Accepted values | Description |
| --- | --- | --- | --- | --- |
| `optInTimePeriod` | integer | 14 | 1–100000 | Smoothing period for the true range average |

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
| <span class="flag-box">✅</span> **Input Must Be Positive** <span class="flag-tip" tabindex="0" role="note" aria-label="Assumes the input is positive, as a price is. A series that reaches or crosses zero, such as an oscillator or a spread, returns success and an output that may have no meaning. See Notes for what a zero or a negative value does." data-tip="Assumes the input is positive, as a price is. A series that reaches or crosses zero, such as an oscillator or a spread, returns success and an output that may have no meaning. See Notes for what a zero or a negative value does.">i</span> |

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
TA_NATR( 0, n - 1, high, low, close, 14, &outBegIdx, &outNBElement, out );
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
let range = core.natr(0, high.len() - 1, &high, &low, &close, 14, &mut out)?;
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
OutRange r = core.natr(0, high.length - 1, high, low, close, 14, out);
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
OutRange r = core.Natr(0, high.Length - 1, high, low, close, 14, outReal);
Console.WriteLine($"{r.Count} values from bar {r.BegIdx}; last outReal = {outReal[r.Count - 1]}");
```

@tab TypeScript

```ts
import { natr } from 'ta-lib';

const high = new Float64Array(252); // your data, oldest first
const low = new Float64Array(252); // your data, oldest first
const close = new Float64Array(252); // your data, oldest first

const { begIdx, values } = natr({ high, low, close }, { timePeriod: 14 });
console.log(values.length + ' values from bar ' + begIdx + '; last values = ' + values[values.length - 1]);
```

:::

## Implementation

Added in 0.4.0.

TA-Lib Definition: [`natr.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/natr/natr.c) · [`natr.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/natr/natr.yaml)

| Native | File |
|--------|------|
| C | [`ta_NATR.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_NATR.c) |
| Rust | [`natr.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/library/src/ta_func/natr.rs) |
| Java | [`Core_NATR.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/fragments/Core_NATR.java) |
| C# | [`Core_NATR.cs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/csharp/library/src/Core_NATR.cs) |
| TypeScript | [`natr.ts`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/ts/library/src/func/natr.ts) |

Python: `talib.NATR`, see [ta-lib-python](https://ta-lib.github.io/ta-lib-python/func_groups/volatility_indicators.html).

TA-Lib is also available for R and more using a [wrapper](/install/#wrappers).

## Aliases

Normalized Average True Range

## See Also

[ATR](/functions/atr.md) · [TRANGE](/functions/trange.md) · [SMA](/functions/sma.md)

## References

- John Forman
