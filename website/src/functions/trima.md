---
title: "Triangular Moving Average (TRIMA)"
description: "Triangular Moving Average: a double-smoothed moving average that weights prices toward the middle of the window most heavily."
---

## Summary

Triangular Moving Average: a double-smoothed moving average that weights prices toward the middle of the window most heavily. Equivalent to an SMA of an SMA, computed here via an incremental triangular-weighted running numerator.

## Chart

<ta-chart src="/functions/trima.svg" ratio="720:274" label="TRIMA over IBM, daily, 1999" alt="Drawn on the price pane over the close as a line, with outReal as a solid line."></ta-chart>

::: details Chart metadata <span class="flag-tip" tabindex="0" role="note" aria-label="The chart is drawn from the function's metadata: the flags and attributes the abstract API reports for it and for each output, and the input series its reference run declares." data-tip="The chart is drawn from the function's metadata: the flags and attributes the abstract API reports for it and for each output, and the input series its reference run declares.">i</span>

Drawn on the price pane over the close as a line, with outReal as a solid line.

Metadata flags used: `overlap` · `outReal: line`

:::

[Reference data (CSV)](/functions/trima.csv) · [Chart (SVG)](/functions/trima.svg)

## Formula

Weights rise then fall (4-period: (1a+2b+2c+1d)/6; 5-period: (1a+2b+3c+2d+1e)/9). With n = period>>1: odd divides by (n+1)^2, even by n(n+1). Equivalent to odd: SMA(SMA(x,(period+1)/2),(period+1)/2); even: SMA(SMA(x,period/2),period/2+1).

## Notes

- Follows the generally accepted (Metastock) definition rather than the TradeStation variant.
- A period of 1 performs no smoothing: the output is a copy of the input. Allowed since 0.6.5 (issues #48/#59).

## Inputs

- `inReal` — Source price series

## Outputs

- `outReal` — Triangular moving average

## Parameters

| Parameter | Type | Default | Accepted values | Description |
| --- | --- | --- | --- | --- |
| `optInTimePeriod` | integer | 30 | 1–100000 | Number of bars in the averaging window |

## Properties

**Numerical Stability:** [Start-Independent](/functions/stability.md#start-independent)

<div class="flag-table">

|  |
| :-- |
| <span class="flag-box">✅</span> **Overlap Input** <span class="flag-tip" tabindex="0" role="note" aria-label="Output is on the same scale as the input price, so it is drawn over the price chart." data-tip="Output is on the same scale as the input price, so it is drawn over the price chart.">i</span> |
| <span class="flag-box">☐</span> <span style="opacity:0.5">Independent Y-Axis</span> |
| <span class="flag-box">☐</span> <span style="opacity:0.5">Candlestick</span> |
| <span class="flag-box">☐</span> <span style="opacity:0.5">Can Output NaN or ±Inf</span> |
| <span class="flag-box">✅</span> **Identity at Period 1** <span class="flag-tip" tabindex="0" role="note" aria-label="A period of 1 performs no smoothing: every output value is a bit-exact copy of its input value." data-tip="A period of 1 performs no smoothing: every output value is a bit-exact copy of its input value.">i</span> |
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
TA_TRIMA( 0, n - 1, series, 30, &outBegIdx, &outNBElement, out );
printf( "%d values from bar %d; last out = %f\n", outNBElement, outBegIdx, out[outNBElement - 1] );
```

@tab Rust

```rust
use ta_lib::Core;

let series = vec![0.0; 252]; // your data, oldest first
let mut out = vec![0.0; 252];

let core = Core::new();
let range = core.trima(0, series.len() - 1, &series, 30, &mut out)?;
println!("{} values from bar {}; last out = {}", range.count, range.beg_idx, out[range.count - 1]);
```

@tab Java

```java
import io.github.talib.Core;
import io.github.talib.OutRange;

double[] series = new double[252]; // your data, oldest first
double[] out = new double[252];

Core core = Core.DEFAULT;
OutRange r = core.trima(0, series.length - 1, series, 30, out);
System.out.println(r.count() + " values from bar " + r.begIdx() + "; last out = " + out[r.count() - 1]);
```

@tab C\#

```csharp
using System;
using TALib;

var series = new double[252]; // your data, oldest first
var outReal = new double[252];

var core = Core.Default;
OutRange r = core.Trima(0, series.Length - 1, series, 30, outReal);
Console.WriteLine($"{r.Count} values from bar {r.BegIdx}; last outReal = {outReal[r.Count - 1]}");
```

@tab TypeScript

```ts
import { trima } from 'ta-lib';

const series = new Float64Array(252); // your data, oldest first

const { begIdx, values } = trima(series, { timePeriod: 30 });
console.log(values.length + ' values from bar ' + begIdx + '; last values = ' + values[values.length - 1]);
```

:::

## Implementation

Added in 0.4.0.

TA-Lib Definition: [`trima.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/trima/trima.c) · [`trima.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/trima/trima.yaml)

| Native | File |
|--------|------|
| C | [`ta_TRIMA.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_TRIMA.c) |
| Rust | [`trima.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/library/src/ta_func/trima.rs) |
| Java | [`Core_TRIMA.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/fragments/Core_TRIMA.java) |
| C# | [`Core_TRIMA.cs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/csharp/library/src/Core_TRIMA.cs) |
| TypeScript | [`trima.ts`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/ts/library/src/func/trima.ts) |

Python: `talib.TRIMA`, see [ta-lib-python](https://ta-lib.github.io/ta-lib-python/func_groups/overlap_studies.html).

TA-Lib is also available for R and more using a [wrapper](/install/#wrappers).

## Aliases

Triangular Moving Average

## See Also

[SMA](/functions/sma.md) · [WMA](/functions/wma.md) · [MA](/functions/ma.md)
