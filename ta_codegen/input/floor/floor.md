# FLOOR

## Summary

Element-wise floor (round down to the nearest integer) of the input series.

## Formula

outReal[i] = floor(inReal[i])

## Inputs

- `inReal` — Input values

## Outputs

- `outReal` — Each input rounded down to nearest integer

## Implementation

TA-Lib Definition: [`floor.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/floor/floor.c) · [`floor.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/floor/floor.yaml)

| Native | File |
|--------|------|
| C | [`ta_FLOOR.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_FLOOR.c) |
| Rust | [`floor.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/library/src/ta_func/floor.rs) |
| Java | [`Core_FLOOR.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/fragments/Core_FLOOR.java) |

TA-Lib is also available for Python, R and more using a [wrapper](/install/#wrappers).

## See Also

CEIL

## References

- Wikipedia, *Floor and ceiling functions*: [en.wikipedia.org/wiki/Floor_and_ceiling_functions](https://en.wikipedia.org/wiki/Floor_and_ceiling_functions)
