---
title: "Rolling Excess Kurtosis (KURTOSIS)"
description: "An estimate, from the trailing window, of the excess kurtosis of the distribution its values come from: the fourth standardised moment minus 3, so a…"
---

## Summary

An estimate, from the trailing window, of the excess kurtosis of the distribution its values come from: the fourth standardised moment minus 3, so a normal distribution reads 0. A tail-weight measure: above 0 the distribution has heavier tails than a normal one with the same variance, below 0 lighter ones. The companion to the shipped [`VAR`](/functions/var.md) and [`STDDEV`](/functions/stddev.md) in the same group.

## Chart

<ta-chart src="/functions/kurtosis.svg" ratio="720:416" label="KURTOSIS over IBM, daily, 1999" alt="Drawn on the price pane over the close as a line; below it, a separate indicator pane with a zero line showing outReal as a solid line."></ta-chart>

::: details Chart metadata <span class="flag-tip" tabindex="0" role="note" aria-label="The chart is drawn automatically from metadata TA-Lib provides. Your app can get the same through the Abstract API." data-tip="The chart is drawn automatically from metadata TA-Lib provides. Your app can get the same through the Abstract API.">i</span>

Drawn on the price pane over the close as a line; below it, a separate indicator pane with a zero line showing outReal as a solid line.

Metadata flags used: `outReal: line, positive, negative`

:::

[Reference data (CSV)](/functions/kurtosis.csv) · [Chart (SVG)](/functions/kurtosis.svg)

## Formula

Sample-adjusted Fisher excess kurtosis, the estimator usually written `G2`. With window size `n`, window mean `x̄`, and sample variance `s² = Σ(x−x̄)²/(n−1)`:

    G2 = [ n(n+1) / ((n−1)(n−2)(n−3)) ] · [ Σ(x−x̄)⁴ / s⁴ ]  −  3(n−1)² / ((n−2)(n−3))

The result is NaN when `s² = 0`, that is when every value in the window is equal.

`optInTimePeriod` must be at least 4: the `(n−2)(n−3)` denominators are undefined below it. The bound is enforced by the parameter range, so a shorter period is refused rather than silently degraded.

## Notes

- This is `G2`, the form Excel `KURT` and `scipy.stats.kurtosis(bias=False)` compute. The other form in circulation is the biased `g2 = m₄/m₂² − 3` over population moments, a different estimator rather than a rounding convention: `G2 = (n−1)((n+1)·g2 + 6) / ((n−2)(n−3))`.
- A point mass has no defensible excess kurtosis, so the degenerate window is NaN rather than a number: `0` would assert normality and `−1.2` uniformity. This is deliberately unlike `VAR`, which floors to zero — a variance of zero says something true about the window.
- The result is at most `n`, the value of one reading apart from `n−1` equal ones, so a window dominated by one outlier is legitimately far above 0.

## Inputs

- `inReal` — The series to measure

## Outputs

- `outReal` — Excess kurtosis of the trailing window, or NaN where the window has no spread

## Parameters

| Parameter | Type | Default | Accepted values | Description |
| --- | --- | --- | --- | --- |
| `optInTimePeriod` | integer | 30 | 4–10000 | Number of trailing values in the window |

## Properties

**Numerical Stability:** [Start-Independent](/functions/stability.md#start-independent)

<div class="flag-table">

|  |
| :-- |
| <span class="flag-box">☐</span> <span style="opacity:0.5">Overlap Input</span> |
| <span class="flag-box">✅</span> **Independent Y-Axis** <span class="flag-tip" tabindex="0" role="note" aria-label="Output is on its own scale, drawn in a separate pane below the price chart." data-tip="Output is on its own scale, drawn in a separate pane below the price chart.">i</span> |
| <span class="flag-box">☐</span> <span style="opacity:0.5">Candlestick</span> |
| <span class="flag-box">✅</span> **Can Output NaN or ±Inf** <span class="flag-tip" tabindex="0" role="note" aria-label="Some inputs have no finite result, so a successful call can return NaN or ±Inf — a gap with nothing to plot. See Notes for when." data-tip="Some inputs have no finite result, so a successful call can return NaN or ±Inf — a gap with nothing to plot. See Notes for when.">i</span> |
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
TA_KURTOSIS( 0, n - 1, series, 30, &outBegIdx, &outNBElement, out );
printf( "%d values from bar %d; last out = %f\n", outNBElement, outBegIdx, out[outNBElement - 1] );
```

@tab Rust

```rust
use ta_lib::Core;

let series = vec![0.0; 252]; // your data, oldest first
let mut out = vec![0.0; 252];

let core = Core::new();
let range = core.kurtosis(0, series.len() - 1, &series, 30, &mut out)?;
println!("{} values from bar {}; last out = {}", range.count, range.beg_idx, out[range.count - 1]);
```

@tab Java

```java
import io.github.talib.Core;
import io.github.talib.OutRange;

double[] series = new double[252]; // your data, oldest first
double[] out = new double[252];

Core core = Core.DEFAULT;
OutRange r = core.kurtosis(0, series.length - 1, series, 30, out);
System.out.println(r.count() + " values from bar " + r.begIdx() + "; last out = " + out[r.count() - 1]);
```

@tab C\#

```csharp
using System;
using TALib;

var series = new double[252]; // your data, oldest first
var outReal = new double[252];

var core = Core.Default;
OutRange r = core.Kurtosis(0, series.Length - 1, series, 30, outReal);
Console.WriteLine($"{r.Count} values from bar {r.BegIdx}; last outReal = {outReal[r.Count - 1]}");
```

@tab TypeScript

```ts
import { kurtosis } from 'ta-lib';

const series = new Float64Array(252); // your data, oldest first

const { begIdx, values } = kurtosis(series, { timePeriod: 30 });
console.log(values.length + ' values from bar ' + begIdx + '; last values = ' + values[values.length - 1]);
```

:::

## Implementation

Added in 0.8.3 (ta-lib-python 0.8.3).

TA-Lib Definition: [`kurtosis.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/kurtosis/kurtosis.c) · [`kurtosis.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/kurtosis/kurtosis.yaml)

| Native | File |
|--------|------|
| C | [`ta_KURTOSIS.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_KURTOSIS.c) |
| Rust | [`kurtosis.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/library/src/ta_func/kurtosis.rs) |
| Java | [`Core_KURTOSIS.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/fragments/Core_KURTOSIS.java) |
| C# | [`Core_KURTOSIS.cs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/csharp/library/src/Core_KURTOSIS.cs) |
| TypeScript | [`kurtosis.ts`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/ts/library/src/func/kurtosis.ts) |

Python: `talib.KURTOSIS` (ta-lib-python 0.8.3 or later), see [ta-lib-python](https://ta-lib.github.io/ta-lib-python/func_groups/statistic_functions.html).

TA-Lib is also available for R and more using a [wrapper](/install/#wrappers).

## Aliases

Excess Kurtosis, Sample Excess Kurtosis, KURT

## See Also

[VAR](/functions/var.md) · [STDDEV](/functions/stddev.md) · [BBANDS](/functions/bbands.md)

## References

- Joanes, D. N. and Gill, C. A. "Comparing measures of sample skewness and kurtosis." *Journal of the Royal Statistical Society: Series D*, 47(1), 1998, 183-189 — the `g1/g2`, `G1/G2`, `b1/b2` families and which is unbiased under which assumption.
- NIST/SEMATECH *e-Handbook of Statistical Methods*, §1.3.5.11 — the biased population form.
- Microsoft, *KURT function* — the `G2` form, and `#DIV/0!` for fewer than four points or zero standard deviation.
