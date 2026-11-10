---
title: "Kicking - bull/bear determined by the longer marubozu (CDLKICKINGBYLENGTH)"
description: "A two-candle pattern of two opposite-color marubozu (long body, very short shadows on both ends) separated by a gap."
---

## Summary

A two-candle pattern of two opposite-color marubozu (long body, very short shadows on both ends) separated by a gap. A strong directional/reversal signal whose bull/bear bias is set by the longer of the two marubozu.

## Chart

<ta-chart src="/functions/cdlkickingbylength.svg" ratio="720:274" label="CDLKICKINGBYLENGTH over Stock data, daily" alt="Drawn on the price pane over candles, with outInteger as markers on the candles, ▲ below the low for a bullish call and ▼ above the high for a bearish one."></ta-chart>

::: details Chart metadata <span class="flag-tip" tabindex="0" role="note" aria-label="The chart is drawn from the function's metadata: the flags and attributes the abstract API reports for it and for each output, and the input series its reference run declares." data-tip="The chart is drawn from the function's metadata: the flags and attributes the abstract API reports for it and for each output, and the input series its reference run declares.">i</span>

Drawn on the price pane over candles, with outInteger as markers on the candles, ▲ below the low for a bullish call and ▼ above the high for a bearish one.

Metadata flags used: `candlestick` · `inPriceOHLC: open, high, low, close` · `outInteger: pattern_bull_bear, zero, positive, negative`

:::

[Reference data (CSV)](/functions/cdlkickingbylength.csv) · [Chart (SVG)](/functions/cdlkickingbylength.svg)

## Notes

- Rare: it fires on the same bars as [`CDLKICKING`](/functions/cdlkicking.md), so a few years of one symbol's daily bars may show none. Bulkowski ranks Kicking's bullish form 100th and bearish 102nd of 103 patterns by how often they occur ([bullish](https://thepatternsite.com/KickingBull.html), [bearish](https://thepatternsite.com/KickingBear.html)).

## Inputs

- `inOpen` — Open price of each bar
- `inHigh` — High price of each bar
- `inLow` — Low price of each bar
- `inClose` — Close price of each bar

## Outputs

- `outInteger` — +100 or -100 on a hit, 0 otherwise. Sign = candlecolor of the candle with the larger realbody (i if realbody(i) > realbody(i-1), else i-1; tie goes to i-1): +100 if that marubozu is white, -100 if black

## Output Values

What candlestick values mean: [Candlestick Outputs](/api/candlestick-outputs/). The function's flags declare which values it can return: [rW7](/spec/inputs-outputs/#rw7).

| Value | Meaning |
|-------|---------|
| -100 | Bearish Kicking by Length: the longer of the two marubozu (the first, when both are as long) closed black |
| 0 | No pattern |
| 100 | Bullish Kicking by Length: the longer of the two marubozu (the first, when both are as long) closed white |

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
TA_CDLKICKINGBYLENGTH( 0, n - 1, open, high, low, close, &outBegIdx, &outNBElement, out );
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
let range = core.cdlkickingbylength(0, open.len() - 1, &open, &high, &low, &close,
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
OutRange r = core.cdlkickingbylength(0, open.length - 1, open, high, low, close, out);
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
OutRange r = core.Cdlkickingbylength(0, open.Length - 1, open, high, low, close, outInteger);
Console.WriteLine($"{r.Count} values from bar {r.BegIdx}; last outInteger = {outInteger[r.Count - 1]}");
```

@tab TypeScript

```ts
import { cdlkickingbylength } from 'ta-lib';

const open = new Float64Array(252); // your data, oldest first
const high = new Float64Array(252); // your data, oldest first
const low = new Float64Array(252); // your data, oldest first
const close = new Float64Array(252); // your data, oldest first

const { begIdx, values } = cdlkickingbylength({ open, high, low, close });
console.log(values.length + ' values from bar ' + begIdx + '; last values = ' + values[values.length - 1]);
```

:::

## Implementation

Added in 0.4.0.

TA-Lib Definition: [`cdlkickingbylength.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/cdlkickingbylength/cdlkickingbylength.c) · [`cdlkickingbylength.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/cdlkickingbylength/cdlkickingbylength.yaml)

| Native | File |
|--------|------|
| C | [`ta_CDLKICKINGBYLENGTH.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_CDLKICKINGBYLENGTH.c) |
| Rust | [`cdlkickingbylength.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/library/src/ta_func/cdlkickingbylength.rs) |
| Java | [`Core_CDLKICKINGBYLENGTH.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/fragments/Core_CDLKICKINGBYLENGTH.java) |
| C# | [`Core_CDLKICKINGBYLENGTH.cs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/csharp/library/src/Core_CDLKICKINGBYLENGTH.cs) |
| TypeScript | [`cdlkickingbylength.ts`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/ts/library/src/func/cdlkickingbylength.ts) |

Python: `talib.CDLKICKINGBYLENGTH`, see [ta-lib-python](https://ta-lib.github.io/ta-lib-python/func_groups/pattern_recognition.html).

TA-Lib is also available for R and more using a [wrapper](/install/#wrappers).

## Aliases

Kicking by Length, Kicking - bull/bear decided by the longer marubozu

## See Also

[CDLKICKING](/functions/cdlkicking.md) · [CDLMARUBOZU](/functions/cdlmarubozu.md) · [CDLGAPSIDESIDEWHITE](/functions/cdlgapsidesidewhite.md)
