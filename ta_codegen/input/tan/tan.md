# TAN

## Summary

Vector trigonometric tangent: applies tan() element-wise to each input value.

## Formula

outReal[i] = tan(inReal[i])

## Inputs

- `inReal` — input values

## Outputs

- `outReal` — tangent of each input

## Implementation

TA-Lib Definition: [`tan.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/tan/tan.c) · [`tan.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/tan/tan.yaml)

| Native | File |
|--------|------|
| C | [`ta_TAN.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_TAN.c) |
| Rust | [`tan.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/src/ta_func/tan.rs) |
| Java | [`Core_TAN.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/Core_TAN.java) |

TA-Lib is also available for Python, R and more using a [wrapper](https://ta-lib.org/wrappers/).

## Aliases

tangent

## See Also

ATAN · SIN · COS · TANH
