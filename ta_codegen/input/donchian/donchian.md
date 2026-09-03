# DONCHIAN

## Summary

Donchian Channels: three overlap lines built from rolling price extrema. The upper band is the highest high and the lower band the lowest low over the period; the middle line is their midpoint. Richard Donchian's original four-week rule — generally credited as the first published systematic trend-following system — buys a break above the high of the preceding weeks and sells a break below their low.

## Formula

Window = the optInTimePeriod bars ending optInLag bars before the current bar

Upper  = Highest High of Window
Lower  = Lowest  Low  of Window
Middle = (Upper + Lower) / 2

## Notes

- The default `optInLag=1` is the original rule: a breakout is measured against a window the breaking bar is **not** part of. With the current bar inside the window (`optInLag=0`) the upper band can never be crossed upward — `High[t]` is already in the max — which is why StockCharts and IncredibleCharts both document the lagged form.
- `optInLag=0` reproduces the inclusive convention used by TradingView (`ta.highest`/`ta.lowest`), NinjaTrader and pandas-ta. Users arriving from those platforms should pass `optInLag=0` to match their charts; the difference is exactly a one-bar shift.
- At `optInLag=0` the three outputs equal `MAX(high, N)`, `MIN(low, N)` and `MIDPRICE(N)` bit for bit.
- The middle line is the channel midpoint, not a moving average of price.
- No smoothing or recursion is involved, so there is no unstable period: outputs are exact from the first bar.

## Inputs

- `inHigh` — High price of each bar
- `inLow` — Low price of each bar

## Outputs

- `outRealUpperBand` — Highest high of the window
- `outRealMiddleBand` — Midpoint of the upper and lower bands
- `outRealLowerBand` — Lowest low of the window

## Parameters

- `optInTimePeriod` — Number of bars in the extrema window
- `optInLag` — Bars the window is held back from the current bar (0 includes the current bar)

## Implementation

TA-Lib Definition: [`donchian.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/donchian/donchian.c) · [`donchian.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/donchian/donchian.yaml)

| Native | File |
|--------|------|
| C | [`ta_DONCHIAN.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_DONCHIAN.c) |
| Rust | [`donchian.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/library/src/ta_func/donchian.rs) |
| Java | [`Core_DONCHIAN.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/fragments/Core_DONCHIAN.java) |

TA-Lib is also available for Python, R and more using a [wrapper](/install/#wrappers).
