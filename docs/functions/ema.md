---
title: EMA
description: "Exponential moving average that weights recent prices more heavily via a recursive smoothing factor. A core building block seeding or composing many other indicators. Reacts faster than SMA; price above/below EMA suggests up/down trend."
---

# EMA

## Summary

Exponential moving average that weights recent prices more heavily via a recursive smoothing factor. A core building block seeding or composing many other indicators. Reacts faster than SMA; price above/below EMA suggests up/down trend.

## Formula

k = 2 / (period + 1); EMA_t = (price_t - EMA_{t-1}) * k + EMA_{t-1}. Seed (DEFAULT): EMA = SMA of first `period` bars.

## Notes

- In Metastock compatibility mode the average is seeded with the first price value and the recursion starts at the second bar, rather than the default of seeding with a simple average of the first period bars.
- A period of 1 performs no smoothing: the output is a copy of the input. Allowed since 0.6.5 (issues #48/#59).

## Inputs

- `inReal` — price/data series to smooth

## Outputs

- `outReal` — the exponential moving average

## Parameters

- `optInTimePeriod` — number of bars in the average; sets smoothing k = 2/(period+1)

## Implementation

TA-Lib Definition: [`ema.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/ema/ema.c) · [`ema.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/ema/ema.yaml)

| Native | File |
|--------|------|
| C | [`ta_EMA.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_EMA.c) |
| Rust | [`ema.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/src/ta_func/ema.rs) |
| Java | [`Core_EMA.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/Core_EMA.java) |

TA-Lib is also available for Python, R and more using a [wrapper](https://ta-lib.org/wrappers/).

## Aliases

Exponential Moving Average, Exponentially Weighted Moving Average, EWMA

## See Also

[SMA](sma.md) · [DEMA](dema.md) · [TEMA](tema.md) · [MA](ma.md) · [MACD](macd.md) · [T3](t3.md)
