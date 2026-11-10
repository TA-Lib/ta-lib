---
title: "Rolling Median (MEDIAN)"
description: "The middle order statistic of the trailing window: the central value when optInTimePeriod is odd, the mean of the two central values when it is even."
---

## Summary

The middle order statistic of the trailing window: the central value when `optInTimePeriod` is odd, the mean of the two central values when it is even. A robust measure of central tendency — unlike [`SMA`](/functions/sma.md), a single spike moves it by at most one rank however large the spike is, which is what makes it useful as a filter rather than as a level.

Not to be confused with [`MEDPRICE`](/functions/medprice.md), which is `(High + Low) / 2` of one bar and is not an order statistic.

## Chart

<ta-chart src="/functions/median.svg" ratio="720:274" label="MEDIAN over IBM, daily, 1999" alt="Drawn on the price pane over the close as a line, with outReal as a solid line."></ta-chart>

::: details Chart metadata <span class="flag-tip" tabindex="0" role="note" aria-label="The chart is drawn automatically from metadata TA-Lib provides. Your app can get the same through the Abstract API." data-tip="The chart is drawn automatically from metadata TA-Lib provides. Your app can get the same through the Abstract API.">i</span>

Drawn on the price pane over the close as a line, with outReal as a solid line.

Metadata flags used: `overlap` · `outReal: line`

:::

[Reference data (CSV)](/functions/median.csv) · [Chart (SVG)](/functions/median.svg)

## Formula

With `W` the window sorted ascending and `W[1]` its smallest value:

    n odd :   MEDIAN = W[(n+1)/2]
    n even:   MEDIAN = ( W[n/2] + W[n/2 + 1] ) / 2

## Notes

- This is not [`PERCENTILE`](/functions/percentile.md) at 50. `PERCENTILE` reports the nearest rank, which at even `n` selects the **lower** of the two central values: on a 4-bar window of `1, 2, 3, 4` this function returns `2.5` and `PERCENTILE` returns `2`. At odd `n` the two agree bit for bit.
- At even `n` the output can therefore be a value the series never traded at, which is the deliberate opposite of `PERCENTILE`'s design property.
- `optInTimePeriod` is not restricted to odd values: TA-Lib has no odd-only range mechanism, and it would surprise any caller reaching for a 20-bar median.
- Every input value must be finite. A NaN makes every comparison against it false, which breaks the order the window is kept in, and the output can stay wrong long after the NaN has left the window.

## Inputs

- `inReal` — The series to take the median of

## Outputs

- `outReal` — Median of the trailing window

## Parameters

| Parameter | Type | Default | Accepted values | Description |
| --- | --- | --- | --- | --- |
| `optInTimePeriod` | integer | 30 | 2–10000 | Number of trailing values in the window |

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
TA_MEDIAN( 0, n - 1, series, 30, &outBegIdx, &outNBElement, out );
printf( "%d values from bar %d; last out = %f\n", outNBElement, outBegIdx, out[outNBElement - 1] );
```

@tab Rust

```rust
use ta_lib::Core;

let series = vec![0.0; 252]; // your data, oldest first
let mut out = vec![0.0; 252];

let core = Core::new();
let range = core.median(0, series.len() - 1, &series, 30, &mut out)?;
println!("{} values from bar {}; last out = {}", range.count, range.beg_idx, out[range.count - 1]);
```

@tab Java

```java
import io.github.talib.Core;
import io.github.talib.OutRange;

double[] series = new double[252]; // your data, oldest first
double[] out = new double[252];

Core core = Core.DEFAULT;
OutRange r = core.median(0, series.length - 1, series, 30, out);
System.out.println(r.count() + " values from bar " + r.begIdx() + "; last out = " + out[r.count() - 1]);
```

@tab C\#

```csharp
using System;
using TALib;

var series = new double[252]; // your data, oldest first
var outReal = new double[252];

var core = Core.Default;
OutRange r = core.Median(0, series.Length - 1, series, 30, outReal);
Console.WriteLine($"{r.Count} values from bar {r.BegIdx}; last outReal = {outReal[r.Count - 1]}");
```

@tab TypeScript

```ts
import { median } from 'ta-lib';

const series = new Float64Array(252); // your data, oldest first

const { begIdx, values } = median(series, { timePeriod: 30 });
console.log(values.length + ' values from bar ' + begIdx + '; last values = ' + values[values.length - 1]);
```

:::

## Implementation

Added in 0.8.3 (ta-lib-python 0.8.3).

TA-Lib Definition: [`median.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/median/median.c) · [`median.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/median/median.yaml)

| Native | File |
|--------|------|
| C | [`ta_MEDIAN.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_MEDIAN.c) |
| Rust | [`median.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/library/src/ta_func/median.rs) |
| Java | [`Core_MEDIAN.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/fragments/Core_MEDIAN.java) |
| C# | [`Core_MEDIAN.cs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/csharp/library/src/Core_MEDIAN.cs) |
| TypeScript | [`median.ts`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/ts/library/src/func/median.ts) |

Python: `talib.MEDIAN` (ta-lib-python 0.8.3 or later), see [ta-lib-python](https://ta-lib.github.io/ta-lib-python/func_groups/statistic_functions.html).

TA-Lib is also available for R and more using a [wrapper](/install/#wrappers).

## Aliases

Rolling Median, Moving Median, Running Median

## See Also

[PERCENTILE](/functions/percentile.md) · [SMA](/functions/sma.md) · [MEDPRICE](/functions/medprice.md)

## References

- NumPy `numpy.median`, R `stats::median`, scipy, Excel `MEDIAN` — four independent implementations of one unambiguous definition.
