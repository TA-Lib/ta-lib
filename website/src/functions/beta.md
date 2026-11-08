---
title: "Beta (BETA)"
description: "Beta: the slope of a least-squares linear regression of one series' percentage returns (y, from inReal1) against another's (x, from inReal0) over a…"
---

## Summary

Beta: the slope of a least-squares linear regression of one series' percentage returns (y, from inReal1) against another's (x, from inReal0) over a rolling window. Measures how much a security moves relative to a market index. Beta = 1 moves with the index; < 1 less volatile, > 1 more volatile.

## Chart

<ta-chart src="/functions/beta.svg" ratio="720:416" label="BETA over IBM, daily, 1999" alt="Drawn on the price pane over candles; below it, a separate indicator pane with a zero line showing outReal as a solid line."></ta-chart>

::: details Chart metadata <span class="flag-tip" tabindex="0" role="note" aria-label="The chart is drawn from the function's metadata: the flags and attributes the abstract API reports for it and for each output, and the input series its reference run declares." data-tip="The chart is drawn from the function's metadata: the flags and attributes the abstract API reports for it and for each output, and the input series its reference run declares.">i</span>

Drawn on the price pane over candles; below it, a separate indicator pane with a zero line showing outReal as a solid line.

Metadata flags used: `outReal: line`

:::

[Reference data (CSV)](/functions/beta.csv) · [Chart (SVG)](/functions/beta.svg)

## Formula

Per-bar returns: $x_i=(p^0_i-p^0_{i-1})/p^0_{i-1}$ from inReal0, $y_i=(p^1_i-p^1_{i-1})/p^1_{i-1}$ from inReal1. With $n$=period over the window: $\beta = \dfrac{n\,S_{xy}-S_x S_y}{n\,S_{xx}-S_x^2}$, where $S_{xx}=\sum x^2,\ S_{xy}=\sum xy,\ S_x=\sum x,\ S_y=\sum y$.

## Inputs

- `inReal0` — Series whose returns are the regression x (market/index)
- `inReal1` — Series whose returns are the regression y (security)

## Outputs

- `outReal` — Beta: regression slope of inReal1-returns on inReal0-returns

## Parameters

| Parameter | Type | Default | Accepted values | Description |
| --- | --- | --- | --- | --- |
| `optInTimePeriod` | integer | 5 | 1–100000 | Rolling window length (number of returns) for the regression sums |

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
double series0[252]; /* your data, oldest first */
double series1[252]; /* your data, oldest first */
double out[252];
int outBegIdx, outNBElement;

TA_Initialize();
TA_BETA( 0, n - 1, series0, series1, 5, &outBegIdx, &outNBElement, out );
printf( "%d values from bar %d; last out = %f\n", outNBElement, outBegIdx, out[outNBElement - 1] );
```

@tab Rust

```rust
use ta_lib::Core;

let series0 = vec![0.0; 252]; // your data, oldest first
let series1 = vec![0.0; 252]; // your data, oldest first
let mut out = vec![0.0; 252];

let core = Core::new();
let range = core.beta(0, series0.len() - 1, &series0, &series1, 5, &mut out)?;
println!("{} values from bar {}; last out = {}", range.count, range.beg_idx, out[range.count - 1]);
```

@tab Java

```java
import io.github.talib.Core;
import io.github.talib.OutRange;

double[] series0 = new double[252]; // your data, oldest first
double[] series1 = new double[252]; // your data, oldest first
double[] out = new double[252];

Core core = Core.DEFAULT;
OutRange r = core.beta(0, series0.length - 1, series0, series1, 5, out);
System.out.println(r.count() + " values from bar " + r.begIdx() + "; last out = " + out[r.count() - 1]);
```

@tab C\#

```csharp
using System;
using TALib;

var series0 = new double[252]; // your data, oldest first
var series1 = new double[252]; // your data, oldest first
var outReal = new double[252];

var core = Core.Default;
OutRange r = core.Beta(0, series0.Length - 1, series0, series1, 5, outReal);
Console.WriteLine($"{r.Count} values from bar {r.BegIdx}; last outReal = {outReal[r.Count - 1]}");
```

@tab TypeScript

```ts
import { beta } from 'ta-lib';

const series0 = new Float64Array(252); // your data, oldest first
const series1 = new Float64Array(252); // your data, oldest first

const { begIdx, values } = beta(series0, series1, { timePeriod: 5 });
console.log(values.length + ' values from bar ' + begIdx + '; last values = ' + values[values.length - 1]);
```

:::

## Implementation

Added in 0.4.0.

TA-Lib Definition: [`beta.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/beta/beta.c) · [`beta.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/beta/beta.yaml)

| Native | File |
|--------|------|
| C | [`ta_BETA.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_BETA.c) |
| Rust | [`beta.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/library/src/ta_func/beta.rs) |
| Java | [`Core_BETA.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/fragments/Core_BETA.java) |
| C# | [`Core_BETA.cs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/csharp/library/src/Core_BETA.cs) |
| TypeScript | [`beta.ts`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/ts/library/src/func/beta.ts) |

Python: `talib.BETA`, see [ta-lib-python](https://ta-lib.github.io/ta-lib-python/func_groups/statistic_functions.html).

TA-Lib is also available for R and more using a [wrapper](/install/#wrappers).

## Aliases

Beta coefficient

## See Also

[CORREL](/functions/correl.md) · [LINEARREG_SLOPE](/functions/linearreg_slope.md) · [VAR](/functions/var.md) · [STDDEV](/functions/stddev.md)
