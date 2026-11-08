---
title: "Positive Volume Index (PVI)"
description: "Positive Volume Index: a running cumulative index that changes only on days when volume rises versus the prior day, compounding that day's percentage…"
---

## Summary

Positive Volume Index: a running cumulative index that changes only on days when
volume rises versus the prior day, compounding that day's percentage price change.
The premise is that active, high-volume days reflect the actions of the
less-informed "crowd", so PVI is read as a proxy for that cohort's positioning.

## Chart

<ta-chart src="/functions/pvi.svg" ratio="720:484" label="PVI over IBM, daily, 1999" alt="Drawn on the price pane over the close as a line; below it, a separate indicator pane showing outReal as a solid line, then a volume pane."></ta-chart>

::: details Chart metadata <span class="flag-tip" tabindex="0" role="note" aria-label="The chart is drawn from the function's metadata alone: the flags the abstract API reports for it and for each output." data-tip="The chart is drawn from the function's metadata alone: the flags the abstract API reports for it and for each output.">i</span>

Drawn on the price pane over the close as a line; below it, a separate indicator pane showing outReal as a solid line, then a volume pane.

Metadata flags used: `inPriceCV: close, volume` · `outReal: line`

:::

[Reference data (CSV)](/functions/pvi.csv) · [Chart (SVG)](/functions/pvi.svg)

## Formula

PVI[startIdx] = 1000

For each subsequent bar i:

    PVI[i] = PVI[i-1] + ( inVolume[i] > inVolume[i-1]
                          ? ((inClose[i] - inClose[i-1]) / inClose[i-1]) * PVI[i-1]
                          : 0 )

The index carries forward unchanged on bars whose volume did not rise (and on the
degenerate case of a zero previous close, which would otherwise divide by zero).

## Notes

- **The input is assumed positive.** The previous close is the divisor. Near zero the divisor is small and the output very large; no error is returned. A series that reaches or crosses zero, such as an oscillator or a spread, has no meaningful result here.
- The index compounds, so it has no upper bound. If a run of large rises ever pushes it past the largest representable number, the last representable value is carried forward instead of returning infinity. Real price series stay far away from that.

## Inputs

- `inClose` — Close price of each bar
- `inVolume` — Volume of each bar

## Outputs

- `outReal` — Cumulative positive volume index (seeded at 1000)

## Properties

**Numerical Stability:** [Path-Dependent](/functions/stability.md#path-dependent)

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
| <span class="flag-box">✅</span> **Input Must Be Positive** <span class="flag-tip" tabindex="0" role="note" aria-label="Assumes the input is positive, as a price is. A series that reaches or crosses zero, such as an oscillator or a spread, returns success and an output that may have no meaning. See Notes for what a zero or a negative value does." data-tip="Assumes the input is positive, as a price is. A series that reaches or crosses zero, such as an oscillator or a spread, returns success and an output that may have no meaning. See Notes for what a zero or a negative value does.">i</span> |

</div>

## Example

::: code-tabs#lang

@tab C

```c
#include <stdio.h>
#include <ta_libc.h>

int n = 252;
double close[252]; /* your data, oldest first */
double volume[252]; /* your data, oldest first */
double out[252];
int outBegIdx, outNBElement;

TA_Initialize();
TA_PVI( 0, n - 1, close, volume, &outBegIdx, &outNBElement, out );
printf( "%d values from bar %d; last out = %f\n", outNBElement, outBegIdx, out[outNBElement - 1] );
```

@tab Rust

```rust
use ta_lib::Core;

let close = vec![0.0; 252]; // your data, oldest first
let volume = vec![0.0; 252]; // your data, oldest first
let mut out = vec![0.0; 252];

let core = Core::new();
let range = core.pvi(0, close.len() - 1, &close, &volume, &mut out)?;
println!("{} values from bar {}; last out = {}", range.count, range.beg_idx, out[range.count - 1]);
```

@tab Java

```java
import io.github.talib.Core;
import io.github.talib.OutRange;

double[] close = new double[252]; // your data, oldest first
double[] volume = new double[252]; // your data, oldest first
double[] out = new double[252];

Core core = Core.DEFAULT;
OutRange r = core.pvi(0, close.length - 1, close, volume, out);
System.out.println(r.count() + " values from bar " + r.begIdx() + "; last out = " + out[r.count() - 1]);
```

@tab C\#

```csharp
using System;
using TALib;

var close = new double[252]; // your data, oldest first
var volume = new double[252]; // your data, oldest first
var outReal = new double[252];

var core = Core.Default;
OutRange r = core.Pvi(0, close.Length - 1, close, volume, outReal);
Console.WriteLine($"{r.Count} values from bar {r.BegIdx}; last outReal = {outReal[r.Count - 1]}");
```

@tab TypeScript

```ts
import { pvi } from 'ta-lib';

const close = new Float64Array(252); // your data, oldest first
const volume = new Float64Array(252); // your data, oldest first

const { begIdx, values } = pvi({ close, volume });
console.log(values.length + ' values from bar ' + begIdx + '; last values = ' + values[values.length - 1]);
```

:::

## Implementation

Added in 0.8.1 (ta-lib-python 0.8.1).

TA-Lib Definition: [`pvi.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/pvi/pvi.c) · [`pvi.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/pvi/pvi.yaml)

| Native | File |
|--------|------|
| C | [`ta_PVI.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_PVI.c) |
| Rust | [`pvi.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/library/src/ta_func/pvi.rs) |
| Java | [`Core_PVI.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/fragments/Core_PVI.java) |
| C# | [`Core_PVI.cs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/csharp/library/src/Core_PVI.cs) |
| TypeScript | [`pvi.ts`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/ts/library/src/func/pvi.ts) |

Python: `talib.PVI` (ta-lib-python 0.8.1 or later), see [ta-lib-python](https://ta-lib.github.io/ta-lib-python/func_groups/volume_indicators.html).

TA-Lib is also available for R and more using a [wrapper](/install/#wrappers).

## Aliases

Positive Volume Index

## References

- Norman G. Fosback, *Stock Market Logic*, The Institute for Econometric Research (ISBN 0917604482)
