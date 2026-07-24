---
title: VWMA
description: "Volume Weighted Moving Average: the mean price over a trailing window of `optInTimePeriod` bars, each bar weighted by its own volume. Heavily traded bars pull the average toward their price; quiet bars barely move it. Read like any moving average — price above is strength, below is weakness. Against a plain [`SMA`](/functions/sma) of the same window it leads on high-volume moves and lags on low-volume drift, so the gap between the two lines measures how volume-confirmed a move is. It has no attributable inventor — charting-package folklore — and every published definition agrees, so there is no competing variant."
---

# VWMA

## Summary

Volume Weighted Moving Average: the mean price over a trailing window of `optInTimePeriod` bars, each bar weighted by its own volume. Heavily traded bars pull the average toward their price; quiet bars barely move it.

Read like any moving average — price above is strength, below is weakness. Against a plain [`SMA`](/functions/sma) of the same window it leads on high-volume moves and lags on low-volume drift, so the gap between the two lines measures how volume-confirmed a move is.

It has no attributable inventor — charting-package folklore — and every published definition agrees, so there is no competing variant.

## Formula

VWMA = ( sum_{k=t-N+1..t} P[k] * V[k] ) / ( sum_{k=t-N+1..t} V[k] ), N = optInTimePeriod

Equivalently, and bit-identically so in TA-Lib, SMA(P * V, N) / SMA(V, N) — the composition TradingView documents for `ta.vwma`. There is no seeding and no recursion, hence no unstable period.

## Notes

- Volume is expected to be non-negative. Individual zero-volume bars are fine: a bar that did not trade simply carries no weight, and the average stays well defined as long as some bar in the window has volume. Only a window in which *every* volume is zero has no weights at all; the weighted mean is then undefined and that element is NaN, as it is in every other implementation. Series carrying no volume on any bar, such as cash-index feeds, are outside what a volume-weighted average can describe — use SMA or WMA there.
- A period of 1 reduces to `(P * V) / V`. That is the price arithmetically, but not a guaranteed IEEE round trip, so unlike SMA of period 1 it must not be relied upon as an exact copy of the input.

## Inputs

- `inReal` — Source price series, close by convention
- `inVolume` — Volume of each bar

## Outputs

- `outReal` — Volume weighted moving average of the input

## Parameters

| Parameter | Type | Default | Accepted values | Description |
| --- | --- | --- | --- | --- |
| `optInTimePeriod` | integer | 30 | 1–100000 | Number of bars in the weighting window |

## Properties

| Numerical<br>Stability | Display<br>Flags |
| :-- | :-- |
| <span class="flag-box">✅</span> **Start-Independent** <span class="flag-tip" tabindex="0" role="note" aria-label="The value at a bar does not depend on where your data starts — safe to compare across different-length windows." data-tip="The value at a bar does not depend on where your data starts — safe to compare across different-length windows.">i</span> | <span class="flag-box">✅</span> **Overlap Input** <span class="flag-tip" tabindex="0" role="note" aria-label="Output is on the same scale as the input price, so it is drawn over the price chart." data-tip="Output is on the same scale as the input price, so it is drawn over the price chart.">i</span> |
| <span class="flag-box">☐</span> <span style="opacity:0.5">Initial Unstable Period</span> <span class="flag-tip" tabindex="0" role="note" aria-label="Early values depend on how much history precedes them but converge as more bars are supplied; tunable via the unstable period." data-tip="Early values depend on how much history precedes them but converge as more bars are supplied; tunable via the unstable period.">i</span> | <span class="flag-box">☐</span> <span style="opacity:0.5">Independent Y-Axis</span> <span class="flag-tip" tabindex="0" role="note" aria-label="Output is on its own scale, drawn in a separate pane below the price chart." data-tip="Output is on its own scale, drawn in a separate pane below the price chart.">i</span> |
| <span class="flag-box">☐</span> <span style="opacity:0.5">Path-Dependent</span> <span class="flag-tip" tabindex="0" role="note" aria-label="Built up from the first bar (a running accumulation or path-tracking state machine): the value depends on where your data begins and never converges — don't compare across different windows." data-tip="Built up from the first bar (a running accumulation or path-tracking state machine): the value depends on where your data begins and never converges — don't compare across different windows.">i</span> | <span class="flag-box">☐</span> <span style="opacity:0.5">Candlestick</span> <span class="flag-tip" tabindex="0" role="note" aria-label="Output is an integer candlestick-pattern signal (e.g. -100 / 0 / +100)." data-tip="Output is an integer candlestick-pattern signal (e.g. -100 / 0 / +100).">i</span> |

## Implementation

TA-Lib Definition: [`vwma.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/vwma/vwma.c) · [`vwma.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/vwma/vwma.yaml)

| Native | File |
|--------|------|
| C | [`ta_VWMA.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_VWMA.c) |
| Rust | [`vwma.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/library/src/ta_func/vwma.rs) |
| Java | [`Core_VWMA.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/library/fragments/Core_VWMA.java) |

TA-Lib is also available for Python, R and more using a [wrapper](https://ta-lib.org/wrappers/).

## Aliases

Volume Weighted Moving Average

## See Also

[SMA](/functions/sma) · [WMA](/functions/wma) · [MA](/functions/ma) · [OBV](/functions/obv)

## References

- VWMA has no separately documented originator; its definition is uniform across charting packages.
- MotiveWave, *Volume Weighted Moving Average* study documentation — the closest thing to a primary definition.
- TradingView, *Volume Weighted Moving Average (VWMA)* — documents the equivalence with SMA(price * volume) / SMA(volume).
