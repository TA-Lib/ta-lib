---
title: "Typical Price (TYPPRICE)"
description: "Typical Price: the average of the high, low, and close of each bar. A single representative price per period."
---

## Summary

Typical Price: the average of the high, low, and close of each bar. A single representative price per period.

## Chart

<ta-chart src="/functions/typprice.svg" ratio="720:274" label="TYPPRICE over IBM, daily, 1999" alt="Drawn on the price pane over candles, with outReal as a solid line."></ta-chart>

Drawn on the price pane over candles, with outReal as a solid line. [Reference data (CSV)](/functions/typprice.csv) · [Chart (SVG)](/functions/typprice.svg)

## Formula

out[i] = (High[i] + Low[i] + Close[i]) / 3

## Inputs

- `inHigh` — High price of each bar
- `inLow` — Low price of each bar
- `inClose` — Close price of each bar

## Outputs

- `outReal` — typical price per bar

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
double close[252]; /* your data, oldest first */
double out[252];
int outBegIdx, outNBElement;

TA_Initialize();
TA_TYPPRICE( 0, n - 1, high, low, close, &outBegIdx, &outNBElement, out );
printf( "%d values from bar %d; last out = %f\n", outNBElement, outBegIdx, out[outNBElement - 1] );
```

@tab Rust

```rust
use ta_lib::Core;

let high = vec![0.0; 252]; // your data, oldest first
let low = vec![0.0; 252]; // your data, oldest first
let close = vec![0.0; 252]; // your data, oldest first
let mut out = vec![0.0; 252];

let core = Core::new();
let range = core.typprice(0, high.len() - 1, &high, &low, &close, &mut out)?;
println!("{} values from bar {}; last out = {}", range.count, range.beg_idx, out[range.count - 1]);
```

@tab Java

```java
import io.github.talib.Core;
import io.github.talib.OutRange;

double[] high = new double[252]; // your data, oldest first
double[] low = new double[252]; // your data, oldest first
double[] close = new double[252]; // your data, oldest first
double[] out = new double[252];

Core core = Core.DEFAULT;
OutRange r = core.typprice(0, high.length - 1, high, low, close, out);
System.out.println(r.count() + " values from bar " + r.begIdx() + "; last out = " + out[r.count() - 1]);
```

@tab C\#

```csharp
using System;
using TALib;

var high = new double[252]; // your data, oldest first
var low = new double[252]; // your data, oldest first
var close = new double[252]; // your data, oldest first
var outReal = new double[252];

var core = Core.Default;
OutRange r = core.Typprice(0, high.Length - 1, high, low, close, outReal);
Console.WriteLine($"{r.Count} values from bar {r.BegIdx}; last outReal = {outReal[r.Count - 1]}");
```

@tab TypeScript

```ts
import { typprice } from 'ta-lib';

const high = new Float64Array(252); // your data, oldest first
const low = new Float64Array(252); // your data, oldest first
const close = new Float64Array(252); // your data, oldest first

const { begIdx, values } = typprice({ high, low, close });
console.log(values.length + ' values from bar ' + begIdx + '; last values = ' + values[values.length - 1]);
```

:::

## Implementation

Added in 0.4.0.

TA-Lib Definition: [`typprice.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/typprice/typprice.c) · [`typprice.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/typprice/typprice.yaml)

| Native | File |
|--------|------|
| C | [`ta_TYPPRICE.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_TYPPRICE.c) |
| Rust | [`typprice.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/library/src/ta_func/typprice.rs) |
| Java | [`Core_TYPPRICE.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/fragments/Core_TYPPRICE.java) |
| C# | [`Core_TYPPRICE.cs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/csharp/library/src/Core_TYPPRICE.cs) |
| TypeScript | [`typprice.ts`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/ts/library/src/func/typprice.ts) |

Python: `talib.TYPPRICE`, see [ta-lib-python](https://ta-lib.github.io/ta-lib-python/func_groups/price_transform.html).

TA-Lib is also available for R and more using a [wrapper](/install/#wrappers).

## Aliases

Typical Price

## See Also

[MEDPRICE](/functions/medprice.md) · [WCLPRICE](/functions/wclprice.md) · [AVGPRICE](/functions/avgprice.md)
