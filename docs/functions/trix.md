---
title: TRIX
description: "1-day Rate-Of-Change of a triple-smoothed EMA of the input. Momentum oscillator that filters out price moves shorter than the chosen period. Oscillates around zero; sign, zero-crossings and slope signal momentum direction."
---

# TRIX

## Summary

1-day Rate-Of-Change of a triple-smoothed EMA of the input. Momentum oscillator that filters out price moves shorter than the chosen period. Oscillates around zero; sign, zero-crossings and slope signal momentum direction.

## Formula

E1 = EMA(inReal, n); E2 = EMA(E1, n); E3 = EMA(E2, n); TRIX = ROC_1(E3) = 100 * (E3_today/E3_yesterday - 1)

## Notes

- The final rate-of-change step yields 0 when the previous smoothed value is exactly zero, rather than being undefined.

## Inputs

- `inReal` — Source series to smooth

## Outputs

- `outReal` — 1-day percent ROC of the triple EMA

## Parameters

- `optInTimePeriod` — EMA period used at each of the three smoothing passes

## Implementation

TA-Lib Definition: [`trix.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/trix/trix.c) · [`trix.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/trix/trix.yaml)

| Native | File |
|--------|------|
| C | [`ta_TRIX.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_TRIX.c) |
| Rust | [`trix.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/src/ta_func/trix.rs) |
| Java | [`Core_TRIX.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/Core_TRIX.java) |

TA-Lib is also available for Python, R and more using a [wrapper](https://ta-lib.org/wrappers/).

## Aliases

Triple Exponential Average

## See Also

[EMA](ema.md) · [ROC](roc.md) · [ROCR](rocr.md) · [TEMA](tema.md)

## References

- Jack K. Hutson, Technical Analysis of Stocks & Commodities (1980s)
