---
url: 'https://ta-lib.org/functions/swak_bp.md'
description: >-
  The band-pass row of John Ehlers' Swiss Army Knife filter: a cycle extractor
  that keeps a band of periods around a chosen centre and removes everything…
---
# Swiss Army Knife - Band-Pass Filter (SWAK_BP)

## Summary

The band-pass row of John Ehlers' Swiss Army Knife filter: a cycle extractor that keeps a band of periods around a chosen centre and removes everything on both sides of it.

Read it as an oscillator, not as price. It answers zero on a constant, zero on a bar-to-bar alternation, and exactly the input, with the same amplitude and no phase shift, on a sine wave at the centre period. Between those it tapers, with the half-power points near `P(1 ± delta)`: at a centre of 20 bars and a delta of 0.1 the band is roughly 20 ± 2 bars.

What separates it from the high-pass rows is that it rejects the fast end too. A detrender keeps everything above its cutoff, including the bar-to-bar noise; this keeps only the band asked for.

## Chart

<ta-chart src="/functions/swak_bp.svg" ratio="720:416" label="SWAK_BP over IBM, daily, 1999" alt="Drawn on the price pane over the close as a line; below it, a separate indicator pane with a zero line showing outReal as a solid line."></ta-chart>

::: details Chart metadata <span class="flag-tip" tabindex="0" role="note" aria-label="The chart is drawn from the function's metadata: the flags and attributes the abstract API reports for it and for each output, and the input series its reference run declares." data-tip="The chart is drawn from the function's metadata: the flags and attributes the abstract API reports for it and for each output, and the input series its reference run declares.">i</span>

Drawn on the price pane over the close as a line; below it, a separate indicator pane with a zero line showing outReal as a solid line.

Metadata flags used: `outReal: line, positive, negative`

:::

[Reference data (CSV)](/functions/swak_bp.csv) · [Chart (SVG)](/functions/swak_bp.svg)

## Formula

```
w    = 2 * pi / optInTimePeriod
beta = cos(w)
t    = 4 * pi * optInDelta / optInTimePeriod
abp  = (1 - sin(t)) / cos(t)

c0   = (1 - abp) / 2
a1   = beta * (1 + abp)
a2   = -abp

y[i] = c0 * (x[i] - x[i-2]) + a1 * y[i-1] + a2 * y[i-2]
```

The filter is seeded in the steady state of a constant input equal to its first bar: both input slots start at that bar and both output slots at zero, since a band-pass of a constant is zero.

## Notes

`abp` is written as `(1 - sin t) / cos t` rather than the published `gamma - sqrt(gamma^2 - 1)` with `gamma = 1 / cos t`. The two are equal for `0 < t < pi/2`, which this function's ranges guarantee, but the published form loses precision as `t` approaches zero because `gamma^2 - 1` goes as `t^2`. Measured against a 60-digit reference, the published form is already 4.5e-13 off at the period cap, which is what would otherwise force a lower cap.

The period range starts at 5 so that `delta <= 0.5` keeps `t` below `pi/2`, where both forms agree and `abp` stays in `(0, 1)`. Below that the published form can return a value that makes the recurrence unstable.

Every term of the recurrence exists at the first bar, so there is no structural lookback. What the first bars carry is the seed, which decays rather than ending: set `TA_FUNC_UNST_SWAK_BP` to discard bars until that transient is below whatever matters for the caller. This row settles more slowly than the others at the same period: its poles sit near the unit circle, which is what makes the band narrow.

The output may alias the input. This row reads `x[i-2]`, which an aliased write would already have overwritten, so the input slots are carried in locals and never re-read from the input array.

## Inputs

* `inReal` — The series to filter; Ehlers' default is the bar midpoint `(H+L)/2`, which the caller passes as `TA_MEDPRICE` output

## Outputs

* `outReal` — The extracted cycle, centred on zero

## Parameters

| Parameter | Type | Default | Accepted values | Description |
| --- | --- | --- | --- | --- |
| `optInTimePeriod` | integer | 20 | 5–2000 | Centre period of the band; the filter passes this one untouched |
| `optInDelta` | real | 0.1 | 0.05–0.5 | Half-bandwidth as a fraction of the centre period; smaller is a narrower band and a longer settling transient |

## Properties

**Numerical Stability:** [Initial Unstable Period](/functions/stability.md#initial-unstable-period)

<div class="flag-table">

|  |
| :-- |
| <span class="flag-box">☐</span> <span style="opacity:0.5">Overlap Input</span> |
| <span class="flag-box">✅</span> **Independent Y-Axis** <span class="flag-tip" tabindex="0" role="note" aria-label="Output is on its own scale, drawn in a separate pane below the price chart." data-tip="Output is on its own scale, drawn in a separate pane below the price chart.">i</span> |
| <span class="flag-box">☐</span> <span style="opacity:0.5">Candlestick</span> |
| <span class="flag-box">☐</span> <span style="opacity:0.5">Can Output NaN or ±Inf</span> |
| <span class="flag-box">☐</span> <span style="opacity:0.5">Identity at Period 1</span> |
| <span class="flag-box">☐</span> <span style="opacity:0.5">Display Shift</span> |
| <span class="flag-box">✅</span> **Uses Transcendental** <span class="flag-tip" tabindex="0" role="note" aria-label="Calls a transcendental math function (exp, log, log10, trigonometric, inverse trigonometric or hyperbolic), so a value may differ slightly between languages, between a stream and a batch call, and between machines. TA-Lib targets a difference below 1e-9." data-tip="Calls a transcendental math function (exp, log, log10, trigonometric, inverse trigonometric or hyperbolic), so a value may differ slightly between languages, between a stream and a batch call, and between machines. TA-Lib targets a difference below 1e-9.">i</span> |
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
TA_SWAK_BP( 0, n - 1, series, 20, 0.1, &outBegIdx, &outNBElement, out );
printf( "%d values from bar %d; last out = %f\n", outNBElement, outBegIdx, out[outNBElement - 1] );
```

@tab Rust

```rust
use ta_lib::Core;

let series = vec![0.0; 252]; // your data, oldest first
let mut out = vec![0.0; 252];

let core = Core::new();
let range = core.swak_bp(0, series.len() - 1, &series, 20, 0.1, &mut out)?;
println!("{} values from bar {}; last out = {}", range.count, range.beg_idx, out[range.count - 1]);
```

@tab Java

```java
import io.github.talib.Core;
import io.github.talib.OutRange;

double[] series = new double[252]; // your data, oldest first
double[] out = new double[252];

Core core = Core.DEFAULT;
OutRange r = core.swakBp(0, series.length - 1, series, 20, 0.1, out);
System.out.println(r.count() + " values from bar " + r.begIdx() + "; last out = " + out[r.count() - 1]);
```

@tab C#

```csharp
using System;
using TALib;

var series = new double[252]; // your data, oldest first
var outReal = new double[252];

var core = Core.Default;
OutRange r = core.SwakBp(0, series.Length - 1, series, 20, 0.1, outReal);
Console.WriteLine($"{r.Count} values from bar {r.BegIdx}; last outReal = {outReal[r.Count - 1]}");
```

@tab TypeScript

```ts
import { swakBp } from 'ta-lib';

const series = new Float64Array(252); // your data, oldest first

const { begIdx, values } = swakBp(series, { timePeriod: 20, delta: 0.1 });
console.log(values.length + ' values from bar ' + begIdx + '; last values = ' + values[values.length - 1]);
```

:::

## Implementation

Added in 0.8.3 (ta-lib-python 0.8.3).

TA-Lib Definition: [`swak_bp.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/swak_bp/swak_bp.c) · [`swak_bp.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/swak_bp/swak_bp.yaml)

| Native | File |
|--------|------|
| C | [`ta_SWAK_BP.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_SWAK_BP.c) |
| Rust | [`swak_bp.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/library/src/ta_func/swak_bp.rs) |
| Java | [`Core_SWAK_BP.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/fragments/Core_SWAK_BP.java) |
| C# | [`Core_SWAK_BP.cs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/csharp/library/src/Core_SWAK_BP.cs) |
| TypeScript | [`swakBp.ts`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/ts/library/src/func/swakBp.ts) |

Python: `talib.SWAK_BP` (ta-lib-python 0.8.3 or later), see [ta-lib-python](https://ta-lib.github.io/ta-lib-python/func_groups/cycle_indicators.html).

TA-Lib is also available for R and more using a [wrapper](/install/#wrappers).

## Aliases

Swiss Army Knife Band-Pass Filter, SWAK BP, Ehlers Band-Pass Filter

## See Also

[SWAK_HP](/functions/swak_hp.md) · [SWAK_2PHP](/functions/swak_2php.md) · [MEDPRICE](/functions/medprice.md)

## References

* Ehlers, John F. "Swiss Army Knife Indicator." *Technical Analysis of Stocks & Commodities* V.24:1 (January 2006), pp. 28-31, 50-53.
