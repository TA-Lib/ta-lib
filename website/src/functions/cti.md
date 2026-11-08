---
title: "Correlation Trend Indicator (CTI)"
description: "John F. Ehlers' Correlation Trend Indicator: the Pearson correlation of the last optInTimePeriod closes against a straight line of positive slope."
---

## Summary

John F. Ehlers' Correlation Trend Indicator: the Pearson correlation of the last `optInTimePeriod` closes against a straight line of positive slope. Bounded in -1..+1 by construction — `+1` is a perfectly linear uptrend, `-1` a perfectly linear downtrend, `0` no linear trend. Trend strength and direction in one bounded number, with no smoothing, no recursion and no filter coefficients.

Arithmetically it is [`CORREL`](/functions/correl.md) of the series against a ramp, with the ramp side collapsed to closed-form constants.

## Chart

<ta-chart src="/functions/cti.svg" ratio="720:416" label="CTI over IBM, daily, 1999" alt="Drawn on the price pane over the close as a line; below it, a separate indicator pane with a zero line showing outReal as a solid line."></ta-chart>

::: details Chart metadata <span class="flag-tip" tabindex="0" role="note" aria-label="The chart is drawn from the function's metadata: the flags and attributes the abstract API reports for it and for each output, and the input series its reference run declares." data-tip="The chart is drawn from the function's metadata: the flags and attributes the abstract API reports for it and for each output, and the input series its reference run declares.">i</span>

Drawn on the price pane over the close as a line; below it, a separate indicator pane with a zero line showing outReal as a solid line.

Metadata flags used: `outReal: line, positive, negative`

:::

[Reference data (CSV)](/functions/cti.csv) · [Chart (SVG)](/functions/cti.svg)

## Formula

With `n` = `optInTimePeriod`, `x` the window's closes and `y` a straight line rising with time:

    CTI = ( n·Σxy − Σx·Σy ) / sqrt( ( n·Σx² − (Σx)² ) · ( n·Σy² − (Σy)² ) ), or 0 when the window is flat

A rising series therefore reads positive. The `y` side is data-independent and collapses exactly: `n·Σy² − (Σy)² = n²(n²−1)/12`.

## Notes

- A flat window returns exactly `0.0` rather than holding the previous value as the author's listing does; holding would make the function path-dependent.
- The result is clamped into -1..+1, as `CORREL` is: rounding in three sums can put a coefficient slightly outside its own range.
- `optInTimePeriod` starts at 2, not 1: at `n = 1` the closed form `n²(n²−1)/12` is identically zero and every window is degenerate.

## Inputs

- `inReal` — The series to measure the trend of

## Outputs

- `outReal` — Correlation against the ramp, in -1..+1

## Parameters

| Parameter | Type | Default | Accepted values | Description |
| --- | --- | --- | --- | --- |
| `optInTimePeriod` | integer | 20 | 2–100000 | Number of trailing values correlated against the ramp |

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
double series[252]; /* your data, oldest first */
double out[252];
int outBegIdx, outNBElement;

TA_Initialize();
TA_CTI( 0, n - 1, series, 20, &outBegIdx, &outNBElement, out );
printf( "%d values from bar %d; last out = %f\n", outNBElement, outBegIdx, out[outNBElement - 1] );
```

@tab Rust

```rust
use ta_lib::Core;

let series = vec![0.0; 252]; // your data, oldest first
let mut out = vec![0.0; 252];

let core = Core::new();
let range = core.cti(0, series.len() - 1, &series, 20, &mut out)?;
println!("{} values from bar {}; last out = {}", range.count, range.beg_idx, out[range.count - 1]);
```

@tab Java

```java
import io.github.talib.Core;
import io.github.talib.OutRange;

double[] series = new double[252]; // your data, oldest first
double[] out = new double[252];

Core core = Core.DEFAULT;
OutRange r = core.cti(0, series.length - 1, series, 20, out);
System.out.println(r.count() + " values from bar " + r.begIdx() + "; last out = " + out[r.count() - 1]);
```

@tab C\#

```csharp
using System;
using TALib;

var series = new double[252]; // your data, oldest first
var outReal = new double[252];

var core = Core.Default;
OutRange r = core.Cti(0, series.Length - 1, series, 20, outReal);
Console.WriteLine($"{r.Count} values from bar {r.BegIdx}; last outReal = {outReal[r.Count - 1]}");
```

@tab TypeScript

```ts
import { cti } from 'ta-lib';

const series = new Float64Array(252); // your data, oldest first

const { begIdx, values } = cti(series, { timePeriod: 20 });
console.log(values.length + ' values from bar ' + begIdx + '; last values = ' + values[values.length - 1]);
```

:::

## Implementation

Added in 0.8.3 (ta-lib-python 0.8.3).

TA-Lib Definition: [`cti.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/cti/cti.c) · [`cti.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/cti/cti.yaml)

| Native | File |
|--------|------|
| C | [`ta_CTI.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_CTI.c) |
| Rust | [`cti.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/library/src/ta_func/cti.rs) |
| Java | [`Core_CTI.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/fragments/Core_CTI.java) |
| C# | [`Core_CTI.cs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/csharp/library/src/Core_CTI.cs) |
| TypeScript | [`cti.ts`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/ts/library/src/func/cti.ts) |

Python: `talib.CTI` (ta-lib-python 0.8.3 or later), see [ta-lib-python](https://ta-lib.github.io/ta-lib-python/func_groups/momentum_indicators.html).

TA-Lib is also available for R and more using a [wrapper](/install/#wrappers).

## Aliases

Correlation Trend Indicator, Ehlers Correlation Trend Indicator, Correlation Trend

## See Also

[CORREL](/functions/correl.md) · [LINEARREG_SLOPE](/functions/linearreg_slope.md) · [VHF](/functions/vhf.md)

## References

- John F. Ehlers, "Correlation As A Trend Indicator", *Technical Analysis of Stocks & Commodities*, V.38:5 (May 2020), Code Listing 1, [author's PDF](https://www.mesasoftware.com/papers/CORRELATION%20AS%20A%20TREND%20INDICATOR.pdf)
