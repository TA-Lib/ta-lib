---
title: APO
description: "Absolute Price Oscillator: the difference between a fast and a slow moving average of the input, in price units. Measures short- vs long-term momentum. Positive when fast MA > slow MA (upward momentum); negative otherwise."
---

# APO

## Summary

Absolute Price Oscillator: the difference between a fast and a slow moving average of the input, in price units. Measures short- vs long-term momentum. Positive when fast MA > slow MA (upward momentum); negative otherwise.

## Formula

$APO = MA_{fast}(inReal) - MA_{slow}(inReal)$, both MAs of type optInMAType

## Inputs

- `inReal` — Source data series

## Outputs

- `outReal` — Fast MA minus slow MA

## Parameters

- `optInFastPeriod` — Period of the fast moving average
- `optInSlowPeriod` — Period of the slow moving average
- `optInMAType` — Moving-average type used for both MAs

## Implementation

TA-Lib Definition: [`apo.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/apo/apo.c) · [`apo.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/apo/apo.yaml)

| Native | File |
|--------|------|
| C | [`ta_APO.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_APO.c) |
| Rust | [`apo.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/library/src/ta_func/apo.rs) |
| Java | [`Core_APO.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/library/fragments/Core_APO.java) |

TA-Lib is also available for Python, R and more using a [wrapper](https://ta-lib.org/wrappers/).

## Aliases

Absolute Price Oscillator

## See Also

[PPO](/functions/ppo) · [MACD](/functions/macd) · [MA](/functions/ma) · [EMA](/functions/ema) · [SMA](/functions/sma)
