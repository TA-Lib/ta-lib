# TAN

## Summary

Element-wise tangent of the input series.

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
| Rust | [`tan.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/library/src/ta_func/tan.rs) |
| Java | [`Core_TAN.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/fragments/Core_TAN.java) |

TA-Lib is also available for Python, R and more using a [wrapper](/install/#wrappers).

## Aliases

tangent

## See Also

ATAN · SIN · COS · TANH

## References

- Wikipedia, *Trigonometric functions*: [en.wikipedia.org/wiki/Trigonometric_functions](https://en.wikipedia.org/wiki/Trigonometric_functions)
