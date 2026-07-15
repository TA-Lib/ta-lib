# DX

## Summary

Wilder's Directional Movement Index: the normalized spread between +DI and -DI. Measures the strength of directional (trending) movement, irrespective of direction. Higher DX = stronger trend (either direction); low DX = ranging market.

## Formula

Seed +DM14, -DM14, TR14 as sums of the first (period-1) one-period values, then Wilder-smooth each: X = X - X/period + today. +DI = 100*(+DM14/TR14), -DI = 100*(-DM14/TR14). DX = 100 * |(-DI) - (+DI)| / ((-DI) + (+DI)).

## Notes

- Wilder's original integer rounding is not applied (it can be unreliable when values are near 1).
- When +DI and -DI sum to zero the value is undefined; the previous bar's DX is carried forward instead (the first such bar outputs zero).

## Inputs

- `inPriceHLC` — High/Low/Close price series

## Outputs

- `outReal` — DX directional movement index value

## Parameters

- `optInTimePeriod` — Smoothing period for the DM and TR sums

## Implementation

TA-Lib Definition: [`dx.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/dx/dx.c) · [`dx.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/dx/dx.yaml)

| Native | File |
|--------|------|
| C | [`ta_DX.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_DX.c) |
| Rust | [`dx.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/library/src/ta_func/dx.rs) |
| Java | [`Core_DX.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/library/fragments/Core_DX.java) |

TA-Lib is also available for Python, R and more using a [wrapper](https://ta-lib.org/wrappers/).

## Aliases

Directional Movement Index, DMI

## See Also

ADX · ADXR · PLUS_DI · MINUS_DI · PLUS_DM · MINUS_DM · TRANGE

## References

- J. Welles Wilder, *New Concepts in Technical Trading Systems*, Trend Research (ISBN 0894590278)
