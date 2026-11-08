---
title: "Double Exponential Moving Average (DEMA)"
description: "Double Exponential Moving Average: an EMA combined with an EMA-of-EMA to reduce lag versus a plain EMA. Overlap Studies overlay on price."
---

## Summary

Double Exponential Moving Average: an EMA combined with an EMA-of-EMA to reduce lag versus a plain EMA. Overlap Studies overlay on price.

## Chart

<ta-chart src="/functions/dema.svg" ratio="720:274" label="DEMA over IBM, daily, 1999" alt="Drawn on the price pane over the close as a line, with outReal as a solid line."></ta-chart>

::: details Chart metadata <span class="flag-tip" tabindex="0" role="note" aria-label="The chart is drawn from the function's metadata: the flags and attributes the abstract API reports for it and for each output, and the input series its reference run declares." data-tip="The chart is drawn from the function's metadata: the flags and attributes the abstract API reports for it and for each output, and the input series its reference run declares.">i</span>

Drawn on the price pane over the close as a line, with outReal as a solid line.

Metadata flags used: `overlap` · `outReal: line`

:::

[Reference data (CSV)](/functions/dema.csv) · [Chart (SVG)](/functions/dema.svg)

## Formula

EMA1 = EMA(inReal, period); EMA2 = EMA(EMA1, period); DEMA = 2*EMA1 - EMA2

## Notes

- A period of 1 performs no smoothing: the output is a copy of the input. Allowed since 0.6.5 (issues #48/#59).

## Inputs

- `inReal` — Source series (typically price)

## Outputs

- `outReal` — DEMA line

## Parameters

| Parameter | Type | Default | Accepted values | Description |
| --- | --- | --- | --- | --- |
| `optInTimePeriod` | integer | 30 | 1–100000 | Smoothing period for both EMA passes |

## Properties

**Numerical Stability:** [Initial Unstable Period](/functions/stability.md#initial-unstable-period) — Inherited from EMA, which DEMA computes internally; tunable via EMA's unstable period.

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
TA_DEMA( 0, n - 1, series, 30, &outBegIdx, &outNBElement, out );
printf( "%d values from bar %d; last out = %f\n", outNBElement, outBegIdx, out[outNBElement - 1] );
```

@tab Rust

```rust
use ta_lib::Core;

let series = vec![0.0; 252]; // your data, oldest first
let mut out = vec![0.0; 252];

let core = Core::new();
let range = core.dema(0, series.len() - 1, &series, 30, &mut out)?;
println!("{} values from bar {}; last out = {}", range.count, range.beg_idx, out[range.count - 1]);
```

@tab Java

```java
import io.github.talib.Core;
import io.github.talib.OutRange;

double[] series = new double[252]; // your data, oldest first
double[] out = new double[252];

Core core = Core.DEFAULT;
OutRange r = core.dema(0, series.length - 1, series, 30, out);
System.out.println(r.count() + " values from bar " + r.begIdx() + "; last out = " + out[r.count() - 1]);
```

@tab C\#

```csharp
using System;
using TALib;

var series = new double[252]; // your data, oldest first
var outReal = new double[252];

var core = Core.Default;
OutRange r = core.Dema(0, series.Length - 1, series, 30, outReal);
Console.WriteLine($"{r.Count} values from bar {r.BegIdx}; last outReal = {outReal[r.Count - 1]}");
```

@tab TypeScript

```ts
import { dema } from 'ta-lib';

const series = new Float64Array(252); // your data, oldest first

const { begIdx, values } = dema(series, { timePeriod: 30 });
console.log(values.length + ' values from bar ' + begIdx + '; last values = ' + values[values.length - 1]);
```

:::

## Implementation

Added in 0.4.0.

TA-Lib Definition: [`dema.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/dema/dema.c) · [`dema.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/dema/dema.yaml)

| Native | File |
|--------|------|
| C | [`ta_DEMA.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_DEMA.c) |
| Rust | [`dema.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/library/src/ta_func/dema.rs) |
| Java | [`Core_DEMA.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/fragments/Core_DEMA.java) |
| C# | [`Core_DEMA.cs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/csharp/library/src/Core_DEMA.cs) |
| TypeScript | [`dema.ts`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/ts/library/src/func/dema.ts) |

Python: `talib.DEMA`, see [ta-lib-python](https://ta-lib.github.io/ta-lib-python/func_groups/overlap_studies.html).

TA-Lib is also available for R and more using a [wrapper](/install/#wrappers).

## Aliases

Double Exponential Moving Average

## See Also

[EMA](/functions/ema.md) · [TEMA](/functions/tema.md) · [MA](/functions/ma.md)

## References

- Patrick G. Mulloy, *Smoothing Data with Faster Moving Averages*, Technical Analysis of Stocks & Commodities, V.12:1 (January 1994)
