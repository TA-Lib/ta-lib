# SQRT

## Summary

Element-wise square root of the input series.

## Formula

outReal[i] = sqrt(inReal[i])

## Notes

- A negative input has no real square root, so those elements come out NaN.

## Inputs

- `inReal` — Input values

## Outputs

- `outReal` — Square root of each input value

## Implementation

TA-Lib Definition: [`sqrt.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/sqrt/sqrt.c) · [`sqrt.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/sqrt/sqrt.yaml)

| Native | File |
|--------|------|
| C | [`ta_SQRT.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_SQRT.c) |
| Rust | [`sqrt.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/library/src/ta_func/sqrt.rs) |
| Java | [`Core_SQRT.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/fragments/Core_SQRT.java) |

TA-Lib is also available for Python, R and more using a [wrapper](/install/#wrappers).

## Aliases

Square Root

## References

- Wikipedia, *Square root*: [en.wikipedia.org/wiki/Square_root](https://en.wikipedia.org/wiki/Square_root)
