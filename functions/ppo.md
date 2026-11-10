---
url: 'https://ta-lib.org/functions/ppo.md'
description: >-
  Percentage Price Oscillator: the difference between a fast and slow moving
  average expressed as a percentage of the slow MA.
---
# Percentage Price Oscillator (PPO)

## Summary

Percentage Price Oscillator: the difference between a fast and slow moving average expressed as a percentage of the slow MA. A normalized (scale-invariant) variant of APO. Positive when the fast MA is above the slow MA (upward momentum), negative otherwise; magnitude is the % deviation.

## Chart

<ta-chart src="/functions/ppo.svg" ratio="720:416" label="PPO over IBM, daily, 1999" alt="Drawn on the price pane over the close as a line; below it, a separate indicator pane with a zero line showing outReal as a solid line."></ta-chart>

<div class="ta-chart-links">

[Reference data (CSV)](/functions/ppo.csv) · [Chart (SVG)](/functions/ppo.svg)

</div>

::: details Chart metadata <span class="flag-tip" tabindex="0" role="note" aria-label="The chart is drawn automatically from metadata TA-Lib provides. Your app can get the same through the Abstract API." data-tip="The chart is drawn automatically from metadata TA-Lib provides. Your app can get the same through the Abstract API.">i</span>

**Outputs**

* `outReal`: `line` · `zero` · `positive` · `negative`

:::

## Formula

PPO = ((fastMA(inReal) - slowMA(inReal)) / slowMA(inReal)) \* 100, both MAs of type optInMAType; output = 0 when slowMA == 0

The standard form is exponential with periods 12 and 26 — ((12-day EMA - 26-day EMA) / 26-day EMA) \* 100, i.e. the MACD oscillator expressed as a percentage. `optInMAType` therefore **defaults to EMA** — the moving average Gerald Appel used for the original PPO/MACD; pass another type (e.g. `TA_MAType_SMA`) to override.

## Notes

* **The input is assumed positive.** The slow moving average is the divisor. Near zero the divisor is small and the output very large; no error is returned. A series that reaches or crosses zero, such as an oscillator or a spread, has no meaningful result here.
* `optInMAType` applies to both the fast and slow moving average. `TA_MAType_MAMA` ignores its period argument, so with `optInMAType = TA_MAType_MAMA` the fast and slow MAs are identical, making the numerator — and therefore the output — zero at every bar.
* If the slow period is set smaller than the fast period, the two are swapped, as in `MACD`.

## Inputs

* `inReal` — Input data series

## Outputs

* `outReal` — PPO value in percent

## Parameters

| Parameter | Type | Default | Accepted values | Description |
| --- | --- | --- | --- | --- |
| `optInFastPeriod` | integer | 12 | 2–100000 | Period of the fast MA |
| `optInSlowPeriod` | integer | 26 | 2–100000 | Period of the slow MA |
| `optInMAType` | MAType | EMA (1) | any MAType | Moving average type used for both MAs |

*`MAType` values: 0 SMA · 1 EMA · 2 WMA · 3 DEMA · 4 TEMA · 5 TRIMA · 6 KAMA · 7 MAMA · 8 T3 · 9 HMA · 10 DISABLED · 11 DEFAULT · 12 ZLEMA · 13 RMA · 14 VIDYA · 15 ALMA*

## Properties

**Numerical Stability:** [Depends on MA Type](/functions/stability.md#depends-on-ma-type) — This function's default, EMA, has an initial unstable period.

<div class="flag-cols">

<div class="flag-table">

|  |
| :-- |
| <span class="flag-box">☐</span> <span style="opacity:0.5">Overlap Input</span> |
| <span class="flag-box">✅</span> **Independent Y-Axis** <span class="flag-tip" tabindex="0" role="note" aria-label="Output is on its own scale, drawn in a separate pane below the price chart." data-tip="Output is on its own scale, drawn in a separate pane below the price chart.">i</span> |
| <span class="flag-box">☐</span> <span style="opacity:0.5">Candlestick</span> |
| <span class="flag-box">☐</span> <span style="opacity:0.5">Display Shift</span> |

</div>

<div class="flag-table">

|  |
| :-- |
| <span class="flag-box">☐</span> <span style="opacity:0.5">Can Output NaN or ±Inf</span> |
| <span class="flag-box">☐</span> <span style="opacity:0.5">Identity at Period 1</span> |
| <span class="flag-box">☐</span> <span style="opacity:0.5">Uses Transcendental</span> |
| <span class="flag-box">✅</span> **Input Must Be Positive** <span class="flag-tip" tabindex="0" role="note" aria-label="Assumes the input is positive, as a price is. A series that reaches or crosses zero, such as an oscillator or a spread, returns success and an output that may have no meaning. See Notes for what a zero or a negative value does." data-tip="Assumes the input is positive, as a price is. A series that reaches or crosses zero, such as an oscillator or a spread, returns success and an output that may have no meaning. See Notes for what a zero or a negative value does.">i</span> |

</div>

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
TA_PPO( 0, n - 1, series, 12, 26, TA_MAType_EMA, &outBegIdx, &outNBElement, out );
printf( "%d values from bar %d; last out = %f\n", outNBElement, outBegIdx, out[outNBElement - 1] );
```

@tab Rust

```rust
use ta_lib::{Core, MAType};

let series = vec![0.0; 252]; // your data, oldest first
let mut out = vec![0.0; 252];

let core = Core::new();
let range = core.ppo(0, series.len() - 1, &series, 12, 26, MAType::EMA, &mut out)?;
println!("{} values from bar {}; last out = {}", range.count, range.beg_idx, out[range.count - 1]);
```

@tab Java

```java
import io.github.talib.Core;
import io.github.talib.MAType;
import io.github.talib.OutRange;

double[] series = new double[252]; // your data, oldest first
double[] out = new double[252];

Core core = Core.DEFAULT;
OutRange r = core.ppo(0, series.length - 1, series, 12, 26, MAType.EMA, out);
System.out.println(r.count() + " values from bar " + r.begIdx() + "; last out = " + out[r.count() - 1]);
```

@tab C#

```csharp
using System;
using TALib;

var series = new double[252]; // your data, oldest first
var outReal = new double[252];

var core = Core.Default;
OutRange r = core.Ppo(0, series.Length - 1, series, 12, 26, MAType.EMA, outReal);
Console.WriteLine($"{r.Count} values from bar {r.BegIdx}; last outReal = {outReal[r.Count - 1]}");
```

@tab TypeScript

```ts
import { MAType, ppo } from 'ta-lib';

const series = new Float64Array(252); // your data, oldest first

const { begIdx, values } = ppo(series, {
  fastPeriod: 12,
  slowPeriod: 26,
  maType: MAType.EMA,
});
console.log(values.length + ' values from bar ' + begIdx + '; last values = ' + values[values.length - 1]);
```

:::

## Implementation

Added in 0.4.0.

TA-Lib Definition: [`ppo.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/ppo/ppo.c) · [`ppo.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/ppo/ppo.yaml)

| Native | File |
|--------|------|
| C | [`ta_PPO.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_PPO.c) |
| Rust | [`ppo.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/library/src/ta_func/ppo.rs) |
| Java | [`Core_PPO.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/fragments/Core_PPO.java) |
| C# | [`Core_PPO.cs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/csharp/library/src/Core_PPO.cs) |
| TypeScript | [`ppo.ts`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/ts/library/src/func/ppo.ts) |

Python: `talib.PPO`, see [ta-lib-python](https://ta-lib.github.io/ta-lib-python/func_groups/momentum_indicators.html).

TA-Lib is also available for R and more using a [wrapper](/install/#wrappers).

## Aliases

Percentage Price Oscillator

## See Also

[APO](/functions/apo.md) · [MACD](/functions/macd.md) · [MA](/functions/ma.md)

## References

* Gerald Appel, creator of the PPO and MACD (MACD introduced 1979 in his *Systems and Forecasts* newsletter). The PPO is the MACD expressed as a percentage of the slow moving average. Appel's original definition uses **exponential** moving averages (periods 12, 26).
