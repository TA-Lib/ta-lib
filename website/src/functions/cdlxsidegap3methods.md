---
title: "Upside/Downside Gap Three Methods (CDLXSIDEGAP3METHODS)"
description: "A three-candle continuation pattern: two same-color candles separated by a real-body gap, followed by an opposite-color candle that fills into the gap."
---

## Summary

A three-candle continuation pattern: two same-color candles separated by a real-body gap, followed by an opposite-color candle that fills into the gap. Bullish (upside) when the first two candles are white, bearish (downside) when they are black.

## Chart

<ta-chart src="/functions/cdlxsidegap3methods.svg" ratio="720:274" label="CDLXSIDEGAP3METHODS over IBM, daily, 1999" alt="Drawn on the price pane over candles, with outInteger as markers on the candles, an arrow up from the bottom of the pane to below the low for a bullish call and an arrow down from the top of the pane to above the high for a bearish one."></ta-chart>

<div class="ta-chart-links">

[Reference data (CSV)](/functions/cdlxsidegap3methods.csv) · [Chart (SVG)](/functions/cdlxsidegap3methods.svg)

</div>

::: details Chart metadata <span class="flag-tip" tabindex="0" role="note" aria-label="The chart is drawn automatically from metadata TA-Lib provides. Your app can get the same through the Abstract API." data-tip="The chart is drawn automatically from metadata TA-Lib provides. Your app can get the same through the Abstract API.">i</span>

**Outputs**

- `outInteger`: `pattern_bull_bear` · `zero` · `positive` · `negative`

:::

## Notes

- This continuation pattern does not verify the prior trend it classically assumes; the caller must confirm the trend.
- Bulkowski's testing found BOTH directions of this pattern actually act as reversals more often than not, opposite the classic continuation label: the upside variant reverses bearish 59% of the time, the downside variant reverses bullish 62% of the time. ([thepatternsite.com](https://thepatternsite.com/UpGap3Methods.html))
- Rare in Bulkowski's tests: he ranks the upside form 85th and the downside 84th of 103 patterns by how often they occur ([upside](https://thepatternsite.com/UpGap3Methods.html), [downside](https://thepatternsite.com/DownGap3Methods.html)). It fires more often in crypto: 204 times in [about 16,000 daily bars](/api/candlestick-outputs/#how-often-patterns-fire).

## Inputs

- `inOpen` — Open price of each bar
- `inHigh` — High price of each bar
- `inLow` — Low price of each bar
- `inClose` — Close price of each bar

## Outputs

- `outInteger` — +100 when the two same-color candles are white (bullish/upside continuation), -100 when black (bearish/downside continuation), 0 otherwise. Equals candlecolor(1st candle) * 100

## Output Values

What candlestick values mean: [Candlestick Outputs](/api/candlestick-outputs/). The function's flags declare which values it can return: [rW7](/spec/inputs-outputs/#rw7).

| Value | Meaning |
|-------|---------|
| -100 | Downside Gap Three Methods: the first two candles are black — bearish continuation |
| 0 | No pattern |
| 100 | Upside Gap Three Methods: the first two candles are white — bullish continuation |

## Properties

**Numerical Stability:** [Start-Independent](/functions/stability.md#start-independent)

<div class="flag-cols">

<div class="flag-table">

|  |
| :-- |
| <span class="flag-box">☐</span> <span style="opacity:0.5">Overlap Input</span> |
| <span class="flag-box">✅</span> **Independent Y-Axis** <span class="flag-tip" tabindex="0" role="note" aria-label="Output is on its own scale, drawn in a separate pane below the price chart." data-tip="Output is on its own scale, drawn in a separate pane below the price chart.">i</span> |
| <span class="flag-box">✅</span> **Candlestick** <span class="flag-tip" tabindex="0" role="note" aria-label="A candlestick pattern; its values are listed under Output Values." data-tip="A candlestick pattern; its values are listed under Output Values.">i</span> |
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
double open[252]; /* your data, oldest first */
double high[252]; /* your data, oldest first */
double low[252]; /* your data, oldest first */
double close[252]; /* your data, oldest first */
int out[252];
int outBegIdx, outNBElement;

TA_Initialize();
TA_CDLXSIDEGAP3METHODS( 0, n - 1, open, high, low, close, &outBegIdx, &outNBElement, out );
printf( "%d values from bar %d; last out = %d\n", outNBElement, outBegIdx, out[outNBElement - 1] );
```

@tab Rust

```rust
use ta_lib::Core;

let open = vec![0.0; 252]; // your data, oldest first
let high = vec![0.0; 252]; // your data, oldest first
let low = vec![0.0; 252]; // your data, oldest first
let close = vec![0.0; 252]; // your data, oldest first
let mut out = vec![0i32; 252];

let core = Core::new();
let range = core.cdlxsidegap3methods(0, open.len() - 1, &open, &high, &low, &close,
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
int[] out = new int[252];

Core core = Core.DEFAULT;
OutRange r = core.cdlxsidegap3methods(0, open.length - 1, open, high, low, close, out);
System.out.println(r.count() + " values from bar " + r.begIdx() + "; last out = " + out[r.count() - 1]);
```

@tab C\#

```csharp
using System;
using TALib;

var open = new double[252]; // your data, oldest first
var high = new double[252]; // your data, oldest first
var low = new double[252]; // your data, oldest first
var close = new double[252]; // your data, oldest first
var outInteger = new int[252];

var core = Core.Default;
OutRange r = core.Cdlxsidegap3methods(0, open.Length - 1, open, high, low, close, outInteger);
Console.WriteLine($"{r.Count} values from bar {r.BegIdx}; last outInteger = {outInteger[r.Count - 1]}");
```

@tab TypeScript

```ts
import { cdlxsidegap3methods } from 'ta-lib';

const open = new Float64Array(252); // your data, oldest first
const high = new Float64Array(252); // your data, oldest first
const low = new Float64Array(252); // your data, oldest first
const close = new Float64Array(252); // your data, oldest first

const { begIdx, values } = cdlxsidegap3methods({ open, high, low, close });
console.log(values.length + ' values from bar ' + begIdx + '; last values = ' + values[values.length - 1]);
```

:::

## Implementation

Added in 0.4.0.

TA-Lib Definition: [`cdlxsidegap3methods.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/cdlxsidegap3methods/cdlxsidegap3methods.c) · [`cdlxsidegap3methods.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/cdlxsidegap3methods/cdlxsidegap3methods.yaml)

| Native | File |
|--------|------|
| C | [`ta_CDLXSIDEGAP3METHODS.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_CDLXSIDEGAP3METHODS.c) |
| Rust | [`cdlxsidegap3methods.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/library/src/ta_func/cdlxsidegap3methods.rs) |
| Java | [`Core_CDLXSIDEGAP3METHODS.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/fragments/Core_CDLXSIDEGAP3METHODS.java) |
| C# | [`Core_CDLXSIDEGAP3METHODS.cs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/csharp/library/src/Core_CDLXSIDEGAP3METHODS.cs) |
| TypeScript | [`cdlxsidegap3methods.ts`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/ts/library/src/func/cdlxsidegap3methods.ts) |

Python: `talib.CDLXSIDEGAP3METHODS`, see [ta-lib-python](https://ta-lib.github.io/ta-lib-python/func_groups/pattern_recognition.html).

TA-Lib is also available for R and more using a [wrapper](/install/#wrappers).

## Aliases

Upside/Downside Gap Three Methods, Upside Gap Three Methods, Downside Gap Three Methods

## See Also

[CDLGAPSIDESIDEWHITE](/functions/cdlgapsidesidewhite.md) · [CDLTASUKIGAP](/functions/cdltasukigap.md) · [CDLRISEFALL3METHODS](/functions/cdlrisefall3methods.md)
