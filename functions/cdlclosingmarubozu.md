---
url: 'https://ta-lib.org/functions/cdlclosingmarubozu.md'
description: >-
  Single-candle pattern: a long real body whose closing end has no or very short
  shadow, so the close sits at the candle's extreme.
---
# Closing Marubozu (CDLCLOSINGMARUBOZU)

## Summary

Single-candle pattern: a long real body whose closing end has no or very short shadow, so the close sits at the candle's extreme. A strong directional bar, not a defined reversal/continuation signal — white is bullish, black is bearish.

## Chart

<ta-chart src="/functions/cdlclosingmarubozu.svg" ratio="720:274" label="CDLCLOSINGMARUBOZU over IBM, daily, 1999" alt="Drawn on the price pane over candles, with outInteger as markers on the candles, an arrow up from the bottom of the pane to below the low for a bullish call and an arrow down from the top of the pane to above the high for a bearish one."></ta-chart>

<div class="ta-chart-links">

[Reference data (CSV)](/functions/cdlclosingmarubozu.csv) · [Chart (SVG)](/functions/cdlclosingmarubozu.svg)

</div>

::: details Chart metadata <span class="flag-tip" tabindex="0" role="note" aria-label="The chart is drawn automatically from metadata TA-Lib provides. Your app can get the same through the Abstract API." data-tip="The chart is drawn automatically from metadata TA-Lib provides. Your app can get the same through the Abstract API.">i</span>

**Outputs**

* `outInteger`: `pattern_bull_bear` · `zero` · `positive` · `negative`

:::

## Formula

One candle. Requires: (1) long real body: real body > the BodyLong average; AND (2) very short shadow at the closing end: if white (close>=open) upper shadow < the ShadowVeryShort average \[close at/near high]; if black (close\<open) lower shadow < the ShadowVeryShort average \[close at/near low].

## Notes

* Bulkowski's testing found Closing Marubozu continues in its expected direction only marginally more than chance — 52% for the black variant — which he calls "near random." ([thepatternsite.com](https://thepatternsite.com/CloseBlkMarubozu.html))

## Inputs

* `inOpen` — Open price of each bar
* `inHigh` — High price of each bar
* `inLow` — Low price of each bar
* `inClose` — Close price of each bar

## Outputs

* `outInteger` — +100 for a white (bullish) closing marubozu, -100 for a black (bearish) one, 0 otherwise

## Output Values

What candlestick values mean: [Candlestick Outputs](/api/candlestick-outputs/). The function's flags declare which values it can return: [rW7](/spec/inputs-outputs/#rw7).

| Value | Meaning |
|-------|---------|
| -100 | Closing Marubozu (bearish) — closes right at the session low, sellers in full control into the close |
| 0 | No pattern |
| 100 | Closing Marubozu (bullish) — closes right at the session high, buyers in full control into the close |

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
TA_CDLCLOSINGMARUBOZU( 0, n - 1, open, high, low, close, &outBegIdx, &outNBElement, out );
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
let range = core.cdlclosingmarubozu(0, open.len() - 1, &open, &high, &low, &close,
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
OutRange r = core.cdlclosingmarubozu(0, open.length - 1, open, high, low, close, out);
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
OutRange r = core.Cdlclosingmarubozu(0, open.Length - 1, open, high, low, close, outInteger);
Console.WriteLine($"{r.Count} values from bar {r.BegIdx}; last outInteger = {outInteger[r.Count - 1]}");
```

@tab TypeScript

```ts
import { cdlclosingmarubozu } from 'ta-lib';

const open = new Float64Array(252); // your data, oldest first
const high = new Float64Array(252); // your data, oldest first
const low = new Float64Array(252); // your data, oldest first
const close = new Float64Array(252); // your data, oldest first

const { begIdx, values } = cdlclosingmarubozu({ open, high, low, close });
console.log(values.length + ' values from bar ' + begIdx + '; last values = ' + values[values.length - 1]);
```

:::

## Implementation

Added in 0.4.0.

TA-Lib Definition: [`cdlclosingmarubozu.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/cdlclosingmarubozu/cdlclosingmarubozu.c) · [`cdlclosingmarubozu.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/cdlclosingmarubozu/cdlclosingmarubozu.yaml)

| Native | File |
|--------|------|
| C | [`ta_CDLCLOSINGMARUBOZU.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_CDLCLOSINGMARUBOZU.c) |
| Rust | [`cdlclosingmarubozu.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/library/src/ta_func/cdlclosingmarubozu.rs) |
| Java | [`Core_CDLCLOSINGMARUBOZU.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/fragments/Core_CDLCLOSINGMARUBOZU.java) |
| C# | [`Core_CDLCLOSINGMARUBOZU.cs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/csharp/library/src/Core_CDLCLOSINGMARUBOZU.cs) |
| TypeScript | [`cdlclosingmarubozu.ts`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/ts/library/src/func/cdlclosingmarubozu.ts) |

Python: `talib.CDLCLOSINGMARUBOZU`, see [ta-lib-python](https://ta-lib.github.io/ta-lib-python/func_groups/pattern_recognition.html).

TA-Lib is also available for R and more using a [wrapper](/install/#wrappers).

## Aliases

Closing Marubozu

## See Also

[CDLMARUBOZU](/functions/cdlmarubozu.md) · [CDLLONGLINE](/functions/cdllongline.md) · [CDLBELTHOLD](/functions/cdlbelthold.md)
