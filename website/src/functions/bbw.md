---
title: "Bollinger BandWidth (BBW)"
description: "Bollinger BandWidth: the distance between the upper and lower Bollinger Bands as a percentage of the middle band."
---

## Summary

Bollinger BandWidth: the distance between the upper and lower Bollinger Bands as a percentage of the middle band. Low values mark contracting volatility, the setup John Bollinger calls the Squeeze; high values mark expanding volatility.

## Chart

<ta-chart src="/functions/bbw.svg" ratio="720:416" label="BBW over IBM, daily, 1999" alt="Drawn on the price pane over the close as a line; below it, a separate indicator pane with a zero line showing outReal as a solid line."></ta-chart>

::: details Chart metadata <span class="flag-tip" tabindex="0" role="note" aria-label="The chart is drawn from the function's metadata: the flags and attributes the abstract API reports for it and for each output, and the input series its reference run declares." data-tip="The chart is drawn from the function's metadata: the flags and attributes the abstract API reports for it and for each output, and the input series its reference run declares.">i</span>

Drawn on the price pane over the close as a line; below it, a separate indicator pane with a zero line showing outReal as a solid line.

Metadata flags used: `outReal: line, positive, negative`

:::

[Reference data (CSV)](/functions/bbw.csv) · [Chart (SVG)](/functions/bbw.svg)

## Formula

$$
\mathrm{BBW}_t = 100 \cdot \frac{\mathrm{upper}_t - \mathrm{lower}_t}{\mathrm{middle}_t}, \text{ or } 0 \text{ when } \mathrm{middle}_t = 0
$$

where $\mathrm{upper}_t$, $\mathrm{middle}_t$ and $\mathrm{lower}_t$ are the [`BBANDS`](/functions/bbands.md) bands of $X$ with period $n$, moving-average type $\text{matype}$ and deviation multipliers $k_{\text{up}}$, $k_{\text{dn}}$. Since both bands sit on the middle band, $\mathrm{BBW}_t = 100\,(k_{\text{up}} + k_{\text{dn}})\,\sigma_t / \mathrm{middle}_t$, with $\sigma_t$ the population standard deviation of the last $n$ values of $X$.

## Notes

- **The input is assumed positive.** The middle band is the divisor. Where it is exactly 0 the output is 0. Near zero the divisor is small and the output very large; no error is returned. A series that reaches or crosses zero, such as an oscillator or a spread, has no meaningful result here.
- With Bollinger's settings (a simple moving average and two deviations on each side) BBW is 400 times the window's coefficient of variation: its standard deviation divided by its mean.
- The two deviation multipliers enter only through their sum.
- The result is in percent: 10 means the bands are 10% of the middle band apart.
- Any `optInMAType` other than SMA is a TA-Lib generalisation, as it is for BBANDS: the deviation stays the population standard deviation about the simple mean.
- Wherever the middle band is not 0, BBW is bit for bit `((upper - lower) / middle) * 100` computed from BBANDS' own outputs.

## Inputs

- `inReal` — Input data series

## Outputs

- `outReal` — Width of the bands as a percentage of the middle band

## Parameters

| Parameter | Type | Default | Accepted values | Description |
| --- | --- | --- | --- | --- |
| `optInTimePeriod` | integer | 20 | 2–100000 | Periods for the MA and standard deviation |
| `optInNbDevUp` | real | 2 | any real | Standard-deviation multiplier for the upper band |
| `optInNbDevDn` | real | 2 | any real | Standard-deviation multiplier for the lower band |
| `optInMAType` | MAType | SMA (0) | any MAType | Moving-average type for the middle band |

*`MAType` values: 0 SMA · 1 EMA · 2 WMA · 3 DEMA · 4 TEMA · 5 TRIMA · 6 KAMA · 7 MAMA · 8 T3 · 9 HMA · 10 DISABLED · 11 DEFAULT · 12 ZLEMA · 13 RMA · 14 VIDYA · 15 ALMA*

## Properties

**Numerical Stability:** [Depends on MA Type](/functions/stability.md#depends-on-ma-type) — This function's default, SMA, is start-independent.

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
TA_BBW( 0, n - 1, series, 20, 2.0, 2.0, TA_MAType_SMA, &outBegIdx, &outNBElement, out );
printf( "%d values from bar %d; last out = %f\n", outNBElement, outBegIdx, out[outNBElement - 1] );
```

@tab Rust

```rust
use ta_lib::{Core, MAType};

let series = vec![0.0; 252]; // your data, oldest first
let mut out = vec![0.0; 252];

let core = Core::new();
let range = core.bbw(0, series.len() - 1, &series, 20, 2.0, 2.0, MAType::SMA, &mut out)?;
println!("{} values from bar {}; last out = {}", range.count, range.beg_idx, out[range.count - 1]);
```

@tab Java

```java
import io.github.talib.Core;
import io.github.talib.MAType;
import io.github.talib.OutRange;

double[] series = new double[252]; // your data, oldest first
double[] out = new double[252];

Core core = Core.DEFAULT;
OutRange r = core.bbw(0, series.length - 1, series, 20, 2.0, 2.0, MAType.SMA, out);
System.out.println(r.count() + " values from bar " + r.begIdx() + "; last out = " + out[r.count() - 1]);
```

@tab C\#

```csharp
using System;
using TALib;

var series = new double[252]; // your data, oldest first
var outReal = new double[252];

var core = Core.Default;
OutRange r = core.Bbw(0, series.Length - 1, series, 20, 2.0, 2.0, MAType.SMA, outReal);
Console.WriteLine($"{r.Count} values from bar {r.BegIdx}; last outReal = {outReal[r.Count - 1]}");
```

@tab TypeScript

```ts
import { MAType, bbw } from 'ta-lib';

const series = new Float64Array(252); // your data, oldest first

const { begIdx, values } = bbw(series, {
  timePeriod: 20,
  nbDevUp: 2,
  nbDevDn: 2,
  maType: MAType.SMA,
});
console.log(values.length + ' values from bar ' + begIdx + '; last values = ' + values[values.length - 1]);
```

:::

## Implementation

Added in 0.8.3 (ta-lib-python 0.8.3).

TA-Lib Definition: [`bbw.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/bbw/bbw.c) · [`bbw.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/bbw/bbw.yaml)

| Native | File |
|--------|------|
| C | [`ta_BBW.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_BBW.c) |
| Rust | [`bbw.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/library/src/ta_func/bbw.rs) |
| Java | [`Core_BBW.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/fragments/Core_BBW.java) |
| C# | [`Core_BBW.cs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/csharp/library/src/Core_BBW.cs) |
| TypeScript | [`bbw.ts`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/ts/library/src/func/bbw.ts) |

Python: `talib.BBW` (ta-lib-python 0.8.3 or later), see [ta-lib-python](https://ta-lib.github.io/ta-lib-python/func_groups/volatility_indicators.html).

TA-Lib is also available for R and more using a [wrapper](/install/#wrappers).

## Aliases

Bollinger BandWidth, Bollinger Band Width, Bollinger Bands Width, BandWidth

## See Also

[BBANDS](/functions/bbands.md) · [PERCENTB](/functions/percentb.md) · [STDDEV](/functions/stddev.md) · [NATR](/functions/natr.md)

## References

- John A. Bollinger, *Bollinger on Bollinger Bands*, McGraw-Hill 2001 (ISBN 0071373683)
- John Bollinger, [Bollinger Band Rules](https://www.bollingerbands.com/bollinger-band-rules), rule 18
