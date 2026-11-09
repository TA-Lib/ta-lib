---
url: 'https://ta-lib.org/functions/rogerssatchell.md'
description: >-
  Rogers-Satchell volatility: a range-based estimator that reads one bar's open,
  high, low and close as a single unbiased estimate of that bar's variance…
---
# Rogers-Satchell Volatility (ROGERSSATCHELL)

## Summary

Rogers-Satchell volatility: a range-based estimator that reads one bar's open, high, low and close as a single unbiased estimate of that bar's variance, then reports the root of the mean over the last `n` bars, scaled to periods per year.

What separates it from the other range estimators is that it is unbiased **whatever the drift**. A bar that opens at its low and closes at its high has travelled in one direction and dispersed nothing around that path, and this estimator reads it as exactly zero, where Parkinson and Garman-Klass read a wide range as volatility. The price of that is a blind spot of its own: the estimator has no close-to-open term, so overnight gaps are invisible to it.

Read the output as a fraction in log-return units, not price units and not percent. At an `optInAnnualization` of 252 it is an annualised figure for daily bars; pass 1 to leave the per-bar figure, 52 for weekly bars, 12 for monthly.

## Chart

<ta-chart src="/functions/rogerssatchell.svg" ratio="720:416" label="ROGERSSATCHELL over IBM, daily, 1999" alt="Drawn on the price pane over candles; below it, a separate indicator pane showing outReal as a solid line."></ta-chart>

::: details Chart metadata <span class="flag-tip" tabindex="0" role="note" aria-label="The chart is drawn from the function's metadata: the flags and attributes the abstract API reports for it and for each output, and the input series its reference run declares." data-tip="The chart is drawn from the function's metadata: the flags and attributes the abstract API reports for it and for each output, and the input series its reference run declares.">i</span>

Drawn on the price pane over candles; below it, a separate indicator pane showing outReal as a solid line.

Metadata flags used: `inPriceOHLC: open, high, low, close` · `outReal: line`

:::

[Reference data (CSV)](/functions/rogerssatchell.csv) · [Chart (SVG)](/functions/rogerssatchell.svg)

## Formula

```
S1[i] = ln(High[i]/Open[i])     I1[i] = ln(Low[i]/Open[i])     X1[i] = ln(Close[i]/Open[i])

term[i] = S1[i] * (S1[i] - X1[i]) + I1[i] * (I1[i] - X1[i])
        = ln(High[i]/Close[i]) * ln(High[i]/Open[i])
        + ln(Low[i]/Close[i])  * ln(Low[i]/Open[i])

S[i]    = term[i-n+1] + ... + term[i]

out[i]  = sqrt(A) * sqrt(S[i] / n)      when S[i] > 0
        = 0                             otherwise
```

with `n` = `optInTimePeriod` and `A` = `optInAnnualization`.

## Notes

The estimator is defined on a consistent bar, one whose low is at or below both the open and the close and whose high is at or above both. Such a bar always gives a term at or above zero. A bar whose high or low sits strictly between its open and close can give a negative term; the window sum is then floored at zero rather than rooted, as VAR floors its variance so STDDEV can root it unconditionally.

A bar carrying a price at or below zero contributes a zero term and still counts toward the window's `n`. The test is exact rather than against a fixed band, so an instrument quoted in small units is not zeroed out by the threshold.

The window sum is carried from bar to bar and rebuilt as a fresh sum whenever it collapses relative to the largest it has held, and periodically otherwise. A window of flat bars therefore reads exactly zero; a sum that is only ever added to and subtracted from would leave the rounding of earlier bars behind instead.

`optInAnnualization` is applied as `sqrt(A)` computed once and multiplied in at the end, so the annualised output is exactly `sqrt(A)` times the per-bar one, and `A = 1` is an identity rather than a rounding.

Every term reads only its own bar, so the lookback is `n - 1`: the first output lands on bar `n - 1`. An estimator that measures the bar against the previous close consumes one more bar and starts at `n`.

## Inputs

* `inOpen` — Open price of each bar
* `inHigh` — High price of each bar
* `inLow` — Low price of each bar
* `inClose` — Close price of each bar

## Outputs

* `outReal` — Estimated volatility, in log-return units

## Parameters

| Parameter | Type | Default | Accepted values | Description |
| --- | --- | --- | --- | --- |
| `optInTimePeriod` | integer | 10 | 1–100000 | Number of bars in the window. `n = 1` is the paper's own single-bar estimator. |
| `optInAnnualization` | real | 252 | ≥ 0 | Periods per year. Pass 1 for the per-bar figure. |

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
| <span class="flag-box">✅</span> **Uses Transcendental** <span class="flag-tip" tabindex="0" role="note" aria-label="Calls a transcendental math function (exp, log, log10, trigonometric, inverse trigonometric or hyperbolic), so a value may differ slightly between languages, between a stream and a batch call, and between machines. TA-Lib targets a difference below 1e-9." data-tip="Calls a transcendental math function (exp, log, log10, trigonometric, inverse trigonometric or hyperbolic), so a value may differ slightly between languages, between a stream and a batch call, and between machines. TA-Lib targets a difference below 1e-9.">i</span> |
| <span class="flag-box">✅</span> **Input Must Be Positive** <span class="flag-tip" tabindex="0" role="note" aria-label="Assumes the input is positive, as a price is. A series that reaches or crosses zero, such as an oscillator or a spread, returns success and an output that may have no meaning. See Notes for what a zero or a negative value does." data-tip="Assumes the input is positive, as a price is. A series that reaches or crosses zero, such as an oscillator or a spread, returns success and an output that may have no meaning. See Notes for what a zero or a negative value does.">i</span> |

</div>

## Example

::: code-tabs#lang

@tab C

```c
#include <stdio.h>
#include <ta_libc.h>

int n = 252;
double open[252]; /* your data, oldest first */
double high[252]; /* your data, oldest first */
double low[252]; /* your data, oldest first */
double close[252]; /* your data, oldest first */
double out[252];
int outBegIdx, outNBElement;

TA_Initialize();
TA_ROGERSSATCHELL( 0, n - 1, open, high, low, close, 10, 252.0, &outBegIdx, &outNBElement, out );
printf( "%d values from bar %d; last out = %f\n", outNBElement, outBegIdx, out[outNBElement - 1] );
```

@tab Rust

```rust
use ta_lib::Core;

let open = vec![0.0; 252]; // your data, oldest first
let high = vec![0.0; 252]; // your data, oldest first
let low = vec![0.0; 252]; // your data, oldest first
let close = vec![0.0; 252]; // your data, oldest first
let mut out = vec![0.0; 252];

let core = Core::new();
let range = core.rogerssatchell(0, open.len() - 1, &open, &high, &low, &close, 10, 252.0,
                                &mut out)?;
println!("{} values from bar {}; last out = {}", range.count, range.beg_idx, out[range.count - 1]);
```

@tab Java

```java
import io.github.talib.Core;
import io.github.talib.OutRange;

double[] open = new double[252]; // your data, oldest first
double[] high = new double[252]; // your data, oldest first
double[] low = new double[252]; // your data, oldest first
double[] close = new double[252]; // your data, oldest first
double[] out = new double[252];

Core core = Core.DEFAULT;
OutRange r = core.rogerssatchell(0, open.length - 1, open, high, low, close, 10, 252.0, out);
System.out.println(r.count() + " values from bar " + r.begIdx() + "; last out = " + out[r.count() - 1]);
```

@tab C#

```csharp
using System;
using TALib;

var open = new double[252]; // your data, oldest first
var high = new double[252]; // your data, oldest first
var low = new double[252]; // your data, oldest first
var close = new double[252]; // your data, oldest first
var outReal = new double[252];

var core = Core.Default;
OutRange r = core.Rogerssatchell(0, open.Length - 1, open, high, low, close, 10, 252.0, outReal);
Console.WriteLine($"{r.Count} values from bar {r.BegIdx}; last outReal = {outReal[r.Count - 1]}");
```

@tab TypeScript

```ts
import { rogerssatchell } from 'ta-lib';

const open = new Float64Array(252); // your data, oldest first
const high = new Float64Array(252); // your data, oldest first
const low = new Float64Array(252); // your data, oldest first
const close = new Float64Array(252); // your data, oldest first

const { begIdx, values } = rogerssatchell({ open, high, low, close }, {
  timePeriod: 10,
  annualization: 252,
});
console.log(values.length + ' values from bar ' + begIdx + '; last values = ' + values[values.length - 1]);
```

:::

## Implementation

Added in 0.8.3 (ta-lib-python 0.8.3).

TA-Lib Definition: [`rogerssatchell.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/rogerssatchell/rogerssatchell.c) · [`rogerssatchell.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/rogerssatchell/rogerssatchell.yaml)

| Native | File |
|--------|------|
| C | [`ta_ROGERSSATCHELL.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_ROGERSSATCHELL.c) |
| Rust | [`rogerssatchell.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/library/src/ta_func/rogerssatchell.rs) |
| Java | [`Core_ROGERSSATCHELL.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/fragments/Core_ROGERSSATCHELL.java) |
| C# | [`Core_ROGERSSATCHELL.cs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/csharp/library/src/Core_ROGERSSATCHELL.cs) |
| TypeScript | [`rogerssatchell.ts`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/ts/library/src/func/rogerssatchell.ts) |

Python: `talib.ROGERSSATCHELL` (ta-lib-python 0.8.3 or later), see [ta-lib-python](https://ta-lib.github.io/ta-lib-python/func_groups/volatility_indicators.html).

TA-Lib is also available for R and more using a [wrapper](/install/#wrappers).

## Aliases

Rogers-Satchell Volatility, Rogers Satchell, RSV

## See Also

[ATR](/functions/atr.md) · [NATR](/functions/natr.md) · [VAR](/functions/var.md)

## References

* Rogers, L. C. G. and Satchell, S. E. "Estimating Variance From High, Low and Closing Prices." *The Annals of Applied Probability* 1(4):504-512, November 1991. doi:10.1214/aoap/1177005835
