---
title: "Belt-hold (CDLBELTHOLD)"
description: "Single-candle pattern with a long real body that opens at (or near) its extreme."
---

## Summary

Single-candle pattern with a long real body that opens at (or near) its extreme. A bullish belt-hold is a long white candle with no/very short lower shadow; a bearish belt-hold is a long black candle with no/very short upper shadow. A white hit is bullish (opens at the low, closes strong); a black hit is bearish (opens at the high, closes weak).

## Chart

<ta-chart src="/functions/cdlbelthold.svg" ratio="720:274" label="CDLBELTHOLD over IBM, daily, 1999" alt="Drawn on the price pane over candles, with outInteger as markers on the candles, ▲ below the low for a bullish call and ▼ above the high for a bearish one."></ta-chart>

<div class="ta-chart-links">

[Reference data (CSV)](/functions/cdlbelthold.csv) · [Chart (SVG)](/functions/cdlbelthold.svg)

</div>

::: details Chart metadata <span class="flag-tip" tabindex="0" role="note" aria-label="The chart is drawn automatically from metadata TA-Lib provides. Your app can get the same through the Abstract API." data-tip="The chart is drawn automatically from metadata TA-Lib provides. Your app can get the same through the Abstract API.">i</span>

**Outputs**

- `outInteger`: `line` · `pattern_bull_bear` · `zero` · `positive` · `negative`

:::

## Formula

One candle. Requires real body > BodyLong average (long body), then either: white body (close>=open) AND lower shadow < ShadowVeryShort average -> bullish; OR black body (close<open) AND upper shadow < ShadowVeryShort average -> bearish. No prior-trend or gap conditions are checked.

## Notes

- Does not verify the prior trend that the pattern's bullish/bearish reading classically assumes.
- Bulkowski's testing ranks the bullish Belt-Hold's 71% reversal rate 11th of 103 patterns for pure reversal reliability (bearish reverses 68% of the time) — though its overall post-breakout performance rank is a more middling 62nd/63rd of 103. ([thepatternsite.com](https://thepatternsite.com/BeltHoldBull.html))

## Inputs

- `inOpen` — Open price of each bar
- `inHigh` — High price of each bar
- `inLow` — Low price of each bar
- `inClose` — Close price of each bar

## Outputs

- `outInteger` — +100 for a bullish (white) belt-hold, -100 for a bearish (black) belt-hold, 0 otherwise

## Output Values

What candlestick values mean: [Candlestick Outputs](/api/candlestick-outputs/). The function's flags declare which values it can return: [rW7](/spec/inputs-outputs/#rw7).

| Value | Meaning |
|-------|---------|
| -100 | Bearish belt-hold — opens at (or near) the high and sells off hard into the close, showing strong seller conviction |
| 0 | No pattern |
| 100 | Bullish belt-hold — opens at (or near) the low and rallies hard into the close, showing strong buyer conviction |

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
TA_CDLBELTHOLD( 0, n - 1, open, high, low, close, &outBegIdx, &outNBElement, out );
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
let range = core.cdlbelthold(0, open.len() - 1, &open, &high, &low, &close, &mut out)?;
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
OutRange r = core.cdlbelthold(0, open.length - 1, open, high, low, close, out);
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
OutRange r = core.Cdlbelthold(0, open.Length - 1, open, high, low, close, outInteger);
Console.WriteLine($"{r.Count} values from bar {r.BegIdx}; last outInteger = {outInteger[r.Count - 1]}");
```

@tab TypeScript

```ts
import { cdlbelthold } from 'ta-lib';

const open = new Float64Array(252); // your data, oldest first
const high = new Float64Array(252); // your data, oldest first
const low = new Float64Array(252); // your data, oldest first
const close = new Float64Array(252); // your data, oldest first

const { begIdx, values } = cdlbelthold({ open, high, low, close });
console.log(values.length + ' values from bar ' + begIdx + '; last values = ' + values[values.length - 1]);
```

:::

## Implementation

Added in 0.4.0.

TA-Lib Definition: [`cdlbelthold.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/cdlbelthold/cdlbelthold.c) · [`cdlbelthold.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/cdlbelthold/cdlbelthold.yaml)

| Native | File |
|--------|------|
| C | [`ta_CDLBELTHOLD.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_CDLBELTHOLD.c) |
| Rust | [`cdlbelthold.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/library/src/ta_func/cdlbelthold.rs) |
| Java | [`Core_CDLBELTHOLD.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/fragments/Core_CDLBELTHOLD.java) |
| C# | [`Core_CDLBELTHOLD.cs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/csharp/library/src/Core_CDLBELTHOLD.cs) |
| TypeScript | [`cdlbelthold.ts`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/ts/library/src/func/cdlbelthold.ts) |

Python: `talib.CDLBELTHOLD`, see [ta-lib-python](https://ta-lib.github.io/ta-lib-python/func_groups/pattern_recognition.html).

TA-Lib is also available for R and more using a [wrapper](/install/#wrappers).

## Aliases

Belt-hold, Belt Hold Line

## See Also

[CDLCLOSINGMARUBOZU](/functions/cdlclosingmarubozu.md) · [CDLMARUBOZU](/functions/cdlmarubozu.md) · [CDLLONGLINE](/functions/cdllongline.md)
