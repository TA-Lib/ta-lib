---
title: "High-Wave Candle (CDLHIGHWAVE)"
description: "Single-candle pattern: a short real body with both a very long upper and a very long lower shadow."
---

## Summary

Single-candle pattern: a short real body with both a very long upper and a very long lower shadow. Signals market indecision; the output sign reports only candle color, not a bullish/bearish direction. A hit marks indecision (long-legged candle); not directional - sign encodes only the candle's color.

## Chart

<ta-chart src="/functions/cdlhighwave.svg" ratio="720:274" label="CDLHIGHWAVE over IBM, daily, 1999" alt="Drawn on the price pane over candles, with outInteger as an arrow up from the bottom of the pane to below the low of every bar it flags, either sign (the candle's colour shows which)."></ta-chart>

<div class="ta-chart-links">

[Reference data (CSV)](/functions/cdlhighwave.csv) · [Chart (SVG)](/functions/cdlhighwave.svg)

</div>

::: details Chart metadata <span class="flag-tip" tabindex="0" role="note" aria-label="The chart is drawn automatically from metadata TA-Lib provides. Your app can get the same through the Abstract API." data-tip="The chart is drawn automatically from metadata TA-Lib provides. Your app can get the same through the Abstract API.">i</span>

**Outputs**

- `outInteger`: `line` · `pattern_color` · `zero` · `positive` · `negative`

:::

## Formula

One candle at index i. Hit when all hold: (1) short real body: real body < the BodyShort average; (2) very long upper shadow: upper shadow > the ShadowVeryLong average; (3) very long lower shadow: lower shadow > the ShadowVeryLong average. No color, gap, or trend condition.

## Notes

- Bulkowski's testing found the High-Wave candle acts as a reversal only 51% of the time — statistically indistinguishable from random — which he notes actually agrees with the pattern's theoretical meaning of pure indecision. ([thepatternsite.com](https://thepatternsite.com/HighWave.html))

## Inputs

- `inOpen` — Open price of each bar
- `inHigh` — High price of each bar
- `inLow` — Low price of each bar
- `inClose` — Close price of each bar

## Outputs

- `outInteger` — On a hit, +100 when the candle is white (close >= open) or -100 when black (close < open); 0 otherwise. Sign denotes color, NOT bull/bear

## Output Values

What candlestick values mean: [Candlestick Outputs](/api/candlestick-outputs/). The function's flags declare which values it can return: [rW7](/spec/inputs-outputs/#rw7).

| Value | Meaning |
|-------|---------|
| -100 | High-Wave candle, black (close < open): sharp indecision after a volatile session; the sign marks the candle's color only, not a bullish/bearish call |
| 0 | No pattern |
| 100 | High-Wave candle, white (close >= open): sharp indecision after a volatile session; the sign marks the candle's color only, not a bullish/bearish call |

## Properties

**Numerical Stability:** [Start-Independent](/functions/stability.md#start-independent)

<div class="flag-cols">

<div class="flag-table">

|  |
| :-- |
| <span class="flag-box">☐</span> <span style="opacity:0.5">Overlap Input</span> |
| <span class="flag-box">✅</span> **Independent Y-Axis** <span class="flag-tip" tabindex="0" role="note" aria-label="Output is on its own scale, drawn in a separate pane below the price chart." data-tip="Output is on its own scale, drawn in a separate pane below the price chart.">i</span> |
| <span class="flag-box">✅</span> **Candlestick** <span class="flag-tip" tabindex="0" role="note" aria-label="A candlestick pattern; its values are listed under Output Values." data-tip="A candlestick pattern; its values are listed under Output Values.">i</span> |
| <span class="flag-box">☐</span> <span style="opacity:0.5">Display Shift</span> |

</div>

<div class="flag-table">

|  |
| :-- |
| <span class="flag-box">☐</span> <span style="opacity:0.5">Can Output NaN or ±Inf</span> |
| <span class="flag-box">☐</span> <span style="opacity:0.5">Identity at Period 1</span> |
| <span class="flag-box">☐</span> <span style="opacity:0.5">Uses Transcendental</span> |
| <span class="flag-box">☐</span> <span style="opacity:0.5">Input Must Be Positive</span> |

</div>

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
TA_CDLHIGHWAVE( 0, n - 1, open, high, low, close, &outBegIdx, &outNBElement, out );
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
let range = core.cdlhighwave(0, open.len() - 1, &open, &high, &low, &close, &mut out)?;
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
OutRange r = core.cdlhighwave(0, open.length - 1, open, high, low, close, out);
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
OutRange r = core.Cdlhighwave(0, open.Length - 1, open, high, low, close, outInteger);
Console.WriteLine($"{r.Count} values from bar {r.BegIdx}; last outInteger = {outInteger[r.Count - 1]}");
```

@tab TypeScript

```ts
import { cdlhighwave } from 'ta-lib';

const open = new Float64Array(252); // your data, oldest first
const high = new Float64Array(252); // your data, oldest first
const low = new Float64Array(252); // your data, oldest first
const close = new Float64Array(252); // your data, oldest first

const { begIdx, values } = cdlhighwave({ open, high, low, close });
console.log(values.length + ' values from bar ' + begIdx + '; last values = ' + values[values.length - 1]);
```

:::

## Implementation

Added in 0.4.0.

TA-Lib Definition: [`cdlhighwave.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/cdlhighwave/cdlhighwave.c) · [`cdlhighwave.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/cdlhighwave/cdlhighwave.yaml)

| Native | File |
|--------|------|
| C | [`ta_CDLHIGHWAVE.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_CDLHIGHWAVE.c) |
| Rust | [`cdlhighwave.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/library/src/ta_func/cdlhighwave.rs) |
| Java | [`Core_CDLHIGHWAVE.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/fragments/Core_CDLHIGHWAVE.java) |
| C# | [`Core_CDLHIGHWAVE.cs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/csharp/library/src/Core_CDLHIGHWAVE.cs) |
| TypeScript | [`cdlhighwave.ts`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/ts/library/src/func/cdlhighwave.ts) |

Python: `talib.CDLHIGHWAVE`, see [ta-lib-python](https://ta-lib.github.io/ta-lib-python/func_groups/pattern_recognition.html).

TA-Lib is also available for R and more using a [wrapper](/install/#wrappers).

## Aliases

High-Wave Candle, High Wave

## See Also

[CDLLONGLEGGEDDOJI](/functions/cdllongleggeddoji.md) · [CDLSPINNINGTOP](/functions/cdlspinningtop.md) · [CDLRICKSHAWMAN](/functions/cdlrickshawman.md) · [CDLDOJI](/functions/cdldoji.md)
