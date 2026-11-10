---
title: "Breakaway (CDLBREAKAWAY)"
description: "A five-candle reversal pattern: a long first candle, a same-colored second candle that gaps away from it by its real body, two more candles extending the…"
---

## Summary

A five-candle reversal pattern: a long first candle, a same-colored second candle that gaps away from it by its real body, two more candles extending the move, and an opposite-colored fifth candle that closes back inside the gap. Emits a bullish signal (bottom reversal) or bearish signal (top reversal).

## Chart

<ta-chart src="/functions/cdlbreakaway.svg" ratio="720:274" label="CDLBREAKAWAY over Stock data, daily" alt="Drawn on the price pane over candles, with outInteger as markers on the candles, ▲ below the low for a bullish call and ▼ above the high for a bearish one."></ta-chart>

::: details Chart metadata <span class="flag-tip" tabindex="0" role="note" aria-label="The chart is drawn automatically from metadata TA-Lib provides. Your app can get the same through the Abstract API." data-tip="The chart is drawn automatically from metadata TA-Lib provides. Your app can get the same through the Abstract API.">i</span>

Drawn on the price pane over candles, with outInteger as markers on the candles, ▲ below the low for a bullish call and ▼ above the high for a bearish one.

Metadata flags used: `candlestick` · `inPriceOHLC: open, high, low, close` · `outInteger: pattern_bull_bear, zero, positive, negative`

:::

[Reference data (CSV)](/functions/cdlbreakaway.csv) · [Chart (SVG)](/functions/cdlbreakaway.svg)

## Notes

- Does not verify the prior trend the pattern classically assumes (a breakaway matters most against a preceding move).
- Bulkowski's data shows a directional asymmetry TA-Lib's symmetric output doesn't capture: bullish Breakaway reverses only 59% of the time ("near random"), while bearish Breakaway reverses 63% of the time overall. ([thepatternsite.com](https://thepatternsite.com/BullBreakaway.html))
- Rare: a few years of one symbol's daily bars may show none. Bulkowski ranks the bullish form 97th and the bearish 98th of 103 patterns by how often they occur ([bullish](https://thepatternsite.com/BullBreakaway.html), [bearish](https://thepatternsite.com/BearBreakaway.html)).

## Inputs

- `inOpen` — Open price of each bar
- `inHigh` — High price of each bar
- `inLow` — Low price of each bar
- `inClose` — Close price of each bar

## Outputs

- `outInteger` — +100 when the fifth candle is white (bullish breakaway), -100 when it is black (bearish breakaway), 0 otherwise

## Output Values

What candlestick values mean: [Candlestick Outputs](/api/candlestick-outputs/). The function's flags declare which values it can return: [rW7](/spec/inputs-outputs/#rw7).

| Value | Meaning |
|-------|---------|
| -100 | Bearish Breakaway — a gap-up advance gets erased by a sharp reversal candle, signaling a top |
| 0 | No pattern |
| 100 | Bullish Breakaway — a gap-down decline gets erased by a sharp reversal candle, signaling a bottom |

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
TA_CDLBREAKAWAY( 0, n - 1, open, high, low, close, &outBegIdx, &outNBElement, out );
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
let range = core.cdlbreakaway(0, open.len() - 1, &open, &high, &low, &close, &mut out)?;
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
OutRange r = core.cdlbreakaway(0, open.length - 1, open, high, low, close, out);
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
OutRange r = core.Cdlbreakaway(0, open.Length - 1, open, high, low, close, outInteger);
Console.WriteLine($"{r.Count} values from bar {r.BegIdx}; last outInteger = {outInteger[r.Count - 1]}");
```

@tab TypeScript

```ts
import { cdlbreakaway } from 'ta-lib';

const open = new Float64Array(252); // your data, oldest first
const high = new Float64Array(252); // your data, oldest first
const low = new Float64Array(252); // your data, oldest first
const close = new Float64Array(252); // your data, oldest first

const { begIdx, values } = cdlbreakaway({ open, high, low, close });
console.log(values.length + ' values from bar ' + begIdx + '; last values = ' + values[values.length - 1]);
```

:::

## Implementation

Added in 0.4.0.

TA-Lib Definition: [`cdlbreakaway.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/cdlbreakaway/cdlbreakaway.c) · [`cdlbreakaway.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/cdlbreakaway/cdlbreakaway.yaml)

| Native | File |
|--------|------|
| C | [`ta_CDLBREAKAWAY.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_CDLBREAKAWAY.c) |
| Rust | [`cdlbreakaway.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/library/src/ta_func/cdlbreakaway.rs) |
| Java | [`Core_CDLBREAKAWAY.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/fragments/Core_CDLBREAKAWAY.java) |
| C# | [`Core_CDLBREAKAWAY.cs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/csharp/library/src/Core_CDLBREAKAWAY.cs) |
| TypeScript | [`cdlbreakaway.ts`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/ts/library/src/func/cdlbreakaway.ts) |

Python: `talib.CDLBREAKAWAY`, see [ta-lib-python](https://ta-lib.github.io/ta-lib-python/func_groups/pattern_recognition.html).

TA-Lib is also available for R and more using a [wrapper](/install/#wrappers).

## Aliases

Breakaway

## See Also

[CDLGAPSIDESIDEWHITE](/functions/cdlgapsidesidewhite.md) · [CDLRISEFALL3METHODS](/functions/cdlrisefall3methods.md) · [CDL3LINESTRIKE](/functions/cdl3linestrike.md)
