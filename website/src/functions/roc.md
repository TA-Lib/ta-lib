---
title: "Rate of change : ((price/prevPrice)-1)*100 (ROC)"
description: "Rate-of-change momentum oscillator: the percent change of price versus the price optInTimePeriod bars earlier."
---

## Summary

Rate-of-change momentum oscillator: the percent change of price versus the price optInTimePeriod bars earlier. Centered at zero with positive and negative values. Positive when price rose over the period, negative when it fell; magnitude scales the move.

## Chart

<ta-chart src="/functions/roc.svg" ratio="720:416" label="ROC over IBM, daily, 1999" alt="Drawn on the price pane over the close as a line; below it, a separate indicator pane with a zero line showing outReal as a solid line."></ta-chart>

::: details Chart metadata <span class="flag-tip" tabindex="0" role="note" aria-label="The chart is drawn from the function's metadata: the flags and attributes the abstract API reports for it and for each output, and the input series its reference run declares." data-tip="The chart is drawn from the function's metadata: the flags and attributes the abstract API reports for it and for each output, and the input series its reference run declares.">i</span>

Drawn on the price pane over the close as a line; below it, a separate indicator pane with a zero line showing outReal as a solid line.

Metadata flags used: `outReal: line`

:::

[Reference data (CSV)](/functions/roc.csv) · [Chart (SVG)](/functions/roc.svg)

## Formula

ROC = ((price / prevPrice) - 1) * 100, where prevPrice = inReal[i - optInTimePeriod]

## Notes

- **The input is assumed positive.** The price `optInTimePeriod` bars back is the divisor. Where it is exactly 0 the output is 0. Near zero the divisor is small and the output very large; no error is returned. A series that reaches or crosses zero, such as an oscillator or a spread, has no meaningful result here.

## Inputs

- `inReal` — Input price series

## Outputs

- `outReal` — Percent rate of change

## Parameters

| Parameter | Type | Default | Accepted values | Description |
| --- | --- | --- | --- | --- |
| `optInTimePeriod` | integer | 10 | 1–100000 | Lookback distance to the prior price |

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
double out[252];
int outBegIdx, outNBElement;

TA_Initialize();
TA_ROC( 0, n - 1, series, 10, &outBegIdx, &outNBElement, out );
printf( "%d values from bar %d; last out = %f\n", outNBElement, outBegIdx, out[outNBElement - 1] );
```

@tab Rust

```rust
use ta_lib::Core;

let series = vec![0.0; 252]; // your data, oldest first
let mut out = vec![0.0; 252];

let core = Core::new();
let range = core.roc(0, series.len() - 1, &series, 10, &mut out)?;
println!("{} values from bar {}; last out = {}", range.count, range.beg_idx, out[range.count - 1]);
```

@tab Java

```java
import io.github.talib.Core;
import io.github.talib.OutRange;

double[] series = new double[252]; // your data, oldest first
double[] out = new double[252];

Core core = Core.DEFAULT;
OutRange r = core.roc(0, series.length - 1, series, 10, out);
System.out.println(r.count() + " values from bar " + r.begIdx() + "; last out = " + out[r.count() - 1]);
```

@tab C\#

```csharp
using System;
using TALib;

var series = new double[252]; // your data, oldest first
var outReal = new double[252];

var core = Core.Default;
OutRange r = core.Roc(0, series.Length - 1, series, 10, outReal);
Console.WriteLine($"{r.Count} values from bar {r.BegIdx}; last outReal = {outReal[r.Count - 1]}");
```

@tab TypeScript

```ts
import { roc } from 'ta-lib';

const series = new Float64Array(252); // your data, oldest first

const { begIdx, values } = roc(series, { timePeriod: 10 });
console.log(values.length + ' values from bar ' + begIdx + '; last values = ' + values[values.length - 1]);
```

:::

## Implementation

Added in 0.4.0.

TA-Lib Definition: [`roc.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/roc/roc.c) · [`roc.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/roc/roc.yaml)

| Native | File |
|--------|------|
| C | [`ta_ROC.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_ROC.c) |
| Rust | [`roc.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/library/src/ta_func/roc.rs) |
| Java | [`Core_ROC.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/fragments/Core_ROC.java) |
| C# | [`Core_ROC.cs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/csharp/library/src/Core_ROC.cs) |
| TypeScript | [`roc.ts`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/ts/library/src/func/roc.ts) |

Python: `talib.ROC`, see [ta-lib-python](https://ta-lib.github.io/ta-lib-python/func_groups/momentum_indicators.html).

TA-Lib is also available for R and more using a [wrapper](/install/#wrappers).

## Aliases

Rate of Change, Price Rate of Change

## See Also

[MOM](/functions/mom.md) · [ROCP](/functions/rocp.md) · [ROCR](/functions/rocr.md) · [ROCR100](/functions/rocr100.md)
