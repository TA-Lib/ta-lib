---
title: APO
description: "Absolute Price Oscillator: the difference between a fast and a slow moving average of the input, in price units. Measures short- vs long-term momentum. Positive when fast MA > slow MA (upward momentum); negative otherwise."
---

# APO

## Summary

Absolute Price Oscillator: the difference between a fast and a slow moving average of the input, in price units. Measures short- vs long-term momentum. Positive when fast MA > slow MA (upward momentum); negative otherwise.

## Formula

$APO = MA_{fast}(inReal) - MA_{slow}(inReal)$, both MAs of type optInMAType

The standard form is exponential — APO with EMA and periods 12/26 is the fast-minus-slow EMA construction underlying the MACD (in price units). `optInMAType` therefore **defaults to EMA** — the moving average Gerald Appel used for the original MACD; pass another type (e.g. `TA_MAType_SMA`) to override.

## Inputs

- `inReal` — Source data series

## Outputs

- `outReal` — Fast MA minus slow MA

## Parameters

| Parameter | Type | Default | Accepted values | Description |
| --- | --- | --- | --- | --- |
| `optInFastPeriod` | integer | 12 | 2–100000 | Period of the fast moving average |
| `optInSlowPeriod` | integer | 26 | 2–100000 | Period of the slow moving average |
| `optInMAType` | MAType | EMA (1) | any MAType | Moving-average type used for both MAs |

*`MAType` values: 0 SMA · 1 EMA · 2 WMA · 3 DEMA · 4 TEMA · 5 TRIMA · 6 KAMA · 7 MAMA · 8 T3 · 9 HMA*

## Properties

| Numerical<br>Stability | Display<br>Flags |
| :-- | :-- |
| <span class="flag-box">✅</span> **Start-Independent** <span class="flag-tip" tabindex="0" role="note" aria-label="The value at a bar does not depend on where your data starts — safe to compare across different-length windows." data-tip="The value at a bar does not depend on where your data starts — safe to compare across different-length windows.">i</span> | <span class="flag-box">☐</span> <span style="opacity:0.5">Overlap Input</span> <span class="flag-tip" tabindex="0" role="note" aria-label="Output is on the same scale as the input price, so it is drawn over the price chart." data-tip="Output is on the same scale as the input price, so it is drawn over the price chart.">i</span> |
| <span class="flag-box">☐</span> <span style="opacity:0.5">Initial Unstable Period</span> <span class="flag-tip" tabindex="0" role="note" aria-label="Early values depend on how much history precedes them but converge as more bars are supplied; tunable via the unstable period." data-tip="Early values depend on how much history precedes them but converge as more bars are supplied; tunable via the unstable period.">i</span> | <span class="flag-box">✅</span> **Independent Y-Axis** <span class="flag-tip" tabindex="0" role="note" aria-label="Output is on its own scale, drawn in a separate pane below the price chart." data-tip="Output is on its own scale, drawn in a separate pane below the price chart.">i</span> |
| <span class="flag-box">☐</span> <span style="opacity:0.5">Path-Dependent</span> <span class="flag-tip" tabindex="0" role="note" aria-label="Built up from the first bar (a running accumulation or path-tracking state machine): the value depends on where your data begins and never converges — don't compare across different windows." data-tip="Built up from the first bar (a running accumulation or path-tracking state machine): the value depends on where your data begins and never converges — don't compare across different windows.">i</span> | <span class="flag-box">☐</span> <span style="opacity:0.5">Candlestick</span> <span class="flag-tip" tabindex="0" role="note" aria-label="Output is an integer candlestick-pattern signal (e.g. -100 / 0 / +100)." data-tip="Output is an integer candlestick-pattern signal (e.g. -100 / 0 / +100).">i</span> |

## Implementation

TA-Lib Definition: [`apo.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/apo/apo.c) · [`apo.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/apo/apo.yaml)

| Native | File |
|--------|------|
| C | [`ta_APO.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_APO.c) |
| Rust | [`apo.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/library/src/ta_func/apo.rs) |
| Java | [`Core_APO.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/library/fragments/Core_APO.java) |

TA-Lib is also available for Python, R and more using a [wrapper](https://ta-lib.org/wrappers/).

## Aliases

Absolute Price Oscillator

## See Also

[PPO](/functions/ppo) · [MACD](/functions/macd) · [MA](/functions/ma) · [EMA](/functions/ema) · [SMA](/functions/sma)

## References

- Gerald Appel, creator of the MACD (introduced 1979 in his *Systems and Forecasts* newsletter). The APO is the same fast-minus-slow moving-average oscillator in price units; with exponential moving averages and periods 12/26 it is the oscillator underlying the MACD line. Appel's original definition uses **exponential** moving averages.
