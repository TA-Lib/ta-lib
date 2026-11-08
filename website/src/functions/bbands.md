---
title: "Bollinger Bands (BBANDS)"
description: "Bollinger Bands: a moving-average middle band with upper and lower bands offset by a multiple of the standard deviation."
---

## Summary

Bollinger Bands: a moving-average middle band with upper and lower bands offset by a multiple of the standard deviation. Used to gauge relative price volatility.

## Chart

<ta-chart src="/functions/bbands.svg" ratio="720:274" label="BBANDS over IBM, daily, 1999" alt="Drawn on the price pane over the close as a line, with outRealMiddleBand as a solid line and a band between outRealUpperBand and outRealLowerBand with a soft fill."></ta-chart>

Drawn on the price pane over the close as a line, with outRealMiddleBand as a solid line and a band between outRealUpperBand and outRealLowerBand with a soft fill. [Reference data (CSV)](/functions/bbands.csv) · [Chart (SVG)](/functions/bbands.svg)

## Formula

$$
\begin{aligned}
\text{middle}_t &= \operatorname{MA}(X, n, \text{matype})_t \\
\sigma_t &= \operatorname{STDDEV}(X, n)_t \\
\text{upper}_t &= \text{middle}_t + k_{\text{up}}\,\sigma_t \\
\text{lower}_t &= \text{middle}_t - k_{\text{dn}}\,\sigma_t
\end{aligned}
$$

where $X$ is the input series, $n$ the period, $\text{matype}$ the moving-average type,
and $k_{\text{up}}$, $k_{\text{dn}}$ the upper and lower deviation multipliers.

## Notes

- The defaults reproduce Bollinger's original definition: a 20-period SMA middle band with
  $k_{\text{up}} = k_{\text{dn}} = 2$. Any other $\text{matype}$ is a TA-Lib generalisation.
- $\text{matype}$ sets where the envelope is centred; $n$ and $k$ set how wide it is. The two are
  independent — $\sigma$ depends only on the price window, so changing the middle band re-centres
  the bands without resizing them.

## Inputs

- `inReal` — Input data series

## Outputs

- `outRealUpperBand` — Middle band plus nbDevUp standard deviations
- `outRealMiddleBand` — The moving average
- `outRealLowerBand` — Middle band minus nbDevDn standard deviations

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
| <span class="flag-box">✅</span> **Overlap Input** <span class="flag-tip" tabindex="0" role="note" aria-label="Output is on the same scale as the input price, so it is drawn over the price chart." data-tip="Output is on the same scale as the input price, so it is drawn over the price chart.">i</span> |
| <span class="flag-box">☐</span> <span style="opacity:0.5">Independent Y-Axis</span> |
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
double upperBand[252], middleBand[252], lowerBand[252];
int outBegIdx, outNBElement;

TA_Initialize();
TA_BBANDS( 0, n - 1, series, 20, 2.0, 2.0, TA_MAType_SMA,
           &outBegIdx, &outNBElement, upperBand, middleBand, lowerBand );
printf( "%d values from bar %d; last upperBand = %f\n", outNBElement, outBegIdx, upperBand[outNBElement - 1] );
```

@tab Rust

```rust
use ta_lib::{Core, MAType};

let series = vec![0.0; 252]; // your data, oldest first
let mut upper_band = vec![0.0; 252];
let mut middle_band = vec![0.0; 252];
let mut lower_band = vec![0.0; 252];

let core = Core::new();
let range = core.bbands(0, series.len() - 1, &series, 20, 2.0, 2.0, MAType::SMA,
                        &mut upper_band, &mut middle_band, &mut lower_band)?;
println!("{} values from bar {}; last upper_band = {}", range.count, range.beg_idx, upper_band[range.count - 1]);
```

@tab Java

```java
import io.github.talib.Core;
import io.github.talib.MAType;
import io.github.talib.OutRange;

double[] series = new double[252]; // your data, oldest first
double[] upperBand = new double[252];
double[] middleBand = new double[252];
double[] lowerBand = new double[252];

Core core = Core.DEFAULT;
OutRange r = core.bbands(0, series.length - 1, series, 20, 2.0, 2.0, MAType.SMA,
                         upperBand, middleBand, lowerBand);
System.out.println(r.count() + " values from bar " + r.begIdx() + "; last upperBand = " + upperBand[r.count() - 1]);
```

@tab C\#

```csharp
using System;
using TALib;

var series = new double[252]; // your data, oldest first
var upperBand = new double[252];
var middleBand = new double[252];
var lowerBand = new double[252];

var core = Core.Default;
OutRange r = core.Bbands(0, series.Length - 1, series, 20, 2.0, 2.0, MAType.SMA,
                         upperBand, middleBand, lowerBand);
Console.WriteLine($"{r.Count} values from bar {r.BegIdx}; last upperBand = {upperBand[r.Count - 1]}");
```

@tab TypeScript

```ts
import { MAType, bbands } from 'ta-lib';

const series = new Float64Array(252); // your data, oldest first

const {
  begIdx,
  outUpperBand,
  outMiddleBand,
  outLowerBand,
} = bbands(series, {
  timePeriod: 20,
  nbDevUp: 2,
  nbDevDn: 2,
  maType: MAType.SMA,
});
console.log(outUpperBand.length + ' values from bar ' + begIdx + '; last outUpperBand = ' + outUpperBand[outUpperBand.length - 1]);
```

:::

## Implementation

Added in 0.4.0.

TA-Lib Definition: [`bbands.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/bbands/bbands.c) · [`bbands.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/bbands/bbands.yaml)

| Native | File |
|--------|------|
| C | [`ta_BBANDS.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_BBANDS.c) |
| Rust | [`bbands.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/library/src/ta_func/bbands.rs) |
| Java | [`Core_BBANDS.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/fragments/Core_BBANDS.java) |
| C# | [`Core_BBANDS.cs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/csharp/library/src/Core_BBANDS.cs) |
| TypeScript | [`bbands.ts`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/ts/library/src/func/bbands.ts) |

Python: `talib.BBANDS`, see [ta-lib-python](https://ta-lib.github.io/ta-lib-python/func_groups/overlap_studies.html).

TA-Lib is also available for R and more using a [wrapper](/install/#wrappers).

## Aliases

Bollinger Bands

## See Also

[MA](/functions/ma.md) · [STDDEV](/functions/stddev.md) · [SMA](/functions/sma.md)

## References

- John A. Bollinger, *Bollinger on Bollinger Bands*, McGraw-Hill Trade (ISBN 0071373683)
