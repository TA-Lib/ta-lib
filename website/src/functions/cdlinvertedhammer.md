---
title: "Inverted Hammer (CDLINVERTEDHAMMER)"
description: "Single-candle pattern: a small real body with a long upper shadow and little-to-no lower shadow that gaps down from the prior candle."
---

## Summary

Single-candle pattern: a small real body with a long upper shadow and little-to-no lower shadow that gaps down from the prior candle. A hit flags a potential bullish reversal.

## Chart

<ta-chart src="/functions/cdlinvertedhammer.svg" ratio="720:274" label="CDLINVERTEDHAMMER over IBM, daily, 1999" alt="Drawn on the price pane over candles, with outInteger as markers on the candles, ▲ below the low for a bullish call."></ta-chart>

::: details Chart metadata <span class="flag-tip" tabindex="0" role="note" aria-label="The chart is drawn from the function's metadata: the flags and attributes the abstract API reports for it and for each output, and the input series its reference run declares." data-tip="The chart is drawn from the function's metadata: the flags and attributes the abstract API reports for it and for each output, and the input series its reference run declares.">i</span>

Drawn on the price pane over candles, with outInteger as markers on the candles, ▲ below the low for a bullish call.

Metadata flags used: `candlestick` · `inPriceOHLC: open, high, low, close` · `outInteger: pattern_bull_bear, zero, positive`

:::

[Reference data (CSV)](/functions/cdlinvertedhammer.csv) · [Chart (SVG)](/functions/cdlinvertedhammer.svg)

## Notes

- Does not verify the preceding downtrend that the pattern classically assumes; it only checks the gap down from the immediately preceding candle.
- Despite the bullish-reversal label, Bulkowski's testing found this actually behaves as a bearish continuation 65% of the time — yet its overall post-breakout performance rank (6th of 103) is among the best of all candlestick patterns he studied. ([thepatternsite.com](https://thepatternsite.com/HammerInv.html))

## Inputs

- `inOpen` — Open price of each bar
- `inHigh` — High price of each bar
- `inLow` — Low price of each bar
- `inClose` — Close price of each bar

## Outputs

- `outInteger` — +100 when the inverted hammer is detected, 0 otherwise. Never emits -100; the pattern is always bullish

## Output Values

What candlestick values mean: [Candlestick Outputs](/api/candlestick-outputs/). The function's flags declare which values it can return: [rW7](/spec/inputs-outputs/#rw7).

| Value | Meaning |
|-------|---------|
| 0 | No pattern |
| 100 | Inverted Hammer detected — a bullish reversal signal that classically assumes a preceding downtrend, unverified by the function |

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
TA_CDLINVERTEDHAMMER( 0, n - 1, open, high, low, close, &outBegIdx, &outNBElement, out );
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
let range = core.cdlinvertedhammer(0, open.len() - 1, &open, &high, &low, &close,
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
OutRange r = core.cdlinvertedhammer(0, open.length - 1, open, high, low, close, out);
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
OutRange r = core.Cdlinvertedhammer(0, open.Length - 1, open, high, low, close, outInteger);
Console.WriteLine($"{r.Count} values from bar {r.BegIdx}; last outInteger = {outInteger[r.Count - 1]}");
```

@tab TypeScript

```ts
import { cdlinvertedhammer } from 'ta-lib';

const open = new Float64Array(252); // your data, oldest first
const high = new Float64Array(252); // your data, oldest first
const low = new Float64Array(252); // your data, oldest first
const close = new Float64Array(252); // your data, oldest first

const { begIdx, values } = cdlinvertedhammer({ open, high, low, close });
console.log(values.length + ' values from bar ' + begIdx + '; last values = ' + values[values.length - 1]);
```

:::

## Implementation

Added in 0.4.0.

TA-Lib Definition: [`cdlinvertedhammer.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/cdlinvertedhammer/cdlinvertedhammer.c) · [`cdlinvertedhammer.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/cdlinvertedhammer/cdlinvertedhammer.yaml)

| Native | File |
|--------|------|
| C | [`ta_CDLINVERTEDHAMMER.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_CDLINVERTEDHAMMER.c) |
| Rust | [`cdlinvertedhammer.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/library/src/ta_func/cdlinvertedhammer.rs) |
| Java | [`Core_CDLINVERTEDHAMMER.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/fragments/Core_CDLINVERTEDHAMMER.java) |
| C# | [`Core_CDLINVERTEDHAMMER.cs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/csharp/library/src/Core_CDLINVERTEDHAMMER.cs) |
| TypeScript | [`cdlinvertedhammer.ts`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/ts/library/src/func/cdlinvertedhammer.ts) |

Python: `talib.CDLINVERTEDHAMMER`, see [ta-lib-python](https://ta-lib.github.io/ta-lib-python/func_groups/pattern_recognition.html).

TA-Lib is also available for R and more using a [wrapper](/install/#wrappers).

## Aliases

Inverted Hammer

## See Also

[CDLHAMMER](/functions/cdlhammer.md) · [CDLSHOOTINGSTAR](/functions/cdlshootingstar.md) · [CDLHANGINGMAN](/functions/cdlhangingman.md)
