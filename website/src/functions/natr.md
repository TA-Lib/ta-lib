---
title: NATR
description: "Average True Range expressed as a percentage of the current close, making volatility comparable across price levels and securities. Same computation as ATR, then normalized by close. Higher values mean greater relative volatility; unit is percent of price."
---

# NATR

## Summary

Average True Range expressed as a percentage of the current close, making volatility comparable across price levels and securities. Same computation as ATR, then normalized by close. Higher values mean greater relative volatility; unit is percent of price.

## Formula

NATR = (ATR / Close) * 100
ATR: first value = SMA of TRANGE over period; then Wilder smoothing ATR_t = (ATR_{t-1}*(period-1) + TR_t) / period

## Inputs

- `inHigh` — High price of each bar
- `inLow` — Low price of each bar
- `inClose` — Close price of each bar

## Outputs

- `outReal` — ATR as a percentage of the close

## Parameters

| Parameter | Type | Default | Accepted values | Description |
| --- | --- | --- | --- | --- |
| `optInTimePeriod` | integer | 14 | 1–100000 | Smoothing period for the true range average |

## Properties

**Numerical Stability:** [Initial Unstable Period](/functions/stability#initial-unstable-period)

| Display<br>Flags |
| :-- |
| <span class="flag-box">☐</span> <span style="opacity:0.5">Overlap Input</span> |
| <span class="flag-box">✅</span> **Independent Y-Axis** <span class="flag-tip" tabindex="0" role="note" aria-label="Output is on its own scale, drawn in a separate pane below the price chart." data-tip="Output is on its own scale, drawn in a separate pane below the price chart.">i</span> |
| <span class="flag-box">☐</span> <span style="opacity:0.5">Candlestick</span> |

## Implementation

TA-Lib Definition: [`natr.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/natr/natr.c) · [`natr.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/natr/natr.yaml)

| Native | File |
|--------|------|
| C | [`ta_NATR.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_NATR.c) |
| Rust | [`natr.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/library/src/ta_func/natr.rs) |
| Java | [`Core_NATR.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/library/fragments/Core_NATR.java) |

TA-Lib is also available for Python, R and more using a [wrapper](https://ta-lib.org/wrappers/).

## Aliases

Normalized Average True Range

## See Also

[ATR](/functions/atr) · [TRANGE](/functions/trange) · [SMA](/functions/sma)

## References

- John Forman
