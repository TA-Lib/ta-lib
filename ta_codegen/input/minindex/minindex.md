# MININDEX

## Summary

Returns the absolute index of the lowest value within a rolling window of the given period. Same scan as MIN but outputs the position of the minimum rather than its value.

## Formula

outInteger[i] = index of min(inReal[i-optInTimePeriod+1 .. i])

## Notes

- When several bars in a window share the lowest value, the index of one of them is returned — not necessarily the first or the last.

## Inputs

- `inReal` — Series to scan for its minimum

## Outputs

- `outInteger` — Absolute index in inReal of the lowest value in each window

## Parameters

- `optInTimePeriod` — Window length over which the minimum is located

## Implementation

TA-Lib Definition: [`minindex.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/minindex/minindex.c) · [`minindex.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/minindex/minindex.yaml)

| Native | File |
|--------|------|
| C | [`ta_MININDEX.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_MININDEX.c) |
| Rust | [`minindex.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/library/src/ta_func/minindex.rs) |
| Java | [`Core_MININDEX.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/fragments/Core_MININDEX.java) |

TA-Lib is also available for Python, R and more using a [wrapper](/install/#wrappers).

## Aliases

Index of Lowest Value, Lowest Value Index, Rolling Argmin

## See Also

MIN · MAXINDEX · MINMAXINDEX · MINMAX
