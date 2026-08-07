---
title: "Plus Directional Indicator (PLUS_DI)"
description: "Plus Directional Indicator: the Wilder-smoothed positive directional movement expressed as a percentage of the true range."
---

## Summary

Plus Directional Indicator: the Wilder-smoothed positive directional movement expressed as a percentage of the true range. Measures the strength of upward price movement. Rising +DI signals strengthening upward direction; compared against MINUS_DI to judge trend direction.

## Formula

+DM1 = (H-Hprev) if (H-Hprev) > 0 and (H-Hprev) > (Lprev-L), else 0.
TR1 = true range = max(H-L, |H-Cprev|, |L-Cprev|).
Seed +DM/TR = sum of first (period-1) one-period values; then Wilder smooth: X = X - X/period + X1.
+DI = 100 * (+DM / TR); if TR = 0, +DI = 0.
When period <= 1: +DI = +DM1 / TR1 (no *100).

## Notes

- Wilder's original integer rounding of intermediate values is not applied (it was unreliable when values are near 1).

## Inputs

- `inHigh` — High price of each bar
- `inLow` — Low price of each bar
- `inClose` — Close price of each bar

## Outputs

- `outReal` — Plus Directional Indicator

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

TA-Lib Definition: [`plus_di.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/plus_di/plus_di.c) · [`plus_di.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/plus_di/plus_di.yaml)

| Native | File |
|--------|------|
| C | [`ta_PLUS_DI.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_PLUS_DI.c) |
| Rust | [`plus_di.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/library/src/ta_func/plus_di.rs) |
| Java | [`Core_PLUS_DI.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/fragments/Core_PLUS_DI.java) |

TA-Lib is also available for Python, R and more using a [wrapper](/install/#wrappers).

## Aliases

+DI, Plus Directional Indicator, PDI

## See Also

[MINUS_DI](/functions/minus_di.md) · [DX](/functions/dx.md) · [ADX](/functions/adx.md) · [ADXR](/functions/adxr.md) · [PLUS_DM](/functions/plus_dm.md) · [TRANGE](/functions/trange.md)

## References

- J. Welles Wilder, *New Concepts in Technical Trading Systems*, Trend Research (ISBN 0894590278)
