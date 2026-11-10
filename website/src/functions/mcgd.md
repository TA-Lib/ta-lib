---
title: "McGinley Dynamic (MCGD)"
description: "McGinley Dynamic (John R. McGinley, Jr.): a moving average whose speed adjusts to the market."
---

## Summary

McGinley Dynamic (John R. McGinley, Jr.): a moving average whose speed adjusts to the market. Each bar closes a fraction `1 / (N * (x/MD)^4)` of the gap to the price: less than `RMA`'s `1/N` while the price is above the line, more while it is below, so the line tracks falling prices faster than rising ones.

McGinley suggests a period of about 60% of the simple moving average being emulated: a Dynamic of 12 to follow a 20-bar average.

## Chart

<ta-chart src="/functions/mcgd.svg" ratio="720:274" label="MCGD over IBM, daily, 1999" alt="Drawn on the price pane over the close as a line, with outReal as a solid line."></ta-chart>

::: details Chart metadata <span class="flag-tip" tabindex="0" role="note" aria-label="The chart is drawn automatically from metadata TA-Lib provides. Your app can get the same through the Abstract API." data-tip="The chart is drawn automatically from metadata TA-Lib provides. Your app can get the same through the Abstract API.">i</span>

Drawn on the price pane over the close as a line, with outReal as a solid line.

Metadata flags used: `overlap` · `outReal: line`

:::

[Reference data (CSV)](/functions/mcgd.csv) · [Chart (SVG)](/functions/mcgd.svg)

## Formula

N = optInTimePeriod

MD[0] = x[0]

for i >= 1:  MD[i] = MD[i-1] + ( x[i] - MD[i-1] ) / ( N * ( x[i] / MD[i-1] )^4 )

The first output is at bar N-1.

## Notes

- Where the price is 0, or so small against the line that the step overflows, the step is undefined and the line keeps its previous value. A line at 0 stays at 0.
- Being recursive, an output depends on how much history precedes it. Close to the price the seed's influence decays by a factor of `1 - 1/N` per bar, as in `RMA`; the unstable period is how much of the warm-up to discard.
- The line is scale-equivariant but meant for positive prices: a series crossing zero sends it off to meaningless values, and a single bar far enough below the line can take the line to 0 or below it.
- Some implementations seed with the simple average of the first N bars. They agree with this one only once the seed's influence has decayed.
- Some implementations write the step's denominator as `0.6 * P * (x / MD)^4`. That is this function at a period of `0.6 * P`, when that is an integer.

## Inputs

- `inReal` — Data on which to compute the average

## Outputs

- `outReal` — McGinley Dynamic line

## Parameters

| Parameter | Type | Default | Accepted values | Description |
| --- | --- | --- | --- | --- |
| `optInTimePeriod` | integer | 14 | 2–100000 | The N of the step's denominator |

## Properties

**Numerical Stability:** [Initial Unstable Period](/functions/stability.md#initial-unstable-period)

<div class="flag-table">

|  |
| :-- |
| <span class="flag-box">✅</span> **Overlap Input** <span class="flag-tip" tabindex="0" role="note" aria-label="Output is on the same scale as the input price, so it is drawn over the price chart." data-tip="Output is on the same scale as the input price, so it is drawn over the price chart.">i</span> |
| <span class="flag-box">☐</span> <span style="opacity:0.5">Independent Y-Axis</span> |
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
TA_MCGD( 0, n - 1, series, 14, &outBegIdx, &outNBElement, out );
printf( "%d values from bar %d; last out = %f\n", outNBElement, outBegIdx, out[outNBElement - 1] );
```

@tab Rust

```rust
use ta_lib::Core;

let series = vec![0.0; 252]; // your data, oldest first
let mut out = vec![0.0; 252];

let core = Core::new();
let range = core.mcgd(0, series.len() - 1, &series, 14, &mut out)?;
println!("{} values from bar {}; last out = {}", range.count, range.beg_idx, out[range.count - 1]);
```

@tab Java

```java
import io.github.talib.Core;
import io.github.talib.OutRange;

double[] series = new double[252]; // your data, oldest first
double[] out = new double[252];

Core core = Core.DEFAULT;
OutRange r = core.mcgd(0, series.length - 1, series, 14, out);
System.out.println(r.count() + " values from bar " + r.begIdx() + "; last out = " + out[r.count() - 1]);
```

@tab C\#

```csharp
using System;
using TALib;

var series = new double[252]; // your data, oldest first
var outReal = new double[252];

var core = Core.Default;
OutRange r = core.Mcgd(0, series.Length - 1, series, 14, outReal);
Console.WriteLine($"{r.Count} values from bar {r.BegIdx}; last outReal = {outReal[r.Count - 1]}");
```

@tab TypeScript

```ts
import { mcgd } from 'ta-lib';

const series = new Float64Array(252); // your data, oldest first

const { begIdx, values } = mcgd(series, { timePeriod: 14 });
console.log(values.length + ' values from bar ' + begIdx + '; last values = ' + values[values.length - 1]);
```

:::

## Implementation

Added in 0.8.3 (ta-lib-python 0.8.3).

TA-Lib Definition: [`mcgd.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/mcgd/mcgd.c) · [`mcgd.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/mcgd/mcgd.yaml)

| Native | File |
|--------|------|
| C | [`ta_MCGD.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_MCGD.c) |
| Rust | [`mcgd.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/library/src/ta_func/mcgd.rs) |
| Java | [`Core_MCGD.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/fragments/Core_MCGD.java) |
| C# | [`Core_MCGD.cs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/csharp/library/src/Core_MCGD.cs) |
| TypeScript | [`mcgd.ts`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/ts/library/src/func/mcgd.ts) |

Python: `talib.MCGD` (ta-lib-python 0.8.3 or later), see [ta-lib-python](https://ta-lib.github.io/ta-lib-python/func_groups/overlap_studies.html).

TA-Lib is also available for R and more using a [wrapper](/install/#wrappers).

## Aliases

McGinley Dynamic, MD, MGD

## See Also

[RMA](/functions/rma.md) · [EMA](/functions/ema.md) · [KAMA](/functions/kama.md) · [SMA](/functions/sma.md)

## References

- **John R. McGinley, Jr., "McGinley Dynamics", MTA Journal (Market Technicians Association), Summer-Fall 1997, pp. 15-18.** The original definition, with the fourth-power adjustment and the 60% rule for choosing N.
