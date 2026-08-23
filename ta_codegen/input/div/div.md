# DIV

## Summary

Element-wise division of two input series.

## Formula

outReal[i] = inReal0[i] / inReal1[i]

## Notes

- Zero divided by zero gives NaN; anything else divided by zero gives positive or negative infinity. Neither is reported as an error.

## Inputs

- `inReal0` — Dividend (numerator) series
- `inReal1` — Divisor (denominator) series

## Outputs

- `outReal` — Per-element quotient inReal0/inReal1

## Implementation

TA-Lib Definition: [`div.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/div/div.c) · [`div.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/div/div.yaml)

| Native | File |
|--------|------|
| C | [`ta_DIV.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_DIV.c) |
| Rust | [`div.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/library/src/ta_func/div.rs) |
| Java | [`Core_DIV.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/fragments/Core_DIV.java) |

TA-Lib is also available for Python, R and more using a [wrapper](/install/#wrappers).

## Aliases

Vector Arithmetic Divide, Divide

## See Also

MULT · ADD · SUB
