---
title: "Price Volume Trend (PVT)"
description: "Price Volume Trend: a running cumulative total of each bar's volume weighted by that bar's fractional price change."
---

## Summary

Price Volume Trend: a running cumulative total of each bar's volume weighted by that bar's fractional price change. It is the On Balance Volume idea with partial credit — where OBV adds or subtracts a bar's whole volume on the sign of the move, PVT adds only the fraction of that volume proportional to the size of the move.

Read the slope and the divergences, not the level: the total's zero point is arbitrary, so only the shape of the curve carries information. A rising PVT while price is flat says volume is accumulating on the up moves; a falling PVT while price rises is the classic bearish divergence.

## Formula

    PVT[i] = PVT[i-1] + inVolume[i] * (inClose[i] - inClose[i-1]) / inClose[i-1]

The series starts at zero on the first bar of the requested range.

## Notes

- The absolute level is arbitrary and depends on where the accumulation started, so two ranges over the same data give curves of the same shape at different offsets.
- Some libraries scale the per-bar term by 100. This implementation follows the fractional definition, which every primary reference below states.
- A bar whose previous close is exactly zero contributes nothing and the running total is carried forward unchanged, rather than dividing by zero. A flat stretch of the output can therefore mean either genuine zero net accumulation or a run of zero previous closes.

## Inputs

- `inClose` — Close price of each bar
- `inVolume` — Volume of each bar

## Outputs

- `outReal` — Cumulative price volume trend, seeded at zero

## Properties

**Numerical Stability:** [Path-Dependent](/functions/stability.md#path-dependent)

<div class="flag-table">

|  |
| :-- |
| <span class="flag-box">☐</span> <span style="opacity:0.5">Overlap Input</span> |
| <span class="flag-box">✅</span> **Independent Y-Axis** <span class="flag-tip" tabindex="0" role="note" aria-label="Output is on its own scale, drawn in a separate pane below the price chart." data-tip="Output is on its own scale, drawn in a separate pane below the price chart.">i</span> |
| <span class="flag-box">☐</span> <span style="opacity:0.5">Candlestick</span> |
| <span class="flag-box">☐</span> <span style="opacity:0.5">Can Output NaN or ±Inf</span> |
| <span class="flag-box">☐</span> <span style="opacity:0.5">Identity at Period 1</span> |

</div>

## Implementation

TA-Lib Definition: [`pvt.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/pvt/pvt.c) · [`pvt.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/pvt/pvt.yaml)

| Native | File |
|--------|------|
| C | [`ta_PVT.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_PVT.c) |
| Rust | [`pvt.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/library/src/ta_func/pvt.rs) |
| Java | [`Core_PVT.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/fragments/Core_PVT.java) |

TA-Lib is also available for Python, R and more using a [wrapper](/install/#wrappers).

## Aliases

Price Volume Trend, Volume Price Trend, VPT

## See Also

[OBV](/functions/obv.md) · [NVI](/functions/nvi.md) · [PVI](/functions/pvi.md) · [PVO](/functions/pvo.md) · [AD](/functions/ad.md)

## References

- [Wikipedia: Volume–price trend](https://en.wikipedia.org/wiki/Volume%E2%80%93price_trend)
- [TradingView: Price Volume Trend (PVT)](https://www.tradingview.com/support/solutions/43000502345-price-volume-trend-pvt/)
