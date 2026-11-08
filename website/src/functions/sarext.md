---
title: "Parabolic SAR - Extended (SAREXT)"
description: "Extended Parabolic SAR (stop and reverse) giving the caller full control over the initial state and separate acceleration factors for long and short…"
---

## Summary

Extended Parabolic SAR (stop and reverse) giving the caller full control over the initial state and separate acceleration factors for long and short positions. Unlike SAR, it returns negative values while short so reversals are distinguishable. Sign flip of the output marks a trend reversal (positive=long stop, negative=short stop).

## Chart

<ta-chart src="/functions/sarext.svg" ratio="720:274" label="SAREXT over IBM, daily, 1999" alt="Drawn on the price pane over candles, with outReal as dots."></ta-chart>

::: details Chart metadata <span class="flag-tip" tabindex="0" role="note" aria-label="The chart is drawn from the function's metadata: the flags and attributes the abstract API reports for it and for each output, and the input series its reference run declares." data-tip="The chart is drawn from the function's metadata: the flags and attributes the abstract API reports for it and for each output, and the input series its reference run declares.">i</span>

Drawn on the price pane over candles, with outReal as dots.

Metadata flags used: `overlap` · `inPriceHL: high, low` · `outReal: dot`

:::

[Reference data (CSV)](/functions/sarext.csv) · [Chart (SVG)](/functions/sarext.svg)

## Formula

SAR_next = SAR + AF*(EP - SAR), then clamped within the prior and current bar's range. On penetration, reverse: set SAR=EP (clamped), reset AF to its Init value, EP=extreme of the new direction. Output is +SAR when long, -SAR when short. On reversal an optional offset is applied: long->short SAR*(1+offset), short->long SAR*(1-offset).

## Inputs

- `inHigh` — High price of each bar
- `inLow` — Low price of each bar

## Outputs

- `outReal` — SAR stop level; positive while long, negative while short

## Parameters

| Parameter | Type | Default | Accepted values | Description |
| --- | --- | --- | --- | --- |
| `optInStartValue` | real | 0 | any real | Initial SAR/direction: 0 auto, >0 start long at value, <0 start short at -value |
| `optInOffsetOnReverse` | real | 0 | ≥ 0 | Fractional offset applied to the stop on each reversal |
| `optInAccelerationInitLong` | real | 0.02 | ≥ 0 | Initial acceleration factor when long |
| `optInAccelerationLong` | real | 0.02 | ≥ 0 | AF increment per new long extreme |
| `optInAccelerationMaxLong` | real | 0.2 | ≥ 0 | Cap on the long acceleration factor |
| `optInAccelerationInitShort` | real | 0.02 | ≥ 0 | Initial acceleration factor when short |
| `optInAccelerationShort` | real | 0.02 | ≥ 0 | AF increment per new short extreme |
| `optInAccelerationMaxShort` | real | 0.2 | ≥ 0 | Cap on the short acceleration factor |

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
TA_SAREXT( 0, n - 1, high, low, 0.0, 0.0, 0.02, 0.02, 0.2, 0.02, 0.02, 0.2,
           &outBegIdx, &outNBElement, out );
printf( "%d values from bar %d; last out = %f\n", outNBElement, outBegIdx, out[outNBElement - 1] );
```

@tab Rust

```rust
use ta_lib::Core;

let high = vec![0.0; 252]; // your data, oldest first
let low = vec![0.0; 252]; // your data, oldest first
let mut out = vec![0.0; 252];

let core = Core::new();
let range = core.sarext(0, high.len() - 1, &high, &low, 0.0, 0.0, 0.02, 0.02, 0.2, 0.02, 0.02, 0.2,
                        &mut out)?;
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
OutRange r = core.sarext(0, high.length - 1, high, low, 0.0, 0.0, 0.02, 0.02, 0.2, 0.02, 0.02, 0.2,
                         out);
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
OutRange r = core.Sarext(0, high.Length - 1, high, low, 0.0, 0.0, 0.02, 0.02, 0.2, 0.02, 0.02, 0.2,
                         outReal);
Console.WriteLine($"{r.Count} values from bar {r.BegIdx}; last outReal = {outReal[r.Count - 1]}");
```

@tab TypeScript

```ts
import { sarext } from 'ta-lib';

const high = new Float64Array(252); // your data, oldest first
const low = new Float64Array(252); // your data, oldest first

const { begIdx, values } = sarext({ high, low }, {
  startValue: 0,
  offsetOnReverse: 0,
  accelerationInitLong: 0.02,
  accelerationLong: 0.02,
  accelerationMaxLong: 0.2,
  accelerationInitShort: 0.02,
  accelerationShort: 0.02,
  accelerationMaxShort: 0.2,
});
console.log(values.length + ' values from bar ' + begIdx + '; last values = ' + values[values.length - 1]);
```

:::

## Implementation

Added in 0.4.0.

TA-Lib Definition: [`sarext.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/sarext/sarext.c) · [`sarext.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/sarext/sarext.yaml)

| Native | File |
|--------|------|
| C | [`ta_SAREXT.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_SAREXT.c) |
| Rust | [`sarext.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/library/src/ta_func/sarext.rs) |
| Java | [`Core_SAREXT.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/fragments/Core_SAREXT.java) |
| C# | [`Core_SAREXT.cs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/csharp/library/src/Core_SAREXT.cs) |
| TypeScript | [`sarext.ts`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/ts/library/src/func/sarext.ts) |

Python: `talib.SAREXT`, see [ta-lib-python](https://ta-lib.github.io/ta-lib-python/func_groups/overlap_studies.html).

TA-Lib is also available for R and more using a [wrapper](/install/#wrappers).

## Aliases

Parabolic SAR Extended, Extended Parabolic Stop and Reverse

## See Also

[SAR](/functions/sar.md) · [MINUS_DM](/functions/minus_dm.md)
