---
title: "Vector Trigonometric Cosh (COSH)"
description: "Element-wise hyperbolic cosine of the input series."
---

## Summary

Element-wise hyperbolic cosine of the input series.

## Chart

<ta-chart src="/functions/cosh.svg" ratio="720:416" label="COSH over a ramp from -5 to 5" alt="Drawn on the price pane over a ramp from -5 to 5 as a line; below it, a separate indicator pane with a zero line showing outReal as a solid line."></ta-chart>

::: details Chart metadata <span class="flag-tip" tabindex="0" role="note" aria-label="The chart is drawn from the function's metadata alone: the flags the abstract API reports for it and for each output." data-tip="The chart is drawn from the function's metadata alone: the flags the abstract API reports for it and for each output.">i</span>

Drawn on the price pane over a ramp from -5 to 5 as a line; below it, a separate indicator pane with a zero line showing outReal as a solid line.

Metadata flags used: `inReal: series ramp [-5, 5]` · `outReal: line`

:::

[Reference data (CSV)](/functions/cosh.csv) · [Chart (SVG)](/functions/cosh.svg)

## Formula

outReal[i] = cosh(inReal[i]) = (e^{inReal[i]} + e^{-inReal[i]}) / 2

## Notes

- An input beyond about 710.48 in magnitude has a result too large for a double, so those elements come out positive infinity.

## Inputs

- `inReal` — Input values to transform

## Outputs

- `outReal` — Hyperbolic cosine of each input

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
| <span class="flag-box">✅</span> **Uses Transcendental** <span class="flag-tip" tabindex="0" role="note" aria-label="Calls a transcendental math function (exp, log, log10, trigonometric, inverse trigonometric or hyperbolic), so a value may differ slightly between languages, between a stream and a batch call, and between machines. TA-Lib targets a difference below 1e-9." data-tip="Calls a transcendental math function (exp, log, log10, trigonometric, inverse trigonometric or hyperbolic), so a value may differ slightly between languages, between a stream and a batch call, and between machines. TA-Lib targets a difference below 1e-9.">i</span> |
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
TA_COSH( 0, n - 1, series, &outBegIdx, &outNBElement, out );
printf( "%d values from bar %d; last out = %f\n", outNBElement, outBegIdx, out[outNBElement - 1] );
```

@tab Rust

```rust
use ta_lib::Core;

let series = vec![0.0; 252]; // your data, oldest first
let mut out = vec![0.0; 252];

let core = Core::new();
let range = core.cosh(0, series.len() - 1, &series, &mut out)?;
println!("{} values from bar {}; last out = {}", range.count, range.beg_idx, out[range.count - 1]);
```

@tab Java

```java
import io.github.talib.Core;
import io.github.talib.OutRange;

double[] series = new double[252]; // your data, oldest first
double[] out = new double[252];

Core core = Core.DEFAULT;
OutRange r = core.cosh(0, series.length - 1, series, out);
System.out.println(r.count() + " values from bar " + r.begIdx() + "; last out = " + out[r.count() - 1]);
```

@tab C\#

```csharp
using System;
using TALib;

var series = new double[252]; // your data, oldest first
var outReal = new double[252];

var core = Core.Default;
OutRange r = core.Cosh(0, series.Length - 1, series, outReal);
Console.WriteLine($"{r.Count} values from bar {r.BegIdx}; last outReal = {outReal[r.Count - 1]}");
```

@tab TypeScript

```ts
import { cosh } from 'ta-lib';

const series = new Float64Array(252); // your data, oldest first

const { begIdx, values } = cosh(series);
console.log(values.length + ' values from bar ' + begIdx + '; last values = ' + values[values.length - 1]);
```

:::

## Implementation

Added in 0.4.0.

TA-Lib Definition: [`cosh.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/cosh/cosh.c) · [`cosh.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/cosh/cosh.yaml)

| Native | File |
|--------|------|
| C | [`ta_COSH.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_COSH.c) |
| Rust | [`cosh.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/library/src/ta_func/cosh.rs) |
| Java | [`Core_COSH.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/fragments/Core_COSH.java) |
| C# | [`Core_COSH.cs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/csharp/library/src/Core_COSH.cs) |
| TypeScript | [`cosh.ts`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/ts/library/src/func/cosh.ts) |

Python: `talib.COSH`, see [ta-lib-python](https://ta-lib.github.io/ta-lib-python/func_groups/math_transform.html).

TA-Lib is also available for R and more using a [wrapper](/install/#wrappers).

## Aliases

Hyperbolic Cosine

## See Also

[SINH](/functions/sinh.md) · [TANH](/functions/tanh.md) · [COS](/functions/cos.md)

## References

- Wikipedia, *Hyperbolic functions*: [en.wikipedia.org/wiki/Hyperbolic_functions](https://en.wikipedia.org/wiki/Hyperbolic_functions)
