---
title: "Vector Ceil (CEIL)"
description: "Element-wise ceiling (round up to the nearest integer) of the input series."
---

## Summary

Element-wise ceiling (round up to the nearest integer) of the input series.

## Chart

<ta-chart src="/functions/ceil.svg" ratio="720:416" label="CEIL over a ramp from -3 to 3" alt="Drawn on the price pane over a ramp from -3 to 3 as a line; below it, a separate indicator pane with a zero line showing outReal as a solid line."></ta-chart>

::: details Chart metadata <span class="flag-tip" tabindex="0" role="note" aria-label="The chart is drawn from the function's metadata: the flags and attributes the abstract API reports for it and for each output, and the input series its reference run declares." data-tip="The chart is drawn from the function's metadata: the flags and attributes the abstract API reports for it and for each output, and the input series its reference run declares.">i</span>

Drawn on the price pane over a ramp from -3 to 3 as a line; below it, a separate indicator pane with a zero line showing outReal as a solid line.

Metadata flags used: `inReal: series ramp [-3, 3]` · `outReal: line`

:::

[Reference data (CSV)](/functions/ceil.csv) · [Chart (SVG)](/functions/ceil.svg)

## Formula

outReal[i] = ceil(inReal[i])

## Inputs

- `inReal` — Input values

## Outputs

- `outReal` — Each input rounded up to nearest integer

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
double series[252]; /* your data, oldest first */
double out[252];
int outBegIdx, outNBElement;

TA_Initialize();
TA_CEIL( 0, n - 1, series, &outBegIdx, &outNBElement, out );
printf( "%d values from bar %d; last out = %f\n", outNBElement, outBegIdx, out[outNBElement - 1] );
```

@tab Rust

```rust
use ta_lib::Core;

let series = vec![0.0; 252]; // your data, oldest first
let mut out = vec![0.0; 252];

let core = Core::new();
let range = core.ceil(0, series.len() - 1, &series, &mut out)?;
println!("{} values from bar {}; last out = {}", range.count, range.beg_idx, out[range.count - 1]);
```

@tab Java

```java
import io.github.talib.Core;
import io.github.talib.OutRange;

double[] series = new double[252]; // your data, oldest first
double[] out = new double[252];

Core core = Core.DEFAULT;
OutRange r = core.ceil(0, series.length - 1, series, out);
System.out.println(r.count() + " values from bar " + r.begIdx() + "; last out = " + out[r.count() - 1]);
```

@tab C\#

```csharp
using System;
using TALib;

var series = new double[252]; // your data, oldest first
var outReal = new double[252];

var core = Core.Default;
OutRange r = core.Ceil(0, series.Length - 1, series, outReal);
Console.WriteLine($"{r.Count} values from bar {r.BegIdx}; last outReal = {outReal[r.Count - 1]}");
```

@tab TypeScript

```ts
import { ceil } from 'ta-lib';

const series = new Float64Array(252); // your data, oldest first

const { begIdx, values } = ceil(series);
console.log(values.length + ' values from bar ' + begIdx + '; last values = ' + values[values.length - 1]);
```

:::

## Implementation

Added in 0.4.0.

TA-Lib Definition: [`ceil.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/ceil/ceil.c) · [`ceil.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/ceil/ceil.yaml)

| Native | File |
|--------|------|
| C | [`ta_CEIL.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_CEIL.c) |
| Rust | [`ceil.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/library/src/ta_func/ceil.rs) |
| Java | [`Core_CEIL.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/fragments/Core_CEIL.java) |
| C# | [`Core_CEIL.cs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/csharp/library/src/Core_CEIL.cs) |
| TypeScript | [`ceil.ts`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/ts/library/src/func/ceil.ts) |

Python: `talib.CEIL`, see [ta-lib-python](https://ta-lib.github.io/ta-lib-python/func_groups/math_transform.html).

TA-Lib is also available for R and more using a [wrapper](/install/#wrappers).

## Aliases

Vector Ceil, Ceiling

## See Also

[FLOOR](/functions/floor.md)

## References

- Wikipedia, *Floor and ceiling functions*: [en.wikipedia.org/wiki/Floor_and_ceiling_functions](https://en.wikipedia.org/wiki/Floor_and_ceiling_functions)
