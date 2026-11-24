---
title: "Moving Average Envelope (MAENV)"
description: "Moving Average Envelope: a moving average with one band a fixed percentage above it and one a fixed percentage below."
---

## Summary

Moving Average Envelope: a moving average with one band a fixed percentage above it and one a fixed percentage below. The bands keep a constant proportional distance from the average, so price reaching a band reads as stretched relative to its recent mean, and a move beyond one as unusual strength or weakness.

## Chart

<ta-chart src="/functions/maenv.svg" ratio="720:274" label="MAENV over IBM, daily, 1999" alt="Drawn on the price pane over the close as a line, with outRealMiddleBand as a solid line and a band between outRealUpperBand and outRealLowerBand with a soft fill."></ta-chart>

<div class="ta-chart-links">

[Reference data (CSV)](/functions/maenv.csv) · [Chart (SVG)](/functions/maenv.svg)

</div>

::: details Chart metadata <span class="flag-tip" tabindex="0" role="note" aria-label="The chart is drawn automatically from metadata TA-Lib provides. Your app can get the same through the Abstract API." data-tip="The chart is drawn automatically from metadata TA-Lib provides. Your app can get the same through the Abstract API.">i</span>

**Outputs**

- `outRealUpperBand`: `upper_limit`
- `outRealMiddleBand`: `line`
- `outRealLowerBand`: `lower_limit`

:::

## Formula

$$
\begin{aligned}
\text{middle}_t &= \operatorname{MA}(X, n, \text{matype})_t \\
\text{upper}_t &= \text{middle}_t \left(1 + \frac{p_{\text{up}}}{100}\right) \\
\text{lower}_t &= \text{middle}_t \left(1 - \frac{p_{\text{dn}}}{100}\right)
\end{aligned}
$$

where $X$ is the input series, $n$ the period, $\text{matype}$ the moving-average type,
and $p_{\text{up}}$, $p_{\text{dn}}$ the upper and lower percentages.

## Notes

- The percentages are in percent: 10, not 0.1, places a band 10% from the moving average.
- Each band has its own percentage; the usual single-percentage envelope is the case where both are equal. A negative percentage puts its band on the other side of the average.
- **The input is assumed positive.** Each band sits a percent of the moving average away from it. Where the average is 0 both bands equal it, and where it is negative the two bands trade places; no error is returned. A series that reaches or crosses zero, such as an oscillator or a spread, has no meaningful result here.
- A displaced envelope is a shift of the plot by a number of bars. The outputs are not displaced.

## Inputs

- `inReal` — Input data series

## Outputs

- `outRealUpperBand` — The moving average plus the upper percentage of it
- `outRealMiddleBand` — The moving average
- `outRealLowerBand` — The moving average minus the lower percentage of it

## Parameters

| Parameter | Type | Default | Accepted values | Description |
| --- | --- | --- | --- | --- |
| `optInTimePeriod` | integer | 20 | 1–100000 | Periods of the moving average |
| `optInPercentUp` | real | 10 | any real | Percent of the moving average added to form the upper band |
| `optInPercentDn` | real | 10 | any real | Percent of the moving average subtracted to form the lower band |
| `optInMAType` | MAType | SMA (0) | any MAType | Moving-average type for the middle band |

*`MAType` values: 0 SMA · 1 EMA · 2 WMA · 3 DEMA · 4 TEMA · 5 TRIMA · 6 KAMA · 7 MAMA · 8 T3 · 9 HMA · 10 DISABLED · 11 DEFAULT · 12 ZLEMA · 13 RMA · 14 VIDYA · 15 ALMA · 16 SSF*

## Properties

**Numerical Stability:** [Depends on MA Type](/functions/stability.md#depends-on-ma-type) — This function's default, SMA, is start-independent.

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
| <span class="flag-box">☐</span> <span style="opacity:0.5">Identity at Period 1</span> |
| <span class="flag-box">☐</span> <span style="opacity:0.5">Uses Transcendental</span> |
| <span class="flag-box">✅</span> **Input Must Be Positive** <span class="flag-tip" tabindex="0" role="note" aria-label="Assumes the input is positive, as a price is. A series that reaches or crosses zero, such as an oscillator or a spread, returns success and an output that may have no meaning. See Notes for what a zero or a negative value does." data-tip="Assumes the input is positive, as a price is. A series that reaches or crosses zero, such as an oscillator or a spread, returns success and an output that may have no meaning. See Notes for what a zero or a negative value does.">i</span> |

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
double upperBand[252], middleBand[252], lowerBand[252];
int outBegIdx, outNBElement;

TA_Initialize();
TA_MAENV( 0, n - 1, series, 20, 10.0, 10.0, TA_MAType_SMA,
          &outBegIdx, &outNBElement, upperBand, middleBand, lowerBand );
printf( "%d values from bar %d:\n", outNBElement, outBegIdx );
printf( "upperBand = [%.4f, ..., %.4f]\n", upperBand[0], upperBand[outNBElement - 1] );
printf( "middleBand = [%.4f, ..., %.4f]\n", middleBand[0], middleBand[outNBElement - 1] );
printf( "lowerBand = [%.4f, ..., %.4f]\n", lowerBand[0], lowerBand[outNBElement - 1] );
```

@tab Rust

```rust
use ta_lib::{Core, MAType};

let series = vec![0.0; 252]; // your data, oldest first
let mut upper_band = vec![0.0; 252];
let mut middle_band = vec![0.0; 252];
let mut lower_band = vec![0.0; 252];

let core = Core::new();
let range = core.maenv(0, series.len() - 1, &series, 20, 10.0, 10.0, MAType::SMA,
                       &mut upper_band, &mut middle_band, &mut lower_band)?;
println!("{} values from bar {}:", range.count, range.beg_idx);
println!("upper_band = [{:.4}, ..., {:.4}]", upper_band[0], upper_band[range.count - 1]);
println!("middle_band = [{:.4}, ..., {:.4}]", middle_band[0], middle_band[range.count - 1]);
println!("lower_band = [{:.4}, ..., {:.4}]", lower_band[0], lower_band[range.count - 1]);
```

@tab Java

```java
import io.github.talib.Core;
import io.github.talib.MAType;
import io.github.talib.OutRange;
import java.util.Locale;

double[] series = new double[252]; // your data, oldest first
double[] upperBand = new double[252];
double[] middleBand = new double[252];
double[] lowerBand = new double[252];

Core core = Core.DEFAULT;
OutRange r = core.maenv(0, series.length - 1, series, 20, 10.0, 10.0, MAType.SMA,
                        upperBand, middleBand, lowerBand);
System.out.printf(Locale.ROOT, "%d values from bar %d:%n", r.count(), r.begIdx());
System.out.printf(Locale.ROOT, "upperBand = [%.4f, ..., %.4f]%n", upperBand[0], upperBand[r.count() - 1]);
System.out.printf(Locale.ROOT, "middleBand = [%.4f, ..., %.4f]%n", middleBand[0], middleBand[r.count() - 1]);
System.out.printf(Locale.ROOT, "lowerBand = [%.4f, ..., %.4f]%n", lowerBand[0], lowerBand[r.count() - 1]);
```

@tab C\#

```csharp
using System;
using TALib;

var series = new double[252]; // your data, oldest first
var upperBand = new double[252];
var middleBand = new double[252];
var lowerBand = new double[252];

var core = Core.Default;
OutRange r = core.Maenv(0, series.Length - 1, series, 20, 10.0, 10.0, MAType.SMA,
                        upperBand, middleBand, lowerBand);
Console.WriteLine($"{r.Count} values from bar {r.BegIdx}:");
Console.WriteLine(FormattableString.Invariant($"upperBand = [{upperBand[0]:F4}, ..., {upperBand[r.Count - 1]:F4}]"));
Console.WriteLine(FormattableString.Invariant($"middleBand = [{middleBand[0]:F4}, ..., {middleBand[r.Count - 1]:F4}]"));
Console.WriteLine(FormattableString.Invariant($"lowerBand = [{lowerBand[0]:F4}, ..., {lowerBand[r.Count - 1]:F4}]"));
```

@tab TypeScript

```ts
import { MAType, maenv } from 'ta-lib';

const series = new Float64Array(252); // your data, oldest first

const {
  begIdx,
  outUpperBand,
  outMiddleBand,
  outLowerBand,
} = maenv(series, {
  timePeriod: 20,
  percentUp: 10,
  percentDn: 10,
  maType: MAType.SMA,
});
console.log(outUpperBand.length + ' values from bar ' + begIdx + ':');
console.log('outUpperBand = [' + outUpperBand[0].toFixed(4) + ', ..., ' + outUpperBand[outUpperBand.length - 1].toFixed(4) + ']');
console.log('outMiddleBand = [' + outMiddleBand[0].toFixed(4) + ', ..., ' + outMiddleBand[outMiddleBand.length - 1].toFixed(4) + ']');
console.log('outLowerBand = [' + outLowerBand[0].toFixed(4) + ', ..., ' + outLowerBand[outLowerBand.length - 1].toFixed(4) + ']');
```

@tab Go

```go
import (
	"fmt"
	"log"

	talib "ta-lib.github.io/go"
)

series := make([]float64, 252) // your data, oldest first
upperBand := make([]float64, 252)
middleBand := make([]float64, 252)
lowerBand := make([]float64, 252)

core := talib.NewCore()
r, err := core.MAENV(0, len(series)-1, series, 20, 10, 10, talib.MATypeSMA,
	upperBand, middleBand, lowerBand)
if err != nil {
	log.Fatal(err)
}
fmt.Printf("%d values from bar %d:\n", r.NBElement, r.BegIdx)
fmt.Printf("upperBand = [%.4f, ..., %.4f]\n", upperBand[0], upperBand[r.NBElement-1])
fmt.Printf("middleBand = [%.4f, ..., %.4f]\n", middleBand[0], middleBand[r.NBElement-1])
fmt.Printf("lowerBand = [%.4f, ..., %.4f]\n", lowerBand[0], lowerBand[r.NBElement-1])
```

:::

## Implementation

Added in 0.8.4.

TA-Lib Definition: [`maenv.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/maenv/maenv.c) · [`maenv.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/maenv/maenv.yaml)

| Native | File |
|--------|------|
| C | [`ta_MAENV.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_MAENV.c) |
| Rust | [`maenv.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/library/src/ta_func/maenv.rs) |
| Java | [`Core_MAENV.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/fragments/Core_MAENV.java) |
| C# | [`Core_MAENV.cs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/csharp/library/src/Core_MAENV.cs) |
| TypeScript | [`maenv.ts`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/ts/library/src/func/maenv.ts) |
| Go | [`maenv.go`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/go/maenv.go) |

Python: `talib.MAENV`, see [ta-lib-python](https://ta-lib.github.io/ta-lib-python/func_groups/overlap_studies.html).

TA-Lib is also available for R and more using a [wrapper](/install/#wrappers).

## Aliases

Envelope, Envelopes, MA Envelopes, Moving Average Envelopes

## See Also

[MA](/functions/ma.md) · [BBANDS](/functions/bbands.md) · [KC](/functions/kc.md) · [ACCBANDS](/functions/accbands.md)

## References

- [Moving Average Envelopes — StockCharts ChartSchool](https://chartschool.stockcharts.com/table-of-contents/technical-indicators-and-overlays/technical-overlays/moving-average-envelopes)
- [Envelope (ENV) — TradingView Help Center](https://www.tradingview.com/support/solutions/43000502260-envelope-env/)
