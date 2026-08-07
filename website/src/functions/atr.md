---
title: "Average True Range (ATR)"
description: "Wilder-smoothed average of the True Range over a period, measuring price volatility regardless of direction."
---

## Summary

Wilder-smoothed average of the True Range over a period, measuring price volatility regardless of direction. Higher ATR means greater volatility; no directional bias.

## Formula

TR_t = max(high-low, |prevClose-high|, |prevClose-low|)
ATR seed = simple average of first `period` TR values
ATR_t = (ATR_{t-1} * (period-1) + TR_t) / period

## Inputs

- `inHigh` — High price of each bar
- `inLow` — Low price of each bar
- `inClose` — Close price of each bar

## Outputs

- `outReal` — Average True Range value

## Parameters

| Parameter | Type | Default | Accepted values | Description |
| --- | --- | --- | --- | --- |
| `optInTimePeriod` | integer | 14 | 1–100000 | Smoothing period |

## Properties

**Numerical Stability:** [Initial Unstable Period](/functions/stability.md#initial-unstable-period)

| Display<br>Flags |
| :-- |
| <span class="flag-box">☐</span> <span style="opacity:0.5">Overlap Input</span> |
| <span class="flag-box">✅</span> **Independent Y-Axis** <span class="flag-tip" tabindex="0" role="note" aria-label="Output is on its own scale, drawn in a separate pane below the price chart." data-tip="Output is on its own scale, drawn in a separate pane below the price chart.">i</span> |
| <span class="flag-box">☐</span> <span style="opacity:0.5">Candlestick</span> |

## Implementation

TA-Lib Definition: [`atr.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/atr/atr.c) · [`atr.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/atr/atr.yaml)

| Native | File |
|--------|------|
| C | [`ta_ATR.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_ATR.c) |
| Rust | [`atr.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/library/src/ta_func/atr.rs) |
| Java | [`Core_ATR.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/fragments/Core_ATR.java) |

TA-Lib is also available for Python, R and more using a [wrapper](/install/#wrappers).

## Aliases

Average True Range

## See Also

[TRANGE](/functions/trange.md) · [NATR](/functions/natr.md) · [SMA](/functions/sma.md) · [EMA](/functions/ema.md)

## References

- J. Welles Wilder, *New Concepts in Technical Trading Systems*, Trend Research (ISBN 0894590278)
