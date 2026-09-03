# DONCHIAN

## Summary

Donchian Channels: three overlap lines built from rolling price extrema. The upper band is the highest high and the lower band the lowest low over the period; the middle line is their midpoint. Richard Donchian's original four-week rule — generally credited as the first published systematic trend-following system — buys a break above the high of the preceding weeks and sells a break below their low.

## Formula

Window = the optInTimePeriod bars ending at the current bar

Upper  = Highest High of Window
Lower  = Lowest  Low  of Window
Middle = (Upper + Lower) / 2

## Notes

- The window includes the current bar, matching TradingView (`ta.highest`/`ta.lowest`), NinjaTrader, ta4j, pandas-ta and every other library that ships Donchian Channels.
- A breakout rule compares the current bar against the **previous** bar's band — `High[t] > Upper[t-1]` — which is where the one-bar offset belongs. Reading `Upper[t]` against `High[t]` can never signal, because `High[t]` is inside the window that produced it.
- Upper, Middle and Lower are bit-identical to `MAX(high, N)`, `MIDPRICE(N)` and `MIN(low, N)`. DONCHIAN computes all three in one pass under the name users look for.
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

## Implementation

TA-Lib Definition: [`donchian.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/donchian/donchian.c) · [`donchian.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/donchian/donchian.yaml)

| Native | File |
|--------|------|
| C | [`ta_DONCHIAN.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_DONCHIAN.c) |
| Rust | [`donchian.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/library/src/ta_func/donchian.rs) |
| Java | [`Core_DONCHIAN.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/fragments/Core_DONCHIAN.java) |

TA-Lib is also available for Python, R and more using a [wrapper](/install/#wrappers).
