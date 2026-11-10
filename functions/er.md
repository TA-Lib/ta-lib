---
url: 'https://ta-lib.org/functions/er.md'
description: >-
  Kaufman Efficiency Ratio (also searched as "KER"): Perry Kaufman's noise
  measure from Smarter Trading (1995) — the net directional movement over the…
---
# Kaufman Efficiency Ratio (ER)

## Summary

Kaufman Efficiency Ratio (also searched as "KER"): Perry Kaufman's noise measure from *Smarter Trading* (1995) — the net directional movement over the period divided by the total path travelled to get there. 1.0 is a perfectly efficient (straight-line) move; values near 0 are churn.

This is exactly the efficiency ratio [`KAMA`](/functions/kama.md) computes internally to set its adaptive smoothing constant, exposed standalone and kept bit-identical to it.

## Chart

<ta-chart src="/functions/er.svg" ratio="720:416" label="ER over IBM, daily, 1999" alt="Drawn on the price pane over the close as a line; below it, a separate indicator pane showing outReal as a solid line."></ta-chart>

<div class="ta-chart-links">

[Reference data (CSV)](/functions/er.csv) · [Chart (SVG)](/functions/er.svg)

</div>

::: details Chart metadata <span class="flag-tip" tabindex="0" role="note" aria-label="The chart is drawn automatically from metadata TA-Lib provides. Your app can get the same through the Abstract API." data-tip="The chart is drawn automatically from metadata TA-Lib provides. Your app can get the same through the Abstract API.">i</span>

**Outputs**

* `outReal`: `line` · `zero` · `positive`

:::

## Formula

`ER[t] = |close[t] − close[t−P]| / Σ |close[k] − close[k−1]|` over the same `P` bars.

Two guards, both shared with `KAMA`: a ratio that floating point would nudge just above 1.0 on a straight-line advance is pinned to exactly 1.0, and a dead-flat window (0/0) also reports 1.0 — a flat market therefore reads as "perfectly efficient", which is `KAMA`'s own convention and what keeps the two reconstructible from each other.

The output is a hard 0..1 — the net move can never exceed the path travelled.

TC2000 documents a signed ×100 variant (−100..+100); the absolute 0..1 form here is the author's, StockCharts', LEAN's, backtrader's and pandas-ta's.

## Inputs

* `inReal` — Source price/value series (canonically close)

## Outputs

* `outReal` — Efficiency ratio

## Parameters

| Parameter | Type | Default | Accepted values | Description |
| --- | --- | --- | --- | --- |
| `optInTimePeriod` | integer | 10 | 2–100000 | Number of one-bar changes in the path sum (`KAMA`'s `optInTimePeriod` is the same window, under its own default) |

## Notes

* First output at index `P` (`P` one-bar changes need `P+1` prices). No unstable period, not start-dependent.

## Properties

**Numerical Stability:** [Start-Independent](/functions/stability.md#start-independent)

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
double series[252]; /* your data, oldest first */
double out[252];
int outBegIdx, outNBElement;

TA_Initialize();
TA_ER( 0, n - 1, series, 10, &outBegIdx, &outNBElement, out );
printf( "%d values from bar %d; last out = %f\n", outNBElement, outBegIdx, out[outNBElement - 1] );
```

@tab Rust

```rust
use ta_lib::Core;

let series = vec![0.0; 252]; // your data, oldest first
let mut out = vec![0.0; 252];

let core = Core::new();
let range = core.er(0, series.len() - 1, &series, 10, &mut out)?;
println!("{} values from bar {}; last out = {}", range.count, range.beg_idx, out[range.count - 1]);
```

@tab Java

```java
import io.github.talib.Core;
import io.github.talib.OutRange;

double[] series = new double[252]; // your data, oldest first
double[] out = new double[252];

Core core = Core.DEFAULT;
OutRange r = core.er(0, series.length - 1, series, 10, out);
System.out.println(r.count() + " values from bar " + r.begIdx() + "; last out = " + out[r.count() - 1]);
```

@tab C#

```csharp
using System;
using TALib;

var series = new double[252]; // your data, oldest first
var outReal = new double[252];

var core = Core.Default;
OutRange r = core.Er(0, series.Length - 1, series, 10, outReal);
Console.WriteLine($"{r.Count} values from bar {r.BegIdx}; last outReal = {outReal[r.Count - 1]}");
```

@tab TypeScript

```ts
import { er } from 'ta-lib';

const series = new Float64Array(252); // your data, oldest first

const { begIdx, values } = er(series, { timePeriod: 10 });
console.log(values.length + ' values from bar ' + begIdx + '; last values = ' + values[values.length - 1]);
```

:::

## Implementation

Added in 0.8.1 (ta-lib-python 0.8.1).

TA-Lib Definition: [`er.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/er/er.c) · [`er.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/er/er.yaml)

| Native | File |
|--------|------|
| C | [`ta_ER.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_ER.c) |
| Rust | [`er.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/library/src/ta_func/er.rs) |
| Java | [`Core_ER.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/fragments/Core_ER.java) |
| C# | [`Core_ER.cs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/csharp/library/src/Core_ER.cs) |
| TypeScript | [`er.ts`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/ts/library/src/func/er.ts) |

Python: `talib.ER` (ta-lib-python 0.8.1 or later), see [ta-lib-python](https://ta-lib.github.io/ta-lib-python/func_groups/momentum_indicators.html).

TA-Lib is also available for R and more using a [wrapper](/install/#wrappers).

## Aliases

Efficiency Ratio · Kaufman Efficiency Ratio · KER

## See Also

[KAMA](/functions/kama.md) · [MAMA](/functions/mama.md) · [STDDEV](/functions/stddev.md) · [VHF](/functions/vhf.md)

## References

* Perry Kaufman, *Smarter Trading: Improving Performance in Changing Markets* (McGraw-Hill, 1995) — the efficiency ratio and the adaptive moving average built on it
* Perry Kaufman, *Trading Systems and Methods*, 6th ed. (Wiley, 2019), the "Efficiency Ratio" section
