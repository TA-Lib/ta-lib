---
title: "Standard Deviation (STDDEV)"
description: "Rolling standard deviation of a series over a window, scaled by a deviations multiplier. Delegates to VAR, then takes the square root."
---

## Summary

Rolling standard deviation of a series over a window, scaled by a deviations multiplier. Delegates to VAR, then takes the square root.

## Chart

<ta-chart src="/functions/stddev.svg" ratio="720:416" label="STDDEV over IBM, daily, 1999" alt="Drawn on the price pane over the close as a line; below it, a separate indicator pane with a zero line showing outReal as a solid line."></ta-chart>

::: details Chart metadata <span class="flag-tip" tabindex="0" role="note" aria-label="The chart is drawn from the function's metadata: the flags and attributes the abstract API reports for it and for each output, and the input series its reference run declares." data-tip="The chart is drawn from the function's metadata: the flags and attributes the abstract API reports for it and for each output, and the input series its reference run declares.">i</span>

Drawn on the price pane over the close as a line; below it, a separate indicator pane with a zero line showing outReal as a solid line.

Metadata flags used: `outReal: line, positive, negative`

:::

[Reference data (CSV)](/functions/stddev.csv) · [Chart (SVG)](/functions/stddev.svg)

## Formula

$\sigma_i = \sqrt{\mathrm{VAR}_i}\cdot nbDev$, where $\mathrm{VAR}_i = \frac{1}{N}\sum x^2 - \left(\frac{1}{N}\sum x\right)^2$ (population variance, $N=$ timePeriod)

## Notes

- Uses population variance (divides by the period, not period minus one), so results differ slightly from the sample standard deviation used by some tools.

## Inputs

- `inReal` — Series to measure dispersion of

## Outputs

- `outReal` — Standard deviation at each bar, scaled by optInNbDev

## Parameters

| Parameter | Type | Default | Accepted values | Description |
| --- | --- | --- | --- | --- |
| `optInTimePeriod` | integer | 5 | 2–100000 | Window length |
| `optInNbDev` | real | 1 | any real | Multiplier applied to the standard deviation |

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
double series[252]; /* your data, oldest first */
double out[252];
int outBegIdx, outNBElement;

TA_Initialize();
TA_STDDEV( 0, n - 1, series, 5, 1.0, &outBegIdx, &outNBElement, out );
printf( "%d values from bar %d; last out = %f\n", outNBElement, outBegIdx, out[outNBElement - 1] );
```

@tab Rust

```rust
use ta_lib::Core;

let series = vec![0.0; 252]; // your data, oldest first
let mut out = vec![0.0; 252];

let core = Core::new();
let range = core.stddev(0, series.len() - 1, &series, 5, 1.0, &mut out)?;
println!("{} values from bar {}; last out = {}", range.count, range.beg_idx, out[range.count - 1]);
```

@tab Java

```java
import io.github.talib.Core;
import io.github.talib.OutRange;

double[] series = new double[252]; // your data, oldest first
double[] out = new double[252];

Core core = Core.DEFAULT;
OutRange r = core.stddev(0, series.length - 1, series, 5, 1.0, out);
System.out.println(r.count() + " values from bar " + r.begIdx() + "; last out = " + out[r.count() - 1]);
```

@tab C\#

```csharp
using System;
using TALib;

var series = new double[252]; // your data, oldest first
var outReal = new double[252];

var core = Core.Default;
OutRange r = core.Stddev(0, series.Length - 1, series, 5, 1.0, outReal);
Console.WriteLine($"{r.Count} values from bar {r.BegIdx}; last outReal = {outReal[r.Count - 1]}");
```

@tab TypeScript

```ts
import { stddev } from 'ta-lib';

const series = new Float64Array(252); // your data, oldest first

const { begIdx, values } = stddev(series, { timePeriod: 5, nbDev: 1 });
console.log(values.length + ' values from bar ' + begIdx + '; last values = ' + values[values.length - 1]);
```

:::

## Implementation

Added in 0.4.0.

TA-Lib Definition: [`stddev.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/stddev/stddev.c) · [`stddev.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/stddev/stddev.yaml)

| Native | File |
|--------|------|
| C | [`ta_STDDEV.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_STDDEV.c) |
| Rust | [`stddev.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/library/src/ta_func/stddev.rs) |
| Java | [`Core_STDDEV.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/fragments/Core_STDDEV.java) |
| C# | [`Core_STDDEV.cs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/csharp/library/src/Core_STDDEV.cs) |
| TypeScript | [`stddev.ts`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/ts/library/src/func/stddev.ts) |

Python: `talib.STDDEV`, see [ta-lib-python](https://ta-lib.github.io/ta-lib-python/func_groups/statistic_functions.html).

TA-Lib is also available for R and more using a [wrapper](/install/#wrappers).

## Aliases

Standard Deviation, SD, sigma

## See Also

[VAR](/functions/var.md) · [BBANDS](/functions/bbands.md) · [SMA](/functions/sma.md)
