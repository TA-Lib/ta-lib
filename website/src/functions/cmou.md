---
title: "Chande Momentum Oscillator (Unsmoothed) (CMOU)"
description: "Chande Momentum Oscillator: Tushar Chande's original momentum oscillator, computed from plain moving-window sums of the up-moves and down-moves over the…"
---

## Summary

Chande Momentum Oscillator: Tushar Chande's original momentum oscillator, computed from **plain moving-window sums** of the up-moves and down-moves over the period.

Bounded in [-100,+100]; positive = net upward momentum, negative = net downward.

CMOU is the version as defined by Chande in his book *The New Technical Trader* (1994), and is the more common implementation used by TradingView (`ta.cmo`) and pandas-ta's default.

See [`CMO`](/functions/cmo.md) for a smoothed variant of CMOU.

## Chart

<ta-chart src="/functions/cmou.svg" ratio="720:416" label="CMOU over IBM, daily, 1999" alt="Drawn on the price pane over the close as a line; below it, a separate indicator pane with a zero line showing outReal as a solid line."></ta-chart>

Drawn on the price pane over the close as a line; below it, a separate indicator pane with a zero line showing outReal as a solid line. [Reference data (CSV)](/functions/cmou.csv) · [Chart (SVG)](/functions/cmou.svg)

## Formula

d = P[t]-P[t-1]; over the trailing `optInTimePeriod` changes accumulate Su = sum of the positive d, Sd = sum of -d for negative d. CMOU = 100 * (Su-Sd)/(Su+Sd); 0 when Su+Sd == 0 (an exactly flat window). Unlike CMO, the sums are the plain period totals (a moving-window sum), not Wilder-smoothed averages, so there is no unstable period.

## Inputs

- `inReal` — Source price/value series

## Outputs

- `outReal` — CMOU oscillator value

## Parameters

| Parameter | Type | Default | Accepted values | Description |
| --- | --- | --- | --- | --- |
| `optInTimePeriod` | integer | 14 | 2–100000 | Number of trailing price changes summed |

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
TA_CMOU( 0, n - 1, series, 14, &outBegIdx, &outNBElement, out );
printf( "%d values from bar %d; last out = %f\n", outNBElement, outBegIdx, out[outNBElement - 1] );
```

@tab Rust

```rust
use ta_lib::Core;

let series = vec![0.0; 252]; // your data, oldest first
let mut out = vec![0.0; 252];

let core = Core::new();
let range = core.cmou(0, series.len() - 1, &series, 14, &mut out)?;
println!("{} values from bar {}; last out = {}", range.count, range.beg_idx, out[range.count - 1]);
```

@tab Java

```java
import io.github.talib.Core;
import io.github.talib.OutRange;

double[] series = new double[252]; // your data, oldest first
double[] out = new double[252];

Core core = Core.DEFAULT;
OutRange r = core.cmou(0, series.length - 1, series, 14, out);
System.out.println(r.count() + " values from bar " + r.begIdx() + "; last out = " + out[r.count() - 1]);
```

@tab C\#

```csharp
using System;
using TALib;

var series = new double[252]; // your data, oldest first
var outReal = new double[252];

var core = Core.Default;
OutRange r = core.Cmou(0, series.Length - 1, series, 14, outReal);
Console.WriteLine($"{r.Count} values from bar {r.BegIdx}; last outReal = {outReal[r.Count - 1]}");
```

@tab TypeScript

```ts
import { cmou } from 'ta-lib';

const series = new Float64Array(252); // your data, oldest first

const { begIdx, values } = cmou(series, { timePeriod: 14 });
console.log(values.length + ' values from bar ' + begIdx + '; last values = ' + values[values.length - 1]);
```

:::

## Implementation

Added in 0.8.1 (ta-lib-python 0.8.1).

TA-Lib Definition: [`cmou.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/cmou/cmou.c) · [`cmou.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/cmou/cmou.yaml)

| Native | File |
|--------|------|
| C | [`ta_CMOU.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_CMOU.c) |
| Rust | [`cmou.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/library/src/ta_func/cmou.rs) |
| Java | [`Core_CMOU.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/fragments/Core_CMOU.java) |
| C# | [`Core_CMOU.cs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/csharp/library/src/Core_CMOU.cs) |
| TypeScript | [`cmou.ts`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/ts/library/src/func/cmou.ts) |

Python: `talib.CMOU` (ta-lib-python 0.8.1 or later), see [ta-lib-python](https://ta-lib.github.io/ta-lib-python/func_groups/momentum_indicators.html).

TA-Lib is also available for R and more using a [wrapper](/install/#wrappers).

## Aliases

Chande Momentum Oscillator (Unsmoothed)

## See Also

[CMO](/functions/cmo.md) · [RSI](/functions/rsi.md)

## References

- Tushar S. Chande, *The New Technical Trader*, John Wiley & Sons (ISBN 0471597805)
