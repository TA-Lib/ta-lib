---
url: 'https://ta-lib.org/functions/rsi.md'
description: >-
  Wilder's Relative Strength Index, a momentum oscillator bounded 0-100 from the
  ratio of average gains to average losses over the period.
---
# Relative Strength Index (RSI)

## Summary

Wilder's Relative Strength Index, a momentum oscillator bounded 0-100 from the ratio of average gains to average losses over the period. Used to gauge overbought/oversold conditions. >70 overbought, <30 oversold.

## Chart

<ta-chart src="/functions/rsi.svg" ratio="720:416" label="RSI over IBM, daily, 1999" alt="Drawn on the price pane over the close as a line; below it, a separate indicator pane showing outReal as a solid line with dashed rules at 30 and 70."></ta-chart>

::: details Chart metadata <span class="flag-tip" tabindex="0" role="note" aria-label="The chart is drawn from the function's metadata: the flags and attributes the abstract API reports for it and for each output, and the input series its reference run declares." data-tip="The chart is drawn from the function's metadata: the flags and attributes the abstract API reports for it and for each output, and the input series its reference run declares.">i</span>

Drawn on the price pane over the close as a line; below it, a separate indicator pane showing outReal as a solid line with dashed rules at 30 and 70.

Metadata flags used: `outReal: line, levels 30, 70`

:::

[Reference data (CSV)](/functions/rsi.csv) · [Chart (SVG)](/functions/rsi.svg)

## Formula

$$
\begin{aligned}
U_t &= \max(X_t - X_{t-1},\ 0)
   &  D_t &= \max(X_{t-1} - X_t,\ 0) \\[4pt]
\overline{U}_t &= \begin{cases}
    \operatorname{SMA}(U, n)_t                 & \text{if } t = n \\[4pt]
    \dfrac{(n-1)\,\overline{U}_{t-1} + U_t}{n} & \text{if } t > n
  \end{cases}
   &  \overline{D}_t &= \begin{cases}
    \operatorname{SMA}(D, n)_t                 & \text{if } t = n \\[4pt]
    \dfrac{(n-1)\,\overline{D}_{t-1} + D_t}{n} & \text{if } t > n
  \end{cases} \\[4pt]
\mathrm{RS}_t &= \frac{\overline{U}_t}{\overline{D}_t}
   &  \mathrm{RSI}_t &= 100 - \frac{100}{1 + \mathrm{RS}_t}
\end{aligned}
$$

where $X$ is the input series and $n$ the period.

## Notes

* While the input has not changed since the first bar the call reads, there is neither a gain nor a loss and RSI is 0/0: the output is the neutral 50. Up to 0.8.1 it was 0, which read as oversold. Input that has only risen gives 100 and input that has only fallen gives 0.
* After a move, an unchanged input holds the last value until the two averages decay to rounding residue: about a thousand unchanged bars at period 2, about ten thousand at period 14. The output then drifts and settles on 50. Input that had only risen or only fallen stays at 100 or 0, except at period 2.

## Inputs

* `inReal` — Price series (typically close)

## Outputs

* `outReal` — RSI value

## Parameters

| Parameter | Type | Default | Accepted values | Description |
| --- | --- | --- | --- | --- |
| `optInTimePeriod` | integer | 14 | 2–100000 | Lookback for the gain/loss averaging |

## Properties

**Numerical Stability:** [Initial Unstable Period](/functions/stability.md#initial-unstable-period)

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
TA_RSI( 0, n - 1, series, 14, &outBegIdx, &outNBElement, out );
printf( "%d values from bar %d; last out = %f\n", outNBElement, outBegIdx, out[outNBElement - 1] );
```

@tab Rust

```rust
use ta_lib::Core;

let series = vec![0.0; 252]; // your data, oldest first
let mut out = vec![0.0; 252];

let core = Core::new();
let range = core.rsi(0, series.len() - 1, &series, 14, &mut out)?;
println!("{} values from bar {}; last out = {}", range.count, range.beg_idx, out[range.count - 1]);
```

@tab Java

```java
import io.github.talib.Core;
import io.github.talib.OutRange;

double[] series = new double[252]; // your data, oldest first
double[] out = new double[252];

Core core = Core.DEFAULT;
OutRange r = core.rsi(0, series.length - 1, series, 14, out);
System.out.println(r.count() + " values from bar " + r.begIdx() + "; last out = " + out[r.count() - 1]);
```

@tab C#

```csharp
using System;
using TALib;

var series = new double[252]; // your data, oldest first
var outReal = new double[252];

var core = Core.Default;
OutRange r = core.Rsi(0, series.Length - 1, series, 14, outReal);
Console.WriteLine($"{r.Count} values from bar {r.BegIdx}; last outReal = {outReal[r.Count - 1]}");
```

@tab TypeScript

```ts
import { rsi } from 'ta-lib';

const series = new Float64Array(252); // your data, oldest first

const { begIdx, values } = rsi(series, { timePeriod: 14 });
console.log(values.length + ' values from bar ' + begIdx + '; last values = ' + values[values.length - 1]);
```

:::

## Implementation

Added in 0.4.0.

TA-Lib Definition: [`rsi.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/rsi/rsi.c) · [`rsi.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/rsi/rsi.yaml)

| Native | File |
|--------|------|
| C | [`ta_RSI.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_RSI.c) |
| Rust | [`rsi.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/library/src/ta_func/rsi.rs) |
| Java | [`Core_RSI.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/fragments/Core_RSI.java) |
| C# | [`Core_RSI.cs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/csharp/library/src/Core_RSI.cs) |
| TypeScript | [`rsi.ts`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/ts/library/src/func/rsi.ts) |

Python: `talib.RSI`, see [ta-lib-python](https://ta-lib.github.io/ta-lib-python/func_groups/momentum_indicators.html).

TA-Lib is also available for R and more using a [wrapper](/install/#wrappers).

## Aliases

relative strength index

## See Also

[CMO](/functions/cmo.md) · [STOCHRSI](/functions/stochrsi.md)

## References

* J. Welles Wilder, *New Concepts in Technical Trading Systems*, Trend Research (ISBN 0894590278)
