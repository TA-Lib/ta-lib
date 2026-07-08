---
title: MINMAXINDEX
description: "Returns the absolute input indices of the lowest and highest values within each rolling window of optInTimePeriod bars. Index variant of MINMAX."
---

# MINMAXINDEX

## Summary

Returns the absolute input indices of the lowest and highest values within each rolling window of optInTimePeriod bars. Index variant of MINMAX.

## Formula

For each t: outMaxIdx[t] = argmax_{i in [t-N+1, t]} inReal[i]; outMinIdx[t] = argmin over the same window (N = optInTimePeriod).

## Notes

- When several bars in a window share the extreme value, which bar's index is returned is not guaranteed to be a specific one of the tied bars.

## Inputs

- `inReal` — Input series scanned for extremes

## Outputs

- `outMinIdx` — Absolute index (into inReal) of the window minimum
- `outMaxIdx` — Absolute index (into inReal) of the window maximum

## Parameters

- `optInTimePeriod` — Window length in bars

## Implementation

TA-Lib Definition: [`minmaxindex.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/minmaxindex/minmaxindex.c) · [`minmaxindex.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/minmaxindex/minmaxindex.yaml)

| Native | File |
|--------|------|
| C | [`ta_MINMAXINDEX.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_MINMAXINDEX.c) |
| Rust | [`minmaxindex.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/src/ta_func/minmaxindex.rs) |
| Java | [`Core_MINMAXINDEX.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/Core_MINMAXINDEX.java) |

TA-Lib is also available for Python, R and more using a [wrapper](https://ta-lib.org/wrappers/).

## Aliases

Lowest/Highest Index

## See Also

[MINMAX](minmax.md) · [MIN](min.md) · [MAX](max.md) · [MININDEX](minindex.md) · [MAXINDEX](maxindex.md)
