---
title: CDLTHRUSTING
description: "A two-candle pattern: a long black candle followed by a white candle that opens below the prior low and closes back into the prior body but below its midpoint. It is a bearish continuation signal. A hit is bearish: the failed white push back into the black body signals continuation of the down move."
---

# CDLTHRUSTING

## Summary

A two-candle pattern: a long black candle followed by a white candle that opens below the prior low and closes back into the prior body but below its midpoint. It is a bearish continuation signal. A hit is bearish: the failed white push back into the black body signals continuation of the down move.

## Notes

- The pattern is classically meaningful only in a downtrend, but this function does not verify any prior trend.
- Although the pattern can be read as bullish in an uptrend or when it recurs, this function ignores trend and always reports it as bearish.

## Inputs

- `inPriceOHLC` — OHLC price series (open, high, low, close)

## Outputs

- `outInteger` — -100 when the pattern is detected, 0 otherwise. Always bearish; never emits +100

## Implementation

TA-Lib Definition: [`cdlthrusting.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/cdlthrusting/cdlthrusting.c) · [`cdlthrusting.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/cdlthrusting/cdlthrusting.yaml)

| Native | File |
|--------|------|
| C | [`ta_CDLTHRUSTING.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_CDLTHRUSTING.c) |
| Rust | [`cdlthrusting.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/library/src/ta_func/cdlthrusting.rs) |
| Java | [`Core_CDLTHRUSTING.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/library/fragments/Core_CDLTHRUSTING.java) |

TA-Lib is also available for Python, R and more using a [wrapper](https://ta-lib.org/wrappers/).

## Aliases

Thrusting Pattern, Thrusting Line

## See Also

[CDLINNECK](/functions/cdlinneck) · [CDLONNECK](/functions/cdlonneck) · [CDLPIERCING](/functions/cdlpiercing) · CDLMEETINGLINES
