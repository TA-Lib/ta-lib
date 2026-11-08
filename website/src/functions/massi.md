---
title: "Mass Index (MASSI)"
description: "Mass Index: Donald Dorsey's non-directional measure of how the trading range itself is expanding or contracting."
---

## Summary

Mass Index: Donald Dorsey's non-directional measure of how the trading range itself is expanding or contracting. The high-low range is smoothed by an exponential moving average, that average is smoothed again by a second one of the same length, and the ratio of the first to the second is summed over a trailing window.

Read it as a bulge detector, not a direction. A ratio above one means the range is widening faster than its own smoothing can absorb, so the sum rises; a narrowing range pulls it back down. Dorsey's own rule is the "reversal bulge": the index rising above 27, then falling back under 26.5, warns that the prevailing trend is about to reverse. Which way it reverses has to come from a trend indicator, because the Mass Index has no sign of its own.

## Chart

<ta-chart src="/functions/massi.svg" ratio="720:416" label="MASSI over IBM, daily, 1999" alt="Drawn on the price pane over candles; below it, a separate indicator pane showing outReal as a solid line."></ta-chart>

::: details Chart metadata <span class="flag-tip" tabindex="0" role="note" aria-label="The chart is drawn from the function's metadata: the flags and attributes the abstract API reports for it and for each output, and the input series its reference run declares." data-tip="The chart is drawn from the function's metadata: the flags and attributes the abstract API reports for it and for each output, and the input series its reference run declares.">i</span>

Drawn on the price pane over candles; below it, a separate indicator pane showing outReal as a solid line.

Metadata flags used: `inPriceHL: high, low` · `outReal: line`

:::

[Reference data (CSV)](/functions/massi.csv) · [Chart (SVG)](/functions/massi.svg)

## Formula

HL = high - low

single = EMA( HL, optInFastPeriod )

double = EMA( single, optInFastPeriod )

MASSI = SUM( single / double, optInSlowPeriod )

Both averages are the standard TA-Lib EMA: smoothing factor 2 / (optInFastPeriod + 1), seeded with the simple average of the first optInFastPeriod inputs of that stage.

## Notes

- The two periods are not interchangeable and are never swapped: `optInFastPeriod` is the length of both exponential averages, `optInSlowPeriod` the length of the summation window. Some implementations reorder them when the summation window is the shorter of the two; this one does not.
- A window in which every bar is exactly flat, high equal to low, leaves both averages at zero. The ratio is reported as 1 there, its continuous limit, so a flat market yields exactly `optInSlowPeriod` rather than a spurious zero.
- Implementations disagree on how the exponential averages are seeded. TA-Lib uses its own EMA convention, the simple average of the first `optInFastPeriod` inputs, where Tulip Indicators, ta4j and trading-signals seed from a single raw value and converge to these values only after many bars. Published sample vectors, including the one in Achelis, are seeded that way and match only in the tail.
- MASSI inherits EMA's unstable period rather than owning one, and inherits it twice: when `TA_FUNC_UNST_EMA`, set to a count or to an Auto level, discards `u` bars from an EMA of `optInFastPeriod`, MASSI's first output moves by 2u.

## Inputs

- `inHigh` — High price
- `inLow` — Low price

## Outputs

- `outReal` — Summed ratio of the two smoothed high-low ranges

## Parameters

| Parameter | Type | Default | Accepted values | Description |
| --- | --- | --- | --- | --- |
| `optInFastPeriod` | integer | 9 | 2–100000 | Number of bars in each of the two exponential averages of the high-low range |
| `optInSlowPeriod` | integer | 25 | 2–100000 | Number of bars the ratio is summed over |

## Properties

**Numerical Stability:** [Initial Unstable Period](/functions/stability.md#initial-unstable-period) — Inherited from EMA, which MASSI computes internally; tunable via EMA's unstable period.

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
double out[252];
int outBegIdx, outNBElement;

TA_Initialize();
TA_MASSI( 0, n - 1, high, low, 9, 25, &outBegIdx, &outNBElement, out );
printf( "%d values from bar %d; last out = %f\n", outNBElement, outBegIdx, out[outNBElement - 1] );
```

@tab Rust

```rust
use ta_lib::Core;

let high = vec![0.0; 252]; // your data, oldest first
let low = vec![0.0; 252]; // your data, oldest first
let mut out = vec![0.0; 252];

let core = Core::new();
let range = core.massi(0, high.len() - 1, &high, &low, 9, 25, &mut out)?;
println!("{} values from bar {}; last out = {}", range.count, range.beg_idx, out[range.count - 1]);
```

@tab Java

```java
import io.github.talib.Core;
import io.github.talib.OutRange;

double[] high = new double[252]; // your data, oldest first
double[] low = new double[252]; // your data, oldest first
double[] out = new double[252];

Core core = Core.DEFAULT;
OutRange r = core.massi(0, high.length - 1, high, low, 9, 25, out);
System.out.println(r.count() + " values from bar " + r.begIdx() + "; last out = " + out[r.count() - 1]);
```

@tab C\#

```csharp
using System;
using TALib;

var high = new double[252]; // your data, oldest first
var low = new double[252]; // your data, oldest first
var outReal = new double[252];

var core = Core.Default;
OutRange r = core.Massi(0, high.Length - 1, high, low, 9, 25, outReal);
Console.WriteLine($"{r.Count} values from bar {r.BegIdx}; last outReal = {outReal[r.Count - 1]}");
```

@tab TypeScript

```ts
import { massi } from 'ta-lib';

const high = new Float64Array(252); // your data, oldest first
const low = new Float64Array(252); // your data, oldest first

const { begIdx, values } = massi({ high, low }, {
  fastPeriod: 9,
  slowPeriod: 25,
});
console.log(values.length + ' values from bar ' + begIdx + '; last values = ' + values[values.length - 1]);
```

:::

## Implementation

Added in 0.8.1 (ta-lib-python 0.8.1).

TA-Lib Definition: [`massi.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/massi/massi.c) · [`massi.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/massi/massi.yaml)

| Native | File |
|--------|------|
| C | [`ta_MASSI.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_MASSI.c) |
| Rust | [`massi.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/library/src/ta_func/massi.rs) |
| Java | [`Core_MASSI.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/fragments/Core_MASSI.java) |
| C# | [`Core_MASSI.cs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/csharp/library/src/Core_MASSI.cs) |
| TypeScript | [`massi.ts`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/ts/library/src/func/massi.ts) |

Python: `talib.MASSI` (ta-lib-python 0.8.1 or later), see [ta-lib-python](https://ta-lib.github.io/ta-lib-python/func_groups/volatility_indicators.html).

TA-Lib is also available for R and more using a [wrapper](/install/#wrappers).

## Aliases

Mass Index, Dorsey Mass Index, Reversal Bulge

## See Also

[CVI](/functions/cvi.md) · [ATR](/functions/atr.md) · [NATR](/functions/natr.md) · [TRANGE](/functions/trange.md) · [EMA](/functions/ema.md) · [SUM](/functions/sum.md)

## References

- Donald Dorsey, "The Mass Index", *Technical Analysis of Stocks & Commodities*, 1992 — the original description
- Steven B. Achelis, *Technical Analysis from A to Z*, McGraw-Hill, 2nd ed. (p. 182)
- [StockCharts ChartSchool: Mass Index](https://school.stockcharts.com/doku.php?id=technical_indicators:mass_index)
- [Tulip Indicators, mass](https://tulipindicators.org/mass) — an independent implementation, differing in its seeding
