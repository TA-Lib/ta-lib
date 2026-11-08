---
title: "Relative Volatility Index (RVI)"
description: "Relative Volatility Index: Donald Dorsey's volatility oscillator, built exactly like RSI except that the quantity routed to the up and down buckets is…"
---

## Summary

Relative Volatility Index: Donald Dorsey's volatility oscillator, built exactly like RSI except that the quantity routed to the up and down buckets is the rolling standard deviation of price rather than the size of the move. The direction of the close-to-close change still decides which bucket a bar feeds.

Bounded in 0..100. High values mean the recent volatility arrived mostly on up bars, low values that it arrived mostly on down bars. Dorsey proposed it as a confirming filter rather than a stand-alone signal: take a long entry only while RVI is above 50, a short only while it is below.

## Chart

<ta-chart src="/functions/rvi.svg" ratio="720:416" label="RVI over IBM, daily, 1999" alt="Drawn on the price pane over the close as a line; below it, a separate indicator pane showing outReal as a solid line."></ta-chart>

::: details Chart metadata <span class="flag-tip" tabindex="0" role="note" aria-label="The chart is drawn from the function's metadata: the flags the abstract API reports for it and for each output, and the input series its reference run declares." data-tip="The chart is drawn from the function's metadata: the flags the abstract API reports for it and for each output, and the input series its reference run declares.">i</span>

Drawn on the price pane over the close as a line; below it, a separate indicator pane showing outReal as a solid line.

Metadata flags used: `outReal: line`

:::

[Reference data (CSV)](/functions/rvi.csv) · [Chart (SVG)](/functions/rvi.svg)

## Formula

With `S` the standard deviation of the last `optInStdDevPeriod` values of `inReal`, and `C` the input series:

    U[i] = S[i] if C[i] > C[i-1], else 0
    D[i] = S[i] if C[i] < C[i-1], else 0
    Up   = RMA(U, optInTimePeriod)
    Down = RMA(D, optInTimePeriod)
    RVI  = 100 * Up / ( Up + Down ), or 50 when Up + Down = 0

`RMA` is Wilder's smoothed moving average, seeded with the simple average of its first `optInTimePeriod` inputs. A bar whose close equals the previous close feeds neither bucket.

## Notes

- RVI is the 1993 version; [`RVIR`](/functions/rvir.md) is the 1995 revision.
- A tie contributes to neither bucket, matching RSI's treatment of an unchanged close. Descriptions that write the denominator as a smoothed `S` instead of `Up + Down` are counting ties as down bars, which is a different indicator.
- Unrelated to the Relative Vigor Index, which several platforms also abbreviate RVI.

## Inputs

- `inReal` — Source price/value series, canonically the close

## Outputs

- `outReal` — Relative Volatility Index value

## Parameters

| Parameter | Type | Default | Accepted values | Description |
| --- | --- | --- | --- | --- |
| `optInTimePeriod` | integer | 14 | 1–100000 | Wilder smoothing period applied to both legs |
| `optInStdDevPeriod` | integer | 10 | 2–100000 | Number of trailing values the standard deviation spans |

## Properties

**Numerical Stability:** [Initial Unstable Period](/functions/stability.md#initial-unstable-period)

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
double series[252]; /* your data, oldest first */
double out[252];
int outBegIdx, outNBElement;

TA_Initialize();
TA_RVI( 0, n - 1, series, 14, 10, &outBegIdx, &outNBElement, out );
printf( "%d values from bar %d; last out = %f\n", outNBElement, outBegIdx, out[outNBElement - 1] );
```

@tab Rust

```rust
use ta_lib::Core;

let series = vec![0.0; 252]; // your data, oldest first
let mut out = vec![0.0; 252];

let core = Core::new();
let range = core.rvi(0, series.len() - 1, &series, 14, 10, &mut out)?;
println!("{} values from bar {}; last out = {}", range.count, range.beg_idx, out[range.count - 1]);
```

@tab Java

```java
import io.github.talib.Core;
import io.github.talib.OutRange;

double[] series = new double[252]; // your data, oldest first
double[] out = new double[252];

Core core = Core.DEFAULT;
OutRange r = core.rvi(0, series.length - 1, series, 14, 10, out);
System.out.println(r.count() + " values from bar " + r.begIdx() + "; last out = " + out[r.count() - 1]);
```

@tab C\#

```csharp
using System;
using TALib;

var series = new double[252]; // your data, oldest first
var outReal = new double[252];

var core = Core.Default;
OutRange r = core.Rvi(0, series.Length - 1, series, 14, 10, outReal);
Console.WriteLine($"{r.Count} values from bar {r.BegIdx}; last outReal = {outReal[r.Count - 1]}");
```

@tab TypeScript

```ts
import { rvi } from 'ta-lib';

const series = new Float64Array(252); // your data, oldest first

const { begIdx, values } = rvi(series, { timePeriod: 14, stdDevPeriod: 10 });
console.log(values.length + ' values from bar ' + begIdx + '; last values = ' + values[values.length - 1]);
```

:::

## Implementation

Added in 0.8.1 (ta-lib-python 0.8.1).

TA-Lib Definition: [`rvi.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/rvi/rvi.c) · [`rvi.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/rvi/rvi.yaml)

| Native | File |
|--------|------|
| C | [`ta_RVI.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_RVI.c) |
| Rust | [`rvi.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/library/src/ta_func/rvi.rs) |
| Java | [`Core_RVI.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/fragments/Core_RVI.java) |
| C# | [`Core_RVI.cs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/csharp/library/src/Core_RVI.cs) |
| TypeScript | [`rvi.ts`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/ts/library/src/func/rvi.ts) |

Python: `talib.RVI` (ta-lib-python 0.8.1 or later), see [ta-lib-python](https://ta-lib.github.io/ta-lib-python/func_groups/volatility_indicators.html).

TA-Lib is also available for R and more using a [wrapper](/install/#wrappers).

## Aliases

Relative Volatility Index, RVIorig

## See Also

[RSI](/functions/rsi.md) · [RMA](/functions/rma.md) · [STDDEV](/functions/stddev.md) · [CMO](/functions/cmo.md)

## References

- Donald Dorsey, "The Relative Volatility Index", *Technical Analysis of Stocks & Commodities*, V.11:6 (June 1993), 253-256
- Donald Dorsey, "Refining the Relative Volatility Index", *Technical Analysis of Stocks & Commodities*, V.13:9 (September 1995), 388-391
- [Chart manual: Relative Volatility Index](https://user42.tuxfamily.org/chart/manual/Relative-Volatility-Index.html)
- [DXcharts: Relative Volatility Index](https://devexperts.com/dxcharts/kb/docs/relative-volatility-index-rvi)
