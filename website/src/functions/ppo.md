---
title: PPO
description: "Percentage Price Oscillator: the difference between a fast and slow moving average expressed as a percentage of the slow MA. A normalized (scale-invariant) variant of APO. Positive when the fast MA is above the slow MA (upward momentum), negative otherwise; magnitude is the % deviation."
---

# PPO

## Summary

Percentage Price Oscillator: the difference between a fast and slow moving average expressed as a percentage of the slow MA. A normalized (scale-invariant) variant of APO. Positive when the fast MA is above the slow MA (upward momentum), negative otherwise; magnitude is the % deviation.

## Formula

PPO = ((fastMA(inReal) - slowMA(inReal)) / slowMA(inReal)) * 100, both MAs of type optInMAType; output = 0 when slowMA == 0

The standard form is exponential with periods 12 and 26 — ((12-day EMA - 26-day EMA) / 26-day EMA) * 100, i.e. the MACD oscillator expressed as a percentage. `optInMAType` therefore **defaults to EMA** — the moving average Gerald Appel used for the original PPO/MACD; pass another type (e.g. `TA_MAType_SMA`) to override.

## Inputs

- `inReal` — Input data series

## Outputs

- `outReal` — PPO value in percent

## Parameters

| Parameter | Type | Default | Accepted values | Description |
| --- | --- | --- | --- | --- |
| `optInFastPeriod` | integer | 12 | 2–100000 | Period of the fast MA |
| `optInSlowPeriod` | integer | 26 | 2–100000 | Period of the slow MA |
| `optInMAType` | MAType | EMA (1) | any MAType | Moving average type used for both MAs |

*`MAType` values: 0 SMA · 1 EMA · 2 WMA · 3 DEMA · 4 TEMA · 5 TRIMA · 6 KAMA · 7 MAMA · 8 T3 · 9 HMA*

## Properties

| Numerical<br>Stability | Display<br>Flags |
| :-- | :-- |
| <span class="flag-box">✅</span> **Start-Independent** <span class="flag-tip" tabindex="0" role="note" aria-label="The value at a bar does not depend on where your data starts — safe to compare across different-length windows." data-tip="The value at a bar does not depend on where your data starts — safe to compare across different-length windows.">i</span> | <span class="flag-box">☐</span> <span style="opacity:0.5">Overlap Input</span> <span class="flag-tip" tabindex="0" role="note" aria-label="Output is on the same scale as the input price, so it is drawn over the price chart." data-tip="Output is on the same scale as the input price, so it is drawn over the price chart.">i</span> |
| <span class="flag-box">☐</span> <span style="opacity:0.5">Initial Unstable Period</span> <span class="flag-tip" tabindex="0" role="note" aria-label="Early values depend on how much history precedes them but converge as more bars are supplied; tunable via the unstable period." data-tip="Early values depend on how much history precedes them but converge as more bars are supplied; tunable via the unstable period.">i</span> | <span class="flag-box">✅</span> **Independent Y-Axis** <span class="flag-tip" tabindex="0" role="note" aria-label="Output is on its own scale, drawn in a separate pane below the price chart." data-tip="Output is on its own scale, drawn in a separate pane below the price chart.">i</span> |
| <span class="flag-box">☐</span> <span style="opacity:0.5">Path-Dependent</span> <span class="flag-tip" tabindex="0" role="note" aria-label="Built up from the first bar (a running accumulation or path-tracking state machine): the value depends on where your data begins and never converges — don't compare across different windows." data-tip="Built up from the first bar (a running accumulation or path-tracking state machine): the value depends on where your data begins and never converges — don't compare across different windows.">i</span> | <span class="flag-box">☐</span> <span style="opacity:0.5">Candlestick</span> <span class="flag-tip" tabindex="0" role="note" aria-label="Output is an integer candlestick-pattern signal (e.g. -100 / 0 / +100)." data-tip="Output is an integer candlestick-pattern signal (e.g. -100 / 0 / +100).">i</span> |

## Implementation

TA-Lib Definition: [`ppo.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/ppo/ppo.c) · [`ppo.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/ppo/ppo.yaml)

| Native | File |
|--------|------|
| C | [`ta_PPO.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_PPO.c) |
| Rust | [`ppo.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/library/src/ta_func/ppo.rs) |
| Java | [`Core_PPO.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/library/fragments/Core_PPO.java) |

TA-Lib is also available for Python, R and more using a [wrapper](https://ta-lib.org/wrappers/).

## Aliases

Percentage Price Oscillator

## See Also

[APO](/functions/apo) · [MACD](/functions/macd) · [MA](/functions/ma)

## References

- Gerald Appel, creator of the PPO and MACD (MACD introduced 1979 in his *Systems and Forecasts* newsletter). The PPO is the MACD expressed as a percentage of the slow moving average. Appel's original definition uses **exponential** moving averages (periods 12, 26).
