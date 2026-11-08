---
title: "Harami Cross Pattern (CDLHARAMICROSS)"
description: "A two-candle reversal pattern: a long real body followed by a doji whose real body is contained within the first candle's real body (the doji variant of…"
---

## Summary

A two-candle reversal pattern: a long real body followed by a doji whose real body is contained within the first candle's real body (the doji variant of the Harami). Bullish after a black first candle, bearish after a white first candle.

## Chart

<ta-chart src="/functions/cdlharamicross.svg" ratio="720:274" label="CDLHARAMICROSS over IBM, daily, 1999" alt="Drawn on the price pane over candles, with outInteger as markers on the candles, ▲ below the low for a bullish call and ▼ above the high for a bearish one, a weak call (80) hollow."></ta-chart>

Drawn on the price pane over candles, with outInteger as markers on the candles, ▲ below the low for a bullish call and ▼ above the high for a bearish one, a weak call (80) hollow. [Reference data (CSV)](/functions/cdlharamicross.csv) · [Chart (SVG)](/functions/cdlharamicross.svg)

## Notes

- Does not verify the prior trend (downtrend for bullish, uptrend for bearish) that the reversal signal assumes.
- Bulkowski's testing found the bearish Harami Cross behaves opposite its textbook label even more strongly than the plain Harami: it acts as a bullish CONTINUATION 57% of the time rather than a bearish reversal, and the bullish Harami Cross likewise fails to reverse the downtrend 55% of the time. ([thepatternsite.com](https://thepatternsite.com/HaramiCrossBear.html))

## Inputs

- `inOpen` — Open price of each bar
- `inHigh` — High price of each bar
- `inLow` — Low price of each bar
- `inClose` — Close price of each bar

## Outputs

- `outInteger` — +100/+80 when the first candle is black (bullish), -100/-80 when the first candle is white (bearish), 0 otherwise. Magnitude 100 for strict containment inside the first body, 80 when one or both real-body ends match

## Output Values

What candlestick values mean: [Candlestick Outputs](/api/candlestick-outputs/). The function's flags declare which values it can return: [rW7](/spec/inputs-outputs/#rw7).

| Value | Meaning |
|-------|---------|
| -100 | Bearish Harami Cross: a doji forms inside the prior long white candle — indecision after an advance, a sharper warning than a plain Harami |
| -80 | Bearish Harami Cross, weaker variant: one or both of the doji's edges line up exactly with the ends of the long white candle's body |
| 0 | No pattern |
| 80 | Bullish Harami Cross, weaker variant: one or both of the doji's edges line up exactly with the ends of the long black candle's body |
| 100 | Bullish Harami Cross: a doji forms inside the prior long black candle — indecision after a decline, a sharper warning than a plain Harami |

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
TA_CDLHARAMICROSS( 0, n - 1, open, high, low, close, &outBegIdx, &outNBElement, out );
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
let range = core.cdlharamicross(0, open.len() - 1, &open, &high, &low, &close, &mut out)?;
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
OutRange r = core.cdlharamicross(0, open.length - 1, open, high, low, close, out);
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
OutRange r = core.Cdlharamicross(0, open.Length - 1, open, high, low, close, outInteger);
Console.WriteLine($"{r.Count} values from bar {r.BegIdx}; last outInteger = {outInteger[r.Count - 1]}");
```

@tab TypeScript

```ts
import { cdlharamicross } from 'ta-lib';

const open = new Float64Array(252); // your data, oldest first
const high = new Float64Array(252); // your data, oldest first
const low = new Float64Array(252); // your data, oldest first
const close = new Float64Array(252); // your data, oldest first

const { begIdx, values } = cdlharamicross({ open, high, low, close });
console.log(values.length + ' values from bar ' + begIdx + '; last values = ' + values[values.length - 1]);
```

:::

## Implementation

Added in 0.4.0.

TA-Lib Definition: [`cdlharamicross.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/cdlharamicross/cdlharamicross.c) · [`cdlharamicross.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/cdlharamicross/cdlharamicross.yaml)

| Native | File |
|--------|------|
| C | [`ta_CDLHARAMICROSS.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_CDLHARAMICROSS.c) |
| Rust | [`cdlharamicross.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/library/src/ta_func/cdlharamicross.rs) |
| Java | [`Core_CDLHARAMICROSS.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/fragments/Core_CDLHARAMICROSS.java) |
| C# | [`Core_CDLHARAMICROSS.cs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/csharp/library/src/Core_CDLHARAMICROSS.cs) |
| TypeScript | [`cdlharamicross.ts`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/ts/library/src/func/cdlharamicross.ts) |

Python: `talib.CDLHARAMICROSS`, see [ta-lib-python](https://ta-lib.github.io/ta-lib-python/func_groups/pattern_recognition.html).

TA-Lib is also available for R and more using a [wrapper](/install/#wrappers).

## Aliases

Harami Cross

## See Also

[CDLHARAMI](/functions/cdlharami.md) · [CDLDOJI](/functions/cdldoji.md)
