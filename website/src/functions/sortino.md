---
title: "Rolling Sortino Ratio (SORTINO)"
description: "The rolling Sortino ratio: the mean return in excess of a minimum acceptable return (MAR), divided by the downside deviation, over a trailing window…"
---

## Summary

The rolling Sortino ratio: the mean return in excess of a minimum acceptable return (MAR), divided by the downside deviation, over a trailing window, scaled to a year. It is the Sharpe ratio with upside volatility taken out of the denominator: only returns below the MAR count as risk. Above 0 the window's returns beat the MAR on average; the larger the value, the more return per unit of downside.

## Chart

<ta-chart src="/functions/sortino.svg" ratio="720:416" label="SORTINO over IBM, daily, 1999" alt="Drawn on the price pane over the close as a line; below it, a separate indicator pane with a zero line showing outReal as a solid line."></ta-chart>

Drawn on the price pane over the close as a line; below it, a separate indicator pane with a zero line showing outReal as a solid line. [Reference data (CSV)](/functions/sortino.csv) · [Chart (SVG)](/functions/sortino.svg)

## Formula

With simple returns $r_i = (x_i - x_{i-1}) / x_{i-1}$, per-bar minimum acceptable return $\text{MAR}$, excess returns $d_i = r_i - \text{MAR}$ and annualization $A$, over the trailing $n$ returns:

$$\text{SORTINO} = \sqrt{A}\,\frac{\bar{d}}{DD}, \qquad \bar{d} = \frac{1}{n}\sum d_i, \qquad DD = \sqrt{\frac{1}{n}\sum \min(d_i, 0)^2}$$

The result is $+\infty$ when no return in the window is below the MAR, and NaN when every return equals it.

## Notes

- The downside deviation divides by the full window `n`: a return above the MAR counts in `n` and adds zero to the sum. This is the definition of Sortino and Price (1994) and the divisor of Bacon's worked example; dividing by `n−1`, or by the number of returns below the MAR, gives a different value.
- The mean is arithmetic, not compounded.
- A window with no return below the MAR has no downside: the result is +Inf, and NaN when every return equals the MAR. In a rising market this is common. With an annualization of 0 it is NaN there and 0 elsewhere.
- The MAR is per bar, in the same unit as the returns. For an annual rate `R`, pass `(1+R)^(1/A) − 1`, where `A` is the annualization; it is not de-annualized for you.
- An annualization of 1 gives the raw per-bar ratio.
- A zero price leaves no return to measure from it: the return of the bar after it counts as 0.

## Inputs

- `inReal` — The price series; its returns are computed internally

## Outputs

- `outReal` — Annualized Sortino ratio of the trailing window, or +Inf or NaN where the window has no return below the MAR

## Parameters

| Parameter | Type | Default | Accepted values | Description |
| --- | --- | --- | --- | --- |
| `optInTimePeriod` | integer | 20 | 2–100000 | Number of returns in the window; one price more is read |
| `optInMAR` | real | 0 | any real | Minimum acceptable return per bar; returns below it are the downside |
| `optInAnnualization` | real | 252 | ≥ 0 | Bars per year; the per-bar ratio is multiplied by its square root |

## Properties

**Numerical Stability:** [Start-Independent](/functions/stability.md#start-independent)

<div class="flag-table">

|  |
| :-- |
| <span class="flag-box">☐</span> <span style="opacity:0.5">Overlap Input</span> |
| <span class="flag-box">✅</span> **Independent Y-Axis** <span class="flag-tip" tabindex="0" role="note" aria-label="Output is on its own scale, drawn in a separate pane below the price chart." data-tip="Output is on its own scale, drawn in a separate pane below the price chart.">i</span> |
| <span class="flag-box">☐</span> <span style="opacity:0.5">Candlestick</span> |
| <span class="flag-box">✅</span> **Can Output NaN or ±Inf** <span class="flag-tip" tabindex="0" role="note" aria-label="Some inputs have no finite result, so a successful call can return NaN or ±Inf — a gap with nothing to plot. See Notes for when." data-tip="Some inputs have no finite result, so a successful call can return NaN or ±Inf — a gap with nothing to plot. See Notes for when.">i</span> |
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
TA_SORTINO( 0, n - 1, series, 20, 0.0, 252.0, &outBegIdx, &outNBElement, out );
printf( "%d values from bar %d; last out = %f\n", outNBElement, outBegIdx, out[outNBElement - 1] );
```

@tab Rust

```rust
use ta_lib::Core;

let series = vec![0.0; 252]; // your data, oldest first
let mut out = vec![0.0; 252];

let core = Core::new();
let range = core.sortino(0, series.len() - 1, &series, 20, 0.0, 252.0, &mut out)?;
println!("{} values from bar {}; last out = {}", range.count, range.beg_idx, out[range.count - 1]);
```

@tab Java

```java
import io.github.talib.Core;
import io.github.talib.OutRange;

double[] series = new double[252]; // your data, oldest first
double[] out = new double[252];

Core core = Core.DEFAULT;
OutRange r = core.sortino(0, series.length - 1, series, 20, 0.0, 252.0, out);
System.out.println(r.count() + " values from bar " + r.begIdx() + "; last out = " + out[r.count() - 1]);
```

@tab C\#

```csharp
using System;
using TALib;

var series = new double[252]; // your data, oldest first
var outReal = new double[252];

var core = Core.Default;
OutRange r = core.Sortino(0, series.Length - 1, series, 20, 0.0, 252.0, outReal);
Console.WriteLine($"{r.Count} values from bar {r.BegIdx}; last outReal = {outReal[r.Count - 1]}");
```

@tab TypeScript

```ts
import { sortino } from 'ta-lib';

const series = new Float64Array(252); // your data, oldest first

const { begIdx, values } = sortino(series, {
  timePeriod: 20,
  mar: 0,
  annualization: 252,
});
console.log(values.length + ' values from bar ' + begIdx + '; last values = ' + values[values.length - 1]);
```

:::

## Implementation

Added in 0.8.4.

TA-Lib Definition: [`sortino.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/sortino/sortino.c) · [`sortino.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/sortino/sortino.yaml)

| Native | File |
|--------|------|
| C | [`ta_SORTINO.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_SORTINO.c) |
| Rust | [`sortino.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/library/src/ta_func/sortino.rs) |
| Java | [`Core_SORTINO.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/fragments/Core_SORTINO.java) |
| C# | [`Core_SORTINO.cs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/csharp/library/src/Core_SORTINO.cs) |
| TypeScript | [`sortino.ts`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/ts/library/src/func/sortino.ts) |

Python: `talib.SORTINO` (ta-lib-python 0.8.4 or later), see [ta-lib-python](https://ta-lib.github.io/ta-lib-python/func_groups/statistic_functions.html).

TA-Lib is also available for R and more using a [wrapper](/install/#wrappers).

## Aliases

Sortino Ratio, Rolling Sortino, Downside Risk-Adjusted Return

## See Also

[SHARPE](/functions/sharpe.md) · [STDDEV](/functions/stddev.md) · [VAR](/functions/var.md) · [ROCP](/functions/rocp.md)

## References

- Sortino, F. A. and Price, L. N. "Performance Measurement in a Downside Risk Framework." *The Journal of Investing*, 3(3), Fall 1994, 59-64.
- Bacon, C. R. *Practical Portfolio Performance Measurement and Attribution*, 2nd ed. Wiley, 2008.
