---
title: "Swiss Army Knife - Butterworth Filter (SWAK_BUTTER)"
description: "The Butterworth row of John Ehlers' Swiss Army Knife filter: the Gaussian row's double real pole with two zeros added at Nyquist, giving a two-pole…"
---

## Summary

The Butterworth row of John Ehlers' Swiss Army Knife filter: the Gaussian row's double real pole with two zeros added at Nyquist, giving a two-pole low-pass that cuts the shortest cycles harder than the Gaussian does while keeping the same pole placement.

Read it as a smoothed price line. Its DC gain is 1, so a flat market returns the price itself and the line sits on the chart with price. The two Nyquist zeros are what separate it from `TA_SWAK_GAUSS`: a bar-to-bar alternation, the fastest motion a sampled series can carry, is removed outright rather than merely attenuated, which is why the numerator weights three consecutive bars 1, 2, 1.

The name keeps the `SWAK` prefix on purpose. This is the Swiss Army Knife's Butterworth row, not the Butterworth filters of Ehlers' separate article, and not the SuperSmoother, which is a third two-pole low-pass with complex poles.

## Chart

<ta-chart src="/functions/swak_butter.svg" ratio="720:274" label="SWAK_BUTTER over IBM, daily, 1999" alt="Drawn on the price pane over the close as a line, with outReal as a solid line."></ta-chart>

<div class="ta-chart-links">

[Reference data (CSV)](/functions/swak_butter.csv) · [Chart (SVG)](/functions/swak_butter.svg)

</div>

::: details Chart metadata <span class="flag-tip" tabindex="0" role="note" aria-label="The chart is drawn automatically from metadata TA-Lib provides. Your app can get the same through the Abstract API." data-tip="The chart is drawn automatically from metadata TA-Lib provides. Your app can get the same through the Abstract API.">i</span>

**Outputs**

- `outReal`: `line`

:::

## Formula

```
w   = 2 * pi / optInTimePeriod
b2p = 2.415 * (1 - cos(w))
a2p = -b2p + sqrt(b2p^2 + 2 * b2p)

c0  = a2p^2 / 4
a1  = 2 * (1 - a2p)
a2  = -(1 - a2p)^2

y[i] = c0 * (x[i] + 2 * x[i-1] + x[i-2]) + a1 * y[i-1] + a2 * y[i-2]
```

The quarter in `c0` offsets the numerator's total weight of 4, which is what holds the DC gain at 1.

The filter is seeded in the steady state of a constant input equal to its first bar: both input slots and both output slots start at that bar's value.

## Notes

Every term of the recurrence exists at the first bar, so there is no structural lookback and nothing is dropped from the front of the request. What the first bars carry is the seed, which decays rather than ending: set `TA_FUNC_UNST_SWAK_BUTTER` to discard bars until that transient is below whatever matters for the caller.

The output may alias the input. This row reads `x[i-1]` and `x[i-2]`, which an aliased write would already have overwritten, so both are carried in locals and never re-read from the input array.

## Inputs

- `inReal` — The series to filter; Ehlers' default is the bar midpoint `(H+L)/2`, which the caller passes as `TA_MEDPRICE` output

## Outputs

- `outReal` — The filtered line, on the same scale as the input

## Parameters

| Parameter | Type | Default | Accepted values | Description |
| --- | --- | --- | --- | --- |
| `optInTimePeriod` | integer | 20 | 2–10000 | Cutoff period; shorter keeps more of the fast motion, longer smooths harder and lags more |

## Properties

**Numerical Stability:** [Initial Unstable Period](/functions/stability.md#initial-unstable-period)

<div class="flag-cols">

<div class="flag-table">

|  |
| :-- |
| <span class="flag-box">✅</span> **Overlap Input** <span class="flag-tip" tabindex="0" role="note" aria-label="Output is on the same scale as the input price, so it is drawn over the price chart." data-tip="Output is on the same scale as the input price, so it is drawn over the price chart.">i</span> |
| <span class="flag-box">☐</span> <span style="opacity:0.5">Independent Y-Axis</span> |
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
double out[252];
int outBegIdx, outNBElement;

TA_Initialize();
TA_SWAK_BUTTER( 0, n - 1, series, 20, &outBegIdx, &outNBElement, out );
printf( "%d values from bar %d; last out = %f\n", outNBElement, outBegIdx, out[outNBElement - 1] );
```

@tab Rust

```rust
use ta_lib::Core;

let series = vec![0.0; 252]; // your data, oldest first
let mut out = vec![0.0; 252];

let core = Core::new();
let range = core.swak_butter(0, series.len() - 1, &series, 20, &mut out)?;
println!("{} values from bar {}; last out = {}", range.count, range.beg_idx, out[range.count - 1]);
```

@tab Java

```java
import io.github.talib.Core;
import io.github.talib.OutRange;

double[] series = new double[252]; // your data, oldest first
double[] out = new double[252];

Core core = Core.DEFAULT;
OutRange r = core.swakButter(0, series.length - 1, series, 20, out);
System.out.println(r.count() + " values from bar " + r.begIdx() + "; last out = " + out[r.count() - 1]);
```

@tab C\#

```csharp
using System;
using TALib;

var series = new double[252]; // your data, oldest first
var outReal = new double[252];

var core = Core.Default;
OutRange r = core.SwakButter(0, series.Length - 1, series, 20, outReal);
Console.WriteLine($"{r.Count} values from bar {r.BegIdx}; last outReal = {outReal[r.Count - 1]}");
```

@tab TypeScript

```ts
import { swakButter } from 'ta-lib';

const series = new Float64Array(252); // your data, oldest first

const { begIdx, values } = swakButter(series, { timePeriod: 20 });
console.log(values.length + ' values from bar ' + begIdx + '; last values = ' + values[values.length - 1]);
```

:::

## Implementation

Added in 0.8.3 (ta-lib-python 0.8.3).

TA-Lib Definition: [`swak_butter.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/swak_butter/swak_butter.c) · [`swak_butter.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/swak_butter/swak_butter.yaml)

| Native | File |
|--------|------|
| C | [`ta_SWAK_BUTTER.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_SWAK_BUTTER.c) |
| Rust | [`swak_butter.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/library/src/ta_func/swak_butter.rs) |
| Java | [`Core_SWAK_BUTTER.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/fragments/Core_SWAK_BUTTER.java) |
| C# | [`Core_SWAK_BUTTER.cs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/csharp/library/src/Core_SWAK_BUTTER.cs) |
| TypeScript | [`swakButter.ts`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/ts/library/src/func/swakButter.ts) |

Python: `talib.SWAK_BUTTER` (ta-lib-python 0.8.3 or later), see [ta-lib-python](https://ta-lib.github.io/ta-lib-python/func_groups/overlap_studies.html).

TA-Lib is also available for R and more using a [wrapper](/install/#wrappers).

## Aliases

Swiss Army Knife Butterworth Filter, SWAK Butter, Ehlers Butterworth Filter

## See Also

[SWAK_GAUSS](/functions/swak_gauss.md) · [EMA](/functions/ema.md) · [MEDPRICE](/functions/medprice.md)

## References

- Ehlers, John F. "Swiss Army Knife Indicator." *Technical Analysis of Stocks & Commodities* V.24:1 (January 2006), pp. 28-31, 50-53.
