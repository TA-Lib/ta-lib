# EXP

## Summary

Element-wise base-e exponential of the input series.

## Formula

outReal[i] = exp(inReal[i]) = e^{inReal[i]}

## Inputs

- `inReal` — Input values

## Outputs

- `outReal` — e raised to each input value

## Implementation

TA-Lib Definition: [`exp.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/exp/exp.c) · [`exp.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/exp/exp.yaml)

| Native | File |
|--------|------|
| C | [`ta_EXP.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_EXP.c) |
| Rust | [`exp.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/library/src/ta_func/exp.rs) |
| Java | [`Core_EXP.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/fragments/Core_EXP.java) |

TA-Lib is also available for Python, R and more using a [wrapper](/install/#wrappers).

## Aliases

exponential, e^x

## See Also

LN · SQRT

## References

- Wikipedia, *Exponential function*: [en.wikipedia.org/wiki/Exponential_function](https://en.wikipedia.org/wiki/Exponential_function)
