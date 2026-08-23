# ACOS

## Summary

Element-wise arc cosine of the input series.

## Formula

outReal[i] = acos(inReal[i])

## Notes

- Outside [-1, 1] there is no angle whose cosine is that value, so those elements come out NaN.

## Inputs

- `inReal` — input values (expected in [-1, 1])

## Outputs

- `outReal` — arc cosine of each input, in radians

## Implementation

TA-Lib Definition: [`acos.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/acos/acos.c) · [`acos.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/acos/acos.yaml)

| Native | File |
|--------|------|
| C | [`ta_ACOS.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_ACOS.c) |
| Rust | [`acos.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/library/src/ta_func/acos.rs) |
| Java | [`Core_ACOS.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/fragments/Core_ACOS.java) |

TA-Lib is also available for Python, R and more using a [wrapper](/install/#wrappers).

## Aliases

Arc Cosine, Inverse Cosine, arccos

## See Also

COS · ASIN · ATAN

## References

- Wikipedia, *Inverse trigonometric functions*: [en.wikipedia.org/wiki/Inverse_trigonometric_functions](https://en.wikipedia.org/wiki/Inverse_trigonometric_functions)
