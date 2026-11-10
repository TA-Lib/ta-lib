---
url: 'https://ta-lib.org/functions/cg.md'
description: >-
  John Ehlers' Center of Gravity oscillator: the balance point of the last
  optInTimePeriod values, each weighted by its value and placed at its position…
---
# Center of Gravity Oscillator (CG)

## Summary

John Ehlers' Center of Gravity oscillator: the balance point of the last `optInTimePeriod` values, each weighted by its value and placed at its position counting back from the current bar, which is position 1, negated so that it rises with price. It is smooth and has essentially no lag. Ehlers reads turning points from it and trades its crossings with a copy of itself delayed by one bar. For a positive series it stays between -optInTimePeriod and -1, and a flat window sits at the midpoint, -(optInTimePeriod+1)/2.

## Chart

<ta-chart src="/functions/cg.svg" ratio="720:416" label="CG over IBM, daily, 1999" alt="Drawn on the price pane over the close as a line; below it, a separate indicator pane showing outReal as a solid line."></ta-chart>

<div class="ta-chart-links">

[Reference data (CSV)](/functions/cg.csv) · [Chart (SVG)](/functions/cg.svg)

</div>

::: details Chart metadata <span class="flag-tip" tabindex="0" role="note" aria-label="The chart is drawn automatically from metadata TA-Lib provides. Your app can get the same through the Abstract API." data-tip="The chart is drawn automatically from metadata TA-Lib provides. Your app can get the same through the Abstract API.">i</span>

**Outputs**

* `outReal`: `line` · `negative`

:::

## Formula

$$
\mathrm{CG}_t = -\frac{\sum_{i=0}^{n-1} (i+1)\,X_{t-i}}{\sum_{i=0}^{n-1} X_{t-i}}, \text{ or } -\frac{n+1}{2} \text{ when } \sum_{i=0}^{n-1} X_{t-i} = 0
$$

where $X$ is the input series, $n$ is `optInTimePeriod` and $i$ counts bars back from the current bar, which carries the smallest weight.

## Notes

* **The input is assumed positive.** The sum of the window is the divisor. Near zero the divisor is small and the output very large; no error is returned. A series that reaches or crosses zero, such as an oscillator or a spread, has no meaningful result here.
* Ehlers' default input is the median price (H+L)/2: pass the output of MEDPRICE to reproduce it.
* His 2004 book presents the same oscillator shifted up by (optInTimePeriod+1)/2, so that a flat window reads 0: add (optInTimePeriod+1)/2 to the output to obtain that form.
* Where the window sums to exactly zero the author's listing keeps its previous value; TA-Lib returns -(optInTimePeriod+1)/2, so every value depends on its own window alone.
* Both sums are exact before the divide, so a window summing to exactly zero is always recognised and the value does not depend on where the call started. The exception is a window holding a non-finite value, a value with bits below 2^-1022 (every subnormal, and only values under about 2e-292), or values that together span more binary digits than the sums can hold exactly (97 at the default period, 84 at 1000, 58 at 100000): it is summed in floating point, oldest value first.

## Inputs

* `inReal` — The series to measure

## Outputs

* `outReal` — Negated center of gravity of the window

## Parameters

| Parameter | Type | Default | Accepted values | Description |
| --- | --- | --- | --- | --- |
| `optInTimePeriod` | integer | 10 | 2–100000 | Number of bars in the window |

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
| <span class="flag-box">✅</span> **Input Must Be Positive** <span class="flag-tip" tabindex="0" role="note" aria-label="Assumes the input is positive, as a price is. A series that reaches or crosses zero, such as an oscillator or a spread, returns success and an output that may have no meaning. See Notes for what a zero or a negative value does." data-tip="Assumes the input is positive, as a price is. A series that reaches or crosses zero, such as an oscillator or a spread, returns success and an output that may have no meaning. See Notes for what a zero or a negative value does.">i</span> |

</div>

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
TA_CG( 0, n - 1, series, 10, &outBegIdx, &outNBElement, out );
printf( "%d values from bar %d; last out = %f\n", outNBElement, outBegIdx, out[outNBElement - 1] );
```

@tab Rust

```rust
use ta_lib::Core;

let series = vec![0.0; 252]; // your data, oldest first
let mut out = vec![0.0; 252];

let core = Core::new();
let range = core.cg(0, series.len() - 1, &series, 10, &mut out)?;
println!("{} values from bar {}; last out = {}", range.count, range.beg_idx, out[range.count - 1]);
```

@tab Java

```java
import io.github.talib.Core;
import io.github.talib.OutRange;

double[] series = new double[252]; // your data, oldest first
double[] out = new double[252];

Core core = Core.DEFAULT;
OutRange r = core.cg(0, series.length - 1, series, 10, out);
System.out.println(r.count() + " values from bar " + r.begIdx() + "; last out = " + out[r.count() - 1]);
```

@tab C#

```csharp
using System;
using TALib;

var series = new double[252]; // your data, oldest first
var outReal = new double[252];

var core = Core.Default;
OutRange r = core.Cg(0, series.Length - 1, series, 10, outReal);
Console.WriteLine($"{r.Count} values from bar {r.BegIdx}; last outReal = {outReal[r.Count - 1]}");
```

@tab TypeScript

```ts
import { cg } from 'ta-lib';

const series = new Float64Array(252); // your data, oldest first

const { begIdx, values } = cg(series, { timePeriod: 10 });
console.log(values.length + ' values from bar ' + begIdx + '; last values = ' + values[values.length - 1]);
```

:::

## Implementation

Added in 0.8.3 (ta-lib-python 0.8.3).

TA-Lib Definition: [`cg.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/cg/cg.c) · [`cg.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/cg/cg.yaml)

| Native | File |
|--------|------|
| C | [`ta_CG.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_CG.c) |
| Rust | [`cg.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/library/src/ta_func/cg.rs) |
| Java | [`Core_CG.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/fragments/Core_CG.java) |
| C# | [`Core_CG.cs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/csharp/library/src/Core_CG.cs) |
| TypeScript | [`cg.ts`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/ts/library/src/func/cg.ts) |

Python: `talib.CG` (ta-lib-python 0.8.3 or later), see [ta-lib-python](https://ta-lib.github.io/ta-lib-python/func_groups/momentum_indicators.html).

TA-Lib is also available for R and more using a [wrapper](/install/#wrappers).

## Aliases

Center of Gravity Oscillator, COG

## See Also

[WMA](/functions/wma.md) · [MEDPRICE](/functions/medprice.md) · [CTI](/functions/cti.md)

## References

* John F. Ehlers, "The CG Oscillator", MESA Software, [author's PDF](https://mesasoftware.com/papers/TheCGOscillator.pdf); published as "The Center Of Gravity Oscillator", *Technical Analysis of Stocks & Commodities*, May 2002
* John F. Ehlers, *Cybernetic Analysis for Stocks and Futures*, Wiley, 2004, chapter 5
