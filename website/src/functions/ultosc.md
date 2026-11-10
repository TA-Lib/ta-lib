---
title: "Ultimate Oscillator (ULTOSC)"
description: "Ultimate Oscillator: momentum indicator combining buying-pressure/true-range ratios over three time periods into one 0-100 weighted average."
---

## Summary

Ultimate Oscillator: momentum indicator combining buying-pressure/true-range ratios over three time periods into one 0-100 weighted average. Blends short-, medium-, and long-term momentum to damp single-period noise. Ranges 0-100; conventionally >70 overbought, <30 oversold.

## Chart

<ta-chart src="/functions/ultosc.svg" ratio="720:416" label="ULTOSC over IBM, daily, 1999" alt="Drawn on the price pane over candles; below it, a separate indicator pane with a zero line showing outReal as a solid line with dashed rules at 30 and 70."></ta-chart>

<div class="ta-chart-links">

[Reference data (CSV)](/functions/ultosc.csv) · [Chart (SVG)](/functions/ultosc.svg)

</div>

::: details Chart metadata <span class="flag-tip" tabindex="0" role="note" aria-label="The chart is drawn automatically from metadata TA-Lib provides. Your app can get the same through the Abstract API." data-tip="The chart is drawn automatically from metadata TA-Lib provides. Your app can get the same through the Abstract API.">i</span>

**Outputs**

- `outReal`: `line` · `zero` · `positive` · `negative` · `levels: 30, 70`

:::

## Formula

trueLow = min(low, prevClose);  BP = close - trueLow
TR = max(high-low, |prevClose-high|, |prevClose-low|)
avg_n = (sum BP over n bars) / (sum TR over n bars)
ULTOSC = 100 * (4*avg_short + 2*avg_mid + avg_long) / 7

## Notes

- The three periods are sorted internally, so the 4/2/1 weighting always applies to the shortest, middle, and longest period regardless of the order in which you pass them.

## Inputs

- `inHigh` — High price of each bar
- `inLow` — Low price of each bar
- `inClose` — Close price of each bar

## Outputs

- `outReal` — Ultimate Oscillator value

## Parameters

| Parameter | Type | Default | Accepted values | Description |
| --- | --- | --- | --- | --- |
| `optInTimePeriod1` | integer | 7 | 1–100000 | Bars for one averaging window |
| `optInTimePeriod2` | integer | 14 | 1–100000 | Bars for another averaging window |
| `optInTimePeriod3` | integer | 28 | 1–100000 | Bars for another averaging window |

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
TA_ULTOSC( 0, n - 1, high, low, close, 7, 14, 28, &outBegIdx, &outNBElement, out );
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
let range = core.ultosc(0, high.len() - 1, &high, &low, &close, 7, 14, 28, &mut out)?;
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
OutRange r = core.ultosc(0, high.length - 1, high, low, close, 7, 14, 28, out);
System.out.println(r.count() + " values from bar " + r.begIdx() + "; last out = " + out[r.count() - 1]);
```

@tab C\#

```csharp
using System;
using TALib;

var high = new double[252]; // your data, oldest first
var low = new double[252]; // your data, oldest first
var close = new double[252]; // your data, oldest first
var outReal = new double[252];

var core = Core.Default;
OutRange r = core.Ultosc(0, high.Length - 1, high, low, close, 7, 14, 28, outReal);
Console.WriteLine($"{r.Count} values from bar {r.BegIdx}; last outReal = {outReal[r.Count - 1]}");
```

@tab TypeScript

```ts
import { ultosc } from 'ta-lib';

const high = new Float64Array(252); // your data, oldest first
const low = new Float64Array(252); // your data, oldest first
const close = new Float64Array(252); // your data, oldest first

const { begIdx, values } = ultosc({ high, low, close }, {
  timePeriod1: 7,
  timePeriod2: 14,
  timePeriod3: 28,
});
console.log(values.length + ' values from bar ' + begIdx + '; last values = ' + values[values.length - 1]);
```

:::

## Implementation

Added in 0.4.0.

TA-Lib Definition: [`ultosc.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/ultosc/ultosc.c) · [`ultosc.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/ultosc/ultosc.yaml)

| Native | File |
|--------|------|
| C | [`ta_ULTOSC.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_ULTOSC.c) |
| Rust | [`ultosc.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/library/src/ta_func/ultosc.rs) |
| Java | [`Core_ULTOSC.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/fragments/Core_ULTOSC.java) |
| C# | [`Core_ULTOSC.cs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/csharp/library/src/Core_ULTOSC.cs) |
| TypeScript | [`ultosc.ts`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/ts/library/src/func/ultosc.ts) |

Python: `talib.ULTOSC`, see [ta-lib-python](https://ta-lib.github.io/ta-lib-python/func_groups/momentum_indicators.html).

TA-Lib is also available for R and more using a [wrapper](/install/#wrappers).

## Aliases

Ultimate Oscillator, UO

## See Also

[ATR](/functions/atr.md) · [TRANGE](/functions/trange.md) · [RSI](/functions/rsi.md)

## References

- Larry Williams, *The Ultimate Oscillator*, Technical Analysis of Stocks & Commodities, V.3:4 (1985)
