---
title: "Three Black Crows (CDL3BLACKCROWS)"
description: "A four-bar pattern: a white candle followed by three consecutive black (down) candles with successively lower closes, each opening inside the prior…"
---

## Summary

A four-bar pattern: a white candle followed by three consecutive black (down) candles with successively lower closes, each opening inside the prior black's real body. It is a bearish reversal signal. A hit (-100) signals a bearish reversal.

## Chart

<ta-chart src="/functions/cdl3blackcrows.svg" ratio="720:274" label="CDL3BLACKCROWS over IBM, daily, 1999" alt="Drawn on the price pane over candles, with outInteger as markers on the candles, ▲ below the low for a bullish call and ▼ above the high for a bearish one (no occurrence in the reference data)."></ta-chart>

Drawn on the price pane over candles, with outInteger as markers on the candles, ▲ below the low for a bullish call and ▼ above the high for a bearish one (no occurrence in the reference data). [Reference data (CSV)](/functions/cdl3blackcrows.csv) · [Chart (SVG)](/functions/cdl3blackcrows.svg)

## Notes

- Does not verify the prior mature uptrend the pattern classically assumes for significance.

## Inputs

- `inOpen` — Open price of each bar
- `inHigh` — High price of each bar
- `inLow` — Low price of each bar
- `inClose` — Close price of each bar

## Outputs

- `outInteger` — -100 when the bearish pattern is detected, 0 otherwise. Never emits +100

## Output Values

What candlestick values mean: [Candlestick Outputs](/api/candlestick-outputs/). The function's flags declare which values it can return: [rW7](/spec/inputs-outputs/#rw7).

| Value | Meaning |
|-------|---------|
| -100 | Three Black Crows pattern detected: bearish |
| 0 | No pattern |

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
TA_CDL3BLACKCROWS( 0, n - 1, open, high, low, close, &outBegIdx, &outNBElement, out );
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
let range = core.cdl3blackcrows(0, open.len() - 1, &open, &high, &low, &close, &mut out)?;
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
OutRange r = core.cdl3blackcrows(0, open.length - 1, open, high, low, close, out);
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
OutRange r = core.Cdl3blackcrows(0, open.Length - 1, open, high, low, close, outInteger);
Console.WriteLine($"{r.Count} values from bar {r.BegIdx}; last outInteger = {outInteger[r.Count - 1]}");
```

@tab TypeScript

```ts
import { cdl3blackcrows } from 'ta-lib';

const open = new Float64Array(252); // your data, oldest first
const high = new Float64Array(252); // your data, oldest first
const low = new Float64Array(252); // your data, oldest first
const close = new Float64Array(252); // your data, oldest first

const { begIdx, values } = cdl3blackcrows({ open, high, low, close });
console.log(values.length + ' values from bar ' + begIdx + '; last values = ' + values[values.length - 1]);
```

:::

## Implementation

Added in 0.4.0.

TA-Lib Definition: [`cdl3blackcrows.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/cdl3blackcrows/cdl3blackcrows.c) · [`cdl3blackcrows.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/cdl3blackcrows/cdl3blackcrows.yaml)

| Native | File |
|--------|------|
| C | [`ta_CDL3BLACKCROWS.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_CDL3BLACKCROWS.c) |
| Rust | [`cdl3blackcrows.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/library/src/ta_func/cdl3blackcrows.rs) |
| Java | [`Core_CDL3BLACKCROWS.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/fragments/Core_CDL3BLACKCROWS.java) |
| C# | [`Core_CDL3BLACKCROWS.cs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/csharp/library/src/Core_CDL3BLACKCROWS.cs) |
| TypeScript | [`cdl3blackcrows.ts`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/ts/library/src/func/cdl3blackcrows.ts) |

Python: `talib.CDL3BLACKCROWS`, see [ta-lib-python](https://ta-lib.github.io/ta-lib-python/func_groups/pattern_recognition.html).

TA-Lib is also available for R and more using a [wrapper](/install/#wrappers).

## Aliases

Three Black Crows, 3 Black Crows

## See Also

[CDL3WHITESOLDIERS](/functions/cdl3whitesoldiers.md) · [CDLIDENTICAL3CROWS](/functions/cdlidentical3crows.md) · [CDLADVANCEBLOCK](/functions/cdladvanceblock.md)
