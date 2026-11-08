---
title: "Takuri (Dragonfly Doji with very long lower shadow) (CDLTAKURI)"
description: "Single-candle pattern: a doji whose open and close sit at the high (no/very short upper shadow) with a very long lower shadow, i.e. a dragonfly doji with…"
---

## Summary

Single-candle pattern: a doji whose open and close sit at the high (no/very short upper shadow) with a very long lower shadow, i.e. a dragonfly doji with an exceptionally long lower shadow. Emitted as a positive signal, but its directional meaning depends on the prevailing trend, which the code does not check. A hit marks a takuri (dragonfly-doji) line; a potential reversal only when read against the trend (typically a bottom/bullish reversal after a downtrend), which the code itself does not verify.

## Chart

<ta-chart src="/functions/cdltakuri.svg" ratio="720:274" label="CDLTAKURI over IBM, daily, 1999" alt="Drawn on the price pane over candles, with outInteger as a diamond above the high of every bar it flags."></ta-chart>

::: details Chart metadata <span class="flag-tip" tabindex="0" role="note" aria-label="The chart is drawn from the function's metadata: the flags the abstract API reports for it and for each output, and the input series its reference run declares." data-tip="The chart is drawn from the function's metadata: the flags the abstract API reports for it and for each output, and the input series its reference run declares.">i</span>

Drawn on the price pane over candles, with outInteger as a diamond above the high of every bar it flags.

Metadata flags used: `candlestick` · `inPriceOHLC: open, high, low, close` · `outInteger: pattern_bool, zero, positive`

:::

[Reference data (CSV)](/functions/cdltakuri.csv) · [Chart (SVG)](/functions/cdltakuri.svg)

## Inputs

- `inOpen` — Open price of each bar
- `inHigh` — High price of each bar
- `inLow` — Low price of each bar
- `inClose` — Close price of each bar

## Outputs

- `outInteger` — +100 when the takuri pattern is detected, 0 otherwise. Never negative; the positive sign is a convention and does not by itself imply bullishness

## Output Values

What candlestick values mean: [Candlestick Outputs](/api/candlestick-outputs/). The function's flags declare which values it can return: [rW7](/spec/inputs-outputs/#rw7).

| Value | Meaning |
|-------|---------|
| 0 | No pattern |
| 100 | Takuri (dragonfly doji with an exceptionally long lower shadow) detected — directional meaning depends on the prevailing trend, which the code does not check |

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
TA_CDLTAKURI( 0, n - 1, open, high, low, close, &outBegIdx, &outNBElement, out );
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
let range = core.cdltakuri(0, open.len() - 1, &open, &high, &low, &close, &mut out)?;
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
OutRange r = core.cdltakuri(0, open.length - 1, open, high, low, close, out);
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
OutRange r = core.Cdltakuri(0, open.Length - 1, open, high, low, close, outInteger);
Console.WriteLine($"{r.Count} values from bar {r.BegIdx}; last outInteger = {outInteger[r.Count - 1]}");
```

@tab TypeScript

```ts
import { cdltakuri } from 'ta-lib';

const open = new Float64Array(252); // your data, oldest first
const high = new Float64Array(252); // your data, oldest first
const low = new Float64Array(252); // your data, oldest first
const close = new Float64Array(252); // your data, oldest first

const { begIdx, values } = cdltakuri({ open, high, low, close });
console.log(values.length + ' values from bar ' + begIdx + '; last values = ' + values[values.length - 1]);
```

:::

## Implementation

Added in 0.4.0.

TA-Lib Definition: [`cdltakuri.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/cdltakuri/cdltakuri.c) · [`cdltakuri.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/cdltakuri/cdltakuri.yaml)

| Native | File |
|--------|------|
| C | [`ta_CDLTAKURI.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_CDLTAKURI.c) |
| Rust | [`cdltakuri.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/library/src/ta_func/cdltakuri.rs) |
| Java | [`Core_CDLTAKURI.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/fragments/Core_CDLTAKURI.java) |
| C# | [`Core_CDLTAKURI.cs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/csharp/library/src/Core_CDLTAKURI.cs) |
| TypeScript | [`cdltakuri.ts`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/ts/library/src/func/cdltakuri.ts) |

Python: `talib.CDLTAKURI`, see [ta-lib-python](https://ta-lib.github.io/ta-lib-python/func_groups/pattern_recognition.html).

TA-Lib is also available for R and more using a [wrapper](/install/#wrappers).

## Aliases

Takuri, Takuri line

## See Also

[CDLDRAGONFLYDOJI](/functions/cdldragonflydoji.md) · [CDLDOJI](/functions/cdldoji.md) · [CDLHAMMER](/functions/cdlhammer.md) · [CDLGRAVESTONEDOJI](/functions/cdlgravestonedoji.md)
