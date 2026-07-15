---
title: AVGPRICE
description: "Average Price: the arithmetic mean of each bar's open, high, low, and close. A price-transform overlap condensing OHLC into a single representative price."
---

# AVGPRICE

## Summary

Average Price: the arithmetic mean of each bar's open, high, low, and close. A price-transform overlap condensing OHLC into a single representative price.

## Formula

outReal[i] = (High[i] + Low[i] + Close[i] + Open[i]) / 4

## Inputs

- `inPriceOHLC` — Open/High/Low/Close price series

## Outputs

- `outReal` — Per-bar average of the four OHLC prices

## Implementation

TA-Lib Definition: [`avgprice.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/avgprice/avgprice.c) · [`avgprice.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/avgprice/avgprice.yaml)

| Native | File |
|--------|------|
| C | [`ta_AVGPRICE.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_AVGPRICE.c) |
| Rust | [`avgprice.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/library/src/ta_func/avgprice.rs) |
| Java | [`Core_AVGPRICE.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/library/fragments/Core_AVGPRICE.java) |

TA-Lib is also available for Python, R and more using a [wrapper](https://ta-lib.org/wrappers/).

## Aliases

Average Price

## See Also

[MEDPRICE](/functions/medprice) · [TYPPRICE](/functions/typprice) · [WCLPRICE](/functions/wclprice)
