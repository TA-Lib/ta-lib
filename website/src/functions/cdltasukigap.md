---
title: "Tasuki Gap (CDLTASUKIGAP)"
description: "A three-candle pattern: a real-body-gapping candle followed by an opposite-color candle that opens inside its body and closes back into the gap without…"
---

## Summary

A three-candle pattern: a real-body-gapping candle followed by an opposite-color candle that opens inside its body and closes back into the gap without filling it. An upside gap is a bullish continuation signal; a downside gap is a bearish continuation signal.

## Chart

<ta-chart src="/functions/cdltasukigap.svg" ratio="720:274" label="CDLTASUKIGAP over WBTC/USDC (Uniswap v3), daily, 2023" alt="Drawn on the price pane over candles, with outInteger as markers on the candles, ▲ below the low for a bullish call and ▼ above the high for a bearish one."></ta-chart>

::: details Chart metadata <span class="flag-tip" tabindex="0" role="note" aria-label="The chart is drawn automatically from metadata TA-Lib provides. Your app can get the same through the Abstract API." data-tip="The chart is drawn automatically from metadata TA-Lib provides. Your app can get the same through the Abstract API.">i</span>

Drawn on the price pane over candles, with outInteger as markers on the candles, ▲ below the low for a bullish call and ▼ above the high for a bearish one.

Metadata flags used: `candlestick` · `inPriceOHLC: open, high, low, close` · `outInteger: pattern_bull_bear, zero, positive, negative`

:::

[Reference data (CSV)](/functions/cdltasukigap.csv) · [Chart (SVG)](/functions/cdltasukigap.svg)

## Notes

- This continuation pattern does not verify the prior trend it classically assumes; the caller must confirm the trend.
- Bulkowski's testing found the downside Tasuki Gap actually acts as a bullish REVERSAL 54% of the time — opposite its textbook bearish-continuation label — while the upside variant does continue as labeled, but only 57% of the time ("near random"). ([thepatternsite.com](https://thepatternsite.com/DownsideTasukiGap.html))

## Inputs

- `inOpen` — Open price of each bar
- `inHigh` — High price of each bar
- `inLow` — Low price of each bar
- `inClose` — Close price of each bar

## Outputs

- `outInteger` — +100 on a bullish (upside-gap) tasuki gap, -100 on a bearish (downside-gap) tasuki gap, 0 otherwise. Sign equals the color of the gap candle i-1 (candlecolor(i-1)*100)

## Output Values

What candlestick values mean: [Candlestick Outputs](/api/candlestick-outputs/). The function's flags declare which values it can return: [rW7](/spec/inputs-outputs/#rw7).

| Value | Meaning |
|-------|---------|
| -100 | Bearish (downside-gap) Tasuki Gap |
| 0 | No pattern |
| 100 | Bullish (upside-gap) Tasuki Gap |

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
TA_CDLTASUKIGAP( 0, n - 1, open, high, low, close, &outBegIdx, &outNBElement, out );
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
let range = core.cdltasukigap(0, open.len() - 1, &open, &high, &low, &close, &mut out)?;
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
OutRange r = core.cdltasukigap(0, open.length - 1, open, high, low, close, out);
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
OutRange r = core.Cdltasukigap(0, open.Length - 1, open, high, low, close, outInteger);
Console.WriteLine($"{r.Count} values from bar {r.BegIdx}; last outInteger = {outInteger[r.Count - 1]}");
```

@tab TypeScript

```ts
import { cdltasukigap } from 'ta-lib';

const open = new Float64Array(252); // your data, oldest first
const high = new Float64Array(252); // your data, oldest first
const low = new Float64Array(252); // your data, oldest first
const close = new Float64Array(252); // your data, oldest first

const { begIdx, values } = cdltasukigap({ open, high, low, close });
console.log(values.length + ' values from bar ' + begIdx + '; last values = ' + values[values.length - 1]);
```

:::

## Implementation

Added in 0.4.0.

TA-Lib Definition: [`cdltasukigap.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/cdltasukigap/cdltasukigap.c) · [`cdltasukigap.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/cdltasukigap/cdltasukigap.yaml)

| Native | File |
|--------|------|
| C | [`ta_CDLTASUKIGAP.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_CDLTASUKIGAP.c) |
| Rust | [`cdltasukigap.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/library/src/ta_func/cdltasukigap.rs) |
| Java | [`Core_CDLTASUKIGAP.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/fragments/Core_CDLTASUKIGAP.java) |
| C# | [`Core_CDLTASUKIGAP.cs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/csharp/library/src/Core_CDLTASUKIGAP.cs) |
| TypeScript | [`cdltasukigap.ts`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/ts/library/src/func/cdltasukigap.ts) |

Python: `talib.CDLTASUKIGAP`, see [ta-lib-python](https://ta-lib.github.io/ta-lib-python/func_groups/pattern_recognition.html).

TA-Lib is also available for R and more using a [wrapper](/install/#wrappers).

## Aliases

Tasuki Gap, Upside/Downside Tasuki Gap

## See Also

[CDLGAPSIDESIDEWHITE](/functions/cdlgapsidesidewhite.md) · [CDLXSIDEGAP3METHODS](/functions/cdlxsidegap3methods.md)
