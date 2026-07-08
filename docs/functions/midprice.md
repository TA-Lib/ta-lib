---
title: MIDPRICE
description: "Midpoint of the price range over a rolling window: the average of the highest high and lowest low across the last optInTimePeriod bars. An overlap-study line plotted on price."
---

# MIDPRICE

## Summary

Midpoint of the price range over a rolling window: the average of the highest high and lowest low across the last optInTimePeriod bars. An overlap-study line plotted on price.

## Formula

MIDPRICE = (Highest(High, N) + Lowest(Low, N)) / 2, over the N=optInTimePeriod bars ending at each index

## Inputs

- `inPriceHL` — High and Low price series

## Outputs

- `outReal` — Midpoint of the period's high/low extremes

## Parameters

- `optInTimePeriod` — Window length over which the high/low extremes are taken

## Implementation

TA-Lib Definition: [`midprice.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/midprice/midprice.c) · [`midprice.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/midprice/midprice.yaml)

| Native | File |
|--------|------|
| C | [`ta_MIDPRICE.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_MIDPRICE.c) |
| Rust | [`midprice.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/src/ta_func/midprice.rs) |
| Java | [`Core_MIDPRICE.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/Core_MIDPRICE.java) |

TA-Lib is also available for Python, R and more using a [wrapper](https://ta-lib.org/wrappers/).

## Aliases

Midpoint Price

## See Also

[MIDPOINT](midpoint.md) · [MEDPRICE](medprice.md)
