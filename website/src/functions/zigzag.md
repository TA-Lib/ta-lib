---
title: "Zig Zag (ZIGZAG)"
description: "The percent-filter swing line."
---

## Summary

The percent-filter swing line. A leg runs from the last confirmed pivot to the most extreme price since it, and it reverses only when price moves at least `optInSensitivity` percent away from that extreme and at least `optInMinTrendLength` bars have passed since the pivot's own bar. Achelis describes the chart form and warns that *"the last 'leg' displayed in a Zig Zag chart can change"*.

This function does not emit the chart line, which is drawn with hindsight. It emits the causal state the line is drawn from, one row per bar and final once emitted: the price of the swing extreme currently being tracked, the absolute bar index of that extreme, and whether the current leg is up or down.

The chart is rebuilt exactly from the outputs and the seed. A pivot is confirmed at bar `i > outBegIdx` whenever `outTrend[i]` differs from `outTrend[i-1]`, and that pivot is `(outPivotIdx[i-1], outZigZag[i-1])`. When `outTrend[outBegIdx]` is +1 a reversal fired on the first output bar, and the pivot it confirmed is the seed, `(outBegIdx - optInMinTrendLength, inLow[outBegIdx - optInMinTrendLength])`, which no output row carries. The last, still-open pivot is `(outPivotIdx[last], outZigZag[last])` — the leg Achelis warns about.

## Chart

<ta-chart src="/functions/zigzag.svg" ratio="720:438" label="ZIGZAG over IBM, daily, 1999" alt="Drawn on the price pane over candles, with outZigZag as a solid line; below it, a step pane of its own for each of outTrend and outPivotIdx."></ta-chart>

::: details Chart metadata <span class="flag-tip" tabindex="0" role="note" aria-label="The chart is drawn from the function's metadata alone: the flags the abstract API reports for it and for each output." data-tip="The chart is drawn from the function's metadata alone: the flags the abstract API reports for it and for each output.">i</span>

Drawn on the price pane over candles, with outZigZag as a solid line; below it, a step pane of its own for each of outTrend and outPivotIdx.

Metadata flags used: `overlap` · `inPriceHL: high, low` · `outZigZag: line` · `outTrend: line` · `outPivotIdx: line`

:::

[Reference data (CSV)](/functions/zigzag.csv) · [Chart (SVG)](/functions/zigzag.svg)

## Formula

With `H = inHigh`, `L = inLow`, `m = optInMinTrendLength`, `s = optInSensitivity / 100`:

    up = 1 + s,  dn = 1 - s

    seed at bar j0 = startIdx - m:   trend = -1,  P = L[j0],  p = j0

    for i = j0+1 .. endIdx:
       trend = -1:  if H[i] >= P*up and i - p >= m   then trend = +1, P = H[i], p = i
                    else if L[i] <= P                then P = L[i], p = i
       trend = +1:  if L[i] <= P*dn and i - p >= m   then trend = -1, P = L[i], p = i
                    else if H[i] >= P                then P = H[i], p = i

       outZigZag = P,  outTrend = trend,  outPivotIdx = p

## Notes

- **Reversal is tested before extension.** On an outside bar that both makes a new extreme and clears the threshold, the leg reverses and the old pivot stays where it was. The other order moves the pivot to the new extreme and defers the reversal to the next bar, against a threshold that has just moved further away: on the 252-bar regtest corpus at 5 percent that costs one bar (R TTR, which takes that order, places the swing low at 148 where this places it at 147), but a range narrower than the sensitivity after the outside bar keeps it from ever firing, and the leg is then lost rather than delayed.
- **Ties extend.** The comparisons are `<=` and `>=`, so at an equal price the pivot moves to the later bar: `outZigZag` does not change and `outPivotIdx` does. That movement is the only thing the index output carries and the price output cannot.
- **The gate counts from the pivot's own bar, the seed included.** No reversal can fire before the first output bar; the first one can fire exactly there.
- **The seed is a low.** The first reversal is therefore always upward. A down reversal compares a bar's low with the tracked high, so a rising series stays on its up leg only while no low falls `optInSensitivity` percent under that high on a bar the gate allows.
- **The input must be positive.** The threshold is a percent of the pivot: at a pivot of 0 it sits on the pivot and below 0 on the wrong side of it, so the leg reverses on nearly every bar the gate allows. The output is undefined for such input and no error is returned. A series that crosses zero, such as an oscillator or a spread, needs an absolute reversal amount, which this function does not have.
- The lookback is `optInMinTrendLength` and does not depend on the sensitivity: the threshold decides whether a reversal fires, never how early it may.
- `optInSensitivity` is a percent, as Achelis, StockCharts, TTR, Skender and TradingView take it. 0 and 100 are degenerate but defined: at 0 a reversal fires on almost every bar the gate allows, and at 100 no downward reversal fires on positive prices.
- The value at a bar depends on where the caller started, through the seed; the function is `path_dependent` for that reason, as SAR and SUPERTREND are.

## Inputs

- `inHigh` — High price of each bar
- `inLow` — Low price of each bar

## Outputs

- `outZigZag` — Price of the swing extreme currently being tracked
- `outTrend` — +1 while the leg is up, -1 while it is down
- `outPivotIdx` — Absolute index, into the input, of the bar that extreme sits on

## Parameters

| Parameter | Type | Default | Accepted values | Description |
| --- | --- | --- | --- | --- |
| `optInSensitivity` | real | 5 | 0–100 | Minimum move away from the current extreme that reverses the leg, in percent |
| `optInMinTrendLength` | integer | 1 | 1–100000 | Minimum number of bars between two pivots |

## Properties

**Numerical Stability:** [Path-Dependent](/functions/stability.md#path-dependent)

<div class="flag-table">

|  |
| :-- |
| <span class="flag-box">✅</span> **Overlap Input** <span class="flag-tip" tabindex="0" role="note" aria-label="Output is on the same scale as the input price, so it is drawn over the price chart." data-tip="Output is on the same scale as the input price, so it is drawn over the price chart.">i</span> |
| <span class="flag-box">☐</span> <span style="opacity:0.5">Independent Y-Axis</span> |
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
double high[252]; /* your data, oldest first */
double low[252]; /* your data, oldest first */
double zigZag[252];
int trend[252], pivotIdx[252];
int outBegIdx, outNBElement;

TA_Initialize();
TA_ZIGZAG( 0, n - 1, high, low, 5.0, 1, &outBegIdx, &outNBElement, zigZag, trend, pivotIdx );
printf( "%d values from bar %d; last zigZag = %f\n", outNBElement, outBegIdx, zigZag[outNBElement - 1] );
```

@tab Rust

```rust
use ta_lib::Core;

let high = vec![0.0; 252]; // your data, oldest first
let low = vec![0.0; 252]; // your data, oldest first
let mut zig_zag = vec![0.0; 252];
let mut trend = vec![0i32; 252];
let mut pivot_idx = vec![0i32; 252];

let core = Core::new();
let range = core.zigzag(0, high.len() - 1, &high, &low, 5.0, 1,
                        &mut zig_zag, &mut trend, &mut pivot_idx)?;
println!("{} values from bar {}; last zig_zag = {}", range.count, range.beg_idx, zig_zag[range.count - 1]);
```

@tab Java

```java
import io.github.talib.Core;
import io.github.talib.OutRange;

double[] high = new double[252]; // your data, oldest first
double[] low = new double[252]; // your data, oldest first
double[] zigZag = new double[252];
int[] trend = new int[252];
int[] pivotIdx = new int[252];

Core core = Core.DEFAULT;
OutRange r = core.zigzag(0, high.length - 1, high, low, 5.0, 1, zigZag, trend, pivotIdx);
System.out.println(r.count() + " values from bar " + r.begIdx() + "; last zigZag = " + zigZag[r.count() - 1]);
```

@tab C\#

```csharp
using System;
using TALib;

var high = new double[252]; // your data, oldest first
var low = new double[252]; // your data, oldest first
var zigZag = new double[252];
var trend = new int[252];
var pivotIdx = new int[252];

var core = Core.Default;
OutRange r = core.Zigzag(0, high.Length - 1, high, low, 5.0, 1, zigZag, trend, pivotIdx);
Console.WriteLine($"{r.Count} values from bar {r.BegIdx}; last zigZag = {zigZag[r.Count - 1]}");
```

@tab TypeScript

```ts
import { zigzag } from 'ta-lib';

const high = new Float64Array(252); // your data, oldest first
const low = new Float64Array(252); // your data, oldest first

const {
  begIdx,
  outZigZag,
  outTrend,
  outPivotIdx,
} = zigzag({ high, low }, { sensitivity: 5, minTrendLength: 1 });
console.log(outZigZag.length + ' values from bar ' + begIdx + '; last outZigZag = ' + outZigZag[outZigZag.length - 1]);
```

:::

## Implementation

Added in 0.8.3 (ta-lib-python 0.8.3).

TA-Lib Definition: [`zigzag.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/zigzag/zigzag.c) · [`zigzag.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/zigzag/zigzag.yaml)

| Native | File |
|--------|------|
| C | [`ta_ZIGZAG.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_ZIGZAG.c) |
| Rust | [`zigzag.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/library/src/ta_func/zigzag.rs) |
| Java | [`Core_ZIGZAG.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/fragments/Core_ZIGZAG.java) |
| C# | [`Core_ZIGZAG.cs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/csharp/library/src/Core_ZIGZAG.cs) |
| TypeScript | [`zigzag.ts`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/ts/library/src/func/zigzag.ts) |

Python: `talib.ZIGZAG` (ta-lib-python 0.8.3 or later), see [ta-lib-python](https://ta-lib.github.io/ta-lib-python/func_groups/overlap_studies.html).

TA-Lib is also available for R and more using a [wrapper](/install/#wrappers).

## Aliases

Zig Zag, ZigZag, Filtered Waves

## See Also

[SAR](/functions/sar.md) · [SUPERTREND](/functions/supertrend.md) · [MINMAXINDEX](/functions/minmaxindex.md) · [MIDPRICE](/functions/midprice.md)

## References

- Arthur A. Merrill, *Filtered Waves, Basic Theory: A Tool for Stock Market Analysis*, The Analysis Press, 1977
- Steven B. Achelis, *Technical Analysis from A to Z*, "Zig Zag"
- StockCharts ChartSchool, "ZigZag"
