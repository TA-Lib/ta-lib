---
title: "Accelerator/Decelerator Oscillator (AC)"
description: "Bill Williams' Accelerator/Decelerator Oscillator (New Trading Dimensions, 1998): the rate at which market momentum is itself speeding up or slowing down."
---

## Summary

Bill Williams' Accelerator/Decelerator Oscillator (*New Trading Dimensions*, 1998): the rate at which market momentum is itself speeding up or slowing down. Where the Awesome Oscillator ([`AO`](/functions/ao.md)) measures momentum, this measures the change in that momentum, by taking the oscillator's distance above or below its own moving average.

Because acceleration turns before speed does, the reading changes sign ahead of the oscillator it is built from — it is meant as the early half of a pair, not as a signal on its own. Above zero acceleration is with the bulls, below zero with the bears, and it is drawn as a zero-centred histogram whose colour convention is the bar-to-bar change: rising bars accelerating, falling bars decelerating. Williams' rule of thumb is that two same-coloured bars are what confirms the turn, which is why the sign and the direction matter more than the level.

The oscillator is one leg of Williams' Profitunity system, alongside the Awesome Oscillator ([`AO`](/functions/ao.md)) and the Alligator.

## Chart

<ta-chart src="/functions/ac.svg" ratio="720:416" label="AC over IBM, daily, 1999" alt="Drawn on the price pane over candles; below it, a separate indicator pane with a zero line showing outReal as a sign-coloured histogram."></ta-chart>

::: details Chart metadata <span class="flag-tip" tabindex="0" role="note" aria-label="The chart is drawn from the function's metadata: the flags and attributes the abstract API reports for it and for each output, and the input series its reference run declares." data-tip="The chart is drawn from the function's metadata: the flags and attributes the abstract API reports for it and for each output, and the input series its reference run declares.">i</span>

Drawn on the price pane over candles; below it, a separate indicator pane with a zero line showing outReal as a sign-coloured histogram.

Metadata flags used: `inPriceHL: high, low` · `outReal: histogram, positive, negative`

:::

[Reference data (CSV)](/functions/ac.csv) · [Chart (SVG)](/functions/ac.svg)

## Formula

median_t = ( high_t + low_t ) / 2  
AO_t = SMA(median, fast)_t − SMA(median, slow)_t  
AC_t = AO_t − SMA(AO, signal)_t

## Inputs

- `inHigh` — High price of each bar
- `inLow` — Low price of each bar

## Outputs

- `outReal` — Distance of the Awesome Oscillator ([`AO`](/functions/ao.md)) from its own moving average, centred on zero

## Parameters

| Parameter | Type | Default | Accepted values | Description |
| --- | --- | --- | --- | --- |
| `optInFastPeriod` | integer | 5 | 2–100000 | Number of bars in the short moving average of the median price. |
| `optInSlowPeriod` | integer | 34 | 2–100000 | Number of bars in the long moving average of the median price. |
| `optInSignalPeriod` | integer | 5 | 2–100000 | Number of bars in the moving average taken over the oscillator. |

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
double out[252];
int outBegIdx, outNBElement;

TA_Initialize();
TA_AC( 0, n - 1, high, low, 5, 34, 5, &outBegIdx, &outNBElement, out );
printf( "%d values from bar %d; last out = %f\n", outNBElement, outBegIdx, out[outNBElement - 1] );
```

@tab Rust

```rust
use ta_lib::Core;

let high = vec![0.0; 252]; // your data, oldest first
let low = vec![0.0; 252]; // your data, oldest first
let mut out = vec![0.0; 252];

let core = Core::new();
let range = core.ac(0, high.len() - 1, &high, &low, 5, 34, 5, &mut out)?;
println!("{} values from bar {}; last out = {}", range.count, range.beg_idx, out[range.count - 1]);
```

@tab Java

```java
import io.github.talib.Core;
import io.github.talib.OutRange;

double[] high = new double[252]; // your data, oldest first
double[] low = new double[252]; // your data, oldest first
double[] out = new double[252];

Core core = Core.DEFAULT;
OutRange r = core.ac(0, high.length - 1, high, low, 5, 34, 5, out);
System.out.println(r.count() + " values from bar " + r.begIdx() + "; last out = " + out[r.count() - 1]);
```

@tab C\#

```csharp
using System;
using TALib;

var high = new double[252]; // your data, oldest first
var low = new double[252]; // your data, oldest first
var outReal = new double[252];

var core = Core.Default;
OutRange r = core.Ac(0, high.Length - 1, high, low, 5, 34, 5, outReal);
Console.WriteLine($"{r.Count} values from bar {r.BegIdx}; last outReal = {outReal[r.Count - 1]}");
```

@tab TypeScript

```ts
import { ac } from 'ta-lib';

const high = new Float64Array(252); // your data, oldest first
const low = new Float64Array(252); // your data, oldest first

const { begIdx, values } = ac({ high, low }, {
  fastPeriod: 5,
  slowPeriod: 34,
  signalPeriod: 5,
});
console.log(values.length + ' values from bar ' + begIdx + '; last values = ' + values[values.length - 1]);
```

:::

## Implementation

Added in 0.8.1 (ta-lib-python 0.8.1).

TA-Lib Definition: [`ac.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/ac/ac.c) · [`ac.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/ac/ac.yaml)

| Native | File |
|--------|------|
| C | [`ta_AC.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_AC.c) |
| Rust | [`ac.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/library/src/ta_func/ac.rs) |
| Java | [`Core_AC.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/fragments/Core_AC.java) |
| C# | [`Core_AC.cs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/csharp/library/src/Core_AC.cs) |
| TypeScript | [`ac.ts`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/ts/library/src/func/ac.ts) |

Python: `talib.AC` (ta-lib-python 0.8.1 or later), see [ta-lib-python](https://ta-lib.github.io/ta-lib-python/func_groups/momentum_indicators.html).

TA-Lib is also available for R and more using a [wrapper](/install/#wrappers).

## Aliases

Accelerator Oscillator, Decelerator Oscillator, Accelerator/Decelerator, Bill Williams Accelerator, AC Oscillator

## See Also

[AO](/functions/ao.md) · [MACD](/functions/macd.md) · [MEDPRICE](/functions/medprice.md) · [PPO](/functions/ppo.md) · [SMA](/functions/sma.md)

## References

- Bill Williams, *New Trading Dimensions*, Wiley, 1998, and *Trading Chaos*, define the Accelerator/Decelerator as the Awesome Oscillator ([`AO`](/functions/ao.md)) less the 5-period simple moving average of that oscillator.
- trading-signals 8.3.0 (`momentum/AC`) computes the same form on the same inputs and reports its first value at the same bar. Its moving average re-sums the stored window on every bar where TA-Lib rolls a running total, so the two agree to rounding rather than to the bit.
