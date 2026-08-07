---
title: "Average Directional Movement Index (ADX)"
description: "Wilder's Average Directional Movement Index, a smoothed measure of trend strength derived from the directional indicators (+DI/-DI)."
---

## Summary

Wilder's Average Directional Movement Index, a smoothed measure of trend strength derived from the directional indicators (+DI/-DI). Quantifies how strongly a market is trending, regardless of direction. Higher values indicate a stronger trend (a common convention treats >25 as trending); says nothing about direction.

## Formula

+DI = 100*(+DM_p/TR_p), -DI = 100*(-DM_p/TR_p); DX = 100*|(-DI)-(+DI)| / ((-DI)+(+DI)); first ADX = mean of the first `period` DX; then ADX = (prevADX*(period-1) + DX)/period. +DM_p/-DM_p/TR_p use Wilder smoothing: X = X - X/period + today's one-bar value.

## Notes

- Wilder's original integer rounding is not applied.

## Inputs

- `inHigh` — High price of each bar
- `inLow` — Low price of each bar
- `inClose` — Close price of each bar

## Outputs

- `outReal` — Smoothed directional trend-strength index (0-100)

## Parameters

| Parameter | Type | Default | Accepted values | Description |
| --- | --- | --- | --- | --- |
| `optInTimePeriod` | integer | 14 | 2–100000 | Smoothing/averaging period for DM, TR, and ADX |

## Properties

**Numerical Stability:** [Initial Unstable Period](/functions/stability.md#initial-unstable-period)

| Display<br>Flags |
| :-- |
| <span class="flag-box">☐</span> <span style="opacity:0.5">Overlap Input</span> |
| <span class="flag-box">✅</span> **Independent Y-Axis** <span class="flag-tip" tabindex="0" role="note" aria-label="Output is on its own scale, drawn in a separate pane below the price chart." data-tip="Output is on its own scale, drawn in a separate pane below the price chart.">i</span> |
| <span class="flag-box">☐</span> <span style="opacity:0.5">Candlestick</span> |

## Implementation

TA-Lib Definition: [`adx.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/adx/adx.c) · [`adx.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/adx/adx.yaml)

| Native | File |
|--------|------|
| C | [`ta_ADX.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_ADX.c) |
| Rust | [`adx.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/library/src/ta_func/adx.rs) |
| Java | [`Core_ADX.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/fragments/Core_ADX.java) |

TA-Lib is also available for Python, R and more using a [wrapper](/install/#wrappers).

## Aliases

Average Directional Movement Index, Average Directional Index

## See Also

[ADXR](/functions/adxr.md) · [DX](/functions/dx.md) · [PLUS_DI](/functions/plus_di.md) · [MINUS_DI](/functions/minus_di.md) · [PLUS_DM](/functions/plus_dm.md) · [MINUS_DM](/functions/minus_dm.md) · [TRANGE](/functions/trange.md)

## References

- J. Welles Wilder, *New Concepts in Technical Trading Systems*, Trend Research (ISBN 0894590278)
