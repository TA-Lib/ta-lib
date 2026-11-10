---
url: 'https://ta-lib.org/functions/cdlhomingpigeon.md'
description: >-
  Two-candle pattern: a long black candle followed by a small black candle whose
  real body sits inside the prior body.
---
# Homing Pigeon (CDLHOMINGPIGEON)

## Summary

Two-candle pattern: a long black candle followed by a small black candle whose real body sits inside the prior body. A hit signals a bullish reversal, most meaningful in a downtrend, which the code does not verify.

## Chart

<ta-chart src="/functions/cdlhomingpigeon.svg" ratio="720:274" label="CDLHOMINGPIGEON over IBM, daily, 1999" alt="Drawn on the price pane over candles, with outInteger as markers on the candles, ▲ below the low for a bullish call."></ta-chart>

<div class="ta-chart-links">

[Reference data (CSV)](/functions/cdlhomingpigeon.csv) · [Chart (SVG)](/functions/cdlhomingpigeon.svg)

</div>

::: details Chart metadata <span class="flag-tip" tabindex="0" role="note" aria-label="The chart is drawn automatically from metadata TA-Lib provides. Your app can get the same through the Abstract API." data-tip="The chart is drawn automatically from metadata TA-Lib provides. Your app can get the same through the Abstract API.">i</span>

**Outputs**

* `outInteger`: `line` · `pattern_bull_bear` · `zero` · `positive`

:::

## Formula

Two candles at i-1 and i. Both black: close\[i-1] < open\[i-1] and close\[i] < open\[i]. First body long: realbody\[i-1] > BodyLong average. Second body short: realbody\[i] <= BodyShort average. Second body contained by first: open\[i] < open\[i-1] and close\[i] > close\[i-1].

## Notes

* Does not verify the preceding downtrend that the bullish reversal classically assumes.
* Despite the bullish-reversal label, Bulkowski's testing found this behaves as a bearish continuation 56% of the time — "near random" by his own description — though its overall post-breakout performance rank (21st of 103) is comparatively strong. ([thepatternsite.com](https://thepatternsite.com/HomingPigeon.html))

## Inputs

* `inOpen` — Open price of each bar
* `inHigh` — High price of each bar
* `inLow` — Low price of each bar
* `inClose` — Close price of each bar

## Outputs

* `outInteger` — +100 when the pattern is detected, 0 otherwise. Never emits -100 (always bullish)

## Output Values

What candlestick values mean: [Candlestick Outputs](/api/candlestick-outputs/). The function's flags declare which values it can return: [rW7](/spec/inputs-outputs/#rw7).

| Value | Meaning |
|-------|---------|
| 0 | No pattern |
| 100 | Homing Pigeon detected — a bullish reversal signal, most meaningful after a downtrend (unverified by the function) |

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
TA_CDLHOMINGPIGEON( 0, n - 1, open, high, low, close, &outBegIdx, &outNBElement, out );
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
let range = core.cdlhomingpigeon(0, open.len() - 1, &open, &high, &low, &close, &mut out)?;
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
OutRange r = core.cdlhomingpigeon(0, open.length - 1, open, high, low, close, out);
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
OutRange r = core.Cdlhomingpigeon(0, open.Length - 1, open, high, low, close, outInteger);
Console.WriteLine($"{r.Count} values from bar {r.BegIdx}; last outInteger = {outInteger[r.Count - 1]}");
```

@tab TypeScript

```ts
import { cdlhomingpigeon } from 'ta-lib';

const open = new Float64Array(252); // your data, oldest first
const high = new Float64Array(252); // your data, oldest first
const low = new Float64Array(252); // your data, oldest first
const close = new Float64Array(252); // your data, oldest first

const { begIdx, values } = cdlhomingpigeon({ open, high, low, close });
console.log(values.length + ' values from bar ' + begIdx + '; last values = ' + values[values.length - 1]);
```

:::

## Implementation

Added in 0.4.0.

TA-Lib Definition: [`cdlhomingpigeon.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/cdlhomingpigeon/cdlhomingpigeon.c) · [`cdlhomingpigeon.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/cdlhomingpigeon/cdlhomingpigeon.yaml)

| Native | File |
|--------|------|
| C | [`ta_CDLHOMINGPIGEON.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_CDLHOMINGPIGEON.c) |
| Rust | [`cdlhomingpigeon.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/library/src/ta_func/cdlhomingpigeon.rs) |
| Java | [`Core_CDLHOMINGPIGEON.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/fragments/Core_CDLHOMINGPIGEON.java) |
| C# | [`Core_CDLHOMINGPIGEON.cs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/csharp/library/src/Core_CDLHOMINGPIGEON.cs) |
| TypeScript | [`cdlhomingpigeon.ts`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/ts/library/src/func/cdlhomingpigeon.ts) |

Python: `talib.CDLHOMINGPIGEON`, see [ta-lib-python](https://ta-lib.github.io/ta-lib-python/func_groups/pattern_recognition.html).

TA-Lib is also available for R and more using a [wrapper](/install/#wrappers).

## Aliases

Homing Pigeon

## See Also

[CDLHARAMI](/functions/cdlharami.md) · [CDLMATCHINGLOW](/functions/cdlmatchinglow.md)
