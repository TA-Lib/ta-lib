---
title: "Bollinger Bands %B (PERCENTB)"
description: "Bollinger Bands %B: where the input sits relative to its Bollinger Bands, 0 at the lower band and 1 at the upper band."
---

## Summary

Bollinger Bands %B: where the input sits relative to its Bollinger Bands, 0 at the lower band and 1 at the upper band. Values below 0 or above 1 mean the input is outside the bands.

## Chart

<ta-chart src="/functions/percentb.svg" ratio="720:416" label="PERCENTB over IBM, daily, 1999" alt="Drawn on the price pane over the close as a line; below it, a separate indicator pane with a zero line showing outReal as a solid line."></ta-chart>

::: details Chart metadata <span class="flag-tip" tabindex="0" role="note" aria-label="The chart is drawn from the function's metadata alone: the flags the abstract API reports for it and for each output." data-tip="The chart is drawn from the function's metadata alone: the flags the abstract API reports for it and for each output.">i</span>

Drawn on the price pane over the close as a line; below it, a separate indicator pane with a zero line showing outReal as a solid line.

Metadata flags used: `outReal: line`

:::

[Reference data (CSV)](/functions/percentb.csv) · [Chart (SVG)](/functions/percentb.svg)

## Formula

$$
\%B_t = \frac{X_t - \mathrm{lower}_t}{\mathrm{upper}_t - \mathrm{lower}_t}, \text{ or } 0.5 \text{ when } \mathrm{upper}_t = \mathrm{lower}_t
$$

where $\mathrm{upper}_t$ and $\mathrm{lower}_t$ are the [`BBANDS`](/functions/bbands.md) bands of $X$ with period $n$, moving-average type $\text{matype}$ and deviation multipliers $k_{\text{up}}$, $k_{\text{dn}}$.

## Notes

- An input on the middle band reads `optInNbDevDn / (optInNbDevUp + optInNbDevDn)`: 0.5 with equal multipliers.
- With a simple moving average and both multipliers equal to k, %B = 0.5 + z / (2k), where z is the z-score of the input in its window.
- The result is a ratio; multiply by 100 to read it as a percentage.
- Any `optInMAType` other than SMA is a TA-Lib generalisation, as it is for BBANDS: the deviation stays the population standard deviation about the simple mean.
- PERCENTB is bit for bit `(inReal - lower) / (upper - lower)` computed from BBANDS' own outputs, and 0.5 wherever those two bands are equal.

## Inputs

- `inReal` — Input data series

## Outputs

- `outReal` — Position of the input between the lower band (0) and the upper band (1)

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
TA_PERCENTB( 0, n - 1, series, 20, 2.0, 2.0, TA_MAType_SMA, &outBegIdx, &outNBElement, out );
printf( "%d values from bar %d; last out = %f\n", outNBElement, outBegIdx, out[outNBElement - 1] );
```

@tab Rust

```rust
use ta_lib::{Core, MAType};

let series = vec![0.0; 252]; // your data, oldest first
let mut out = vec![0.0; 252];

let core = Core::new();
let range = core.percentb(0, series.len() - 1, &series, 20, 2.0, 2.0, MAType::SMA,
                          &mut out)?;
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
OutRange r = core.percentb(0, series.length - 1, series, 20, 2.0, 2.0, MAType.SMA, out);
System.out.println(r.count() + " values from bar " + r.begIdx() + "; last out = " + out[r.count() - 1]);
```

@tab C\#

```csharp
using System;
using TALib;

var series = new double[252]; // your data, oldest first
var outReal = new double[252];

var core = Core.Default;
OutRange r = core.Percentb(0, series.Length - 1, series, 20, 2.0, 2.0, MAType.SMA, outReal);
Console.WriteLine($"{r.Count} values from bar {r.BegIdx}; last outReal = {outReal[r.Count - 1]}");
```

@tab TypeScript

```ts
import { MAType, percentb } from 'ta-lib';

const series = new Float64Array(252); // your data, oldest first

const { begIdx, values } = percentb(series, {
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

TA-Lib Definition: [`percentb.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/percentb/percentb.c) · [`percentb.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/percentb/percentb.yaml)

| Native | File |
|--------|------|
| C | [`ta_PERCENTB.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_PERCENTB.c) |
| Rust | [`percentb.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/library/src/ta_func/percentb.rs) |
| Java | [`Core_PERCENTB.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/fragments/Core_PERCENTB.java) |
| C# | [`Core_PERCENTB.cs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/csharp/library/src/Core_PERCENTB.cs) |
| TypeScript | [`percentb.ts`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/ts/library/src/func/percentb.ts) |

Python: `talib.PERCENTB` (ta-lib-python 0.8.3 or later), see [ta-lib-python](https://ta-lib.github.io/ta-lib-python/func_groups/volatility_indicators.html).

TA-Lib is also available for R and more using a [wrapper](/install/#wrappers).

## Aliases

Bollinger %b, %b, %B, Percent B, BB %B, BBP, Bollinger Bands %B

## See Also

[BBANDS](/functions/bbands.md) · [BBW](/functions/bbw.md) · [STOCHF](/functions/stochf.md)

## References

- John A. Bollinger, *Bollinger on Bollinger Bands*, McGraw-Hill 2001 (ISBN 0071373683)
- John Bollinger, [Bollinger Band Rules](https://www.bollingerbands.com/bollinger-band-rules), rules 15-17
