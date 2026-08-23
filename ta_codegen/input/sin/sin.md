# SIN

## Summary

Element-wise sine of the input series.

## Formula

outReal[i] = sin(inReal[i])

## Inputs

- `inReal` — Input values (radians)

## Outputs

- `outReal` — Sine of each input

## Implementation

TA-Lib Definition: [`sin.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/sin/sin.c) · [`sin.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/sin/sin.yaml)

| Native | File |
|--------|------|
| C | [`ta_SIN.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_SIN.c) |
| Rust | [`sin.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/library/src/ta_func/sin.rs) |
| Java | [`Core_SIN.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/fragments/Core_SIN.java) |

TA-Lib is also available for Python, R and more using a [wrapper](/install/#wrappers).

## Aliases

sine

## See Also

COS · TAN · ASIN

## References

- Wikipedia, *Trigonometric functions*: [en.wikipedia.org/wiki/Trigonometric_functions](https://en.wikipedia.org/wiki/Trigonometric_functions)
