---
title: "Williams' %R (WILLR)"
description: "Williams' %R momentum oscillator over a rolling period, bounded in [-100, 0]."
---

## Summary

Williams' %R momentum oscillator over a rolling period, bounded in [-100, 0]. Measures where the current close sits relative to the high-low range of the last N bars. Near 0 = close at period high (overbought); near -100 = close at period low (oversold).

## Chart

<ta-chart src="/functions/willr.svg" ratio="720:416" label="WILLR over IBM, daily, 1999" alt="Drawn on the price pane over candles; below it, a separate indicator pane with a zero line showing outReal as a solid line."></ta-chart>

::: details Chart metadata <span class="flag-tip" tabindex="0" role="note" aria-label="The chart is drawn from the function's metadata: the flags and attributes the abstract API reports for it and for each output, and the input series its reference run declares." data-tip="The chart is drawn from the function's metadata: the flags and attributes the abstract API reports for it and for each output, and the input series its reference run declares.">i</span>

Drawn on the price pane over candles; below it, a separate indicator pane with a zero line showing outReal as a solid line.

Metadata flags used: `inPriceHLC: high, low, close` · `outReal: line`

:::

[Reference data (CSV)](/functions/willr.csv) · [Chart (SVG)](/functions/willr.svg)

## Formula

%R = ((highestHigh - close) / (highestHigh - lowestLow)) * -100 over the trailing optInTimePeriod bars, clamped to [-100, 0]; if highestHigh == lowestLow, output 0.

## Inputs

- `inHigh` — High price of each bar
- `inLow` — Low price of each bar
- `inClose` — Close price of each bar

## Outputs

- `outReal` — Williams' %R value in [-100, 0]

## Parameters

| Parameter | Type | Default | Accepted values | Description |
| --- | --- | --- | --- | --- |
| `optInTimePeriod` | integer | 14 | 2–100000 | Lookback bars for the high/low range |

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
TA_WILLR( 0, n - 1, high, low, close, 14, &outBegIdx, &outNBElement, out );
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
let range = core.willr(0, high.len() - 1, &high, &low, &close, 14, &mut out)?;
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
OutRange r = core.willr(0, high.length - 1, high, low, close, 14, out);
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
OutRange r = core.Willr(0, high.Length - 1, high, low, close, 14, outReal);
Console.WriteLine($"{r.Count} values from bar {r.BegIdx}; last outReal = {outReal[r.Count - 1]}");
```

@tab TypeScript

```ts
import { willr } from 'ta-lib';

const high = new Float64Array(252); // your data, oldest first
const low = new Float64Array(252); // your data, oldest first
const close = new Float64Array(252); // your data, oldest first

const { begIdx, values } = willr({ high, low, close }, { timePeriod: 14 });
console.log(values.length + ' values from bar ' + begIdx + '; last values = ' + values[values.length - 1]);
```

:::

## Implementation

Added in 0.4.0.

TA-Lib Definition: [`willr.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/willr/willr.c) · [`willr.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/willr/willr.yaml)

| Native | File |
|--------|------|
| C | [`ta_WILLR.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_WILLR.c) |
| Rust | [`willr.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/library/src/ta_func/willr.rs) |
| Java | [`Core_WILLR.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/fragments/Core_WILLR.java) |
| C# | [`Core_WILLR.cs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/csharp/library/src/Core_WILLR.cs) |
| TypeScript | [`willr.ts`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/ts/library/src/func/willr.ts) |

Python: `talib.WILLR`, see [ta-lib-python](https://ta-lib.github.io/ta-lib-python/func_groups/momentum_indicators.html).

TA-Lib is also available for R and more using a [wrapper](/install/#wrappers).

## Aliases

Williams %R, Williams Percent R, %R

## See Also

[STOCH](/functions/stoch.md) · [STOCHF](/functions/stochf.md) · [MINMAX](/functions/minmax.md)
