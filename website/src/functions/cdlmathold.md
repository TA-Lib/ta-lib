---
title: "Mat Hold (CDLMATHOLD)"
description: "A five-candle bullish continuation pattern: a long white candle, an upside real-body-gapped small black candle, two more small falling candles that hold…"
---

## Summary

A five-candle bullish continuation pattern: a long white candle, an upside real-body-gapped small black candle, two more small falling candles that hold within the first body, and a final white candle closing above the reaction days' highs. Signals continuation of the prior uptrend. Hit = bullish continuation of the existing uptrend.

## Chart

<ta-chart src="/functions/cdlmathold.svg" ratio="720:274" label="CDLMATHOLD over WBTC/USDC (Uniswap v3), daily, 2026" alt="Drawn on the price pane over candles, with outInteger as markers on the candles, an arrow up from the bottom of the pane to below the low for a bullish call."></ta-chart>

<div class="ta-chart-links">

[Reference data (CSV)](/functions/cdlmathold.csv) · [Chart (SVG)](/functions/cdlmathold.svg)

</div>

::: details Chart metadata <span class="flag-tip" tabindex="0" role="note" aria-label="The chart is drawn automatically from metadata TA-Lib provides. Your app can get the same through the Abstract API." data-tip="The chart is drawn automatically from metadata TA-Lib provides. Your app can get the same through the Abstract API.">i</span>

**Outputs**

- `outInteger`: `line` · `pattern_bull_bear` · `zero` · `positive`

:::

## Notes

- The colors of the third and fourth (reaction) candles are not checked, although they are classically black.
- The continuation reading assumes a prior uptrend, which is not verified.
- Bulkowski's own dataset contains only 52 Mat Hold occurrences out of 4.7 million candle lines; he explicitly warns the 78% continuation rate he measured "will likely be wrong or at least subject to large change as additional samples become available." ([thepatternsite.com](https://thepatternsite.com/MatHold.html))
- Rare: a few years of one symbol's daily bars may show none. Bulkowski ranks it 93rd of 103 patterns by how often it occurs ([Mat Hold](https://thepatternsite.com/MatHold.html)), and it fires once in [about 16,000 daily crypto bars](/api/candlestick-outputs/#how-often-patterns-fire).

## Inputs

- `inOpen` — Open price of each bar
- `inHigh` — High price of each bar
- `inLow` — Low price of each bar
- `inClose` — Close price of each bar

## Outputs

- `outInteger` — +100 when the bullish Mat Hold is detected, 0 otherwise. Never emits -100

## Output Values

What candlestick values mean: [Candlestick Outputs](/api/candlestick-outputs/). The function's flags declare which values it can return: [rW7](/spec/inputs-outputs/#rw7).

| Value | Meaning |
|-------|---------|
| 0 | No pattern |
| 100 | Mat Hold detected — bullish continuation signal |

## Parameters

| Parameter | Type | Default | Accepted values | Description |
| --- | --- | --- | --- | --- |
| `optInPenetration` | real | 0.5 | ≥ 0 | Max fraction of the 1st white body the reaction days (3rd, 4th) may penetrate |

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
TA_CDLMATHOLD( 0, n - 1, open, high, low, close, 0.5, &outBegIdx, &outNBElement, out );
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
let range = core.cdlmathold(0, open.len() - 1, &open, &high, &low, &close, 0.5, &mut out)?;
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
OutRange r = core.cdlmathold(0, open.length - 1, open, high, low, close, 0.5, out);
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
OutRange r = core.Cdlmathold(0, open.Length - 1, open, high, low, close, 0.5, outInteger);
Console.WriteLine($"{r.Count} values from bar {r.BegIdx}; last outInteger = {outInteger[r.Count - 1]}");
```

@tab TypeScript

```ts
import { cdlmathold } from 'ta-lib';

const open = new Float64Array(252); // your data, oldest first
const high = new Float64Array(252); // your data, oldest first
const low = new Float64Array(252); // your data, oldest first
const close = new Float64Array(252); // your data, oldest first

const { begIdx, values } = cdlmathold({ open, high, low, close }, {
  penetration: 0.5,
});
console.log(values.length + ' values from bar ' + begIdx + '; last values = ' + values[values.length - 1]);
```

:::

## Implementation

Added in 0.4.0.

TA-Lib Definition: [`cdlmathold.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/cdlmathold/cdlmathold.c) · [`cdlmathold.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/cdlmathold/cdlmathold.yaml)

| Native | File |
|--------|------|
| C | [`ta_CDLMATHOLD.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_CDLMATHOLD.c) |
| Rust | [`cdlmathold.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/library/src/ta_func/cdlmathold.rs) |
| Java | [`Core_CDLMATHOLD.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/fragments/Core_CDLMATHOLD.java) |
| C# | [`Core_CDLMATHOLD.cs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/csharp/library/src/Core_CDLMATHOLD.cs) |
| TypeScript | [`cdlmathold.ts`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/ts/library/src/func/cdlmathold.ts) |

Python: `talib.CDLMATHOLD`, see [ta-lib-python](https://ta-lib.github.io/ta-lib-python/func_groups/pattern_recognition.html).

TA-Lib is also available for R and more using a [wrapper](/install/#wrappers).

## Aliases

Mat Hold

## See Also

[CDLRISEFALL3METHODS](/functions/cdlrisefall3methods.md) · [CDLXSIDEGAP3METHODS](/functions/cdlxsidegap3methods.md)
