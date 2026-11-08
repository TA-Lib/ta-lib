---
title: "Percentile (nearest rank) (PERCENTILE)"
description: "Rolling percentile by the nearest-rank method: sort the trailing window ascending and report the value whose 1-based ordinal rank is the P-th percentile…"
---

## Summary

Rolling percentile by the nearest-rank method: sort the trailing window ascending and report the value whose 1-based ordinal rank is the P-th percentile of the window size. The result is always a value that actually occurred in the window, never an interpolation, so it stays on the price scale and never invents a level the series never traded at. At P = 50 with an odd window it is the rolling median; at the extremes it degenerates to the rolling minimum and maximum.

## Chart

<ta-chart src="/functions/percentile.svg" ratio="720:274" label="PERCENTILE over IBM, daily, 1999" alt="Drawn on the price pane over the close as a line, with outReal as a solid line."></ta-chart>

::: details Chart metadata <span class="flag-tip" tabindex="0" role="note" aria-label="The chart is drawn from the function's metadata: the flags the abstract API reports for it and for each output, and the input series its reference run declares." data-tip="The chart is drawn from the function's metadata: the flags the abstract API reports for it and for each output, and the input series its reference run declares.">i</span>

Drawn on the price pane over the close as a line, with outReal as a solid line.

Metadata flags used: `overlap` · `outReal: line`

:::

[Reference data (CSV)](/functions/percentile.csv) · [Chart (SVG)](/functions/percentile.svg)

## Formula

$W_t = \operatorname{sort}(x_{t-N+1}, \dots, x_t)$; $k = \left\lceil \frac{P \cdot N}{100} \right\rceil$ clamped to $[1, N]$; $PERCENTILE_t = W_t[k]$ (N = optInTimePeriod, P = optInPercentile, $W_t[1]$ the smallest)

## Notes

- The nearest-rank method is one of several incompatible percentile conventions. The linear-interpolation family (Hyndman & Fan type 7, the default of most statistical packages, and TradingView's `ta.percentile_linear_interpolation`) reports a weighted blend of two neighbouring order statistics and can emit a value that never occurred. That is a different indicator, not a mode of this one: PERCENTILE's parameter list is fixed at a window and a percentage, and a method selector cannot be appended to it later without changing the function's arity.
- A tail percentile rests on the few values beyond it: at N = 30, P = 95 is the second-largest value in the window. For a stable estimate keep about ten values beyond the percentile, N ≥ 1000 / min(P, 100 − P): 100 bars at P = 10 or 90, 200 at P = 5 or 95.
- Every input value in the window must be finite. A NaN makes every comparison against it false, which breaks the ordering the rank index is read from.

## Inputs

- `inReal` — Source series to take the percentile of

## Outputs

- `outReal` — The value at the requested rank within the trailing window

## Parameters

| Parameter | Type | Default | Accepted values | Description |
| --- | --- | --- | --- | --- |
| `optInTimePeriod` | integer | 100 | 2–10000 | Number of bars in the trailing window |
| `optInPercentile` | real | 50 | 0–100 | Percentage position within the sorted window |

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
TA_PERCENTILE( 0, n - 1, series, 100, 50.0, &outBegIdx, &outNBElement, out );
printf( "%d values from bar %d; last out = %f\n", outNBElement, outBegIdx, out[outNBElement - 1] );
```

@tab Rust

```rust
use ta_lib::Core;

let series = vec![0.0; 252]; // your data, oldest first
let mut out = vec![0.0; 252];

let core = Core::new();
let range = core.percentile(0, series.len() - 1, &series, 100, 50.0, &mut out)?;
println!("{} values from bar {}; last out = {}", range.count, range.beg_idx, out[range.count - 1]);
```

@tab Java

```java
import io.github.talib.Core;
import io.github.talib.OutRange;

double[] series = new double[252]; // your data, oldest first
double[] out = new double[252];

Core core = Core.DEFAULT;
OutRange r = core.percentile(0, series.length - 1, series, 100, 50.0, out);
System.out.println(r.count() + " values from bar " + r.begIdx() + "; last out = " + out[r.count() - 1]);
```

@tab C\#

```csharp
using System;
using TALib;

var series = new double[252]; // your data, oldest first
var outReal = new double[252];

var core = Core.Default;
OutRange r = core.Percentile(0, series.Length - 1, series, 100, 50.0, outReal);
Console.WriteLine($"{r.Count} values from bar {r.BegIdx}; last outReal = {outReal[r.Count - 1]}");
```

@tab TypeScript

```ts
import { percentile } from 'ta-lib';

const series = new Float64Array(252); // your data, oldest first

const { begIdx, values } = percentile(series, {
  timePeriod: 100,
  percentile: 50,
});
console.log(values.length + ' values from bar ' + begIdx + '; last values = ' + values[values.length - 1]);
```

:::

## Implementation

Added in 0.8.1 (ta-lib-python 0.8.1).

TA-Lib Definition: [`percentile.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/percentile/percentile.c) · [`percentile.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/percentile/percentile.yaml)

| Native | File |
|--------|------|
| C | [`ta_PERCENTILE.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_PERCENTILE.c) |
| Rust | [`percentile.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/library/src/ta_func/percentile.rs) |
| Java | [`Core_PERCENTILE.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/fragments/Core_PERCENTILE.java) |
| C# | [`Core_PERCENTILE.cs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/csharp/library/src/Core_PERCENTILE.cs) |
| TypeScript | [`percentile.ts`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/ts/library/src/func/percentile.ts) |

Python: `talib.PERCENTILE` (ta-lib-python 0.8.1 or later), see [ta-lib-python](https://ta-lib.github.io/ta-lib-python/func_groups/statistic_functions.html).

TA-Lib is also available for R and more using a [wrapper](/install/#wrappers).

## Aliases

Percentile Nearest Rank, Rolling Percentile, Rolling Quantile

## See Also

[MIN](/functions/min.md) · [MAX](/functions/max.md) · [MEDPRICE](/functions/medprice.md) · [STDDEV](/functions/stddev.md)

## References

- [Percentile — nearest-rank method](https://en.wikipedia.org/wiki/Percentile), the ordinal rank definition this function implements.
- Rob J. Hyndman and Yanan Fan, "Sample Quantiles in Statistical Packages", *The American Statistician* 50(4), 1996 — the nearest-rank rule is their type 1.
