---
title: "Wilder Swing Index (SI)"
description: "Wilder's Swing Index: one number rating each bar against the bar before it, from both bars' open and close and the current bar's high and low, signed by…"
---

## Summary

Wilder's Swing Index: one number rating each bar against the bar before it, from both bars' open and close and the current bar's high and low, signed by the direction of the swing. A strong close above the prior close on a wide range reads high and positive, a strong down swing reads high and negative. It is scaled against the limit move, the largest one-bar move the market allows, and stays between -100 and +100 while every move is within that limit. J. Welles Wilder Jr. introduced it in 1978 and trades its running total, ASI.

## Chart

<ta-chart src="/functions/si.svg" ratio="720:416" label="SI over IBM, daily, 1999" alt="Drawn on the price pane over candles; below it, a separate indicator pane with a zero line showing outReal as a solid line."></ta-chart>

::: details Chart metadata <span class="flag-tip" tabindex="0" role="note" aria-label="The chart is drawn automatically from metadata TA-Lib provides. Your app can get the same through the Abstract API." data-tip="The chart is drawn automatically from metadata TA-Lib provides. Your app can get the same through the Abstract API.">i</span>

Drawn on the price pane over candles; below it, a separate indicator pane with a zero line showing outReal as a solid line.

Metadata flags used: `inPriceOHLC: open, high, low, close` · `outReal: line, positive, negative`

:::

[Reference data (CSV)](/functions/si.csv) · [Chart (SVG)](/functions/si.svg)

## Formula

$$
\mathrm{SI}_t = 50 \cdot \frac{N_t}{R_t} \cdot \frac{K_t}{T}, \text{ or } 0 \text{ when } R_t = 0
$$

$$
N_t = (C_t - C_{t-1}) + \tfrac{1}{2}(C_t - O_t) + \tfrac{1}{4}(C_{t-1} - O_{t-1}) \qquad K_t = \max(a, b)
$$

$$
R_t = \begin{cases} a - \tfrac{1}{2}b + q & \text{if } a \ge b \text{ and } a \ge d \\ b - \tfrac{1}{2}a + q & \text{else if } b \ge d \\ d + q & \text{otherwise} \end{cases}
$$

where $a = |H_t - C_{t-1}|$, $b = |L_t - C_{t-1}|$, $d = |H_t - L_t|$, $q = \tfrac{1}{4}|C_{t-1} - O_{t-1}|$, and $T$ is `optInLimitMove` (Wilder writes it $L$).

## Notes

- Wilder rounds each value to a whole number when working by hand; the output is not rounded.
- A bar that moves more than the limit move can read beyond -100 or +100; the output is not clamped.

## Inputs

- `inOpen` — Open price of each bar
- `inHigh` — High price of each bar
- `inLow` — Low price of each bar
- `inClose` — Close price of each bar

## Outputs

- `outReal` — Swing index of the bar against the previous bar

## Parameters

| Parameter | Type | Default | Accepted values | Description |
| --- | --- | --- | --- | --- |
| `optInLimitMove` | real | 3 | ≥ 0.00000001 | Limit move, the largest one-bar price move the index is scaled against, in price units |

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
double high[252]; /* your data, oldest first */
double low[252]; /* your data, oldest first */
double close[252]; /* your data, oldest first */
double out[252];
int outBegIdx, outNBElement;

TA_Initialize();
TA_SI( 0, n - 1, open, high, low, close, 3.0, &outBegIdx, &outNBElement, out );
printf( "%d values from bar %d; last out = %f\n", outNBElement, outBegIdx, out[outNBElement - 1] );
```

@tab Rust

```rust
use ta_lib::Core;

let open = vec![0.0; 252]; // your data, oldest first
let high = vec![0.0; 252]; // your data, oldest first
let low = vec![0.0; 252]; // your data, oldest first
let close = vec![0.0; 252]; // your data, oldest first
let mut out = vec![0.0; 252];

let core = Core::new();
let range = core.si(0, open.len() - 1, &open, &high, &low, &close, 3.0, &mut out)?;
println!("{} values from bar {}; last out = {}", range.count, range.beg_idx, out[range.count - 1]);
```

@tab Java

```java
import io.github.talib.Core;
import io.github.talib.OutRange;

double[] open = new double[252]; // your data, oldest first
double[] high = new double[252]; // your data, oldest first
double[] low = new double[252]; // your data, oldest first
double[] close = new double[252]; // your data, oldest first
double[] out = new double[252];

Core core = Core.DEFAULT;
OutRange r = core.si(0, open.length - 1, open, high, low, close, 3.0, out);
System.out.println(r.count() + " values from bar " + r.begIdx() + "; last out = " + out[r.count() - 1]);
```

@tab C\#

```csharp
using System;
using TALib;

var open = new double[252]; // your data, oldest first
var high = new double[252]; // your data, oldest first
var low = new double[252]; // your data, oldest first
var close = new double[252]; // your data, oldest first
var outReal = new double[252];

var core = Core.Default;
OutRange r = core.Si(0, open.Length - 1, open, high, low, close, 3.0, outReal);
Console.WriteLine($"{r.Count} values from bar {r.BegIdx}; last outReal = {outReal[r.Count - 1]}");
```

@tab TypeScript

```ts
import { si } from 'ta-lib';

const open = new Float64Array(252); // your data, oldest first
const high = new Float64Array(252); // your data, oldest first
const low = new Float64Array(252); // your data, oldest first
const close = new Float64Array(252); // your data, oldest first

const { begIdx, values } = si({ open, high, low, close }, { limitMove: 3 });
console.log(values.length + ' values from bar ' + begIdx + '; last values = ' + values[values.length - 1]);
```

:::

## Implementation

Added in 0.8.3 (ta-lib-python 0.8.3).

TA-Lib Definition: [`si.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/si/si.c) · [`si.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/si/si.yaml)

| Native | File |
|--------|------|
| C | [`ta_SI.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_SI.c) |
| Rust | [`si.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/library/src/ta_func/si.rs) |
| Java | [`Core_SI.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/fragments/Core_SI.java) |
| C# | [`Core_SI.cs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/csharp/library/src/Core_SI.cs) |
| TypeScript | [`si.ts`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/ts/library/src/func/si.ts) |

Python: `talib.SI` (ta-lib-python 0.8.3 or later), see [ta-lib-python](https://ta-lib.github.io/ta-lib-python/func_groups/momentum_indicators.html).

TA-Lib is also available for R and more using a [wrapper](/install/#wrappers).

## See Also

[ASI](/functions/asi.md) · [WAD](/functions/wad.md) · [TRANGE](/functions/trange.md) · [BOP](/functions/bop.md)

## References

- J. Welles Wilder Jr., *New Concepts in Technical Trading Systems*, Trend Research, 1978, section VIII "The Swing Index", pp.87-106
