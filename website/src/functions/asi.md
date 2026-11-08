---
title: "Wilder Accumulative Swing Index (ASI)"
description: "Wilder's Accumulative Swing Index: the running total of the Swing Index (SI), a line meant to trace the market's real swings through the noise of the…"
---

## Summary

Wilder's Accumulative Swing Index: the running total of the Swing Index (SI), a line meant to trace the market's real swings through the noise of the daily closes. Wilder draws trendlines and breakout points on it as on a price chart: a breakout of the ASI that the price has not confirmed yet is his signal. J. Welles Wilder Jr. introduced both in 1978. The line starts at 0 on the first bar of the requested range, so only its shape carries meaning, not its level.

## Chart

<ta-chart src="/functions/asi.svg" ratio="720:416" label="ASI over IBM, daily, 1999" alt="Drawn on the price pane over candles; below it, a separate indicator pane with a zero line showing outReal as a solid line."></ta-chart>

Drawn on the price pane over candles; below it, a separate indicator pane with a zero line showing outReal as a solid line. [Reference data (CSV)](/functions/asi.csv) · [Chart (SVG)](/functions/asi.svg)

## Formula

$$
\mathrm{ASI}_s = 0 \qquad \mathrm{ASI}_t = \mathrm{ASI}_{t-1} + \mathrm{SI}_t \quad (t > s)
$$

where $s$ is the first bar of the requested range and $\mathrm{SI}_t$ is the Swing Index of bar $t$ against bar $t-1$ at the same `optInLimitMove`.

## Notes

- Wilder accumulates Swing Index values rounded to whole numbers; the running total here is of the unrounded values.

## Inputs

- `inOpen` — Open price of each bar
- `inHigh` — High price of each bar
- `inLow` — Low price of each bar
- `inClose` — Close price of each bar

## Outputs

- `outReal` — Running total of the swing index from the first bar of the range

## Parameters

| Parameter | Type | Default | Accepted values | Description |
| --- | --- | --- | --- | --- |
| `optInLimitMove` | real | 3 | ≥ 0.00000001 | Limit move, the largest one-bar price move the index is scaled against, in price units |

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
| <span class="flag-box">☐</span> <span style="opacity:0.5">Input Must Be Positive</span> |

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
double out[252];
int outBegIdx, outNBElement;

TA_Initialize();
TA_ASI( 0, n - 1, open, high, low, close, 3.0, &outBegIdx, &outNBElement, out );
printf( "%d values from bar %d; last out = %f\n", outNBElement, outBegIdx, out[outNBElement - 1] );
```

@tab Rust

```rust
use ta_lib::Core;

let open = vec![0.0; 252]; // your data, oldest first
let high = vec![0.0; 252]; // your data, oldest first
let low = vec![0.0; 252]; // your data, oldest first
let close = vec![0.0; 252]; // your data, oldest first
let mut out = vec![0.0; 252];

let core = Core::new();
let range = core.asi(0, open.len() - 1, &open, &high, &low, &close, 3.0, &mut out)?;
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
double[] out = new double[252];

Core core = Core.DEFAULT;
OutRange r = core.asi(0, open.length - 1, open, high, low, close, 3.0, out);
System.out.println(r.count() + " values from bar " + r.begIdx() + "; last out = " + out[r.count() - 1]);
```

@tab C\#

```csharp
using System;
using TALib;

var open = new double[252]; // your data, oldest first
var high = new double[252]; // your data, oldest first
var low = new double[252]; // your data, oldest first
var close = new double[252]; // your data, oldest first
var outReal = new double[252];

var core = Core.Default;
OutRange r = core.Asi(0, open.Length - 1, open, high, low, close, 3.0, outReal);
Console.WriteLine($"{r.Count} values from bar {r.BegIdx}; last outReal = {outReal[r.Count - 1]}");
```

@tab TypeScript

```ts
import { asi } from 'ta-lib';

const open = new Float64Array(252); // your data, oldest first
const high = new Float64Array(252); // your data, oldest first
const low = new Float64Array(252); // your data, oldest first
const close = new Float64Array(252); // your data, oldest first

const { begIdx, values } = asi({ open, high, low, close }, { limitMove: 3 });
console.log(values.length + ' values from bar ' + begIdx + '; last values = ' + values[values.length - 1]);
```

:::

## Implementation

Added in 0.8.3 (ta-lib-python 0.8.3).

TA-Lib Definition: [`asi.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/asi/asi.c) · [`asi.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/asi/asi.yaml)

| Native | File |
|--------|------|
| C | [`ta_ASI.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_ASI.c) |
| Rust | [`asi.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/library/src/ta_func/asi.rs) |
| Java | [`Core_ASI.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/fragments/Core_ASI.java) |
| C# | [`Core_ASI.cs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/csharp/library/src/Core_ASI.cs) |
| TypeScript | [`asi.ts`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/ts/library/src/func/asi.ts) |

Python: `talib.ASI` (ta-lib-python 0.8.3 or later), see [ta-lib-python](https://ta-lib.github.io/ta-lib-python/func_groups/momentum_indicators.html).

TA-Lib is also available for R and more using a [wrapper](/install/#wrappers).

## Aliases

Accumulation Swing Index

## See Also

[SI](/functions/si.md) · [WAD](/functions/wad.md) · [CUMSUM](/functions/cumsum.md)

## References

- J. Welles Wilder Jr., *New Concepts in Technical Trading Systems*, Trend Research, 1978, section VIII "The Swing Index", pp.87-106
