---
title: ROCP
description: "Rate of change expressed as a fraction of the price optInTimePeriod bars ago. Normalized and centered at zero (positive or negative). >0 rising vs N bars ago, <0 falling; equals ROC/100."
---

# ROCP

## Summary

Rate of change expressed as a fraction of the price optInTimePeriod bars ago. Normalized and centered at zero (positive or negative). >0 rising vs N bars ago, <0 falling; equals ROC/100.

## Formula

ROCP = (price - prevPrice) / prevPrice, prevPrice = inReal[i - optInTimePeriod]

## Inputs

- `inReal` — Source price series

## Outputs

- `outReal` — Fractional rate of change vs the value optInTimePeriod bars earlier

## Parameters

- `optInTimePeriod` — Lookback distance to the previous price

## Implementation

TA-Lib Definition: [`rocp.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/rocp/rocp.c) · [`rocp.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/rocp/rocp.yaml)

| Native | File |
|--------|------|
| C | [`ta_ROCP.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_ROCP.c) |
| Rust | [`rocp.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/src/ta_func/rocp.rs) |
| Java | [`Core_ROCP.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/Core_ROCP.java) |

TA-Lib is also available for Python, R and more using a [wrapper](https://ta-lib.org/wrappers/).

## Aliases

Rate of Change Percentage, Percent Change

## See Also

[ROC](/functions/roc.md) · [ROCR](/functions/rocr.md) · [ROCR100](/functions/rocr100.md) · [MOM](/functions/mom.md)
