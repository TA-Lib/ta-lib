---
title: LN
description: "Vector natural logarithm: applies the natural log (base e) elementwise to the input series."
---

# LN

## Summary

Vector natural logarithm: applies the natural log (base e) elementwise to the input series.

## Formula

outReal[i] = log(inReal[i])

## Inputs

- `inReal` — Input value series

## Outputs

- `outReal` — Natural log of each input value

## Implementation

TA-Lib Definition: [`ln.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/ln/ln.c) · [`ln.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/ln/ln.yaml)

| Native | File |
|--------|------|
| C | [`ta_LN.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_LN.c) |
| Rust | [`ln.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/src/ta_func/ln.rs) |
| Java | [`Core_LN.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/Core_LN.java) |

TA-Lib is also available for Python, R and more using a [wrapper](https://ta-lib.org/wrappers/).

## Aliases

Natural Log, Vector Log Natural, Log

## See Also

[LOG10](log10.md) · [EXP](exp.md) · [SQRT](sqrt.md)
