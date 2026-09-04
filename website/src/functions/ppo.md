---
title: "Percentage Price Oscillator (PPO)"
description: "Percentage Price Oscillator: the difference between a fast and slow moving average expressed as a percentage of the slow MA."
---

## Summary

Percentage Price Oscillator: the difference between a fast and slow moving average expressed as a percentage of the slow MA. A normalized (scale-invariant) variant of APO. Positive when the fast MA is above the slow MA (upward momentum), negative otherwise; magnitude is the % deviation.

## Formula

PPO = ((fastMA(inReal) - slowMA(inReal)) / slowMA(inReal)) * 100, both MAs of type optInMAType; output = 0 when slowMA == 0

The standard form is exponential with periods 12 and 26 — ((12-day EMA - 26-day EMA) / 26-day EMA) * 100, i.e. the MACD oscillator expressed as a percentage. `optInMAType` therefore **defaults to EMA** — the moving average Gerald Appel used for the original PPO/MACD; pass another type (e.g. `TA_MAType_SMA`) to override.

## Notes

- `optInMAType` applies to both the fast and slow moving average. `TA_MAType_MAMA` ignores its period argument, so with `optInMAType = TA_MAType_MAMA` the fast and slow MAs are identical, making the numerator — and therefore the output — zero at every bar.

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

*`MAType` values: 0 SMA · 1 EMA · 2 WMA · 3 DEMA · 4 TEMA · 5 TRIMA · 6 KAMA · 7 MAMA · 8 T3 · 9 HMA · 10 DISABLED · 11 DEFAULT · 12 ZLEMA*

## Properties

**Numerical Stability:** [Depends on MA Type](/functions/stability.md#depends-on-ma-type) — This function's default, EMA, has an initial unstable period.

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

TA-Lib Definition: [`ppo.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/ppo/ppo.c) · [`ppo.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/ppo/ppo.yaml)

| Native | File |
|--------|------|
| C | [`ta_PPO.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_PPO.c) |
| Rust | [`ppo.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/library/src/ta_func/ppo.rs) |
| Java | [`Core_PPO.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/fragments/Core_PPO.java) |

TA-Lib is also available for Python, R and more using a [wrapper](/install/#wrappers).

## Aliases

Percentage Price Oscillator

## See Also

[APO](/functions/apo.md) · [MACD](/functions/macd.md) · [MA](/functions/ma.md)

## References

- Gerald Appel, creator of the PPO and MACD (MACD introduced 1979 in his *Systems and Forecasts* newsletter). The PPO is the MACD expressed as a percentage of the slow moving average. Appel's original definition uses **exponential** moving averages (periods 12, 26).
