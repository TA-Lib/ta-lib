---
title: DEMA
description: "Double Exponential Moving Average: an EMA combined with an EMA-of-EMA to reduce lag versus a plain EMA. Overlap Studies overlay on price."
---

# DEMA

## Summary

Double Exponential Moving Average: an EMA combined with an EMA-of-EMA to reduce lag versus a plain EMA. Overlap Studies overlay on price.

## Formula

EMA1 = EMA(inReal, period); EMA2 = EMA(EMA1, period); DEMA = 2*EMA1 - EMA2

## Notes

- A period of 1 performs no smoothing: the output is a copy of the input. Allowed since 0.6.5 (issues #48/#59).

## Inputs

- `inReal` — Source series (typically price)

## Outputs

- `outReal` — DEMA line

## Parameters

- `optInTimePeriod` — Smoothing period for both EMA passes

## Implementation

TA-Lib Definition: [`dema.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/dema/dema.c) · [`dema.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/dema/dema.yaml)

| Native | File |
|--------|------|
| C | [`ta_DEMA.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_DEMA.c) |
| Rust | [`dema.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/library/src/ta_func/dema.rs) |
| Java | [`Core_DEMA.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/library/fragments/Core_DEMA.java) |

TA-Lib is also available for Python, R and more using a [wrapper](https://ta-lib.org/wrappers/).

## Aliases

Double Exponential Moving Average

## See Also

[EMA](/functions/ema) · [TEMA](/functions/tema) · [MA](/functions/ma)

## References

- Patrick G. Mulloy, *Smoothing Data with Faster Moving Averages*, Technical Analysis of Stocks & Commodities, V.12:1 (January 1994)
