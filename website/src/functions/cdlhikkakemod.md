---
title: "Modified Hikkake Pattern (CDLHIKKAKEMOD)"
description: "A four-candle pattern: two successively narrower inside bars, then a breakout bar, with the second candle closing near one extreme of its range."
---

## Summary

A four-candle pattern: two successively narrower inside bars, then a breakout bar, with the second candle closing near one extreme of its range. Bullish or bearish reversal signal. Bullish (+) or bearish (-) reversal; per the code's note it is significant in a downtrend (bull) or uptrend (bear), context the code does not verify.

## Chart

<ta-chart src="/functions/cdlhikkakemod.svg" ratio="720:274" label="CDLHIKKAKEMOD over WBTC/USDC (Uniswap v3), daily, 2022-2023" alt="Drawn on the price pane over candles, with outInteger as markers on the candles, an arrow up from the bottom of the pane to below the low for a bullish call and an arrow down from the top of the pane to above the high for a bearish one, a confirmed call (200) with a double head."></ta-chart>

<div class="ta-chart-links">

[Reference data (CSV)](/functions/cdlhikkakemod.csv) · [Chart (SVG)](/functions/cdlhikkakemod.svg)

</div>

::: details Chart metadata <span class="flag-tip" tabindex="0" role="note" aria-label="The chart is drawn automatically from metadata TA-Lib provides. Your app can get the same through the Abstract API." data-tip="The chart is drawn automatically from metadata TA-Lib provides. Your app can get the same through the Abstract API.">i</span>

**Outputs**

- `outInteger`: `line` · `pattern_bull_bear` · `zero` · `positive` · `negative` · `pattern_confirm`

:::

## Notes

- Does not verify the prior trend (downtrend for bullish, uptrend for bearish) that this reversal pattern assumes.
- A bar that is both a new setup and the confirmation of an earlier one reports the new setup; the earlier one ends unconfirmed.

## Inputs

- `inOpen` — Open price of each bar
- `inHigh` — High price of each bar
- `inLow` — Low price of each bar
- `inClose` — Close price of each bar

## Outputs

- `outInteger` — +100 bullish hikkake bar, -100 bearish; +200 confirmed bullish, -200 confirmed bearish; 0 otherwise

## Output Values

What candlestick values mean: [Candlestick Outputs](/api/candlestick-outputs/). The function's flags declare which values it can return: [rW7](/spec/inputs-outputs/#rw7).

| Value | Meaning |
|-------|---------|
| -200 | Bearish Modified Hikkake confirmed: within 3 bars, a close below the last inside bar's low confirms the reversal lower |
| -100 | Bearish Modified Hikkake — a false upside breakout traps buyers, warning that the uptrend may be topping out |
| 0 | No pattern on this bar; a setup may still be awaiting confirmation |
| 100 | Bullish Modified Hikkake — a false downside breakout traps sellers, warning that the downtrend may be bottoming out |
| 200 | Bullish Modified Hikkake confirmed: within 3 bars, a close above the last inside bar's high confirms the reversal higher |

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
TA_CDLHIKKAKEMOD( 0, n - 1, open, high, low, close, &outBegIdx, &outNBElement, out );
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
let range = core.cdlhikkakemod(0, open.len() - 1, &open, &high, &low, &close, &mut out)?;
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
OutRange r = core.cdlhikkakemod(0, open.length - 1, open, high, low, close, out);
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
OutRange r = core.Cdlhikkakemod(0, open.Length - 1, open, high, low, close, outInteger);
Console.WriteLine($"{r.Count} values from bar {r.BegIdx}; last outInteger = {outInteger[r.Count - 1]}");
```

@tab TypeScript

```ts
import { cdlhikkakemod } from 'ta-lib';

const open = new Float64Array(252); // your data, oldest first
const high = new Float64Array(252); // your data, oldest first
const low = new Float64Array(252); // your data, oldest first
const close = new Float64Array(252); // your data, oldest first

const { begIdx, values } = cdlhikkakemod({ open, high, low, close });
console.log(values.length + ' values from bar ' + begIdx + '; last values = ' + values[values.length - 1]);
```

:::

## Implementation

Added in 0.4.0.

TA-Lib Definition: [`cdlhikkakemod.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/cdlhikkakemod/cdlhikkakemod.c) · [`cdlhikkakemod.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/cdlhikkakemod/cdlhikkakemod.yaml)

| Native | File |
|--------|------|
| C | [`ta_CDLHIKKAKEMOD.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_CDLHIKKAKEMOD.c) |
| Rust | [`cdlhikkakemod.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/library/src/ta_func/cdlhikkakemod.rs) |
| Java | [`Core_CDLHIKKAKEMOD.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/fragments/Core_CDLHIKKAKEMOD.java) |
| C# | [`Core_CDLHIKKAKEMOD.cs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/csharp/library/src/Core_CDLHIKKAKEMOD.cs) |
| TypeScript | [`cdlhikkakemod.ts`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/ts/library/src/func/cdlhikkakemod.ts) |

Python: `talib.CDLHIKKAKEMOD`, see [ta-lib-python](https://ta-lib.github.io/ta-lib-python/func_groups/pattern_recognition.html).

TA-Lib is also available for R and more using a [wrapper](/install/#wrappers).

## Aliases

Modified Hikkake, Modified Hikkake Pattern

## See Also

[CDLHIKKAKE](/functions/cdlhikkake.md)
