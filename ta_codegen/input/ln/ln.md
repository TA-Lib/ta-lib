# LN

## Summary

Element-wise natural logarithm of the input series.

## Formula

outReal[i] = log(inReal[i])

## Notes

- The logarithm is defined only for positive values: a negative input gives NaN, and a zero input gives negative infinity.

## Inputs

- `inReal` — Input value series

## Outputs

- `outReal` — Natural log of each input value

## Implementation

TA-Lib Definition: [`ln.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/ln/ln.c) · [`ln.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/ln/ln.yaml)

| Native | File |
|--------|------|
| C | [`ta_LN.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_LN.c) |
| Rust | [`ln.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/library/src/ta_func/ln.rs) |
| Java | [`Core_LN.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/fragments/Core_LN.java) |

TA-Lib is also available for Python, R and more using a [wrapper](/install/#wrappers).

## Aliases

Natural Log, Vector Log Natural, Log

## See Also

LOG10 · EXP · SQRT

## References

- Wikipedia, *Natural logarithm*: [en.wikipedia.org/wiki/Natural_logarithm](https://en.wikipedia.org/wiki/Natural_logarithm)
