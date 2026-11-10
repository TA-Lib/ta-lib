---
url: 'https://ta-lib.org/functions/adosc.md'
description: >-
  Chaikin A/D Oscillator: the difference between a fast and a slow EMA of the
  Accumulation/Distribution line.
---
# Chaikin A/D Oscillator (ADOSC)

## Summary

Chaikin A/D Oscillator: the difference between a fast and a slow EMA of the Accumulation/Distribution line. Highlights momentum in accumulation/distribution volume flow. Positive/rising suggests accumulation; negative/falling suggests distribution.

## Chart

<ta-chart src="/functions/adosc.svg" ratio="720:484" label="ADOSC over IBM, daily, 1999" alt="Drawn on the price pane over candles; below it, a separate indicator pane with a zero line showing outReal as a solid line, then a volume pane."></ta-chart>

<div class="ta-chart-links">

[Reference data (CSV)](/functions/adosc.csv) · [Chart (SVG)](/functions/adosc.svg)

</div>

::: details Chart metadata <span class="flag-tip" tabindex="0" role="note" aria-label="The chart is drawn automatically from metadata TA-Lib provides. Your app can get the same through the Abstract API." data-tip="The chart is drawn automatically from metadata TA-Lib provides. Your app can get the same through the Abstract API.">i</span>

**Outputs**

* `outReal`: `line` · `zero` · `positive` · `negative`

:::

## Formula

ad += ((close-low)-(high-close))/(high-low) \* volume   (only when high>low)
fastEMA = fastk\*ad + (1-fastk)*fastEMA,  fastk = 2/(optInFastPeriod+1)
slowEMA = slowk*ad + (1-slowk)\*slowEMA,  slowk = 2/(optInSlowPeriod+1)
ADOSC = fastEMA - slowEMA

## Inputs

* `inHigh` — High price of each bar
* `inLow` — Low price of each bar
* `inClose` — Close price of each bar
* `inVolume` — Volume of each bar

## Outputs

* `outReal` — Fast-EMA minus slow-EMA of the A/D line

## Parameters

| Parameter | Type | Default | Accepted values | Description |
| --- | --- | --- | --- | --- |
| `optInFastPeriod` | integer | 3 | 2–100000 | Period of the fast A/D EMA |
| `optInSlowPeriod` | integer | 10 | 2–100000 | Period of the slow A/D EMA |

## Properties

**Numerical Stability:** [Initial Unstable Period](/functions/stability.md#initial-unstable-period) — Inherited from EMA, which ADOSC computes internally; tunable via EMA's unstable period.

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
| <span class="flag-box">☐</span> <span style="opacity:0.5">Input Must Be Positive</span> |

</div>

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
double volume[252]; /* your data, oldest first */
double out[252];
int outBegIdx, outNBElement;

TA_Initialize();
TA_ADOSC( 0, n - 1, high, low, close, volume, 3, 10, &outBegIdx, &outNBElement, out );
printf( "%d values from bar %d; last out = %f\n", outNBElement, outBegIdx, out[outNBElement - 1] );
```

@tab Rust

```rust
use ta_lib::Core;

let high = vec![0.0; 252]; // your data, oldest first
let low = vec![0.0; 252]; // your data, oldest first
let close = vec![0.0; 252]; // your data, oldest first
let volume = vec![0.0; 252]; // your data, oldest first
let mut out = vec![0.0; 252];

let core = Core::new();
let range = core.adosc(0, high.len() - 1, &high, &low, &close, &volume, 3, 10, &mut out)?;
println!("{} values from bar {}; last out = {}", range.count, range.beg_idx, out[range.count - 1]);
```

@tab Java

```java
import io.github.talib.Core;
import io.github.talib.OutRange;

double[] high = new double[252]; // your data, oldest first
double[] low = new double[252]; // your data, oldest first
double[] close = new double[252]; // your data, oldest first
double[] volume = new double[252]; // your data, oldest first
double[] out = new double[252];

Core core = Core.DEFAULT;
OutRange r = core.adosc(0, high.length - 1, high, low, close, volume, 3, 10, out);
System.out.println(r.count() + " values from bar " + r.begIdx() + "; last out = " + out[r.count() - 1]);
```

@tab C#

```csharp
using System;
using TALib;

var high = new double[252]; // your data, oldest first
var low = new double[252]; // your data, oldest first
var close = new double[252]; // your data, oldest first
var volume = new double[252]; // your data, oldest first
var outReal = new double[252];

var core = Core.Default;
OutRange r = core.Adosc(0, high.Length - 1, high, low, close, volume, 3, 10, outReal);
Console.WriteLine($"{r.Count} values from bar {r.BegIdx}; last outReal = {outReal[r.Count - 1]}");
```

@tab TypeScript

```ts
import { adosc } from 'ta-lib';

const high = new Float64Array(252); // your data, oldest first
const low = new Float64Array(252); // your data, oldest first
const close = new Float64Array(252); // your data, oldest first
const volume = new Float64Array(252); // your data, oldest first

const { begIdx, values } = adosc({ high, low, close, volume }, {
  fastPeriod: 3,
  slowPeriod: 10,
});
console.log(values.length + ' values from bar ' + begIdx + '; last values = ' + values[values.length - 1]);
```

:::

## Implementation

Added in 0.4.0.

TA-Lib Definition: [`adosc.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/adosc/adosc.c) · [`adosc.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/adosc/adosc.yaml)

| Native | File |
|--------|------|
| C | [`ta_ADOSC.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_ADOSC.c) |
| Rust | [`adosc.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/library/src/ta_func/adosc.rs) |
| Java | [`Core_ADOSC.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/fragments/Core_ADOSC.java) |
| C# | [`Core_ADOSC.cs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/csharp/library/src/Core_ADOSC.cs) |
| TypeScript | [`adosc.ts`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/ts/library/src/func/adosc.ts) |

Python: `talib.ADOSC`, see [ta-lib-python](https://ta-lib.github.io/ta-lib-python/func_groups/volume_indicators.html).

TA-Lib is also available for R and more using a [wrapper](/install/#wrappers).

## Aliases

Chaikin A/D Oscillator, Chaikin Oscillator

## See Also

[AD](/functions/ad.md) · [EMA](/functions/ema.md)

## References

* Marc Chaikin
