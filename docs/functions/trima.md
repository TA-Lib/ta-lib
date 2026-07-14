---
title: TRIMA
description: "Triangular Moving Average: a double-smoothed moving average that weights prices toward the middle of the window most heavily. Equivalent to an SMA of an SMA, computed here via an incremental triangular-weighted running numerator."
---

# TRIMA

## Summary

Triangular Moving Average: a double-smoothed moving average that weights prices toward the middle of the window most heavily. Equivalent to an SMA of an SMA, computed here via an incremental triangular-weighted running numerator.

## Formula

Weights rise then fall (4-period: (1a+2b+2c+1d)/6; 5-period: (1a+2b+3c+2d+1e)/9). With n = period>>1: odd divides by (n+1)^2, even by n(n+1). Equivalent to odd: SMA(SMA(x,(period+1)/2),(period+1)/2); even: SMA(SMA(x,period/2),period/2+1).

## Notes

- Follows the generally accepted (Metastock) definition rather than the TradeStation variant.
- A period of 1 performs no smoothing: the output is a copy of the input. Allowed since 0.6.5 (issues #48/#59).

## Inputs

- `inReal` — Source price series

## Outputs

- `outReal` — Triangular moving average

## Parameters

- `optInTimePeriod` — Number of bars in the averaging window

## Implementation

TA-Lib Definition: [`trima.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/trima/trima.c) · [`trima.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/trima/trima.yaml)

| Native | File |
|--------|------|
| C | [`ta_TRIMA.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_TRIMA.c) |
| Rust | [`trima.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/src/ta_func/trima.rs) |
| Java | [`Core_TRIMA.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/Core_TRIMA.java) |

TA-Lib is also available for Python, R and more using a [wrapper](https://ta-lib.org/wrappers/).

## Aliases

Triangular Moving Average

## See Also

[SMA](/functions/sma.md) · [WMA](/functions/wma.md) · [MA](/functions/ma.md)
