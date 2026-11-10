---
title: "Engulfing Pattern (CDLENGULFING)"
description: "A two-candle reversal pattern where the second candle's real body engulfs the first candle's opposite-colored real body."
---

## Summary

A two-candle reversal pattern where the second candle's real body engulfs the first candle's opposite-colored real body. Bullish (white engulfs black) or bearish (black engulfs white) reversal signal; ideally after a downtrend (bullish) or uptrend (bearish), which the code does not verify.

## Chart

<ta-chart src="/functions/cdlengulfing.svg" ratio="720:274" label="CDLENGULFING over IBM, daily, 1999" alt="Drawn on the price pane over candles, with outInteger as markers on the candles, ▲ below the low for a bullish call and ▼ above the high for a bearish one, a weak call (80) hollow."></ta-chart>

::: details Chart metadata <span class="flag-tip" tabindex="0" role="note" aria-label="The chart is drawn automatically from metadata TA-Lib provides. Your app can get the same through the Abstract API." data-tip="The chart is drawn automatically from metadata TA-Lib provides. Your app can get the same through the Abstract API.">i</span>

Drawn on the price pane over candles, with outInteger as markers on the candles, ▲ below the low for a bullish call and ▼ above the high for a bearish one, a weak call (80) hollow.

Metadata flags used: `candlestick` · `inPriceOHLC: open, high, low, close` · `outInteger: pattern_bull_bear, pattern_weak, zero, positive, negative`

:::

[Reference data (CSV)](/functions/cdlengulfing.csv) · [Chart (SVG)](/functions/cdlengulfing.svg)

## Notes

- Does not verify the prior trend (down for bullish, up for bearish) the reversal classically assumes.
- Bulkowski's testing found bearish Engulfing has a strong 79% reversal rate (5th-best of 103 patterns by that measure alone) but a weak overall post-breakout performance rank of 91st of 103 — the reversal fires reliably but rarely sustains. Bullish Engulfing reverses 63% of the time with a similarly weak overall rank of 84th of 103. ([thepatternsite.com](https://thepatternsite.com/BearEngulfing.html))

## Inputs

- `inOpen` — Open price of each bar
- `inHigh` — High price of each bar
- `inLow` — Low price of each bar
- `inClose` — Close price of each bar

## Outputs

- `outInteger` — +100/+80 (bullish, white engulfs black), -100/-80 (bearish, black engulfs white), 0 otherwise. Magnitude 100 when the second body strictly engulfs both ends; 80 when the bodies share an exact endpoint (open[i]==close[i-1] or close[i]==open[i-1])

## Output Values

What candlestick values mean: [Candlestick Outputs](/api/candlestick-outputs/). The function's flags declare which values it can return: [rW7](/spec/inputs-outputs/#rw7).

| Value | Meaning |
|-------|---------|
| -100 | Bearish engulfing: the black body fully swallows the prior white body — a strong reversal signal |
| -80 | Bearish engulfing, weaker variant: one edge of the two bodies lines up exactly rather than fully overhanging it |
| 0 | No pattern |
| 80 | Bullish engulfing, weaker variant: one edge of the two bodies lines up exactly rather than fully overhanging it |
| 100 | Bullish engulfing: the white body fully swallows the prior black body — a strong reversal signal |

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
TA_CDLENGULFING( 0, n - 1, open, high, low, close, &outBegIdx, &outNBElement, out );
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
let range = core.cdlengulfing(0, open.len() - 1, &open, &high, &low, &close, &mut out)?;
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
OutRange r = core.cdlengulfing(0, open.length - 1, open, high, low, close, out);
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
OutRange r = core.Cdlengulfing(0, open.Length - 1, open, high, low, close, outInteger);
Console.WriteLine($"{r.Count} values from bar {r.BegIdx}; last outInteger = {outInteger[r.Count - 1]}");
```

@tab TypeScript

```ts
import { cdlengulfing } from 'ta-lib';

const open = new Float64Array(252); // your data, oldest first
const high = new Float64Array(252); // your data, oldest first
const low = new Float64Array(252); // your data, oldest first
const close = new Float64Array(252); // your data, oldest first

const { begIdx, values } = cdlengulfing({ open, high, low, close });
console.log(values.length + ' values from bar ' + begIdx + '; last values = ' + values[values.length - 1]);
```

:::

## Implementation

Added in 0.4.0.

TA-Lib Definition: [`cdlengulfing.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/cdlengulfing/cdlengulfing.c) · [`cdlengulfing.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/cdlengulfing/cdlengulfing.yaml)

| Native | File |
|--------|------|
| C | [`ta_CDLENGULFING.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_CDLENGULFING.c) |
| Rust | [`cdlengulfing.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/library/src/ta_func/cdlengulfing.rs) |
| Java | [`Core_CDLENGULFING.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/fragments/Core_CDLENGULFING.java) |
| C# | [`Core_CDLENGULFING.cs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/csharp/library/src/Core_CDLENGULFING.cs) |
| TypeScript | [`cdlengulfing.ts`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/ts/library/src/func/cdlengulfing.ts) |

Python: `talib.CDLENGULFING`, see [ta-lib-python](https://ta-lib.github.io/ta-lib-python/func_groups/pattern_recognition.html).

TA-Lib is also available for R and more using a [wrapper](/install/#wrappers).

## Aliases

Engulfing Pattern, Engulfing, Bullish/Bearish Engulfing

## See Also

[CDLHARAMI](/functions/cdlharami.md) · [CDLCOUNTERATTACK](/functions/cdlcounterattack.md) · [CDLHARAMICROSS](/functions/cdlharamicross.md)
