---
title: CEIL
description: "Vector ceiling: element-wise ceiling of each input value (smallest integer >= input)."
---

# CEIL

## Summary

Vector ceiling: element-wise ceiling of each input value (smallest integer >= input).

## Formula

outReal[i] = ceil(inReal[i])

## Inputs

- `inReal` — Input values

## Outputs

- `outReal` — Each input rounded up to nearest integer

## Implementation

TA-Lib Definition: [`ceil.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/ceil/ceil.c) · [`ceil.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/ceil/ceil.yaml)

| Native | File |
|--------|------|
| C | [`ta_CEIL.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_CEIL.c) |
| Rust | [`ceil.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/src/ta_func/ceil.rs) |
| Java | [`Core_CEIL.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/Core_CEIL.java) |

TA-Lib is also available for Python, R and more using a [wrapper](https://ta-lib.org/wrappers/).

## Aliases

Vector Ceil, Ceiling

## See Also

[FLOOR](/functions/floor.md)
