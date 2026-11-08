---
title: "Ichimoku Kinko Hyo (ICHIMOKU)"
description: "Ichimoku Kinko Hyo, \"one glance equilibrium chart\": four lines built from highs and lows alone."
---

## Summary

Ichimoku Kinko Hyo, "one glance equilibrium chart": four lines built from highs and lows alone. Goichi Hosoda's reading is that a market's balance is visible without any smoothing — each line is the midpoint of a window, the mean of its highest high and its lowest low, so it marks the level at which that stretch of trading was evenly divided. The conversion line turns fastest, the base line is the reference, and the two leading spans are drawn ahead of price, where the band between them is read as support or resistance before it is reached.

## Chart

<ta-chart src="/functions/ichimoku.svg" ratio="720:274" label="ICHIMOKU over IBM, daily, 1999" alt="Drawn on the price pane over candles, with outTenkanSen as a solid line, outKijunSen as a solid line, outSenkouSpanA as a solid line moved 26 bars forward and outSenkouSpanB as a solid line moved 26 bars forward."></ta-chart>

Drawn on the price pane over candles, with outTenkanSen as a solid line, outKijunSen as a solid line, outSenkouSpanA as a solid line moved 26 bars forward and outSenkouSpanB as a solid line moved 26 bars forward. [Reference data (CSV)](/functions/ichimoku.csv) · [Chart (SVG)](/functions/ichimoku.svg)

## Formula

MID(n) = ( highest high over n bars + lowest low over n bars ) / 2

Tenkan-sen = MID(tenkanPeriod)

Kijun-sen = MID(kijunPeriod)

Senkou Span A = ( Tenkan-sen + Kijun-sen ) / 2

Senkou Span B = MID(senkouBPeriod)

## Notes

- Each line is `TA_MIDPRICE` over its own period, and Span A is `TA_MEDPRICE` of the Tenkan and Kijun lines: it halves two midpoints that are already rounded. Averaging the four extremes in one step is the same number in real arithmetic and can differ in the last bit.
- The two spans are drawn `kijunPeriod` bars ahead of the bar that computed them. That is a display shift: `TA_ICHIMOKU_DisplayShift` reports it, and it changes nothing about the values, the lookback or the returned range. Every output is written at the bar that computed it, so the last `kijunPeriod` span values are the cloud a chart draws past the last bar.
- Hosoda's 26 counts the current bar, which is a chart offset of 25. QuantConnect LEAN, pandas-ta and MetaTrader offset by 26, and that is what the display shift reports at the default periods; a caller who wants Hosoda's count draws the spans one bar earlier.
- The lookback is the longest of the three periods less one. Nothing orders the three, so any of them can be the longest.
- The Chikou span is the close drawn `kijunPeriod` bars back. It carries no computation and is not an output. Its usual signal, the close above the close `n` bars earlier, is `TA_MOM(close, n) > 0`.

## Inputs

- `inHigh` — High price series
- `inLow` — Low price series

## Outputs

- `outTenkanSen` — Conversion line
- `outKijunSen` — Base line
- `outSenkouSpanA` — First leading span, drawn ahead by the base period
- `outSenkouSpanB` — Second leading span, drawn ahead by the base period

## Parameters

| Parameter | Type | Default | Accepted values | Description |
| --- | --- | --- | --- | --- |
| `optInTenkanPeriod` | integer | 9 | 2–100000 | Period of the conversion line |
| `optInKijunPeriod` | integer | 26 | 2–100000 | Period of the base line, and the forward shift of the two spans |
| `optInSenkouBPeriod` | integer | 52 | 2–100000 | Period of the second leading span |

## Properties

**Numerical Stability:** [Start-Independent](/functions/stability.md#start-independent)

<div class="flag-table">

|  |
| :-- |
| <span class="flag-box">✅</span> **Overlap Input** <span class="flag-tip" tabindex="0" role="note" aria-label="Output is on the same scale as the input price, so it is drawn over the price chart." data-tip="Output is on the same scale as the input price, so it is drawn over the price chart.">i</span> |
| <span class="flag-box">☐</span> <span style="opacity:0.5">Independent Y-Axis</span> |
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
double high[252]; /* your data, oldest first */
double low[252]; /* your data, oldest first */
double tenkanSen[252], kijunSen[252], senkouSpanA[252], senkouSpanB[252];
int outBegIdx, outNBElement;

TA_Initialize();
TA_ICHIMOKU( 0, n - 1, high, low, 9, 26, 52,
             &outBegIdx, &outNBElement, tenkanSen, kijunSen, senkouSpanA, senkouSpanB );
printf( "%d values from bar %d; last tenkanSen = %f\n", outNBElement, outBegIdx, tenkanSen[outNBElement - 1] );
```

@tab Rust

```rust
use ta_lib::Core;

let high = vec![0.0; 252]; // your data, oldest first
let low = vec![0.0; 252]; // your data, oldest first
let mut tenkan_sen = vec![0.0; 252];
let mut kijun_sen = vec![0.0; 252];
let mut senkou_span_a = vec![0.0; 252];
let mut senkou_span_b = vec![0.0; 252];

let core = Core::new();
let range = core.ichimoku(0, high.len() - 1, &high, &low, 9, 26, 52,
                          &mut tenkan_sen, &mut kijun_sen, &mut senkou_span_a, &mut senkou_span_b)?;
println!("{} values from bar {}; last tenkan_sen = {}", range.count, range.beg_idx, tenkan_sen[range.count - 1]);
```

@tab Java

```java
import io.github.talib.Core;
import io.github.talib.OutRange;

double[] high = new double[252]; // your data, oldest first
double[] low = new double[252]; // your data, oldest first
double[] tenkanSen = new double[252];
double[] kijunSen = new double[252];
double[] senkouSpanA = new double[252];
double[] senkouSpanB = new double[252];

Core core = Core.DEFAULT;
OutRange r = core.ichimoku(0, high.length - 1, high, low, 9, 26, 52,
                           tenkanSen, kijunSen, senkouSpanA, senkouSpanB);
System.out.println(r.count() + " values from bar " + r.begIdx() + "; last tenkanSen = " + tenkanSen[r.count() - 1]);
```

@tab C\#

```csharp
using System;
using TALib;

var high = new double[252]; // your data, oldest first
var low = new double[252]; // your data, oldest first
var tenkanSen = new double[252];
var kijunSen = new double[252];
var senkouSpanA = new double[252];
var senkouSpanB = new double[252];

var core = Core.Default;
OutRange r = core.Ichimoku(0, high.Length - 1, high, low, 9, 26, 52,
                           tenkanSen, kijunSen, senkouSpanA, senkouSpanB);
Console.WriteLine($"{r.Count} values from bar {r.BegIdx}; last tenkanSen = {tenkanSen[r.Count - 1]}");
```

@tab TypeScript

```ts
import { ichimoku } from 'ta-lib';

const high = new Float64Array(252); // your data, oldest first
const low = new Float64Array(252); // your data, oldest first

const {
  begIdx,
  outTenkanSen,
  outKijunSen,
  outSenkouSpanA,
  outSenkouSpanB,
} = ichimoku({ high, low }, {
  tenkanPeriod: 9,
  kijunPeriod: 26,
  senkouBPeriod: 52,
});
console.log(outTenkanSen.length + ' values from bar ' + begIdx + '; last outTenkanSen = ' + outTenkanSen[outTenkanSen.length - 1]);
```

:::

## Implementation

Added in 0.8.3 (ta-lib-python 0.8.3).

TA-Lib Definition: [`ichimoku.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/ichimoku/ichimoku.c) · [`ichimoku.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/ichimoku/ichimoku.yaml)

| Native | File |
|--------|------|
| C | [`ta_ICHIMOKU.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_ICHIMOKU.c) |
| Rust | [`ichimoku.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/library/src/ta_func/ichimoku.rs) |
| Java | [`Core_ICHIMOKU.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/fragments/Core_ICHIMOKU.java) |
| C# | [`Core_ICHIMOKU.cs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/csharp/library/src/Core_ICHIMOKU.cs) |
| TypeScript | [`ichimoku.ts`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/ts/library/src/func/ichimoku.ts) |

Python: `talib.ICHIMOKU` (ta-lib-python 0.8.3 or later), see [ta-lib-python](https://ta-lib.github.io/ta-lib-python/func_groups/overlap_studies.html).

TA-Lib is also available for R and more using a [wrapper](/install/#wrappers).

## Aliases

ichimoku, ichimoku kinko hyo, ichimoku cloud, kumo

## See Also

[MIDPRICE](/functions/midprice.md) · [MEDPRICE](/functions/medprice.md) · [SAR](/functions/sar.md) · [BBANDS](/functions/bbands.md)

## References

- Goichi Hosoda, *Ichimoku Kinko Hyo*, 1969
