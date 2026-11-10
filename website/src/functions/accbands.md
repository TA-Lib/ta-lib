---
title: "Acceleration Bands (ACCBANDS)"
description: "Acceleration Bands: three overlap lines around price."
---

## Summary

Acceleration Bands: three overlap lines around price. The middle band is an SMA of the close; the upper/lower bands are SMAs of the high/low scaled by an intraday-range factor.

## Chart

<ta-chart src="/functions/accbands.svg" ratio="720:274" label="ACCBANDS over IBM, daily, 1999" alt="Drawn on the price pane over candles, with outRealMiddleBand as a solid line and a band between outRealUpperBand and outRealLowerBand with a soft fill."></ta-chart>

<div class="ta-chart-links">

[Reference data (CSV)](/functions/accbands.csv) · [Chart (SVG)](/functions/accbands.svg)

</div>

::: details Chart metadata <span class="flag-tip" tabindex="0" role="note" aria-label="The chart is drawn automatically from metadata TA-Lib provides. Your app can get the same through the Abstract API." data-tip="The chart is drawn automatically from metadata TA-Lib provides. Your app can get the same through the Abstract API.">i</span>

**Outputs**

- `outRealUpperBand`: `upper_limit`
- `outRealMiddleBand`: `line`
- `outRealLowerBand`: `lower_limit`

:::

## Formula

factor = 4*(H-L)/(H+L)
upperRaw = H*(1+factor), lowerRaw = L*(1-factor)
Upper = SMA(upperRaw, N), Middle = SMA(Close, N), Lower = SMA(lowerRaw, N)

## Inputs

- `inHigh` — High price of each bar
- `inLow` — Low price of each bar
- `inClose` — Close price of each bar

## Outputs

- `outRealUpperBand` — SMA of the range-scaled high band
- `outRealMiddleBand` — SMA of the close
- `outRealLowerBand` — SMA of the range-scaled low band

## Parameters

| Parameter | Type | Default | Accepted values | Description |
| --- | --- | --- | --- | --- |
| `optInTimePeriod` | integer | 20 | 2–100000 | SMA smoothing period for all three bands |

## Properties

**Numerical Stability:** [Start-Independent](/functions/stability.md#start-independent)

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
double high[252]; /* your data, oldest first */
double low[252]; /* your data, oldest first */
double close[252]; /* your data, oldest first */
double upperBand[252], middleBand[252], lowerBand[252];
int outBegIdx, outNBElement;

TA_Initialize();
TA_ACCBANDS( 0, n - 1, high, low, close, 20,
             &outBegIdx, &outNBElement, upperBand, middleBand, lowerBand );
printf( "%d values from bar %d; last upperBand = %f\n", outNBElement, outBegIdx, upperBand[outNBElement - 1] );
```

@tab Rust

```rust
use ta_lib::Core;

let high = vec![0.0; 252]; // your data, oldest first
let low = vec![0.0; 252]; // your data, oldest first
let close = vec![0.0; 252]; // your data, oldest first
let mut upper_band = vec![0.0; 252];
let mut middle_band = vec![0.0; 252];
let mut lower_band = vec![0.0; 252];

let core = Core::new();
let range = core.accbands(0, high.len() - 1, &high, &low, &close, 20,
                          &mut upper_band, &mut middle_band, &mut lower_band)?;
println!("{} values from bar {}; last upper_band = {}", range.count, range.beg_idx, upper_band[range.count - 1]);
```

@tab Java

```java
import io.github.talib.Core;
import io.github.talib.OutRange;

double[] high = new double[252]; // your data, oldest first
double[] low = new double[252]; // your data, oldest first
double[] close = new double[252]; // your data, oldest first
double[] upperBand = new double[252];
double[] middleBand = new double[252];
double[] lowerBand = new double[252];

Core core = Core.DEFAULT;
OutRange r = core.accbands(0, high.length - 1, high, low, close, 20,
                           upperBand, middleBand, lowerBand);
System.out.println(r.count() + " values from bar " + r.begIdx() + "; last upperBand = " + upperBand[r.count() - 1]);
```

@tab C\#

```csharp
using System;
using TALib;

var high = new double[252]; // your data, oldest first
var low = new double[252]; // your data, oldest first
var close = new double[252]; // your data, oldest first
var upperBand = new double[252];
var middleBand = new double[252];
var lowerBand = new double[252];

var core = Core.Default;
OutRange r = core.Accbands(0, high.Length - 1, high, low, close, 20,
                           upperBand, middleBand, lowerBand);
Console.WriteLine($"{r.Count} values from bar {r.BegIdx}; last upperBand = {upperBand[r.Count - 1]}");
```

@tab TypeScript

```ts
import { accbands } from 'ta-lib';

const high = new Float64Array(252); // your data, oldest first
const low = new Float64Array(252); // your data, oldest first
const close = new Float64Array(252); // your data, oldest first

const {
  begIdx,
  outUpperBand,
  outMiddleBand,
  outLowerBand,
} = accbands({ high, low, close }, { timePeriod: 20 });
console.log(outUpperBand.length + ' values from bar ' + begIdx + '; last outUpperBand = ' + outUpperBand[outUpperBand.length - 1]);
```

:::

## Implementation

Added in 0.5.0.

TA-Lib Definition: [`accbands.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/accbands/accbands.c) · [`accbands.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/accbands/accbands.yaml)

| Native | File |
|--------|------|
| C | [`ta_ACCBANDS.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_ACCBANDS.c) |
| Rust | [`accbands.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/library/src/ta_func/accbands.rs) |
| Java | [`Core_ACCBANDS.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/fragments/Core_ACCBANDS.java) |
| C# | [`Core_ACCBANDS.cs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/csharp/library/src/Core_ACCBANDS.cs) |
| TypeScript | [`accbands.ts`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/ts/library/src/func/accbands.ts) |

Python: `talib.ACCBANDS`, see [ta-lib-python](https://ta-lib.github.io/ta-lib-python/func_groups/overlap_studies.html).

TA-Lib is also available for R and more using a [wrapper](/install/#wrappers).

## Aliases

Acceleration Bands

## See Also

[SMA](/functions/sma.md) · [BBANDS](/functions/bbands.md)
