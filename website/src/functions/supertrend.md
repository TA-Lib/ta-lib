---
title: "SuperTrend (SUPERTREND)"
description: "An ATR-scaled trailing band that follows price on one side at a time and flips to the other side when the close breaks through it."
---

## Summary

An ATR-scaled trailing band that follows price on one side at a time and flips to the other side when the close breaks through it. The trend rides the lower band while it is up and the upper band while it is down, so the line is usually below price in an uptrend and above it in a downtrend, and the flip is the signal. Attributed to Olivier Seban.

## Formula

Median = (High + Low) / 2
BasicUpper = Median + Multiplier * ATR(TimePeriod)
BasicLower = Median - Multiplier * ATR(TimePeriod)

Upper = BasicUpper, when BasicUpper < previous Upper or previous Close > previous Upper; otherwise the previous Upper
Lower = BasicLower, when BasicLower > previous Lower or previous Close < previous Lower; otherwise the previous Lower

SuperTrend = Lower while the trend is up, until Close < Lower flips it down
SuperTrend = Upper while the trend is down, until Close > Upper flips it up

## Notes

- Both bands are carried forward on every bar, and the trend is decided against the current bar's band. This is the form Investopedia, TradingView and ta4j all describe. A second published form, from the AmiBroker script attributed to Seban, carries only the band the trend is riding and lets the other float free; the two agree on almost every bar and part company at a flip, where this form hands back a band it has been carrying all along and that one hands back a fresh value.
- The recurrence has no value before the first bar it can be computed on, so the trend is seeded up there and both bands take their unclamped value. Published implementations are split on that seed; this is ta4j's. The choice stays visible for as long as the first trend lasts, it never washes out on a series whose close never leaves the band, and it is why the same bar computed from a later start index can differ.
- The direction is reported as +1 for an uptrend and -1 for a downtrend, the sign every other signed output in this library uses for bullish. TradingView's built-in `ta.supertrend` returns the opposite signs for the same two states, and seeds the other way; a strategy ported from Pine has to swap them.
- The two carried bands are released by different conditions, so nothing keeps the lower one below the upper one. Where they cross, a flip leaves the line on the far side of the close, and within a single trend the line can widen away from price instead of tightening toward it. Both are ordinary consequences of the published formula, not a variation on it; they are simply not the guarantees the indicator is usually described as giving.
- A multiplier of zero is degenerate but defined: the two basic bands collapse onto the median price exactly. The carried bands do not collapse with them — each is still released by its own condition — so they coincide on some bars and differ on others, and the trend flips on most bars rather than on every one.
- The band inherits the Average True Range's warm-up, so a caller who wants it converged sets `TA_FUNC_UNST_ATR`, exactly as when calling that function directly.

## Inputs

- `inHigh` — High price of each bar
- `inLow` — Low price of each bar
- `inClose` — Close price of each bar

## Outputs

- `outReal` — The SuperTrend line: the band the trend is currently riding
- `outInteger` — Trend direction: +1 while the trend rides the lower band, -1 while it rides the upper one

## Parameters

| Parameter | Type | Default | Accepted values | Description |
| --- | --- | --- | --- | --- |
| `optInTimePeriod` | integer | 10 | 2–100000 | Smoothing period of the Average True Range |
| `optInMultiplier` | real | 3 | ≥ 0 | Multiplier applied to the Average True Range to set the band width |

## Properties

**Numerical Stability:** [Path-Dependent](/functions/stability.md#path-dependent) — It also computes ATR internally, so ATR's unstable period governs how many leading values are discarded.

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

TA-Lib Definition: [`supertrend.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/supertrend/supertrend.c) · [`supertrend.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/supertrend/supertrend.yaml)

| Native | File |
|--------|------|
| C | [`ta_SUPERTREND.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_SUPERTREND.c) |
| Rust | [`supertrend.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/library/src/ta_func/supertrend.rs) |
| Java | [`Core_SUPERTREND.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/fragments/Core_SUPERTREND.java) |

TA-Lib is also available for Python, R and more using a [wrapper](/install/#wrappers).

## Aliases

Super Trend, Supertrend Indicator

## See Also

[ATR](/functions/atr.md) · [MEDPRICE](/functions/medprice.md) · [SAR](/functions/sar.md) · [SAREXT](/functions/sarext.md) · [KC](/functions/kc.md)

## References

- [Supertrend — TradingView](https://www.tradingview.com/support/solutions/43000634738-supertrend/)
- [Supertrend Indicator — Investopedia](https://www.investopedia.com/supertrend-indicator-7976167)
- Olivier Seban, *Tout le monde mérite d'être riche*, Maxima
