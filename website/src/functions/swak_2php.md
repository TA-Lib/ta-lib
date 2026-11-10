---
title: "Swiss Army Knife - Two-Pole High-Pass Filter (SWAK_2PHP)"
description: "The two-pole high-pass row of John Ehlers' Swiss Army Knife filter: the same double real pole the Gaussian and Butterworth rows use, with a detrending…"
---

## Summary

The two-pole high-pass row of John Ehlers' Swiss Army Knife filter: the same double real pole the Gaussian and Butterworth rows use, with a detrending numerator instead of a smoothing one. It removes what is slower than the cutoff period and rolls off twice as steeply as the one-pole row.

Read it as an oscillator, not as price. Its DC gain is 0, so a flat market returns zero and a trend returns its departure from itself. Against `TA_SWAK_HP` the difference is the slope of the transition: the second pole buys a sharper separation between what is kept and what is removed.

## Chart

<ta-chart src="/functions/swak_2php.svg" ratio="720:416" label="SWAK_2PHP over IBM, daily, 1999" alt="Drawn on the price pane over the close as a line; below it, a separate indicator pane with a zero line showing outReal as a solid line."></ta-chart>

<div class="ta-chart-links">

[Reference data (CSV)](/functions/swak_2php.csv) · [Chart (SVG)](/functions/swak_2php.svg)

</div>

::: details Chart metadata <span class="flag-tip" tabindex="0" role="note" aria-label="The chart is drawn automatically from metadata TA-Lib provides. Your app can get the same through the Abstract API." data-tip="The chart is drawn automatically from metadata TA-Lib provides. Your app can get the same through the Abstract API.">i</span>

**Outputs**

- `outReal`: `line` · `zero` · `positive` · `negative`

:::

## Formula

```
w   = 2 * pi / optInTimePeriod
b2p = 2.415 * (1 - cos(w))
a2p = -b2p + sqrt(b2p^2 + 2 * b2p)

c0  = (1 - a2p / 2)^2
a1  = 2 * (1 - a2p)
a2  = -(1 - a2p)^2

y[i] = c0 * (x[i] - 2 * x[i-1] + x[i-2]) + a1 * y[i-1] + a2 * y[i-2]
```

The numerator is zero on a constant, which is what makes the DC gain 0. At Nyquist it weighs 4, and `c0` divides that back to exactly 1.

The filter is seeded in the steady state of a constant input equal to its first bar: both input slots start at that bar and both output slots at zero.

## Notes

Every term of the recurrence exists at the first bar, so there is no structural lookback. What the first bars carry is the seed, which decays rather than ending: set `TA_FUNC_UNST_SWAK_2PHP` to discard bars until that transient is below whatever matters for the caller. The pole is a repeated real root, so the transient decays as `k * (1 - a2p)^k` rather than as a plain geometric.

The output may alias the input. This row reads `x[i-1]` and `x[i-2]`, which an aliased write would already have overwritten, so both are carried in locals and never re-read from the input array.

## Inputs

- `inReal` — The series to detrend; Ehlers' default is the bar midpoint `(H+L)/2`, which the caller passes as `TA_MEDPRICE` output

## Outputs

- `outReal` — The detrended line, centred on zero

## Parameters

| Parameter | Type | Default | Accepted values | Description |
| --- | --- | --- | --- | --- |
| `optInTimePeriod` | integer | 20 | 2–10000 | Cutoff period; cycles longer than it are removed, cycles shorter pass |

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
double out[252];
int outBegIdx, outNBElement;

TA_Initialize();
TA_SWAK_2PHP( 0, n - 1, series, 20, &outBegIdx, &outNBElement, out );
printf( "%d values from bar %d; last out = %f\n", outNBElement, outBegIdx, out[outNBElement - 1] );
```

@tab Rust

```rust
use ta_lib::Core;

let series = vec![0.0; 252]; // your data, oldest first
let mut out = vec![0.0; 252];

let core = Core::new();
let range = core.swak_2php(0, series.len() - 1, &series, 20, &mut out)?;
println!("{} values from bar {}; last out = {}", range.count, range.beg_idx, out[range.count - 1]);
```

@tab Java

```java
import io.github.talib.Core;
import io.github.talib.OutRange;

double[] series = new double[252]; // your data, oldest first
double[] out = new double[252];

Core core = Core.DEFAULT;
OutRange r = core.swak2php(0, series.length - 1, series, 20, out);
System.out.println(r.count() + " values from bar " + r.begIdx() + "; last out = " + out[r.count() - 1]);
```

@tab C\#

```csharp
using System;
using TALib;

var series = new double[252]; // your data, oldest first
var outReal = new double[252];

var core = Core.Default;
OutRange r = core.Swak2php(0, series.Length - 1, series, 20, outReal);
Console.WriteLine($"{r.Count} values from bar {r.BegIdx}; last outReal = {outReal[r.Count - 1]}");
```

@tab TypeScript

```ts
import { swak2php } from 'ta-lib';

const series = new Float64Array(252); // your data, oldest first

const { begIdx, values } = swak2php(series, { timePeriod: 20 });
console.log(values.length + ' values from bar ' + begIdx + '; last values = ' + values[values.length - 1]);
```

:::

## Implementation

Added in 0.8.3 (ta-lib-python 0.8.3).

TA-Lib Definition: [`swak_2php.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/swak_2php/swak_2php.c) · [`swak_2php.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/swak_2php/swak_2php.yaml)

| Native | File |
|--------|------|
| C | [`ta_SWAK_2PHP.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_SWAK_2PHP.c) |
| Rust | [`swak_2php.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/library/src/ta_func/swak_2php.rs) |
| Java | [`Core_SWAK_2PHP.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/fragments/Core_SWAK_2PHP.java) |
| C# | [`Core_SWAK_2PHP.cs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/csharp/library/src/Core_SWAK_2PHP.cs) |
| TypeScript | [`swak2php.ts`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/ts/library/src/func/swak2php.ts) |

Python: `talib.SWAK_2PHP` (ta-lib-python 0.8.3 or later), see [ta-lib-python](https://ta-lib.github.io/ta-lib-python/func_groups/cycle_indicators.html).

TA-Lib is also available for R and more using a [wrapper](/install/#wrappers).

## Aliases

Swiss Army Knife Two-Pole High-Pass Filter, SWAK 2PHP, Ehlers Two-Pole High-Pass Filter

## See Also

[SWAK_HP](/functions/swak_hp.md) · [SWAK_BP](/functions/swak_bp.md) · [MEDPRICE](/functions/medprice.md)

## References

- Ehlers, John F. "Swiss Army Knife Indicator." *Technical Analysis of Stocks & Commodities* V.24:1 (January 2006), pp. 28-31, 50-53.
