---
title: "Spinning Top (CDLSPINNINGTOP)"
description: "Single-candle pattern: a small real body with both an upper and a lower shadow longer than the body."
---

## Summary

Single-candle pattern: a small real body with both an upper and a lower shadow longer than the body. Signals indecision; the code does not classify it as bullish or bearish.

## Chart

<ta-chart src="/functions/cdlspinningtop.svg" ratio="720:274" label="CDLSPINNINGTOP over IBM, daily, 1999" alt="Drawn on the price pane over candles, with outInteger as markers on the candles by sign (the candle's colour), ▲ above the high for + and ▼ below the low for −."></ta-chart>

Drawn on the price pane over candles, with outInteger as markers on the candles by sign (the candle's colour), ▲ above the high for + and ▼ below the low for −. [Reference data (CSV)](/functions/cdlspinningtop.csv) · [Chart (SVG)](/functions/cdlspinningtop.svg)

## Formula

One candle where: upper shadow > real body AND lower shadow > real body AND real body < the BodyShort average. The BodyShort average is the factor-scaled mean body over the prior avgPeriod candles.

## Inputs

- `inOpen` — Open price of each bar
- `inHigh` — High price of each bar
- `inLow` — Low price of each bar
- `inClose` — Close price of each bar

## Outputs

- `outInteger` — +100 when the candle is white (close>=open), -100 when black (close<open), 0 when no pattern. Sign is candle color, NOT bullish/bearish

## Output Values

What candlestick values mean: [Candlestick Outputs](/api/candlestick-outputs/). The function's flags declare which values it can return: [rW7](/spec/inputs-outputs/#rw7).

| Value | Meaning |
|-------|---------|
| -100 | Black spinning top (close < open): indecision; the sign is the color only, not a bearish call |
| 0 | No pattern |
| 100 | White spinning top (close >= open): indecision; the sign is the color only, not a bullish call |

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
TA_CDLSPINNINGTOP( 0, n - 1, open, high, low, close, &outBegIdx, &outNBElement, out );
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
let range = core.cdlspinningtop(0, open.len() - 1, &open, &high, &low, &close, &mut out)?;
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
OutRange r = core.cdlspinningtop(0, open.length - 1, open, high, low, close, out);
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
OutRange r = core.Cdlspinningtop(0, open.Length - 1, open, high, low, close, outInteger);
Console.WriteLine($"{r.Count} values from bar {r.BegIdx}; last outInteger = {outInteger[r.Count - 1]}");
```

@tab TypeScript

```ts
import { cdlspinningtop } from 'ta-lib';

const open = new Float64Array(252); // your data, oldest first
const high = new Float64Array(252); // your data, oldest first
const low = new Float64Array(252); // your data, oldest first
const close = new Float64Array(252); // your data, oldest first

const { begIdx, values } = cdlspinningtop({ open, high, low, close });
console.log(values.length + ' values from bar ' + begIdx + '; last values = ' + values[values.length - 1]);
```

:::

## Implementation

Added in 0.4.0.

TA-Lib Definition: [`cdlspinningtop.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/cdlspinningtop/cdlspinningtop.c) · [`cdlspinningtop.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/cdlspinningtop/cdlspinningtop.yaml)

| Native | File |
|--------|------|
| C | [`ta_CDLSPINNINGTOP.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_CDLSPINNINGTOP.c) |
| Rust | [`cdlspinningtop.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/library/src/ta_func/cdlspinningtop.rs) |
| Java | [`Core_CDLSPINNINGTOP.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/fragments/Core_CDLSPINNINGTOP.java) |
| C# | [`Core_CDLSPINNINGTOP.cs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/csharp/library/src/Core_CDLSPINNINGTOP.cs) |
| TypeScript | [`cdlspinningtop.ts`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/ts/library/src/func/cdlspinningtop.ts) |

Python: `talib.CDLSPINNINGTOP`, see [ta-lib-python](https://ta-lib.github.io/ta-lib-python/func_groups/pattern_recognition.html).

TA-Lib is also available for R and more using a [wrapper](/install/#wrappers).

## Aliases

Spinning Top

## See Also

[CDLDOJI](/functions/cdldoji.md) · [CDLHIGHWAVE](/functions/cdlhighwave.md) · [CDLLONGLEGGEDDOJI](/functions/cdllongleggeddoji.md)
