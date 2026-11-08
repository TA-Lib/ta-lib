---
title: "Percentage Volume Oscillator (PVO)"
description: "Percentage Volume Oscillator: a variation of the Percentage Price Oscillator (PPO, created by Gerald Appel) applied to the volume series instead of price."
---

## Summary

Percentage Volume Oscillator: a variation of the [Percentage Price Oscillator](/functions/ppo.md) (PPO, created by Gerald Appel) applied to the **volume** series instead of price. It is the difference between a fast and slow moving average of volume, expressed as a percentage of the slow MA. Positive when short-term volume is above its longer-term average (rising participation), negative when below. The default periods (12, 26) match MACD and PPO.

## Chart

<ta-chart src="/functions/pvo.svg" ratio="720:484" label="PVO over IBM, daily, 1999" alt="Drawn on the price pane over the close as a line; below it, a separate indicator pane with a zero line showing outReal as a solid line, then a volume pane."></ta-chart>

Drawn on the price pane over the close as a line; below it, a separate indicator pane with a zero line showing outReal as a solid line, then a volume pane. [Reference data (CSV)](/functions/pvo.csv) · [Chart (SVG)](/functions/pvo.svg)

## Formula

PVO = ((fastMA(inVolume) - slowMA(inVolume)) / slowMA(inVolume)) * 100, both MAs of type optInMAType; output = 0 when slowMA == 0

The standard form is exponential with periods 12 and 26 — ((12-day EMA of Volume - 26-day EMA of Volume) / 26-day EMA of Volume) * 100, i.e. the PPO/MACD oscillator computed on volume. `optInMAType` therefore **defaults to EMA** — the moving average Gerald Appel used for the original PPO/MACD; pass another type (e.g. `TA_MAType_SMA`) to override.

## Notes

- `optInMAType` applies to both the fast and slow moving average. `TA_MAType_MAMA` ignores its period argument, so with `optInMAType = TA_MAType_MAMA` the fast and slow MAs are identical, making the numerator — and therefore the output — zero at every bar.
- If the slow period is set smaller than the fast period, the two are swapped, as in `MACD`.

## Inputs

- `inVolume` — Volume of each bar

## Outputs

- `outReal` — PVO value in percent

## Parameters

| Parameter | Type | Default | Accepted values | Description |
| --- | --- | --- | --- | --- |
| `optInFastPeriod` | integer | 12 | 2–100000 | Period of the fast MA |
| `optInSlowPeriod` | integer | 26 | 2–100000 | Period of the slow MA |
| `optInMAType` | MAType | EMA (1) | any MAType | Moving average type used for both MAs |

*`MAType` values: 0 SMA · 1 EMA · 2 WMA · 3 DEMA · 4 TEMA · 5 TRIMA · 6 KAMA · 7 MAMA · 8 T3 · 9 HMA · 10 DISABLED · 11 DEFAULT · 12 ZLEMA · 13 RMA · 14 VIDYA · 15 ALMA*

## Properties

**Numerical Stability:** [Depends on MA Type](/functions/stability.md#depends-on-ma-type) — This function's default, EMA, has an initial unstable period.

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
double volume[252]; /* your data, oldest first */
double out[252];
int outBegIdx, outNBElement;

TA_Initialize();
TA_PVO( 0, n - 1, volume, 12, 26, TA_MAType_EMA, &outBegIdx, &outNBElement, out );
printf( "%d values from bar %d; last out = %f\n", outNBElement, outBegIdx, out[outNBElement - 1] );
```

@tab Rust

```rust
use ta_lib::{Core, MAType};

let volume = vec![0.0; 252]; // your data, oldest first
let mut out = vec![0.0; 252];

let core = Core::new();
let range = core.pvo(0, volume.len() - 1, &volume, 12, 26, MAType::EMA, &mut out)?;
println!("{} values from bar {}; last out = {}", range.count, range.beg_idx, out[range.count - 1]);
```

@tab Java

```java
import io.github.talib.Core;
import io.github.talib.MAType;
import io.github.talib.OutRange;

double[] volume = new double[252]; // your data, oldest first
double[] out = new double[252];

Core core = Core.DEFAULT;
OutRange r = core.pvo(0, volume.length - 1, volume, 12, 26, MAType.EMA, out);
System.out.println(r.count() + " values from bar " + r.begIdx() + "; last out = " + out[r.count() - 1]);
```

@tab C\#

```csharp
using System;
using TALib;

var volume = new double[252]; // your data, oldest first
var outReal = new double[252];

var core = Core.Default;
OutRange r = core.Pvo(0, volume.Length - 1, volume, 12, 26, MAType.EMA, outReal);
Console.WriteLine($"{r.Count} values from bar {r.BegIdx}; last outReal = {outReal[r.Count - 1]}");
```

@tab TypeScript

```ts
import { MAType, pvo } from 'ta-lib';

const volume = new Float64Array(252); // your data, oldest first

const { begIdx, values } = pvo({ volume }, {
  fastPeriod: 12,
  slowPeriod: 26,
  maType: MAType.EMA,
});
console.log(values.length + ' values from bar ' + begIdx + '; last values = ' + values[values.length - 1]);
```

:::

## Implementation

Added in 0.8.1 (ta-lib-python 0.8.1).

TA-Lib Definition: [`pvo.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/pvo/pvo.c) · [`pvo.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/pvo/pvo.yaml)

| Native | File |
|--------|------|
| C | [`ta_PVO.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_PVO.c) |
| Rust | [`pvo.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/library/src/ta_func/pvo.rs) |
| Java | [`Core_PVO.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/fragments/Core_PVO.java) |
| C# | [`Core_PVO.cs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/csharp/library/src/Core_PVO.cs) |
| TypeScript | [`pvo.ts`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/ts/library/src/func/pvo.ts) |

Python: `talib.PVO` (ta-lib-python 0.8.1 or later), see [ta-lib-python](https://ta-lib.github.io/ta-lib-python/func_groups/volume_indicators.html).

TA-Lib is also available for R and more using a [wrapper](/install/#wrappers).

## Aliases

Percentage Volume Oscillator

## See Also

[PPO](/functions/ppo.md) · [OBV](/functions/obv.md) · [MACD](/functions/macd.md)

## References

- PVO has no separately documented originator; it applies the PPO/MACD oscillator (Gerald Appel) to the volume series.
- Formula and standard (12, 26, 9) parameters: [Percentage Volume Oscillator (PVO)](https://chartschool.stockcharts.com/table-of-contents/technical-indicators-and-overlays/technical-indicators/percentage-volume-oscillator-pvo), StockCharts ChartSchool; also documented by [TradingView](https://www.tradingview.com/support/solutions/43000591350-percentage-volume-oscillator-pvo/).
