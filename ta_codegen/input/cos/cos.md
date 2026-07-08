# COS

## Summary

Element-wise trigonometric cosine of the input series. Applies the C library cos() to each sample.

## Formula

outReal[i] = cos(inReal[i])

## Inputs

- `inReal` — Input values, treated as angles in radians

## Outputs

- `outReal` — Cosine of each input value

## Implementation

TA-Lib Definition: [`cos.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/cos/cos.c) · [`cos.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/cos/cos.yaml)

| Native | File |
|--------|------|
| C | [`ta_COS.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_COS.c) |
| Rust | [`cos.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/src/ta_func/cos.rs) |
| Java | [`Core_COS.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/Core_COS.java) |

TA-Lib is also available for Python, R and more using a [wrapper](https://ta-lib.org/wrappers/).

## Aliases

Cosine, Vector Trigonometric Cos

## See Also

ACOS · SIN · TAN · COSH
