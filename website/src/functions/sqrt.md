---
title: "Vector Square Root (SQRT)"
description: "Element-wise square root of the input series."
---

## Summary

Element-wise square root of the input series.

## Chart

<ta-chart src="/functions/sqrt.svg" ratio="720:416" label="SQRT over a ramp from 0 to 100" alt="Drawn on the price pane over a ramp from 0 to 100 as a line; below it, a separate indicator pane with a zero line showing outReal as a solid line."></ta-chart>

::: details Chart metadata <span class="flag-tip" tabindex="0" role="note" aria-label="The chart is drawn from the function's metadata alone: the flags the abstract API reports for it and for each output." data-tip="The chart is drawn from the function's metadata alone: the flags the abstract API reports for it and for each output.">i</span>

Drawn on the price pane over a ramp from 0 to 100 as a line; below it, a separate indicator pane with a zero line showing outReal as a solid line.

Metadata flags used: `inReal: series ramp [0, 100]` · `outReal: line`

:::

[Reference data (CSV)](/functions/sqrt.csv) · [Chart (SVG)](/functions/sqrt.svg)

## Formula

outReal[i] = sqrt(inReal[i])

## Notes

- A negative input has no real square root, so those elements come out NaN. A zero input gives 0.

## Inputs

- `inReal` — Input values

## Outputs

- `outReal` — Square root of each input value

## Properties

**Numerical Stability:** [Start-Independent](/functions/stability.md#start-independent)

<div class="flag-table">

|  |
| :-- |
| <span class="flag-box">☐</span> <span style="opacity:0.5">Overlap Input</span> |
| <span class="flag-box">✅</span> **Independent Y-Axis** <span class="flag-tip" tabindex="0" role="note" aria-label="Output is on its own scale, drawn in a separate pane below the price chart." data-tip="Output is on its own scale, drawn in a separate pane below the price chart.">i</span> |
| <span class="flag-box">☐</span> <span style="opacity:0.5">Candlestick</span> |
| <span class="flag-box">✅</span> **Can Output NaN or ±Inf** <span class="flag-tip" tabindex="0" role="note" aria-label="Some inputs have no finite result, so a successful call can return NaN or ±Inf — a gap with nothing to plot. See Notes for when." data-tip="Some inputs have no finite result, so a successful call can return NaN or ±Inf — a gap with nothing to plot. See Notes for when.">i</span> |
| <span class="flag-box">☐</span> <span style="opacity:0.5">Identity at Period 1</span> |
| <span class="flag-box">☐</span> <span style="opacity:0.5">Display Shift</span> |
| <span class="flag-box">☐</span> <span style="opacity:0.5">Uses Transcendental</span> |
| <span class="flag-box">✅</span> **Input Must Be Positive** <span class="flag-tip" tabindex="0" role="note" aria-label="Assumes the input is positive, as a price is. A series that reaches or crosses zero, such as an oscillator or a spread, returns success and an output that may have no meaning. See Notes for what a zero or a negative value does." data-tip="Assumes the input is positive, as a price is. A series that reaches or crosses zero, such as an oscillator or a spread, returns success and an output that may have no meaning. See Notes for what a zero or a negative value does.">i</span> |

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
TA_SQRT( 0, n - 1, series, &outBegIdx, &outNBElement, out );
printf( "%d values from bar %d; last out = %f\n", outNBElement, outBegIdx, out[outNBElement - 1] );
```

@tab Rust

```rust
use ta_lib::Core;

let series = vec![0.0; 252]; // your data, oldest first
let mut out = vec![0.0; 252];

let core = Core::new();
let range = core.sqrt(0, series.len() - 1, &series, &mut out)?;
println!("{} values from bar {}; last out = {}", range.count, range.beg_idx, out[range.count - 1]);
```

@tab Java

```java
import io.github.talib.Core;
import io.github.talib.OutRange;

double[] series = new double[252]; // your data, oldest first
double[] out = new double[252];

Core core = Core.DEFAULT;
OutRange r = core.sqrt(0, series.length - 1, series, out);
System.out.println(r.count() + " values from bar " + r.begIdx() + "; last out = " + out[r.count() - 1]);
```

@tab C\#

```csharp
using System;
using TALib;

var series = new double[252]; // your data, oldest first
var outReal = new double[252];

var core = Core.Default;
OutRange r = core.Sqrt(0, series.Length - 1, series, outReal);
Console.WriteLine($"{r.Count} values from bar {r.BegIdx}; last outReal = {outReal[r.Count - 1]}");
```

@tab TypeScript

```ts
import { sqrt } from 'ta-lib';

const series = new Float64Array(252); // your data, oldest first

const { begIdx, values } = sqrt(series);
console.log(values.length + ' values from bar ' + begIdx + '; last values = ' + values[values.length - 1]);
```

:::

## Implementation

Added in 0.4.0.

TA-Lib Definition: [`sqrt.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/sqrt/sqrt.c) · [`sqrt.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/sqrt/sqrt.yaml)

| Native | File |
|--------|------|
| C | [`ta_SQRT.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_SQRT.c) |
| Rust | [`sqrt.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/library/src/ta_func/sqrt.rs) |
| Java | [`Core_SQRT.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/fragments/Core_SQRT.java) |
| C# | [`Core_SQRT.cs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/csharp/library/src/Core_SQRT.cs) |
| TypeScript | [`sqrt.ts`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/ts/library/src/func/sqrt.ts) |

Python: `talib.SQRT`, see [ta-lib-python](https://ta-lib.github.io/ta-lib-python/func_groups/math_transform.html).

TA-Lib is also available for R and more using a [wrapper](/install/#wrappers).

## Aliases

Square Root

## References

- Wikipedia, *Square root*: [en.wikipedia.org/wiki/Square_root](https://en.wikipedia.org/wiki/Square_root)
