---
title: MINUS_DI
description: "Wilder's Minus Directional Indicator: the Wilder-smoothed downward directional movement (-DM) normalized by smoothed True Range. Measures the strength of downward price movement. Higher -DI indicates a stronger downtrend; compared against +DI to gauge directional dominance."
---

# MINUS_DI

## Summary

Wilder's Minus Directional Indicator: the Wilder-smoothed downward directional movement (-DM) normalized by smoothed True Range. Measures the strength of downward price movement. Higher -DI indicates a stronger downtrend; compared against +DI to gauge directional dominance.

## Formula

-DM1 = (prevLow - low) if (prevLow-low)>0 and (high-prevHigh)<(prevLow-low), else 0. Seed -DM/TR = sum of first (period-1) -DM1/TR1, then Wilder-smooth each: X = X - X/period + today. -DI = 100 * (-DM / TR); TR from ta_true_range. If period<=1: -DI1 = -DM1/TR1 (no ×100).

## Notes

- Wilder's original integer rounding is not applied (it was removed as unreliable when values are near 1).

## Inputs

- `inHigh` — High price of each bar
- `inLow` — Low price of each bar
- `inClose` — Close price of each bar

## Outputs

- `outReal` — The Minus Directional Indicator (-DI) line

## Parameters

| Parameter | Type | Default | Accepted values | Description |
| --- | --- | --- | --- | --- |
| `optInTimePeriod` | integer | 14 | 1–100000 | Smoothing/lookback period for -DM and TR |

## Properties

| Numerical<br>Stability | Display<br>Flags |
| :-- | :-- |
| <span class="flag-box">☐</span> <span style="opacity:0.5">Start-Independent</span> <span class="flag-tip" tabindex="0" role="note" aria-label="The value at a bar does not depend on where your data starts — safe to compare across different-length windows." data-tip="The value at a bar does not depend on where your data starts — safe to compare across different-length windows.">i</span> | <span class="flag-box">☐</span> <span style="opacity:0.5">Overlap Input</span> <span class="flag-tip" tabindex="0" role="note" aria-label="Output is on the same scale as the input price, so it is drawn over the price chart." data-tip="Output is on the same scale as the input price, so it is drawn over the price chart.">i</span> |
| <span class="flag-box">✅</span> **Initial Unstable Period** <span class="flag-tip" tabindex="0" role="note" aria-label="Early values depend on how much history precedes them but converge as more bars are supplied; tunable via the unstable period." data-tip="Early values depend on how much history precedes them but converge as more bars are supplied; tunable via the unstable period.">i</span> | <span class="flag-box">✅</span> **Independent Y-Axis** <span class="flag-tip" tabindex="0" role="note" aria-label="Output is on its own scale, drawn in a separate pane below the price chart." data-tip="Output is on its own scale, drawn in a separate pane below the price chart.">i</span> |
| <span class="flag-box">☐</span> <span style="opacity:0.5">Path-Dependent</span> <span class="flag-tip" tabindex="0" role="note" aria-label="Built up from the first bar (a running accumulation or path-tracking state machine): the value depends on where your data begins and never converges — don't compare across different windows." data-tip="Built up from the first bar (a running accumulation or path-tracking state machine): the value depends on where your data begins and never converges — don't compare across different windows.">i</span> | <span class="flag-box">☐</span> <span style="opacity:0.5">Candlestick</span> <span class="flag-tip" tabindex="0" role="note" aria-label="Output is an integer candlestick-pattern signal (e.g. -100 / 0 / +100)." data-tip="Output is an integer candlestick-pattern signal (e.g. -100 / 0 / +100).">i</span> |

## Implementation

TA-Lib Definition: [`minus_di.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/minus_di/minus_di.c) · [`minus_di.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/minus_di/minus_di.yaml)

| Native | File |
|--------|------|
| C | [`ta_MINUS_DI.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_MINUS_DI.c) |
| Rust | [`minus_di.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/library/src/ta_func/minus_di.rs) |
| Java | [`Core_MINUS_DI.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/library/fragments/Core_MINUS_DI.java) |

TA-Lib is also available for Python, R and more using a [wrapper](https://ta-lib.org/wrappers/).

## Aliases

-DI, Negative Directional Indicator

## See Also

[PLUS_DI](/functions/plus_di) · [MINUS_DM](/functions/minus_dm) · [DX](/functions/dx) · [ADX](/functions/adx) · [ADXR](/functions/adxr) · [TRANGE](/functions/trange)

## References

- J. Welles Wilder, *New Concepts in Technical Trading Systems*, Trend Research (ISBN 0894590278)
