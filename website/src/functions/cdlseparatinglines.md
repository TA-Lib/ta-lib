---
title: "Separating Lines (CDLSEPARATINGLINES)"
description: "A two-candle continuation pattern: the second candle opposes the first in color, opens at the same price as the first, and is a long-bodied belt hold."
---

## Summary

A two-candle continuation pattern: the second candle opposes the first in color, opens at the same price as the first, and is a long-bodied belt hold. Bullish (white second candle) or bearish (black second candle) continuation signal.

## Chart

<ta-chart src="/functions/cdlseparatinglines.svg" ratio="720:274" label="CDLSEPARATINGLINES over IBM, daily, 1999" alt="Drawn on the price pane over candles, with outInteger as markers on the candles, ▲ below the low for a bullish call and ▼ above the high for a bearish one."></ta-chart>

::: details Chart metadata <span class="flag-tip" tabindex="0" role="note" aria-label="The chart is drawn from the function's metadata: the flags and attributes the abstract API reports for it and for each output, and the input series its reference run declares." data-tip="The chart is drawn from the function's metadata: the flags and attributes the abstract API reports for it and for each output, and the input series its reference run declares.">i</span>

Drawn on the price pane over candles, with outInteger as markers on the candles, ▲ below the low for a bullish call and ▼ above the high for a bearish one.

Metadata flags used: `candlestick` · `inPriceOHLC: open, high, low, close` · `outInteger: pattern_bull_bear, zero, positive, negative`

:::

[Reference data (CSV)](/functions/cdlseparatinglines.csv) · [Chart (SVG)](/functions/cdlseparatinglines.svg)

## Formula

Two consecutive candles i-1, i: (1) opposite colors: color(i-1) == -color(i); (2) same open: open[i-1]-Equal_avg <= open[i] <= open[i-1]+Equal_avg; (3) long body: realbody(i) > BodyLong_avg; (4) belt hold: if i is white, lowershadow(i) < ShadowVeryShort_avg; if i is black, uppershadow(i) < ShadowVeryShort_avg.

## Notes

- A prior trend is not verified, nor that the pattern aligns with it.
- Rare in Bulkowski's tests: he ranks the bullish form 76th and the bearish 82nd of 103 patterns by how often they occur ([bullish](https://thepatternsite.com/SeparateLinesBull.html), [bearish](https://thepatternsite.com/SeparateLinesBear.html)). It fires more often in crypto: 120 times in [about 16,000 daily bars](/api/candlestick-outputs/#how-often-patterns-fire).

## Inputs

- `inOpen` — Open price of each bar
- `inHigh` — High price of each bar
- `inLow` — Low price of each bar
- `inClose` — Close price of each bar

## Outputs

- `outInteger` — +100 for a bullish (white second candle) hit, -100 for a bearish (black second candle) hit, 0 otherwise

## Output Values

What candlestick values mean: [Candlestick Outputs](/api/candlestick-outputs/). The function's flags declare which values it can return: [rW7](/spec/inputs-outputs/#rw7).

| Value | Meaning |
|-------|---------|
| -100 | Bearish Separating Lines: the second candle (belt hold) closed black |
| 0 | No pattern |
| 100 | Bullish Separating Lines: the second candle (belt hold) closed white |

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
TA_CDLSEPARATINGLINES( 0, n - 1, open, high, low, close, &outBegIdx, &outNBElement, out );
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
let range = core.cdlseparatinglines(0, open.len() - 1, &open, &high, &low, &close,
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
OutRange r = core.cdlseparatinglines(0, open.length - 1, open, high, low, close, out);
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
OutRange r = core.Cdlseparatinglines(0, open.Length - 1, open, high, low, close, outInteger);
Console.WriteLine($"{r.Count} values from bar {r.BegIdx}; last outInteger = {outInteger[r.Count - 1]}");
```

@tab TypeScript

```ts
import { cdlseparatinglines } from 'ta-lib';

const open = new Float64Array(252); // your data, oldest first
const high = new Float64Array(252); // your data, oldest first
const low = new Float64Array(252); // your data, oldest first
const close = new Float64Array(252); // your data, oldest first

const { begIdx, values } = cdlseparatinglines({ open, high, low, close });
console.log(values.length + ' values from bar ' + begIdx + '; last values = ' + values[values.length - 1]);
```

:::

## Implementation

Added in 0.4.0.

TA-Lib Definition: [`cdlseparatinglines.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/cdlseparatinglines/cdlseparatinglines.c) · [`cdlseparatinglines.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/cdlseparatinglines/cdlseparatinglines.yaml)

| Native | File |
|--------|------|
| C | [`ta_CDLSEPARATINGLINES.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_CDLSEPARATINGLINES.c) |
| Rust | [`cdlseparatinglines.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/library/src/ta_func/cdlseparatinglines.rs) |
| Java | [`Core_CDLSEPARATINGLINES.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/fragments/Core_CDLSEPARATINGLINES.java) |
| C# | [`Core_CDLSEPARATINGLINES.cs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/csharp/library/src/Core_CDLSEPARATINGLINES.cs) |
| TypeScript | [`cdlseparatinglines.ts`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/ts/library/src/func/cdlseparatinglines.ts) |

Python: `talib.CDLSEPARATINGLINES`, see [ta-lib-python](https://ta-lib.github.io/ta-lib-python/func_groups/pattern_recognition.html).

TA-Lib is also available for R and more using a [wrapper](/install/#wrappers).

## Aliases

Separating Lines

## See Also

[CDLBELTHOLD](/functions/cdlbelthold.md) · CDLMEETINGLINES
