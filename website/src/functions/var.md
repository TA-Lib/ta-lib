---
title: "Variance (VAR)"
description: "Rolling population variance of a real series over a given period. Measures dispersion of values around their mean."
---

## Summary

Rolling population variance of a real series over a given period. Measures dispersion of values around their mean. Higher values indicate greater dispersion; 0 means constant input.

## Chart

<ta-chart src="/functions/var.svg" ratio="720:416" label="VAR over IBM, daily, 1999" alt="Drawn on the price pane over the close as a line; below it, a separate indicator pane with a zero line showing outReal as a solid line."></ta-chart>

Drawn on the price pane over the close as a line; below it, a separate indicator pane with a zero line showing outReal as a solid line. [Reference data (CSV)](/functions/var.csv) · [Chart (SVG)](/functions/var.svg)

## Formula

$\mathrm{VAR} = \frac{1}{n}\sum x_i^2 - \left(\frac{1}{n}\sum x_i\right)^2$, over the last $n$ = optInTimePeriod values (population, divides by $n$).

## Notes

- Computes population variance (divides by the period), not the sample variance (n-1) used by some definitions.
- The deviation-count parameter is accepted but has no effect on the result.

## Inputs

- `inReal` — Source series

## Outputs

- `outReal` — Rolling population variance

## Parameters

| Parameter | Type | Default | Accepted values | Description |
| --- | --- | --- | --- | --- |
| `optInTimePeriod` | integer | 5 | 1–100000 | Window length for the variance |
| `optInNbDev` | real | 1 | any real | Deviation count accepted by the API but never used in the computation |

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
TA_VAR( 0, n - 1, series, 5, 1.0, &outBegIdx, &outNBElement, out );
printf( "%d values from bar %d; last out = %f\n", outNBElement, outBegIdx, out[outNBElement - 1] );
```

@tab Rust

```rust
use ta_lib::Core;

let series = vec![0.0; 252]; // your data, oldest first
let mut out = vec![0.0; 252];

let core = Core::new();
let range = core.var(0, series.len() - 1, &series, 5, 1.0, &mut out)?;
println!("{} values from bar {}; last out = {}", range.count, range.beg_idx, out[range.count - 1]);
```

@tab Java

```java
import io.github.talib.Core;
import io.github.talib.OutRange;

double[] series = new double[252]; // your data, oldest first
double[] out = new double[252];

Core core = Core.DEFAULT;
OutRange r = core.var(0, series.length - 1, series, 5, 1.0, out);
System.out.println(r.count() + " values from bar " + r.begIdx() + "; last out = " + out[r.count() - 1]);
```

@tab C\#

```csharp
using System;
using TALib;

var series = new double[252]; // your data, oldest first
var outReal = new double[252];

var core = Core.Default;
OutRange r = core.Var(0, series.Length - 1, series, 5, 1.0, outReal);
Console.WriteLine($"{r.Count} values from bar {r.BegIdx}; last outReal = {outReal[r.Count - 1]}");
```

@tab TypeScript

```ts
import { var_ } from 'ta-lib';

const series = new Float64Array(252); // your data, oldest first

const { begIdx, values } = var_(series, { timePeriod: 5, nbDev: 1 });
console.log(values.length + ' values from bar ' + begIdx + '; last values = ' + values[values.length - 1]);
```

:::

## Implementation

Added in 0.4.0.

TA-Lib Definition: [`var.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/var/var.c) · [`var.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/var/var.yaml)

| Native | File |
|--------|------|
| C | [`ta_VAR.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_VAR.c) |
| Rust | [`var.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/library/src/ta_func/var.rs) |
| Java | [`Core_VAR.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/fragments/Core_VAR.java) |
| C# | [`Core_VAR.cs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/csharp/library/src/Core_VAR.cs) |
| TypeScript | [`var.ts`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/ts/library/src/func/var.ts) |

Python: `talib.VAR`, see [ta-lib-python](https://ta-lib.github.io/ta-lib-python/func_groups/statistic_functions.html).

TA-Lib is also available for R and more using a [wrapper](/install/#wrappers).

## Aliases

Variance

## See Also

[STDDEV](/functions/stddev.md)
