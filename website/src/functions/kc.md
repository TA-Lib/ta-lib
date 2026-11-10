---
title: "Keltner Channels (KC)"
description: "Keltner Channels: three overlap lines around price."
---

## Summary

Keltner Channels: three overlap lines around price. The centre line is an exponential moving average of the typical price; the outer bands sit a multiple of the Average True Range above and below it. The band width tracks volatility, so the channel widens in fast markets and narrows in quiet ones.

## Chart

<ta-chart src="/functions/kc.svg" ratio="720:274" label="KC over IBM, daily, 1999" alt="Drawn on the price pane over candles, with outRealMiddleBand as a solid line and a band between outRealUpperBand and outRealLowerBand with a soft fill."></ta-chart>

::: details Chart metadata <span class="flag-tip" tabindex="0" role="note" aria-label="The chart is drawn automatically from metadata TA-Lib provides. Your app can get the same through the Abstract API." data-tip="The chart is drawn automatically from metadata TA-Lib provides. Your app can get the same through the Abstract API.">i</span>

Drawn on the price pane over candles, with outRealMiddleBand as a solid line and a band between outRealUpperBand and outRealLowerBand with a soft fill.

Metadata flags used: `overlap` · `inPriceHLC: high, low, close` · `outRealUpperBand: upper_limit` · `outRealMiddleBand: line` · `outRealLowerBand: lower_limit`

:::

[Reference data (CSV)](/functions/kc.csv) · [Chart (SVG)](/functions/kc.svg)

## Formula

TP = (High + Low + Close) / 3
Middle = EMA(TP, N)
Band = ATR(M)
Upper = Middle + Deviations * Band
Lower = Middle - Deviations * Band

## Notes

- Several incompatible indicators are published under the name "Keltner Channel", disagreeing by percent rather than by rounding. This is the typical-price centre line with a Wilder-smoothed Average True Range band, the form implemented by TTR and ta4j.
- Chester Keltner's 1960 original smooths the typical price with a simple moving average and takes the band from the plain daily range; the widely charted modern variant centres on the close instead. Expect a visible difference against a package plotting either.
- TTR ties the Average True Range period to the centre line's period. Here the two are independent, so the band width can be tuned separately.
- The centre line and the band are separate recursions, each with its own warm-up. They are entered at their own lookbacks, so a caller who wants either one converged sets that function's unstable period — `TA_FUNC_UNST_EMA` for the centre line, `TA_FUNC_UNST_ATR` for the band.

## Inputs

- `inHigh` — High price of each bar
- `inLow` — Low price of each bar
- `inClose` — Close price of each bar

## Outputs

- `outRealUpperBand` — Centre line plus the scaled Average True Range
- `outRealMiddleBand` — Exponential moving average of the typical price
- `outRealLowerBand` — Centre line minus the scaled Average True Range

## Parameters

| Parameter | Type | Default | Accepted values | Description |
| --- | --- | --- | --- | --- |
| `optInTimePeriod` | integer | 20 | 2–100000 | Smoothing period of the typical price moving average |
| `optInATRPeriod` | integer | 10 | 1–100000 | Smoothing period of the Average True Range |
| `optInNbDev` | real | 2 | any real | Multiplier applied to the Average True Range |

## Properties

**Numerical Stability:** [Initial Unstable Period](/functions/stability.md#initial-unstable-period) — Inherited from ATR and EMA, which KC computes internally; tunable via ATR and EMA's unstable period.

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
double high[252]; /* your data, oldest first */
double low[252]; /* your data, oldest first */
double close[252]; /* your data, oldest first */
double upperBand[252], middleBand[252], lowerBand[252];
int outBegIdx, outNBElement;

TA_Initialize();
TA_KC( 0, n - 1, high, low, close, 20, 10, 2.0,
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
let range = core.kc(0, high.len() - 1, &high, &low, &close, 20, 10, 2.0,
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
OutRange r = core.kc(0, high.length - 1, high, low, close, 20, 10, 2.0,
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
OutRange r = core.Kc(0, high.Length - 1, high, low, close, 20, 10, 2.0,
                     upperBand, middleBand, lowerBand);
Console.WriteLine($"{r.Count} values from bar {r.BegIdx}; last upperBand = {upperBand[r.Count - 1]}");
```

@tab TypeScript

```ts
import { kc } from 'ta-lib';

const high = new Float64Array(252); // your data, oldest first
const low = new Float64Array(252); // your data, oldest first
const close = new Float64Array(252); // your data, oldest first

const {
  begIdx,
  outUpperBand,
  outMiddleBand,
  outLowerBand,
} = kc({ high, low, close }, { timePeriod: 20, atrPeriod: 10, nbDev: 2 });
console.log(outUpperBand.length + ' values from bar ' + begIdx + '; last outUpperBand = ' + outUpperBand[outUpperBand.length - 1]);
```

:::

## Implementation

Added in 0.8.1 (ta-lib-python 0.8.1).

TA-Lib Definition: [`kc.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/kc/kc.c) · [`kc.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/kc/kc.yaml)

| Native | File |
|--------|------|
| C | [`ta_KC.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_KC.c) |
| Rust | [`kc.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/library/src/ta_func/kc.rs) |
| Java | [`Core_KC.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/fragments/Core_KC.java) |
| C# | [`Core_KC.cs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/csharp/library/src/Core_KC.cs) |
| TypeScript | [`kc.ts`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/ts/library/src/func/kc.ts) |

Python: `talib.KC` (ta-lib-python 0.8.1 or later), see [ta-lib-python](https://ta-lib.github.io/ta-lib-python/func_groups/overlap_studies.html).

TA-Lib is also available for R and more using a [wrapper](/install/#wrappers).

## Aliases

Keltner Channel

## See Also

[EMA](/functions/ema.md) · [ATR](/functions/atr.md) · [TYPPRICE](/functions/typprice.md) · [BBANDS](/functions/bbands.md) · [ACCBANDS](/functions/accbands.md)

## References

- [Keltner channel](https://en.wikipedia.org/wiki/Keltner_channel)
- [Keltner Channels — StockCharts ChartSchool](https://chartschool.stockcharts.com/table-of-contents/technical-indicators-and-overlays/technical-overlays/keltner-channels)
