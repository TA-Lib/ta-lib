---
url: 'https://ta-lib.org/functions/kama.md'
description: >-
  Kaufman Adaptive Moving Average: an EMA whose smoothing factor adapts each bar
  to an efficiency ratio (directional move vs. total volatility).
---
# Kaufman Adaptive Moving Average (KAMA)

## Summary

Kaufman Adaptive Moving Average: an EMA whose smoothing factor adapts each bar to an efficiency ratio (directional move vs. total volatility). Reacts fast in trends and smooths in ranging markets. Flat KAMA = non-trending/ranging market. KAMA tracking price closely = efficient trend.

## Chart

<ta-chart src="/functions/kama.svg" ratio="720:274" label="KAMA over IBM, daily, 1999" alt="Drawn on the price pane over the close as a line, with outReal as a solid line."></ta-chart>

::: details Chart metadata <span class="flag-tip" tabindex="0" role="note" aria-label="The chart is drawn from the function's metadata: the flags and attributes the abstract API reports for it and for each output, and the input series its reference run declares." data-tip="The chart is drawn from the function's metadata: the flags and attributes the abstract API reports for it and for each output, and the input series its reference run declares.">i</span>

Drawn on the price pane over the close as a line, with outReal as a solid line.

Metadata flags used: `overlap` · `outReal: line`

:::

[Reference data (CSV)](/functions/kama.csv) · [Chart (SVG)](/functions/kama.svg)

## Formula

ER = |price\[t] - price\[t-period]| / sum(|price\[i]-price\[i-1]|, last period bars)
SC = (ER\*(2/3 - 2/31) + 2/31)^2
KAMA\[t] = KAMA\[t-1] + SC\*(price\[t] - KAMA\[t-1])

## Notes

* A period of 1 performs no smoothing: the output is a copy of the input, consistent with `MA(period=1)` for every MAType. (The natural KAMA math at period 1 would degenerate to a fixed-alpha EMA because the efficiency ratio is always 1, so the copy is made explicit.) Allowed since 0.6.5.
* The output never leaves the range of the prices it has seen.

## Inputs

* `inReal` — Source price series

## Outputs

* `outReal` — Adaptive moving average line

## Parameters

| Parameter | Type | Default | Accepted values | Description |
| --- | --- | --- | --- | --- |
| `optInTimePeriod` | integer | 30 | 1–100000 | Lookback window for the efficiency ratio |

## Properties

**Numerical Stability:** [Initial Unstable Period](/functions/stability.md#initial-unstable-period)

<div class="flag-table">

|  |
| :-- |
| <span class="flag-box">✅</span> **Overlap Input** <span class="flag-tip" tabindex="0" role="note" aria-label="Output is on the same scale as the input price, so it is drawn over the price chart." data-tip="Output is on the same scale as the input price, so it is drawn over the price chart.">i</span> |
| <span class="flag-box">☐</span> <span style="opacity:0.5">Independent Y-Axis</span> |
| <span class="flag-box">☐</span> <span style="opacity:0.5">Candlestick</span> |
| <span class="flag-box">☐</span> <span style="opacity:0.5">Can Output NaN or ±Inf</span> |
| <span class="flag-box">✅</span> **Identity at Period 1** <span class="flag-tip" tabindex="0" role="note" aria-label="A period of 1 performs no smoothing: every output value is a bit-exact copy of its input value." data-tip="A period of 1 performs no smoothing: every output value is a bit-exact copy of its input value.">i</span> |
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
double series[252]; /* your data, oldest first */
double out[252];
int outBegIdx, outNBElement;

TA_Initialize();
TA_KAMA( 0, n - 1, series, 30, &outBegIdx, &outNBElement, out );
printf( "%d values from bar %d; last out = %f\n", outNBElement, outBegIdx, out[outNBElement - 1] );
```

@tab Rust

```rust
use ta_lib::Core;

let series = vec![0.0; 252]; // your data, oldest first
let mut out = vec![0.0; 252];

let core = Core::new();
let range = core.kama(0, series.len() - 1, &series, 30, &mut out)?;
println!("{} values from bar {}; last out = {}", range.count, range.beg_idx, out[range.count - 1]);
```

@tab Java

```java
import io.github.talib.Core;
import io.github.talib.OutRange;

double[] series = new double[252]; // your data, oldest first
double[] out = new double[252];

Core core = Core.DEFAULT;
OutRange r = core.kama(0, series.length - 1, series, 30, out);
System.out.println(r.count() + " values from bar " + r.begIdx() + "; last out = " + out[r.count() - 1]);
```

@tab C#

```csharp
using System;
using TALib;

var series = new double[252]; // your data, oldest first
var outReal = new double[252];

var core = Core.Default;
OutRange r = core.Kama(0, series.Length - 1, series, 30, outReal);
Console.WriteLine($"{r.Count} values from bar {r.BegIdx}; last outReal = {outReal[r.Count - 1]}");
```

@tab TypeScript

```ts
import { kama } from 'ta-lib';

const series = new Float64Array(252); // your data, oldest first

const { begIdx, values } = kama(series, { timePeriod: 30 });
console.log(values.length + ' values from bar ' + begIdx + '; last values = ' + values[values.length - 1]);
```

:::

## Implementation

Added in 0.4.0.

TA-Lib Definition: [`kama.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/kama/kama.c) · [`kama.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/kama/kama.yaml)

| Native | File |
|--------|------|
| C | [`ta_KAMA.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_KAMA.c) |
| Rust | [`kama.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/library/src/ta_func/kama.rs) |
| Java | [`Core_KAMA.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/fragments/Core_KAMA.java) |
| C# | [`Core_KAMA.cs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/csharp/library/src/Core_KAMA.cs) |
| TypeScript | [`kama.ts`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/ts/library/src/func/kama.ts) |

Python: `talib.KAMA`, see [ta-lib-python](https://ta-lib.github.io/ta-lib-python/func_groups/overlap_studies.html).

TA-Lib is also available for R and more using a [wrapper](/install/#wrappers).

## Aliases

Kaufman Adaptive Moving Average, Kaufman's Adaptive Moving Average

## See Also

[MAMA](/functions/mama.md) · [EMA](/functions/ema.md) · [MA](/functions/ma.md)

## References

* Perry J. Kaufman, *Smarter Trading: Improving Performance in Changing Markets*, McGraw-Hill (1995)
