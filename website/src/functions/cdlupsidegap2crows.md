---
title: "Upside Gap Two Crows (CDLUPSIDEGAP2CROWS)"
description: "A three-candle bearish reversal pattern: a long white candle, then a small black candle gapping up (a gap between the real bodies), then a black candle…"
---

## Summary

A three-candle bearish reversal pattern: a long white candle, then a small black candle gapping up (a gap between the real bodies), then a black candle that engulfs the second candle's real body but still closes above the first candle's close. Signals a bearish reversal. A hit (-100) is a bearish reversal signal, most meaningful in an uptrend.

## Chart

<ta-chart src="/functions/cdlupsidegap2crows.svg" ratio="720:274" label="CDLUPSIDEGAP2CROWS over Stock data, daily" alt="Drawn on the price pane over candles, with outInteger as markers on the candles, ▼ above the high for a bearish call."></ta-chart>

::: details Chart metadata <span class="flag-tip" tabindex="0" role="note" aria-label="The chart is drawn automatically from metadata TA-Lib provides. Your app can get the same through the Abstract API." data-tip="The chart is drawn automatically from metadata TA-Lib provides. Your app can get the same through the Abstract API.">i</span>

Drawn on the price pane over candles, with outInteger as markers on the candles, ▼ above the high for a bearish call.

Metadata flags used: `candlestick` · `inPriceOHLC: open, high, low, close` · `outInteger: pattern_bull_bear, zero, negative`

:::

[Reference data (CSV)](/functions/cdlupsidegap2crows.csv) · [Chart (SVG)](/functions/cdlupsidegap2crows.svg)

## Notes

- The pattern classically assumes a prior uptrend, but this function does not verify any trend.
- Although classically a bearish reversal, Bulkowski's testing found this actually acts as a bullish continuation 60% of the time, and even when it does work "the price move is often lousy." ([thepatternsite.com](https://www.thepatternsite.com/UpGapTwoCrows.html))
- Rare: a few years of one symbol's daily bars may show none. Bulkowski ranks it 75th of 103 patterns by how often it occurs ([Upside Gap Two Crows](https://thepatternsite.com/UpGapTwoCrows.html)).

## Inputs

- `inOpen` — Open price of each bar
- `inHigh` — High price of each bar
- `inLow` — Low price of each bar
- `inClose` — Close price of each bar

## Outputs

- `outInteger` — -100 on a pattern bar, 0 otherwise. Bearish-only: this pattern never emits +100

## Output Values

What candlestick values mean: [Candlestick Outputs](/api/candlestick-outputs/). The function's flags declare which values it can return: [rW7](/spec/inputs-outputs/#rw7).

| Value | Meaning |
|-------|---------|
| -100 | Upside Gap Two Crows pattern detected: bearish |
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
TA_CDLUPSIDEGAP2CROWS( 0, n - 1, open, high, low, close, &outBegIdx, &outNBElement, out );
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
let range = core.cdlupsidegap2crows(0, open.len() - 1, &open, &high, &low, &close,
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
OutRange r = core.cdlupsidegap2crows(0, open.length - 1, open, high, low, close, out);
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
OutRange r = core.Cdlupsidegap2crows(0, open.Length - 1, open, high, low, close, outInteger);
Console.WriteLine($"{r.Count} values from bar {r.BegIdx}; last outInteger = {outInteger[r.Count - 1]}");
```

@tab TypeScript

```ts
import { cdlupsidegap2crows } from 'ta-lib';

const open = new Float64Array(252); // your data, oldest first
const high = new Float64Array(252); // your data, oldest first
const low = new Float64Array(252); // your data, oldest first
const close = new Float64Array(252); // your data, oldest first

const { begIdx, values } = cdlupsidegap2crows({ open, high, low, close });
console.log(values.length + ' values from bar ' + begIdx + '; last values = ' + values[values.length - 1]);
```

:::

## Implementation

Added in 0.4.0.

TA-Lib Definition: [`cdlupsidegap2crows.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/cdlupsidegap2crows/cdlupsidegap2crows.c) · [`cdlupsidegap2crows.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/cdlupsidegap2crows/cdlupsidegap2crows.yaml)

| Native | File |
|--------|------|
| C | [`ta_CDLUPSIDEGAP2CROWS.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_CDLUPSIDEGAP2CROWS.c) |
| Rust | [`cdlupsidegap2crows.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/library/src/ta_func/cdlupsidegap2crows.rs) |
| Java | [`Core_CDLUPSIDEGAP2CROWS.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/fragments/Core_CDLUPSIDEGAP2CROWS.java) |
| C# | [`Core_CDLUPSIDEGAP2CROWS.cs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/csharp/library/src/Core_CDLUPSIDEGAP2CROWS.cs) |
| TypeScript | [`cdlupsidegap2crows.ts`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/ts/library/src/func/cdlupsidegap2crows.ts) |

Python: `talib.CDLUPSIDEGAP2CROWS`, see [ta-lib-python](https://ta-lib.github.io/ta-lib-python/func_groups/pattern_recognition.html).

TA-Lib is also available for R and more using a [wrapper](/install/#wrappers).

## Aliases

Upside Gap Two Crows

## See Also

[CDL2CROWS](/functions/cdl2crows.md) · [CDLGAPSIDESIDEWHITE](/functions/cdlgapsidesidewhite.md)
