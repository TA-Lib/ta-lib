---
title: "Cumulative Sum (CUMSUM)"
description: "Cumulative Sum: the running total of a series from the anchor bar forward."
---

## Summary

Cumulative Sum: the running total of a series from the anchor bar forward. A math primitive rather than an indicator — it is the one operation between the shipped corpus and three named classical breadth indicators: the A/D Line is `CUMSUM(SUB(advances, declines))`, the A/D Volume Line is `CUMSUM(SUB(advancingVolume, decliningVolume))`, and the McClellan Summation Index is `CUMSUM` of the McClellan Oscillator.

[`SUM`](/functions/sum.md) is a *rolling window* over `optInTimePeriod` bars; `CUMSUM` has no window — every bar since the anchor contributes.

## Chart

<ta-chart src="/functions/cumsum.svg" ratio="720:416" label="CUMSUM over IBM, daily, 1999" alt="Drawn on the price pane over the close as a line; below it, a separate indicator pane with a zero line showing outReal as a solid line."></ta-chart>

<div class="ta-chart-links">

[Reference data (CSV)](/functions/cumsum.csv) · [Chart (SVG)](/functions/cumsum.svg)

</div>

::: details Chart metadata <span class="flag-tip" tabindex="0" role="note" aria-label="The chart is drawn automatically from metadata TA-Lib provides. Your app can get the same through the Abstract API." data-tip="The chart is drawn automatically from metadata TA-Lib provides. Your app can get the same through the Abstract API.">i</span>

**Outputs**

- `outReal`: `line` · `zero` · `positive` · `negative`

:::

## Formula

`out[j] = inReal[startIdx] + inReal[startIdx+1] + … + inReal[startIdx+j]`

Left-to-right in one double, no compensation — the same plain `+=` convention the shipped accumulators (`AD`, `OBV`) use.

**The accumulator re-seeds at the anchor.** `CUMSUM(3, 7, x)` starts its total at `x[3]`; it does not warm up from `x[0]`. This is the published contract of the indicators built on it (StockCharts: only the A/D Line's *shape* carries meaning, the first value is "simply Net Advances for one period") and the convention of every shipped path-dependent function. The `path_dependent` flag declares exactly this class.

## Inputs

- `inReal` — Source series (canonically a per-bar net figure, e.g. advances − declines)

## Outputs

- `outReal` — Running total since the anchor bar

## Notes

- Lookback 0: `outBegIdx = startIdx`, one output per input bar. Streaming state is a single accumulator, so a peek commits nothing by construction.
- The sum is uncompensated. A Kahan or Neumaier variant would diverge from `AD`'s own convention, which this follows.

## Properties

**Numerical Stability:** [Path-Dependent](/functions/stability.md#path-dependent)

<div class="flag-cols">

<div class="flag-table">

|  |
| :-- |
| <span class="flag-box">☐</span> <span style="opacity:0.5">Overlap Input</span> |
| <span class="flag-box">✅</span> **Independent Y-Axis** <span class="flag-tip" tabindex="0" role="note" aria-label="Output is on its own scale, drawn in a separate pane below the price chart." data-tip="Output is on its own scale, drawn in a separate pane below the price chart.">i</span> |
| <span class="flag-box">☐</span> <span style="opacity:0.5">Candlestick</span> |
| <span class="flag-box">☐</span> <span style="opacity:0.5">Display Shift</span> |

</div>

<div class="flag-table">

|  |
| :-- |
| <span class="flag-box">☐</span> <span style="opacity:0.5">Can Output NaN or ±Inf</span> |
| <span class="flag-box">☐</span> <span style="opacity:0.5">Identity at Period 1</span> |
| <span class="flag-box">☐</span> <span style="opacity:0.5">Uses Transcendental</span> |
| <span class="flag-box">☐</span> <span style="opacity:0.5">Input Must Be Positive</span> |

</div>

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
TA_CUMSUM( 0, n - 1, series, &outBegIdx, &outNBElement, out );
printf( "%d values from bar %d; last out = %f\n", outNBElement, outBegIdx, out[outNBElement - 1] );
```

@tab Rust

```rust
use ta_lib::Core;

let series = vec![0.0; 252]; // your data, oldest first
let mut out = vec![0.0; 252];

let core = Core::new();
let range = core.cumsum(0, series.len() - 1, &series, &mut out)?;
println!("{} values from bar {}; last out = {}", range.count, range.beg_idx, out[range.count - 1]);
```

@tab Java

```java
import io.github.talib.Core;
import io.github.talib.OutRange;

double[] series = new double[252]; // your data, oldest first
double[] out = new double[252];

Core core = Core.DEFAULT;
OutRange r = core.cumsum(0, series.length - 1, series, out);
System.out.println(r.count() + " values from bar " + r.begIdx() + "; last out = " + out[r.count() - 1]);
```

@tab C\#

```csharp
using System;
using TALib;

var series = new double[252]; // your data, oldest first
var outReal = new double[252];

var core = Core.Default;
OutRange r = core.Cumsum(0, series.Length - 1, series, outReal);
Console.WriteLine($"{r.Count} values from bar {r.BegIdx}; last outReal = {outReal[r.Count - 1]}");
```

@tab TypeScript

```ts
import { cumsum } from 'ta-lib';

const series = new Float64Array(252); // your data, oldest first

const { begIdx, values } = cumsum(series);
console.log(values.length + ' values from bar ' + begIdx + '; last values = ' + values[values.length - 1]);
```

:::

## Implementation

Added in 0.8.1 (ta-lib-python 0.8.1).

TA-Lib Definition: [`cumsum.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/cumsum/cumsum.c) · [`cumsum.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/cumsum/cumsum.yaml)

| Native | File |
|--------|------|
| C | [`ta_CUMSUM.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_CUMSUM.c) |
| Rust | [`cumsum.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/library/src/ta_func/cumsum.rs) |
| Java | [`Core_CUMSUM.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/fragments/Core_CUMSUM.java) |
| C# | [`Core_CUMSUM.cs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/csharp/library/src/Core_CUMSUM.cs) |
| TypeScript | [`cumsum.ts`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/ts/library/src/func/cumsum.ts) |

Python: `talib.CUMSUM` (ta-lib-python 0.8.1 or later), see [ta-lib-python](https://ta-lib.github.io/ta-lib-python/func_groups/math_operators.html).

TA-Lib is also available for R and more using a [wrapper](/install/#wrappers).
