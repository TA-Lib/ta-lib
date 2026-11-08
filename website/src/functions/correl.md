---
title: "Pearson's Correlation Coefficient (r) (CORREL)"
description: "Pearson's correlation coefficient (r) between two input series over a rolling window of optInTimePeriod bars."
---

## Summary

Pearson's correlation coefficient (r) between two input series over a rolling window of optInTimePeriod bars. Measures how linearly the two series move together. r near +1: strong positive co-movement; near -1: strong inverse; near 0: no linear relationship.

## Chart

<ta-chart src="/functions/correl.svg" ratio="720:416" label="CORREL over IBM, daily, 1999" alt="Drawn on the price pane over the close as a line; below it, a separate indicator pane showing outReal as a solid line."></ta-chart>

::: details Chart metadata <span class="flag-tip" tabindex="0" role="note" aria-label="The chart is drawn from the function's metadata alone: the flags the abstract API reports for it and for each output." data-tip="The chart is drawn from the function's metadata alone: the flags the abstract API reports for it and for each output.">i</span>

Drawn on the price pane over the close as a line; below it, a separate indicator pane showing outReal as a solid line.

Metadata flags used: `outReal: line`

:::

[Reference data (CSV)](/functions/correl.csv) · [Chart (SVG)](/functions/correl.svg)

## Formula

r = (sumXY - sumX*sumY/n) / sqrt((sumX2 - sumX^2/n) * (sumY2 - sumY^2/n)),  n = optInTimePeriod, sums over the window

## Notes

- When the correlation is undefined for a window (for example a constant series), the output is 0 rather than an error or NaN.

## Inputs

- `inReal0` — First data series (X)
- `inReal1` — Second data series (Y)

## Outputs

- `outReal` — Correlation coefficient r in [-1, 1]

## Parameters

| Parameter | Type | Default | Accepted values | Description |
| --- | --- | --- | --- | --- |
| `optInTimePeriod` | integer | 30 | 1–100000 | Rolling window length |

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
TA_CORREL( 0, n - 1, series0, series1, 30, &outBegIdx, &outNBElement, out );
printf( "%d values from bar %d; last out = %f\n", outNBElement, outBegIdx, out[outNBElement - 1] );
```

@tab Rust

```rust
use ta_lib::Core;

let series0 = vec![0.0; 252]; // your data, oldest first
let series1 = vec![0.0; 252]; // your data, oldest first
let mut out = vec![0.0; 252];

let core = Core::new();
let range = core.correl(0, series0.len() - 1, &series0, &series1, 30, &mut out)?;
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
OutRange r = core.correl(0, series0.length - 1, series0, series1, 30, out);
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
OutRange r = core.Correl(0, series0.Length - 1, series0, series1, 30, outReal);
Console.WriteLine($"{r.Count} values from bar {r.BegIdx}; last outReal = {outReal[r.Count - 1]}");
```

@tab TypeScript

```ts
import { correl } from 'ta-lib';

const series0 = new Float64Array(252); // your data, oldest first
const series1 = new Float64Array(252); // your data, oldest first

const { begIdx, values } = correl(series0, series1, { timePeriod: 30 });
console.log(values.length + ' values from bar ' + begIdx + '; last values = ' + values[values.length - 1]);
```

:::

## Implementation

Added in 0.4.0.

TA-Lib Definition: [`correl.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/correl/correl.c) · [`correl.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/correl/correl.yaml)

| Native | File |
|--------|------|
| C | [`ta_CORREL.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_CORREL.c) |
| Rust | [`correl.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/library/src/ta_func/correl.rs) |
| Java | [`Core_CORREL.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/fragments/Core_CORREL.java) |
| C# | [`Core_CORREL.cs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/csharp/library/src/Core_CORREL.cs) |
| TypeScript | [`correl.ts`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/ts/library/src/func/correl.ts) |

Python: `talib.CORREL`, see [ta-lib-python](https://ta-lib.github.io/ta-lib-python/func_groups/statistic_functions.html).

TA-Lib is also available for R and more using a [wrapper](/install/#wrappers).

## Aliases

Pearson Correlation, Correlation Coefficient, r

## See Also

[BETA](/functions/beta.md) · [STDDEV](/functions/stddev.md) · [VAR](/functions/var.md)

## References

- Karl Pearson
