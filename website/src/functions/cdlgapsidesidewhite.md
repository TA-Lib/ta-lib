---
title: "Up/Down-gap side-by-side white lines (CDLGAPSIDESIDEWHITE)"
description: "A three-candle pattern: a first candle followed by two white candles of similar body size that both gap the same direction (up or down) from the first…"
---

## Summary

A three-candle pattern: a first candle followed by two white candles of similar body size that both gap the same direction (up or down) from the first candle's real body and open at about the same level. It is a continuation signal whose sign reports the gap direction; the code does not verify a prior trend.

## Chart

<ta-chart src="/functions/cdlgapsidesidewhite.svg" ratio="720:274" label="CDLGAPSIDESIDEWHITE over WBTC/USDC (Uniswap v3), daily, 2022" alt="Drawn on the price pane over candles, with outInteger as markers on the candles, ▲ below the low for a bullish call and ▼ above the high for a bearish one."></ta-chart>

<div class="ta-chart-links">

[Reference data (CSV)](/functions/cdlgapsidesidewhite.csv) · [Chart (SVG)](/functions/cdlgapsidesidewhite.svg)

</div>

::: details Chart metadata <span class="flag-tip" tabindex="0" role="note" aria-label="The chart is drawn automatically from metadata TA-Lib provides. Your app can get the same through the Abstract API." data-tip="The chart is drawn automatically from metadata TA-Lib provides. Your app can get the same through the Abstract API.">i</span>

**Outputs**

- `outInteger`: `line` · `pattern_bull_bear` · `zero` · `positive` · `negative`

:::

## Notes

- Does not verify the prior trend the continuation signal classically assumes.
- Bulkowski's data shows the bullish form continues as labeled 66% of the time; the bearish form's 56% continuation rate is "near random", and Bulkowski cautions the bearish sample is too thin to trust. ([thepatternsite.com](https://thepatternsite.com/SidebySideWhiteLinesBull.html))
- Rare in Bulkowski's tests: he ranks the bullish form 73rd and the bearish 86th of 103 patterns by how often they occur ([bullish](https://thepatternsite.com/SidebySideWhiteLinesBull.html), [bearish](https://thepatternsite.com/SidebySideWhiteLinesBear.html)). It fires more often in crypto: 18 times in [about 16,000 daily bars](/api/candlestick-outputs/#how-often-patterns-fire).

## Inputs

- `inOpen` — Open price of each bar
- `inHigh` — High price of each bar
- `inLow` — Low price of each bar
- `inClose` — Close price of each bar

## Outputs

- `outInteger` — +100 for an up-gap (bullish continuation), -100 for a down-gap (bearish continuation), 0 when no pattern. Sign is set solely by the C2-vs-C1 gap direction (realbodygapup ? 100 : -100)

## Output Values

What candlestick values mean: [Candlestick Outputs](/api/candlestick-outputs/). The function's flags declare which values it can return: [rW7](/spec/inputs-outputs/#rw7).

| Value | Meaning |
|-------|---------|
| -100 | Bearish continuation: two similar white candles gapped down together and held the gap, suggesting the decline will resume |
| 0 | No pattern |
| 100 | Bullish continuation: two similar white candles gapped up together and held the gap, suggesting the advance will resume |

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
TA_CDLGAPSIDESIDEWHITE( 0, n - 1, open, high, low, close, &outBegIdx, &outNBElement, out );
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
let range = core.cdlgapsidesidewhite(0, open.len() - 1, &open, &high, &low, &close,
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
OutRange r = core.cdlgapsidesidewhite(0, open.length - 1, open, high, low, close, out);
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
OutRange r = core.Cdlgapsidesidewhite(0, open.Length - 1, open, high, low, close, outInteger);
Console.WriteLine($"{r.Count} values from bar {r.BegIdx}; last outInteger = {outInteger[r.Count - 1]}");
```

@tab TypeScript

```ts
import { cdlgapsidesidewhite } from 'ta-lib';

const open = new Float64Array(252); // your data, oldest first
const high = new Float64Array(252); // your data, oldest first
const low = new Float64Array(252); // your data, oldest first
const close = new Float64Array(252); // your data, oldest first

const { begIdx, values } = cdlgapsidesidewhite({ open, high, low, close });
console.log(values.length + ' values from bar ' + begIdx + '; last values = ' + values[values.length - 1]);
```

:::

## Implementation

Added in 0.4.0.

TA-Lib Definition: [`cdlgapsidesidewhite.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/cdlgapsidesidewhite/cdlgapsidesidewhite.c) · [`cdlgapsidesidewhite.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/cdlgapsidesidewhite/cdlgapsidesidewhite.yaml)

| Native | File |
|--------|------|
| C | [`ta_CDLGAPSIDESIDEWHITE.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_CDLGAPSIDESIDEWHITE.c) |
| Rust | [`cdlgapsidesidewhite.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/library/src/ta_func/cdlgapsidesidewhite.rs) |
| Java | [`Core_CDLGAPSIDESIDEWHITE.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/fragments/Core_CDLGAPSIDESIDEWHITE.java) |
| C# | [`Core_CDLGAPSIDESIDEWHITE.cs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/csharp/library/src/Core_CDLGAPSIDESIDEWHITE.cs) |
| TypeScript | [`cdlgapsidesidewhite.ts`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/ts/library/src/func/cdlgapsidesidewhite.ts) |

Python: `talib.CDLGAPSIDESIDEWHITE`, see [ta-lib-python](https://ta-lib.github.io/ta-lib-python/func_groups/pattern_recognition.html).

TA-Lib is also available for R and more using a [wrapper](/install/#wrappers).

## Aliases

Up/Down-gap side-by-side white lines, Gapping side-by-side white lines

## See Also

[CDLTASUKIGAP](/functions/cdltasukigap.md) · [CDLXSIDEGAP3METHODS](/functions/cdlxsidegap3methods.md)
