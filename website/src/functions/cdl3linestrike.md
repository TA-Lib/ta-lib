---
title: "Three-Line Strike (CDL3LINESTRIKE)"
description: "A four-candle pattern: three same-color candles with consecutively higher (or lower) closes, each opening within or near the prior real body, then a…"
---

## Summary

A four-candle pattern: three same-color candles with consecutively higher (or lower) closes, each opening within or near the prior real body, then a fourth opposite-color candle that opens beyond the third close and closes past the first candle's open. TA-Lib emits a signed continuation-style signal keyed to the color of the first three candles, traditionally read as significant only inside a trend matching those three candles.

## Chart

<ta-chart src="/functions/cdl3linestrike.svg" ratio="720:274" label="CDL3LINESTRIKE over IBM, daily, 1999" alt="Drawn on the price pane over candles, with outInteger as markers on the candles, ▲ below the low for a bullish call and ▼ above the high for a bearish one (no occurrence in the reference data)."></ta-chart>

::: details Chart metadata <span class="flag-tip" tabindex="0" role="note" aria-label="The chart is drawn from the function's metadata alone: the flags the abstract API reports for it and for each output." data-tip="The chart is drawn from the function's metadata alone: the flags the abstract API reports for it and for each output.">i</span>

Drawn on the price pane over candles, with outInteger as markers on the candles, ▲ below the low for a bullish call and ▼ above the high for a bearish one (no occurrence in the reference data).

Metadata flags used: `candlestick` · `inPriceOHLC: open, high, low, close` · `outInteger: line, pattern_bull_bear, zero, positive, negative`

:::

[Reference data (CSV)](/functions/cdl3linestrike.csv) · [Chart (SVG)](/functions/cdl3linestrike.svg)

## Notes

- Does not verify the surrounding trend the pattern classically assumes for significance.
- TA-Lib's sign follows the classic continuation reading. Thomas Bulkowski's statistical study of the pattern (*Encyclopedia of Candlestick Charts*) found the opposite in practice — it acted as a reversal far more often than a continuation — so traders who follow his research read this pattern's signal in the opposite direction from what its sign here suggests.

## Inputs

- `inOpen` — Open price of each bar
- `inHigh` — High price of each bar
- `inLow` — Low price of each bar
- `inClose` — Close price of each bar

## Outputs

- `outInteger` — +100 for a white (rising) three-line strike, -100 for a black (falling) three-line strike, 0 otherwise. Sign is the color of the first three candles: candlecolor(i-1)*100

## Output Values

What candlestick values mean: [Candlestick Outputs](/api/candlestick-outputs/). The function's flags declare which values it can return: [rW7](/spec/inputs-outputs/#rw7).

| Value | Meaning |
|-------|---------|
| -100 | Bearish Three-Line Strike — three falling candles abruptly erased by one strong reversal bar; classically read as a bearish continuation despite the apparent reversal |
| 0 | No pattern |
| 100 | Bullish Three-Line Strike — three rising candles abruptly erased by one strong reversal bar; classically read as a bullish continuation despite the apparent reversal |

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
TA_CDL3LINESTRIKE( 0, n - 1, open, high, low, close, &outBegIdx, &outNBElement, out );
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
let range = core.cdl3linestrike(0, open.len() - 1, &open, &high, &low, &close, &mut out)?;
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
OutRange r = core.cdl3linestrike(0, open.length - 1, open, high, low, close, out);
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
OutRange r = core.Cdl3linestrike(0, open.Length - 1, open, high, low, close, outInteger);
Console.WriteLine($"{r.Count} values from bar {r.BegIdx}; last outInteger = {outInteger[r.Count - 1]}");
```

@tab TypeScript

```ts
import { cdl3linestrike } from 'ta-lib';

const open = new Float64Array(252); // your data, oldest first
const high = new Float64Array(252); // your data, oldest first
const low = new Float64Array(252); // your data, oldest first
const close = new Float64Array(252); // your data, oldest first

const { begIdx, values } = cdl3linestrike({ open, high, low, close });
console.log(values.length + ' values from bar ' + begIdx + '; last values = ' + values[values.length - 1]);
```

:::

## Implementation

Added in 0.4.0.

TA-Lib Definition: [`cdl3linestrike.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/cdl3linestrike/cdl3linestrike.c) · [`cdl3linestrike.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/cdl3linestrike/cdl3linestrike.yaml)

| Native | File |
|--------|------|
| C | [`ta_CDL3LINESTRIKE.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_CDL3LINESTRIKE.c) |
| Rust | [`cdl3linestrike.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/library/src/ta_func/cdl3linestrike.rs) |
| Java | [`Core_CDL3LINESTRIKE.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/fragments/Core_CDL3LINESTRIKE.java) |
| C# | [`Core_CDL3LINESTRIKE.cs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/csharp/library/src/Core_CDL3LINESTRIKE.cs) |
| TypeScript | [`cdl3linestrike.ts`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/ts/library/src/func/cdl3linestrike.ts) |

Python: `talib.CDL3LINESTRIKE`, see [ta-lib-python](https://ta-lib.github.io/ta-lib-python/func_groups/pattern_recognition.html).

TA-Lib is also available for R and more using a [wrapper](/install/#wrappers).

## Aliases

Three-Line Strike, 3-Line Strike

## See Also

[CDL3WHITESOLDIERS](/functions/cdl3whitesoldiers.md) · [CDL3BLACKCROWS](/functions/cdl3blackcrows.md)
