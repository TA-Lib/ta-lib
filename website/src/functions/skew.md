---
title: "Rolling Skewness (SKEW)"
description: "An estimate, from the trailing window, of the skewness of the distribution its values come from: the third standardised moment."
---

## Summary

An estimate, from the trailing window, of the skewness of the distribution its values come from: the third standardised moment. An asymmetry measure: above 0 the distribution has a longer tail above its mean, below 0 a longer tail below it, and a symmetric window reads 0. The companion to [`KURTOSIS`](/functions/kurtosis.md), [`VAR`](/functions/var.md) and [`STDDEV`](/functions/stddev.md) in the same group.

## Chart

<ta-chart src="/functions/skew.svg" ratio="720:416" label="SKEW over IBM, daily, 1999" alt="Drawn on the price pane over the close as a line; below it, a separate indicator pane with a zero line showing outReal as a solid line."></ta-chart>

Drawn on the price pane over the close as a line; below it, a separate indicator pane with a zero line showing outReal as a solid line. [Reference data (CSV)](/functions/skew.csv) · [Chart (SVG)](/functions/skew.svg)

## Formula

Adjusted Fisher-Pearson coefficient of skewness, the estimator usually written `G1`. With window size `n`, window mean `x̄` and population moments `mₖ = Σ(x−x̄)ᵏ / n`:

    g1 = m₃ / m₂^(3/2)

    G1 = g1 · √(n(n−1)) / (n−2)

The result is 0 when `m₂ = 0`, that is when every value in the window is equal: a point mass is symmetric.

`optInTimePeriod` must be at least 3: the `n−2` denominator is undefined below it. The bound is enforced by the parameter range, so a shorter period is refused rather than silently degraded.

## Notes

- This is `G1`, the form Excel `SKEW`, `scipy.stats.skew(bias=False)` and R `e1071::skewness(type = 2)` compute. Two other estimators are in circulation, each a constant multiple of `g1` rather than a rounding convention: `g1` itself (`scipy.stats.skew(bias=True)`) and `b1 = g1 · ((n−1)/n)^(3/2)` (`type = 3`).
- One reading apart from `n−1` equal ones gives exactly `√n`, above the rest when the odd one is above and `−√n` when it is below. That is the largest magnitude a window can reach, so a window dominated by one outlier is legitimately far from 0.

## Inputs

- `inReal` — The series to measure

## Outputs

- `outReal` — Skewness of the trailing window, or 0 where the window has no spread

## Parameters

| Parameter | Type | Default | Accepted values | Description |
| --- | --- | --- | --- | --- |
| `optInTimePeriod` | integer | 30 | 3–100000 | Number of trailing values in the window |

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
double series[252]; /* your data, oldest first */
double out[252];
int outBegIdx, outNBElement;

TA_Initialize();
TA_SKEW( 0, n - 1, series, 30, &outBegIdx, &outNBElement, out );
printf( "%d values from bar %d; last out = %f\n", outNBElement, outBegIdx, out[outNBElement - 1] );
```

@tab Rust

```rust
use ta_lib::Core;

let series = vec![0.0; 252]; // your data, oldest first
let mut out = vec![0.0; 252];

let core = Core::new();
let range = core.skew(0, series.len() - 1, &series, 30, &mut out)?;
println!("{} values from bar {}; last out = {}", range.count, range.beg_idx, out[range.count - 1]);
```

@tab Java

```java
import io.github.talib.Core;
import io.github.talib.OutRange;

double[] series = new double[252]; // your data, oldest first
double[] out = new double[252];

Core core = Core.DEFAULT;
OutRange r = core.skew(0, series.length - 1, series, 30, out);
System.out.println(r.count() + " values from bar " + r.begIdx() + "; last out = " + out[r.count() - 1]);
```

@tab C\#

```csharp
using System;
using TALib;

var series = new double[252]; // your data, oldest first
var outReal = new double[252];

var core = Core.Default;
OutRange r = core.Skew(0, series.Length - 1, series, 30, outReal);
Console.WriteLine($"{r.Count} values from bar {r.BegIdx}; last outReal = {outReal[r.Count - 1]}");
```

@tab TypeScript

```ts
import { skew } from 'ta-lib';

const series = new Float64Array(252); // your data, oldest first

const { begIdx, values } = skew(series, { timePeriod: 30 });
console.log(values.length + ' values from bar ' + begIdx + '; last values = ' + values[values.length - 1]);
```

:::

## Implementation

Added in 0.8.4.

TA-Lib Definition: [`skew.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/skew/skew.c) · [`skew.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/skew/skew.yaml)

| Native | File |
|--------|------|
| C | [`ta_SKEW.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_SKEW.c) |
| Rust | [`skew.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/library/src/ta_func/skew.rs) |
| Java | [`Core_SKEW.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/fragments/Core_SKEW.java) |
| C# | [`Core_SKEW.cs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/csharp/library/src/Core_SKEW.cs) |
| TypeScript | [`skew.ts`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/ts/library/src/func/skew.ts) |

Python: `talib.SKEW` (ta-lib-python 0.8.4 or later), see [ta-lib-python](https://ta-lib.github.io/ta-lib-python/func_groups/statistic_functions.html).

TA-Lib is also available for R and more using a [wrapper](/install/#wrappers).

## Aliases

Skewness, Sample Skewness, Fisher-Pearson Skewness

## See Also

[KURTOSIS](/functions/kurtosis.md) · [VAR](/functions/var.md) · [STDDEV](/functions/stddev.md)

## References

- Joanes, D. N. and Gill, C. A. "Comparing measures of sample skewness and kurtosis." *Journal of the Royal Statistical Society: Series D*, 47(1), 1998, 183-189 — the `g1`, `G1`, `b1` family and which is unbiased under which assumption.
- NIST/SEMATECH *e-Handbook of Statistical Methods*, §1.3.5.11 — the Fisher-Pearson coefficient and its adjusted form.
- Microsoft, *SKEW function* — the `G1` form.
