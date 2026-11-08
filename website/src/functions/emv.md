---
title: "Arms Ease of Movement (EMV)"
description: "Ease of Movement: the bar-to-bar move of the high-low midpoint divided by a box ratio of volume to range, averaged over a trailing window."
---

## Summary

Ease of Movement: the bar-to-bar move of the high-low midpoint divided by a box ratio of volume to range, averaged over a trailing window. It is the numeric form of Richard W. Arms, Jr.'s Equivolume box.

The box ratio is positive whenever the bar traded and has a range, so the sign follows the midpoint move. A large positive value means price rose easily, on light volume relative to its range; a large negative value means it fell easily. Values near zero mean volume was heavy for the distance travelled, or price hardly moved.

The output scales with the instrument's volume and with the volume divisor, so its level is comparable only within one instrument at one divisor. Traders mostly watch its sign and its zero crossings.

## Chart

<ta-chart src="/functions/emv.svg" ratio="720:484" label="EMV over IBM, daily, 1999" alt="Drawn on the price pane over candles; below it, a separate indicator pane with a zero line showing outReal as a solid line, then a volume pane."></ta-chart>

Drawn on the price pane over candles; below it, a separate indicator pane with a zero line showing outReal as a solid line, then a volume pane. [Reference data (CSV)](/functions/emv.csv) · [Chart (SVG)](/functions/emv.svg)

## Formula

mid[i] = (high[i] + low[i]) / 2

box[i] = (volume[i] / D) / (high[i] - low[i])

raw[i] = (mid[i] - mid[i-1]) / box[i]

EMV[i] = ( sum_{k=i-N+1..i} raw[k] ) / N, N = optInTimePeriod, D = optInVolumeDivisor

A bar with no range or no volume has no box ratio and contributes 0. The next bar still measures its midpoint move from that bar.

## Notes

- The range is in price points. Achelis's text gives it in eighths of a point, the pre-decimal US quote unit; that reading is reached by multiplying the divisor by 8.
- The divisor is a pure output scale: doubling it doubles every value. Pick one that suits the instrument's volume.
- A period of 1 returns the unsmoothed one-bar values. Smoothing is a simple moving average; for an exponential one, apply `EMA` to this function's output at a period of 1.

## Inputs

- `inHigh` — High price of each bar
- `inLow` — Low price of each bar
- `inVolume` — Volume of each bar

## Outputs

- `outReal` — Ease of Movement, averaged over the window

## Parameters

| Parameter | Type | Default | Accepted values | Description |
| --- | --- | --- | --- | --- |
| `optInTimePeriod` | integer | 14 | 1–100000 | Number of one-bar values in the simple moving average |
| `optInVolumeDivisor` | real | 10000 | ≥ 1 | Volume is divided by this before it forms the box ratio |

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
double volume[252]; /* your data, oldest first */
double out[252];
int outBegIdx, outNBElement;

TA_Initialize();
TA_EMV( 0, n - 1, high, low, volume, 14, 10000.0, &outBegIdx, &outNBElement, out );
printf( "%d values from bar %d; last out = %f\n", outNBElement, outBegIdx, out[outNBElement - 1] );
```

@tab Rust

```rust
use ta_lib::Core;

let high = vec![0.0; 252]; // your data, oldest first
let low = vec![0.0; 252]; // your data, oldest first
let volume = vec![0.0; 252]; // your data, oldest first
let mut out = vec![0.0; 252];

let core = Core::new();
let range = core.emv(0, high.len() - 1, &high, &low, &volume, 14, 10000.0, &mut out)?;
println!("{} values from bar {}; last out = {}", range.count, range.beg_idx, out[range.count - 1]);
```

@tab Java

```java
import io.github.talib.Core;
import io.github.talib.OutRange;

double[] high = new double[252]; // your data, oldest first
double[] low = new double[252]; // your data, oldest first
double[] volume = new double[252]; // your data, oldest first
double[] out = new double[252];

Core core = Core.DEFAULT;
OutRange r = core.emv(0, high.length - 1, high, low, volume, 14, 10000.0, out);
System.out.println(r.count() + " values from bar " + r.begIdx() + "; last out = " + out[r.count() - 1]);
```

@tab C\#

```csharp
using System;
using TALib;

var high = new double[252]; // your data, oldest first
var low = new double[252]; // your data, oldest first
var volume = new double[252]; // your data, oldest first
var outReal = new double[252];

var core = Core.Default;
OutRange r = core.Emv(0, high.Length - 1, high, low, volume, 14, 10000.0, outReal);
Console.WriteLine($"{r.Count} values from bar {r.BegIdx}; last outReal = {outReal[r.Count - 1]}");
```

@tab TypeScript

```ts
import { emv } from 'ta-lib';

const high = new Float64Array(252); // your data, oldest first
const low = new Float64Array(252); // your data, oldest first
const volume = new Float64Array(252); // your data, oldest first

const { begIdx, values } = emv({ high, low, volume }, {
  timePeriod: 14,
  volumeDivisor: 10000,
});
console.log(values.length + ' values from bar ' + begIdx + '; last values = ' + values[values.length - 1]);
```

:::

## Implementation

Added in 0.8.3 (ta-lib-python 0.8.3).

TA-Lib Definition: [`emv.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/emv/emv.c) · [`emv.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/emv/emv.yaml)

| Native | File |
|--------|------|
| C | [`ta_EMV.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_EMV.c) |
| Rust | [`emv.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/library/src/ta_func/emv.rs) |
| Java | [`Core_EMV.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/fragments/Core_EMV.java) |
| C# | [`Core_EMV.cs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/csharp/library/src/Core_EMV.cs) |
| TypeScript | [`emv.ts`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/ts/library/src/func/emv.ts) |

Python: `talib.EMV` (ta-lib-python 0.8.3 or later), see [ta-lib-python](https://ta-lib.github.io/ta-lib-python/func_groups/volume_indicators.html).

TA-Lib is also available for R and more using a [wrapper](/install/#wrappers).

## Aliases

Ease of Movement, EOM, Arms Ease of Movement

## See Also

[MEDPRICE](/functions/medprice.md) · [MARKETFI](/functions/marketfi.md) · [EFI](/functions/efi.md) · [SMA](/functions/sma.md) · [EMA](/functions/ema.md)

## References

- Richard W. Arms, Jr., *Volume Cycles in the Stock Market: Market Timing Through Equivolume Charting*, Dow Jones-Irwin, 1983.
- Steven B. Achelis, *Technical Analysis from A to Z*, 2nd edition, McGraw-Hill, 2000, "Ease of Movement".
- W. A. Thorp, [Arms' Ease of Movement: Adding Volume to the Equation](https://www.aaii.com/journal/article/arms-ease-of-movement-adding-volume-to-the-equation), *AAII Journal*, October 2001.
- StockCharts ChartSchool, [Ease of Movement (EMV)](https://chartschool.stockcharts.com/table-of-contents/technical-indicators-and-overlays/technical-indicators/ease-of-movement-emv).
