---
title: "Weighted Close Price (WCLPRICE)"
description: "Weighted Close Price: a per-bar price average giving the close double weight relative to high and low."
---

## Summary

Weighted Close Price: a per-bar price average giving the close double weight relative to high and low.

## Chart

<ta-chart src="/functions/wclprice.svg" ratio="720:274" label="WCLPRICE over IBM, daily, 1999" alt="Drawn on the price pane over candles, with outReal as a solid line."></ta-chart>

::: details Chart metadata <span class="flag-tip" tabindex="0" role="note" aria-label="The chart is drawn from the function's metadata: the flags and attributes the abstract API reports for it and for each output, and the input series its reference run declares." data-tip="The chart is drawn from the function's metadata: the flags and attributes the abstract API reports for it and for each output, and the input series its reference run declares.">i</span>

Drawn on the price pane over candles, with outReal as a solid line.

Metadata flags used: `overlap` · `inPriceHLC: high, low, close` · `outReal: line`

:::

[Reference data (CSV)](/functions/wclprice.csv) · [Chart (SVG)](/functions/wclprice.svg)

## Formula

$\text{WCLPRICE} = \dfrac{\text{High} + \text{Low} + 2\cdot\text{Close}}{4}$

## Inputs

- `inHigh` — High price of each bar
- `inLow` — Low price of each bar
- `inClose` — Close price of each bar

## Outputs

- `outReal` — Weighted close price per bar

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
TA_WCLPRICE( 0, n - 1, high, low, close, &outBegIdx, &outNBElement, out );
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
let range = core.wclprice(0, high.len() - 1, &high, &low, &close, &mut out)?;
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
OutRange r = core.wclprice(0, high.length - 1, high, low, close, out);
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
OutRange r = core.Wclprice(0, high.Length - 1, high, low, close, outReal);
Console.WriteLine($"{r.Count} values from bar {r.BegIdx}; last outReal = {outReal[r.Count - 1]}");
```

@tab TypeScript

```ts
import { wclprice } from 'ta-lib';

const high = new Float64Array(252); // your data, oldest first
const low = new Float64Array(252); // your data, oldest first
const close = new Float64Array(252); // your data, oldest first

const { begIdx, values } = wclprice({ high, low, close });
console.log(values.length + ' values from bar ' + begIdx + '; last values = ' + values[values.length - 1]);
```

:::

## Implementation

Added in 0.4.0.

TA-Lib Definition: [`wclprice.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/wclprice/wclprice.c) · [`wclprice.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/wclprice/wclprice.yaml)

| Native | File |
|--------|------|
| C | [`ta_WCLPRICE.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_WCLPRICE.c) |
| Rust | [`wclprice.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/library/src/ta_func/wclprice.rs) |
| Java | [`Core_WCLPRICE.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/fragments/Core_WCLPRICE.java) |
| C# | [`Core_WCLPRICE.cs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/csharp/library/src/Core_WCLPRICE.cs) |
| TypeScript | [`wclprice.ts`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/ts/library/src/func/wclprice.ts) |

Python: `talib.WCLPRICE`, see [ta-lib-python](https://ta-lib.github.io/ta-lib-python/func_groups/price_transform.html).

TA-Lib is also available for R and more using a [wrapper](/install/#wrappers).

## Aliases

Weighted Close Price, Weighted Close

## See Also

[TYPPRICE](/functions/typprice.md) · [MEDPRICE](/functions/medprice.md) · [AVGPRICE](/functions/avgprice.md)
