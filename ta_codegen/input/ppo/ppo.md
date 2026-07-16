# PPO

## Summary

Percentage Price Oscillator: the difference between a fast and slow moving average expressed as a percentage of the slow MA. A normalized (scale-invariant) variant of APO. Positive when the fast MA is above the slow MA (upward momentum), negative otherwise; magnitude is the % deviation.

## Formula

PPO = ((fastMA(inReal) - slowMA(inReal)) / slowMA(inReal)) * 100, both MAs of type optInMAType; output = 0 when slowMA == 0

The standard form is exponential with periods 12 and 26 — ((12-day EMA - 26-day EMA) / 26-day EMA) * 100, i.e. the MACD oscillator expressed as a percentage. `optInMAType` therefore **defaults to EMA** — the moving average Gerald Appel used for the original PPO/MACD; pass another type (e.g. `TA_MAType_SMA`) to override.

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
| Rust | [`ppo.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/library/src/ta_func/ppo.rs) |
| Java | [`Core_PPO.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/library/fragments/Core_PPO.java) |

TA-Lib is also available for Python, R and more using a [wrapper](https://ta-lib.org/wrappers/).

## Aliases

Percentage Price Oscillator

## See Also

APO · MACD · MA

## References

- Gerald Appel, creator of the PPO and MACD (MACD introduced 1979 in his *Systems and Forecasts* newsletter). The PPO is the MACD expressed as a percentage of the slow moving average. Appel's original definition uses **exponential** moving averages (periods 12, 26).
