---
title: SUM
description: "Rolling sum of the input over a fixed period. Each output is the sum of the most recent optInTimePeriod input values."
---

# SUM

## Summary

Rolling sum of the input over a fixed period. Each output is the sum of the most recent optInTimePeriod input values.

## Formula

$out_i = \sum_{j=i-(N-1)}^{i} inReal_j$, N = optInTimePeriod

## Inputs

- `inReal` — Values to sum

## Outputs

- `outReal` — Windowed sum over the period

## Parameters

- `optInTimePeriod` — Window length summed

## Implementation

TA-Lib Definition: [`sum.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/sum/sum.c) · [`sum.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/sum/sum.yaml)

| Native | File |
|--------|------|
| C | [`ta_SUM.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_SUM.c) |
| Rust | [`sum.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/src/ta_func/sum.rs) |
| Java | [`Core_SUM.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/Core_SUM.java) |

TA-Lib is also available for Python, R and more using a [wrapper](https://ta-lib.org/wrappers/).

## Aliases

Summation, Rolling Sum, Moving Sum

## See Also

[SMA](/functions/sma.md)
