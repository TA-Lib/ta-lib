---
title: "Hilbert Transform - SineWave (HT_SINE)"
description: "Hilbert Transform SineWave: derives the dominant-cycle phase from price and emits its sine plus a 45-degree-lead sine."
---

## Summary

Hilbert Transform SineWave: derives the dominant-cycle phase from price and emits its sine plus a 45-degree-lead sine. The two curves cross near cycle turning points. outSine and outLeadSine crossing marks cycle turning points.

## Chart

<ta-chart src="/functions/ht_sine.svg" ratio="720:416" label="HT_SINE over IBM, daily, 1999" alt="Drawn on the price pane over the close as a line; below it, a separate indicator pane with a zero line showing outSine as a solid line and outLeadSine as a dashed line."></ta-chart>

::: details Chart metadata <span class="flag-tip" tabindex="0" role="note" aria-label="The chart is drawn automatically from metadata TA-Lib provides. Your app can get the same through the Abstract API." data-tip="The chart is drawn automatically from metadata TA-Lib provides. Your app can get the same through the Abstract API.">i</span>

Drawn on the price pane over the close as a line; below it, a separate indicator pane with a zero line showing outSine as a solid line and outLeadSine as a dashed line.

Metadata flags used: `outSine: line, positive, negative` · `outLeadSine: dash_line, positive, negative`

:::

[Reference data (CSV)](/functions/ht_sine.csv) · [Chart (SVG)](/functions/ht_sine.svg)

## Inputs

- `inReal` — Source price series

## Outputs

- `outSine` — Sine of the dominant-cycle phase
- `outLeadSine` — Sine of the phase advanced 45 degrees (lead)

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
double sine[252], leadSine[252];
int outBegIdx, outNBElement;

TA_Initialize();
TA_HT_SINE( 0, n - 1, series, &outBegIdx, &outNBElement, sine, leadSine );
printf( "%d values from bar %d; last sine = %f\n", outNBElement, outBegIdx, sine[outNBElement - 1] );
```

@tab Rust

```rust
use ta_lib::Core;

let series = vec![0.0; 252]; // your data, oldest first
let mut sine = vec![0.0; 252];
let mut lead_sine = vec![0.0; 252];

let core = Core::new();
let range = core.ht_sine(0, series.len() - 1, &series, &mut sine, &mut lead_sine)?;
println!("{} values from bar {}; last sine = {}", range.count, range.beg_idx, sine[range.count - 1]);
```

@tab Java

```java
import io.github.talib.Core;
import io.github.talib.OutRange;

double[] series = new double[252]; // your data, oldest first
double[] sine = new double[252];
double[] leadSine = new double[252];

Core core = Core.DEFAULT;
OutRange r = core.htSine(0, series.length - 1, series, sine, leadSine);
System.out.println(r.count() + " values from bar " + r.begIdx() + "; last sine = " + sine[r.count() - 1]);
```

@tab C\#

```csharp
using System;
using TALib;

var series = new double[252]; // your data, oldest first
var sine = new double[252];
var leadSine = new double[252];

var core = Core.Default;
OutRange r = core.HtSine(0, series.Length - 1, series, sine, leadSine);
Console.WriteLine($"{r.Count} values from bar {r.BegIdx}; last sine = {sine[r.Count - 1]}");
```

@tab TypeScript

```ts
import { htSine } from 'ta-lib';

const series = new Float64Array(252); // your data, oldest first

const { begIdx, outSine, outLeadSine } = htSine(series);
console.log(outSine.length + ' values from bar ' + begIdx + '; last outSine = ' + outSine[outSine.length - 1]);
```

:::

## Implementation

Added in 0.4.0.

TA-Lib Definition: [`ht_sine.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/ht_sine/ht_sine.c) · [`ht_sine.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/ht_sine/ht_sine.yaml)

| Native | File |
|--------|------|
| C | [`ta_HT_SINE.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_HT_SINE.c) |
| Rust | [`ht_sine.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/library/src/ta_func/ht_sine.rs) |
| Java | [`Core_HT_SINE.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/fragments/Core_HT_SINE.java) |
| C# | [`Core_HT_SINE.cs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/csharp/library/src/Core_HT_SINE.cs) |
| TypeScript | [`htSine.ts`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/ts/library/src/func/htSine.ts) |

Python: `talib.HT_SINE`, see [ta-lib-python](https://ta-lib.github.io/ta-lib-python/func_groups/cycle_indicators.html).

TA-Lib is also available for R and more using a [wrapper](/install/#wrappers).

## Aliases

Hilbert Transform SineWave, Ehlers SineWave, SineWave Indicator

## See Also

[HT_DCPHASE](/functions/ht_dcphase.md) · [HT_DCPERIOD](/functions/ht_dcperiod.md) · [HT_PHASOR](/functions/ht_phasor.md) · [HT_TRENDMODE](/functions/ht_trendmode.md) · [MAMA](/functions/mama.md)

## References

- John F. Ehlers, *Rocket Science for Traders: Digital Signal Processing Applications*, John Wiley & Sons (ISBN 0471405671)
