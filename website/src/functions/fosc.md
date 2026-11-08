---
title: "Forecast Oscillator (FOSC)"
description: "Forecast Oscillator: the percentage by which the close deviates from the Time Series Forecast that was made one bar earlier for the current bar."
---

## Summary

Forecast Oscillator: the percentage by which the close deviates from the Time Series Forecast that was made one bar earlier for the current bar.

Positive means price came in above what the regression projected, negative below; the value oscillates around zero and crosses it whenever price meets its own forecast. Persistent readings far from zero say the trend is running ahead of, or lagging, its regression line.

## Chart

<ta-chart src="/functions/fosc.svg" ratio="720:416" label="FOSC over IBM, daily, 1999" alt="Drawn on the price pane over the close as a line; below it, a separate indicator pane with a zero line showing outReal as a solid line."></ta-chart>

::: details Chart metadata <span class="flag-tip" tabindex="0" role="note" aria-label="The chart is drawn from the function's metadata: the flags and attributes the abstract API reports for it and for each output, and the input series its reference run declares." data-tip="The chart is drawn from the function's metadata: the flags and attributes the abstract API reports for it and for each output, and the input series its reference run declares.">i</span>

Drawn on the price pane over the close as a line; below it, a separate indicator pane with a zero line showing outReal as a solid line.

Metadata flags used: `outReal: line`

:::

[Reference data (CSV)](/functions/fosc.csv) · [Chart (SVG)](/functions/fosc.svg)

## Formula

FOSC[t] = 100 * (P[t] - TSF[t-1]) / P[t], where TSF[t-1] is the Time Series Forecast fitted over the N bars ending at t-1 and evaluated one x-step beyond that window — the forecast for bar t made without seeing it.

## Notes

- **The input is assumed positive.** The current value is the divisor. Where it is exactly 0 the output is 0. Near zero the divisor is small and the output very large; no error is returned. A series that reaches or crosses zero, such as an oscillator or a spread, has no meaningful result here.
- Several vendors publish a "Chande Forecast Oscillator (CFO)" that compares the close to the regression value of the window *ending at the same bar*, with no lag. FOSC is the lagged form Chande and Achelis describe.
- The default window is Chande's own suggestion, shorter than the one TA-Lib's TSF and LINEARREG default to.

## Inputs

- `inReal` — Source price/value series

## Outputs

- `outReal` — Percentage deviation of the close from the previous bar's forecast

## Parameters

| Parameter | Type | Default | Accepted values | Description |
| --- | --- | --- | --- | --- |
| `optInTimePeriod` | integer | 5 | 2–100000 | Number of bars in the regression window |

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
| <span class="flag-box">✅</span> **Input Must Be Positive** <span class="flag-tip" tabindex="0" role="note" aria-label="Assumes the input is positive, as a price is. A series that reaches or crosses zero, such as an oscillator or a spread, returns success and an output that may have no meaning. See Notes for what a zero or a negative value does." data-tip="Assumes the input is positive, as a price is. A series that reaches or crosses zero, such as an oscillator or a spread, returns success and an output that may have no meaning. See Notes for what a zero or a negative value does.">i</span> |

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
TA_FOSC( 0, n - 1, series, 5, &outBegIdx, &outNBElement, out );
printf( "%d values from bar %d; last out = %f\n", outNBElement, outBegIdx, out[outNBElement - 1] );
```

@tab Rust

```rust
use ta_lib::Core;

let series = vec![0.0; 252]; // your data, oldest first
let mut out = vec![0.0; 252];

let core = Core::new();
let range = core.fosc(0, series.len() - 1, &series, 5, &mut out)?;
println!("{} values from bar {}; last out = {}", range.count, range.beg_idx, out[range.count - 1]);
```

@tab Java

```java
import io.github.talib.Core;
import io.github.talib.OutRange;

double[] series = new double[252]; // your data, oldest first
double[] out = new double[252];

Core core = Core.DEFAULT;
OutRange r = core.fosc(0, series.length - 1, series, 5, out);
System.out.println(r.count() + " values from bar " + r.begIdx() + "; last out = " + out[r.count() - 1]);
```

@tab C\#

```csharp
using System;
using TALib;

var series = new double[252]; // your data, oldest first
var outReal = new double[252];

var core = Core.Default;
OutRange r = core.Fosc(0, series.Length - 1, series, 5, outReal);
Console.WriteLine($"{r.Count} values from bar {r.BegIdx}; last outReal = {outReal[r.Count - 1]}");
```

@tab TypeScript

```ts
import { fosc } from 'ta-lib';

const series = new Float64Array(252); // your data, oldest first

const { begIdx, values } = fosc(series, { timePeriod: 5 });
console.log(values.length + ' values from bar ' + begIdx + '; last values = ' + values[values.length - 1]);
```

:::

## Implementation

Added in 0.8.1 (ta-lib-python 0.8.1).

TA-Lib Definition: [`fosc.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/fosc/fosc.c) · [`fosc.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/fosc/fosc.yaml)

| Native | File |
|--------|------|
| C | [`ta_FOSC.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_FOSC.c) |
| Rust | [`fosc.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/library/src/ta_func/fosc.rs) |
| Java | [`Core_FOSC.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/fragments/Core_FOSC.java) |
| C# | [`Core_FOSC.cs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/csharp/library/src/Core_FOSC.cs) |
| TypeScript | [`fosc.ts`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/ts/library/src/func/fosc.ts) |

Python: `talib.FOSC` (ta-lib-python 0.8.1 or later), see [ta-lib-python](https://ta-lib.github.io/ta-lib-python/func_groups/momentum_indicators.html).

TA-Lib is also available for R and more using a [wrapper](/install/#wrappers).

## Aliases

Forecast Oscillator

## See Also

[TSF](/functions/tsf.md) · [LINEARREG](/functions/linearreg.md) · [CMOU](/functions/cmou.md)

## References

- Tushar S. Chande and Stanley Kroll, *The New Technical Trader*, John Wiley & Sons (ISBN 0471597805)
- Steven B. Achelis, *Technical Analysis from A to Z*, page 147
