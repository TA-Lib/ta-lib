---
title: "Chaikin's Volatility (CVI)"
description: "Chaikin's Volatility: Marc Chaikin's reading of how fast a market's daily trading range is widening or narrowing."
---

## Summary

Chaikin's Volatility: Marc Chaikin's reading of how fast a market's daily trading range is widening or narrowing. The high-low spread is smoothed by an exponential moving average, and the indicator reports the percent that average has changed over a lookback of its own.

Read it as a rate of expansion. Positive means the smoothed range is wider than it was; negative means it has contracted. Chaikin's own interpretation is contrarian on the fast side: a range that widens sharply over a short span is typical of the panic near a market bottom, while a range that narrows steadily over a long span is typical of a market topping out. It measures range, not direction, so it says nothing about which way price is heading.

## Formula

HL = high - low

E = EMA( HL, optInTimePeriod )

CVI = 100 * ( E - E[optInROCPeriod bars ago] ) / E[optInROCPeriod bars ago]

The inner average is the standard TA-Lib EMA: smoothing factor 2 / (optInTimePeriod + 1), seeded with the simple average of the first optInTimePeriod spreads.

## Notes

- The averaging length and the rate-of-change length are independent, as in Achelis's relay of the author ("an exponential moving average of the difference between the daily high and low prices ... then the percent that this moving average has changed over a specified time period") and in the MathWorks `chaikvolat` signature. Implementations that expose a single length are the special case where both are set to the same value.
- Some vendors default the rate-of-change length to 12 rather than to Achelis's recommendation, which is the same for both lengths.
- A window whose bars are all exactly flat, high equal to low, leaves the lagged average at zero. CVI reports 0 there. Tulip Indicators and pandas-ta-classic leave the division unguarded and emit NaN; trading-signals returns 0, as here.
- Implementations disagree on how the inner EMA is seeded. TA-Lib uses its own EMA convention, the simple average of the first `optInTimePeriod` spreads, where Tulip Indicators and trading-signals seed from a single raw spread and converge to these values only after many bars.
- CVI inherits EMA's unstable period rather than owning one: `TA_SetUnstablePeriod(TA_FUNC_UNST_EMA, ...)` moves CVI's first output too.

## Inputs

- `inHigh` — High price
- `inLow` — Low price

## Outputs

- `outReal` — Percent change of the smoothed high-low spread

## Parameters

| Parameter | Type | Default | Accepted values | Description |
| --- | --- | --- | --- | --- |
| `optInTimePeriod` | integer | 10 | 2–100000 | Number of bars in the exponential average of the high-low spread |
| `optInROCPeriod` | integer | 10 | 1–100000 | How many bars back the percent change reaches |

## Properties

**Numerical Stability:** [Initial Unstable Period](/functions/stability.md#initial-unstable-period) — Inherited from EMA, which CVI computes internally; tunable via EMA's unstable period.

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

TA-Lib Definition: [`cvi.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/cvi/cvi.c) · [`cvi.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/cvi/cvi.yaml)

| Native | File |
|--------|------|
| C | [`ta_CVI.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_CVI.c) |
| Rust | [`cvi.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/library/src/ta_func/cvi.rs) |
| Java | [`Core_CVI.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/fragments/Core_CVI.java) |

TA-Lib is also available for Python, R and more using a [wrapper](/install/#wrappers).

## Aliases

Chaikin Volatility, Chaikin's Volatility, CHV

## See Also

[ATR](/functions/atr.md) · [NATR](/functions/natr.md) · [TRANGE](/functions/trange.md) · [EMA](/functions/ema.md) · [ROCP](/functions/rocp.md)

## References

- Steven B. Achelis, *Technical Analysis from A to Z*, McGraw-Hill, 2nd ed. (pp. 304-305) — Chaikin's description, relayed verbatim
- [MetaStock TAAZ: Volatility, Chaikin's](https://www.metastock.com/customer/resources/taaz/?p=120)
- [MathWorks Financial Toolbox: chaikvolat](https://www.mathworks.com/help/finance/chaikvolat.html) — independent corroboration of the two-length form
- [Tulip Indicators, cvi](https://tulipindicators.org/cvi) — an independent implementation, differing in its seeding and in collapsing the two lengths into one
