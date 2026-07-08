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

- `inPriceHLC` — High, low, close price series

## Outputs

- `outReal` — ATR as a percentage of the close

## Parameters

- `optInTimePeriod` — Smoothing period for the true range average

## Implementation

TA-Lib Definition: [`natr.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/natr/natr.c) · [`natr.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/natr/natr.yaml)

| Native | File |
|--------|------|
| C | [`ta_NATR.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_NATR.c) |
| Rust | [`natr.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/src/ta_func/natr.rs) |
| Java | [`Core_NATR.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/Core_NATR.java) |

TA-Lib is also available for Python, R and more using a [wrapper](https://ta-lib.org/wrappers/).

## Aliases

Normalized Average True Range

## See Also

[ATR](atr.md) · [TRANGE](trange.md) · [SMA](sma.md)

## References

- John Forman
