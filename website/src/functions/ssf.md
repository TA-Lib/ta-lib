---
title: "Ehlers SuperSmoother Filter (SSF)"
description: "John Ehlers' SuperSmoother: a two-pole low-pass smoother with a zero at Nyquist, which he recommends for use instead of an EMA and builds most of his…"
---

## Summary

John Ehlers' SuperSmoother: a two-pole low-pass smoother with a zero at Nyquist, which he recommends for use instead of an EMA and builds most of his later indicators on. Read it as a smoothed price line with less lag than an EMA of comparable smoothness.

Its DC gain is 1, so a flat market returns the price itself and the line sits on the chart with price. The critical period is the wavelength that divides the pass band from the reject band: cycles longer than it pass, shorter ones fall off at 12 dB per octave, and a two-bar alternation is removed entirely. The poles are a complex pair, which no EMA of an EMA and none of the `SWAK_*` rows can place.

## Chart

<ta-chart src="/functions/ssf.svg" ratio="720:274" label="SSF over IBM, daily, 1999" alt="Drawn on the price pane over the close as a line, with outReal as a solid line."></ta-chart>

<div class="ta-chart-links">

[Reference data (CSV)](/functions/ssf.csv) · [Chart (SVG)](/functions/ssf.svg)

</div>

::: details Chart metadata <span class="flag-tip" tabindex="0" role="note" aria-label="The chart is drawn automatically from metadata TA-Lib provides. Your app can get the same through the Abstract API." data-tip="The chart is drawn automatically from metadata TA-Lib provides. Your app can get the same through the Abstract API.">i</span>

**Outputs**

- `outReal`: `line`

:::

## Formula

```
P  = optInTimePeriod
a1 = exp(-1.414 * 3.14159 / P)
c2 = 2 * a1 * cos(1.414 * pi / P)
c3 = -a1^2
c1 = 1 - c2 - c3

y[i] = c1 * (x[i] + x[i-1]) / 2 + c2 * y[i-1] + c3 * y[i-2]
```

The constants are Ehlers' own: `1.414` and `3.14159` in the radius, and a cosine he takes in degrees as `cos(1.414 * 180 / P)`. The first three bars of the history seed the filter with the price; the first output is a recursion step, at bar `max(P - 1, 3)`.

## Notes

- A period of 1 performs no smoothing: the output is a copy of the input, consistent with `MA(period=1)` for every MAType.
- Ehlers' 2004 form averages no bars in the numerator (`c1 * x[i]`, no zero at Nyquist); he replaced it by 2013, and implementations still computing it differ from this function by up to a few percent.
- Implementations with an exact `sqrt(2)` and `pi` in place of Ehlers' constants differ from this function in about the fifth significant digit.

## Inputs

- `inReal` — The series to smooth; Ehlers' examples use the close

## Outputs

- `outReal` — The smoothed line, on the same scale as the input

## Parameters

| Parameter | Type | Default | Accepted values | Description |
| --- | --- | --- | --- | --- |
| `optInTimePeriod` | integer | 10 | 1–10000 | Critical period: the wavelength that divides the pass band and the reject band |

## Properties

**Numerical Stability:** [Initial Unstable Period](/functions/stability.md#initial-unstable-period)

<div class="flag-cols">

<div class="flag-table">

|  |
| :-- |
| <span class="flag-box">✅</span> **Overlap Input** <span class="flag-tip" tabindex="0" role="note" aria-label="Output is on the same scale as the input price, so it is drawn over the price chart." data-tip="Output is on the same scale as the input price, so it is drawn over the price chart.">i</span> |
| <span class="flag-box">☐</span> <span style="opacity:0.5">Independent Y-Axis</span> |
| <span class="flag-box">☐</span> <span style="opacity:0.5">Candlestick</span> |
| <span class="flag-box">☐</span> <span style="opacity:0.5">Display Shift</span> |

</div>

<div class="flag-table">

|  |
| :-- |
| <span class="flag-box">☐</span> <span style="opacity:0.5">Can Output NaN or ±Inf</span> |
| <span class="flag-box">✅</span> **Identity at Period 1** <span class="flag-tip" tabindex="0" role="note" aria-label="A period of 1 performs no smoothing: every output value is a bit-exact copy of its input value." data-tip="A period of 1 performs no smoothing: every output value is a bit-exact copy of its input value.">i</span> |
| <span class="flag-box">✅</span> **Uses Transcendental** <span class="flag-tip" tabindex="0" role="note" aria-label="Calls a transcendental math function (exp, log, log10, trigonometric, inverse trigonometric or hyperbolic), so a value may differ slightly between languages, between a stream and a batch call, and between machines. TA-Lib targets a difference below 1e-9." data-tip="Calls a transcendental math function (exp, log, log10, trigonometric, inverse trigonometric or hyperbolic), so a value may differ slightly between languages, between a stream and a batch call, and between machines. TA-Lib targets a difference below 1e-9.">i</span> |
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
TA_SSF( 0, n - 1, series, 10, &outBegIdx, &outNBElement, out );
printf( "%d values from bar %d: [%.4f, ..., %.4f]\n", outNBElement, outBegIdx, out[0], out[outNBElement - 1] );
```

@tab Rust

```rust
use ta_lib::Core;

let series = vec![0.0; 252]; // your data, oldest first
let mut out = vec![0.0; 252];

let core = Core::new();
let range = core.ssf(0, series.len() - 1, &series, 10, &mut out)?;
println!("{} values from bar {}: [{:.4}, ..., {:.4}]", range.count, range.beg_idx, out[0], out[range.count - 1]);
```

@tab Java

```java
import io.github.talib.Core;
import io.github.talib.OutRange;
import java.util.Locale;

double[] series = new double[252]; // your data, oldest first
double[] out = new double[252];

Core core = Core.DEFAULT;
OutRange r = core.ssf(0, series.length - 1, series, 10, out);
System.out.printf(Locale.ROOT, "%d values from bar %d: [%.4f, ..., %.4f]%n", r.count(), r.begIdx(), out[0], out[r.count() - 1]);
```

@tab C\#

```csharp
using System;
using TALib;

var series = new double[252]; // your data, oldest first
var outReal = new double[252];

var core = Core.Default;
OutRange r = core.Ssf(0, series.Length - 1, series, 10, outReal);
Console.WriteLine(FormattableString.Invariant($"{r.Count} values from bar {r.BegIdx}: [{outReal[0]:F4}, ..., {outReal[r.Count - 1]:F4}]"));
```

@tab TypeScript

```ts
import { ssf } from 'ta-lib';

const series = new Float64Array(252); // your data, oldest first

const { begIdx, values } = ssf(series, { timePeriod: 10 });
console.log(values.length + ' values from bar ' + begIdx + ': [' + values[0].toFixed(4) + ', ..., ' + values[values.length - 1].toFixed(4) + ']');
```

@tab Go

```go
import (
	"fmt"
	"log"

	talib "ta-lib.github.io/go"
)

series := make([]float64, 252) // your data, oldest first
out := make([]float64, 252)

core := talib.NewCore()
r, err := core.SSF(0, len(series)-1, series, 10, out)
if err != nil {
	log.Fatal(err)
}
fmt.Printf("%d values from bar %d: [%.4f, ..., %.4f]\n", r.NBElement, r.BegIdx, out[0], out[r.NBElement-1])
```

:::

## Implementation

Added in 0.8.4.

TA-Lib Definition: [`ssf.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/ssf/ssf.c) · [`ssf.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/ssf/ssf.yaml)

| Native | File |
|--------|------|
| C | [`ta_SSF.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_SSF.c) |
| Rust | [`ssf.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/library/src/ta_func/ssf.rs) |
| Java | [`Core_SSF.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/fragments/Core_SSF.java) |
| C# | [`Core_SSF.cs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/csharp/library/src/Core_SSF.cs) |
| TypeScript | [`ssf.ts`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/ts/library/src/func/ssf.ts) |
| Go | [`ssf.go`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/go/ssf.go) |

Python: `talib.SSF`, see [ta-lib-python](https://ta-lib.github.io/ta-lib-python/func_groups/overlap_studies.html).

TA-Lib is also available for R and more using a [wrapper](/install/#wrappers).

## Aliases

SuperSmoother, Super Smoother, Ehlers SuperSmoother, SuperSmoother Filter

## See Also

[EMA](/functions/ema.md) · [SWAK_BUTTER](/functions/swak_butter.md) · [SWAK_GAUSS](/functions/swak_gauss.md) · [MA](/functions/ma.md)

## References

- Ehlers, John F. [The Ultimate Smoother](https://www.mesasoftware.com/papers/UltimateSmoother.pdf) (2024), Code Listing 1, `$SuperSmoother`.
- Ehlers, John F. "Predictive and Successful Indicators." *Technical Analysis of Stocks & Commodities* (January 2014).
- Ehlers, John F. *Cybernetic Analysis for Stocks and Futures*, Wiley (2004), chapter 13.
