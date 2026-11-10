---
title: "Volume Weighted Moving Average (VWMA)"
description: "Volume Weighted Moving Average: the mean price over a trailing window of optInTimePeriod bars, each bar weighted by its own volume."
---

## Summary

Volume Weighted Moving Average: the mean price over a trailing window of `optInTimePeriod` bars, each bar weighted by its own volume. Heavily traded bars pull the average toward their price; quiet bars barely move it.

Read like any moving average — price above is strength, below is weakness. Against a plain [`SMA`](/functions/sma.md) of the same window it leads on high-volume moves and lags on low-volume drift, so the gap between the two lines measures how volume-confirmed a move is.

It has no attributable inventor — charting-package folklore — and every published definition agrees, so there is no competing variant.

## Chart

<ta-chart src="/functions/vwma.svg" ratio="720:342" label="VWMA over IBM, daily, 1999" alt="Drawn on the price pane over the close as a line, with outReal as a solid line; below it, a volume pane."></ta-chart>

<div class="ta-chart-links">

[Reference data (CSV)](/functions/vwma.csv) · [Chart (SVG)](/functions/vwma.svg)

</div>

::: details Chart metadata <span class="flag-tip" tabindex="0" role="note" aria-label="The chart is drawn automatically from metadata TA-Lib provides. Your app can get the same through the Abstract API." data-tip="The chart is drawn automatically from metadata TA-Lib provides. Your app can get the same through the Abstract API.">i</span>

**Outputs**

- `outReal`: `line`

:::

## Formula

VWMA = ( sum_{k=t-N+1..t} P[k] * V[k] ) / ( sum_{k=t-N+1..t} V[k] ), N = optInTimePeriod

Equivalently, SMA(P * V, N) / SMA(V, N), the composition TradingView documents for `ta.vwma`. In TA-Lib the two are bit-identical for N of 2 or more, up to the first window whose volume is entirely zero. There is no seeding and no recursion, hence no unstable period.

## Notes

- A period of 1 performs no smoothing: the output is a copy of the input, whatever the volume.
- Volume is expected to be non-negative. Individual zero-volume bars are fine: a bar that did not trade simply carries no weight, and the average stays well defined as long as some bar in the window has volume. At a period of 2 or more, a window in which *every* volume is zero has no weights at all; the weighted mean is then undefined and that element is NaN, as it is in every other implementation. Series carrying no volume on any bar, such as cash-index feeds, are outside what a volume-weighted average can describe — use SMA or WMA there.

## Inputs

- `inReal` — Source price series, close by convention
- `inVolume` — Volume of each bar

## Outputs

- `outReal` — Volume weighted moving average of the input

## Parameters

| Parameter | Type | Default | Accepted values | Description |
| --- | --- | --- | --- | --- |
| `optInTimePeriod` | integer | 30 | 1–100000 | Number of bars in the weighting window |

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
| <span class="flag-box">✅</span> **Can Output NaN or ±Inf** <span class="flag-tip" tabindex="0" role="note" aria-label="Some inputs have no finite result, so a successful call can return NaN or ±Inf — a gap with nothing to plot. See Notes for when." data-tip="Some inputs have no finite result, so a successful call can return NaN or ±Inf — a gap with nothing to plot. See Notes for when.">i</span> |
| <span class="flag-box">✅</span> **Identity at Period 1** <span class="flag-tip" tabindex="0" role="note" aria-label="A period of 1 performs no smoothing: every output value is a bit-exact copy of its input value." data-tip="A period of 1 performs no smoothing: every output value is a bit-exact copy of its input value.">i</span> |
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
double series[252]; /* your data, oldest first */
double volume[252]; /* your data, oldest first */
double out[252];
int outBegIdx, outNBElement;

TA_Initialize();
TA_VWMA( 0, n - 1, series, volume, 30, &outBegIdx, &outNBElement, out );
printf( "%d values from bar %d; last out = %f\n", outNBElement, outBegIdx, out[outNBElement - 1] );
```

@tab Rust

```rust
use ta_lib::Core;

let series = vec![0.0; 252]; // your data, oldest first
let volume = vec![0.0; 252]; // your data, oldest first
let mut out = vec![0.0; 252];

let core = Core::new();
let range = core.vwma(0, series.len() - 1, &series, &volume, 30, &mut out)?;
println!("{} values from bar {}; last out = {}", range.count, range.beg_idx, out[range.count - 1]);
```

@tab Java

```java
import io.github.talib.Core;
import io.github.talib.OutRange;

double[] series = new double[252]; // your data, oldest first
double[] volume = new double[252]; // your data, oldest first
double[] out = new double[252];

Core core = Core.DEFAULT;
OutRange r = core.vwma(0, series.length - 1, series, volume, 30, out);
System.out.println(r.count() + " values from bar " + r.begIdx() + "; last out = " + out[r.count() - 1]);
```

@tab C\#

```csharp
using System;
using TALib;

var series = new double[252]; // your data, oldest first
var volume = new double[252]; // your data, oldest first
var outReal = new double[252];

var core = Core.Default;
OutRange r = core.Vwma(0, series.Length - 1, series, volume, 30, outReal);
Console.WriteLine($"{r.Count} values from bar {r.BegIdx}; last outReal = {outReal[r.Count - 1]}");
```

@tab TypeScript

```ts
import { vwma } from 'ta-lib';

const series = new Float64Array(252); // your data, oldest first
const volume = new Float64Array(252); // your data, oldest first

const { begIdx, values } = vwma(series, { volume }, { timePeriod: 30 });
console.log(values.length + ' values from bar ' + begIdx + '; last values = ' + values[values.length - 1]);
```

:::

## Implementation

Added in 0.8.1 (ta-lib-python 0.8.1).

TA-Lib Definition: [`vwma.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/vwma/vwma.c) · [`vwma.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/vwma/vwma.yaml)

| Native | File |
|--------|------|
| C | [`ta_VWMA.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_VWMA.c) |
| Rust | [`vwma.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/library/src/ta_func/vwma.rs) |
| Java | [`Core_VWMA.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/fragments/Core_VWMA.java) |
| C# | [`Core_VWMA.cs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/csharp/library/src/Core_VWMA.cs) |
| TypeScript | [`vwma.ts`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/ts/library/src/func/vwma.ts) |

Python: `talib.VWMA` (ta-lib-python 0.8.1 or later), see [ta-lib-python](https://ta-lib.github.io/ta-lib-python/func_groups/overlap_studies.html).

TA-Lib is also available for R and more using a [wrapper](/install/#wrappers).

## Aliases

Volume Weighted Moving Average

## See Also

[SMA](/functions/sma.md) · [WMA](/functions/wma.md) · [MA](/functions/ma.md) · [OBV](/functions/obv.md)

## References

- VWMA has no separately documented originator; its definition is uniform across charting packages.
- MotiveWave, *Volume Weighted Moving Average* study documentation — the closest thing to a primary definition.
- TradingView, *Volume Weighted Moving Average (VWMA)* — documents the equivalence with SMA(price * volume) / SMA(volume).
