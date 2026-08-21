---
title: "Volume Weighted Average Price (VWAP)"
description: "Volume Weighted Average Price: the average price paid per unit of volume traded, accumulated from the first bar of the range onward."
---

## Summary

Volume Weighted Average Price: the average price paid per unit of volume traded, accumulated from the first bar of the range onward. Because every bar is weighted by the volume that traded on it, VWAP tracks where the bulk of the money actually changed hands rather than where the last trade printed. Price above VWAP is read as buyers paying up relative to the session's average cost, price below it as the reverse, which is why execution desks quote fills against it.

VWAP is a running mean, not a moving average: it has no window and no decay, so each new bar carries a smaller share of the total and the line grows steadily more sluggish the further it runs from its anchor. It stays within the range of the typical prices it averages, but over a long trending range it can sit far from the current price.

## Formula

TP_t = ( High_t + Low_t + Close_t ) / 3; VWAP_t = ( Σ TP · Volume ) / ( Σ Volume ), both sums running from the first bar of the range

## Notes

- The sums run from the first bar of the range and are never reset. Charting packages anchor VWAP to a trading session and restart it at each session boundary; no TA-Lib function takes a timestamp or a session boundary, so the anchor is the range the caller asks for — pass one session's bars to get that session's VWAP. This is how AD and OBV, the other cumulative volume functions, are already used across sessions.
- Volume is expected to be non-negative. A zero-volume bar carries no weight, so one occurring after volume has traded leaves the average exactly where it was. Before *any* volume has traded there are no weights at all and the weighted mean is undefined; those bars carry the previous value forward, which is 0 until the first bar with volume. A successful call never emits NaN or ±Inf. Other implementations differ here: pandas-ta-classic divides through and emits NaN, and trading-signals emits no value for the bar at all.
- A bar whose price or volume is not a finite number cannot be weighted, so it is left out of the average entirely and repeats the previous value. It is skipped, not absorbed: the running average stays usable and resumes on the next bar that can be weighted, rather than being held at one stale value for the remainder of the range.

## Inputs

- `inHigh` — High price of each bar
- `inLow` — Low price of each bar
- `inClose` — Close price of each bar
- `inVolume` — Volume of each bar

## Outputs

- `outReal` — Volume weighted average price, cumulative from the first bar of the range

## Properties

**Numerical Stability:** [Path-Dependent](/functions/stability.md#path-dependent)

<div class="flag-table">

|  |
| :-- |
| <span class="flag-box">✅</span> **Overlap Input** <span class="flag-tip" tabindex="0" role="note" aria-label="Output is on the same scale as the input price, so it is drawn over the price chart." data-tip="Output is on the same scale as the input price, so it is drawn over the price chart.">i</span> |
| <span class="flag-box">☐</span> <span style="opacity:0.5">Independent Y-Axis</span> |
| <span class="flag-box">☐</span> <span style="opacity:0.5">Candlestick</span> |
| <span class="flag-box">☐</span> <span style="opacity:0.5">Can Output NaN or ±Inf</span> |
| <span class="flag-box">☐</span> <span style="opacity:0.5">Identity at Period 1</span> |

</div>

## Implementation

TA-Lib Definition: [`vwap.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/vwap/vwap.c) · [`vwap.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/vwap/vwap.yaml)

| Native | File |
|--------|------|
| C | [`ta_VWAP.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_VWAP.c) |
| Rust | [`vwap.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/library/src/ta_func/vwap.rs) |
| Java | [`Core_VWAP.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/fragments/Core_VWAP.java) |
| C# | [`Core_VWAP.cs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/csharp/library/src/Core_VWAP.cs) |

TA-Lib is also available for Python, R and more using a [wrapper](/install/#wrappers).

## Aliases

Volume Weighted Average Price

## See Also

[AD](/functions/ad.md) · [OBV](/functions/obv.md) · [TYPPRICE](/functions/typprice.md) · [VWMA](/functions/vwma.md)

## References

- No single attributable author: VWAP is charting-package convention rather than a published indicator, and the sources below all state the same definition.
- Investopedia, *Volume-Weighted Average Price (VWAP)*: the typical price of each bar weighted by that bar's volume, cumulated over the trading day: [investopedia.com/terms/v/vwap.asp](https://www.investopedia.com/terms/v/vwap.asp)
- StockCharts ChartSchool, *VWAP Intraday*: the same cumulative form, and the source of the session-reset convention: [stockcharts.com/school](https://stockcharts.com/school/doku.php?id=chart_school:technical_indicators:vwap_intraday)
- TradingView, *Volume Weighted Average Price*: `VWAP = cumulative(typical price x volume) / cumulative(volume)`, anchored to the session: [tradingview.com/wiki](https://www.tradingview.com/wiki/Volume_Weighted_Average_Price_%28VWAP%29)
- pandas-ta-classic `overlap/vwap.py` states the core form verbatim in its own docstring, `VWAP = tpv.cumsum() / volume.cumsum()`, and reaches the session reset only by grouping on a `DatetimeIndex`. trading-signals `trend/VWAP` implements the cumulative form with no anchor at all.
