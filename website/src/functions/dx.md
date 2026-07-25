---
title: DX
description: "Wilder's Directional Movement Index: the normalized spread between +DI and -DI. Measures the strength of directional (trending) movement, irrespective of direction. Higher DX = stronger trend (either direction); low DX = ranging market."
---

# DX

## Summary

Wilder's Directional Movement Index: the normalized spread between +DI and -DI. Measures the strength of directional (trending) movement, irrespective of direction. Higher DX = stronger trend (either direction); low DX = ranging market.

## Formula

Seed +DM14, -DM14, TR14 as sums of the first (period-1) one-period values, then Wilder-smooth each: X = X - X/period + today. +DI = 100*(+DM14/TR14), -DI = 100*(-DM14/TR14). DX = 100 * |(-DI) - (+DI)| / ((-DI) + (+DI)).

## Notes

- Wilder's original integer rounding is not applied (it can be unreliable when values are near 1).
- When +DI and -DI sum to zero the value is undefined; the previous bar's DX is carried forward instead (the first such bar outputs zero).

## Inputs

- `inHigh` — High price of each bar
- `inLow` — Low price of each bar
- `inClose` — Close price of each bar

## Outputs

- `outReal` — DX directional movement index value

## Parameters

| Parameter | Type | Default | Accepted values | Description |
| --- | --- | --- | --- | --- |
| `optInTimePeriod` | integer | 14 | 2–100000 | Smoothing period for the DM and TR sums |

## Properties

**Numerical Stability:** [Initial Unstable Period](/functions/stability#initial-unstable-period)

| Display<br>Flags |
| :-- |
| <span class="flag-box">☐</span> <span style="opacity:0.5">Overlap Input</span> |
| <span class="flag-box">✅</span> **Independent Y-Axis** <span class="flag-tip" tabindex="0" role="note" aria-label="Output is on its own scale, drawn in a separate pane below the price chart." data-tip="Output is on its own scale, drawn in a separate pane below the price chart.">i</span> |
| <span class="flag-box">☐</span> <span style="opacity:0.5">Candlestick</span> |

## Implementation

TA-Lib Definition: [`dx.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/dx/dx.c) · [`dx.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/dx/dx.yaml)

| Native | File |
|--------|------|
| C | [`ta_DX.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_DX.c) |
| Rust | [`dx.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/library/src/ta_func/dx.rs) |
| Java | [`Core_DX.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/library/fragments/Core_DX.java) |

TA-Lib is also available for Python, R and more using a [wrapper](https://ta-lib.org/wrappers/).

## Aliases

Directional Movement Index, DMI

## See Also

[ADX](/functions/adx) · [ADXR](/functions/adxr) · [PLUS_DI](/functions/plus_di) · [MINUS_DI](/functions/minus_di) · [PLUS_DM](/functions/plus_dm) · [MINUS_DM](/functions/minus_dm) · [TRANGE](/functions/trange)

## References

- J. Welles Wilder, *New Concepts in Technical Trading Systems*, Trend Research (ISBN 0894590278)
