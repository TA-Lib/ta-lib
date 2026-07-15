# MAXINDEX

## Summary

Returns the index of the highest input value within a rolling window of optInTimePeriod bars. Same as MAX but outputs the location instead of the value.

## Formula

outInteger[i] = argmax_{j in [i-optInTimePeriod+1, i]} inReal[j]

## Notes

- When several bars in a window share the highest value, which bar's index is returned is not guaranteed to be a specific one of the tied bars.

## Inputs

- `inReal` — Input series to scan

## Outputs

- `outInteger` — Absolute index (into inReal) of the highest value in each window

## Parameters

- `optInTimePeriod` — Window length over which the max is located

## Implementation

TA-Lib Definition: [`maxindex.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/maxindex/maxindex.c) · [`maxindex.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/maxindex/maxindex.yaml)

| Native | File |
|--------|------|
| C | [`ta_MAXINDEX.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_MAXINDEX.c) |
| Rust | [`maxindex.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/library/src/ta_func/maxindex.rs) |
| Java | [`Core_MAXINDEX.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/library/fragments/Core_MAXINDEX.java) |

TA-Lib is also available for Python, R and more using a [wrapper](https://ta-lib.org/wrappers/).

## Aliases

Index of Highest Value, Highest Value Index, argmax

## See Also

MAX · MININDEX · MIN · MINMAXINDEX
