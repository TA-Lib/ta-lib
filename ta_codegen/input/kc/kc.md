# KC

## Summary

Keltner Channels: three overlap lines around price. The centre line is an exponential moving average of the typical price; the outer bands sit a multiple of the Average True Range above and below it. The band width tracks volatility, so the channel widens in fast markets and narrows in quiet ones.

## Formula

TP = (High + Low + Close) / 3
Middle = EMA(TP, N)
Band = ATR(M)
Upper = Middle + Deviations * Band
Lower = Middle - Deviations * Band

## Notes

- Several incompatible indicators are published under the name "Keltner Channel", disagreeing by percent rather than by rounding. This is the typical-price centre line with a Wilder-smoothed Average True Range band, the form implemented by TTR and ta4j.
- Chester Keltner's 1960 original smooths the typical price with a simple moving average and takes the band from the plain daily range; the widely charted modern variant centres on the close instead. Expect a visible difference against a package plotting either.
- TTR ties the Average True Range period to the centre line's period. Here the two are independent, so the band width can be tuned separately.
- The centre line and the band are separate recursions, each with its own warm-up. They are entered at their own lookbacks, so a caller who wants either one converged sets that function's unstable period — `TA_FUNC_UNST_EMA` for the centre line, `TA_FUNC_UNST_ATR` for the band.

## Inputs

- `inHigh` — High price of each bar
- `inLow` — Low price of each bar
- `inClose` — Close price of each bar

## Outputs

- `outRealUpperBand` — Centre line plus the scaled Average True Range
- `outRealMiddleBand` — Exponential moving average of the typical price
- `outRealLowerBand` — Centre line minus the scaled Average True Range

## Parameters

- `optInTimePeriod` — Smoothing period of the typical price moving average
- `optInATRPeriod` — Smoothing period of the Average True Range
- `optInNbDev` — Multiplier applied to the Average True Range

## Implementation

TA-Lib Definition: [`kc.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/kc/kc.c) · [`kc.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/kc/kc.yaml)

| Native | File |
|--------|------|
| C | [`ta_KC.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_KC.c) |
| Rust | [`kc.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/library/src/ta_func/kc.rs) |
| Java | [`Core_KC.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/fragments/Core_KC.java) |

TA-Lib is also available for Python, R and more using a [wrapper](/install/#wrappers).

## Aliases

Keltner Channel

## See Also

EMA · ATR · TYPPRICE · BBANDS · ACCBANDS

## References

- [Keltner channel](https://en.wikipedia.org/wiki/Keltner_channel)
- [Keltner Channels — StockCharts ChartSchool](https://chartschool.stockcharts.com/table-of-contents/technical-indicators-and-overlays/technical-overlays/keltner-channels)
