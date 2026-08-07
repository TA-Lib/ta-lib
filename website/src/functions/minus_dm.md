---
title: "Minus Directional Movement (MINUS_DM)"
description: "Minus Directional Movement, the downward component of Wilder's directional movement system. Measures Wilder-smoothed downward price motion over the period."
---

## Summary

Minus Directional Movement, the downward component of Wilder's directional movement system. Measures Wilder-smoothed downward price motion over the period. Higher -DM indicates stronger downward directional movement.

## Formula

diffP = high - prevHigh; diffM = prevLow - low
-DM1 = diffM if (diffM > 0 and diffP < diffM) else 0
period<=1: output raw -DM1 per bar.
period>1: seed = sum of first (period-1) -DM1; then Wilder smooth each bar:
-DM = prevMinusDM - prevMinusDM/period (+ -DM1 when the bar qualifies)

## Inputs

- `inHigh` — High price of each bar
- `inLow` — Low price of each bar

## Outputs

- `outReal` — Smoothed minus directional movement

## Parameters

| Parameter | Type | Default | Accepted values | Description |
| --- | --- | --- | --- | --- |
| `optInTimePeriod` | integer | 14 | 1–100000 | Wilder smoothing period |

## Properties

**Numerical Stability:** [Initial Unstable Period](/functions/stability.md#initial-unstable-period)

| Display<br>Flags |
| :-- |
| <span class="flag-box">☐</span> <span style="opacity:0.5">Overlap Input</span> |
| <span class="flag-box">✅</span> **Independent Y-Axis** <span class="flag-tip" tabindex="0" role="note" aria-label="Output is on its own scale, drawn in a separate pane below the price chart." data-tip="Output is on its own scale, drawn in a separate pane below the price chart.">i</span> |
| <span class="flag-box">☐</span> <span style="opacity:0.5">Candlestick</span> |

## Implementation

TA-Lib Definition: [`minus_dm.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/minus_dm/minus_dm.c) · [`minus_dm.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/minus_dm/minus_dm.yaml)

| Native | File |
|--------|------|
| C | [`ta_MINUS_DM.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_MINUS_DM.c) |
| Rust | [`minus_dm.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/library/src/ta_func/minus_dm.rs) |
| Java | [`Core_MINUS_DM.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/fragments/Core_MINUS_DM.java) |

TA-Lib is also available for Python, R and more using a [wrapper](/install/#wrappers).

## Aliases

Minus Directional Movement, -DM

## See Also

[PLUS_DM](/functions/plus_dm.md) · [MINUS_DI](/functions/minus_di.md) · [PLUS_DI](/functions/plus_di.md) · [DX](/functions/dx.md) · [ADX](/functions/adx.md) · [ADXR](/functions/adxr.md)

## References

- J. Welles Wilder, *New Concepts in Technical Trading Systems*, Trend Research (ISBN 0894590278)
