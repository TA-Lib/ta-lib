---
title: "Average Day Range (ADR)"
description: "Average Day Range: the arithmetic mean of the last optInTimePeriod bar ranges, high minus low."
---

## Summary

Average Day Range: the arithmetic mean of the last `optInTimePeriod` bar ranges, high minus low. It answers how far price travels *within* a bar, and is read as a volatility budget — a stop or a target much smaller than ADR is inside the noise the instrument produces on an ordinary bar, one much larger asks for a move that rarely happens.

Same family as ATR, and deliberately the narrower member: the range excludes the overnight gap, so on a gapping instrument ADR is systematically smaller than ATR. Having both is the point.

## Formula

Range_t = High_t - Low_t; ADR_t = ( Σ Range over the last `optInTimePeriod` bars ) / optInTimePeriod

The average is a plain SMA, so there is no seeding convention and none of the cross-library divergence that comes with one.

## Notes

- The mean of the ranges, not the difference of the means. `SMA(high) - SMA(low)` is algebraically the same quantity and is what both TradingView pages spell, but it subtracts two price-magnitude averages to reach a range-magnitude answer and inherits the larger scale's rounding; TC2000's `AVG(H-L, x)` and kand's `SMA(High-Low, period)` spell the form implemented here.
- The "day" is not a calendar day or a trading session. No TA-Lib function takes a timestamp or a session boundary, so the bars the caller passes *are* the days — pass daily bars for a daily range, hourly bars for an hourly one. This is the convention VWAP already ships under.
- `high` below `low` is not rejected. The library validates ranges and parameters, not price sanity, so a bar entered upside down contributes a negative range and the average simply comes out lower, possibly negative, with no error.
- Not the width of a Donchian channel. `MAX(high, n) - MIN(low, n)` is how far the window's extremes lie apart; ADR is the mean of the per-bar ranges, which is smaller whenever the window trends. `DONCHIAN` ships the two extremes that width is built from, not the width itself.
- The request this function answers (`TA-Lib/ta-lib-python#575`) named "Average Day Range" but the freqtrade code behind it computes `MAX(close, 24) - MIN(close, 24)`, a channel width on the closes with no averaging and no high/low. That is a different series and already reachable, as `TA_SUB(TA_MAX(close, 24), TA_MIN(close, 24))`.
- No percentage form is emitted. The two published ones disagree by more than 20% on ordinary data — Qullamaggie's `100 · (SMA(high/low, 20) - 1)` averages ratios, TradingView's `(SMA(high, 14) - SMA(low, 14)) / close · 100` takes a ratio of averages — so picking one silently would ship a second indicator under this one's name.

## Inputs

- `inHigh` — High price of each bar
- `inLow` — Low price of each bar

## Outputs

- `outReal` — Average bar range over the window, in price units

## Parameters

| Parameter | Type | Default | Accepted values | Description |
| --- | --- | --- | --- | --- |
| `optInTimePeriod` | integer | 14 | 1–100000 | Number of bar ranges averaged. Published conventions differ and none is authoritative: TradingView's ADR indicator page works its example over 7 bars, TC2000's over 10, and the Qullamaggie screener community reads "ADR" as 20. The value shipped here is ATR's, so the two volatility measures are comparable out of the box. |

## Properties

**Numerical Stability:** [Start-Independent](/functions/stability.md#start-independent)

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

TA-Lib Definition: [`adr.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/adr/adr.c) · [`adr.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/adr/adr.yaml)

| Native | File |
|--------|------|
| C | [`ta_ADR.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_ADR.c) |
| Rust | [`adr.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/library/src/ta_func/adr.rs) |
| Java | [`Core_ADR.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/fragments/Core_ADR.java) |

TA-Lib is also available for Python, R and more using a [wrapper](/install/#wrappers).

## Aliases

Average Daily Range

## See Also

[ATR](/functions/atr.md) · [DONCHIAN](/functions/donchian.md) · [NATR](/functions/natr.md) · [QSTICK](/functions/qstick.md) · [TRANGE](/functions/trange.md)

## References

- [TC2000 PCF help, *Average Daily Range (ADR)*](https://help.tc2000.com/m/69445/l/1993818-average-daily-range-adr) — `AVG(Hz-Lz, x)`, the mean-of-ranges form, with worked examples over 10 bars.
- [TradingView, *Average Daily Range (ADR) indicator*](https://www.tradingview.com/support/solutions/43000695003-average-daily-range-adr-indicator/) — the difference-of-averages form, over a user-supplied Length and Timeframe.
- [TradingView, *How are ADR% and ATR% calculated?*](https://www.tradingview.com/support/solutions/43000734653-how-are-adr-and-atr-calculated/) — the percentage form, and the statement that ADR "does not take gaps into account".
- kand `ohlcv/adr.rs` (Rust) states `Daily Range = High - Low; ADR = SMA(Daily Range, period)` and is the external implementation the regression goldens are captured from.
