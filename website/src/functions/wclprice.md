---
title: WCLPRICE
description: "Weighted Close Price: a per-bar price average giving the close double weight relative to high and low."
---

# WCLPRICE

## Summary

Weighted Close Price: a per-bar price average giving the close double weight relative to high and low.

## Formula

$\text{WCLPRICE} = \dfrac{\text{High} + \text{Low} + 2\cdot\text{Close}}{4}$

## Inputs

- `inPriceHLC` — High, low, and close price series

## Outputs

- `outReal` — Weighted close price per bar

## Implementation

TA-Lib Definition: [`wclprice.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/wclprice/wclprice.c) · [`wclprice.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/wclprice/wclprice.yaml)

| Native | File |
|--------|------|
| C | [`ta_WCLPRICE.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_WCLPRICE.c) |
| Rust | [`wclprice.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/library/src/ta_func/wclprice.rs) |
| Java | [`Core_WCLPRICE.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/library/fragments/Core_WCLPRICE.java) |

TA-Lib is also available for Python, R and more using a [wrapper](https://ta-lib.org/wrappers/).

## Aliases

Weighted Close Price, Weighted Close

## See Also

[TYPPRICE](/functions/typprice) · [MEDPRICE](/functions/medprice) · [AVGPRICE](/functions/avgprice)
