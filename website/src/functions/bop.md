---
title: BOP
description: "Balance Of Power compares where the close sits relative to the open, normalized by the bar's high-low range. A per-bar oscillator with no smoothing. Positive: close above open (buyers dominated); negative: sellers dominated."
---

# BOP

## Summary

Balance Of Power compares where the close sits relative to the open, normalized by the bar's high-low range. A per-bar oscillator with no smoothing. Positive: close above open (buyers dominated); negative: sellers dominated.

## Formula

BOP = (Close - Open) / (High - Low)

## Inputs

- `inPriceOHLC` — OHLC price bars (open, high, low, close)

## Outputs

- `outReal` — Balance of Power value per bar

## Implementation

TA-Lib Definition: [`bop.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/bop/bop.c) · [`bop.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/bop/bop.yaml)

| Native | File |
|--------|------|
| C | [`ta_BOP.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_BOP.c) |
| Rust | [`bop.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/library/src/ta_func/bop.rs) |
| Java | [`Core_BOP.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/library/fragments/Core_BOP.java) |

TA-Lib is also available for Python, R and more using a [wrapper](https://ta-lib.org/wrappers/).

## Aliases

Balance Of Power, Balance of Power
