---
title: TANH
description: "Vector hyperbolic tangent: applies tanh element-wise to the input series."
---

# TANH

## Summary

Vector hyperbolic tangent: applies tanh element-wise to the input series.

## Formula

outReal[i] = tanh(inReal[i])

## Inputs

- `inReal` — Input value series

## Outputs

- `outReal` — Hyperbolic tangent of each input

## Implementation

TA-Lib Definition: [`tanh.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/tanh/tanh.c) · [`tanh.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/tanh/tanh.yaml)

| Native | File |
|--------|------|
| C | [`ta_TANH.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_TANH.c) |
| Rust | [`tanh.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/library/src/ta_func/tanh.rs) |
| Java | [`Core_TANH.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/library/fragments/Core_TANH.java) |

TA-Lib is also available for Python, R and more using a [wrapper](https://ta-lib.org/wrappers/).

## Aliases

Hyperbolic Tangent

## See Also

[SINH](/functions/sinh) · [COSH](/functions/cosh) · [TAN](/functions/tan)
