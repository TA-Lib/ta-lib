---
title: ROCR
description: "Rate of Change Ratio: the ratio of the current price to the price optInTimePeriod bars ago. A momentum measure centered at 1. Always positive, centered at 1: >1 rising, <1 falling."
---

# ROCR

## Summary

Rate of Change Ratio: the ratio of the current price to the price optInTimePeriod bars ago. A momentum measure centered at 1. Always positive, centered at 1: >1 rising, <1 falling.

## Formula

ROCR = price / price[t - optInTimePeriod]

## Inputs

- `inReal` — Price series

## Outputs

- `outReal` — Ratio of current price to prior price

## Parameters

- `optInTimePeriod` — Lookback distance in bars for the prior price

## Implementation

TA-Lib Definition: [`rocr.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/rocr/rocr.c) · [`rocr.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/rocr/rocr.yaml)

| Native | File |
|--------|------|
| C | [`ta_ROCR.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_ROCR.c) |
| Rust | [`rocr.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/src/ta_func/rocr.rs) |
| Java | [`Core_ROCR.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/Core_ROCR.java) |

TA-Lib is also available for Python, R and more using a [wrapper](https://ta-lib.org/wrappers/).

## Aliases

Rate of Change Ratio

## See Also

[ROC](/functions/roc.md) · [ROCP](/functions/rocp.md) · [ROCR100](/functions/rocr100.md) · [MOM](/functions/mom.md)
