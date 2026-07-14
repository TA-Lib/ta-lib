---
title: CCI
description: "Commodity Channel Index: measures the current typical price relative to its simple moving average, scaled by mean absolute deviation. Momentum oscillator flagging overbought/oversold extremes. CCI > +100 overbought; CCI < -100 oversold."
---

# CCI

## Summary

Commodity Channel Index: measures the current typical price relative to its simple moving average, scaled by mean absolute deviation. Momentum oscillator flagging overbought/oversold extremes. CCI > +100 overbought; CCI < -100 oversold.

## Formula

TP_i = (High_i + Low_i + Close_i)/3
SMA = (1/N) * sum(TP over N bars)
meanDev = (1/N) * sum(|TP - SMA| over N bars)
CCI = (TP_last - SMA) / (0.015 * meanDev)

## Inputs

- `inPriceHLC` — High, Low, Close series (typical price is their average)

## Outputs

- `outReal` — CCI value per bar

## Parameters

- `optInTimePeriod` — Number of bars in the averaging/deviation window

## Implementation

TA-Lib Definition: [`cci.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/cci/cci.c) · [`cci.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/cci/cci.yaml)

| Native | File |
|--------|------|
| C | [`ta_CCI.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_CCI.c) |
| Rust | [`cci.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/src/ta_func/cci.rs) |
| Java | [`Core_CCI.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/Core_CCI.java) |

TA-Lib is also available for Python, R and more using a [wrapper](https://ta-lib.org/wrappers/).

## Aliases

Commodity Channel Index

## See Also

[TYPPRICE](/functions/typprice) · [SMA](/functions/sma)

## References

- Donald Lambert
