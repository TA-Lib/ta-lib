---
title: "Relative Volatility Index, refined high/low form (RVIR)"
description: "Relative Volatility Index, refined form: Donald Dorsey's 1995 revision of his own indicator, which runs the 1993 RVI over the daily highs and again over…"
---

## Summary

Relative Volatility Index, refined form: Donald Dorsey's 1995 revision of his own indicator, which runs the 1993 RVI over the daily highs and again over the daily lows and averages the two indices.

Output is bounded [0..100] and is interpreted like RVI: above 50 the highs and lows have been more volatile while rising than while falling, below 50 the reverse. Dorsey's stated reason for the revision is that a high and a low carry the day's range, so the pair answers the question the close alone can only approximate.

## Chart

<ta-chart src="/functions/rvir.svg" ratio="720:416" label="RVIR over IBM, daily, 1999" alt="Drawn on the price pane over candles; below it, a separate indicator pane showing outReal as a solid line."></ta-chart>

::: details Chart metadata <span class="flag-tip" tabindex="0" role="note" aria-label="The chart is drawn from the function's metadata: the flags the abstract API reports for it and for each output, and the input series its reference run declares." data-tip="The chart is drawn from the function's metadata: the flags the abstract API reports for it and for each output, and the input series its reference run declares.">i</span>

Drawn on the price pane over candles; below it, a separate indicator pane showing outReal as a solid line.

Metadata flags used: `inPriceHL: high, low` · `outReal: line`

:::

[Reference data (CSV)](/functions/rvir.csv) · [Chart (SVG)](/functions/rvir.svg)

## Formula

    RVIR[i] = 0.5 * ( RVI(high)[i] + RVI(low)[i] )

Both legs use the same `optInTimePeriod` and `optInStdDevPeriod`, so they warm on the same bar.

## Notes

- RVIR is the 1995 revision; [`RVI`](/functions/rvi.md) is the 1993 version.

## Inputs

- `inHigh` — High price of each bar
- `inLow` — Low price of each bar

## Outputs

- `outReal` — The averaged index, in 0..100

## Parameters

| Parameter | Type | Default | Accepted values | Description |
| --- | --- | --- | --- | --- |
| `optInTimePeriod` | integer | 14 | 1–100000 | Wilder smoothing period applied to both legs of both indices |
| `optInStdDevPeriod` | integer | 10 | 2–100000 | Number of trailing values each standard deviation spans |

## Properties

**Numerical Stability:** [Initial Unstable Period](/functions/stability.md#initial-unstable-period) — Inherited from RVI, which RVIR computes internally; tunable via RVI's unstable period.

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
double out[252];
int outBegIdx, outNBElement;

TA_Initialize();
TA_RVIR( 0, n - 1, high, low, 14, 10, &outBegIdx, &outNBElement, out );
printf( "%d values from bar %d; last out = %f\n", outNBElement, outBegIdx, out[outNBElement - 1] );
```

@tab Rust

```rust
use ta_lib::Core;

let high = vec![0.0; 252]; // your data, oldest first
let low = vec![0.0; 252]; // your data, oldest first
let mut out = vec![0.0; 252];

let core = Core::new();
let range = core.rvir(0, high.len() - 1, &high, &low, 14, 10, &mut out)?;
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
OutRange r = core.rvir(0, high.length - 1, high, low, 14, 10, out);
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
OutRange r = core.Rvir(0, high.Length - 1, high, low, 14, 10, outReal);
Console.WriteLine($"{r.Count} values from bar {r.BegIdx}; last outReal = {outReal[r.Count - 1]}");
```

@tab TypeScript

```ts
import { rvir } from 'ta-lib';

const high = new Float64Array(252); // your data, oldest first
const low = new Float64Array(252); // your data, oldest first

const { begIdx, values } = rvir({ high, low }, {
  timePeriod: 14,
  stdDevPeriod: 10,
});
console.log(values.length + ' values from bar ' + begIdx + '; last values = ' + values[values.length - 1]);
```

:::

## Implementation

Added in 0.8.3 (ta-lib-python 0.8.3).

TA-Lib Definition: [`rvir.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/rvir/rvir.c) · [`rvir.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/rvir/rvir.yaml)

| Native | File |
|--------|------|
| C | [`ta_RVIR.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_RVIR.c) |
| Rust | [`rvir.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/library/src/ta_func/rvir.rs) |
| Java | [`Core_RVIR.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/fragments/Core_RVIR.java) |
| C# | [`Core_RVIR.cs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/csharp/library/src/Core_RVIR.cs) |
| TypeScript | [`rvir.ts`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/ts/library/src/func/rvir.ts) |

Python: `talib.RVIR` (ta-lib-python 0.8.3 or later), see [ta-lib-python](https://ta-lib.github.io/ta-lib-python/func_groups/volatility_indicators.html).

TA-Lib is also available for R and more using a [wrapper](/install/#wrappers).

## Aliases

Relative Volatility Index (1995 revision), RVIr

## See Also

[RVI](/functions/rvi.md) · [STDDEV](/functions/stddev.md) · [ATR](/functions/atr.md)

## References

- Dorsey, Donald. "Refining the Relative Volatility Index." *Technical Analysis of Stocks & Commodities*, V.13:9 (September 1995), 388-391.
- Dorsey, Donald. "The Relative Volatility Index." *Technical Analysis of Stocks & Commodities*, V.11:6 (June 1993), 253-256.
