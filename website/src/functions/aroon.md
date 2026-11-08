---
title: "Aroon (AROON)"
description: "Aroon reports how recently the highest high and lowest low occurred within a rolling window of length optInTimePeriod, as two 0-100 oscillators."
---

## Summary

Aroon reports how recently the highest high and lowest low occurred within a rolling window of length optInTimePeriod, as two 0-100 oscillators. Indicates trend strength and direction. Up near 100 = a very recent new high (strong uptrend); Down near 100 = a very recent new low. Up/Down crossovers signal trend shifts.

## Chart

<ta-chart src="/functions/aroon.svg" ratio="720:416" label="AROON over IBM, daily, 1999" alt="Drawn on the price pane over candles; below it, a separate indicator pane with a zero line showing outAroonDown as a dashed line and outAroonUp as a solid line."></ta-chart>

Drawn on the price pane over candles; below it, a separate indicator pane with a zero line showing outAroonDown as a dashed line and outAroonUp as a solid line. [Reference data (CSV)](/functions/aroon.csv) · [Chart (SVG)](/functions/aroon.svg)

## Formula

Up = 100*(period-(today-highestIdx))/period; Down = 100*(period-(today-lowestIdx))/period, where highestIdx/lowestIdx index the highest high / lowest low over the window [today-period .. today].

## Inputs

- `inHigh` — High price of each bar
- `inLow` — Low price of each bar

## Outputs

- `outAroonDown` — Recency of the lowest low (100 = it is the current bar, decaying as it ages)
- `outAroonUp` — Recency of the highest high (100 = it is the current bar, decaying as it ages)

## Parameters

| Parameter | Type | Default | Accepted values | Description |
| --- | --- | --- | --- | --- |
| `optInTimePeriod` | integer | 14 | 2–100000 | Lookback window length |

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
double high[252]; /* your data, oldest first */
double low[252]; /* your data, oldest first */
double aroonDown[252], aroonUp[252];
int outBegIdx, outNBElement;

TA_Initialize();
TA_AROON( 0, n - 1, high, low, 14, &outBegIdx, &outNBElement, aroonDown, aroonUp );
printf( "%d values from bar %d; last aroonDown = %f\n", outNBElement, outBegIdx, aroonDown[outNBElement - 1] );
```

@tab Rust

```rust
use ta_lib::Core;

let high = vec![0.0; 252]; // your data, oldest first
let low = vec![0.0; 252]; // your data, oldest first
let mut aroon_down = vec![0.0; 252];
let mut aroon_up = vec![0.0; 252];

let core = Core::new();
let range = core.aroon(0, high.len() - 1, &high, &low, 14, &mut aroon_down, &mut aroon_up)?;
println!("{} values from bar {}; last aroon_down = {}", range.count, range.beg_idx, aroon_down[range.count - 1]);
```

@tab Java

```java
import io.github.talib.Core;
import io.github.talib.OutRange;

double[] high = new double[252]; // your data, oldest first
double[] low = new double[252]; // your data, oldest first
double[] aroonDown = new double[252];
double[] aroonUp = new double[252];

Core core = Core.DEFAULT;
OutRange r = core.aroon(0, high.length - 1, high, low, 14, aroonDown, aroonUp);
System.out.println(r.count() + " values from bar " + r.begIdx() + "; last aroonDown = " + aroonDown[r.count() - 1]);
```

@tab C\#

```csharp
using System;
using TALib;

var high = new double[252]; // your data, oldest first
var low = new double[252]; // your data, oldest first
var aroonDown = new double[252];
var aroonUp = new double[252];

var core = Core.Default;
OutRange r = core.Aroon(0, high.Length - 1, high, low, 14, aroonDown, aroonUp);
Console.WriteLine($"{r.Count} values from bar {r.BegIdx}; last aroonDown = {aroonDown[r.Count - 1]}");
```

@tab TypeScript

```ts
import { aroon } from 'ta-lib';

const high = new Float64Array(252); // your data, oldest first
const low = new Float64Array(252); // your data, oldest first

const { begIdx, outAroonDown, outAroonUp } = aroon({ high, low }, {
  timePeriod: 14,
});
console.log(outAroonDown.length + ' values from bar ' + begIdx + '; last outAroonDown = ' + outAroonDown[outAroonDown.length - 1]);
```

:::

## Implementation

Added in 0.4.0.

TA-Lib Definition: [`aroon.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/aroon/aroon.c) · [`aroon.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/aroon/aroon.yaml)

| Native | File |
|--------|------|
| C | [`ta_AROON.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_AROON.c) |
| Rust | [`aroon.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/library/src/ta_func/aroon.rs) |
| Java | [`Core_AROON.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/fragments/Core_AROON.java) |
| C# | [`Core_AROON.cs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/csharp/library/src/Core_AROON.cs) |
| TypeScript | [`aroon.ts`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/ts/library/src/func/aroon.ts) |

Python: `talib.AROON`, see [ta-lib-python](https://ta-lib.github.io/ta-lib-python/func_groups/momentum_indicators.html).

TA-Lib is also available for R and more using a [wrapper](/install/#wrappers).

## See Also

[AROONOSC](/functions/aroonosc.md) · [MINMAXINDEX](/functions/minmaxindex.md) · [MIN](/functions/min.md) · [MAX](/functions/max.md)

## References

- Tushar S. Chande
