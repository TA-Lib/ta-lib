---
title: MA
description: "Generic moving-average dispatcher that forwards the job to a concrete MA implementation selected by optInMAType. Single uniform interface over all TA-Lib moving averages."
---

# MA

## Summary

Generic moving-average dispatcher that forwards the job to a concrete MA implementation selected by optInMAType. Single uniform interface over all TA-Lib moving averages.

## Formula

outReal = MA_of_type(optInMAType)(inReal, optInTimePeriod); default type = SMA

## Notes

- A period of 1 performs no smoothing for every MAType: the output is a copy of the input.

## Inputs

- `inReal` — Series to average

## Outputs

- `outReal` — Selected moving average of the input

## Parameters

- `optInTimePeriod` — Averaging window length
- `optInMAType` — Which moving-average algorithm to dispatch to

## Implementation

TA-Lib Definition: [`ma.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/ma/ma.c) · [`ma.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/ma/ma.yaml)

| Native | File |
|--------|------|
| C | [`ta_MA.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_MA.c) |
| Rust | [`ma.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/src/ta_func/ma.rs) |
| Java | [`Core_MA.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/Core_MA.java) |

TA-Lib is also available for Python, R and more using a [wrapper](https://ta-lib.org/wrappers/).

## Aliases

Moving Average, MovingAverage

## See Also

[SMA](/functions/sma) · [EMA](/functions/ema) · [WMA](/functions/wma) · [DEMA](/functions/dema) · [TEMA](/functions/tema) · [TRIMA](/functions/trima) · [KAMA](/functions/kama) · [MAMA](/functions/mama) · [T3](/functions/t3)
