---
url: 'https://ta-lib.org/functions/cdl2crows.md'
description: >-
  Three-candle bearish reversal pattern: a long white candle, then a black
  candle gapping up, then a black candle that opens inside the second body and…
---
# Two Crows (CDL2CROWS)

## Summary

Three-candle bearish reversal pattern: a long white candle, then a black candle gapping up, then a black candle that opens inside the second body and closes down inside the first white body. A hit (-100) signals a bearish reversal; significant in an uptrend, which this function does not verify.

## Chart

<ta-chart src="/functions/cdl2crows.svg" ratio="720:274" label="CDL2CROWS over WBTC/USDC (Uniswap v3), daily, 2024" alt="Drawn on the price pane over candles, with outInteger as markers on the candles, an arrow down from the top of the pane to above the high for a bearish call."></ta-chart>

<div class="ta-chart-links">

[Reference data (CSV)](/functions/cdl2crows.csv) · [Chart (SVG)](/functions/cdl2crows.svg)

</div>

::: details Chart metadata <span class="flag-tip" tabindex="0" role="note" aria-label="The chart is drawn automatically from metadata TA-Lib provides. Your app can get the same through the Abstract API." data-tip="The chart is drawn automatically from metadata TA-Lib provides. Your app can get the same through the Abstract API.">i</span>

**Outputs**

* `outInteger`: `pattern_bull_bear` · `zero` · `negative`

:::

## Notes

* Does not verify the prior uptrend the pattern classically assumes for significance.
* Bulkowski's testing found this reverses bearishly only 54% of the time — "near random" — despite the pattern's classic always-bearish label; the breakout direction cannot be predicted with any real accuracy. ([thepatternsite.com](https://thepatternsite.com/TwoCrows.html))
* Rare: a few years of one symbol's daily bars may show none. Bulkowski ranks it 64th of 103 patterns by how often it occurs ([Two Crows](https://thepatternsite.com/TwoCrows.html)), and it fires once in [about 16,000 daily crypto bars](/api/candlestick-outputs/#how-often-patterns-fire).

## Inputs

* `inOpen` — Open price of each bar
* `inHigh` — High price of each bar
* `inLow` — Low price of each bar
* `inClose` — Close price of each bar

## Outputs

* `outInteger` — -100 on a detected pattern (always bearish), 0 otherwise. Never emits +100

## Output Values

What candlestick values mean: [Candlestick Outputs](/api/candlestick-outputs/). The function's flags declare which values it can return: [rW7](/spec/inputs-outputs/#rw7).

| Value | Meaning |
|-------|---------|
| -100 | Two Crows pattern detected: bearish |
| 0 | No pattern |

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
TA_CDL2CROWS( 0, n - 1, open, high, low, close, &outBegIdx, &outNBElement, out );
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
let range = core.cdl2crows(0, open.len() - 1, &open, &high, &low, &close, &mut out)?;
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
OutRange r = core.cdl2crows(0, open.length - 1, open, high, low, close, out);
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
OutRange r = core.Cdl2crows(0, open.Length - 1, open, high, low, close, outInteger);
Console.WriteLine($"{r.Count} values from bar {r.BegIdx}; last outInteger = {outInteger[r.Count - 1]}");
```

@tab TypeScript

```ts
import { cdl2crows } from 'ta-lib';

const open = new Float64Array(252); // your data, oldest first
const high = new Float64Array(252); // your data, oldest first
const low = new Float64Array(252); // your data, oldest first
const close = new Float64Array(252); // your data, oldest first

const { begIdx, values } = cdl2crows({ open, high, low, close });
console.log(values.length + ' values from bar ' + begIdx + '; last values = ' + values[values.length - 1]);
```

:::

## Implementation

Added in 0.4.0.

TA-Lib Definition: [`cdl2crows.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/cdl2crows/cdl2crows.c) · [`cdl2crows.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/cdl2crows/cdl2crows.yaml)

| Native | File |
|--------|------|
| C | [`ta_CDL2CROWS.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_CDL2CROWS.c) |
| Rust | [`cdl2crows.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/library/src/ta_func/cdl2crows.rs) |
| Java | [`Core_CDL2CROWS.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/fragments/Core_CDL2CROWS.java) |
| C# | [`Core_CDL2CROWS.cs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/csharp/library/src/Core_CDL2CROWS.cs) |
| TypeScript | [`cdl2crows.ts`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/ts/library/src/func/cdl2crows.ts) |

Python: `talib.CDL2CROWS`, see [ta-lib-python](https://ta-lib.github.io/ta-lib-python/func_groups/pattern_recognition.html).

TA-Lib is also available for R and more using a [wrapper](/install/#wrappers).

## Aliases

Two Crows

## See Also

[CDLUPSIDEGAP2CROWS](/functions/cdlupsidegap2crows.md) · [CDLIDENTICAL3CROWS](/functions/cdlidentical3crows.md)
