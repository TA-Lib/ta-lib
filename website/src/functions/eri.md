---
title: "Elder Ray Index (Bull Power / Bear Power) (ERI)"
description: "Elder Ray Index: Alexander Elder's Bull Power / Bear Power pair from Trading for a Living (1993) — how far the bar's high and low sit from an EMA of the…"
---

## Summary

Elder Ray Index: Alexander Elder's Bull Power / Bear Power pair from *Trading for a Living* (1993) — how far the bar's high and low sit from an EMA of the close. Bulls strong enough to push the high above the average read as positive Bull Power; bears dragging the low below it read as negative Bear Power.

## Chart

<ta-chart src="/functions/eri.svg" ratio="720:416" label="ERI over IBM, daily, 1999" alt="Drawn on the price pane over candles; below it, a separate indicator pane with a zero line showing outBullPower as a solid line and outBearPower as a solid line."></ta-chart>

Drawn on the price pane over candles; below it, a separate indicator pane with a zero line showing outBullPower as a solid line and outBearPower as a solid line. [Reference data (CSV)](/functions/eri.csv) · [Chart (SVG)](/functions/eri.svg)

## Formula

`Bull Power = High − EMA(Close, n)` and `Bear Power = Low − EMA(Close, n)`, both lines against the **same** EMA. Bull ≥ Bear on every bar since high ≥ low. TradingView's built-in *Bull Bear Power* — which its own support page calls "otherwise known as the Elder-Ray Index" — plots only the sum of the two, not the pair; StockCharts, TC2000 and pandas-ta all ship the two lines.

Because the underlying average is an [`EMA`](/functions/ema.md), ERI inherits its unstable period: its warm-up is exactly that of `EMA` at the same period, whether `TA_FUNC_UNST_EMA` is set to a count or to an Auto level.

## Inputs

- `inHigh` — High price series
- `inLow` — Low price series
- `inClose` — Close price series

## Outputs

- `outBullPower` — High minus the EMA of close
- `outBearPower` — Low minus the EMA of close

## Parameters

| Parameter | Type | Default | Accepted values | Description |
| --- | --- | --- | --- | --- |
| `optInTimePeriod` | integer | 13 | 1–100000 | Number of bars in the EMA of close |

## Notes

- ERI is a cancelling difference: near the zero crossings that carry its signal, tiny EMA discrepancies are amplified without bound in relative terms. Compare against external values with an absolute tolerance.
- No MAType parameter: every canonical source fixes the EMA, and a selectable average would invent a variant nobody ships.

## Properties

**Numerical Stability:** [Initial Unstable Period](/functions/stability.md#initial-unstable-period) — Inherited from EMA, which ERI computes internally; tunable via EMA's unstable period.

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
double bullPower[252], bearPower[252];
int outBegIdx, outNBElement;

TA_Initialize();
TA_ERI( 0, n - 1, high, low, close, 13, &outBegIdx, &outNBElement, bullPower, bearPower );
printf( "%d values from bar %d; last bullPower = %f\n", outNBElement, outBegIdx, bullPower[outNBElement - 1] );
```

@tab Rust

```rust
use ta_lib::Core;

let high = vec![0.0; 252]; // your data, oldest first
let low = vec![0.0; 252]; // your data, oldest first
let close = vec![0.0; 252]; // your data, oldest first
let mut bull_power = vec![0.0; 252];
let mut bear_power = vec![0.0; 252];

let core = Core::new();
let range = core.eri(0, high.len() - 1, &high, &low, &close, 13,
                     &mut bull_power, &mut bear_power)?;
println!("{} values from bar {}; last bull_power = {}", range.count, range.beg_idx, bull_power[range.count - 1]);
```

@tab Java

```java
import io.github.talib.Core;
import io.github.talib.OutRange;

double[] high = new double[252]; // your data, oldest first
double[] low = new double[252]; // your data, oldest first
double[] close = new double[252]; // your data, oldest first
double[] bullPower = new double[252];
double[] bearPower = new double[252];

Core core = Core.DEFAULT;
OutRange r = core.eri(0, high.length - 1, high, low, close, 13, bullPower, bearPower);
System.out.println(r.count() + " values from bar " + r.begIdx() + "; last bullPower = " + bullPower[r.count() - 1]);
```

@tab C\#

```csharp
using System;
using TALib;

var high = new double[252]; // your data, oldest first
var low = new double[252]; // your data, oldest first
var close = new double[252]; // your data, oldest first
var bullPower = new double[252];
var bearPower = new double[252];

var core = Core.Default;
OutRange r = core.Eri(0, high.Length - 1, high, low, close, 13, bullPower, bearPower);
Console.WriteLine($"{r.Count} values from bar {r.BegIdx}; last bullPower = {bullPower[r.Count - 1]}");
```

@tab TypeScript

```ts
import { eri } from 'ta-lib';

const high = new Float64Array(252); // your data, oldest first
const low = new Float64Array(252); // your data, oldest first
const close = new Float64Array(252); // your data, oldest first

const { begIdx, outBullPower, outBearPower } = eri({ high, low, close }, {
  timePeriod: 13,
});
console.log(outBullPower.length + ' values from bar ' + begIdx + '; last outBullPower = ' + outBullPower[outBullPower.length - 1]);
```

:::

## Implementation

Added in 0.8.1 (ta-lib-python 0.8.1).

TA-Lib Definition: [`eri.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/eri/eri.c) · [`eri.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/eri/eri.yaml)

| Native | File |
|--------|------|
| C | [`ta_ERI.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_ERI.c) |
| Rust | [`eri.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/library/src/ta_func/eri.rs) |
| Java | [`Core_ERI.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/fragments/Core_ERI.java) |
| C# | [`Core_ERI.cs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/csharp/library/src/Core_ERI.cs) |
| TypeScript | [`eri.ts`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/ts/library/src/func/eri.ts) |

Python: `talib.ERI` (ta-lib-python 0.8.1 or later), see [ta-lib-python](https://ta-lib.github.io/ta-lib-python/func_groups/momentum_indicators.html).

TA-Lib is also available for R and more using a [wrapper](/install/#wrappers).

## Aliases

Elder-Ray Index · Bull Power / Bear Power

## See Also

[EMA](/functions/ema.md) · [EFI](/functions/efi.md) · [MACD](/functions/macd.md)

## References

- Alexander Elder, *Trading for a Living* (Wiley, 1993), the Elder-Ray chapter
