# MOM

## Summary

Momentum: current price minus the price optInTimePeriod bars ago. The absolute (unnormalized) rate of change. Positive = price rose over the period, negative = fell; centered at zero.

## Formula

MOM[i] = inReal[i] - inReal[i - optInTimePeriod]

## Inputs

- `inReal` — Input price series

## Outputs

- `outReal` — Momentum (current minus value optInTimePeriod bars ago)

## Parameters

- `optInTimePeriod` — Lookback distance in bars

## Implementation

TA-Lib Definition: [`mom.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/mom/mom.c) · [`mom.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/mom/mom.yaml)

| Native | File |
|--------|------|
| C | [`ta_MOM.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_MOM.c) |
| Rust | [`mom.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/src/ta_func/mom.rs) |
| Java | [`Core_MOM.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/Core_MOM.java) |

TA-Lib is also available for Python, R and more using a [wrapper](https://ta-lib.org/wrappers/).

## Aliases

Momentum

## See Also

ROC · ROCP · ROCR · ROCR100
