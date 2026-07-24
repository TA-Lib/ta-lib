---
title: STOCHF
description: "Fast Stochastic Oscillator: the raw %K line and its moving-average-smoothed %D line. Unlike STOCH (which slows both lines), STOCHF returns the unsmoothed FastK and FastD. Oscillates 0-100; >80 overbought, <20 oversold."
---

# STOCHF

## Summary

Fast Stochastic Oscillator: the raw %K line and its moving-average-smoothed %D line. Unlike STOCH (which slows both lines), STOCHF returns the unsmoothed FastK and FastD. Oscillates 0-100; >80 overbought, <20 oversold.

## Formula

FastK = 100 * (Close - LowestLow) / (HighestHigh - LowestLow), over the last FastK_Period bars (incl. today)
FastD = MA(FastK, FastD_Period, FastD_MAType)

## Notes

- When the high-low range over the window is zero, %K is set to 0 instead of being undefined.

## Inputs

- `inHigh` — High price of each bar
- `inLow` — Low price of each bar
- `inClose` — Close price of each bar

## Outputs

- `outFastK` — Raw %K stochastic line
- `outFastD` — MA-smoothed %K (signal line)

## Parameters

| Parameter | Type | Default | Accepted values | Description |
| --- | --- | --- | --- | --- |
| `optInFastK_Period` | integer | 5 | 1–100000 | Lookback window for the highest-high/lowest-low of Fast-K |
| `optInFastD_Period` | integer | 3 | 1–100000 | Smoothing period for the Fast-D line |
| `optInFastD_MAType` | MAType | SMA (0) | any MAType | Moving-average type used to smooth Fast-D |

*`MAType` values: 0 SMA · 1 EMA · 2 WMA · 3 DEMA · 4 TEMA · 5 TRIMA · 6 KAMA · 7 MAMA · 8 T3 · 9 HMA · 10 DISABLED*

## Properties

| Numerical<br>Stability | Display<br>Flags |
| :-- | :-- |
| <span class="flag-box">✅</span> **Start-Independent** <span class="flag-tip" tabindex="0" role="note" aria-label="The value at a bar does not depend on where your data starts — safe to compare across different-length windows." data-tip="The value at a bar does not depend on where your data starts — safe to compare across different-length windows.">i</span> | <span class="flag-box">☐</span> <span style="opacity:0.5">Overlap Input</span> <span class="flag-tip" tabindex="0" role="note" aria-label="Output is on the same scale as the input price, so it is drawn over the price chart." data-tip="Output is on the same scale as the input price, so it is drawn over the price chart.">i</span> |
| <span class="flag-box">☐</span> <span style="opacity:0.5">Initial Unstable Period</span> <span class="flag-tip" tabindex="0" role="note" aria-label="Early values depend on how much history precedes them but converge as more bars are supplied; tunable via the unstable period." data-tip="Early values depend on how much history precedes them but converge as more bars are supplied; tunable via the unstable period.">i</span> | <span class="flag-box">✅</span> **Independent Y-Axis** <span class="flag-tip" tabindex="0" role="note" aria-label="Output is on its own scale, drawn in a separate pane below the price chart." data-tip="Output is on its own scale, drawn in a separate pane below the price chart.">i</span> |
| <span class="flag-box">☐</span> <span style="opacity:0.5">Path-Dependent</span> <span class="flag-tip" tabindex="0" role="note" aria-label="Built up from the first bar (a running accumulation or path-tracking state machine): the value depends on where your data begins and never converges — don't compare across different windows." data-tip="Built up from the first bar (a running accumulation or path-tracking state machine): the value depends on where your data begins and never converges — don't compare across different windows.">i</span> | <span class="flag-box">☐</span> <span style="opacity:0.5">Candlestick</span> <span class="flag-tip" tabindex="0" role="note" aria-label="Output is an integer candlestick-pattern signal (e.g. -100 / 0 / +100)." data-tip="Output is an integer candlestick-pattern signal (e.g. -100 / 0 / +100).">i</span> |

## Implementation

TA-Lib Definition: [`stochf.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/stochf/stochf.c) · [`stochf.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/stochf/stochf.yaml)

| Native | File |
|--------|------|
| C | [`ta_STOCHF.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_STOCHF.c) |
| Rust | [`stochf.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/library/src/ta_func/stochf.rs) |
| Java | [`Core_STOCHF.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/library/fragments/Core_STOCHF.java) |

TA-Lib is also available for Python, R and more using a [wrapper](https://ta-lib.org/wrappers/).

## Aliases

Stochastic Fast, Fast Stochastic Oscillator

## See Also

[STOCH](/functions/stoch) · [STOCHRSI](/functions/stochrsi) · [MA](/functions/ma)
