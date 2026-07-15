---
title: ATAN
description: "Vector trigonometric arc tangent: applies atan element-wise to each input. Pure math transform with no lookback."
---

# ATAN

## Summary

Vector trigonometric arc tangent: applies atan element-wise to each input. Pure math transform with no lookback.

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
| Java | [`Core_ATAN.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/library/fragments/Core_ATAN.java) |

TA-Lib is also available for Python, R and more using a [wrapper](https://ta-lib.org/wrappers/).

## Aliases

arctangent, arctan, inverse tangent

## See Also

[TAN](/functions/tan) · [ACOS](/functions/acos) · [ASIN](/functions/asin)
