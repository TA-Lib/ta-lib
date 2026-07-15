# COSH

## Summary

Vector hyperbolic cosine: applies cosh element-wise to each input value. A Math Transform primitive with no lookback.

## Formula

outReal[i] = cosh(inReal[i]) = (e^{inReal[i]} + e^{-inReal[i]}) / 2

## Inputs

- `inReal` — Input values to transform

## Outputs

- `outReal` — Hyperbolic cosine of each input

## Implementation

TA-Lib Definition: [`cosh.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/cosh/cosh.c) · [`cosh.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/cosh/cosh.yaml)

| Native | File |
|--------|------|
| C | [`ta_COSH.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_COSH.c) |
| Rust | [`cosh.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/library/src/ta_func/cosh.rs) |
| Java | [`Core_COSH.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/library/fragments/Core_COSH.java) |

TA-Lib is also available for Python, R and more using a [wrapper](https://ta-lib.org/wrappers/).

## Aliases

Hyperbolic Cosine

## See Also

SINH · TANH · COS
