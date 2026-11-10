---
title: "Matching Low (CDLMATCHINGLOW)"
description: "A two-candle pattern of two consecutive black (bearish) candles with equal closes (within a tolerance). Treated as a bullish reversal signal."
---

## Summary

A two-candle pattern of two consecutive black (bearish) candles with equal closes (within a tolerance). Treated as a bullish reversal signal. A hit signals a potential bullish reversal (shared support close after two down candles).

## Chart

<ta-chart src="/functions/cdlmatchinglow.svg" ratio="720:274" label="CDLMATCHINGLOW over IBM, daily, 1999" alt="Drawn on the price pane over candles, with outInteger as markers on the candles, ▲ below the low for a bullish call."></ta-chart>

<div class="ta-chart-links">

[Reference data (CSV)](/functions/cdlmatchinglow.csv) · [Chart (SVG)](/functions/cdlmatchinglow.svg)

</div>

::: details Chart metadata <span class="flag-tip" tabindex="0" role="note" aria-label="The chart is drawn automatically from metadata TA-Lib provides. Your app can get the same through the Abstract API." data-tip="The chart is drawn automatically from metadata TA-Lib provides. Your app can get the same through the Abstract API.">i</span>

**Outputs**

- `outInteger`: `line` · `pattern_bull_bear` · `zero` · `positive`

:::

## Formula

Two candles i-1, i. Candle i-1: black (close<open). Candle i: black (close<open). Equal closes: close[i-1]-E <= close[i] <= close[i-1]+E, where E = the Equal average. No shadow, body-size, or gap conditions are checked.

## Notes

- The bullish-reversal reading assumes a prior downtrend, which is not verified.
- Although classically read as a bullish reversal (and TA-Lib only emits +100), Bulkowski's testing found it actually acts as a bearish continuation 61% of the time — even so, it still ranks a strong 8th of 103 patterns for overall performance. ([thepatternsite.com](https://thepatternsite.com/MatchingLow.html))

## Inputs

- `inOpen` — Open price of each bar
- `inHigh` — High price of each bar
- `inLow` — Low price of each bar
- `inClose` — Close price of each bar

## Outputs

- `outInteger` — +100 when the pattern is present, 0 otherwise. Only +100 is ever emitted (matching low is always bullish); never -100

## Output Values

What candlestick values mean: [Candlestick Outputs](/api/candlestick-outputs/). The function's flags declare which values it can return: [rW7](/spec/inputs-outputs/#rw7).

| Value | Meaning |
|-------|---------|
| 0 | No pattern |
| 100 | Matching Low detected — bullish reversal signal (shared support close after two black candles) |

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
TA_CDLMATCHINGLOW( 0, n - 1, open, high, low, close, &outBegIdx, &outNBElement, out );
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
let range = core.cdlmatchinglow(0, open.len() - 1, &open, &high, &low, &close, &mut out)?;
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
OutRange r = core.cdlmatchinglow(0, open.length - 1, open, high, low, close, out);
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
OutRange r = core.Cdlmatchinglow(0, open.Length - 1, open, high, low, close, outInteger);
Console.WriteLine($"{r.Count} values from bar {r.BegIdx}; last outInteger = {outInteger[r.Count - 1]}");
```

@tab TypeScript

```ts
import { cdlmatchinglow } from 'ta-lib';

const open = new Float64Array(252); // your data, oldest first
const high = new Float64Array(252); // your data, oldest first
const low = new Float64Array(252); // your data, oldest first
const close = new Float64Array(252); // your data, oldest first

const { begIdx, values } = cdlmatchinglow({ open, high, low, close });
console.log(values.length + ' values from bar ' + begIdx + '; last values = ' + values[values.length - 1]);
```

:::

## Implementation

Added in 0.4.0.

TA-Lib Definition: [`cdlmatchinglow.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/cdlmatchinglow/cdlmatchinglow.c) · [`cdlmatchinglow.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/cdlmatchinglow/cdlmatchinglow.yaml)

| Native | File |
|--------|------|
| C | [`ta_CDLMATCHINGLOW.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_CDLMATCHINGLOW.c) |
| Rust | [`cdlmatchinglow.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/library/src/ta_func/cdlmatchinglow.rs) |
| Java | [`Core_CDLMATCHINGLOW.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/fragments/Core_CDLMATCHINGLOW.java) |
| C# | [`Core_CDLMATCHINGLOW.cs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/csharp/library/src/Core_CDLMATCHINGLOW.cs) |
| TypeScript | [`cdlmatchinglow.ts`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/ts/library/src/func/cdlmatchinglow.ts) |

Python: `talib.CDLMATCHINGLOW`, see [ta-lib-python](https://ta-lib.github.io/ta-lib-python/func_groups/pattern_recognition.html).

TA-Lib is also available for R and more using a [wrapper](/install/#wrappers).

## Aliases

Matching Low

## See Also

CDLMATCHINGHIGH · [CDLHOMINGPIGEON](/functions/cdlhomingpigeon.md)
