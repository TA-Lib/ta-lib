---
title: TRANGE
description: "True Range: the greatest of today's high-low span and the two gaps between yesterday's close and today's high/low. Base volatility measure used to build ATR/NATR. Larger values mean wider or gappier bars (higher volatility)."
---

# TRANGE

## Summary

True Range: the greatest of today's high-low span and the two gaps between yesterday's close and today's high/low. Base volatility measure used to build ATR/NATR. Larger values mean wider or gappier bars (higher volatility).

## Formula

TR = max( high - low, |prevClose - high|, |prevClose - low| )

## Notes

- The first bar produces no value because it has no prior close; unlike some definitions, it does not fall back to the high-low range for that bar.

## Inputs

- `inPriceHLC` — High, low, and close price series

## Outputs

- `outReal` — True Range value per bar

## Implementation

TA-Lib Definition: [`trange.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/trange/trange.c) · [`trange.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/trange/trange.yaml)

| Native | File |
|--------|------|
| C | [`ta_TRANGE.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_TRANGE.c) |
| Rust | [`trange.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/src/ta_func/trange.rs) |
| Java | [`Core_TRANGE.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/Core_TRANGE.java) |

TA-Lib is also available for Python, R and more using a [wrapper](https://ta-lib.org/wrappers/).

## Aliases

True Range, TR

## See Also

[ATR](atr.md) · [NATR](natr.md)

## References

- J. Welles Wilder, *New Concepts in Technical Trading Systems*, Trend Research (ISBN 0894590278)
