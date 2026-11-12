---
url: 'https://ta-lib.org/functions/cdldarkcloudcover.md'
description: >-
  A two-candle bearish reversal pattern: a long white candle followed by a black
  candle that opens above the prior high and closes deep into the prior…
---
# Dark Cloud Cover (CDLDARKCLOUDCOVER)

## Summary

A two-candle bearish reversal pattern: a long white candle followed by a black candle that opens above the prior high and closes deep into the prior white body past a penetration threshold. Signals a potential top. A hit (-100) is a bearish reversal signal, most meaningful after an uptrend.

## Chart

<ta-chart src="/functions/cdldarkcloudcover.svg" ratio="720:274" label="CDLDARKCLOUDCOVER over IBM, daily, 1999" alt="Drawn on the price pane over candles, with outInteger as markers on the candles, an arrow down from the top of the pane to above the high for a bearish call."></ta-chart>

<div class="ta-chart-links">

[Reference data (CSV)](/functions/cdldarkcloudcover.csv) · [Chart (SVG)](/functions/cdldarkcloudcover.svg)

</div>

::: details Chart metadata <span class="flag-tip" tabindex="0" role="note" aria-label="The chart is drawn automatically from metadata TA-Lib provides. Your app can get the same through the Abstract API." data-tip="The chart is drawn automatically from metadata TA-Lib provides. Your app can get the same through the Abstract API.">i</span>

**Outputs**

* `outInteger`: `pattern_bull_bear` · `zero` · `negative`

:::

## Notes

* Does not verify the preceding uptrend the bearish reversal classically assumes.

## Inputs

* `inOpen` — Open price of each bar
* `inHigh` — High price of each bar
* `inLow` — Low price of each bar
* `inClose` — Close price of each bar

## Outputs

* `outInteger` — -100 when the pattern is detected (always bearish), 0 otherwise; never emits +100

## Output Values

What candlestick values mean: [Candlestick Outputs](/api/candlestick-outputs/). The function's flags declare which values it can return: [rW7](/spec/inputs-outputs/#rw7).

| Value | Meaning |
|-------|---------|
| -100 | Dark Cloud Cover pattern detected: bearish |
| 0 | No pattern |

## Parameters

| Parameter | Type | Default | Accepted values | Description |
| --- | --- | --- | --- | --- |
| `optInPenetration` | real | 0.5 | ≥ 0 | Fraction of candle 1's real body that candle 2's close must penetrate below close\[i-1]; larger values require deeper penetration |

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
TA_CDLDARKCLOUDCOVER( 0, n - 1, open, high, low, close, 0.5, &outBegIdx, &outNBElement, out );
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
let range = core.cdldarkcloudcover(0, open.len() - 1, &open, &high, &low, &close, 0.5,
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
OutRange r = core.cdldarkcloudcover(0, open.length - 1, open, high, low, close, 0.5, out);
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
OutRange r = core.Cdldarkcloudcover(0, open.Length - 1, open, high, low, close, 0.5, outInteger);
Console.WriteLine($"{r.Count} values from bar {r.BegIdx}; last outInteger = {outInteger[r.Count - 1]}");
```

@tab TypeScript

```ts
import { cdldarkcloudcover } from 'ta-lib';

const open = new Float64Array(252); // your data, oldest first
const high = new Float64Array(252); // your data, oldest first
const low = new Float64Array(252); // your data, oldest first
const close = new Float64Array(252); // your data, oldest first

const { begIdx, values } = cdldarkcloudcover({ open, high, low, close }, {
  penetration: 0.5,
});
console.log(values.length + ' values from bar ' + begIdx + '; last values = ' + values[values.length - 1]);
```

:::

## Implementation

Added in 0.4.0.

TA-Lib Definition: [`cdldarkcloudcover.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/cdldarkcloudcover/cdldarkcloudcover.c) · [`cdldarkcloudcover.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/cdldarkcloudcover/cdldarkcloudcover.yaml)

| Native | File |
|--------|------|
| C | [`ta_CDLDARKCLOUDCOVER.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_CDLDARKCLOUDCOVER.c) |
| Rust | [`cdldarkcloudcover.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/library/src/ta_func/cdldarkcloudcover.rs) |
| Java | [`Core_CDLDARKCLOUDCOVER.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/fragments/Core_CDLDARKCLOUDCOVER.java) |
| C# | [`Core_CDLDARKCLOUDCOVER.cs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/csharp/library/src/Core_CDLDARKCLOUDCOVER.cs) |
| TypeScript | [`cdldarkcloudcover.ts`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/ts/library/src/func/cdldarkcloudcover.ts) |

Python: `talib.CDLDARKCLOUDCOVER`, see [ta-lib-python](https://ta-lib.github.io/ta-lib-python/func_groups/pattern_recognition.html).

TA-Lib is also available for R and more using a [wrapper](/install/#wrappers).

## Aliases

Dark Cloud Cover

## See Also

[CDLPIERCING](/functions/cdlpiercing.md) · [CDLENGULFING](/functions/cdlengulfing.md) · [CDLONNECK](/functions/cdlonneck.md)
