---
title: PLUS_DM
description: "Plus Directional Movement: the Wilder-smoothed accumulation of upward directional movement (+DM1). A component of the Directional Movement System used to build +DI/DX/ADX."
---

# PLUS_DM

## Summary

Plus Directional Movement: the Wilder-smoothed accumulation of upward directional movement (+DM1). A component of the Directional Movement System used to build +DI/DX/ADX.

## Formula

+DM1 = (high - prevHigh) if (high-prevHigh) > 0 and > (prevLow-low), else 0.
period<=1: output = +DM1 per bar.
period>1: seed = sum of first (period-1) +DM1; then Wilder smoothing:
+DM = prevPlusDM - prevPlusDM/period + +DM1(today)

## Inputs

- `inPriceHL` — High and low price series

## Outputs

- `outReal` — Smoothed plus directional movement

## Parameters

- `optInTimePeriod` — Wilder smoothing period

## Implementation

TA-Lib Definition: [`plus_dm.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/plus_dm/plus_dm.c) · [`plus_dm.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/plus_dm/plus_dm.yaml)

| Native | File |
|--------|------|
| C | [`ta_PLUS_DM.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_PLUS_DM.c) |
| Rust | [`plus_dm.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/library/src/ta_func/plus_dm.rs) |
| Java | [`Core_PLUS_DM.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/library/fragments/Core_PLUS_DM.java) |

TA-Lib is also available for Python, R and more using a [wrapper](https://ta-lib.org/wrappers/).

## Aliases

+DM, Plus Directional Movement

## See Also

[MINUS_DM](/functions/minus_dm) · [PLUS_DI](/functions/plus_di) · [MINUS_DI](/functions/minus_di) · [DX](/functions/dx) · [ADX](/functions/adx) · [ADXR](/functions/adxr)

## References

- J. Welles Wilder, *New Concepts in Technical Trading Systems*, Trend Research (ISBN 0894590278)
