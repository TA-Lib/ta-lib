---
title: "Exponential Moving Average (EMA)"
description: "Exponential moving average that weights recent prices more heavily via a recursive smoothing factor."
---

## Summary

Exponential moving average that weights recent prices more heavily via a recursive smoothing factor. A core building block seeding or composing many other indicators. Reacts faster than SMA; price above/below EMA suggests up/down trend.

## Chart

<ta-chart src="/functions/ema.svg" ratio="720:274" label="EMA over IBM, daily, 1999" alt="Drawn on the price pane over the close as a line, with outReal as a solid line."></ta-chart>

::: details Chart metadata <span class="flag-tip" tabindex="0" role="note" aria-label="The chart is drawn from the function's metadata alone: the flags the abstract API reports for it and for each output." data-tip="The chart is drawn from the function's metadata alone: the flags the abstract API reports for it and for each output.">i</span>

Drawn on the price pane over the close as a line, with outReal as a solid line.

Metadata flags used: `overlap` · `outReal: line`

:::

[Reference data (CSV)](/functions/ema.csv) · [Chart (SVG)](/functions/ema.svg)

## Formula

k = 2 / (period + 1); EMA_t = (price_t - EMA_{t-1}) * k + EMA_{t-1}. Seed: EMA = SMA of first `period` bars.

## Notes

- A period of 1 performs no smoothing: the output is a copy of the input. Allowed since 0.6.5 (issues #48/#59).

## Inputs

- `inReal` — price/data series to smooth

## Outputs

- `outReal` — the exponential moving average

## Parameters

| Parameter | Type | Default | Accepted values | Description |
| --- | --- | --- | --- | --- |
| `optInTimePeriod` | integer | 30 | 1–100000 | Number of bars in the average; sets smoothing k = 2/(period+1) |

## Properties

**Numerical Stability:** [Initial Unstable Period](/functions/stability.md#initial-unstable-period)

<div class="flag-table">

|  |
| :-- |
| <span class="flag-box">✅</span> **Overlap Input** <span class="flag-tip" tabindex="0" role="note" aria-label="Output is on the same scale as the input price, so it is drawn over the price chart." data-tip="Output is on the same scale as the input price, so it is drawn over the price chart.">i</span> |
| <span class="flag-box">☐</span> <span style="opacity:0.5">Independent Y-Axis</span> |
| <span class="flag-box">☐</span> <span style="opacity:0.5">Candlestick</span> |
| <span class="flag-box">☐</span> <span style="opacity:0.5">Can Output NaN or ±Inf</span> |
| <span class="flag-box">✅</span> **Identity at Period 1** <span class="flag-tip" tabindex="0" role="note" aria-label="A period of 1 performs no smoothing: every output value is a bit-exact copy of its input value." data-tip="A period of 1 performs no smoothing: every output value is a bit-exact copy of its input value.">i</span> |
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
double out[252];
int outBegIdx, outNBElement;

TA_Initialize();
TA_EMA( 0, n - 1, series, 30, &outBegIdx, &outNBElement, out );
printf( "%d values from bar %d; last out = %f\n", outNBElement, outBegIdx, out[outNBElement - 1] );
```

@tab Rust

```rust
use ta_lib::Core;

let series = vec![0.0; 252]; // your data, oldest first
let mut out = vec![0.0; 252];

let core = Core::new();
let range = core.ema(0, series.len() - 1, &series, 30, &mut out)?;
println!("{} values from bar {}; last out = {}", range.count, range.beg_idx, out[range.count - 1]);
```

@tab Java

```java
import io.github.talib.Core;
import io.github.talib.OutRange;

double[] series = new double[252]; // your data, oldest first
double[] out = new double[252];

Core core = Core.DEFAULT;
OutRange r = core.ema(0, series.length - 1, series, 30, out);
System.out.println(r.count() + " values from bar " + r.begIdx() + "; last out = " + out[r.count() - 1]);
```

@tab C\#

```csharp
using System;
using TALib;

var series = new double[252]; // your data, oldest first
var outReal = new double[252];

var core = Core.Default;
OutRange r = core.Ema(0, series.Length - 1, series, 30, outReal);
Console.WriteLine($"{r.Count} values from bar {r.BegIdx}; last outReal = {outReal[r.Count - 1]}");
```

@tab TypeScript

```ts
import { ema } from 'ta-lib';

const series = new Float64Array(252); // your data, oldest first

const { begIdx, values } = ema(series, { timePeriod: 30 });
console.log(values.length + ' values from bar ' + begIdx + '; last values = ' + values[values.length - 1]);
```

:::

## Implementation

Added in 0.4.0.

TA-Lib Definition: [`ema.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/ema/ema.c) · [`ema.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/ema/ema.yaml)

| Native | File |
|--------|------|
| C | [`ta_EMA.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_EMA.c) |
| Rust | [`ema.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/library/src/ta_func/ema.rs) |
| Java | [`Core_EMA.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/fragments/Core_EMA.java) |
| C# | [`Core_EMA.cs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/csharp/library/src/Core_EMA.cs) |
| TypeScript | [`ema.ts`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/ts/library/src/func/ema.ts) |

Python: `talib.EMA`, see [ta-lib-python](https://ta-lib.github.io/ta-lib-python/func_groups/overlap_studies.html).

TA-Lib is also available for R and more using a [wrapper](/install/#wrappers).

## Aliases

Exponential Moving Average, Exponentially Weighted Moving Average, EWMA

## See Also

[SMA](/functions/sma.md) · [DEMA](/functions/dema.md) · [TEMA](/functions/tema.md) · [MA](/functions/ma.md) · [MACD](/functions/macd.md) · [T3](/functions/t3.md)
