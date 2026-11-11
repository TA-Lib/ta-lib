---
title: "Evening Doji Star (CDLEVENINGDOJISTAR)"
description: "A three-candle bearish reversal pattern: a long white candle, a doji that gaps up (the star), then a black candle closing well down into the first…"
---

## Summary

A three-candle bearish reversal pattern: a long white candle, a doji that gaps up (the star), then a black candle closing well down into the first candle's body. A stricter Evening Star whose middle candle must be a doji. Hit (-100) signals a bearish top reversal.

## Chart

<ta-chart src="/functions/cdleveningdojistar.svg" ratio="720:274" label="CDLEVENINGDOJISTAR over IBM, daily, 1999" alt="Drawn on the price pane over candles, with outInteger as markers on the candles, an arrow down from the top of the pane to above the high for a bearish call."></ta-chart>

<div class="ta-chart-links">

[Reference data (CSV)](/functions/cdleveningdojistar.csv) · [Chart (SVG)](/functions/cdleveningdojistar.svg)

</div>

::: details Chart metadata <span class="flag-tip" tabindex="0" role="note" aria-label="The chart is drawn automatically from metadata TA-Lib provides. Your app can get the same through the Abstract API." data-tip="The chart is drawn automatically from metadata TA-Lib provides. Your app can get the same through the Abstract API.">i</span>

**Outputs**

- `outInteger`: `pattern_bull_bear` · `zero` · `negative`

:::

## Notes

- Does not verify the preceding uptrend the bearish reversal classically assumes.
- Rare in Bulkowski's tests: he ranks it 81st of 103 patterns by how often it occurs ([Evening Doji Star](https://thepatternsite.com/EveningDojiStar.html)). It fires more often in crypto: 25 times in [about 16,000 daily bars](/api/candlestick-outputs/#how-often-patterns-fire).

## Inputs

- `inOpen` — Open price of each bar
- `inHigh` — High price of each bar
- `inLow` — Low price of each bar
- `inClose` — Close price of each bar

## Outputs

- `outInteger` — -100 when the pattern is detected, 0 otherwise. Always bearish; never emits +100

## Output Values

What candlestick values mean: [Candlestick Outputs](/api/candlestick-outputs/). The function's flags declare which values it can return: [rW7](/spec/inputs-outputs/#rw7).

| Value | Meaning |
|-------|---------|
| -100 | Evening Doji Star pattern detected: bearish |
| 0 | No pattern |

## Parameters

| Parameter | Type | Default | Accepted values | Description |
| --- | --- | --- | --- | --- |
| `optInPenetration` | real | 0.3 | ≥ 0 | Fraction of the 1st real body the 3rd candle's close must penetrate; larger demands a deeper close into the first body |

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
TA_CDLEVENINGDOJISTAR( 0, n - 1, open, high, low, close, 0.3, &outBegIdx, &outNBElement, out );
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
let range = core.cdleveningdojistar(0, open.len() - 1, &open, &high, &low, &close, 0.3,
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
OutRange r = core.cdleveningdojistar(0, open.length - 1, open, high, low, close, 0.3, out);
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
OutRange r = core.Cdleveningdojistar(0, open.Length - 1, open, high, low, close, 0.3, outInteger);
Console.WriteLine($"{r.Count} values from bar {r.BegIdx}; last outInteger = {outInteger[r.Count - 1]}");
```

@tab TypeScript

```ts
import { cdleveningdojistar } from 'ta-lib';

const open = new Float64Array(252); // your data, oldest first
const high = new Float64Array(252); // your data, oldest first
const low = new Float64Array(252); // your data, oldest first
const close = new Float64Array(252); // your data, oldest first

const { begIdx, values } = cdleveningdojistar({ open, high, low, close }, {
  penetration: 0.3,
});
console.log(values.length + ' values from bar ' + begIdx + '; last values = ' + values[values.length - 1]);
```

:::

## Implementation

Added in 0.4.0.

TA-Lib Definition: [`cdleveningdojistar.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/cdleveningdojistar/cdleveningdojistar.c) · [`cdleveningdojistar.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/cdleveningdojistar/cdleveningdojistar.yaml)

| Native | File |
|--------|------|
| C | [`ta_CDLEVENINGDOJISTAR.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_CDLEVENINGDOJISTAR.c) |
| Rust | [`cdleveningdojistar.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/library/src/ta_func/cdleveningdojistar.rs) |
| Java | [`Core_CDLEVENINGDOJISTAR.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/fragments/Core_CDLEVENINGDOJISTAR.java) |
| C# | [`Core_CDLEVENINGDOJISTAR.cs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/csharp/library/src/Core_CDLEVENINGDOJISTAR.cs) |
| TypeScript | [`cdleveningdojistar.ts`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/ts/library/src/func/cdleveningdojistar.ts) |

Python: `talib.CDLEVENINGDOJISTAR`, see [ta-lib-python](https://ta-lib.github.io/ta-lib-python/func_groups/pattern_recognition.html).

TA-Lib is also available for R and more using a [wrapper](/install/#wrappers).

## Aliases

Evening Doji Star

## See Also

[CDLEVENINGSTAR](/functions/cdleveningstar.md) · [CDLMORNINGDOJISTAR](/functions/cdlmorningdojistar.md) · [CDLDOJISTAR](/functions/cdldojistar.md) · [CDLABANDONEDBABY](/functions/cdlabandonedbaby.md)
