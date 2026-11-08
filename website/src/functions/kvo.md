---
title: "Klinger Volume Oscillator (KVO)"
description: "Klinger Volume Oscillator: each bar's volume is signed by the direction of its high-low-close sum and weighted by where the bar's own range sits inside…"
---

## Summary

Klinger Volume Oscillator: each bar's volume is signed by the direction of its high-low-close sum and weighted by where the bar's own range sits inside the cumulative range of the current trend run, then the resulting volume force is smoothed twice and differenced. Klinger's reading is that volume alone says how much trading happened but not what it accomplished, so he scales it by the share of the trend's accumulated range that this bar actually covered. The oscillator crosses zero as the balance of that force changes hands, and crosses its own trigger line earlier.

## Chart

<ta-chart src="/functions/kvo.svg" ratio="720:484" label="KVO over IBM, daily, 1999" alt="Drawn on the price pane over candles; below it, a separate indicator pane with a zero line showing outKVO as a solid line and outKVOSignal as a dashed line, then a volume pane."></ta-chart>

::: details Chart metadata <span class="flag-tip" tabindex="0" role="note" aria-label="The chart is drawn from the function's metadata: the flags the abstract API reports for it and for each output, and the input series its reference run declares." data-tip="The chart is drawn from the function's metadata: the flags the abstract API reports for it and for each output, and the input series its reference run declares.">i</span>

Drawn on the price pane over candles; below it, a separate indicator pane with a zero line showing outKVO as a solid line and outKVOSignal as a dashed line, then a volume pane.

Metadata flags used: `inPriceHLCV: high, low, close, volume` · `outKVO: line` · `outKVOSignal: dash_line`

:::

[Reference data (CSV)](/functions/kvo.csv) · [Chart (SVG)](/functions/kvo.svg)

## Formula

hlc = high + low + close;  dm = high - low

trend = +1 if hlc rose, -1 if hlc fell, unchanged if hlc repeated

cm = previous dm + current dm on a trend change, otherwise previous cm + current dm

vf = volume * |2 * (dm / cm) - 1| * 100 * trend

KVO = EMA(vf, fastPeriod) - EMA(vf, slowPeriod)

Signal = EMA(KVO, signalPeriod)

## Notes

- The trend is **held** when the high-low-close sum repeats, which is the article's own tenet: the existing trend is maintained on equality. A reading that breaks the tie in either direction changes every later bar, because the cumulative range is reset by a trend change.
- The cumulative range is reset by every trend change, so two calls starting at different bars carry different range state only until the first trend change after both; what remains between them is the seed error of the three averages, which fades at the rate of the longest period. KVO converges, but as a small difference of two large averages it needs more bars than one of them would.
- All three exponential averages are seeded from the volume force of the same anchor bar rather than from a simple average of their own first inputs, so one warm-up covers all three. KVO is exactly zero on that seed bar, which falls inside the warm-up and is never output. `TA_SetUnstablePeriod(TA_FUNC_UNST_EMA, ...)` discards more of that warm-up.
- The weight is the 1997 form, `|2*(dm/cm) - 1|`, which is what the December 1997 Traders' Tips print. Some later implementations use `|2*(dm/cm - 1)|`, which is a different series.
- A cumulative range of exactly zero leaves the volume force at zero rather than dividing.

## Inputs

- `inHigh` — High price series
- `inLow` — Low price series
- `inClose` — Close price series
- `inVolume` — Volume series

## Outputs

- `outKVO` — Klinger Volume Oscillator
- `outKVOSignal` — Trigger line, an exponential average of the oscillator

## Parameters

| Parameter | Type | Default | Accepted values | Description |
| --- | --- | --- | --- | --- |
| `optInFastPeriod` | integer | 34 | 2–100000 | Period of the faster smoothing of the volume force |
| `optInSlowPeriod` | integer | 55 | 2–100000 | Period of the slower smoothing of the volume force |
| `optInSignalPeriod` | integer | 13 | 2–100000 | Smoothing period of the trigger line |

## Properties

**Numerical Stability:** [Initial Unstable Period](/functions/stability.md#initial-unstable-period) — Inherited from EMA, which KVO computes internally; tunable via EMA's unstable period.

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
double kvo[252], kvoSignal[252];
int outBegIdx, outNBElement;

TA_Initialize();
TA_KVO( 0, n - 1, high, low, close, volume, 34, 55, 13, &outBegIdx, &outNBElement, kvo, kvoSignal );
printf( "%d values from bar %d; last kvo = %f\n", outNBElement, outBegIdx, kvo[outNBElement - 1] );
```

@tab Rust

```rust
use ta_lib::Core;

let high = vec![0.0; 252]; // your data, oldest first
let low = vec![0.0; 252]; // your data, oldest first
let close = vec![0.0; 252]; // your data, oldest first
let volume = vec![0.0; 252]; // your data, oldest first
let mut kvo = vec![0.0; 252];
let mut kvo_signal = vec![0.0; 252];

let core = Core::new();
let range = core.kvo(0, high.len() - 1, &high, &low, &close, &volume, 34, 55, 13,
                     &mut kvo, &mut kvo_signal)?;
println!("{} values from bar {}; last kvo = {}", range.count, range.beg_idx, kvo[range.count - 1]);
```

@tab Java

```java
import io.github.talib.Core;
import io.github.talib.OutRange;

double[] high = new double[252]; // your data, oldest first
double[] low = new double[252]; // your data, oldest first
double[] close = new double[252]; // your data, oldest first
double[] volume = new double[252]; // your data, oldest first
double[] kvo = new double[252];
double[] kvoSignal = new double[252];

Core core = Core.DEFAULT;
OutRange r = core.kvo(0, high.length - 1, high, low, close, volume, 34, 55, 13, kvo, kvoSignal);
System.out.println(r.count() + " values from bar " + r.begIdx() + "; last kvo = " + kvo[r.count() - 1]);
```

@tab C\#

```csharp
using System;
using TALib;

var high = new double[252]; // your data, oldest first
var low = new double[252]; // your data, oldest first
var close = new double[252]; // your data, oldest first
var volume = new double[252]; // your data, oldest first
var kvo = new double[252];
var kvoSignal = new double[252];

var core = Core.Default;
OutRange r = core.Kvo(0, high.Length - 1, high, low, close, volume, 34, 55, 13, kvo, kvoSignal);
Console.WriteLine($"{r.Count} values from bar {r.BegIdx}; last kvo = {kvo[r.Count - 1]}");
```

@tab TypeScript

```ts
import { kvo } from 'ta-lib';

const high = new Float64Array(252); // your data, oldest first
const low = new Float64Array(252); // your data, oldest first
const close = new Float64Array(252); // your data, oldest first
const volume = new Float64Array(252); // your data, oldest first

const { begIdx, outKvo, outKvoSignal } = kvo({ high, low, close, volume }, {
  fastPeriod: 34,
  slowPeriod: 55,
  signalPeriod: 13,
});
console.log(outKvo.length + ' values from bar ' + begIdx + '; last outKvo = ' + outKvo[outKvo.length - 1]);
```

:::

## Implementation

Added in 0.8.3 (ta-lib-python 0.8.3).

TA-Lib Definition: [`kvo.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/kvo/kvo.c) · [`kvo.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/kvo/kvo.yaml)

| Native | File |
|--------|------|
| C | [`ta_KVO.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_KVO.c) |
| Rust | [`kvo.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/library/src/ta_func/kvo.rs) |
| Java | [`Core_KVO.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/fragments/Core_KVO.java) |
| C# | [`Core_KVO.cs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/csharp/library/src/Core_KVO.cs) |
| TypeScript | [`kvo.ts`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/ts/library/src/func/kvo.ts) |

Python: `talib.KVO` (ta-lib-python 0.8.3 or later), see [ta-lib-python](https://ta-lib.github.io/ta-lib-python/func_groups/volume_indicators.html).

TA-Lib is also available for R and more using a [wrapper](/install/#wrappers).

## Aliases

klinger, klinger volume oscillator, klinger oscillator

## See Also

[ADOSC](/functions/adosc.md) · [OBV](/functions/obv.md) · [MFI](/functions/mfi.md) · [AD](/functions/ad.md)

## References

- Stephen J. Klinger, "Identifying Trends With Volume Analysis", *Technical Analysis of Stocks & Commodities*, v15:12 (December 1997)
