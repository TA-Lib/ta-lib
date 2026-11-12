---
url: 'https://ta-lib.org/functions/cdllongleggeddoji.md'
description: >-
  Single-candle doji (open ~ close) with at least one long shadow. Signals
  market indecision, not a directional bias.
---
# Long Legged Doji (CDLLONGLEGGEDDOJI)

## Summary

Single-candle doji (open ~ close) with at least one long shadow. Signals market indecision, not a directional bias. Marks indecision/uncertainty; not inherently bullish or bearish despite the positive sign.

## Chart

<ta-chart src="/functions/cdllongleggeddoji.svg" ratio="720:274" label="CDLLONGLEGGEDDOJI over IBM, daily, 1999" alt="Drawn on the price pane over candles, with outInteger as an arrow up from the bottom of the pane to below the low of every bar it flags."></ta-chart>

<div class="ta-chart-links">

[Reference data (CSV)](/functions/cdllongleggeddoji.csv) · [Chart (SVG)](/functions/cdllongleggeddoji.svg)

</div>

::: details Chart metadata <span class="flag-tip" tabindex="0" role="note" aria-label="The chart is drawn automatically from metadata TA-Lib provides. Your app can get the same through the Abstract API." data-tip="The chart is drawn automatically from metadata TA-Lib provides. Your app can get the same through the Abstract API.">i</span>

**Outputs**

* `outInteger`: `pattern_bool` · `zero` · `positive`

:::

## Formula

One candle. Hit when: real body <= BodyDoji average (doji body) AND (lower shadow > ShadowLong average OR upper shadow > ShadowLong average), i.e. at least one long shadow.

## Notes

* Only one long shadow (upper or lower) is required, whereas the classic pattern shows both long upper and lower shadows.
* Bulkowski's testing found this continues in the direction of the prior trend only 51% of the time — statistically random — and ranks 37th of 103 patterns overall; in his words, "it means nothing." ([thepatternsite.com](https://thepatternsite.com/LongLegDoji.html))

## Inputs

* `inOpen` — Open price of each bar
* `inHigh` — High price of each bar
* `inLow` — Low price of each bar
* `inClose` — Close price of each bar

## Outputs

* `outInteger` — +100 when the pattern is present, 0 otherwise. Only +100 is emitted; the code never emits -100, and the positive sign does NOT mean bullish

## Output Values

What candlestick values mean: [Candlestick Outputs](/api/candlestick-outputs/). The function's flags declare which values it can return: [rW7](/spec/inputs-outputs/#rw7).

| Value | Meaning |
|-------|---------|
| 0 | No pattern |
| 100 | Long-legged doji detected — signals market indecision, not a directional (bullish/bearish) bias |

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
TA_CDLLONGLEGGEDDOJI( 0, n - 1, open, high, low, close, &outBegIdx, &outNBElement, out );
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
let range = core.cdllongleggeddoji(0, open.len() - 1, &open, &high, &low, &close,
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
OutRange r = core.cdllongleggeddoji(0, open.length - 1, open, high, low, close, out);
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
OutRange r = core.Cdllongleggeddoji(0, open.Length - 1, open, high, low, close, outInteger);
Console.WriteLine($"{r.Count} values from bar {r.BegIdx}; last outInteger = {outInteger[r.Count - 1]}");
```

@tab TypeScript

```ts
import { cdllongleggeddoji } from 'ta-lib';

const open = new Float64Array(252); // your data, oldest first
const high = new Float64Array(252); // your data, oldest first
const low = new Float64Array(252); // your data, oldest first
const close = new Float64Array(252); // your data, oldest first

const { begIdx, values } = cdllongleggeddoji({ open, high, low, close });
console.log(values.length + ' values from bar ' + begIdx + '; last values = ' + values[values.length - 1]);
```

:::

## Implementation

Added in 0.4.0.

TA-Lib Definition: [`cdllongleggeddoji.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/cdllongleggeddoji/cdllongleggeddoji.c) · [`cdllongleggeddoji.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/cdllongleggeddoji/cdllongleggeddoji.yaml)

| Native | File |
|--------|------|
| C | [`ta_CDLLONGLEGGEDDOJI.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_CDLLONGLEGGEDDOJI.c) |
| Rust | [`cdllongleggeddoji.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/library/src/ta_func/cdllongleggeddoji.rs) |
| Java | [`Core_CDLLONGLEGGEDDOJI.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/fragments/Core_CDLLONGLEGGEDDOJI.java) |
| C# | [`Core_CDLLONGLEGGEDDOJI.cs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/csharp/library/src/Core_CDLLONGLEGGEDDOJI.cs) |
| TypeScript | [`cdllongleggeddoji.ts`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/ts/library/src/func/cdllongleggeddoji.ts) |

Python: `talib.CDLLONGLEGGEDDOJI`, see [ta-lib-python](https://ta-lib.github.io/ta-lib-python/func_groups/pattern_recognition.html).

TA-Lib is also available for R and more using a [wrapper](/install/#wrappers).

## Aliases

Long Legged Doji

## See Also

[CDLDOJI](/functions/cdldoji.md) · [CDLGRAVESTONEDOJI](/functions/cdlgravestonedoji.md) · [CDLDRAGONFLYDOJI](/functions/cdldragonflydoji.md) · [CDLRICKSHAWMAN](/functions/cdlrickshawman.md)
