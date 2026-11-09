---
url: 'https://ta-lib.org/functions/cdlharami.md'
description: >-
  Two-candle pattern: a long real body followed by a short real body contained
  within the first candle's real body.
---
# Harami Pattern (CDLHARAMI)

## Summary

Two-candle pattern: a long real body followed by a short real body contained within the first candle's real body. A reversal signal whose direction is the opposite of the first candle's color.

## Chart

<ta-chart src="/functions/cdlharami.svg" ratio="720:274" label="CDLHARAMI over IBM, daily, 1999" alt="Drawn on the price pane over candles, with outInteger as markers on the candles, ▲ below the low for a bullish call and ▼ above the high for a bearish one, a weak call (80) hollow."></ta-chart>

::: details Chart metadata <span class="flag-tip" tabindex="0" role="note" aria-label="The chart is drawn from the function's metadata: the flags and attributes the abstract API reports for it and for each output, and the input series its reference run declares." data-tip="The chart is drawn from the function's metadata: the flags and attributes the abstract API reports for it and for each output, and the input series its reference run declares.">i</span>

Drawn on the price pane over candles, with outInteger as markers on the candles, ▲ below the low for a bullish call and ▼ above the high for a bearish one, a weak call (80) hollow.

Metadata flags used: `candlestick` · `inPriceOHLC: open, high, low, close` · `outInteger: pattern_bull_bear, pattern_weak, zero, positive, negative`

:::

[Reference data (CSV)](/functions/cdlharami.csv) · [Chart (SVG)](/functions/cdlharami.svg)

## Notes

* Does not verify the prior trend (downtrend for bullish, uptrend for bearish) that the reversal signal assumes.
* Bulkowski's testing found the bearish Harami actually acts as a bullish CONTINUATION 53% of the time — more often than it reverses the prior uptrend — rating the pattern "near random" overall (rank 72 of 103). ([thepatternsite.com](https://thepatternsite.com/HaramiBear.html))

## Inputs

* `inOpen` — Open price of each bar
* `inHigh` — High price of each bar
* `inLow` — Low price of each bar
* `inClose` — Close price of each bar

## Outputs

* `outInteger` — +100/+80 when the long 1st candle is black (bullish), -100/-80 when it is white (bearish), 0 otherwise; 80 when the two real bodies share an end, 100 when the 1st body strictly overhangs both ends of the 2nd

## Output Values

What candlestick values mean: [Candlestick Outputs](/api/candlestick-outputs/). The function's flags declare which values it can return: [rW7](/spec/inputs-outputs/#rw7).

| Value | Meaning |
|-------|---------|
| -100 | Bearish Harami: a small real body forms inside the prior long white candle — momentum stalling after an advance |
| -80 | Bearish Harami, weaker variant: one or both of the small body's edges line up exactly with the ends of the long white candle's body |
| 0 | No pattern |
| 80 | Bullish Harami, weaker variant: one or both of the small body's edges line up exactly with the ends of the long black candle's body |
| 100 | Bullish Harami: a small real body forms inside the prior long black candle — momentum stalling after a decline |

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
TA_CDLHARAMI( 0, n - 1, open, high, low, close, &outBegIdx, &outNBElement, out );
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
let range = core.cdlharami(0, open.len() - 1, &open, &high, &low, &close, &mut out)?;
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
OutRange r = core.cdlharami(0, open.length - 1, open, high, low, close, out);
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
OutRange r = core.Cdlharami(0, open.Length - 1, open, high, low, close, outInteger);
Console.WriteLine($"{r.Count} values from bar {r.BegIdx}; last outInteger = {outInteger[r.Count - 1]}");
```

@tab TypeScript

```ts
import { cdlharami } from 'ta-lib';

const open = new Float64Array(252); // your data, oldest first
const high = new Float64Array(252); // your data, oldest first
const low = new Float64Array(252); // your data, oldest first
const close = new Float64Array(252); // your data, oldest first

const { begIdx, values } = cdlharami({ open, high, low, close });
console.log(values.length + ' values from bar ' + begIdx + '; last values = ' + values[values.length - 1]);
```

:::

## Implementation

Added in 0.4.0.

TA-Lib Definition: [`cdlharami.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/cdlharami/cdlharami.c) · [`cdlharami.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/cdlharami/cdlharami.yaml)

| Native | File |
|--------|------|
| C | [`ta_CDLHARAMI.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_CDLHARAMI.c) |
| Rust | [`cdlharami.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/library/src/ta_func/cdlharami.rs) |
| Java | [`Core_CDLHARAMI.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/fragments/Core_CDLHARAMI.java) |
| C# | [`Core_CDLHARAMI.cs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/csharp/library/src/Core_CDLHARAMI.cs) |
| TypeScript | [`cdlharami.ts`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/ts/library/src/func/cdlharami.ts) |

Python: `talib.CDLHARAMI`, see [ta-lib-python](https://ta-lib.github.io/ta-lib-python/func_groups/pattern_recognition.html).

TA-Lib is also available for R and more using a [wrapper](/install/#wrappers).

## Aliases

Harami, Harami Pattern

## See Also

[CDLHARAMICROSS](/functions/cdlharamicross.md) · [CDLENGULFING](/functions/cdlengulfing.md)
