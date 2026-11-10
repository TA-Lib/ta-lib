---
title: "Hilbert Transform - Trend vs Cycle Mode (HT_TRENDMODE)"
description: "Hilbert Transform classifier that labels each bar 1 (trending — favor trend-following) or 0 (cycling — favor mean-reversion)."
---

## Summary

Hilbert Transform classifier that labels each bar 1 (trending — favor trend-following) or 0 (cycling — favor mean-reversion). Built from the same MAMA dominant-cycle/phase DSP plus a SineWave/trendline test used across the other HT_* functions.

## Chart

<ta-chart src="/functions/ht_trendmode.svg" ratio="720:416" label="HT_TRENDMODE over IBM, daily, 1999" alt="Drawn on the price pane over the close as a line; below it, a step pane of its own for the integer output outInteger."></ta-chart>

<div class="ta-chart-links">

[Reference data (CSV)](/functions/ht_trendmode.csv) · [Chart (SVG)](/functions/ht_trendmode.svg)

</div>

::: details Chart metadata <span class="flag-tip" tabindex="0" role="note" aria-label="The chart is drawn automatically from metadata TA-Lib provides. Your app can get the same through the Abstract API." data-tip="The chart is drawn automatically from metadata TA-Lib provides. Your app can get the same through the Abstract API.">i</span>

**Outputs**

- `outInteger`: `line`

:::

## Interpretation

Mean-reversion is the trading assumption that price will swing back toward its recent average rather than keep moving in one direction — instead of chasing a breakout, it buys near the low end of the range and sells near the high end, betting on a reversal rather than continuation. A 0 reading is HT_TRENDMODE's signal that this bar fits that regime: price is oscillating rather than trending, so fading the extremes is expected to hold up better than following the move.

## Inputs

- `inReal` — Source price series

## Outputs

- `outInteger` — 1 = trend mode, 0 = cycle mode

## Properties

**Numerical Stability:** [Initial Unstable Period](/functions/stability.md#initial-unstable-period)

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
| <span class="flag-box">✅</span> **Uses Transcendental** <span class="flag-tip" tabindex="0" role="note" aria-label="Calls a transcendental math function (exp, log, log10, trigonometric, inverse trigonometric or hyperbolic), so a value may differ slightly between languages, between a stream and a batch call, and between machines. TA-Lib targets a difference below 1e-9." data-tip="Calls a transcendental math function (exp, log, log10, trigonometric, inverse trigonometric or hyperbolic), so a value may differ slightly between languages, between a stream and a batch call, and between machines. TA-Lib targets a difference below 1e-9.">i</span> |
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
int out[252];
int outBegIdx, outNBElement;

TA_Initialize();
TA_HT_TRENDMODE( 0, n - 1, series, &outBegIdx, &outNBElement, out );
printf( "%d values from bar %d; last out = %d\n", outNBElement, outBegIdx, out[outNBElement - 1] );
```

@tab Rust

```rust
use ta_lib::Core;

let series = vec![0.0; 252]; // your data, oldest first
let mut out = vec![0i32; 252];

let core = Core::new();
let range = core.ht_trendmode(0, series.len() - 1, &series, &mut out)?;
println!("{} values from bar {}; last out = {}", range.count, range.beg_idx, out[range.count - 1]);
```

@tab Java

```java
import io.github.talib.Core;
import io.github.talib.OutRange;

double[] series = new double[252]; // your data, oldest first
int[] out = new int[252];

Core core = Core.DEFAULT;
OutRange r = core.htTrendmode(0, series.length - 1, series, out);
System.out.println(r.count() + " values from bar " + r.begIdx() + "; last out = " + out[r.count() - 1]);
```

@tab C\#

```csharp
using System;
using TALib;

var series = new double[252]; // your data, oldest first
var outInteger = new int[252];

var core = Core.Default;
OutRange r = core.HtTrendmode(0, series.Length - 1, series, outInteger);
Console.WriteLine($"{r.Count} values from bar {r.BegIdx}; last outInteger = {outInteger[r.Count - 1]}");
```

@tab TypeScript

```ts
import { htTrendmode } from 'ta-lib';

const series = new Float64Array(252); // your data, oldest first

const { begIdx, values } = htTrendmode(series);
console.log(values.length + ' values from bar ' + begIdx + '; last values = ' + values[values.length - 1]);
```

:::

## Implementation

Added in 0.4.0.

TA-Lib Definition: [`ht_trendmode.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/ht_trendmode/ht_trendmode.c) · [`ht_trendmode.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/ht_trendmode/ht_trendmode.yaml)

| Native | File |
|--------|------|
| C | [`ta_HT_TRENDMODE.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_HT_TRENDMODE.c) |
| Rust | [`ht_trendmode.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/library/src/ta_func/ht_trendmode.rs) |
| Java | [`Core_HT_TRENDMODE.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/fragments/Core_HT_TRENDMODE.java) |
| C# | [`Core_HT_TRENDMODE.cs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/csharp/library/src/Core_HT_TRENDMODE.cs) |
| TypeScript | [`htTrendmode.ts`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/ts/library/src/func/htTrendmode.ts) |

Python: `talib.HT_TRENDMODE`, see [ta-lib-python](https://ta-lib.github.io/ta-lib-python/func_groups/cycle_indicators.html).

TA-Lib is also available for R and more using a [wrapper](/install/#wrappers).

## Aliases

Hilbert Transform Trend vs Cycle Mode, Trend Mode

## See Also

[HT_TRENDLINE](/functions/ht_trendline.md) · [HT_SINE](/functions/ht_sine.md) · [HT_DCPHASE](/functions/ht_dcphase.md) · [HT_DCPERIOD](/functions/ht_dcperiod.md) · [MAMA](/functions/mama.md)

## References

- John F. Ehlers, *Rocket Science for Traders: Digital Signal Processing Applications*, John Wiley & Sons (ISBN 0471405671)
