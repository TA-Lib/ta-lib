---
title: "Choppiness Index (True Range Box) (CHOPTR)"
description: "Choppiness Index with a true-range box: the true range travelled over a window against the span from its highest true high to its lowest true low."
---

## Summary

Choppiness Index with a true-range box: the true range travelled over a window against the span from its highest true high to its lowest true low. Bill Dreiss's form as published in 1993. Log-scaled so that a straight run reads 0 and bars that each fill the whole box read 100. Measures whether the market is trending, not in which direction: low values mean a directional run, high values sideways chop.

## Chart

<ta-chart src="/functions/choptr.svg" ratio="720:416" label="CHOPTR over IBM, daily, 1999" alt="Drawn on the price pane over candles; below it, a separate indicator pane showing outReal as a solid line."></ta-chart>

::: details Chart metadata <span class="flag-tip" tabindex="0" role="note" aria-label="The chart is drawn from the function's metadata: the flags the abstract API reports for it and for each output, and the input series its reference run declares." data-tip="The chart is drawn from the function's metadata: the flags the abstract API reports for it and for each output, and the input series its reference run declares.">i</span>

Drawn on the price pane over candles; below it, a separate indicator pane showing outReal as a solid line.

Metadata flags used: `inPriceHLC: high, low, close` · `outReal: line`

:::

[Reference data (CSV)](/functions/choptr.csv) · [Chart (SVG)](/functions/choptr.svg)

## Formula

TH[j] = max(High[j], Close[j-1]) and TL[j] = min(Low[j], Close[j-1]), the true high and true low. TR[j] = TH[j] - TL[j].

S = SUM( TR[j] ) and R = MAX( TH[j] ) - MIN( TL[j] ), both over j = t-optInTimePeriod+1 .. t.

CHOPTR = 100 * log10( S / R ) / log10( optInTimePeriod ).

The first true range reads the close one bar before the window, so the first value needs `optInTimePeriod` + 1 bars.

## Notes

- The box reaches the close just before each bar. Most charting platforms instead use the window's highest high minus its lowest low; that form is CHOP. The two differ only on bars where the close before the window lies outside the window's high-low range: a gap into the window.
- With every close inside its own bar's high-low range, the value stays within 0 to 100, up to rounding.
- A window with no true range reports 100.
- Dreiss's 3-bar smoothing of the index is not built in; apply a moving average to `outReal` to obtain it.

## Inputs

- `inHigh` — High price of each bar
- `inLow` — Low price of each bar
- `inClose` — Close price of each bar

## Outputs

- `outReal` — Choppiness Index value

## Parameters

| Parameter | Type | Default | Accepted values | Description |
| --- | --- | --- | --- | --- |
| `optInTimePeriod` | integer | 14 | 2–100000 | Number of bars in the window |

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
| <span class="flag-box">✅</span> **Uses Transcendental** <span class="flag-tip" tabindex="0" role="note" aria-label="Calls a transcendental math function (exp, log, log10, trigonometric, inverse trigonometric or hyperbolic), so a value may differ slightly between languages, between a stream and a batch call, and between machines. TA-Lib targets a difference below 1e-9." data-tip="Calls a transcendental math function (exp, log, log10, trigonometric, inverse trigonometric or hyperbolic), so a value may differ slightly between languages, between a stream and a batch call, and between machines. TA-Lib targets a difference below 1e-9.">i</span> |
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
double out[252];
int outBegIdx, outNBElement;

TA_Initialize();
TA_CHOPTR( 0, n - 1, high, low, close, 14, &outBegIdx, &outNBElement, out );
printf( "%d values from bar %d; last out = %f\n", outNBElement, outBegIdx, out[outNBElement - 1] );
```

@tab Rust

```rust
use ta_lib::Core;

let high = vec![0.0; 252]; // your data, oldest first
let low = vec![0.0; 252]; // your data, oldest first
let close = vec![0.0; 252]; // your data, oldest first
let mut out = vec![0.0; 252];

let core = Core::new();
let range = core.choptr(0, high.len() - 1, &high, &low, &close, 14, &mut out)?;
println!("{} values from bar {}; last out = {}", range.count, range.beg_idx, out[range.count - 1]);
```

@tab Java

```java
import io.github.talib.Core;
import io.github.talib.OutRange;

double[] high = new double[252]; // your data, oldest first
double[] low = new double[252]; // your data, oldest first
double[] close = new double[252]; // your data, oldest first
double[] out = new double[252];

Core core = Core.DEFAULT;
OutRange r = core.choptr(0, high.length - 1, high, low, close, 14, out);
System.out.println(r.count() + " values from bar " + r.begIdx() + "; last out = " + out[r.count() - 1]);
```

@tab C\#

```csharp
using System;
using TALib;

var high = new double[252]; // your data, oldest first
var low = new double[252]; // your data, oldest first
var close = new double[252]; // your data, oldest first
var outReal = new double[252];

var core = Core.Default;
OutRange r = core.Choptr(0, high.Length - 1, high, low, close, 14, outReal);
Console.WriteLine($"{r.Count} values from bar {r.BegIdx}; last outReal = {outReal[r.Count - 1]}");
```

@tab TypeScript

```ts
import { choptr } from 'ta-lib';

const high = new Float64Array(252); // your data, oldest first
const low = new Float64Array(252); // your data, oldest first
const close = new Float64Array(252); // your data, oldest first

const { begIdx, values } = choptr({ high, low, close }, { timePeriod: 14 });
console.log(values.length + ' values from bar ' + begIdx + '; last values = ' + values[values.length - 1]);
```

:::

## Implementation

Added in 0.8.3 (ta-lib-python 0.8.3).

TA-Lib Definition: [`choptr.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/choptr/choptr.c) · [`choptr.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/choptr/choptr.yaml)

| Native | File |
|--------|------|
| C | [`ta_CHOPTR.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_CHOPTR.c) |
| Rust | [`choptr.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/library/src/ta_func/choptr.rs) |
| Java | [`Core_CHOPTR.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/fragments/Core_CHOPTR.java) |
| C# | [`Core_CHOPTR.cs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/csharp/library/src/Core_CHOPTR.cs) |
| TypeScript | [`choptr.ts`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/ts/library/src/func/choptr.ts) |

Python: `talib.CHOPTR` (ta-lib-python 0.8.3 or later), see [ta-lib-python](https://ta-lib.github.io/ta-lib-python/func_groups/momentum_indicators.html).

TA-Lib is also available for R and more using a [wrapper](/install/#wrappers).

## Aliases

Choppiness Index, Dreiss Choppiness Index, True Range Choppiness Index

## See Also

[CHOP](/functions/chop.md) · [VHF](/functions/vhf.md) · [ADX](/functions/adx.md) · [TRANGE](/functions/trange.md)

## References

- Gibbons Burke, "Measuring market choppiness with chaos", *Futures*, October 1993, pp. 52-53
- [Incredible Charts: Choppiness Index](https://www.incrediblecharts.com/indicators/choppiness-index.php)
