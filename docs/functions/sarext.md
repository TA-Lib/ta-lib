---
title: SAREXT
description: "Extended Parabolic SAR (stop and reverse) giving the caller full control over the initial state and separate acceleration factors for long and short positions. Unlike SAR, it returns negative values while short so reversals are distinguishable. Sign flip of the output marks a trend reversal (positive=long stop, negative=short stop)."
---

# SAREXT

## Summary

Extended Parabolic SAR (stop and reverse) giving the caller full control over the initial state and separate acceleration factors for long and short positions. Unlike SAR, it returns negative values while short so reversals are distinguishable. Sign flip of the output marks a trend reversal (positive=long stop, negative=short stop).

## Formula

SAR_next = SAR + AF*(EP - SAR), then clamped within the prior and current bar's range. On penetration, reverse: set SAR=EP (clamped), reset AF to its Init value, EP=extreme of the new direction. Output is +SAR when long, -SAR when short. On reversal an optional offset is applied: long->short SAR*(1+offset), short->long SAR*(1-offset).

## Inputs

- `inPriceHL` — High and low price series

## Outputs

- `outReal` — SAR stop level; positive while long, negative while short

## Parameters

- `optInStartValue` — Initial SAR/direction: 0 auto, >0 start long at value, <0 start short at |value|
- `optInOffsetOnReverse` — Fractional offset applied to the stop on each reversal
- `optInAccelerationInitLong` — Initial acceleration factor when long
- `optInAccelerationLong` — AF increment per new long extreme
- `optInAccelerationMaxLong` — Cap on the long acceleration factor
- `optInAccelerationInitShort` — Initial acceleration factor when short
- `optInAccelerationShort` — AF increment per new short extreme
- `optInAccelerationMaxShort` — Cap on the short acceleration factor

## Implementation

TA-Lib Definition: [`sarext.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/sarext/sarext.c) · [`sarext.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/sarext/sarext.yaml)

| Native | File |
|--------|------|
| C | [`ta_SAREXT.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_SAREXT.c) |
| Rust | [`sarext.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/src/ta_func/sarext.rs) |
| Java | [`Core_SAREXT.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/Core_SAREXT.java) |

TA-Lib is also available for Python, R and more using a [wrapper](https://ta-lib.org/wrappers/).

## Aliases

Parabolic SAR Extended, Extended Parabolic Stop and Reverse

## See Also

[SAR](/functions/sar.md) · [MINUS_DM](/functions/minus_dm.md)
