---
title: ACCBANDS
description: "Acceleration Bands: three overlap lines around price. The middle band is an SMA of the close; the upper/lower bands are SMAs of the high/low scaled by an intraday-range factor."
---

# ACCBANDS

## Summary

Acceleration Bands: three overlap lines around price. The middle band is an SMA of the close; the upper/lower bands are SMAs of the high/low scaled by an intraday-range factor.

## Formula

factor = 4*(H-L)/(H+L)
upperRaw = H*(1+factor), lowerRaw = L*(1-factor)
Upper = SMA(upperRaw, N), Middle = SMA(Close, N), Lower = SMA(lowerRaw, N)

## Inputs

- `inPriceHLC` — High/Low/Close price bars

## Outputs

- `outRealUpperBand` — SMA of the range-scaled high band
- `outRealMiddleBand` — SMA of the close
- `outRealLowerBand` — SMA of the range-scaled low band

## Parameters

- `optInTimePeriod` — SMA smoothing period for all three bands

## Implementation

TA-Lib Definition: [`accbands.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/accbands/accbands.c) · [`accbands.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/accbands/accbands.yaml)

| Native | File |
|--------|------|
| C | [`ta_ACCBANDS.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_ACCBANDS.c) |
| Rust | [`accbands.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/src/ta_func/accbands.rs) |
| Java | [`Core_ACCBANDS.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/Core_ACCBANDS.java) |

TA-Lib is also available for Python, R and more using a [wrapper](https://ta-lib.org/wrappers/).

## Aliases

Acceleration Bands

## See Also

[SMA](sma.md) · [BBANDS](bbands.md)
