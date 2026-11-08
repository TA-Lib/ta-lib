---
title: "Dragonfly Doji (CDLDRAGONFLYDOJI)"
description: "Single-candle pattern: a doji (open and close nearly equal) sitting at the top of the range, with no meaningful upper shadow and a long lower shadow."
---

## Summary

Single-candle pattern: a doji (open and close nearly equal) sitting at the top of the range, with no meaningful upper shadow and a long lower shadow. A reversal signal, but its bullish/bearish meaning depends on the prior trend (the code does not judge direction). A hit marks a dragonfly doji; treated as a potential reversal, but direction (bullish/bearish) must be read from the trend it appears in.

## Chart

<ta-chart src="/functions/cdldragonflydoji.svg" ratio="720:274" label="CDLDRAGONFLYDOJI over IBM, daily, 1999" alt="Drawn on the price pane over candles, with outInteger as a diamond above the high of every bar it flags."></ta-chart>

::: details Chart metadata <span class="flag-tip" tabindex="0" role="note" aria-label="The chart is drawn from the function's metadata alone: the flags the abstract API reports for it and for each output." data-tip="The chart is drawn from the function's metadata alone: the flags the abstract API reports for it and for each output.">i</span>

Drawn on the price pane over candles, with outInteger as a diamond above the high of every bar it flags.

Metadata flags used: `candlestick` · `inPriceOHLC: open, high, low, close` · `outInteger: line, pattern_bool, zero, positive`

:::

[Reference data (CSV)](/functions/cdldragonflydoji.csv) · [Chart (SVG)](/functions/cdldragonflydoji.svg)

## Formula

Single candle. realbody <= BodyDoji average (doji body) AND upper shadow < ShadowVeryShort average (no/very short upper shadow) AND lower shadow > ShadowVeryShort average (lower shadow present, not very short). No color, gap, or trend test.

## Notes

- Does not verify the prior trend that determines the pattern's bullish/bearish meaning.
- Bulkowski's testing found this reverses the prior trend only about 50% of the time — statistically no better than a coin flip — and ranks 98th of 103 candlestick patterns for post-breakout performance. ([thepatternsite.com](https://thepatternsite.com/Dragonfly.html))

## Inputs

- `inOpen` — Open price of each bar
- `inHigh` — High price of each bar
- `inLow` — Low price of each bar
- `inClose` — Close price of each bar

## Outputs

- `outInteger` — +100 when the pattern is present, 0 otherwise; never -100. The +100 does not itself imply bullishness (must be read against the trend)

## Output Values

What candlestick values mean: [Candlestick Outputs](/api/candlestick-outputs/). The function's flags declare which values it can return: [rW7](/spec/inputs-outputs/#rw7).

| Value | Meaning |
|-------|---------|
| 0 | No pattern |
| 100 | Dragonfly Doji detected — a potential reversal; direction (bullish/bearish) must be read from the prevailing trend, which this function does not check |

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
TA_CDLDRAGONFLYDOJI( 0, n - 1, open, high, low, close, &outBegIdx, &outNBElement, out );
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
let range = core.cdldragonflydoji(0, open.len() - 1, &open, &high, &low, &close, &mut out)?;
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
OutRange r = core.cdldragonflydoji(0, open.length - 1, open, high, low, close, out);
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
OutRange r = core.Cdldragonflydoji(0, open.Length - 1, open, high, low, close, outInteger);
Console.WriteLine($"{r.Count} values from bar {r.BegIdx}; last outInteger = {outInteger[r.Count - 1]}");
```

@tab TypeScript

```ts
import { cdldragonflydoji } from 'ta-lib';

const open = new Float64Array(252); // your data, oldest first
const high = new Float64Array(252); // your data, oldest first
const low = new Float64Array(252); // your data, oldest first
const close = new Float64Array(252); // your data, oldest first

const { begIdx, values } = cdldragonflydoji({ open, high, low, close });
console.log(values.length + ' values from bar ' + begIdx + '; last values = ' + values[values.length - 1]);
```

:::

## Implementation

Added in 0.4.0.

TA-Lib Definition: [`cdldragonflydoji.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/cdldragonflydoji/cdldragonflydoji.c) · [`cdldragonflydoji.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/cdldragonflydoji/cdldragonflydoji.yaml)

| Native | File |
|--------|------|
| C | [`ta_CDLDRAGONFLYDOJI.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_CDLDRAGONFLYDOJI.c) |
| Rust | [`cdldragonflydoji.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/library/src/ta_func/cdldragonflydoji.rs) |
| Java | [`Core_CDLDRAGONFLYDOJI.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/fragments/Core_CDLDRAGONFLYDOJI.java) |
| C# | [`Core_CDLDRAGONFLYDOJI.cs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/csharp/library/src/Core_CDLDRAGONFLYDOJI.cs) |
| TypeScript | [`cdldragonflydoji.ts`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/ts/library/src/func/cdldragonflydoji.ts) |

Python: `talib.CDLDRAGONFLYDOJI`, see [ta-lib-python](https://ta-lib.github.io/ta-lib-python/func_groups/pattern_recognition.html).

TA-Lib is also available for R and more using a [wrapper](/install/#wrappers).

## Aliases

Dragonfly Doji

## See Also

[CDLDOJI](/functions/cdldoji.md) · [CDLGRAVESTONEDOJI](/functions/cdlgravestonedoji.md) · [CDLLONGLEGGEDDOJI](/functions/cdllongleggeddoji.md) · [CDLTAKURI](/functions/cdltakuri.md)
