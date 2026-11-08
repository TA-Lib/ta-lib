---
title: "Parabolic SAR (SAR)"
description: "Wilder's Parabolic SAR (Stop And Reverse): a trailing stop/reverse level that accelerates toward price via an acceleration factor."
---

## Summary

Wilder's Parabolic SAR (Stop And Reverse): a trailing stop/reverse level that accelerates toward price via an acceleration factor. Signals trend direction and trailing exit points. SAR below price = uptrend (long); SAR above price = downtrend (short). Price crossing SAR flips direction.

## Chart

<ta-chart src="/functions/sar.svg" ratio="720:274" label="SAR over IBM, daily, 1999" alt="Drawn on the price pane over candles, with outReal as dots."></ta-chart>

Drawn on the price pane over candles, with outReal as dots. [Reference data (CSV)](/functions/sar.csv) · [Chart (SVG)](/functions/sar.svg)

## Formula

SAR_next = SAR + af * (EP - SAR)
EP = extreme point (highest high in long / lowest low in short); af starts at Acceleration, += Acceleration each new EP, capped at Maximum.
On penetration: reverse, SAR := prior EP, reset af = Acceleration. SAR clamped each bar so it does not penetrate the prior/current bar's range.

## Inputs

- `inHigh` — High price of each bar
- `inLow` — Low price of each bar

## Outputs

- `outReal` — Parabolic SAR stop/reverse level per bar

## Parameters

| Parameter | Type | Default | Accepted values | Description |
| --- | --- | --- | --- | --- |
| `optInAcceleration` | real | 0.02 | ≥ 0 | Step added to the acceleration factor on each new extreme point |
| `optInMaximum` | real | 0.2 | ≥ 0 | Ceiling on the acceleration factor |

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
TA_SAR( 0, n - 1, high, low, 0.02, 0.2, &outBegIdx, &outNBElement, out );
printf( "%d values from bar %d; last out = %f\n", outNBElement, outBegIdx, out[outNBElement - 1] );
```

@tab Rust

```rust
use ta_lib::Core;

let high = vec![0.0; 252]; // your data, oldest first
let low = vec![0.0; 252]; // your data, oldest first
let mut out = vec![0.0; 252];

let core = Core::new();
let range = core.sar(0, high.len() - 1, &high, &low, 0.02, 0.2, &mut out)?;
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
OutRange r = core.sar(0, high.length - 1, high, low, 0.02, 0.2, out);
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
OutRange r = core.Sar(0, high.Length - 1, high, low, 0.02, 0.2, outReal);
Console.WriteLine($"{r.Count} values from bar {r.BegIdx}; last outReal = {outReal[r.Count - 1]}");
```

@tab TypeScript

```ts
import { sar } from 'ta-lib';

const high = new Float64Array(252); // your data, oldest first
const low = new Float64Array(252); // your data, oldest first

const { begIdx, values } = sar({ high, low }, {
  acceleration: 0.02,
  maximum: 0.2,
});
console.log(values.length + ' values from bar ' + begIdx + '; last values = ' + values[values.length - 1]);
```

:::

## Implementation

Added in 0.4.0.

TA-Lib Definition: [`sar.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/sar/sar.c) · [`sar.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/sar/sar.yaml)

| Native | File |
|--------|------|
| C | [`ta_SAR.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_SAR.c) |
| Rust | [`sar.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/library/src/ta_func/sar.rs) |
| Java | [`Core_SAR.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/fragments/Core_SAR.java) |
| C# | [`Core_SAR.cs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/csharp/library/src/Core_SAR.cs) |
| TypeScript | [`sar.ts`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/ts/library/src/func/sar.ts) |

Python: `talib.SAR`, see [ta-lib-python](https://ta-lib.github.io/ta-lib-python/func_groups/overlap_studies.html).

TA-Lib is also available for R and more using a [wrapper](/install/#wrappers).

## Aliases

Parabolic SAR, PSAR, Stop and Reverse

## See Also

[SAREXT](/functions/sarext.md) · [MINUS_DM](/functions/minus_dm.md) · [PLUS_DM](/functions/plus_dm.md)

## References

- J. Welles Wilder, *New Concepts in Technical Trading Systems*, Trend Research (ISBN 0894590278)
