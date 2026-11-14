---
title: "Heikin-Ashi Candles (HA)"
description: "Heikin-Ashi candles: an OHLC-to-OHLC transform that replaces each bar with a smoothed synthetic candle."
---

## Summary

Heikin-Ashi candles: an OHLC-to-OHLC transform that replaces each bar with a smoothed synthetic candle. The synthetic close is the bar's four-price average, the synthetic open is the midpoint of the previous synthetic candle's own open and close, and the synthetic high and low extend the raw extremes far enough to contain that pair.

Read the result as a trend filter rather than as price: consecutive candles of one colour run longer than on the raw chart, small counter-trend bars are absorbed, and a candle with no shadow on the trend side is the usual continuation cue. The cost is that the synthetic open and close are not tradeable prices and gaps disappear entirely, so orders must still be placed against the raw series.

`HA` is recursive: every candle carries the previous one, so the first candle of a request is seeded from its own bar and its influence halves on each bar that follows.

## Chart

<ta-chart src="/functions/ha.svg" ratio="720:486" label="HA over IBM, daily, 1999" alt="Drawn on the price pane over candles; below it, a candle pane of its own for outHAOpen, outHAHigh, outHALow and outHAClose, each candle coloured by outHAClose against outHAOpen."></ta-chart>

<div class="ta-chart-links">

[Reference data (CSV)](/functions/ha.csv) · [Chart (SVG)](/functions/ha.svg)

</div>

::: details Chart metadata <span class="flag-tip" tabindex="0" role="note" aria-label="The chart is drawn automatically from metadata TA-Lib provides. Your app can get the same through the Abstract API." data-tip="The chart is drawn automatically from metadata TA-Lib provides. Your app can get the same through the Abstract API.">i</span>

**Outputs**

- `outHAOpen`: `line`
- `outHAHigh`: `line` · `upper_limit`
- `outHALow`: `line` · `lower_limit`
- `outHAClose`: `line`

:::

## Formula

HA_close[i] = ( O[i] + H[i] + L[i] + C[i] ) / 4

HA_open[0]  = ( O[0] + C[0] ) / 2
HA_open[i]  = ( HA_open[i-1] + HA_close[i-1] ) / 2

HA_high[i]  = max( H[i], HA_open[i], HA_close[i] )
HA_low[i]   = min( L[i], HA_open[i], HA_close[i] )

## Notes

- The first candle has no predecessor, so its open is seeded with the midpoint of the raw open and close. Other conventions exist — ta4j emits the raw bar unchanged as its first candle — and they differ only while the seed still carries weight.
- Both divisors are exact powers of two, so implementations that scale by `0.5` and `0.25` produce the same doubles as those that divide by 2 and 4.
- The unstable period discards candles of warm-up before the first output, trading history for a smaller residual difference between two requests that start at different bars.
- Averaging four prices of one bar is also what [`AVGPRICE`](/functions/avgprice.md) computes, but it sums them in a different order, so the two can differ in the last bits.

## Inputs

- `inOpen` — Open price of each bar
- `inHigh` — High price of each bar
- `inLow` — Low price of each bar
- `inClose` — Close price of each bar

## Outputs

- `outHAOpen` — Heikin-Ashi open
- `outHAHigh` — Heikin-Ashi high
- `outHALow` — Heikin-Ashi low
- `outHAClose` — Heikin-Ashi close

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
double open[252]; /* your data, oldest first */
double high[252]; /* your data, oldest first */
double low[252]; /* your data, oldest first */
double close[252]; /* your data, oldest first */
double haOpen[252], haHigh[252], haLow[252], haClose[252];
int outBegIdx, outNBElement;

TA_Initialize();
TA_HA( 0, n - 1, open, high, low, close,
       &outBegIdx, &outNBElement, haOpen, haHigh, haLow, haClose );
printf( "%d values from bar %d:\n", outNBElement, outBegIdx );
printf( "haOpen = [%.4f, ..., %.4f]\n", haOpen[0], haOpen[outNBElement - 1] );
printf( "haHigh = [%.4f, ..., %.4f]\n", haHigh[0], haHigh[outNBElement - 1] );
printf( "haLow = [%.4f, ..., %.4f]\n", haLow[0], haLow[outNBElement - 1] );
printf( "haClose = [%.4f, ..., %.4f]\n", haClose[0], haClose[outNBElement - 1] );
```

@tab Rust

```rust
use ta_lib::Core;

let open = vec![0.0; 252]; // your data, oldest first
let high = vec![0.0; 252]; // your data, oldest first
let low = vec![0.0; 252]; // your data, oldest first
let close = vec![0.0; 252]; // your data, oldest first
let mut ha_open = vec![0.0; 252];
let mut ha_high = vec![0.0; 252];
let mut ha_low = vec![0.0; 252];
let mut ha_close = vec![0.0; 252];

let core = Core::new();
let range = core.ha(0, open.len() - 1, &open, &high, &low, &close,
                    &mut ha_open, &mut ha_high, &mut ha_low, &mut ha_close)?;
println!("{} values from bar {}:", range.count, range.beg_idx);
println!("ha_open = [{:.4}, ..., {:.4}]", ha_open[0], ha_open[range.count - 1]);
println!("ha_high = [{:.4}, ..., {:.4}]", ha_high[0], ha_high[range.count - 1]);
println!("ha_low = [{:.4}, ..., {:.4}]", ha_low[0], ha_low[range.count - 1]);
println!("ha_close = [{:.4}, ..., {:.4}]", ha_close[0], ha_close[range.count - 1]);
```

@tab Java

```java
import io.github.talib.Core;
import io.github.talib.OutRange;
import java.util.Locale;

double[] open = new double[252]; // your data, oldest first
double[] high = new double[252]; // your data, oldest first
double[] low = new double[252]; // your data, oldest first
double[] close = new double[252]; // your data, oldest first
double[] haOpen = new double[252];
double[] haHigh = new double[252];
double[] haLow = new double[252];
double[] haClose = new double[252];

Core core = Core.DEFAULT;
OutRange r = core.ha(0, open.length - 1, open, high, low, close, haOpen, haHigh, haLow, haClose);
System.out.printf(Locale.ROOT, "%d values from bar %d:%n", r.count(), r.begIdx());
System.out.printf(Locale.ROOT, "haOpen = [%.4f, ..., %.4f]%n", haOpen[0], haOpen[r.count() - 1]);
System.out.printf(Locale.ROOT, "haHigh = [%.4f, ..., %.4f]%n", haHigh[0], haHigh[r.count() - 1]);
System.out.printf(Locale.ROOT, "haLow = [%.4f, ..., %.4f]%n", haLow[0], haLow[r.count() - 1]);
System.out.printf(Locale.ROOT, "haClose = [%.4f, ..., %.4f]%n", haClose[0], haClose[r.count() - 1]);
```

@tab C\#

```csharp
using System;
using TALib;

var open = new double[252]; // your data, oldest first
var high = new double[252]; // your data, oldest first
var low = new double[252]; // your data, oldest first
var close = new double[252]; // your data, oldest first
var haOpen = new double[252];
var haHigh = new double[252];
var haLow = new double[252];
var haClose = new double[252];

var core = Core.Default;
OutRange r = core.Ha(0, open.Length - 1, open, high, low, close, haOpen, haHigh, haLow, haClose);
Console.WriteLine($"{r.Count} values from bar {r.BegIdx}:");
Console.WriteLine(FormattableString.Invariant($"haOpen = [{haOpen[0]:F4}, ..., {haOpen[r.Count - 1]:F4}]"));
Console.WriteLine(FormattableString.Invariant($"haHigh = [{haHigh[0]:F4}, ..., {haHigh[r.Count - 1]:F4}]"));
Console.WriteLine(FormattableString.Invariant($"haLow = [{haLow[0]:F4}, ..., {haLow[r.Count - 1]:F4}]"));
Console.WriteLine(FormattableString.Invariant($"haClose = [{haClose[0]:F4}, ..., {haClose[r.Count - 1]:F4}]"));
```

@tab TypeScript

```ts
import { ha } from 'ta-lib';

const open = new Float64Array(252); // your data, oldest first
const high = new Float64Array(252); // your data, oldest first
const low = new Float64Array(252); // your data, oldest first
const close = new Float64Array(252); // your data, oldest first

const {
  begIdx,
  outHaOpen,
  outHaHigh,
  outHaLow,
  outHaClose,
} = ha({ open, high, low, close });
console.log(outHaOpen.length + ' values from bar ' + begIdx + ':');
console.log('outHaOpen = [' + outHaOpen[0].toFixed(4) + ', ..., ' + outHaOpen[outHaOpen.length - 1].toFixed(4) + ']');
console.log('outHaHigh = [' + outHaHigh[0].toFixed(4) + ', ..., ' + outHaHigh[outHaHigh.length - 1].toFixed(4) + ']');
console.log('outHaLow = [' + outHaLow[0].toFixed(4) + ', ..., ' + outHaLow[outHaLow.length - 1].toFixed(4) + ']');
console.log('outHaClose = [' + outHaClose[0].toFixed(4) + ', ..., ' + outHaClose[outHaClose.length - 1].toFixed(4) + ']');
```

@tab Go

```go
import (
	"fmt"
	"log"

	talib "ta-lib.github.io/go"
)

open := make([]float64, 252) // your data, oldest first
high := make([]float64, 252) // your data, oldest first
low := make([]float64, 252) // your data, oldest first
close := make([]float64, 252) // your data, oldest first
haOpen := make([]float64, 252)
haHigh := make([]float64, 252)
haLow := make([]float64, 252)
haClose := make([]float64, 252)

core := talib.NewCore()
r, err := core.HA(0, len(open)-1, open, high, low, close, haOpen, haHigh, haLow, haClose)
if err != nil {
	log.Fatal(err)
}
fmt.Printf("%d values from bar %d:\n", r.NBElement, r.BegIdx)
fmt.Printf("haOpen = [%.4f, ..., %.4f]\n", haOpen[0], haOpen[r.NBElement-1])
fmt.Printf("haHigh = [%.4f, ..., %.4f]\n", haHigh[0], haHigh[r.NBElement-1])
fmt.Printf("haLow = [%.4f, ..., %.4f]\n", haLow[0], haLow[r.NBElement-1])
fmt.Printf("haClose = [%.4f, ..., %.4f]\n", haClose[0], haClose[r.NBElement-1])
```

:::

## Implementation

Added in 0.8.1 (ta-lib-python 0.8.1).

TA-Lib Definition: [`ha.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/ha/ha.c) · [`ha.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/ha/ha.yaml)

| Native | File |
|--------|------|
| C | [`ta_HA.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_HA.c) |
| Rust | [`ha.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/library/src/ta_func/ha.rs) |
| Java | [`Core_HA.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/fragments/Core_HA.java) |
| C# | [`Core_HA.cs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/csharp/library/src/Core_HA.cs) |
| TypeScript | [`ha.ts`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/ts/library/src/func/ha.ts) |
| Go | [`ha.go`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/go/ha.go) |

Python: `talib.HA` (ta-lib-python 0.8.1 or later), see [ta-lib-python](https://ta-lib.github.io/ta-lib-python/func_groups/price_transform.html).

TA-Lib is also available for R and more using a [wrapper](/install/#wrappers).

## Aliases

Heikin-Ashi, Heikin Ashi Candles, Heiken Ashi, Average Bar

## See Also

[AVGPRICE](/functions/avgprice.md) · [MEDPRICE](/functions/medprice.md) · [TYPPRICE](/functions/typprice.md) · [WCLPRICE](/functions/wclprice.md)

## References

- [StockCharts ChartSchool, *Heikin-Ashi Candlesticks*](https://chartschool.stockcharts.com/table-of-contents/chart-analysis/chart-types/heikin-ashi-candlesticks) — the four formulas and the seeding convention used here.
- [Investopedia, *Heikin-Ashi Technique*](https://www.investopedia.com/trading/heikin-ashi-better-candlestick/) — interpretation and the trading caveats.
