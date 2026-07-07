---
title: TEMA
description: "Triple Exponential Moving Average: a smoothed price overlay built from three successively-applied EMAs to reduce lag versus a plain EMA. Distinct from EMA3, also called \"triple EMA\" in the literature."
---

# TEMA

## Summary

Triple Exponential Moving Average: a smoothed price overlay built from three successively-applied EMAs to reduce lag versus a plain EMA. Distinct from EMA3, also called "triple EMA" in the literature.

## Formula

EMA1=EMA(t,period); EMA2=EMA(EMA1,period); EMA3=EMA(EMA2,period); TEMA = 3*EMA1 - 3*EMA2 + EMA3

## Notes

- A period of 1 performs no smoothing: the output is a copy of the input. Allowed since 0.6.5 (issues #48/#59).

## Inputs

- `inReal` — Source price/data series

## Outputs

- `outReal` — The TEMA line

## Parameters

- `optInTimePeriod` — EMA period used for all three passes

## Implementation

TA-Lib Definition: [`tema.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/tema/tema.c) · [`tema.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/tema/tema.yaml)

| Native | File |
|--------|------|
| C | [`ta_TEMA.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_TEMA.c) |
| Rust | [`tema.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/src/ta_func/tema.rs) |
| Java | [`Core.java`](https://github.com/TA-Lib/ta-lib/blob/main/java/src/com/tictactec/ta/lib/Core.java) |

TA-Lib is also available for Python, R and more using a [wrapper](https://ta-lib.org/wrappers/).

## Aliases

Triple Exponential Moving Average

## See Also

[EMA](ema.md) · [DEMA](dema.md) · [T3](t3.md)

## References

- Patrick G. Mulloy, *Smoothing Data with Faster Moving Averages*, Technical Analysis of Stocks & Commodities, V.12:1 (January 1994)
