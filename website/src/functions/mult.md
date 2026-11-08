---
title: "Vector Arithmetic Mult (MULT)"
description: "Element-wise multiplication of two input series."
---

## Summary

Element-wise multiplication of two input series.

## Chart

<ta-chart src="/functions/mult.svg" ratio="720:416" label="MULT over IBM, daily, 1999" alt="Drawn on the price pane over the close as a line; below it, a separate indicator pane showing outReal as a solid line."></ta-chart>

Drawn on the price pane over the close as a line; below it, a separate indicator pane showing outReal as a solid line. [Reference data (CSV)](/functions/mult.csv) · [Chart (SVG)](/functions/mult.svg)

## Formula

outReal[i] = inReal0[i] * inReal1[i]

## Inputs

- `inReal0` — First operand series
- `inReal1` — Second operand series

## Outputs

- `outReal` — Product of the two inputs at each index

## Properties

**Numerical Stability:** [Start-Independent](/functions/stability.md#start-independent)

<div class="flag-table">

|  |
| :-- |
| <span class="flag-box">☐</span> <span style="opacity:0.5">Overlap Input</span> |
| <span class="flag-box">✅</span> **Independent Y-Axis** <span class="flag-tip" tabindex="0" role="note" aria-label="Output is on its own scale, drawn in a separate pane below the price chart." data-tip="Output is on its own scale, drawn in a separate pane below the price chart.">i</span> |
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
double series0[252]; /* your data, oldest first */
double series1[252]; /* your data, oldest first */
double out[252];
int outBegIdx, outNBElement;

TA_Initialize();
TA_MULT( 0, n - 1, series0, series1, &outBegIdx, &outNBElement, out );
printf( "%d values from bar %d; last out = %f\n", outNBElement, outBegIdx, out[outNBElement - 1] );
```

@tab Rust

```rust
use ta_lib::Core;

let series0 = vec![0.0; 252]; // your data, oldest first
let series1 = vec![0.0; 252]; // your data, oldest first
let mut out = vec![0.0; 252];

let core = Core::new();
let range = core.mult(0, series0.len() - 1, &series0, &series1, &mut out)?;
println!("{} values from bar {}; last out = {}", range.count, range.beg_idx, out[range.count - 1]);
```

@tab Java

```java
import io.github.talib.Core;
import io.github.talib.OutRange;

double[] series0 = new double[252]; // your data, oldest first
double[] series1 = new double[252]; // your data, oldest first
double[] out = new double[252];

Core core = Core.DEFAULT;
OutRange r = core.mult(0, series0.length - 1, series0, series1, out);
System.out.println(r.count() + " values from bar " + r.begIdx() + "; last out = " + out[r.count() - 1]);
```

@tab C\#

```csharp
using System;
using TALib;

var series0 = new double[252]; // your data, oldest first
var series1 = new double[252]; // your data, oldest first
var outReal = new double[252];

var core = Core.Default;
OutRange r = core.Mult(0, series0.Length - 1, series0, series1, outReal);
Console.WriteLine($"{r.Count} values from bar {r.BegIdx}; last outReal = {outReal[r.Count - 1]}");
```

@tab TypeScript

```ts
import { mult } from 'ta-lib';

const series0 = new Float64Array(252); // your data, oldest first
const series1 = new Float64Array(252); // your data, oldest first

const { begIdx, values } = mult(series0, series1);
console.log(values.length + ' values from bar ' + begIdx + '; last values = ' + values[values.length - 1]);
```

:::

## Implementation

Added in 0.4.0.

TA-Lib Definition: [`mult.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/mult/mult.c) · [`mult.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/mult/mult.yaml)

| Native | File |
|--------|------|
| C | [`ta_MULT.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_MULT.c) |
| Rust | [`mult.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/library/src/ta_func/mult.rs) |
| Java | [`Core_MULT.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/fragments/Core_MULT.java) |
| C# | [`Core_MULT.cs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/csharp/library/src/Core_MULT.cs) |
| TypeScript | [`mult.ts`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/ts/library/src/func/mult.ts) |

Python: `talib.MULT`, see [ta-lib-python](https://ta-lib.github.io/ta-lib-python/func_groups/math_operators.html).

TA-Lib is also available for R and more using a [wrapper](/install/#wrappers).

## Aliases

Vector Multiply, Vector Arithmetic Mult, Element-wise Product

## See Also

[ADD](/functions/add.md) · [SUB](/functions/sub.md) · [DIV](/functions/div.md)
