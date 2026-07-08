---
title: ASIN
description: "Element-wise arcsine (inverse sine) of each input value. A vector math transform, not a market indicator."
---

# ASIN

## Summary

Element-wise arcsine (inverse sine) of each input value. A vector math transform, not a market indicator.

## Formula

outReal[i] = asin(inReal[i])

## Inputs

- `inReal` — Input values (domain [-1,1] for a real result)

## Outputs

- `outReal` — Arcsine of each input, in radians

## Implementation

TA-Lib Definition: [`asin.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/asin/asin.c) · [`asin.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/asin/asin.yaml)

| Native | File |
|--------|------|
| C | [`ta_ASIN.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_ASIN.c) |
| Rust | [`asin.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/src/ta_func/asin.rs) |
| Java | [`Core_ASIN.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/Core_ASIN.java) |

TA-Lib is also available for Python, R and more using a [wrapper](https://ta-lib.org/wrappers/).

## Aliases

arcsine, inverse sine

## See Also

[ACOS](acos.md) · [ATAN](atan.md) · [SIN](sin.md) · [COS](cos.md)
