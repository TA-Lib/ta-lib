---
title: STOCHRSI
description: "Applies the Fast Stochastic (STOCHF) oscillator to an RSI series instead of price, measuring where RSI sits within its recent min/max range. Oscillates 0-100; high = RSI near its recent top, low = near its recent bottom."
---

# STOCHRSI

## Summary

Applies the Fast Stochastic (STOCHF) oscillator to an RSI series instead of price, measuring where RSI sits within its recent min/max range. Oscillates 0-100; high = RSI near its recent top, low = near its recent bottom.

## Formula

rsi = RSI(inReal, optInTimePeriod)
FastK = 100 * (rsi_t - min(rsi, FastK_Period)) / (max(rsi, FastK_Period) - min(rsi, FastK_Period))
FastD = MA(FastK, FastD_Period, FastD_MAType)

## Notes

- To reproduce the original article's unsmoothed Stochastic RSI, set the RSI period equal to the %K period and read the raw %K output.
- When the RSI's recent range is zero, %K is set to 0 instead of being undefined.

## Inputs

- `inReal` — Source series fed into the RSI calculation

## Outputs

- `outFastK` — Unsmoothed stochastic of the RSI (raw %K)
- `outFastD` — %K smoothed over FastD_Period (signal line)

## Parameters

| Parameter | Type | Default | Accepted values | Description |
| --- | --- | --- | --- | --- |
| `optInTimePeriod` | integer | 14 | 2–100000 | RSI period |
| `optInFastK_Period` | integer | 5 | 1–100000 | Lookback window for the RSI min/max stochastic |
| `optInFastD_Period` | integer | 3 | 1–100000 | Smoothing period for %D |
| `optInFastD_MAType` | MAType | SMA (0) | any MAType | MA type used to smooth %D |

*`MAType` values: 0 SMA · 1 EMA · 2 WMA · 3 DEMA · 4 TEMA · 5 TRIMA · 6 KAMA · 7 MAMA · 8 T3 · 9 HMA*

## Properties

| Numerical<br>Stability | Display<br>Flags |
| :-- | :-- |
| <span class="flag-box">✅</span> **Start-Independent** <span class="flag-tip" tabindex="0" role="note" aria-label="The value at a bar does not depend on where your data starts — safe to compare across different-length windows." data-tip="The value at a bar does not depend on where your data starts — safe to compare across different-length windows.">i</span> | <span class="flag-box">☐</span> <span style="opacity:0.5">Overlap Input</span> <span class="flag-tip" tabindex="0" role="note" aria-label="Output is on the same scale as the input price, so it is drawn over the price chart." data-tip="Output is on the same scale as the input price, so it is drawn over the price chart.">i</span> |
| <span class="flag-box">☐</span> <span style="opacity:0.5">Initial Unstable Period</span> <span class="flag-tip" tabindex="0" role="note" aria-label="Early values depend on how much history precedes them but converge as more bars are supplied; tunable via the unstable period." data-tip="Early values depend on how much history precedes them but converge as more bars are supplied; tunable via the unstable period.">i</span> | <span class="flag-box">✅</span> **Independent Y-Axis** <span class="flag-tip" tabindex="0" role="note" aria-label="Output is on its own scale, drawn in a separate pane below the price chart." data-tip="Output is on its own scale, drawn in a separate pane below the price chart.">i</span> |
| <span class="flag-box">☐</span> <span style="opacity:0.5">Path-Dependent</span> <span class="flag-tip" tabindex="0" role="note" aria-label="Built up from the first bar (a running accumulation or path-tracking state machine): the value depends on where your data begins and never converges — don't compare across different windows." data-tip="Built up from the first bar (a running accumulation or path-tracking state machine): the value depends on where your data begins and never converges — don't compare across different windows.">i</span> | <span class="flag-box">☐</span> <span style="opacity:0.5">Candlestick</span> <span class="flag-tip" tabindex="0" role="note" aria-label="Output is an integer candlestick-pattern signal (e.g. -100 / 0 / +100)." data-tip="Output is an integer candlestick-pattern signal (e.g. -100 / 0 / +100).">i</span> |

## Implementation

TA-Lib Definition: [`stochrsi.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/stochrsi/stochrsi.c) · [`stochrsi.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/stochrsi/stochrsi.yaml)

| Native | File |
|--------|------|
| C | [`ta_STOCHRSI.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_STOCHRSI.c) |
| Rust | [`stochrsi.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/library/src/ta_func/stochrsi.rs) |
| Java | [`Core_STOCHRSI.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/library/fragments/Core_STOCHRSI.java) |

TA-Lib is also available for Python, R and more using a [wrapper](https://ta-lib.org/wrappers/).

## Aliases

Stochastic RSI

## See Also

[RSI](/functions/rsi) · [STOCHF](/functions/stochf) · [STOCH](/functions/stoch) · [MA](/functions/ma)

## References

- Tushar S. Chande, Stanley Kroll, *The New Technical Trader*, John Wiley & Sons (ISBN 0471597805)
