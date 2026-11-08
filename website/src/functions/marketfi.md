---
title: "Market Facilitation Index (MARKETFI)"
description: "Bill Williams' Market Facilitation Index (Trading Chaos, 1995): the price range a bar travelled per unit of volume traded — how much movement the market…"
---

## Summary

Bill Williams' Market Facilitation Index (*Trading Chaos*, 1995): the price range a bar travelled per unit of volume traded — how much movement the market "facilitated" per tick. A rising index on rising volume is read as a move the market is absorbing; a rising index on falling volume as one it is not.

Retail material commonly abbreviates this "MFI" or "BW MFI". TA-Lib already ships `TA_MFI` for the Money Flow Index, so this carries the `MARKETFI` name used by Tulip and pandas-ta-classic.

Charting packages often overlay a four-state colour code (green / fade / fake / squat) derived from the signs of the bar-to-bar change in this index and in volume. That is an interpretive layer on top of the series, not part of it; `outReal` is the scalar only.

## Chart

<ta-chart src="/functions/marketfi.svg" ratio="720:484" label="MARKETFI over IBM, daily, 1999" alt="Drawn on the price pane over candles; below it, a separate indicator pane with a zero line showing outReal as a solid line, then a volume pane."></ta-chart>

Drawn on the price pane over candles; below it, a separate indicator pane with a zero line showing outReal as a solid line, then a volume pane. [Reference data (CSV)](/functions/marketfi.csv) · [Chart (SVG)](/functions/marketfi.svg)

## Formula

MARKETFI_t = (high_t - low_t) / volume_t

A bar with zero volume reports 0 rather than dividing: it facilitated no movement, and a successful call never emits NaN or ±Inf.

## Inputs

- `inHigh` — High price of each bar
- `inLow` — Low price of each bar
- `inVolume` — Volume of each bar

## Outputs

- `outReal` — Range travelled per unit of volume, per bar

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
double volume[252]; /* your data, oldest first */
double out[252];
int outBegIdx, outNBElement;

TA_Initialize();
TA_MARKETFI( 0, n - 1, high, low, volume, &outBegIdx, &outNBElement, out );
printf( "%d values from bar %d; last out = %f\n", outNBElement, outBegIdx, out[outNBElement - 1] );
```

@tab Rust

```rust
use ta_lib::Core;

let high = vec![0.0; 252]; // your data, oldest first
let low = vec![0.0; 252]; // your data, oldest first
let volume = vec![0.0; 252]; // your data, oldest first
let mut out = vec![0.0; 252];

let core = Core::new();
let range = core.marketfi(0, high.len() - 1, &high, &low, &volume, &mut out)?;
println!("{} values from bar {}; last out = {}", range.count, range.beg_idx, out[range.count - 1]);
```

@tab Java

```java
import io.github.talib.Core;
import io.github.talib.OutRange;

double[] high = new double[252]; // your data, oldest first
double[] low = new double[252]; // your data, oldest first
double[] volume = new double[252]; // your data, oldest first
double[] out = new double[252];

Core core = Core.DEFAULT;
OutRange r = core.marketfi(0, high.length - 1, high, low, volume, out);
System.out.println(r.count() + " values from bar " + r.begIdx() + "; last out = " + out[r.count() - 1]);
```

@tab C\#

```csharp
using System;
using TALib;

var high = new double[252]; // your data, oldest first
var low = new double[252]; // your data, oldest first
var volume = new double[252]; // your data, oldest first
var outReal = new double[252];

var core = Core.Default;
OutRange r = core.Marketfi(0, high.Length - 1, high, low, volume, outReal);
Console.WriteLine($"{r.Count} values from bar {r.BegIdx}; last outReal = {outReal[r.Count - 1]}");
```

@tab TypeScript

```ts
import { marketfi } from 'ta-lib';

const high = new Float64Array(252); // your data, oldest first
const low = new Float64Array(252); // your data, oldest first
const volume = new Float64Array(252); // your data, oldest first

const { begIdx, values } = marketfi({ high, low, volume });
console.log(values.length + ' values from bar ' + begIdx + '; last values = ' + values[values.length - 1]);
```

:::

## Implementation

Added in 0.8.1 (ta-lib-python 0.8.1).

TA-Lib Definition: [`marketfi.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/marketfi/marketfi.c) · [`marketfi.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/marketfi/marketfi.yaml)

| Native | File |
|--------|------|
| C | [`ta_MARKETFI.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_MARKETFI.c) |
| Rust | [`marketfi.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/library/src/ta_func/marketfi.rs) |
| Java | [`Core_MARKETFI.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/fragments/Core_MARKETFI.java) |
| C# | [`Core_MARKETFI.cs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/csharp/library/src/Core_MARKETFI.cs) |
| TypeScript | [`marketfi.ts`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/ts/library/src/func/marketfi.ts) |

Python: `talib.MARKETFI` (ta-lib-python 0.8.1 or later), see [ta-lib-python](https://ta-lib.github.io/ta-lib-python/func_groups/volume_indicators.html).

TA-Lib is also available for R and more using a [wrapper](/install/#wrappers).

## See Also

[AD](/functions/ad.md) · [ADOSC](/functions/adosc.md) · [NVI](/functions/nvi.md) · [OBV](/functions/obv.md) · [PVI](/functions/pvi.md)

## References

- Bill Williams, *Trading Chaos*, 1995, introduces the Market Facilitation Index as the price range a bar travelled per unit of volume.
- Tulip Indicators `ti_marketfi` and pandas-ta-classic `marketfi` compute the same form, but divide unconditionally: both emit ±Inf on a zero-volume bar where this returns 0.
- The four-state green/fade/fake/squat colour code charting packages overlay is derived from the signs of the bar-to-bar change in this index and in volume. It is an interpretive layer, not part of the series; `outReal` is the scalar only.
