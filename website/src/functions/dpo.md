---
title: "Detrended Price Oscillator (DPO)"
description: "Detrended Price Oscillator: the price a half-cycle back, less the moving average that spans the cycle."
---

## Summary

Detrended Price Oscillator: the price a half-cycle back, less the moving average that spans the cycle. Removing the average removes the trend, leaving the shorter oscillation that the trend was hiding.

It crosses zero as price crosses its own average, so peaks and troughs mark the cycle rather than the direction of the market. The distance between successive peaks estimates the cycle length, and the amplitude is in price units, so it is comparable across time only for one instrument.

## Chart

<ta-chart src="/functions/dpo.svg" ratio="720:416" label="DPO over IBM, daily, 1999" alt="Drawn on the price pane over the close as a line; below it, a separate indicator pane with a zero line showing outReal as a solid line moved 11 bars back."></ta-chart>

::: details Chart metadata <span class="flag-tip" tabindex="0" role="note" aria-label="The chart is drawn from the function's metadata: the flags the abstract API reports for it and for each output, and the input series its reference run declares." data-tip="The chart is drawn from the function's metadata: the flags the abstract API reports for it and for each output, and the input series its reference run declares.">i</span>

Drawn on the price pane over the close as a line; below it, a separate indicator pane with a zero line showing outReal as a solid line moved 11 bars back.

Metadata flags used: `outReal: line, display_shift (-11)`

:::

[Reference data (CSV)](/functions/dpo.csv) · [Chart (SVG)](/functions/dpo.svg)

## Formula

Let `t = optInTimePeriod / 2 + 1`, an integer division, so a period and its odd successor share the same displacement.

    DPO[i] = P[i - t] - SMA(P, optInTimePeriod)[i]

## Notes

- The value is emitted at the bar whose moving average produced it. Charting packages usually draw it `t` bars to the left instead, which is a plotting convention rather than a different series; a caller wanting that view shifts `outReal` itself, by the display shift the function reports, `-t`.
- A causal variant, `P[i] - SMA(P, optInTimePeriod)[i - t]`, displaces the average instead of the price. It is a genuinely different series, not a re-indexing of this one, and is not implemented here.

## Inputs

- `inReal` — Source price/value series, canonically the close

## Outputs

- `outReal` — Detrended Price Oscillator value, in the units of the input

## Parameters

| Parameter | Type | Default | Accepted values | Description |
| --- | --- | --- | --- | --- |
| `optInTimePeriod` | integer | 20 | 2–100000 | Number of bars spanned by the moving average being removed; the displacement is derived from it |

## Properties

**Numerical Stability:** [Start-Independent](/functions/stability.md#start-independent)

<div class="flag-table">

|  |
| :-- |
| <span class="flag-box">☐</span> <span style="opacity:0.5">Overlap Input</span> |
| <span class="flag-box">✅</span> **Independent Y-Axis** <span class="flag-tip" tabindex="0" role="note" aria-label="Output is on its own scale, drawn in a separate pane below the price chart." data-tip="Output is on its own scale, drawn in a separate pane below the price chart.">i</span> |
| <span class="flag-box">☐</span> <span style="opacity:0.5">Candlestick</span> |
| <span class="flag-box">☐</span> <span style="opacity:0.5">Can Output NaN or ±Inf</span> |
| <span class="flag-box">☐</span> <span style="opacity:0.5">Identity at Period 1</span> |
| <span class="flag-box">✅</span> **Display Shift** <span class="flag-tip" tabindex="0" role="note" aria-label="A chart draws at least one output ahead of or behind the bar that computed it. The display-shift query gives the number of bars; the values are not shifted." data-tip="A chart draws at least one output ahead of or behind the bar that computed it. The display-shift query gives the number of bars; the values are not shifted.">i</span> |
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
TA_DPO( 0, n - 1, series, 20, &outBegIdx, &outNBElement, out );
printf( "%d values from bar %d; last out = %f\n", outNBElement, outBegIdx, out[outNBElement - 1] );
```

@tab Rust

```rust
use ta_lib::Core;

let series = vec![0.0; 252]; // your data, oldest first
let mut out = vec![0.0; 252];

let core = Core::new();
let range = core.dpo(0, series.len() - 1, &series, 20, &mut out)?;
println!("{} values from bar {}; last out = {}", range.count, range.beg_idx, out[range.count - 1]);
```

@tab Java

```java
import io.github.talib.Core;
import io.github.talib.OutRange;

double[] series = new double[252]; // your data, oldest first
double[] out = new double[252];

Core core = Core.DEFAULT;
OutRange r = core.dpo(0, series.length - 1, series, 20, out);
System.out.println(r.count() + " values from bar " + r.begIdx() + "; last out = " + out[r.count() - 1]);
```

@tab C\#

```csharp
using System;
using TALib;

var series = new double[252]; // your data, oldest first
var outReal = new double[252];

var core = Core.Default;
OutRange r = core.Dpo(0, series.Length - 1, series, 20, outReal);
Console.WriteLine($"{r.Count} values from bar {r.BegIdx}; last outReal = {outReal[r.Count - 1]}");
```

@tab TypeScript

```ts
import { dpo } from 'ta-lib';

const series = new Float64Array(252); // your data, oldest first

const { begIdx, values } = dpo(series, { timePeriod: 20 });
console.log(values.length + ' values from bar ' + begIdx + '; last values = ' + values[values.length - 1]);
```

:::

## Implementation

Added in 0.8.1 (ta-lib-python 0.8.1).

TA-Lib Definition: [`dpo.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/dpo/dpo.c) · [`dpo.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/dpo/dpo.yaml)

| Native | File |
|--------|------|
| C | [`ta_DPO.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_DPO.c) |
| Rust | [`dpo.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/library/src/ta_func/dpo.rs) |
| Java | [`Core_DPO.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/fragments/Core_DPO.java) |
| C# | [`Core_DPO.cs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/csharp/library/src/Core_DPO.cs) |
| TypeScript | [`dpo.ts`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/ts/library/src/func/dpo.ts) |

Python: `talib.DPO` (ta-lib-python 0.8.1 or later), see [ta-lib-python](https://ta-lib.github.io/ta-lib-python/func_groups/momentum_indicators.html).

TA-Lib is also available for R and more using a [wrapper](/install/#wrappers).

## Aliases

Detrended Price Oscillator

## See Also

[SMA](/functions/sma.md) · [MOM](/functions/mom.md) · [APO](/functions/apo.md)

## References

- Steven B. Achelis, *Technical Analysis from A to Z*, McGraw-Hill (p. 119)
- [StockCharts ChartSchool: Detrended Price Oscillator](https://chartschool.stockcharts.com/table-of-contents/technical-indicators-and-overlays/technical-indicators/detrended-price-oscillator-dpo)
