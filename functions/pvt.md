---
url: 'https://ta-lib.org/functions/pvt.md'
description: >-
  Price Volume Trend: a running cumulative total of each bar's volume weighted
  by that bar's fractional price change.
---
# Price Volume Trend (PVT)

## Summary

Price Volume Trend: a running cumulative total of each bar's volume weighted by that bar's fractional price change. It is the On Balance Volume idea with partial credit — where OBV adds or subtracts a bar's whole volume on the sign of the move, PVT adds only the fraction of that volume proportional to the size of the move.

Read the slope and the divergences, not the level: the total's zero point is arbitrary, so only the shape of the curve carries information. A rising PVT while price is flat says volume is accumulating on the up moves; a falling PVT while price rises is the classic bearish divergence.

## Chart

<ta-chart src="/functions/pvt.svg" ratio="720:484" label="PVT over IBM, daily, 1999" alt="Drawn on the price pane over the close as a line; below it, a separate indicator pane with a zero line showing outReal as a solid line, then a volume pane."></ta-chart>

::: details Chart metadata <span class="flag-tip" tabindex="0" role="note" aria-label="The chart is drawn from the function's metadata: the flags and attributes the abstract API reports for it and for each output, and the input series its reference run declares." data-tip="The chart is drawn from the function's metadata: the flags and attributes the abstract API reports for it and for each output, and the input series its reference run declares.">i</span>

Drawn on the price pane over the close as a line; below it, a separate indicator pane with a zero line showing outReal as a solid line, then a volume pane.

Metadata flags used: `inPriceCV: close, volume` · `outReal: line, positive, negative`

:::

[Reference data (CSV)](/functions/pvt.csv) · [Chart (SVG)](/functions/pvt.svg)

## Formula

```
PVT[i] = PVT[i-1] + inVolume[i] * (inClose[i] - inClose[i-1]) / inClose[i-1]
```

The series starts at zero on the first bar of the requested range.

## Notes

* **The input is assumed positive.** The previous close is the divisor. Near zero the divisor is small and the output very large; no error is returned. A series that reaches or crosses zero, such as an oscillator or a spread, has no meaningful result here.
* The absolute level is arbitrary and depends on where the accumulation started, so two ranges over the same data give curves of the same shape at different offsets.
* Some libraries scale the per-bar term by 100. This implementation follows the fractional definition, which every primary reference below states.
* A bar whose previous close is exactly zero contributes nothing and the running total is carried forward unchanged, rather than dividing by zero. A flat stretch of the output can therefore mean either genuine zero net accumulation or a run of zero previous closes.

## Inputs

* `inClose` — Close price of each bar
* `inVolume` — Volume of each bar

## Outputs

* `outReal` — Cumulative price volume trend, seeded at zero

## Properties

**Numerical Stability:** [Path-Dependent](/functions/stability.md#path-dependent)

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
double close[252]; /* your data, oldest first */
double volume[252]; /* your data, oldest first */
double out[252];
int outBegIdx, outNBElement;

TA_Initialize();
TA_PVT( 0, n - 1, close, volume, &outBegIdx, &outNBElement, out );
printf( "%d values from bar %d; last out = %f\n", outNBElement, outBegIdx, out[outNBElement - 1] );
```

@tab Rust

```rust
use ta_lib::Core;

let close = vec![0.0; 252]; // your data, oldest first
let volume = vec![0.0; 252]; // your data, oldest first
let mut out = vec![0.0; 252];

let core = Core::new();
let range = core.pvt(0, close.len() - 1, &close, &volume, &mut out)?;
println!("{} values from bar {}; last out = {}", range.count, range.beg_idx, out[range.count - 1]);
```

@tab Java

```java
import io.github.talib.Core;
import io.github.talib.OutRange;

double[] close = new double[252]; // your data, oldest first
double[] volume = new double[252]; // your data, oldest first
double[] out = new double[252];

Core core = Core.DEFAULT;
OutRange r = core.pvt(0, close.length - 1, close, volume, out);
System.out.println(r.count() + " values from bar " + r.begIdx() + "; last out = " + out[r.count() - 1]);
```

@tab C#

```csharp
using System;
using TALib;

var close = new double[252]; // your data, oldest first
var volume = new double[252]; // your data, oldest first
var outReal = new double[252];

var core = Core.Default;
OutRange r = core.Pvt(0, close.Length - 1, close, volume, outReal);
Console.WriteLine($"{r.Count} values from bar {r.BegIdx}; last outReal = {outReal[r.Count - 1]}");
```

@tab TypeScript

```ts
import { pvt } from 'ta-lib';

const close = new Float64Array(252); // your data, oldest first
const volume = new Float64Array(252); // your data, oldest first

const { begIdx, values } = pvt({ close, volume });
console.log(values.length + ' values from bar ' + begIdx + '; last values = ' + values[values.length - 1]);
```

:::

## Implementation

Added in 0.8.1 (ta-lib-python 0.8.1).

TA-Lib Definition: [`pvt.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/pvt/pvt.c) · [`pvt.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/pvt/pvt.yaml)

| Native | File |
|--------|------|
| C | [`ta_PVT.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_PVT.c) |
| Rust | [`pvt.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/library/src/ta_func/pvt.rs) |
| Java | [`Core_PVT.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/fragments/Core_PVT.java) |
| C# | [`Core_PVT.cs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/csharp/library/src/Core_PVT.cs) |
| TypeScript | [`pvt.ts`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/ts/library/src/func/pvt.ts) |

Python: `talib.PVT` (ta-lib-python 0.8.1 or later), see [ta-lib-python](https://ta-lib.github.io/ta-lib-python/func_groups/volume_indicators.html).

TA-Lib is also available for R and more using a [wrapper](/install/#wrappers).

## Aliases

Price Volume Trend, Volume Price Trend, VPT

## See Also

[OBV](/functions/obv.md) · [NVI](/functions/nvi.md) · [PVI](/functions/pvi.md) · [PVO](/functions/pvo.md) · [AD](/functions/ad.md)

## References

* [Wikipedia: Volume–price trend](https://en.wikipedia.org/wiki/Volume%E2%80%93price_trend)
* [TradingView: Price Volume Trend (PVT)](https://www.tradingview.com/support/solutions/43000502345-price-volume-trend-pvt/)
