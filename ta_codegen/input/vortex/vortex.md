# VORTEX

## Summary

Vortex Indicator: Etienne Botes and Douglas Siepman's two-line trend indicator (*Technical Analysis of Stocks & Commodities* 28:1, January 2010). Positive and negative "vortex movement" — the reach from today's high to yesterday's low and from today's low to yesterday's high — each summed over the period and normalized by the summed true range.

A +VI line crossing above −VI is the bullish signal the authors describe; the two lines are conventionally plotted together.

## Formula

Per bar, `TR[i] = max(H[i]−L[i], |C[i−1]−H[i]|, |C[i−1]−L[i]|)` (exactly [`TRANGE`](/functions/trange)), `VMP[i] = |H[i] − L[i−1]|` and `VMM[i] = |L[i] − H[i−1]|`. Then `+VI = SUM(VMP, n) / SUM(TR, n)` and `−VI = SUM(VMM, n) / SUM(TR, n)`.

No smoothing, no recursion, no seeding — three rolling sums over per-bar terms. Every source (the original TASC article, StockCharts, Wikipedia, TradingView) states the identical formula; the only cross-source difference is the suggested period (14 vs Wikipedia's worked 21). A window whose every bar is flat sums the true range to zero; both lines then emit 0.0, the convention the external implementations share.

## Inputs

- `inHigh` — High price series
- `inLow` — Low price series
- `inClose` — Close price series

## Outputs

- `outPlusVI` — Positive vortex line (+VI)
- `outMinusVI` — Negative vortex line (−VI)

## Parameters

- `optInTimePeriod` — Number of bars in the rolling sums

## Notes

- Bar 0 has no term (all three need a prior bar) and is consumed exactly as [`TRANGE`](/functions/trange) consumes it, so the first output sits at index `optInTimePeriod`, not `optInTimePeriod − 1`.
- Not start-dependent: each output depends only on the finite trailing window. No unstable period.

## Implementation

TA-Lib Definition: [`vortex.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/vortex/vortex.c) · [`vortex.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/vortex/vortex.yaml)

| Native | File |
|--------|------|
| C | [`ta_VORTEX.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_VORTEX.c) |
| Rust | [`vortex.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/library/src/ta_func/vortex.rs) |
| Java | [`Core_VORTEX.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/fragments/Core_VORTEX.java) |

TA-Lib is also available for Python, R and more using a [wrapper](/install/#wrappers).

## Aliases

Vortex Indicator (VI)

## See Also

TRANGE · PLUS_DI · MINUS_DI · ADX

## References

- Etienne Botes and Douglas Siepman, "The Vortex Indicator", *Technical Analysis of Stocks & Commodities* 28:1 (January 2010), pp. 20–30
