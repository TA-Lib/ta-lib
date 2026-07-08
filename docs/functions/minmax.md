---
title: MINMAX
description: "Returns both the lowest and highest values of the input over a rolling window of the last optInTimePeriod bars. An overlap-study companion to MIN and MAX that computes both extrema in one pass."
---

# MINMAX

## Summary

Returns both the lowest and highest values of the input over a rolling window of the last optInTimePeriod bars. An overlap-study companion to MIN and MAX that computes both extrema in one pass.

## Inputs

- `inReal` — Values scanned for the window min and max

## Outputs

- `outMin` — Lowest value in each rolling window
- `outMax` — Highest value in each rolling window

## Parameters

- `optInTimePeriod` — Rolling window length

## Implementation

TA-Lib Definition: [`minmax.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/minmax/minmax.c) · [`minmax.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/minmax/minmax.yaml)

| Native | File |
|--------|------|
| C | [`ta_MINMAX.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_MINMAX.c) |
| Rust | [`minmax.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/src/ta_func/minmax.rs) |
| Java | [`Core_MINMAX.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/Core_MINMAX.java) |

TA-Lib is also available for Python, R and more using a [wrapper](https://ta-lib.org/wrappers/).

## Aliases

Highest Lowest

## See Also

[MIN](min.md) · [MAX](max.md) · [MINMAXINDEX](minmaxindex.md) · [MININDEX](minindex.md) · [MAXINDEX](maxindex.md)
