---
title: "Stalled Pattern (CDLSTALLEDPATTERN)"
description: "A three-candle pattern of three white candles with consecutively higher closes where the third loses momentum (a small body riding on the shoulder of the…"
---

## Summary

A three-candle pattern of three white candles with consecutively higher closes where the third loses momentum (a small body riding on the shoulder of the second's long body). It is a bearish reversal signal of a stalling advance. A hit (-100) is bearish: the uptrend is stalling and may reverse.

## Chart

<ta-chart src="/functions/cdlstalledpattern.svg" ratio="720:274" label="CDLSTALLEDPATTERN over IBM, daily, 1999" alt="Drawn on the price pane over candles, with outInteger as markers on the candles, ▼ above the high for a bearish call."></ta-chart>

::: details Chart metadata <span class="flag-tip" tabindex="0" role="note" aria-label="The chart is drawn automatically from metadata TA-Lib provides. Your app can get the same through the Abstract API." data-tip="The chart is drawn automatically from metadata TA-Lib provides. Your app can get the same through the Abstract API.">i</span>

Drawn on the price pane over candles, with outInteger as markers on the candles, ▼ above the high for a bearish call.

Metadata flags used: `candlestick` · `inPriceOHLC: open, high, low, close` · `outInteger: pattern_bull_bear, zero, negative`

:::

[Reference data (CSV)](/functions/cdlstalledpattern.csv) · [Chart (SVG)](/functions/cdlstalledpattern.svg)

## Notes

- The pattern classically appears in an uptrend, but this function does not verify a prior uptrend; the caller must confirm it.
- Bulkowski's testing shows this classically-bearish pattern actually acts as a bullish continuation 77% of the time — the reverse of the label — because price tends to close above the pattern's top rather than turning down. ([thepatternsite.com](https://thepatternsite.com/Deliberation.html))

## Inputs

- `inOpen` — Open price of each bar
- `inHigh` — High price of each bar
- `inLow` — Low price of each bar
- `inClose` — Close price of each bar

## Outputs

- `outInteger` — -100 when the pattern is detected (always bearish), 0 otherwise. Never emits +100

## Output Values

What candlestick values mean: [Candlestick Outputs](/api/candlestick-outputs/). The function's flags declare which values it can return: [rW7](/spec/inputs-outputs/#rw7).

| Value | Meaning |
|-------|---------|
| -100 | Stalled Pattern detected: bearish |
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
TA_CDLSTALLEDPATTERN( 0, n - 1, open, high, low, close, &outBegIdx, &outNBElement, out );
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
let range = core.cdlstalledpattern(0, open.len() - 1, &open, &high, &low, &close,
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
OutRange r = core.cdlstalledpattern(0, open.length - 1, open, high, low, close, out);
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
OutRange r = core.Cdlstalledpattern(0, open.Length - 1, open, high, low, close, outInteger);
Console.WriteLine($"{r.Count} values from bar {r.BegIdx}; last outInteger = {outInteger[r.Count - 1]}");
```

@tab TypeScript

```ts
import { cdlstalledpattern } from 'ta-lib';

const open = new Float64Array(252); // your data, oldest first
const high = new Float64Array(252); // your data, oldest first
const low = new Float64Array(252); // your data, oldest first
const close = new Float64Array(252); // your data, oldest first

const { begIdx, values } = cdlstalledpattern({ open, high, low, close });
console.log(values.length + ' values from bar ' + begIdx + '; last values = ' + values[values.length - 1]);
```

:::

## Implementation

Added in 0.4.0.

TA-Lib Definition: [`cdlstalledpattern.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/cdlstalledpattern/cdlstalledpattern.c) · [`cdlstalledpattern.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/cdlstalledpattern/cdlstalledpattern.yaml)

| Native | File |
|--------|------|
| C | [`ta_CDLSTALLEDPATTERN.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_CDLSTALLEDPATTERN.c) |
| Rust | [`cdlstalledpattern.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/library/src/ta_func/cdlstalledpattern.rs) |
| Java | [`Core_CDLSTALLEDPATTERN.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/fragments/Core_CDLSTALLEDPATTERN.java) |
| C# | [`Core_CDLSTALLEDPATTERN.cs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/csharp/library/src/Core_CDLSTALLEDPATTERN.cs) |
| TypeScript | [`cdlstalledpattern.ts`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/ts/library/src/func/cdlstalledpattern.ts) |

Python: `talib.CDLSTALLEDPATTERN`, see [ta-lib-python](https://ta-lib.github.io/ta-lib-python/func_groups/pattern_recognition.html).

TA-Lib is also available for R and more using a [wrapper](/install/#wrappers).

## Aliases

Stalled Pattern, Deliberation Pattern

## See Also

[CDLADVANCEBLOCK](/functions/cdladvanceblock.md) · [CDL3WHITESOLDIERS](/functions/cdl3whitesoldiers.md) · [CDLXSIDEGAP3METHODS](/functions/cdlxsidegap3methods.md)
