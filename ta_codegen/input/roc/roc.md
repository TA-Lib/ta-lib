# ROC

## Summary

Rate-of-change momentum oscillator: the percent change of price versus the price optInTimePeriod bars earlier. Centered at zero with positive and negative values. Positive when price rose over the period, negative when it fell; magnitude scales the move.

## Formula

ROC = ((price / prevPrice) - 1) * 100, where prevPrice = inReal[i - optInTimePeriod]

## Inputs

- `inReal` — Input price series

## Outputs

- `outReal` — Percent rate of change

## Parameters

- `optInTimePeriod` — Lookback distance to the prior price

## Implementation

TA-Lib Definition: [`roc.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/roc/roc.c) · [`roc.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/roc/roc.yaml)

| Native | File |
|--------|------|
| C | [`ta_ROC.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_ROC.c) |
| Rust | [`roc.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/library/src/ta_func/roc.rs) |
| Java | [`Core_ROC.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/library/fragments/Core_ROC.java) |

TA-Lib is also available for Python, R and more using a [wrapper](https://ta-lib.org/wrappers/).

## Aliases

Rate of Change, Price Rate of Change

## See Also

MOM · ROCP · ROCR · ROCR100
