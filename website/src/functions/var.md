---
title: VAR
description: "Rolling population variance of a real series over a given period. Measures dispersion of values around their mean. Higher values indicate greater dispersion; 0 means constant input."
---

# VAR

## Summary

Rolling population variance of a real series over a given period. Measures dispersion of values around their mean. Higher values indicate greater dispersion; 0 means constant input.

## Formula

$\mathrm{VAR} = \frac{1}{n}\sum x_i^2 - \left(\frac{1}{n}\sum x_i\right)^2$, over the last $n$ = optInTimePeriod values (population, divides by $n$).

## Notes

- Computes population variance (divides by the period), not the sample variance (n-1) used by some definitions.
- The deviation-count parameter is accepted but has no effect on the result.

## Inputs

- `inReal` — Source series

## Outputs

- `outReal` — Rolling population variance

## Parameters

- `optInTimePeriod` — Window length for the variance
- `optInNbDev` — Deviation count accepted by the API but never used in the computation

## Implementation

TA-Lib Definition: [`var.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/var/var.c) · [`var.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/var/var.yaml)

| Native | File |
|--------|------|
| C | [`ta_VAR.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_VAR.c) |
| Rust | [`var.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/library/src/ta_func/var.rs) |
| Java | [`Core_VAR.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/library/fragments/Core_VAR.java) |

TA-Lib is also available for Python, R and more using a [wrapper](https://ta-lib.org/wrappers/).

## Aliases

Variance

## See Also

[STDDEV](/functions/stddev)
