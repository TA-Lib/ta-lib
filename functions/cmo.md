---
url: 'https://ta-lib.org/functions/cmo.md'
description: >-
  Chande Momentum Oscillator: bounded momentum measure from Wilder-smoothed
  average up-moves and down-moves.
---
# Chande Momentum Oscillator (CMO)

## Summary

Chande Momentum Oscillator: bounded momentum measure from Wilder-smoothed average up-moves and down-moves. Identical to RSI except the numerator uses (gain-loss) instead of gain. Bounded in \[-100,+100]; positive = net upward momentum, negative = net downward.

## Chart

<ta-chart src="/functions/cmo.svg" ratio="720:416" label="CMO over IBM, daily, 1999" alt="Drawn on the price pane over the close as a line; below it, a separate indicator pane with a zero line showing outReal as a solid line with dashed rules at -50 and 50."></ta-chart>

::: details Chart metadata <span class="flag-tip" tabindex="0" role="note" aria-label="The chart is drawn from the function's metadata: the flags and attributes the abstract API reports for it and for each output, and the input series its reference run declares." data-tip="The chart is drawn from the function's metadata: the flags and attributes the abstract API reports for it and for each output, and the input series its reference run declares.">i</span>

Drawn on the price pane over the close as a line; below it, a separate indicator pane with a zero line showing outReal as a solid line with dashed rules at -50 and 50.

Metadata flags used: `outReal: line, positive, negative, levels -50, 50`

:::

[Reference data (CSV)](/functions/cmo.csv) · [Chart (SVG)](/functions/cmo.svg)

## Formula

d = P\[t]-P\[t-1]; over the initial period accumulate gain = sum of positive d, loss = sum of -d for negative d. Wilder-smooth each: prevGain = (prevGain\*(period-1) + gain_today)/period (same for loss). CMO = 100 \* (prevGain-prevLoss)/(prevGain+prevLoss); 0 when prevGain+prevLoss == 0.

## Notes

* Gains and losses are smoothed with Wilder's method (as in RSI) rather than the simple period sums of Chande's original definition.

## Inputs

* `inReal` — Source price/value series

## Outputs

* `outReal` — CMO oscillator value

## Parameters

| Parameter | Type | Default | Accepted values | Description |
| --- | --- | --- | --- | --- |
| `optInTimePeriod` | integer | 14 | 2–100000 | Bars over which gains/losses are smoothed |

## Properties

**Numerical Stability:** [Initial Unstable Period](/functions/stability.md#initial-unstable-period)

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
TA_CMO( 0, n - 1, series, 14, &outBegIdx, &outNBElement, out );
printf( "%d values from bar %d; last out = %f\n", outNBElement, outBegIdx, out[outNBElement - 1] );
```

@tab Rust

```rust
use ta_lib::Core;

let series = vec![0.0; 252]; // your data, oldest first
let mut out = vec![0.0; 252];

let core = Core::new();
let range = core.cmo(0, series.len() - 1, &series, 14, &mut out)?;
println!("{} values from bar {}; last out = {}", range.count, range.beg_idx, out[range.count - 1]);
```

@tab Java

```java
import io.github.talib.Core;
import io.github.talib.OutRange;

double[] series = new double[252]; // your data, oldest first
double[] out = new double[252];

Core core = Core.DEFAULT;
OutRange r = core.cmo(0, series.length - 1, series, 14, out);
System.out.println(r.count() + " values from bar " + r.begIdx() + "; last out = " + out[r.count() - 1]);
```

@tab C#

```csharp
using System;
using TALib;

var series = new double[252]; // your data, oldest first
var outReal = new double[252];

var core = Core.Default;
OutRange r = core.Cmo(0, series.Length - 1, series, 14, outReal);
Console.WriteLine($"{r.Count} values from bar {r.BegIdx}; last outReal = {outReal[r.Count - 1]}");
```

@tab TypeScript

```ts
import { cmo } from 'ta-lib';

const series = new Float64Array(252); // your data, oldest first

const { begIdx, values } = cmo(series, { timePeriod: 14 });
console.log(values.length + ' values from bar ' + begIdx + '; last values = ' + values[values.length - 1]);
```

:::

## Implementation

Added in 0.4.0.

TA-Lib Definition: [`cmo.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/cmo/cmo.c) · [`cmo.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/cmo/cmo.yaml)

| Native | File |
|--------|------|
| C | [`ta_CMO.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_CMO.c) |
| Rust | [`cmo.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/library/src/ta_func/cmo.rs) |
| Java | [`Core_CMO.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/fragments/Core_CMO.java) |
| C# | [`Core_CMO.cs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/csharp/library/src/Core_CMO.cs) |
| TypeScript | [`cmo.ts`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/ts/library/src/func/cmo.ts) |

Python: `talib.CMO`, see [ta-lib-python](https://ta-lib.github.io/ta-lib-python/func_groups/momentum_indicators.html).

TA-Lib is also available for R and more using a [wrapper](/install/#wrappers).

## Aliases

Chande Momentum Oscillator

## See Also

[RSI](/functions/rsi.md)

## References

* Tushar S. Chande, *The New Technical Trader*, John Wiley & Sons (ISBN 0471597805)
