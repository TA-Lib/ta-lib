---
title: ULTOSC
description: "Ultimate Oscillator: momentum indicator combining buying-pressure/true-range ratios over three time periods into one 0-100 weighted average. Blends short-, medium-, and long-term momentum to damp single-period noise. Ranges 0-100; conventionally >70 overbought, <30 oversold."
---

# ULTOSC

## Summary

Ultimate Oscillator: momentum indicator combining buying-pressure/true-range ratios over three time periods into one 0-100 weighted average. Blends short-, medium-, and long-term momentum to damp single-period noise. Ranges 0-100; conventionally >70 overbought, <30 oversold.

## Formula

trueLow = min(low, prevClose);  BP = close - trueLow
TR = max(high-low, |prevClose-high|, |prevClose-low|)
avg_n = (sum BP over n bars) / (sum TR over n bars)
ULTOSC = 100 * (4*avg_short + 2*avg_mid + avg_long) / 7

## Notes

- The three periods are sorted internally, so the 4/2/1 weighting always applies to the shortest, middle, and longest period regardless of the order in which you pass them.

## Inputs

- `inHigh` — High price of each bar
- `inLow` — Low price of each bar
- `inClose` — Close price of each bar

## Outputs

- `outReal` — Ultimate Oscillator value

## Parameters

| Parameter | Type | Default | Accepted values | Description |
| --- | --- | --- | --- | --- |
| `optInTimePeriod1` | integer | 7 | 1–100000 | Bars for one averaging window |
| `optInTimePeriod2` | integer | 14 | 1–100000 | Bars for another averaging window |
| `optInTimePeriod3` | integer | 28 | 1–100000 | Bars for another averaging window |

## Properties

| Numerical<br>Stability | Display<br>Flags |
| :-- | :-- |
| <span class="flag-box">✅</span> **Start-Independent** <span class="flag-tip" tabindex="0" role="note" aria-label="The value at a bar does not depend on where your data starts — safe to compare across different-length windows." data-tip="The value at a bar does not depend on where your data starts — safe to compare across different-length windows.">i</span> | <span class="flag-box">☐</span> <span style="opacity:0.5">Overlap Input</span> <span class="flag-tip" tabindex="0" role="note" aria-label="Output is on the same scale as the input price, so it is drawn over the price chart." data-tip="Output is on the same scale as the input price, so it is drawn over the price chart.">i</span> |
| <span class="flag-box">☐</span> <span style="opacity:0.5">Initial Unstable Period</span> <span class="flag-tip" tabindex="0" role="note" aria-label="Early values depend on how much history precedes them but converge as more bars are supplied; tunable via the unstable period." data-tip="Early values depend on how much history precedes them but converge as more bars are supplied; tunable via the unstable period.">i</span> | <span class="flag-box">✅</span> **Independent Y-Axis** <span class="flag-tip" tabindex="0" role="note" aria-label="Output is on its own scale, drawn in a separate pane below the price chart." data-tip="Output is on its own scale, drawn in a separate pane below the price chart.">i</span> |
| <span class="flag-box">☐</span> <span style="opacity:0.5">Path-Dependent</span> <span class="flag-tip" tabindex="0" role="note" aria-label="Built up from the first bar (a running accumulation or path-tracking state machine): the value depends on where your data begins and never converges — don't compare across different windows." data-tip="Built up from the first bar (a running accumulation or path-tracking state machine): the value depends on where your data begins and never converges — don't compare across different windows.">i</span> | <span class="flag-box">☐</span> <span style="opacity:0.5">Candlestick</span> <span class="flag-tip" tabindex="0" role="note" aria-label="Output is an integer candlestick-pattern signal (e.g. -100 / 0 / +100)." data-tip="Output is an integer candlestick-pattern signal (e.g. -100 / 0 / +100).">i</span> |

## Implementation

TA-Lib Definition: [`ultosc.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/ultosc/ultosc.c) · [`ultosc.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/ultosc/ultosc.yaml)

| Native | File |
|--------|------|
| C | [`ta_ULTOSC.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_ULTOSC.c) |
| Rust | [`ultosc.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/library/src/ta_func/ultosc.rs) |
| Java | [`Core_ULTOSC.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/library/fragments/Core_ULTOSC.java) |

TA-Lib is also available for Python, R and more using a [wrapper](https://ta-lib.org/wrappers/).

## Aliases

Ultimate Oscillator, UO

## See Also

[ATR](/functions/atr) · [TRANGE](/functions/trange) · [RSI](/functions/rsi)

## References

- Larry Williams, *The Ultimate Oscillator*, Technical Analysis of Stocks & Commodities, V.3:4 (1985)
