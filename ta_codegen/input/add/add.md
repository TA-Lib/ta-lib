# ADD

## Summary

Vector arithmetic addition. Outputs the element-wise sum of two input series.

## Formula

outReal[i] = inReal0[i] + inReal1[i]

## Inputs

- `inReal0` — First operand series
- `inReal1` — Second operand series

## Outputs

- `outReal` — Element-wise sum of the two inputs

## Implementation

TA-Lib Definition: [`add.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/add/add.c) · [`add.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/add/add.yaml)

| Native | File |
|--------|------|
| C | [`ta_ADD.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_ADD.c) |
| Rust | [`add.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/library/src/ta_func/add.rs) |
| Java | [`Core_ADD.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/library/fragments/Core_ADD.java) |

TA-Lib is also available for Python, R and more using a [wrapper](https://ta-lib.org/wrappers/).

## Aliases

Vector Add, Vector Arithmetic Add

## See Also

SUB · MULT · DIV
