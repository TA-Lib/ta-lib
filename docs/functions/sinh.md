---
title: SINH
description: "Element-wise hyperbolic sine of the input series. A vector math transform applying sinh() to each value."
---

# SINH

## Summary

Element-wise hyperbolic sine of the input series. A vector math transform applying sinh() to each value.

## Formula

outReal[i] = sinh(inReal[i])

## Inputs

- `inReal` — Input series

## Outputs

- `outReal` — Hyperbolic sine of each input value

## Implementation

TA-Lib Definition: [`sinh.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/sinh/sinh.c) · [`sinh.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/sinh/sinh.yaml)

| Native | File |
|--------|------|
| C | [`ta_SINH.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_SINH.c) |
| Rust | [`sinh.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/src/ta_func/sinh.rs) |
| Java | [`Core_SINH.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/Core_SINH.java) |

TA-Lib is also available for Python, R and more using a [wrapper](https://ta-lib.org/wrappers/).

## Aliases

Hyperbolic Sine

## See Also

[COSH](/functions/cosh.md) · [TANH](/functions/tanh.md)
