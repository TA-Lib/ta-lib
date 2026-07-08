---
title: MAX
description: "Highest input value over a rolling window of the last optInTimePeriod bars. A moving-window maximum."
---

# MAX

## Summary

Highest input value over a rolling window of the last optInTimePeriod bars. A moving-window maximum.

## Formula

outReal[i] = max(inReal[i-optInTimePeriod+1 .. i])

## Inputs

- `inReal` — Series to take the rolling maximum of

## Outputs

- `outReal` — Highest value within each trailing window

## Parameters

- `optInTimePeriod` — Window length in bars

## Implementation

TA-Lib Definition: [`max.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/max/max.c) · [`max.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/max/max.yaml)

| Native | File |
|--------|------|
| C | [`ta_MAX.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_MAX.c) |
| Rust | [`max.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/src/ta_func/max.rs) |
| Java | [`Core_MAX.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/Core_MAX.java) |

TA-Lib is also available for Python, R and more using a [wrapper](https://ta-lib.org/wrappers/).

## Aliases

Highest, Highest High, Rolling Maximum

## See Also

[MIN](min.md) · [MAXINDEX](maxindex.md) · [MINMAX](minmax.md)
