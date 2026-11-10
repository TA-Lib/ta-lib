---
title: "Percent Rank (PERCENTRANK)"
description: "Percent Rank: where the current value sits inside the distribution of the values that came before it, as a percentage."
---

## Summary

Percent Rank: where the current value sits inside the distribution of the values that came before it, as a percentage.

Each bar is compared against the `optInTimePeriod` values immediately preceding it — the current bar is not part of its own comparison set — and the output is the share of them it exceeds. 0 means the current value is at or below every one of them, 100 means it is above all of them, 50 means it is above half.

It is a distribution-relative reading rather than a price-relative one: unlike an oscillator built from ranges or averages, it says nothing about how far the value moved, only how many of its recent predecessors it overtook. That makes it flat-scaled across instruments and directly comparable between them. It is best known as the third leg of Connors' ConnorsRSI, applied there to one-day returns rather than to price.

## Chart

<ta-chart src="/functions/percentrank.svg" ratio="720:416" label="PERCENTRANK over IBM, daily, 1999" alt="Drawn on the price pane over the close as a line; below it, a separate indicator pane showing outReal as a solid line."></ta-chart>

::: details Chart metadata <span class="flag-tip" tabindex="0" role="note" aria-label="The chart is drawn automatically from metadata TA-Lib provides. Your app can get the same through the Abstract API." data-tip="The chart is drawn automatically from metadata TA-Lib provides. Your app can get the same through the Abstract API.">i</span>

Drawn on the price pane over the close as a line; below it, a separate indicator pane showing outReal as a solid line.

Metadata flags used: `outReal: line`

:::

[Reference data (CSV)](/functions/percentrank.csv) · [Chart (SVG)](/functions/percentrank.svg)

## Formula

For each bar t, over the previous optInTimePeriod values:

count = number of j in [t-optInTimePeriod, t-1] with inReal[j] < inReal[t]

PERCENTRANK[t] = ( count / optInTimePeriod ) * 100

The comparison is strictly less-than, so a value tied with a predecessor does not count that predecessor.

## Notes

- Ties are strict: a predecessor equal to the current value is not counted. A constant series therefore reports 0 on every bar. TradingView's `ta.percentrank` counts ties as well (less-than-or-equal), which reports 100 on that same series; Pine parity is a different function, not a variant of this one, and the two agree only on windows with no repeated values.
- +0.0 and -0.0 compare equal, so a sign-only difference never contributes to the count.
- Finite input is a precondition. A NaN in the window fails every comparison, so it silently lowers the rank instead of propagating: the output stays finite and no error is reported.

## Inputs

- `inReal` — Source price/value series

## Outputs

- `outReal` — Percentage of the preceding window strictly below the current value, 0 to 100

## Parameters

| Parameter | Type | Default | Accepted values | Description |
| --- | --- | --- | --- | --- |
| `optInTimePeriod` | integer | 100 | 2–10000 | Number of preceding values the current value is ranked against |

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
TA_PERCENTRANK( 0, n - 1, series, 100, &outBegIdx, &outNBElement, out );
printf( "%d values from bar %d; last out = %f\n", outNBElement, outBegIdx, out[outNBElement - 1] );
```

@tab Rust

```rust
use ta_lib::Core;

let series = vec![0.0; 252]; // your data, oldest first
let mut out = vec![0.0; 252];

let core = Core::new();
let range = core.percentrank(0, series.len() - 1, &series, 100, &mut out)?;
println!("{} values from bar {}; last out = {}", range.count, range.beg_idx, out[range.count - 1]);
```

@tab Java

```java
import io.github.talib.Core;
import io.github.talib.OutRange;

double[] series = new double[252]; // your data, oldest first
double[] out = new double[252];

Core core = Core.DEFAULT;
OutRange r = core.percentrank(0, series.length - 1, series, 100, out);
System.out.println(r.count() + " values from bar " + r.begIdx() + "; last out = " + out[r.count() - 1]);
```

@tab C\#

```csharp
using System;
using TALib;

var series = new double[252]; // your data, oldest first
var outReal = new double[252];

var core = Core.Default;
OutRange r = core.Percentrank(0, series.Length - 1, series, 100, outReal);
Console.WriteLine($"{r.Count} values from bar {r.BegIdx}; last outReal = {outReal[r.Count - 1]}");
```

@tab TypeScript

```ts
import { percentrank } from 'ta-lib';

const series = new Float64Array(252); // your data, oldest first

const { begIdx, values } = percentrank(series, { timePeriod: 100 });
console.log(values.length + ' values from bar ' + begIdx + '; last values = ' + values[values.length - 1]);
```

:::

## Implementation

Added in 0.8.1 (ta-lib-python 0.8.1).

TA-Lib Definition: [`percentrank.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/percentrank/percentrank.c) · [`percentrank.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/percentrank/percentrank.yaml)

| Native | File |
|--------|------|
| C | [`ta_PERCENTRANK.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_PERCENTRANK.c) |
| Rust | [`percentrank.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/library/src/ta_func/percentrank.rs) |
| Java | [`Core_PERCENTRANK.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/fragments/Core_PERCENTRANK.java) |
| C# | [`Core_PERCENTRANK.cs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/csharp/library/src/Core_PERCENTRANK.cs) |
| TypeScript | [`percentrank.ts`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/ts/library/src/func/percentrank.ts) |

Python: `talib.PERCENTRANK` (ta-lib-python 0.8.1 or later), see [ta-lib-python](https://ta-lib.github.io/ta-lib-python/func_groups/statistic_functions.html).

TA-Lib is also available for R and more using a [wrapper](/install/#wrappers).

## See Also

[RSI](/functions/rsi.md) · [WILLR](/functions/willr.md) · [STDDEV](/functions/stddev.md)

## References

- [Connors Research, *An Introduction to ConnorsRSI* (2012), p. 8](https://www.qmatix.com/ConnorsRSI-Pullbacks-Guidebook.pdf)
- [ta4j `PercentRankIndicator`](https://github.com/ta4j/ta4j)
