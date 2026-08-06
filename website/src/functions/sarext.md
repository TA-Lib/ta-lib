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

- `inHigh` — High price of each bar
- `inLow` — Low price of each bar

## Outputs

- `outReal` — SAR stop level; positive while long, negative while short

## Parameters

| Parameter | Type | Default | Accepted values | Description |
| --- | --- | --- | --- | --- |
| `optInStartValue` | real | 0 | any real | Initial SAR/direction: 0 auto, >0 start long at value, <0 start short at \|value\| |
| `optInOffsetOnReverse` | real | 0 | ≥ 0 | Fractional offset applied to the stop on each reversal |
| `optInAccelerationInitLong` | real | 0.02 | ≥ 0 | Initial acceleration factor when long |
| `optInAccelerationLong` | real | 0.02 | ≥ 0 | AF increment per new long extreme |
| `optInAccelerationMaxLong` | real | 0.2 | ≥ 0 | Cap on the long acceleration factor |
| `optInAccelerationInitShort` | real | 0.02 | ≥ 0 | Initial acceleration factor when short |
| `optInAccelerationShort` | real | 0.02 | ≥ 0 | AF increment per new short extreme |
| `optInAccelerationMaxShort` | real | 0.2 | ≥ 0 | Cap on the short acceleration factor |

## Properties

**Numerical Stability:** [Path-Dependent](/functions/stability#path-dependent)

| Display<br>Flags |
| :-- |
| <span class="flag-box">✅</span> **Overlap Input** <span class="flag-tip" tabindex="0" role="note" aria-label="Output is on the same scale as the input price, so it is drawn over the price chart." data-tip="Output is on the same scale as the input price, so it is drawn over the price chart.">i</span> |
| <span class="flag-box">☐</span> <span style="opacity:0.5">Independent Y-Axis</span> |
| <span class="flag-box">☐</span> <span style="opacity:0.5">Candlestick</span> |

## Implementation

TA-Lib Definition: [`sarext.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/sarext/sarext.c) · [`sarext.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/sarext/sarext.yaml)

| Native | File |
|--------|------|
| C | [`ta_SAREXT.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_SAREXT.c) |
| Rust | [`sarext.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/library/src/ta_func/sarext.rs) |
| Java | [`Core_SAREXT.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/fragments/Core_SAREXT.java) |

TA-Lib is also available for Python, R and more using a [wrapper](https://ta-lib.org/wrappers/).

## Aliases

Parabolic SAR Extended, Extended Parabolic Stop and Reverse

## See Also

[SAR](/functions/sar) · [MINUS_DM](/functions/minus_dm)
