---
title: BBANDS
description: "Bollinger Bands: a moving-average middle band with upper and lower bands offset by a multiple of the standard deviation. Used to gauge relative price volatility."
---

# BBANDS

## Summary

Bollinger Bands: a moving-average middle band with upper and lower bands offset by a multiple of the standard deviation. Used to gauge relative price volatility.

## Formula

middle = MA(inReal, period); sd = stddev(inReal, period); upper = middle + nbDevUp*sd; lower = middle - nbDevDn*sd

## Notes

- The standard deviation uses the population form (dividing by the period), not the sample form.
- The standard deviation is always computed with a simple moving average regardless of the selected MA type.

## Inputs

- `inReal` — Input data series

## Outputs

- `outRealUpperBand` — Middle band plus nbDevUp standard deviations
- `outRealMiddleBand` — The moving average
- `outRealLowerBand` — Middle band minus nbDevDn standard deviations

## Parameters

- `optInTimePeriod` — Periods for the MA and standard deviation
- `optInNbDevUp` — Standard-deviation multiplier for the upper band
- `optInNbDevDn` — Standard-deviation multiplier for the lower band
- `optInMAType` — Moving-average type for the middle band

## Implementation

TA-Lib Definition: [`bbands.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/bbands/bbands.c) · [`bbands.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/bbands/bbands.yaml)

| Native | File |
|--------|------|
| C | [`ta_BBANDS.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_BBANDS.c) |
| Rust | [`bbands.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/src/ta_func/bbands.rs) |
| Java | [`Core_BBANDS.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/Core_BBANDS.java) |

TA-Lib is also available for Python, R and more using a [wrapper](https://ta-lib.org/wrappers/).

## Aliases

Bollinger Bands

## See Also

[MA](ma.md) · [STDDEV](stddev.md) · [SMA](sma.md)

## References

- John A. Bollinger, *Bollinger on Bollinger Bands*, McGraw-Hill Trade (ISBN 0071373683)
