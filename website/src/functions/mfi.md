---
title: "Money Flow Index (MFI)"
description: "Money Flow Index: a volume-weighted momentum oscillator (0-100) comparing positive vs negative money flow over a period."
---

## Summary

Money Flow Index: a volume-weighted momentum oscillator (0-100) comparing positive vs negative money flow over a period. A volume-based analog of RSI. >80 overbought, <20 oversold.

## Chart

<ta-chart src="/functions/mfi.svg" ratio="720:484" label="MFI over IBM, daily, 1999" alt="Drawn on the price pane over candles; below it, a separate indicator pane showing outReal as a solid line, then a volume pane."></ta-chart>

::: details Chart metadata <span class="flag-tip" tabindex="0" role="note" aria-label="The chart is drawn from the function's metadata: the flags the abstract API reports for it and for each output, and the input series its reference run declares." data-tip="The chart is drawn from the function's metadata: the flags the abstract API reports for it and for each output, and the input series its reference run declares.">i</span>

Drawn on the price pane over candles; below it, a separate indicator pane showing outReal as a solid line, then a volume pane.

Metadata flags used: `inPriceHLCV: high, low, close, volume` · `outReal: line`

:::

[Reference data (CSV)](/functions/mfi.csv) · [Chart (SVG)](/functions/mfi.svg)

## Formula

TP = (High+Low+Close)/3; MF = TP*Volume, classed positive if TP>prevTP, negative if TP<prevTP, neither if equal. MFI = 100 * posSumMF/(posSumMF+negSumMF).

## Notes

- When the typical price is unchanged from the prior bar, that bar's money flow is counted as neither positive nor negative.
- A window in which no bar contributed any money flow — every typical price unchanged, or no volume traded — leaves the index undefined (0/0); 0 is returned. The result does not otherwise depend on the size of the money flow: scaling every volume, or quoting the instrument in a different unit, leaves the index unchanged.

## Inputs

- `inHigh` — High price of each bar
- `inLow` — Low price of each bar
- `inClose` — Close price of each bar
- `inVolume` — Volume of each bar

## Outputs

- `outReal` — Money Flow Index

## Parameters

| Parameter | Type | Default | Accepted values | Description |
| --- | --- | --- | --- | --- |
| `optInTimePeriod` | integer | 14 | 2–100000 | Lookback window for summing money flow |

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
double volume[252]; /* your data, oldest first */
double out[252];
int outBegIdx, outNBElement;

TA_Initialize();
TA_MFI( 0, n - 1, high, low, close, volume, 14, &outBegIdx, &outNBElement, out );
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
let range = core.mfi(0, high.len() - 1, &high, &low, &close, &volume, 14, &mut out)?;
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
OutRange r = core.mfi(0, high.length - 1, high, low, close, volume, 14, out);
System.out.println(r.count() + " values from bar " + r.begIdx() + "; last out = " + out[r.count() - 1]);
```

@tab C\#

```csharp
using System;
using TALib;

var high = new double[252]; // your data, oldest first
var low = new double[252]; // your data, oldest first
var close = new double[252]; // your data, oldest first
var volume = new double[252]; // your data, oldest first
var outReal = new double[252];

var core = Core.Default;
OutRange r = core.Mfi(0, high.Length - 1, high, low, close, volume, 14, outReal);
Console.WriteLine($"{r.Count} values from bar {r.BegIdx}; last outReal = {outReal[r.Count - 1]}");
```

@tab TypeScript

```ts
import { mfi } from 'ta-lib';

const high = new Float64Array(252); // your data, oldest first
const low = new Float64Array(252); // your data, oldest first
const close = new Float64Array(252); // your data, oldest first
const volume = new Float64Array(252); // your data, oldest first

const { begIdx, values } = mfi({ high, low, close, volume }, {
  timePeriod: 14,
});
console.log(values.length + ' values from bar ' + begIdx + '; last values = ' + values[values.length - 1]);
```

:::

## Implementation

Added in 0.4.0.

TA-Lib Definition: [`mfi.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/mfi/mfi.c) · [`mfi.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/mfi/mfi.yaml)

| Native | File |
|--------|------|
| C | [`ta_MFI.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_MFI.c) |
| Rust | [`mfi.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/library/src/ta_func/mfi.rs) |
| Java | [`Core_MFI.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/fragments/Core_MFI.java) |
| C# | [`Core_MFI.cs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/csharp/library/src/Core_MFI.cs) |
| TypeScript | [`mfi.ts`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/ts/library/src/func/mfi.ts) |

Python: `talib.MFI`, see [ta-lib-python](https://ta-lib.github.io/ta-lib-python/func_groups/momentum_indicators.html).

TA-Lib is also available for R and more using a [wrapper](/install/#wrappers).

## Aliases

Money Flow Index

## See Also

[RSI](/functions/rsi.md) · [AD](/functions/ad.md) · [ADOSC](/functions/adosc.md)

## References

- Gene Quong & Avrum Soudack, *Volume-Weighted RSI: Money Flow*, Technical Analysis of Stocks & Commodities, V.7:3 (March 1989)
