---
url: 'https://ta-lib.org/functions/cdldojistar.md'
description: >-
  A two-candle reversal pattern: a long real body followed by a doji whose real
  body gaps away from it (up after a white body, down after a black body).
---
# Doji Star (CDLDOJISTAR)

## Summary

A two-candle reversal pattern: a long real body followed by a doji whose real body gaps away from it (up after a white body, down after a black body). Signals a potential reversal of the prevailing trend. A hit flags a likely trend reversal; true direction depends on the prevailing trend (bullish in a downtrend, bearish in an uptrend), which the code does not itself verify.

## Chart

<ta-chart src="/functions/cdldojistar.svg" ratio="720:274" label="CDLDOJISTAR over IBM, daily, 1999" alt="Drawn on the price pane over candles, with outInteger as markers on the candles, ▲ below the low for a bullish call and ▼ above the high for a bearish one."></ta-chart>

::: details Chart metadata <span class="flag-tip" tabindex="0" role="note" aria-label="The chart is drawn from the function's metadata: the flags and attributes the abstract API reports for it and for each output, and the input series its reference run declares." data-tip="The chart is drawn from the function's metadata: the flags and attributes the abstract API reports for it and for each output, and the input series its reference run declares.">i</span>

Drawn on the price pane over candles, with outInteger as markers on the candles, ▲ below the low for a bullish call and ▼ above the high for a bearish one.

Metadata flags used: `candlestick` · `inPriceOHLC: open, high, low, close` · `outInteger: pattern_bull_bear, zero, positive, negative`

:::

[Reference data (CSV)](/functions/cdldojistar.csv) · [Chart (SVG)](/functions/cdldojistar.svg)

## Formula

Two candles. Candle 1: long real body (realbody > BodyLong average). Candle 2: doji (realbody <= BodyDoji average). Gap: either candle 1 white (color==1) AND candle 2 real body gaps up above it (the real bodies gap up), or candle 1 black (color==-1) AND candle 2 real body gaps down below it (the real bodies gap down).

## Notes

* Does not verify the prior trend the reversal signal classically assumes.
* Bulkowski's testing contradicts the classic reading for the bullish case: theory says a bullish Doji Star (gapping down after a black candle) should be a bullish reversal, but he found it instead acts as a bearish CONTINUATION 64% of the time — almost 2 out of 3, the opposite of the textbook signal. ([thepatternsite.com](https://thepatternsite.com/DojiStarBull.html))

## Inputs

* `inOpen` — Open price of each bar
* `inHigh` — High price of each bar
* `inLow` — Low price of each bar
* `inClose` — Close price of each bar

## Outputs

* `outInteger` — Emits +100 or -100 on a hit, 0 otherwise. Value is -candlecolor(candle1)\*100: -100 when candle 1 is white (gap up), +100 when candle 1 is black (gap down)

## Output Values

What candlestick values mean: [Candlestick Outputs](/api/candlestick-outputs/). The function's flags declare which values it can return: [rW7](/spec/inputs-outputs/#rw7).

| Value | Meaning |
|-------|---------|
| -100 | Doji Star gapping up after a strong advance — waning momentum, a bearish warning if it appears in an uptrend |
| 0 | No pattern |
| 100 | Doji Star gapping down after a strong decline — waning momentum, a bullish warning if it appears in a downtrend |

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
TA_CDLDOJISTAR( 0, n - 1, open, high, low, close, &outBegIdx, &outNBElement, out );
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
let range = core.cdldojistar(0, open.len() - 1, &open, &high, &low, &close, &mut out)?;
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
OutRange r = core.cdldojistar(0, open.length - 1, open, high, low, close, out);
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
var outInteger = new int[252];

var core = Core.Default;
OutRange r = core.Cdldojistar(0, open.Length - 1, open, high, low, close, outInteger);
Console.WriteLine($"{r.Count} values from bar {r.BegIdx}; last outInteger = {outInteger[r.Count - 1]}");
```

@tab TypeScript

```ts
import { cdldojistar } from 'ta-lib';

const open = new Float64Array(252); // your data, oldest first
const high = new Float64Array(252); // your data, oldest first
const low = new Float64Array(252); // your data, oldest first
const close = new Float64Array(252); // your data, oldest first

const { begIdx, values } = cdldojistar({ open, high, low, close });
console.log(values.length + ' values from bar ' + begIdx + '; last values = ' + values[values.length - 1]);
```

:::

## Implementation

Added in 0.4.0.

TA-Lib Definition: [`cdldojistar.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/cdldojistar/cdldojistar.c) · [`cdldojistar.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/cdldojistar/cdldojistar.yaml)

| Native | File |
|--------|------|
| C | [`ta_CDLDOJISTAR.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_CDLDOJISTAR.c) |
| Rust | [`cdldojistar.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/library/src/ta_func/cdldojistar.rs) |
| Java | [`Core_CDLDOJISTAR.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/fragments/Core_CDLDOJISTAR.java) |
| C# | [`Core_CDLDOJISTAR.cs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/csharp/library/src/Core_CDLDOJISTAR.cs) |
| TypeScript | [`cdldojistar.ts`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/ts/library/src/func/cdldojistar.ts) |

Python: `talib.CDLDOJISTAR`, see [ta-lib-python](https://ta-lib.github.io/ta-lib-python/func_groups/pattern_recognition.html).

TA-Lib is also available for R and more using a [wrapper](/install/#wrappers).

## Aliases

Doji Star

## See Also

[CDLMORNINGDOJISTAR](/functions/cdlmorningdojistar.md) · [CDLEVENINGDOJISTAR](/functions/cdleveningdojistar.md) · [CDLDOJI](/functions/cdldoji.md) · [CDLMORNINGSTAR](/functions/cdlmorningstar.md) · [CDLEVENINGSTAR](/functions/cdleveningstar.md)
