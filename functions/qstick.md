---
url: 'https://ta-lib.org/functions/qstick.md'
description: >-
  Tushar Chande and Stanley Kroll's Qstick (The New Technical Trader, 1994): a
  simple moving average of the candle body, close minus open.
---
# Qstick (QSTICK)

## Summary

Tushar Chande and Stanley Kroll's Qstick (*The New Technical Trader*, 1994): a simple moving average of the candle body, close minus open. It measures how bullish or bearish the bodies have been over the window, independently of the wicks — above zero the bodies closed up on balance, below zero they closed down, and the zero-line crossings are the signal.

## Chart

<ta-chart src="/functions/qstick.svg" ratio="720:416" label="QSTICK over IBM, daily, 1999" alt="Drawn on the price pane over candles; below it, a separate indicator pane with a zero line showing outReal as a solid line."></ta-chart>

::: details Chart metadata <span class="flag-tip" tabindex="0" role="note" aria-label="The chart is drawn from the function's metadata: the flags and attributes the abstract API reports for it and for each output, and the input series its reference run declares." data-tip="The chart is drawn from the function's metadata: the flags and attributes the abstract API reports for it and for each output, and the input series its reference run declares.">i</span>

Drawn on the price pane over candles; below it, a separate indicator pane with a zero line showing outReal as a solid line.

Metadata flags used: `inPriceOC: open, close` · `outReal: line, positive, negative`

:::

[Reference data (CSV)](/functions/qstick.csv) · [Chart (SVG)](/functions/qstick.svg)

## Formula

body_t = close_t - open_t; QSTICK_t = ( Σ body over the last `optInTimePeriod` bars ) / optInTimePeriod

The moving average is a plain SMA, so there is no seeding convention and none of the cross-library divergence that comes with one. `optInTimePeriod` of 1 leaves the raw body.

## Inputs

* `inOpen` — Open price of each bar
* `inClose` — Close price of each bar

## Outputs

* `outReal` — Average candle body over the window

## Parameters

| Parameter | Type | Default | Accepted values | Description |
| --- | --- | --- | --- | --- |
| `optInTimePeriod` | integer | 10 | 1–100000 | Number of bars averaged. Default 10, matching Tulip Indicators and pandas-ta-classic. Other packages differ: TraderEvolution documents 1, and AmiBroker community code commonly uses 8. |

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
double open[252]; /* your data, oldest first */
double close[252]; /* your data, oldest first */
double out[252];
int outBegIdx, outNBElement;

TA_Initialize();
TA_QSTICK( 0, n - 1, open, close, 10, &outBegIdx, &outNBElement, out );
printf( "%d values from bar %d; last out = %f\n", outNBElement, outBegIdx, out[outNBElement - 1] );
```

@tab Rust

```rust
use ta_lib::Core;

let open = vec![0.0; 252]; // your data, oldest first
let close = vec![0.0; 252]; // your data, oldest first
let mut out = vec![0.0; 252];

let core = Core::new();
let range = core.qstick(0, open.len() - 1, &open, &close, 10, &mut out)?;
println!("{} values from bar {}; last out = {}", range.count, range.beg_idx, out[range.count - 1]);
```

@tab Java

```java
import io.github.talib.Core;
import io.github.talib.OutRange;

double[] open = new double[252]; // your data, oldest first
double[] close = new double[252]; // your data, oldest first
double[] out = new double[252];

Core core = Core.DEFAULT;
OutRange r = core.qstick(0, open.length - 1, open, close, 10, out);
System.out.println(r.count() + " values from bar " + r.begIdx() + "; last out = " + out[r.count() - 1]);
```

@tab C#

```csharp
using System;
using TALib;

var open = new double[252]; // your data, oldest first
var close = new double[252]; // your data, oldest first
var outReal = new double[252];

var core = Core.Default;
OutRange r = core.Qstick(0, open.Length - 1, open, close, 10, outReal);
Console.WriteLine($"{r.Count} values from bar {r.BegIdx}; last outReal = {outReal[r.Count - 1]}");
```

@tab TypeScript

```ts
import { qstick } from 'ta-lib';

const open = new Float64Array(252); // your data, oldest first
const close = new Float64Array(252); // your data, oldest first

const { begIdx, values } = qstick({ open, close }, { timePeriod: 10 });
console.log(values.length + ' values from bar ' + begIdx + '; last values = ' + values[values.length - 1]);
```

:::

## Implementation

Added in 0.8.1 (ta-lib-python 0.8.1).

TA-Lib Definition: [`qstick.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/qstick/qstick.c) · [`qstick.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/qstick/qstick.yaml)

| Native | File |
|--------|------|
| C | [`ta_QSTICK.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_QSTICK.c) |
| Rust | [`qstick.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/library/src/ta_func/qstick.rs) |
| Java | [`Core_QSTICK.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/fragments/Core_QSTICK.java) |
| C# | [`Core_QSTICK.cs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/csharp/library/src/Core_QSTICK.cs) |
| TypeScript | [`qstick.ts`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/ts/library/src/func/qstick.ts) |

Python: `talib.QSTICK` (ta-lib-python 0.8.1 or later), see [ta-lib-python](https://ta-lib.github.io/ta-lib-python/func_groups/momentum_indicators.html).

TA-Lib is also available for R and more using a [wrapper](/install/#wrappers).

## See Also

[CMO](/functions/cmo.md) · [IMI](/functions/imi.md) · [MOM](/functions/mom.md) · [SMA](/functions/sma.md)

## References

* Tushar S. Chande and Stanley Kroll, *The New Technical Trader*, Wiley, 1994, define Qstick as an n-period moving average of `Close - Open`.
* Steven B. Achelis, *Technical Analysis from A to Z*, page 280 carries the worked example pinned in the test suite; it is exact in binary, every price on the page being a sixteenth.
* Tulip Indicators `ti_qstick` and pandas-ta-classic `qstick` compute the same form and both default the period to 10, which is where this default comes from.
* TraderEvolution documents a default period of 1 and offers a choice of moving average; AmiBroker community code commonly cites 8. Neither is verifiable against the book, and this ships the SMA-only form the authors define.
