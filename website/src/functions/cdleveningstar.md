---
title: "Evening Star (CDLEVENINGSTAR)"
description: "A three-candle bearish reversal pattern: a long white candle, a short-bodied star gapping up, then a black candle closing well down into the first…"
---

## Summary

A three-candle bearish reversal pattern: a long white candle, a short-bodied star gapping up, then a black candle closing well down into the first candle's body. A hit signals a bearish reversal (most significant in an uptrend).

## Chart

<ta-chart src="/functions/cdleveningstar.svg" ratio="720:274" label="CDLEVENINGSTAR over IBM, daily, 1999" alt="Drawn on the price pane over candles, with outInteger as markers on the candles, ▼ above the high for a bearish call."></ta-chart>

::: details Chart metadata <span class="flag-tip" tabindex="0" role="note" aria-label="The chart is drawn from the function's metadata: the flags and attributes the abstract API reports for it and for each output, and the input series its reference run declares." data-tip="The chart is drawn from the function's metadata: the flags and attributes the abstract API reports for it and for each output, and the input series its reference run declares.">i</span>

Drawn on the price pane over candles, with outInteger as markers on the candles, ▼ above the high for a bearish call.

Metadata flags used: `candlestick` · `inPriceOHLC: open, high, low, close` · `outInteger: pattern_bull_bear, zero, negative`

:::

[Reference data (CSV)](/functions/cdleveningstar.csv) · [Chart (SVG)](/functions/cdleveningstar.svg)

## Notes

- Does not verify the preceding uptrend the bearish reversal classically assumes.
- The third candle only needs a body longer than short, not the full long body some definitions require.

## Inputs

- `inOpen` — Open price of each bar
- `inHigh` — High price of each bar
- `inLow` — Low price of each bar
- `inClose` — Close price of each bar

## Outputs

- `outInteger` — -100 when detected (always bearish), 0 otherwise. Never emits +100

## Output Values

What candlestick values mean: [Candlestick Outputs](/api/candlestick-outputs/). The function's flags declare which values it can return: [rW7](/spec/inputs-outputs/#rw7).

| Value | Meaning |
|-------|---------|
| -100 | Evening Star pattern detected: bearish |
| 0 | No pattern |

## Parameters

| Parameter | Type | Default | Accepted values | Description |
| --- | --- | --- | --- | --- |
| `optInPenetration` | real | 0.3 | ≥ 0 | Fraction of the 1st candle's real body the 3rd close must penetrate below the 1st close; larger requires deeper penetration |

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
TA_CDLEVENINGSTAR( 0, n - 1, open, high, low, close, 0.3, &outBegIdx, &outNBElement, out );
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
let range = core.cdleveningstar(0, open.len() - 1, &open, &high, &low, &close, 0.3,
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
OutRange r = core.cdleveningstar(0, open.length - 1, open, high, low, close, 0.3, out);
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
OutRange r = core.Cdleveningstar(0, open.Length - 1, open, high, low, close, 0.3, outInteger);
Console.WriteLine($"{r.Count} values from bar {r.BegIdx}; last outInteger = {outInteger[r.Count - 1]}");
```

@tab TypeScript

```ts
import { cdleveningstar } from 'ta-lib';

const open = new Float64Array(252); // your data, oldest first
const high = new Float64Array(252); // your data, oldest first
const low = new Float64Array(252); // your data, oldest first
const close = new Float64Array(252); // your data, oldest first

const { begIdx, values } = cdleveningstar({ open, high, low, close }, {
  penetration: 0.3,
});
console.log(values.length + ' values from bar ' + begIdx + '; last values = ' + values[values.length - 1]);
```

:::

## Implementation

Added in 0.4.0.

TA-Lib Definition: [`cdleveningstar.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/cdleveningstar/cdleveningstar.c) · [`cdleveningstar.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/cdleveningstar/cdleveningstar.yaml)

| Native | File |
|--------|------|
| C | [`ta_CDLEVENINGSTAR.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_CDLEVENINGSTAR.c) |
| Rust | [`cdleveningstar.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/library/src/ta_func/cdleveningstar.rs) |
| Java | [`Core_CDLEVENINGSTAR.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/fragments/Core_CDLEVENINGSTAR.java) |
| C# | [`Core_CDLEVENINGSTAR.cs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/csharp/library/src/Core_CDLEVENINGSTAR.cs) |
| TypeScript | [`cdleveningstar.ts`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/ts/library/src/func/cdleveningstar.ts) |

Python: `talib.CDLEVENINGSTAR`, see [ta-lib-python](https://ta-lib.github.io/ta-lib-python/func_groups/pattern_recognition.html).

TA-Lib is also available for R and more using a [wrapper](/install/#wrappers).

## Aliases

Evening Star

## See Also

[CDLEVENINGDOJISTAR](/functions/cdleveningdojistar.md) · [CDLMORNINGSTAR](/functions/cdlmorningstar.md) · [CDLMORNINGDOJISTAR](/functions/cdlmorningdojistar.md) · CDLSTARSINSOUTH
