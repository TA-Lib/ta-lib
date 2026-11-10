---
url: 'https://ta-lib.org/functions/ibs.md'
description: 'Internal Bar Strength: where the close sits inside its own bar''s range.'
---
# Internal Bar Strength (IBS)

## Summary

Internal Bar Strength: where the close sits inside its own bar's range. It is 0 when the bar closes at its low, 1 at its high and 0.5 at the midpoint, and it reads one bar only, with no window and no state.

It is the closing half of a stochastic oscillator taken over a single bar, which is how the paper that named the effect describes it. Low readings say the session ended on weakness, high readings on strength, and the published use is mean reversion on the next bar rather than trend confirmation.

## Chart

<ta-chart src="/functions/ibs.svg" ratio="720:416" label="IBS over IBM, daily, 1999" alt="Drawn on the price pane over candles; below it, a separate indicator pane showing outReal as a solid line with dashed rules at 0.2 and 0.8."></ta-chart>

<div class="ta-chart-links">

[Reference data (CSV)](/functions/ibs.csv) · [Chart (SVG)](/functions/ibs.svg)

</div>

::: details Chart metadata <span class="flag-tip" tabindex="0" role="note" aria-label="The chart is drawn automatically from metadata TA-Lib provides. Your app can get the same through the Abstract API." data-tip="The chart is drawn automatically from metadata TA-Lib provides. Your app can get the same through the Abstract API.">i</span>

**Outputs**

* `outReal`: `line` · `zero` · `positive` · `levels: 0.2, 0.8`

:::

## Formula

range\[i] = high\[i] - low\[i]

IBS\[i] = (close\[i] - low\[i]) / range\[i], when range\[i] > 0

IBS\[i] = 0.5, when range\[i] <= 0

## Notes

A bar with no range -- one traded price, or none -- has no inside to locate the close in, and answers 0.5, the bar's own midpoint. The same value is returned for an inverted bar, one whose high is below its low. Zero is not that neutral point: it is the reading for a close at the low, which is the published signal to buy.

The test on the range is exact rather than a tolerance band. A bar's range is the difference of two prices within a factor of two of each other, so it is exact in IEEE arithmetic and is never a residue of cancellation. A fixed band would instead scale with the quote unit and would zero out any instrument quoted small enough to fall under it.

The output is not clamped. A close outside its own bar, which is bad data rather than a market event, passes through as a value below 0 or above 1.

Multiplying by 100 gives the percent form used by some platforms, and a moving average over this output gives the smoothed form.

## Inputs

* `inHigh` — High price of each bar
* `inLow` — Low price of each bar
* `inClose` — Close price of each bar

## Outputs

* `outReal` — Position of the close within the bar's range, 0 to 1 on a bar with range

## Properties

**Numerical Stability:** [Start-Independent](/functions/stability.md#start-independent)

<div class="flag-cols">

<div class="flag-table">

|  |
| :-- |
| <span class="flag-box">☐</span> <span style="opacity:0.5">Overlap Input</span> |
| <span class="flag-box">✅</span> **Independent Y-Axis** <span class="flag-tip" tabindex="0" role="note" aria-label="Output is on its own scale, drawn in a separate pane below the price chart." data-tip="Output is on its own scale, drawn in a separate pane below the price chart.">i</span> |
| <span class="flag-box">☐</span> <span style="opacity:0.5">Candlestick</span> |
| <span class="flag-box">☐</span> <span style="opacity:0.5">Display Shift</span> |

</div>

<div class="flag-table">

|  |
| :-- |
| <span class="flag-box">☐</span> <span style="opacity:0.5">Can Output NaN or ±Inf</span> |
| <span class="flag-box">☐</span> <span style="opacity:0.5">Identity at Period 1</span> |
| <span class="flag-box">☐</span> <span style="opacity:0.5">Uses Transcendental</span> |
| <span class="flag-box">☐</span> <span style="opacity:0.5">Input Must Be Positive</span> |

</div>

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
double close[252]; /* your data, oldest first */
double out[252];
int outBegIdx, outNBElement;

TA_Initialize();
TA_IBS( 0, n - 1, high, low, close, &outBegIdx, &outNBElement, out );
printf( "%d values from bar %d; last out = %f\n", outNBElement, outBegIdx, out[outNBElement - 1] );
```

@tab Rust

```rust
use ta_lib::Core;

let high = vec![0.0; 252]; // your data, oldest first
let low = vec![0.0; 252]; // your data, oldest first
let close = vec![0.0; 252]; // your data, oldest first
let mut out = vec![0.0; 252];

let core = Core::new();
let range = core.ibs(0, high.len() - 1, &high, &low, &close, &mut out)?;
println!("{} values from bar {}; last out = {}", range.count, range.beg_idx, out[range.count - 1]);
```

@tab Java

```java
import io.github.talib.Core;
import io.github.talib.OutRange;

double[] high = new double[252]; // your data, oldest first
double[] low = new double[252]; // your data, oldest first
double[] close = new double[252]; // your data, oldest first
double[] out = new double[252];

Core core = Core.DEFAULT;
OutRange r = core.ibs(0, high.length - 1, high, low, close, out);
System.out.println(r.count() + " values from bar " + r.begIdx() + "; last out = " + out[r.count() - 1]);
```

@tab C#

```csharp
using System;
using TALib;

var high = new double[252]; // your data, oldest first
var low = new double[252]; // your data, oldest first
var close = new double[252]; // your data, oldest first
var outReal = new double[252];

var core = Core.Default;
OutRange r = core.Ibs(0, high.Length - 1, high, low, close, outReal);
Console.WriteLine($"{r.Count} values from bar {r.BegIdx}; last outReal = {outReal[r.Count - 1]}");
```

@tab TypeScript

```ts
import { ibs } from 'ta-lib';

const high = new Float64Array(252); // your data, oldest first
const low = new Float64Array(252); // your data, oldest first
const close = new Float64Array(252); // your data, oldest first

const { begIdx, values } = ibs({ high, low, close });
console.log(values.length + ' values from bar ' + begIdx + '; last values = ' + values[values.length - 1]);
```

:::

## Implementation

Added in 0.8.3 (ta-lib-python 0.8.3).

TA-Lib Definition: [`ibs.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/ibs/ibs.c) · [`ibs.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/ibs/ibs.yaml)

| Native | File |
|--------|------|
| C | [`ta_IBS.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_IBS.c) |
| Rust | [`ibs.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/library/src/ta_func/ibs.rs) |
| Java | [`Core_IBS.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/fragments/Core_IBS.java) |
| C# | [`Core_IBS.cs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/csharp/library/src/Core_IBS.cs) |
| TypeScript | [`ibs.ts`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/ts/library/src/func/ibs.ts) |

Python: `talib.IBS` (ta-lib-python 0.8.3 or later), see [ta-lib-python](https://ta-lib.github.io/ta-lib-python/func_groups/momentum_indicators.html).

TA-Lib is also available for R and more using a [wrapper](/install/#wrappers).

## Aliases

Internal Bar Strength, Closing Range

## See Also

* [BOP](bop.md) — the same bar's range, measuring the close against the open instead of the low
* [STOCHF](stochf.md) — the same ratio over a window of bars, on a 0 to 100 scale
* [PERCENTB](percentb.md) — the position of the close inside Bollinger Bands rather than inside the bar

## References

* Alexander Soffronow Pagonidis, "The IBS Effect: Mean Reversion in Equity ETFs", 2013 — names the effect, gives the formula and the 0.2 / 0.8 thresholds.
* Pandey and Joshi, "Using Internal Bar Strength as a Key Indicator for Trading Country ETFs", arXiv:2306.12434, 2023.
