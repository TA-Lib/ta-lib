---
title: "Fractal Adaptive Moving Average (FRAMA)"
description: "Fractal Adaptive Moving Average (John Ehlers): an EMA whose smoothing factor adapts each bar to the fractal dimension of the window, estimated from the…"
---

## Summary

Fractal Adaptive Moving Average (John Ehlers): an EMA whose smoothing factor adapts each bar to the fractal dimension of the window, estimated from the high-low ranges of the window and of its two halves. A straight run gives dimension 1 and the output follows the price; dense congestion gives dimension 2 and very slow smoothing.

## Chart

<ta-chart src="/functions/frama.svg" ratio="720:274" label="FRAMA over IBM, daily, 1999" alt="Drawn on the price pane over candles, with outReal as a solid line."></ta-chart>

::: details Chart metadata <span class="flag-tip" tabindex="0" role="note" aria-label="The chart is drawn automatically from metadata TA-Lib provides. Your app can get the same through the Abstract API." data-tip="The chart is drawn automatically from metadata TA-Lib provides. Your app can get the same through the Abstract API.">i</span>

Drawn on the price pane over candles, with outReal as a solid line.

Metadata flags used: `overlap` · `inPriceHL: high, low` · `outReal: line`

:::

[Reference data (CSV)](/functions/frama.csv) · [Chart (SVG)](/functions/frama.svg)

## Formula

P[t] = (High[t] + Low[t]) / 2
R1 = range of the newer half, R2 = range of the older half, R = range of the whole window
D = 1 + log2((R1 + R2) / R)
alpha = exp(-4.6 * (D - 1)), at most 1; alpha = 1 when R1 or R2 is 0
FRAMA[t] = alpha * P[t] + (1 - alpha) * FRAMA[t-1], seeded with P on the bar before the first output

## Notes

- Where either half of the window is flat, alpha is 1 and the output is the price; Ehlers' listing keeps the previous bar's dimension there instead.
- The period must be even; an odd period is rejected.

## Inputs

- `inHigh` — High price of each bar
- `inLow` — Low price of each bar

## Outputs

- `outReal` — Adaptive moving average line

## Parameters

| Parameter | Type | Default | Accepted values | Description |
| --- | --- | --- | --- | --- |
| `optInTimePeriod` | integer | 16 | 2–100000 | Number of bars in the window, split into two equal halves |

## Properties

**Numerical Stability:** [Initial Unstable Period](/functions/stability.md#initial-unstable-period)

<div class="flag-table">

|  |
| :-- |
| <span class="flag-box">✅</span> **Overlap Input** <span class="flag-tip" tabindex="0" role="note" aria-label="Output is on the same scale as the input price, so it is drawn over the price chart." data-tip="Output is on the same scale as the input price, so it is drawn over the price chart.">i</span> |
| <span class="flag-box">☐</span> <span style="opacity:0.5">Independent Y-Axis</span> |
| <span class="flag-box">☐</span> <span style="opacity:0.5">Candlestick</span> |
| <span class="flag-box">☐</span> <span style="opacity:0.5">Can Output NaN or ±Inf</span> |
| <span class="flag-box">☐</span> <span style="opacity:0.5">Identity at Period 1</span> |
| <span class="flag-box">☐</span> <span style="opacity:0.5">Display Shift</span> |
| <span class="flag-box">✅</span> **Uses Transcendental** <span class="flag-tip" tabindex="0" role="note" aria-label="Calls a transcendental math function (exp, log, log10, trigonometric, inverse trigonometric or hyperbolic), so a value may differ slightly between languages, between a stream and a batch call, and between machines. TA-Lib targets a difference below 1e-9." data-tip="Calls a transcendental math function (exp, log, log10, trigonometric, inverse trigonometric or hyperbolic), so a value may differ slightly between languages, between a stream and a batch call, and between machines. TA-Lib targets a difference below 1e-9.">i</span> |
| <span class="flag-box">☐</span> <span style="opacity:0.5">Input Must Be Positive</span> |

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
double out[252];
int outBegIdx, outNBElement;

TA_Initialize();
TA_FRAMA( 0, n - 1, high, low, 16, &outBegIdx, &outNBElement, out );
printf( "%d values from bar %d; last out = %f\n", outNBElement, outBegIdx, out[outNBElement - 1] );
```

@tab Rust

```rust
use ta_lib::Core;

let high = vec![0.0; 252]; // your data, oldest first
let low = vec![0.0; 252]; // your data, oldest first
let mut out = vec![0.0; 252];

let core = Core::new();
let range = core.frama(0, high.len() - 1, &high, &low, 16, &mut out)?;
println!("{} values from bar {}; last out = {}", range.count, range.beg_idx, out[range.count - 1]);
```

@tab Java

```java
import io.github.talib.Core;
import io.github.talib.OutRange;

double[] high = new double[252]; // your data, oldest first
double[] low = new double[252]; // your data, oldest first
double[] out = new double[252];

Core core = Core.DEFAULT;
OutRange r = core.frama(0, high.length - 1, high, low, 16, out);
System.out.println(r.count() + " values from bar " + r.begIdx() + "; last out = " + out[r.count() - 1]);
```

@tab C\#

```csharp
using System;
using TALib;

var high = new double[252]; // your data, oldest first
var low = new double[252]; // your data, oldest first
var outReal = new double[252];

var core = Core.Default;
OutRange r = core.Frama(0, high.Length - 1, high, low, 16, outReal);
Console.WriteLine($"{r.Count} values from bar {r.BegIdx}; last outReal = {outReal[r.Count - 1]}");
```

@tab TypeScript

```ts
import { frama } from 'ta-lib';

const high = new Float64Array(252); // your data, oldest first
const low = new Float64Array(252); // your data, oldest first

const { begIdx, values } = frama({ high, low }, { timePeriod: 16 });
console.log(values.length + ' values from bar ' + begIdx + '; last values = ' + values[values.length - 1]);
```

:::

## Implementation

Added in 0.8.3 (ta-lib-python 0.8.3).

TA-Lib Definition: [`frama.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/frama/frama.c) · [`frama.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/frama/frama.yaml)

| Native | File |
|--------|------|
| C | [`ta_FRAMA.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_FRAMA.c) |
| Rust | [`frama.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/library/src/ta_func/frama.rs) |
| Java | [`Core_FRAMA.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/fragments/Core_FRAMA.java) |
| C# | [`Core_FRAMA.cs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/csharp/library/src/Core_FRAMA.cs) |
| TypeScript | [`frama.ts`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/ts/library/src/func/frama.ts) |

Python: `talib.FRAMA` (ta-lib-python 0.8.3 or later), see [ta-lib-python](https://ta-lib.github.io/ta-lib-python/func_groups/overlap_studies.html).

TA-Lib is also available for R and more using a [wrapper](/install/#wrappers).

## Aliases

Fractal Adaptive Moving Average, FrAMA

## See Also

[KAMA](/functions/kama.md) · [MAMA](/functions/mama.md) · [MEDPRICE](/functions/medprice.md)

## References

- John F. Ehlers, "Fractal Adaptive Moving Averages", Technical Analysis of Stocks & Commodities V.23:10 (October 2005)
