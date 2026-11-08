---
title: "Schaff Trend Cycle (STC)"
description: "Schaff Trend Cycle (Doug Schaff): a MACD line passed twice through a stochastic, each pass smoothed by half."
---

## Summary

Schaff Trend Cycle (Doug Schaff): a MACD line passed twice through a stochastic, each pass smoothed by half. Bounded 0 to 100, read against 25 and 75: turning up from below 25 is bullish, turning down from above 75 bearish.

## Chart

<ta-chart src="/functions/stc.svg" ratio="720:416" label="STC over IBM, daily, 1999" alt="Drawn on the price pane over the close as a line; below it, a separate indicator pane with a zero line showing outReal as a solid line."></ta-chart>

::: details Chart metadata <span class="flag-tip" tabindex="0" role="note" aria-label="The chart is drawn from the function's metadata: the flags the abstract API reports for it and for each output, and the input series its reference run declares." data-tip="The chart is drawn from the function's metadata: the flags the abstract API reports for it and for each output, and the input series its reference run declares.">i</span>

Drawn on the price pane over the close as a line; below it, a separate indicator pane with a zero line showing outReal as a solid line.

Metadata flags used: `outReal: line`

:::

[Reference data (CSV)](/functions/stc.csv) · [Chart (SVG)](/functions/stc.svg)

## Formula

MACD[t] = EMA(x, fast)[t] - EMA(x, slow)[t]

Frac1[t] = 100 * (MACD[t] - min(MACD, cycle)) / (max(MACD, cycle) - min(MACD, cycle))

PF[t] = PF[t-1] + 0.5 * (Frac1[t] - PF[t-1])

Frac2[t] = 100 * (PF[t] - min(PF, cycle)) / (max(PF, cycle) - min(PF, cycle))

STC[t] = PFF[t] = PFF[t-1] + 0.5 * (Frac2[t] - PFF[t-1])

min and max are taken over the last cycle values, current one included. When a range is zero, the fraction keeps its previous value (0 before any). Each smoother starts on its first input.

## Notes

- The smoothing factor is fixed at 0.5, as in Schaff's published code.
- The MACD line is TA-Lib's: both EMAs are seeded with a simple average over windows ending on the same bar. The published code runs the EMAs from the first bar of the data.
- If the slow period is set smaller than the fast period, the two are swapped, as in `MACD`.
- Being recursive, an output depends on how much history precedes it. The unstable period warms the two smoothers; the EMA unstable period warms the MACD line.

## Inputs

- `inReal` — Input series (typically close)

## Outputs

- `outReal` — Schaff Trend Cycle, from 0 to 100

## Parameters

| Parameter | Type | Default | Accepted values | Description |
| --- | --- | --- | --- | --- |
| `optInFastPeriod` | integer | 23 | 2–100000 | Period of the fast EMA |
| `optInSlowPeriod` | integer | 50 | 2–100000 | Period of the slow EMA |
| `optInCyclePeriod` | integer | 10 | 2–100000 | Window of both stochastic stages |

## Properties

**Numerical Stability:** [Initial Unstable Period](/functions/stability.md#initial-unstable-period) — Tunable via STC's own unstable period and EMA's, which STC computes internally.

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
TA_STC( 0, n - 1, series, 23, 50, 10, &outBegIdx, &outNBElement, out );
printf( "%d values from bar %d; last out = %f\n", outNBElement, outBegIdx, out[outNBElement - 1] );
```

@tab Rust

```rust
use ta_lib::Core;

let series = vec![0.0; 252]; // your data, oldest first
let mut out = vec![0.0; 252];

let core = Core::new();
let range = core.stc(0, series.len() - 1, &series, 23, 50, 10, &mut out)?;
println!("{} values from bar {}; last out = {}", range.count, range.beg_idx, out[range.count - 1]);
```

@tab Java

```java
import io.github.talib.Core;
import io.github.talib.OutRange;

double[] series = new double[252]; // your data, oldest first
double[] out = new double[252];

Core core = Core.DEFAULT;
OutRange r = core.stc(0, series.length - 1, series, 23, 50, 10, out);
System.out.println(r.count() + " values from bar " + r.begIdx() + "; last out = " + out[r.count() - 1]);
```

@tab C\#

```csharp
using System;
using TALib;

var series = new double[252]; // your data, oldest first
var outReal = new double[252];

var core = Core.Default;
OutRange r = core.Stc(0, series.Length - 1, series, 23, 50, 10, outReal);
Console.WriteLine($"{r.Count} values from bar {r.BegIdx}; last outReal = {outReal[r.Count - 1]}");
```

@tab TypeScript

```ts
import { stc } from 'ta-lib';

const series = new Float64Array(252); // your data, oldest first

const { begIdx, values } = stc(series, {
  fastPeriod: 23,
  slowPeriod: 50,
  cyclePeriod: 10,
});
console.log(values.length + ' values from bar ' + begIdx + '; last values = ' + values[values.length - 1]);
```

:::

## Implementation

Added in 0.8.3 (ta-lib-python 0.8.3).

TA-Lib Definition: [`stc.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/stc/stc.c) · [`stc.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/stc/stc.yaml)

| Native | File |
|--------|------|
| C | [`ta_STC.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_STC.c) |
| Rust | [`stc.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/library/src/ta_func/stc.rs) |
| Java | [`Core_STC.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/fragments/Core_STC.java) |
| C# | [`Core_STC.cs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/csharp/library/src/Core_STC.cs) |
| TypeScript | [`stc.ts`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/ts/library/src/func/stc.ts) |

Python: `talib.STC` (ta-lib-python 0.8.3 or later), see [ta-lib-python](https://ta-lib.github.io/ta-lib-python/func_groups/momentum_indicators.html).

TA-Lib is also available for R and more using a [wrapper](/install/#wrappers).

## Aliases

Schaff Trend Cycle, Schaff TC

## See Also

[MACD](/functions/macd.md) · [STOCHF](/functions/stochf.md) · [STOCHRSI](/functions/stochrsi.md)

## References

- Doug Schaff, ["Releasing the Code to the Schaff Trend Cycle"](https://web.archive.org/web/20090418215759/mediaserver.fxstreet.com/Reports/99afdb5f-d41d-4a2c-802c-f5d787df886c/ebfbf387-4b27-4a0f-848c-039f4ab77c00.pdf), FX-Strategy.com, published on FXStreet.com, February 15, 2008. The EasyLanguage source, and the 23/50/10 defaults.
- Doug Schaff, "Catching Currency Moves with The Schaff Trend Cycle Indicator", *Chartpoint*, July/August 2002.
