---
title: RSI
description: "Wilder's Relative Strength Index, a momentum oscillator bounded 0-100 from the ratio of average gains to average losses over the period. Used to gauge overbought/oversold conditions. >70 overbought, <30 oversold."
---

# RSI

## Summary

Wilder's Relative Strength Index, a momentum oscillator bounded 0-100 from the ratio of average gains to average losses over the period. Used to gauge overbought/oversold conditions. >70 overbought, <30 oversold.

## Formula

Initial avgGain/avgLoss = simple mean of up/down moves over the period, then Wilder-smoothed each bar: $avg = (avg_{prev}\cdot(period-1) + move)/period$. $RSI = 100\cdot avgGain/(avgGain+avgLoss)$ (equivalent to $100 - 100/(1+RS)$).

## Notes

- In Metastock-compatibility mode an extra initial value is emitted, treating the first bar as having no gain or loss.

## Inputs

- `inReal` — Price series (typically close)

## Outputs

- `outReal` — RSI value

## Parameters

- `optInTimePeriod` — Lookback for the gain/loss averaging

## Implementation

TA-Lib Definition: [`rsi.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/rsi/rsi.c) · [`rsi.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/rsi/rsi.yaml)

| Native | File |
|--------|------|
| C | [`ta_RSI.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_RSI.c) |
| Rust | [`rsi.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/src/ta_func/rsi.rs) |
| Java | [`Core_RSI.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/Core_RSI.java) |

TA-Lib is also available for Python, R and more using a [wrapper](https://ta-lib.org/wrappers/).

## Aliases

relative strength index

## See Also

[CMO](/functions/cmo) · [STOCHRSI](/functions/stochrsi)

## References

- J. Welles Wilder, *New Concepts in Technical Trading Systems*, Trend Research (ISBN 0894590278)
