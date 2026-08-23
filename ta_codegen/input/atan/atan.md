# ATAN

## Summary

Element-wise arctangent of the input series.

## Formula

outReal[i] = atan(inReal[i])  (radians, range (-pi/2, pi/2))

## Inputs

- `inReal` — Input values

## Outputs

- `outReal` — Arc tangent of each input, in radians

## Implementation

TA-Lib Definition: [`atan.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/atan/atan.c) · [`atan.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/atan/atan.yaml)

| Native | File |
|--------|------|
| C | [`ta_ATAN.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_ATAN.c) |
| Rust | [`atan.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/library/src/ta_func/atan.rs) |
| Java | [`Core_ATAN.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/fragments/Core_ATAN.java) |

TA-Lib is also available for Python, R and more using a [wrapper](/install/#wrappers).

## Aliases

arctangent, arctan, inverse tangent

## See Also

TAN · ACOS · ASIN

## References

- Wikipedia, *Inverse trigonometric functions*: [en.wikipedia.org/wiki/Inverse_trigonometric_functions](https://en.wikipedia.org/wiki/Inverse_trigonometric_functions)
