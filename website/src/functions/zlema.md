---
title: "Zero-Lag Exponential Moving Average (ZLEMA)"
description: "Zero-Lag Exponential Moving Average: an EMA applied not to price but to a de-lagged series that extrapolates price forward by the EMA's own lag…"
---

## Summary

Zero-Lag Exponential Moving Average: an [`EMA`](/functions/ema.md) applied not to price but to a de-lagged series that extrapolates price forward by the EMA's own lag, cancelling that lag to first order. It tracks a trend far more closely than an EMA of the same length, at the cost of overshooting sharp reversals — the extrapolation keeps pushing in the old direction for a bar or two. Read it as an EMA that turns sooner: crossings of price and average, and changes in its slope, arrive earlier than the equivalent EMA signal, and its overshoot after a spike is a property of the filter rather than a move in the market.

ZLEMA is also selectable as a moving-average type (`TA_MAType_ZLEMA`) wherever an `optInMAType` parameter is accepted ([`MA`](/functions/ma.md), [`BBANDS`](/functions/bbands.md), [`STOCH`](/functions/stoch.md), [`MACDEXT`](/functions/macdext.md), ...).

## Chart

<ta-chart src="/functions/zlema.svg" ratio="720:274" label="ZLEMA over IBM, daily, 1999" alt="Drawn on the price pane over the close as a line, with outReal as a solid line."></ta-chart>

::: details Chart metadata <span class="flag-tip" tabindex="0" role="note" aria-label="The chart is drawn from the function's metadata: the flags the abstract API reports for it and for each output, and the input series its reference run declares." data-tip="The chart is drawn from the function's metadata: the flags the abstract API reports for it and for each output, and the input series its reference run declares.">i</span>

Drawn on the price pane over the close as a line, with outReal as a solid line.

Metadata flags used: `overlap` · `outReal: line`

:::

[Reference data (CSV)](/functions/zlema.csv) · [Chart (SVG)](/functions/zlema.svg)

## Formula

lag = Integer( (n - 1) / 2 )
d = 2 * Price - Price[lag bars ago]
ZLEMA(n) = EMA( d, n )

The inner average is the standard TA-Lib EMA: smoothing factor 2 / (n + 1), seeded with the simple average of the first n de-lagged values.

## Notes

- **The paper this indicator is usually credited to describes a different filter.** Ehlers and Way's *Zero Lag (Well, Almost)* specifies an error-correcting EMA with a per-bar gain search; neither the de-lagged series nor the `(n-1)/2` lag appears anywhere in it. What TA-Lib ships here is the de-lagged-EMA construction published under the "zero lag" name by Tulip Indicators, pandas-ta, TradingView Pine and others, for which no primary source is traceable.
- `lag` **truncates**: `Integer((n-1)/2)`. For an even period that is one bar shorter than the round-to-nearest convention some descriptions use, which moves the whole line, not just its warm-up. Tulip Indicators, pandas-ta and Pine all truncate. R's TTR instead takes the lag as `(n+1)/2` without truncating, and for an even period interpolates between the two bars either side of it: a de-lag one to one and a half bars longer than this one, so its line stays apart from these values however long the series runs.
- The de-lag is computed as `2 * Price - Price[lag]` in one rounding, rather than the algebraically equal `Price + (Price - Price[lag])` that Tulip Indicators, TradingView Pine and the Wikipedia statement use. The second form's extra rounding is one unit in the last place of the larger price — negligible against the de-lagged value, except where that value nearly cancels. When price is near double its value `lag` bars ago the two forms differ by about 5e-12 relative, so expect that much disagreement against those implementations on a strongly trending series, and do not attribute it to the seed or the smoothing factor.
- Implementations disagree on how the inner EMA is seeded — TA-Lib uses its own EMA convention (the simple average of the first `n` de-lagged values), where Tulip Indicators seeds from a single raw price and so emits its first value earlier and converges to these values only after many bars.
- ZLEMA inherits EMA's unstable period rather than owning one: `TA_SetUnstablePeriod(TA_FUNC_UNST_EMA, ...)` moves ZLEMA's first output too.
- A period of 1 performs no smoothing: the output is a copy of the input.

## Inputs

- `inReal` — Source price series, close by convention

## Outputs

- `outReal` — Zero-lag exponential moving average of the input

## Parameters

| Parameter | Type | Default | Accepted values | Description |
| --- | --- | --- | --- | --- |
| `optInTimePeriod` | integer | 30 | 1–100000 | Number of bars in the exponential average; the de-lag distance derives from it |

## Properties

**Numerical Stability:** [Initial Unstable Period](/functions/stability.md#initial-unstable-period) — Inherited from EMA, which ZLEMA computes internally; tunable via EMA's unstable period.

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
TA_ZLEMA( 0, n - 1, series, 30, &outBegIdx, &outNBElement, out );
printf( "%d values from bar %d; last out = %f\n", outNBElement, outBegIdx, out[outNBElement - 1] );
```

@tab Rust

```rust
use ta_lib::Core;

let series = vec![0.0; 252]; // your data, oldest first
let mut out = vec![0.0; 252];

let core = Core::new();
let range = core.zlema(0, series.len() - 1, &series, 30, &mut out)?;
println!("{} values from bar {}; last out = {}", range.count, range.beg_idx, out[range.count - 1]);
```

@tab Java

```java
import io.github.talib.Core;
import io.github.talib.OutRange;

double[] series = new double[252]; // your data, oldest first
double[] out = new double[252];

Core core = Core.DEFAULT;
OutRange r = core.zlema(0, series.length - 1, series, 30, out);
System.out.println(r.count() + " values from bar " + r.begIdx() + "; last out = " + out[r.count() - 1]);
```

@tab C\#

```csharp
using System;
using TALib;

var series = new double[252]; // your data, oldest first
var outReal = new double[252];

var core = Core.Default;
OutRange r = core.Zlema(0, series.Length - 1, series, 30, outReal);
Console.WriteLine($"{r.Count} values from bar {r.BegIdx}; last outReal = {outReal[r.Count - 1]}");
```

@tab TypeScript

```ts
import { zlema } from 'ta-lib';

const series = new Float64Array(252); // your data, oldest first

const { begIdx, values } = zlema(series, { timePeriod: 30 });
console.log(values.length + ' values from bar ' + begIdx + '; last values = ' + values[values.length - 1]);
```

:::

## Implementation

Added in 0.8.1 (ta-lib-python 0.8.1).

TA-Lib Definition: [`zlema.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/zlema/zlema.c) · [`zlema.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/zlema/zlema.yaml)

| Native | File |
|--------|------|
| C | [`ta_ZLEMA.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_ZLEMA.c) |
| Rust | [`zlema.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/library/src/ta_func/zlema.rs) |
| Java | [`Core_ZLEMA.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/fragments/Core_ZLEMA.java) |
| C# | [`Core_ZLEMA.cs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/csharp/library/src/Core_ZLEMA.cs) |
| TypeScript | [`zlema.ts`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/ts/library/src/func/zlema.ts) |

Python: `talib.ZLEMA` (ta-lib-python 0.8.1 or later), see [ta-lib-python](https://ta-lib.github.io/ta-lib-python/func_groups/overlap_studies.html).

TA-Lib is also available for R and more using a [wrapper](/install/#wrappers).

## Aliases

Zero Lag Exponential Moving Average, Zero-Lag EMA, ZLMA

## See Also

[EMA](/functions/ema.md) · [DEMA](/functions/dema.md) · [TEMA](/functions/tema.md) · [HMA](/functions/hma.md) · [MA](/functions/ma.md)

## References

- John Ehlers and Ric Way, *Zero Lag (Well, Almost)*, Technical Analysis of Stocks & Commodities, November 2010 — the paper the name comes from, describing a different, error-correcting filter: [mesasoftware.com/papers/ZeroLag.pdf](https://www.mesasoftware.com/papers/ZeroLag.pdf)
- Tulip Indicators, *zlema* — an independent implementation of the de-lagged form, differing in its seeding: [tulipindicators.org/zlema](https://tulipindicators.org/zlema)
