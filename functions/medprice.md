---
url: 'https://ta-lib.org/functions/medprice.md'
description: >-
  Median Price: the midpoint of each bar's high and low. A price-transform
  overlay.
---
# Median Price (MEDPRICE)

## Summary

Median Price: the midpoint of each bar's high and low. A price-transform overlay.

## Chart

<ta-chart src="/functions/medprice.svg" ratio="720:274" label="MEDPRICE over IBM, daily, 1999" alt="Drawn on the price pane over candles, with outReal as a solid line."></ta-chart>

::: details Chart metadata <span class="flag-tip" tabindex="0" role="note" aria-label="The chart is drawn from the function's metadata: the flags and attributes the abstract API reports for it and for each output, and the input series its reference run declares." data-tip="The chart is drawn from the function's metadata: the flags and attributes the abstract API reports for it and for each output, and the input series its reference run declares.">i</span>

Drawn on the price pane over candles, with outReal as a solid line.

Metadata flags used: `overlap` · `inPriceHL: high, low` · `outReal: line`

:::

[Reference data (CSV)](/functions/medprice.csv) · [Chart (SVG)](/functions/medprice.svg)

## Formula

$MEDPRICE_i = (High_i + Low_i) / 2$

## Inputs

* `inHigh` — High price of each bar
* `inLow` — Low price of each bar

## Outputs

* `outReal` — Midpoint of each bar's high and low

## Properties

**Numerical Stability:** [Start-Independent](/functions/stability.md#start-independent)

<div class="flag-table">

|  |
| :-- |
| <span class="flag-box">✅</span> **Overlap Input** <span class="flag-tip" tabindex="0" role="note" aria-label="Output is on the same scale as the input price, so it is drawn over the price chart." data-tip="Output is on the same scale as the input price, so it is drawn over the price chart.">i</span> |
| <span class="flag-box">☐</span> <span style="opacity:0.5">Independent Y-Axis</span> |
| <span class="flag-box">☐</span> <span style="opacity:0.5">Candlestick</span> |
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
double high[252]; /* your data, oldest first */
double low[252]; /* your data, oldest first */
double out[252];
int outBegIdx, outNBElement;

TA_Initialize();
TA_MEDPRICE( 0, n - 1, high, low, &outBegIdx, &outNBElement, out );
printf( "%d values from bar %d; last out = %f\n", outNBElement, outBegIdx, out[outNBElement - 1] );
```

@tab Rust

```rust
use ta_lib::Core;

let high = vec![0.0; 252]; // your data, oldest first
let low = vec![0.0; 252]; // your data, oldest first
let mut out = vec![0.0; 252];

let core = Core::new();
let range = core.medprice(0, high.len() - 1, &high, &low, &mut out)?;
println!("{} values from bar {}; last out = {}", range.count, range.beg_idx, out[range.count - 1]);
```

@tab Java

```java
import io.github.talib.Core;
import io.github.talib.OutRange;

double[] high = new double[252]; // your data, oldest first
double[] low = new double[252]; // your data, oldest first
double[] out = new double[252];

Core core = Core.DEFAULT;
OutRange r = core.medprice(0, high.length - 1, high, low, out);
System.out.println(r.count() + " values from bar " + r.begIdx() + "; last out = " + out[r.count() - 1]);
```

@tab C#

```csharp
using System;
using TALib;

var high = new double[252]; // your data, oldest first
var low = new double[252]; // your data, oldest first
var outReal = new double[252];

var core = Core.Default;
OutRange r = core.Medprice(0, high.Length - 1, high, low, outReal);
Console.WriteLine($"{r.Count} values from bar {r.BegIdx}; last outReal = {outReal[r.Count - 1]}");
```

@tab TypeScript

```ts
import { medprice } from 'ta-lib';

const high = new Float64Array(252); // your data, oldest first
const low = new Float64Array(252); // your data, oldest first

const { begIdx, values } = medprice({ high, low });
console.log(values.length + ' values from bar ' + begIdx + '; last values = ' + values[values.length - 1]);
```

:::

## Implementation

Added in 0.4.0.

TA-Lib Definition: [`medprice.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/medprice/medprice.c) · [`medprice.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/medprice/medprice.yaml)

| Native | File |
|--------|------|
| C | [`ta_MEDPRICE.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_MEDPRICE.c) |
| Rust | [`medprice.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/library/src/ta_func/medprice.rs) |
| Java | [`Core_MEDPRICE.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/fragments/Core_MEDPRICE.java) |
| C# | [`Core_MEDPRICE.cs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/csharp/library/src/Core_MEDPRICE.cs) |
| TypeScript | [`medprice.ts`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/ts/library/src/func/medprice.ts) |

Python: `talib.MEDPRICE`, see [ta-lib-python](https://ta-lib.github.io/ta-lib-python/func_groups/price_transform.html).

TA-Lib is also available for R and more using a [wrapper](/install/#wrappers).

## Aliases

Median Price

## See Also

[MIDPRICE](/functions/midprice.md) · [AVGPRICE](/functions/avgprice.md) · [TYPPRICE](/functions/typprice.md) · [WCLPRICE](/functions/wclprice.md)
