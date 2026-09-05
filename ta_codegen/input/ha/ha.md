# HA

## Summary

Heikin-Ashi candles: an OHLC-to-OHLC transform that replaces each bar with a smoothed synthetic candle. The synthetic close is the bar's four-price average, the synthetic open is the midpoint of the previous synthetic candle's own open and close, and the synthetic high and low extend the raw extremes far enough to contain that pair.

Read the result as a trend filter rather than as price: consecutive candles of one colour run longer than on the raw chart, small counter-trend bars are absorbed, and a candle with no shadow on the trend side is the usual continuation cue. The cost is that the synthetic open and close are not tradeable prices and gaps disappear entirely, so orders must still be placed against the raw series.

`HA` is recursive: every candle carries the previous one, so the first candle of a request is seeded from its own bar and its influence halves on each bar that follows.

## Formula

HA_close[i] = ( O[i] + H[i] + L[i] + C[i] ) / 4

HA_open[0]  = ( O[0] + C[0] ) / 2
HA_open[i]  = ( HA_open[i-1] + HA_close[i-1] ) / 2

HA_high[i]  = max( H[i], HA_open[i], HA_close[i] )
HA_low[i]   = min( L[i], HA_open[i], HA_close[i] )

## Notes

- The first candle has no predecessor, so its open is seeded with the midpoint of the raw open and close. Other conventions exist — ta4j emits the raw bar unchanged as its first candle — and they differ only while the seed still carries weight.
- Both divisors are exact powers of two, so implementations that scale by `0.5` and `0.25` produce the same doubles as those that divide by 2 and 4.
- The unstable period discards that many candles of warm-up before the first output, trading history for a smaller residual difference between two requests that start at different bars.
- Averaging four prices of one bar is also what [`AVGPRICE`](/functions/avgprice) computes, but it sums them in a different order, so the two can differ in the last bits.

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

## Implementation

TA-Lib Definition: [`ha.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/ha/ha.c) · [`ha.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/ha/ha.yaml)

| Native | File |
|--------|------|
| C | [`ta_HA.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_HA.c) |
| Rust | [`ha.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/library/src/ta_func/ha.rs) |
| Java | [`Core_HA.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/fragments/Core_HA.java) |

TA-Lib is also available for Python, R and more using a [wrapper](/install/#wrappers).

## Aliases

Heikin-Ashi, Heikin Ashi Candles, Heiken Ashi, Average Bar

## See Also

AVGPRICE · MEDPRICE · TYPPRICE · WCLPRICE

## References

- [StockCharts ChartSchool, *Heikin-Ashi Candlesticks*](https://chartschool.stockcharts.com/table-of-contents/chart-analysis/chart-types/heikin-ashi-candlesticks) — the four formulas and the seeding convention used here.
- [Investopedia, *Heikin-Ashi Technique*](https://www.investopedia.com/trading/heikin-ashi-better-candlestick/) — interpretation and the trading caveats.
