---
title: "Chaikin Money Flow (CMF)"
description: "Chaikin Money Flow: over a trailing window of optInTimePeriod bars, the sum of each bar's money flow volume divided by the sum of its volume."
---

## Summary

Chaikin Money Flow: over a trailing window of `optInTimePeriod` bars, the sum of each bar's money flow volume divided by the sum of its volume. The result is a ratio in `[-1, +1]`.

A bar's money flow volume is its volume scaled by where the close sat inside the bar's range: a close at the high contributes the full volume, a close at the low contributes minus the full volume, and a close at the midpoint contributes nothing. Summing that over a window and dividing by the window's volume answers "over these N bars, what share of the traded volume closed near the top of its range?"

Above zero is accumulation, below zero is distribution, and the distance from zero measures conviction. Because the divisor is the window's own volume, the output is comparable across instruments and across time in a way a raw accumulation total is not.

Created by Marc Chaikin, who also authored the [`AD`](/functions/ad.md) line this shares its per-bar multiplier with. CMF is that same multiplier summed over a fixed window and normalised, where AD accumulates it from the start of the series without bound.

## Chart

<ta-chart src="/functions/cmf.svg" ratio="720:484" label="CMF over IBM, daily, 1999" alt="Drawn on the price pane over candles; below it, a separate indicator pane with a zero line showing outReal as a solid line, then a volume pane."></ta-chart>

Drawn on the price pane over candles; below it, a separate indicator pane with a zero line showing outReal as a solid line, then a volume pane. [Reference data (CSV)](/functions/cmf.csv) · [Chart (SVG)](/functions/cmf.svg)

## Formula

t = high[i] - low[i]

mfv[i] = ((close[i] - low[i]) - (high[i] - close[i])) / t * volume[i], or 0 when t is not positive

CMF[i] = ( sum_{k=i-N+1..i} mfv[k] ) / ( sum_{k=i-N+1..i} volume[k] ), N = optInTimePeriod

There is no seeding and no recursion, hence no unstable period. Each output depends only on the N bars in its own window.

## Notes

- The output is the raw ratio in `[-1, +1]`, matching every published definition. Some retail platforms display it multiplied by 100; that is a presentation choice, not a different indicator.
- Each bar's close is expected to lie within its own `[low, high]`, and its volume to be finite and non-negative. A close outside its bar makes the multiplier exceed ±1 and is passed through unclamped, exactly as [`AD`](/functions/ad.md) does.
- A bar whose high equals its low has no range for the close to sit inside, so it contributes exactly zero money flow volume rather than dividing by zero. Its volume still counts toward the divisor.
- A window whose volume is entirely zero has no money flow to distribute and reports 0.0. Published references are silent here and other implementations divide by zero; TA-Lib does not return NaN from a successful call.
- Bars where the low exceeds the high are malformed rather than degenerate, and also contribute zero.
- The default period of 20 follows the original write-up, which describes 20 or 21 bars.

## Inputs

- `inHigh` — High price of each bar
- `inLow` — Low price of each bar
- `inClose` — Close price of each bar
- `inVolume` — Volume of each bar

## Outputs

- `outReal` — Chaikin money flow, in the range -1 to +1

## Parameters

| Parameter | Type | Default | Accepted values | Description |
| --- | --- | --- | --- | --- |
| `optInTimePeriod` | integer | 20 | 2–100000 | Number of bars in the window |

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
double close[252]; /* your data, oldest first */
double volume[252]; /* your data, oldest first */
double out[252];
int outBegIdx, outNBElement;

TA_Initialize();
TA_CMF( 0, n - 1, high, low, close, volume, 20, &outBegIdx, &outNBElement, out );
printf( "%d values from bar %d; last out = %f\n", outNBElement, outBegIdx, out[outNBElement - 1] );
```

@tab Rust

```rust
use ta_lib::Core;

let high = vec![0.0; 252]; // your data, oldest first
let low = vec![0.0; 252]; // your data, oldest first
let close = vec![0.0; 252]; // your data, oldest first
let volume = vec![0.0; 252]; // your data, oldest first
let mut out = vec![0.0; 252];

let core = Core::new();
let range = core.cmf(0, high.len() - 1, &high, &low, &close, &volume, 20, &mut out)?;
println!("{} values from bar {}; last out = {}", range.count, range.beg_idx, out[range.count - 1]);
```

@tab Java

```java
import io.github.talib.Core;
import io.github.talib.OutRange;

double[] high = new double[252]; // your data, oldest first
double[] low = new double[252]; // your data, oldest first
double[] close = new double[252]; // your data, oldest first
double[] volume = new double[252]; // your data, oldest first
double[] out = new double[252];

Core core = Core.DEFAULT;
OutRange r = core.cmf(0, high.length - 1, high, low, close, volume, 20, out);
System.out.println(r.count() + " values from bar " + r.begIdx() + "; last out = " + out[r.count() - 1]);
```

@tab C\#

```csharp
using System;
using TALib;

var high = new double[252]; // your data, oldest first
var low = new double[252]; // your data, oldest first
var close = new double[252]; // your data, oldest first
var volume = new double[252]; // your data, oldest first
var outReal = new double[252];

var core = Core.Default;
OutRange r = core.Cmf(0, high.Length - 1, high, low, close, volume, 20, outReal);
Console.WriteLine($"{r.Count} values from bar {r.BegIdx}; last outReal = {outReal[r.Count - 1]}");
```

@tab TypeScript

```ts
import { cmf } from 'ta-lib';

const high = new Float64Array(252); // your data, oldest first
const low = new Float64Array(252); // your data, oldest first
const close = new Float64Array(252); // your data, oldest first
const volume = new Float64Array(252); // your data, oldest first

const { begIdx, values } = cmf({ high, low, close, volume }, {
  timePeriod: 20,
});
console.log(values.length + ' values from bar ' + begIdx + '; last values = ' + values[values.length - 1]);
```

:::

## Implementation

Added in 0.8.1 (ta-lib-python 0.8.1).

TA-Lib Definition: [`cmf.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/cmf/cmf.c) · [`cmf.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/cmf/cmf.yaml)

| Native | File |
|--------|------|
| C | [`ta_CMF.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_CMF.c) |
| Rust | [`cmf.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/library/src/ta_func/cmf.rs) |
| Java | [`Core_CMF.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/fragments/Core_CMF.java) |
| C# | [`Core_CMF.cs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/csharp/library/src/Core_CMF.cs) |
| TypeScript | [`cmf.ts`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/ts/library/src/func/cmf.ts) |

Python: `talib.CMF` (ta-lib-python 0.8.1 or later), see [ta-lib-python](https://ta-lib.github.io/ta-lib-python/func_groups/volume_indicators.html).

TA-Lib is also available for R and more using a [wrapper](/install/#wrappers).

## Aliases

Chaikin Money Flow

## See Also

[AD](/functions/ad.md) · [ADOSC](/functions/adosc.md) · [MFI](/functions/mfi.md) · [OBV](/functions/obv.md)

## References

- Marc Chaikin is the originator; StockCharts ChartSchool carries the canonical three-step derivation, "20-period Sum of Money Flow Volume / 20-period Sum of Volume".
- TradingView, *Chaikin Money Flow (CMF)* — the same derivation with a worked 21-period example.
- Kirkpatrick and Dahlquist, *Technical Analysis: The Complete Resource for Financial Market Technicians*, 2nd edition, pages 419 and 421.
