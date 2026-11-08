---
title: "Rising/Falling Three Methods (CDLRISEFALL3METHODS)"
description: "A five-candle continuation pattern: a long candle, three small counter-color candles that stay partly within the first candle's high-low range, then a…"
---

## Summary

A five-candle continuation pattern: a long candle, three small counter-color candles that stay partly within the first candle's high-low range, then a long same-color candle that resumes the trend. Bullish (rising) or bearish (falling) continuation signal.

## Chart

<ta-chart src="/functions/cdlrisefall3methods.svg" ratio="720:274" label="CDLRISEFALL3METHODS over IBM, daily, 1999" alt="Drawn on the price pane over candles, with outInteger as markers on the candles, ▲ below the low for a bullish call and ▼ above the high for a bearish one (no occurrence in the reference data)."></ta-chart>

::: details Chart metadata <span class="flag-tip" tabindex="0" role="note" aria-label="The chart is drawn from the function's metadata: the flags the abstract API reports for it and for each output, and the input series its reference run declares." data-tip="The chart is drawn from the function's metadata: the flags the abstract API reports for it and for each output, and the input series its reference run declares.">i</span>

Drawn on the price pane over candles, with outInteger as markers on the candles, ▲ below the low for a bullish call and ▼ above the high for a bearish one (no occurrence in the reference data).

Metadata flags used: `candlestick` · `inPriceOHLC: open, high, low, close` · `outInteger: pattern_bull_bear, zero, positive, negative`

:::

[Reference data (CSV)](/functions/cdlrisefall3methods.csv) · [Chart (SVG)](/functions/cdlrisefall3methods.svg)

## Notes

- Only the three-small-candle variant is detected; the classic pattern allowing two or more small candles is not supported.
- The middle candles need only partially overlap the first candle's range, not be fully contained within it.
- The prior trend the continuation reading assumes is not verified.
- Bulkowski's testing found Rising Three Methods continues 74% of the time (102 examples out of 4.7M candle lines) and Falling Three Methods continues 71% of the time (just 64 examples) — both act as classically labeled, but Bulkowski flags the samples as too thin to trust: Falling Three Methods is so rare he omitted its statistics from his book entirely. ([thepatternsite.com](https://thepatternsite.com/Rising3Methods.html))

## Inputs

- `inOpen` — Open price of each bar
- `inHigh` — High price of each bar
- `inLow` — Low price of each bar
- `inClose` — Close price of each bar

## Outputs

- `outInteger` — +100 when candle 1 is white (rising/bullish continuation), -100 when candle 1 is black (falling/bearish continuation), 0 otherwise. Sign = 100 * color of candle 1

## Output Values

What candlestick values mean: [Candlestick Outputs](/api/candlestick-outputs/). The function's flags declare which values it can return: [rW7](/spec/inputs-outputs/#rw7).

| Value | Meaning |
|-------|---------|
| -100 | Falling Three Methods: candle 1 is black — bearish continuation |
| 0 | No pattern |
| 100 | Rising Three Methods: candle 1 is white — bullish continuation |

## Properties

**Numerical Stability:** [Start-Independent](/functions/stability.md#start-independent)

<div class="flag-table">

|  |
| :-- |
| <span class="flag-box">☐</span> <span style="opacity:0.5">Overlap Input</span> |
| <span class="flag-box">✅</span> **Independent Y-Axis** <span class="flag-tip" tabindex="0" role="note" aria-label="Output is on its own scale, drawn in a separate pane below the price chart." data-tip="Output is on its own scale, drawn in a separate pane below the price chart.">i</span> |
| <span class="flag-box">✅</span> **Candlestick** <span class="flag-tip" tabindex="0" role="note" aria-label="A candlestick pattern; its values are listed under Output Values." data-tip="A candlestick pattern; its values are listed under Output Values.">i</span> |
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
double open[252]; /* your data, oldest first */
double high[252]; /* your data, oldest first */
double low[252]; /* your data, oldest first */
double close[252]; /* your data, oldest first */
int out[252];
int outBegIdx, outNBElement;

TA_Initialize();
TA_CDLRISEFALL3METHODS( 0, n - 1, open, high, low, close, &outBegIdx, &outNBElement, out );
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
let range = core.cdlrisefall3methods(0, open.len() - 1, &open, &high, &low, &close,
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
OutRange r = core.cdlrisefall3methods(0, open.length - 1, open, high, low, close, out);
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
OutRange r = core.Cdlrisefall3methods(0, open.Length - 1, open, high, low, close, outInteger);
Console.WriteLine($"{r.Count} values from bar {r.BegIdx}; last outInteger = {outInteger[r.Count - 1]}");
```

@tab TypeScript

```ts
import { cdlrisefall3methods } from 'ta-lib';

const open = new Float64Array(252); // your data, oldest first
const high = new Float64Array(252); // your data, oldest first
const low = new Float64Array(252); // your data, oldest first
const close = new Float64Array(252); // your data, oldest first

const { begIdx, values } = cdlrisefall3methods({ open, high, low, close });
console.log(values.length + ' values from bar ' + begIdx + '; last values = ' + values[values.length - 1]);
```

:::

## Implementation

Added in 0.4.0.

TA-Lib Definition: [`cdlrisefall3methods.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/cdlrisefall3methods/cdlrisefall3methods.c) · [`cdlrisefall3methods.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/cdlrisefall3methods/cdlrisefall3methods.yaml)

| Native | File |
|--------|------|
| C | [`ta_CDLRISEFALL3METHODS.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_CDLRISEFALL3METHODS.c) |
| Rust | [`cdlrisefall3methods.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/library/src/ta_func/cdlrisefall3methods.rs) |
| Java | [`Core_CDLRISEFALL3METHODS.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/fragments/Core_CDLRISEFALL3METHODS.java) |
| C# | [`Core_CDLRISEFALL3METHODS.cs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/csharp/library/src/Core_CDLRISEFALL3METHODS.cs) |
| TypeScript | [`cdlrisefall3methods.ts`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/ts/library/src/func/cdlrisefall3methods.ts) |

Python: `talib.CDLRISEFALL3METHODS`, see [ta-lib-python](https://ta-lib.github.io/ta-lib-python/func_groups/pattern_recognition.html).

TA-Lib is also available for R and more using a [wrapper](/install/#wrappers).

## Aliases

Rising/Falling Three Methods, Rising Three Methods, Falling Three Methods

## See Also

[CDLXSIDEGAP3METHODS](/functions/cdlxsidegap3methods.md) · [CDL3INSIDE](/functions/cdl3inside.md) · [CDL3OUTSIDE](/functions/cdl3outside.md)
