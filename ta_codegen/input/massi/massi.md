# MASSI

## Summary

Mass Index: Donald Dorsey's non-directional measure of how the trading range itself is expanding or contracting. The high-low range is smoothed by an exponential moving average, that average is smoothed again by a second one of the same length, and the ratio of the first to the second is summed over a trailing window.

Read it as a bulge detector, not a direction. A ratio above one means the range is widening faster than its own smoothing can absorb, so the sum rises; a narrowing range pulls it back down. Dorsey's own rule is the "reversal bulge": the index rising above 27, then falling back under 26.5, warns that the prevailing trend is about to reverse. Which way it reverses has to come from a trend indicator, because the Mass Index has no sign of its own.

## Formula

HL = high - low

single = EMA( HL, optInFastPeriod )

double = EMA( single, optInFastPeriod )

MASSI = SUM( single / double, optInSlowPeriod )

Both averages are the standard TA-Lib EMA: smoothing factor 2 / (optInFastPeriod + 1), seeded with the simple average of the first optInFastPeriod inputs of that stage.

## Notes

- The two periods are not interchangeable and are never swapped: `optInFastPeriod` is the length of both exponential averages, `optInSlowPeriod` the length of the summation window. Some implementations reorder them when the summation window is the shorter of the two; this one does not.
- A window in which every bar is exactly flat, high equal to low, leaves both averages at zero. The ratio is reported as 1 there, its continuous limit, so a flat market yields exactly `optInSlowPeriod` rather than a spurious zero.
- Implementations disagree on how the exponential averages are seeded. TA-Lib uses its own EMA convention, the simple average of the first `optInFastPeriod` inputs, where Tulip Indicators, ta4j and trading-signals seed from a single raw value and converge to these values only after many bars. Published sample vectors, including the one in Achelis, are seeded that way and match only in the tail.
- MASSI inherits EMA's unstable period rather than owning one, and inherits it twice: `TA_SetUnstablePeriod(TA_FUNC_UNST_EMA, u)` moves the first output by 2u.

## Inputs

- `inHigh` — High price
- `inLow` — Low price

## Outputs

- `outReal` — Summed ratio of the two smoothed high-low ranges

## Parameters

- `optInFastPeriod` — Number of bars in each of the two exponential averages of the high-low range
- `optInSlowPeriod` — Number of bars the ratio is summed over

## Implementation

TA-Lib Definition: [`massi.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/massi/massi.c) · [`massi.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/massi/massi.yaml)

| Native | File |
|--------|------|
| C | [`ta_MASSI.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_MASSI.c) |
| Rust | [`massi.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/library/src/ta_func/massi.rs) |
| Java | [`Core_MASSI.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/fragments/Core_MASSI.java) |

TA-Lib is also available for Python, R and more using a [wrapper](/install/#wrappers).

## Aliases

Mass Index, Dorsey Mass Index, Reversal Bulge

## See Also

CVI · ATR · NATR · TRANGE · EMA · SUM

## References

- Donald Dorsey, "The Mass Index", *Technical Analysis of Stocks & Commodities*, 1992 — the original description
- Steven B. Achelis, *Technical Analysis from A to Z*, McGraw-Hill, 2nd ed. (p. 182)
- [StockCharts ChartSchool: Mass Index](https://school.stockcharts.com/doku.php?id=technical_indicators:mass_index)
- [Tulip Indicators, mass](https://tulipindicators.org/mass) — an independent implementation, differing in its seeding
