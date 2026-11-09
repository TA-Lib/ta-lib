---
url: 'https://ta-lib.org/functions/sharpe.md'
description: >-
  The rolling Sharpe ratio: the mean return in excess of a risk-free rate,
  divided by the standard deviation of that excess return, over a trailing
  window.
---
# Rolling Sharpe Ratio (SHARPE)

## Summary

The rolling Sharpe ratio: the mean return in excess of a risk-free rate, divided by the standard deviation of that excess return, over a trailing window. A drift-over-volatility measure of the series itself, scaled to a year. Above 0 the window's returns beat the risk-free rate on average; the larger the value, the more return per unit of volatility.

## Chart

<ta-chart src="/functions/sharpe.svg" ratio="720:416" label="SHARPE over IBM, daily, 1999" alt="Drawn on the price pane over the close as a line; below it, a separate indicator pane with a zero line showing outReal as a solid line."></ta-chart>

::: details Chart metadata <span class="flag-tip" tabindex="0" role="note" aria-label="The chart is drawn from the function's metadata: the flags and attributes the abstract API reports for it and for each output, and the input series its reference run declares." data-tip="The chart is drawn from the function's metadata: the flags and attributes the abstract API reports for it and for each output, and the input series its reference run declares.">i</span>

Drawn on the price pane over the close as a line; below it, a separate indicator pane with a zero line showing outReal as a solid line.

Metadata flags used: `outReal: line, positive, negative`

:::

[Reference data (CSV)](/functions/sharpe.csv) · [Chart (SVG)](/functions/sharpe.svg)

## Formula

With simple returns $r_i = (x_i - x_{i-1}) / x_{i-1}$, per-bar risk-free rate $R_f$, excess returns $d_i = r_i - R_f$ and annualization $A$, over the trailing $n$ returns:

$\text{SHARPE} = \sqrt{A}\,\frac{\bar{d}}{\sigma_d}, \qquad \bar{d} = \frac{1}{n}\sum d_i, \qquad \sigma_d = \sqrt{\frac{1}{n-1}\sum (d_i - \bar{d})^2}$

The result is $\pm\infty$ by the sign of $\bar{d}$ when every excess return in the window is equal, and NaN when they are also exactly 0.

## Notes

* This is Sharpe's 1994 *ex post* ratio with a constant benchmark, over simple returns and the sample (`n−1`) standard deviation.
* A window whose returns are all equal has no spread: the result is +Inf or -Inf by the sign of the mean excess return, and NaN when that mean is exactly 0. With an annualization of 0 it is NaN there and 0 elsewhere.
* The risk-free rate is per bar, in the same unit as the returns. For an annual rate `R`, pass `(1+R)^(1/A) − 1`, where `A` is the annualization; it is not de-annualized for you.
* An annualization of 1 gives the raw per-bar ratio.
* A zero price leaves no return to measure from it: the return of the bar after it counts as 0.

## Inputs

* `inReal` — The price series; its returns are computed internally

## Outputs

* `outReal` — Annualized Sharpe ratio of the trailing window, or ±Inf or NaN where the window has no spread

## Parameters

| Parameter | Type | Default | Accepted values | Description |
| --- | --- | --- | --- | --- |
| `optInTimePeriod` | integer | 20 | 2–100000 | Number of returns in the window; one price more is read |
| `optInRiskFreeRate` | real | 0 | any real | Risk-free return per bar, subtracted from each return |
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
TA_SHARPE( 0, n - 1, series, 20, 0.0, 252.0, &outBegIdx, &outNBElement, out );
printf( "%d values from bar %d; last out = %f\n", outNBElement, outBegIdx, out[outNBElement - 1] );
```

@tab Rust

```rust
use ta_lib::Core;

let series = vec![0.0; 252]; // your data, oldest first
let mut out = vec![0.0; 252];

let core = Core::new();
let range = core.sharpe(0, series.len() - 1, &series, 20, 0.0, 252.0, &mut out)?;
println!("{} values from bar {}; last out = {}", range.count, range.beg_idx, out[range.count - 1]);
```

@tab Java

```java
import io.github.talib.Core;
import io.github.talib.OutRange;

double[] series = new double[252]; // your data, oldest first
double[] out = new double[252];

Core core = Core.DEFAULT;
OutRange r = core.sharpe(0, series.length - 1, series, 20, 0.0, 252.0, out);
System.out.println(r.count() + " values from bar " + r.begIdx() + "; last out = " + out[r.count() - 1]);
```

@tab C#

```csharp
using System;
using TALib;

var series = new double[252]; // your data, oldest first
var outReal = new double[252];

var core = Core.Default;
OutRange r = core.Sharpe(0, series.Length - 1, series, 20, 0.0, 252.0, outReal);
Console.WriteLine($"{r.Count} values from bar {r.BegIdx}; last outReal = {outReal[r.Count - 1]}");
```

@tab TypeScript

```ts
import { sharpe } from 'ta-lib';

const series = new Float64Array(252); // your data, oldest first

const { begIdx, values } = sharpe(series, {
  timePeriod: 20,
  riskFreeRate: 0,
  annualization: 252,
});
console.log(values.length + ' values from bar ' + begIdx + '; last values = ' + values[values.length - 1]);
```

:::

## Implementation

Added in 0.8.4.

TA-Lib Definition: [`sharpe.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/sharpe/sharpe.c) · [`sharpe.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/sharpe/sharpe.yaml)

| Native | File |
|--------|------|
| C | [`ta_SHARPE.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_SHARPE.c) |
| Rust | [`sharpe.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/library/src/ta_func/sharpe.rs) |
| Java | [`Core_SHARPE.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/fragments/Core_SHARPE.java) |
| C# | [`Core_SHARPE.cs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/csharp/library/src/Core_SHARPE.cs) |
| TypeScript | [`sharpe.ts`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/ts/library/src/func/sharpe.ts) |

Python: `talib.SHARPE`, see [ta-lib-python](https://ta-lib.github.io/ta-lib-python/func_groups/statistic_functions.html).

TA-Lib is also available for R and more using a [wrapper](/install/#wrappers).

## Aliases

Sharpe Ratio, Reward-to-Variability Ratio, Rolling Sharpe

## See Also

[STDDEV](/functions/stddev.md) · [VAR](/functions/var.md) · [BETA](/functions/beta.md) · [ROCP](/functions/rocp.md)

## References

* Sharpe, W. F. "The Sharpe Ratio." *The Journal of Portfolio Management*, 21(1), Fall 1994, 49-58.
