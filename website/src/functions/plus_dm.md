---
title: "Plus Directional Movement (PLUS_DM)"
description: "Plus Directional Movement: the Wilder-smoothed accumulation of upward directional movement (+DM1)."
---

## Summary

Plus Directional Movement: the Wilder-smoothed accumulation of upward directional movement (+DM1). A component of the Directional Movement System used to build +DI/DX/ADX.

## Formula

+DM1 = (high - prevHigh) if (high-prevHigh) > 0 and > (prevLow-low), else 0.
period<=1: output = +DM1 per bar.
period>1: seed = sum of first (period-1) +DM1; then Wilder smoothing:
+DM = prevPlusDM - prevPlusDM/period + +DM1(today)

## Inputs

- `inHigh` — High price of each bar
- `inLow` — Low price of each bar

## Outputs

- `outReal` — Smoothed plus directional movement

## Parameters

| Parameter | Type | Default | Accepted values | Description |
| --- | --- | --- | --- | --- |
| `optInTimePeriod` | integer | 14 | 1–100000 | Wilder smoothing period |

## Properties

**Numerical Stability:** [Initial Unstable Period](/functions/stability.md#initial-unstable-period)

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

TA-Lib Definition: [`plus_dm.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/plus_dm/plus_dm.c) · [`plus_dm.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/plus_dm/plus_dm.yaml)

| Native | File |
|--------|------|
| C | [`ta_PLUS_DM.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_PLUS_DM.c) |
| Rust | [`plus_dm.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/library/src/ta_func/plus_dm.rs) |
| Java | [`Core_PLUS_DM.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/fragments/Core_PLUS_DM.java) |

TA-Lib is also available for Python, R and more using a [wrapper](/install/#wrappers).

## Aliases

+DM, Plus Directional Movement

## See Also

[MINUS_DM](/functions/minus_dm.md) · [PLUS_DI](/functions/plus_di.md) · [MINUS_DI](/functions/minus_di.md) · [DX](/functions/dx.md) · [ADX](/functions/adx.md) · [ADXR](/functions/adxr.md)

## References

- J. Welles Wilder, *New Concepts in Technical Trading Systems*, Trend Research (ISBN 0894590278)
