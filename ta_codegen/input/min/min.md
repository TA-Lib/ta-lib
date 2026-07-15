# MIN

## Summary

Rolling minimum: the lowest input value over the trailing period.

## Formula

outReal[i] = min(inReal[i-optInTimePeriod+1 .. i])

## Inputs

- `inReal` — Source series to take the minimum of

## Outputs

- `outReal` — Lowest input value over the trailing window

## Parameters

- `optInTimePeriod` — Number of bars in the trailing window

## Implementation

TA-Lib Definition: [`min.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/min/min.c) · [`min.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/min/min.yaml)

| Native | File |
|--------|------|
| C | [`ta_MIN.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_MIN.c) |
| Rust | [`min.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/library/src/ta_func/min.rs) |
| Java | [`Core_MIN.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/library/fragments/Core_MIN.java) |

TA-Lib is also available for Python, R and more using a [wrapper](https://ta-lib.org/wrappers/).

## Aliases

Lowest, Rolling Min, Min Value

## See Also

MAX · MININDEX · MINMAX
