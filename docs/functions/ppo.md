---
title: PPO
description: "Percentage Price Oscillator: the difference between a fast and slow moving average expressed as a percentage of the slow MA. A normalized (scale-invariant) variant of APO. Positive when the fast MA is above the slow MA (upward momentum), negative otherwise; magnitude is the % deviation."
---

# PPO

## Summary

Percentage Price Oscillator: the difference between a fast and slow moving average expressed as a percentage of the slow MA. A normalized (scale-invariant) variant of APO. Positive when the fast MA is above the slow MA (upward momentum), negative otherwise; magnitude is the % deviation.

## Formula

PPO = ((fastMA(inReal) - slowMA(inReal)) / slowMA(inReal)) * 100, both MAs of type optInMAType; output = 0 when slowMA == 0

## Inputs

- `inReal` — Input data series

## Outputs

- `outReal` — PPO value in percent

## Parameters

- `optInFastPeriod` — Period of the fast MA
- `optInSlowPeriod` — Period of the slow MA
- `optInMAType` — Moving average type used for both MAs

## Implementation

TA-Lib Definition: [`ppo.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/ppo/ppo.c) · [`ppo.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/ppo/ppo.yaml)

| Native | File |
|--------|------|
| C | [`ta_PPO.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_PPO.c) |
| Rust | [`ppo.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/src/ta_func/ppo.rs) |
| Java | [`Core_PPO.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/Core_PPO.java) |

TA-Lib is also available for Python, R and more using a [wrapper](https://ta-lib.org/wrappers/).

## Aliases

Percentage Price Oscillator

## See Also

[APO](/functions/apo.md) · [MACD](/functions/macd.md) · [MA](/functions/ma.md)
