---
title: SAR
description: "Wilder's Parabolic SAR (Stop And Reverse): a trailing stop/reverse level that accelerates toward price via an acceleration factor. Signals trend direction and trailing exit points. SAR below price = uptrend (long); SAR above price = downtrend (short). Price crossing SAR flips direction."
---

# SAR

## Summary

Wilder's Parabolic SAR (Stop And Reverse): a trailing stop/reverse level that accelerates toward price via an acceleration factor. Signals trend direction and trailing exit points. SAR below price = uptrend (long); SAR above price = downtrend (short). Price crossing SAR flips direction.

## Formula

SAR_next = SAR + af * (EP - SAR)
EP = extreme point (highest high in long / lowest low in short); af starts at Acceleration, += Acceleration each new EP, capped at Maximum.
On penetration: reverse, SAR := prior EP, reset af = Acceleration. SAR clamped each bar so it does not penetrate the prior/current bar's range.

## Inputs

- `inPriceHL` — High/Low price series

## Outputs

- `outReal` — Parabolic SAR stop/reverse level per bar

## Parameters

- `optInAcceleration` — Step added to the acceleration factor on each new extreme point
- `optInMaximum` — Ceiling on the acceleration factor

## Implementation

TA-Lib Definition: [`sar.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/sar/sar.c) · [`sar.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/sar/sar.yaml)

| Native | File |
|--------|------|
| C | [`ta_SAR.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_SAR.c) |
| Rust | [`sar.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/src/ta_func/sar.rs) |
| Java | [`Core_SAR.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/Core_SAR.java) |

TA-Lib is also available for Python, R and more using a [wrapper](https://ta-lib.org/wrappers/).

## Aliases

Parabolic SAR, PSAR, Stop and Reverse

## See Also

[SAREXT](/functions/sarext.md) · [MINUS_DM](/functions/minus_dm.md) · [PLUS_DM](/functions/plus_dm.md)

## References

- J. Welles Wilder, *New Concepts in Technical Trading Systems*, Trend Research (ISBN 0894590278)
