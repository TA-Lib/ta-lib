---
title: "Connors Relative Strength Index (CRSI)"
description: "Connors RSI: a 0 to 100 oscillator, the plain average of three short-term momentum readings on that same scale."
---

## Summary

Connors RSI: a 0 to 100 oscillator, the plain average of three short-term momentum readings on that same scale.

The first is Wilder's RSI of the closes over `optInTimePeriod` bars. The second is an RSI over `optInStreakPeriod` bars of the up/down streak, the signed length of the current run of higher or lower closes (+3 after three higher closes in a row, -2 after two lower ones); an unchanged close resets the streak to 0. The third is the percent rank of the bar's one-bar return, (close - previous close) / previous close, among the `optInRankPeriod` returns before it: the percentage of them strictly below it.

High readings mark a short-term overbought market and low readings an oversold one.

## Chart

<ta-chart src="/functions/crsi.svg" ratio="720:416" label="CRSI over IBM, daily, 1999" alt="Drawn on the price pane over the close as a line; below it, a separate indicator pane with a zero line showing outReal as a solid line."></ta-chart>

Drawn on the price pane over the close as a line; below it, a separate indicator pane with a zero line showing outReal as a solid line. [Reference data (CSV)](/functions/crsi.csv) · [Chart (SVG)](/functions/crsi.svg)

## Formula

$$
\begin{aligned}
\mathrm{CRSI}_t &= \frac{\mathrm{RSI}(\mathrm{Close}, m_1)_t + \mathrm{RSI}(\mathrm{Streak}, m_2)_t + \mathrm{PercentRank}(\mathrm{ROC1}, P)_t}{3} \\[6pt]
\mathrm{Streak}_t &= \begin{cases}
    \max(\mathrm{Streak}_{t-1},\ 0) + 1 & \text{if } \mathrm{Close}_t > \mathrm{Close}_{t-1} \\
    \min(\mathrm{Streak}_{t-1},\ 0) - 1 & \text{if } \mathrm{Close}_t < \mathrm{Close}_{t-1} \\
    0                                   & \text{if } \mathrm{Close}_t = \mathrm{Close}_{t-1}
  \end{cases} \\[6pt]
\mathrm{ROC1}_t &= \frac{\mathrm{Close}_t - \mathrm{Close}_{t-1}}{\mathrm{Close}_{t-1}} \\[6pt]
\mathrm{PercentRank}(\mathrm{ROC1}, P)_t &= \frac{\left|\lbrace\, j : t-P \le j \le t-1,\ \mathrm{ROC1}_j < \mathrm{ROC1}_t \,\rbrace\right|}{P} \times 100
\end{aligned}
$$

where $\mathrm{Close}$ is the input series, $m_1$ is `optInTimePeriod`, $m_2$ is `optInStreakPeriod` and $P$ is `optInRankPeriod`. Both RSIs are Wilder's [`RSI`](/functions/rsi.md). The streak is defined from the second bar, taking the streak before it as 0, and $\mathrm{ROC1}_t$ is 0 when $\mathrm{Close}_{t-1} = 0$.

## Notes

- CRSI has no unstable period of its own: both RSI legs take `TA_FUNC_UNST_RSI`. They start warming up on the first bar the call reads (the streak's RSI one bar later, the streak being a change), so a leg that needs fewer bars than the lookback warms up over the spare ones.
- Finite input is a precondition. A NaN close does not propagate: the output stays finite and no error is reported, but the values that follow it are wrong.

## Inputs

- `inReal` — Close price series

## Outputs

- `outReal` — The averaged reading, 0 to 100

## Parameters

| Parameter | Type | Default | Accepted values | Description |
| --- | --- | --- | --- | --- |
| `optInTimePeriod` | integer | 3 | 2–100000 | Period of the RSI of the closes |
| `optInStreakPeriod` | integer | 2 | 2–100000 | Period of the RSI of the up/down streak |
| `optInRankPeriod` | integer | 100 | 2–10000 | Number of earlier one-bar returns each return is ranked against |

## Properties

**Numerical Stability:** [Initial Unstable Period](/functions/stability.md#initial-unstable-period) — Inherited from RSI, which CRSI computes internally; tunable via RSI's unstable period.

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
double series[252]; /* your data, oldest first */
double out[252];
int outBegIdx, outNBElement;

TA_Initialize();
TA_CRSI( 0, n - 1, series, 3, 2, 100, &outBegIdx, &outNBElement, out );
printf( "%d values from bar %d; last out = %f\n", outNBElement, outBegIdx, out[outNBElement - 1] );
```

@tab Rust

```rust
use ta_lib::Core;

let series = vec![0.0; 252]; // your data, oldest first
let mut out = vec![0.0; 252];

let core = Core::new();
let range = core.crsi(0, series.len() - 1, &series, 3, 2, 100, &mut out)?;
println!("{} values from bar {}; last out = {}", range.count, range.beg_idx, out[range.count - 1]);
```

@tab Java

```java
import io.github.talib.Core;
import io.github.talib.OutRange;

double[] series = new double[252]; // your data, oldest first
double[] out = new double[252];

Core core = Core.DEFAULT;
OutRange r = core.crsi(0, series.length - 1, series, 3, 2, 100, out);
System.out.println(r.count() + " values from bar " + r.begIdx() + "; last out = " + out[r.count() - 1]);
```

@tab C\#

```csharp
using System;
using TALib;

var series = new double[252]; // your data, oldest first
var outReal = new double[252];

var core = Core.Default;
OutRange r = core.Crsi(0, series.Length - 1, series, 3, 2, 100, outReal);
Console.WriteLine($"{r.Count} values from bar {r.BegIdx}; last outReal = {outReal[r.Count - 1]}");
```

@tab TypeScript

```ts
import { crsi } from 'ta-lib';

const series = new Float64Array(252); // your data, oldest first

const { begIdx, values } = crsi(series, {
  timePeriod: 3,
  streakPeriod: 2,
  rankPeriod: 100,
});
console.log(values.length + ' values from bar ' + begIdx + '; last values = ' + values[values.length - 1]);
```

:::

## Implementation

Added in 0.8.3 (ta-lib-python 0.8.3).

TA-Lib Definition: [`crsi.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/crsi/crsi.c) · [`crsi.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/crsi/crsi.yaml)

| Native | File |
|--------|------|
| C | [`ta_CRSI.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_CRSI.c) |
| Rust | [`crsi.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/library/src/ta_func/crsi.rs) |
| Java | [`Core_CRSI.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/fragments/Core_CRSI.java) |
| C# | [`Core_CRSI.cs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/csharp/library/src/Core_CRSI.cs) |
| TypeScript | [`crsi.ts`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/ts/library/src/func/crsi.ts) |

Python: `talib.CRSI` (ta-lib-python 0.8.3 or later), see [ta-lib-python](https://ta-lib.github.io/ta-lib-python/func_groups/momentum_indicators.html).

TA-Lib is also available for R and more using a [wrapper](/install/#wrappers).

## Aliases

ConnorsRSI, Connors RSI, Connors Relative Strength Index

## See Also

[RSI](/functions/rsi.md) · [PERCENTRANK](/functions/percentrank.md) · [STOCHRSI](/functions/stochrsi.md)

## References

- [Connors Research, *An Introduction to ConnorsRSI* (2012), pp. 7-9](https://www.qmatix.com/ConnorsRSI-Pullbacks-Guidebook.pdf)
- [StockCharts ChartSchool, *ConnorsRSI*](https://chartschool.stockcharts.com/table-of-contents/technical-indicators-and-overlays/technical-indicators/connorsrsi)
