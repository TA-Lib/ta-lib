---
url: 'https://ta-lib.org/functions/cdlmorningdojistar.md'
description: >-
  A three-candle bullish reversal pattern: a long black candle, then a doji that
  gaps down, then a white candle closing well up into the first candle's body.
---
# Morning Doji Star (CDLMORNINGDOJISTAR)

## Summary

A three-candle bullish reversal pattern: a long black candle, then a doji that gaps down, then a white candle closing well up into the first candle's body. It is the doji-star variant of the morning star. A hit (+100) signals a bullish reversal; most meaningful after a downtrend, which this function does not verify.

## Chart

<ta-chart src="/functions/cdlmorningdojistar.svg" ratio="720:274" label="CDLMORNINGDOJISTAR over WBTC/USDC (Uniswap v3), daily" alt="Drawn on the price pane over candles, with outInteger as markers on the candles, ▲ below the low for a bullish call."></ta-chart>

::: details Chart metadata <span class="flag-tip" tabindex="0" role="note" aria-label="The chart is drawn from the function's metadata: the flags and attributes the abstract API reports for it and for each output, and the input series its reference run declares." data-tip="The chart is drawn from the function's metadata: the flags and attributes the abstract API reports for it and for each output, and the input series its reference run declares.">i</span>

Drawn on the price pane over candles, with outInteger as markers on the candles, ▲ below the low for a bullish call.

Metadata flags used: `candlestick` · `inPriceOHLC: open, high, low, close` · `outInteger: pattern_bull_bear, zero, positive`

:::

[Reference data (CSV)](/functions/cdlmorningdojistar.csv) · [Chart (SVG)](/functions/cdlmorningdojistar.svg)

## Notes

* The gap-down is measured between the candles' real bodies, not between their high/low ranges.
* A prior downtrend is not verified.

## Inputs

* `inOpen` — Open price of each bar
* `inHigh` — High price of each bar
* `inLow` — Low price of each bar
* `inClose` — Close price of each bar

## Outputs

* `outInteger` — +100 when the pattern is detected, 0 otherwise. Always bullish; never emits -100

## Output Values

What candlestick values mean: [Candlestick Outputs](/api/candlestick-outputs/). The function's flags declare which values it can return: [rW7](/spec/inputs-outputs/#rw7).

| Value | Meaning |
|-------|---------|
| 0 | No pattern |
| 100 | Morning Doji Star detected — bullish reversal signal |

## Parameters

| Parameter | Type | Default | Accepted values | Description |
| --- | --- | --- | --- | --- |
| `optInPenetration` | real | 0.3 | ≥ 0 | Fraction of the 1st candle's real body the 3rd close must exceed above close\[i-2]; larger values demand deeper penetration into the black body |

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
TA_CDLMORNINGDOJISTAR( 0, n - 1, open, high, low, close, 0.3, &outBegIdx, &outNBElement, out );
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
let range = core.cdlmorningdojistar(0, open.len() - 1, &open, &high, &low, &close, 0.3,
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
OutRange r = core.cdlmorningdojistar(0, open.length - 1, open, high, low, close, 0.3, out);
System.out.println(r.count() + " values from bar " + r.begIdx() + "; last out = " + out[r.count() - 1]);
```

@tab C#

```csharp
using System;
using TALib;

var open = new double[252]; // your data, oldest first
var high = new double[252]; // your data, oldest first
var low = new double[252]; // your data, oldest first
var close = new double[252]; // your data, oldest first
var outInteger = new int[252];

var core = Core.Default;
OutRange r = core.Cdlmorningdojistar(0, open.Length - 1, open, high, low, close, 0.3, outInteger);
Console.WriteLine($"{r.Count} values from bar {r.BegIdx}; last outInteger = {outInteger[r.Count - 1]}");
```

@tab TypeScript

```ts
import { cdlmorningdojistar } from 'ta-lib';

const open = new Float64Array(252); // your data, oldest first
const high = new Float64Array(252); // your data, oldest first
const low = new Float64Array(252); // your data, oldest first
const close = new Float64Array(252); // your data, oldest first

const { begIdx, values } = cdlmorningdojistar({ open, high, low, close }, {
  penetration: 0.3,
});
console.log(values.length + ' values from bar ' + begIdx + '; last values = ' + values[values.length - 1]);
```

:::

## Implementation

Added in 0.4.0.

TA-Lib Definition: [`cdlmorningdojistar.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/cdlmorningdojistar/cdlmorningdojistar.c) · [`cdlmorningdojistar.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/cdlmorningdojistar/cdlmorningdojistar.yaml)

| Native | File |
|--------|------|
| C | [`ta_CDLMORNINGDOJISTAR.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_CDLMORNINGDOJISTAR.c) |
| Rust | [`cdlmorningdojistar.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/library/src/ta_func/cdlmorningdojistar.rs) |
| Java | [`Core_CDLMORNINGDOJISTAR.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/fragments/Core_CDLMORNINGDOJISTAR.java) |
| C# | [`Core_CDLMORNINGDOJISTAR.cs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/csharp/library/src/Core_CDLMORNINGDOJISTAR.cs) |
| TypeScript | [`cdlmorningdojistar.ts`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/ts/library/src/func/cdlmorningdojistar.ts) |

Python: `talib.CDLMORNINGDOJISTAR`, see [ta-lib-python](https://ta-lib.github.io/ta-lib-python/func_groups/pattern_recognition.html).

TA-Lib is also available for R and more using a [wrapper](/install/#wrappers).

## Aliases

Morning Doji Star

## See Also

[CDLMORNINGSTAR](/functions/cdlmorningstar.md) · [CDLEVENINGDOJISTAR](/functions/cdleveningdojistar.md) · [CDLEVENINGSTAR](/functions/cdleveningstar.md) · [CDLDOJISTAR](/functions/cdldojistar.md)
