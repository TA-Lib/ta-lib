---
title: "Elder's Force Index (EFI)"
description: "Alexander Elder's Force Index (Trading for a Living, 1993): volume-weighted momentum."
---

## Summary

Alexander Elder's Force Index (*Trading for a Living*, 1993): volume-weighted momentum. Each bar's close-to-close move is weighted by that bar's volume, and the result is smoothed with an exponential moving average. The sign is the direction of the move; the size combines how far price travelled with how much volume stood behind it.

Elder reads two settings — 2 for the short term, which he pairs with a 22-period EMA of price to mark corrections against an established trend, and 13 for the intermediate term, the default here. A divergence against price can be confirmed by a zero-line cross. Beyond Elder, much longer settings are also in use, 100 or so, for the longer-term balance between buyers and sellers. Nothing normalises the result, so it scales with the instrument's own volume: read its sign and its shape over time, not its level against another instrument.

## Chart

<ta-chart src="/functions/efi.svg" ratio="720:484" label="EFI over IBM, daily, 1999" alt="Drawn on the price pane over the close as a line; below it, a separate indicator pane with a zero line showing outReal as a solid line, then a volume pane."></ta-chart>

::: details Chart metadata <span class="flag-tip" tabindex="0" role="note" aria-label="The chart is drawn automatically from metadata TA-Lib provides. Your app can get the same through the Abstract API." data-tip="The chart is drawn automatically from metadata TA-Lib provides. Your app can get the same through the Abstract API.">i</span>

Drawn on the price pane over the close as a line; below it, a separate indicator pane with a zero line showing outReal as a solid line, then a volume pane.

Metadata flags used: `inPriceCV: close, volume` · `outReal: line, positive, negative`

:::

[Reference data (CSV)](/functions/efi.csv) · [Chart (SVG)](/functions/efi.svg)

## Formula

force_t = ( close_t - close_{t-1} ) * volume_t; EFI = EMA( force, optInTimePeriod )

The EMA is TA-Lib's, seeded with a simple average of the first `optInTimePeriod` force values. A period of 1 leaves the raw one-bar Force Index.

## Inputs

- `inClose` — Close price of each bar
- `inVolume` — Volume of each bar

## Outputs

- `outReal` — Smoothed force

## Parameters

| Parameter | Type | Default | Accepted values | Description |
| --- | --- | --- | --- | --- |
| `optInTimePeriod` | integer | 13 | 1–100000 | EMA period applied to the force series |

## Properties

**Numerical Stability:** [Initial Unstable Period](/functions/stability.md#initial-unstable-period) — Inherited from EMA, which EFI computes internally; tunable via EMA's unstable period.

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
double close[252]; /* your data, oldest first */
double volume[252]; /* your data, oldest first */
double out[252];
int outBegIdx, outNBElement;

TA_Initialize();
TA_EFI( 0, n - 1, close, volume, 13, &outBegIdx, &outNBElement, out );
printf( "%d values from bar %d; last out = %f\n", outNBElement, outBegIdx, out[outNBElement - 1] );
```

@tab Rust

```rust
use ta_lib::Core;

let close = vec![0.0; 252]; // your data, oldest first
let volume = vec![0.0; 252]; // your data, oldest first
let mut out = vec![0.0; 252];

let core = Core::new();
let range = core.efi(0, close.len() - 1, &close, &volume, 13, &mut out)?;
println!("{} values from bar {}; last out = {}", range.count, range.beg_idx, out[range.count - 1]);
```

@tab Java

```java
import io.github.talib.Core;
import io.github.talib.OutRange;

double[] close = new double[252]; // your data, oldest first
double[] volume = new double[252]; // your data, oldest first
double[] out = new double[252];

Core core = Core.DEFAULT;
OutRange r = core.efi(0, close.length - 1, close, volume, 13, out);
System.out.println(r.count() + " values from bar " + r.begIdx() + "; last out = " + out[r.count() - 1]);
```

@tab C\#

```csharp
using System;
using TALib;

var close = new double[252]; // your data, oldest first
var volume = new double[252]; // your data, oldest first
var outReal = new double[252];

var core = Core.Default;
OutRange r = core.Efi(0, close.Length - 1, close, volume, 13, outReal);
Console.WriteLine($"{r.Count} values from bar {r.BegIdx}; last outReal = {outReal[r.Count - 1]}");
```

@tab TypeScript

```ts
import { efi } from 'ta-lib';

const close = new Float64Array(252); // your data, oldest first
const volume = new Float64Array(252); // your data, oldest first

const { begIdx, values } = efi({ close, volume }, { timePeriod: 13 });
console.log(values.length + ' values from bar ' + begIdx + '; last values = ' + values[values.length - 1]);
```

:::

## Implementation

Added in 0.8.1 (ta-lib-python 0.8.1).

TA-Lib Definition: [`efi.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/efi/efi.c) · [`efi.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/efi/efi.yaml)

| Native | File |
|--------|------|
| C | [`ta_EFI.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_EFI.c) |
| Rust | [`efi.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/library/src/ta_func/efi.rs) |
| Java | [`Core_EFI.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/fragments/Core_EFI.java) |
| C# | [`Core_EFI.cs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/csharp/library/src/Core_EFI.cs) |
| TypeScript | [`efi.ts`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/ts/library/src/func/efi.ts) |

Python: `talib.EFI` (ta-lib-python 0.8.1 or later), see [ta-lib-python](https://ta-lib.github.io/ta-lib-python/func_groups/volume_indicators.html).

TA-Lib is also available for R and more using a [wrapper](/install/#wrappers).

## See Also

[AD](/functions/ad.md) · [EMA](/functions/ema.md) · [MFI](/functions/mfi.md) · [OBV](/functions/obv.md) · [PVO](/functions/pvo.md)

## References

- Alexander Elder, *Trading for a Living*, Wiley, 1993, introduces the Force Index and the 2-period and 13-period smoothing he reads as short- and intermediate-term.
- StockCharts ChartSchool, *Force Index*: `Force Index(1) = {Close - Close prior} x Volume`, `Force Index(13) = 13-period EMA of Force Index(1)`.
- MotiveWave, *Elder's Force Index*: `rawForce = vol * (price - prevP)`, smoothed by a moving average whose default method is EMA, at 2 and 13. No competing formula was found.
