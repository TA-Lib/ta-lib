---
title: ADX
description: "Wilder's Average Directional Movement Index, a smoothed measure of trend strength derived from the directional indicators (+DI/-DI). Quantifies how strongly a market is trending, regardless of direction. Higher values indicate a stronger trend (a common convention treats >25 as trending); says nothing about direction."
---

# ADX

## Summary

Wilder's Average Directional Movement Index, a smoothed measure of trend strength derived from the directional indicators (+DI/-DI). Quantifies how strongly a market is trending, regardless of direction. Higher values indicate a stronger trend (a common convention treats >25 as trending); says nothing about direction.

## Formula

+DI = 100*(+DM_p/TR_p), -DI = 100*(-DM_p/TR_p); DX = 100*|(-DI)-(+DI)| / ((-DI)+(+DI)); first ADX = mean of the first `period` DX; then ADX = (prevADX*(period-1) + DX)/period. +DM_p/-DM_p/TR_p use Wilder smoothing: X = X - X/period + today's one-bar value.

## Notes

- Wilder's original integer rounding is not applied.

## Inputs

- `inPriceHLC` — High/Low/Close price series

## Outputs

- `outReal` — Smoothed directional trend-strength index (0-100)

## Parameters

- `optInTimePeriod` — Smoothing/averaging period for DM, TR, and ADX

## Implementation

TA-Lib Definition: [`adx.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/adx/adx.c) · [`adx.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/adx/adx.yaml)

| Native | File |
|--------|------|
| C | [`ta_ADX.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_ADX.c) |
| Rust | [`adx.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/src/ta_func/adx.rs) |
| Java | [`Core_ADX.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/Core_ADX.java) |

TA-Lib is also available for Python, R and more using a [wrapper](https://ta-lib.org/wrappers/).

## Aliases

Average Directional Movement Index, Average Directional Index

## See Also

[ADXR](adxr.md) · [DX](dx.md) · [PLUS_DI](plus_di.md) · [MINUS_DI](minus_di.md) · [PLUS_DM](plus_dm.md) · [MINUS_DM](minus_dm.md) · [TRANGE](trange.md)

## References

- J. Welles Wilder, *New Concepts in Technical Trading Systems*, Trend Research (ISBN 0894590278)
