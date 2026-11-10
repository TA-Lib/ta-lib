---
title: "Hikkake Pattern (CDLHIKKAKE)"
description: "A 3-bar pattern: an inside bar followed by a false breakout, optionally later confirmed by a follow-through bar."
---

## Summary

A 3-bar pattern: an inside bar followed by a false breakout, optionally later confirmed by a follow-through bar. Signals a bullish or bearish reversal/continuation depending on the breakout direction. A false-breakout setup: positive = bullish, negative = bearish; magnitude 200 flags the confirming bar.

## Chart

<ta-chart src="/functions/cdlhikkake.svg" ratio="720:274" label="CDLHIKKAKE over IBM, daily, 1999" alt="Drawn on the price pane over candles, with outInteger as markers on the candles, ▲ below the low for a bullish call and ▼ above the high for a bearish one, a confirmed call (200) ringed."></ta-chart>

::: details Chart metadata <span class="flag-tip" tabindex="0" role="note" aria-label="The chart is drawn automatically from metadata TA-Lib provides. Your app can get the same through the Abstract API." data-tip="The chart is drawn automatically from metadata TA-Lib provides. Your app can get the same through the Abstract API.">i</span>

Drawn on the price pane over candles, with outInteger as markers on the candles, ▲ below the low for a bullish call and ▼ above the high for a bearish one, a confirmed call (200) ringed.

Metadata flags used: `candlestick` · `inPriceOHLC: open, high, low, close` · `outInteger: pattern_bull_bear, pattern_confirm, zero, positive, negative`

:::

[Reference data (CSV)](/functions/cdlhikkake.csv) · [Chart (SVG)](/functions/cdlhikkake.svg)

## Notes

- The name comes from the Japanese word for a deceptive move or "trap" — fitting, since the pattern exists to catch traders acting on a false breakout. Bulkowski's testing of the confirmed pattern found the trap itself barely beats a coin flip: the bullish variant continues as expected only 52% of the time and the bearish variant exactly 50% ("random"), both ranking in the bottom fifth (83rd-84th of 105) for post-breakout performance. ([thepatternsite.com](https://thepatternsite.com/HikkakeBull.html))
- A bar that is both a new setup and the confirmation of an earlier one reports the new setup; the earlier one ends unconfirmed.

## Inputs

- `inOpen` — Open price of each bar
- `inHigh` — High price of each bar
- `inLow` — Low price of each bar
- `inClose` — Close price of each bar

## Outputs

- `outInteger` — +100/-100 at the hikkake (breakout) bar for bull/bear; +200/-200 at a later confirmation bar; 0 otherwise

## Output Values

What candlestick values mean: [Candlestick Outputs](/api/candlestick-outputs/). The function's flags declare which values it can return: [rW7](/spec/inputs-outputs/#rw7).

| Value | Meaning |
|-------|---------|
| -200 | Bearish Hikkake confirmed: within 3 bars, a close below the inside bar's low validates the trap and the move lower |
| -100 | Bearish Hikkake — a false upside breakout from a tight, inside-bar range traps buyers before price turns back down |
| 0 | No pattern on this bar; a setup may still be awaiting confirmation |
| 100 | Bullish Hikkake — a false downside breakout from a tight, inside-bar range traps sellers before price turns back up |
| 200 | Bullish Hikkake confirmed: within 3 bars, a close above the inside bar's high validates the trap and the move higher |

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
TA_CDLHIKKAKE( 0, n - 1, open, high, low, close, &outBegIdx, &outNBElement, out );
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
let range = core.cdlhikkake(0, open.len() - 1, &open, &high, &low, &close, &mut out)?;
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
OutRange r = core.cdlhikkake(0, open.length - 1, open, high, low, close, out);
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
OutRange r = core.Cdlhikkake(0, open.Length - 1, open, high, low, close, outInteger);
Console.WriteLine($"{r.Count} values from bar {r.BegIdx}; last outInteger = {outInteger[r.Count - 1]}");
```

@tab TypeScript

```ts
import { cdlhikkake } from 'ta-lib';

const open = new Float64Array(252); // your data, oldest first
const high = new Float64Array(252); // your data, oldest first
const low = new Float64Array(252); // your data, oldest first
const close = new Float64Array(252); // your data, oldest first

const { begIdx, values } = cdlhikkake({ open, high, low, close });
console.log(values.length + ' values from bar ' + begIdx + '; last values = ' + values[values.length - 1]);
```

:::

## Implementation

Added in 0.4.0.

TA-Lib Definition: [`cdlhikkake.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/cdlhikkake/cdlhikkake.c) · [`cdlhikkake.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/cdlhikkake/cdlhikkake.yaml)

| Native | File |
|--------|------|
| C | [`ta_CDLHIKKAKE.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_CDLHIKKAKE.c) |
| Rust | [`cdlhikkake.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/library/src/ta_func/cdlhikkake.rs) |
| Java | [`Core_CDLHIKKAKE.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/fragments/Core_CDLHIKKAKE.java) |
| C# | [`Core_CDLHIKKAKE.cs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/csharp/library/src/Core_CDLHIKKAKE.cs) |
| TypeScript | [`cdlhikkake.ts`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/ts/library/src/func/cdlhikkake.ts) |

Python: `talib.CDLHIKKAKE`, see [ta-lib-python](https://ta-lib.github.io/ta-lib-python/func_groups/pattern_recognition.html).

TA-Lib is also available for R and more using a [wrapper](/install/#wrappers).

## Aliases

Hikkake Pattern, Hikkake

## See Also

[CDLHIKKAKEMOD](/functions/cdlhikkakemod.md) · [CDLHARAMI](/functions/cdlharami.md)
