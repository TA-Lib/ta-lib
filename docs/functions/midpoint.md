---
title: MIDPOINT
description: "Midpoint over a period: the average of the highest and lowest input values within the lookback window. A single-series overlap smoother (use MIDPRICE for separate high/low price bars)."
---

# MIDPOINT

## Summary

Midpoint over a period: the average of the highest and lowest input values within the lookback window. A single-series overlap smoother (use MIDPRICE for separate high/low price bars).

## Formula

MIDPOINT = (Highest(inReal, period) + Lowest(inReal, period)) / 2

## Inputs

- `inReal` — Series to compute the midpoint over

## Outputs

- `outReal` — Midpoint of the period's high/low range

## Parameters

- `optInTimePeriod` — Lookback window length

## Implementation

TA-Lib Definition: [`midpoint.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/midpoint/midpoint.c) · [`midpoint.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/midpoint/midpoint.yaml)

| Native | File |
|--------|------|
| C | [`ta_MIDPOINT.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_MIDPOINT.c) |
| Rust | [`midpoint.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/src/ta_func/midpoint.rs) |
| Java | [`Core_MIDPOINT.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/Core_MIDPOINT.java) |

TA-Lib is also available for Python, R and more using a [wrapper](https://ta-lib.org/wrappers/).

## See Also

[MIDPRICE](/functions/midprice) · [MAX](/functions/max) · [MIN](/functions/min)
