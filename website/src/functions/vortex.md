---
title: "Vortex Indicator (VORTEX)"
description: "Vortex Indicator: Etienne Botes and Douglas Siepman's two-line trend indicator (Technical Analysis of Stocks & Commodities 28:1, January 2010)."
---

## Summary

Vortex Indicator: Etienne Botes and Douglas Siepman's two-line trend indicator (*Technical Analysis of Stocks & Commodities* 28:1, January 2010). Positive and negative "vortex movement" — the reach from today's high to yesterday's low and from today's low to yesterday's high — each summed over the period and normalized by the summed true range.

A +VI line crossing above −VI is the bullish signal the authors describe; the two lines are conventionally plotted together.

## Chart

<ta-chart src="/functions/vortex.svg" ratio="720:416" label="VORTEX over IBM, daily, 1999" alt="Drawn on the price pane over candles; below it, a separate indicator pane showing outPlusVI as a solid line and outMinusVI as a solid line."></ta-chart>

::: details Chart metadata <span class="flag-tip" tabindex="0" role="note" aria-label="The chart is drawn from the function's metadata: the flags and attributes the abstract API reports for it and for each output, and the input series its reference run declares." data-tip="The chart is drawn from the function's metadata: the flags and attributes the abstract API reports for it and for each output, and the input series its reference run declares.">i</span>

Drawn on the price pane over candles; below it, a separate indicator pane showing outPlusVI as a solid line and outMinusVI as a solid line.

Metadata flags used: `inPriceHLC: high, low, close` · `outPlusVI: line` · `outMinusVI: line`

:::

[Reference data (CSV)](/functions/vortex.csv) · [Chart (SVG)](/functions/vortex.svg)

## Formula

Per bar, `TR[i] = max(H[i]−L[i], |C[i−1]−H[i]|, |C[i−1]−L[i]|)` (exactly [`TRANGE`](/functions/trange.md)), `VMP[i] = |H[i] − L[i−1]|` and `VMM[i] = |L[i] − H[i−1]|`. Then `+VI = SUM(VMP, n) / SUM(TR, n)` and `−VI = SUM(VMM, n) / SUM(TR, n)`.

No smoothing, no recursion, no seeding — three rolling sums over per-bar terms. Every source (the original TASC article, StockCharts, Wikipedia, TradingView) states the identical formula; the only cross-source difference is the suggested period (14 vs Wikipedia's worked 21). A window whose every bar is flat sums the true range to zero; both lines then emit 0.0, the convention the external implementations share.

## Inputs

- `inHigh` — High price series
- `inLow` — Low price series
- `inClose` — Close price series

## Outputs

- `outPlusVI` — Positive vortex line (+VI)
- `outMinusVI` — Negative vortex line (−VI)

## Parameters

| Parameter | Type | Default | Accepted values | Description |
| --- | --- | --- | --- | --- |
| `optInTimePeriod` | integer | 14 | 1–100000 | Number of bars in the rolling sums |

## Notes

- Bar 0 has no term (all three need a prior bar) and is consumed exactly as [`TRANGE`](/functions/trange.md) consumes it, so the first output sits at index `optInTimePeriod`, not `optInTimePeriod − 1`.
- Not start-dependent: each output depends only on the finite trailing window. No unstable period.

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
double close[252]; /* your data, oldest first */
double plusVi[252], minusVi[252];
int outBegIdx, outNBElement;

TA_Initialize();
TA_VORTEX( 0, n - 1, high, low, close, 14, &outBegIdx, &outNBElement, plusVi, minusVi );
printf( "%d values from bar %d; last plusVi = %f\n", outNBElement, outBegIdx, plusVi[outNBElement - 1] );
```

@tab Rust

```rust
use ta_lib::Core;

let high = vec![0.0; 252]; // your data, oldest first
let low = vec![0.0; 252]; // your data, oldest first
let close = vec![0.0; 252]; // your data, oldest first
let mut plus_vi = vec![0.0; 252];
let mut minus_vi = vec![0.0; 252];

let core = Core::new();
let range = core.vortex(0, high.len() - 1, &high, &low, &close, 14,
                        &mut plus_vi, &mut minus_vi)?;
println!("{} values from bar {}; last plus_vi = {}", range.count, range.beg_idx, plus_vi[range.count - 1]);
```

@tab Java

```java
import io.github.talib.Core;
import io.github.talib.OutRange;

double[] high = new double[252]; // your data, oldest first
double[] low = new double[252]; // your data, oldest first
double[] close = new double[252]; // your data, oldest first
double[] plusVi = new double[252];
double[] minusVi = new double[252];

Core core = Core.DEFAULT;
OutRange r = core.vortex(0, high.length - 1, high, low, close, 14, plusVi, minusVi);
System.out.println(r.count() + " values from bar " + r.begIdx() + "; last plusVi = " + plusVi[r.count() - 1]);
```

@tab C\#

```csharp
using System;
using TALib;

var high = new double[252]; // your data, oldest first
var low = new double[252]; // your data, oldest first
var close = new double[252]; // your data, oldest first
var plusVi = new double[252];
var minusVi = new double[252];

var core = Core.Default;
OutRange r = core.Vortex(0, high.Length - 1, high, low, close, 14, plusVi, minusVi);
Console.WriteLine($"{r.Count} values from bar {r.BegIdx}; last plusVi = {plusVi[r.Count - 1]}");
```

@tab TypeScript

```ts
import { vortex } from 'ta-lib';

const high = new Float64Array(252); // your data, oldest first
const low = new Float64Array(252); // your data, oldest first
const close = new Float64Array(252); // your data, oldest first

const { begIdx, outPlusVi, outMinusVi } = vortex({ high, low, close }, {
  timePeriod: 14,
});
console.log(outPlusVi.length + ' values from bar ' + begIdx + '; last outPlusVi = ' + outPlusVi[outPlusVi.length - 1]);
```

:::

## Implementation

Added in 0.8.1 (ta-lib-python 0.8.1).

TA-Lib Definition: [`vortex.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/vortex/vortex.c) · [`vortex.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/vortex/vortex.yaml)

| Native | File |
|--------|------|
| C | [`ta_VORTEX.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_VORTEX.c) |
| Rust | [`vortex.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/library/src/ta_func/vortex.rs) |
| Java | [`Core_VORTEX.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/fragments/Core_VORTEX.java) |
| C# | [`Core_VORTEX.cs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/csharp/library/src/Core_VORTEX.cs) |
| TypeScript | [`vortex.ts`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/ts/library/src/func/vortex.ts) |

Python: `talib.VORTEX` (ta-lib-python 0.8.1 or later), see [ta-lib-python](https://ta-lib.github.io/ta-lib-python/func_groups/momentum_indicators.html).

TA-Lib is also available for R and more using a [wrapper](/install/#wrappers).

## Aliases

Vortex Indicator (VI)

## See Also

[TRANGE](/functions/trange.md) · [PLUS_DI](/functions/plus_di.md) · [MINUS_DI](/functions/minus_di.md) · [ADX](/functions/adx.md)

## References

- Etienne Botes and Douglas Siepman, "The Vortex Indicator", *Technical Analysis of Stocks & Commodities* 28:1 (January 2010), pp. 20–30
