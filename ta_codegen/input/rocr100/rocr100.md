# ROCR100

## Summary

Rate-of-change ratio scaled by 100: current price as a percentage of the price optInTimePeriod bars ago. Momentum measure centered at 100 and always positive. Above 100 = price rose vs n bars ago; below 100 = price fell.

## Formula

$ROCR100_t = \dfrac{price_t}{price_{t-n}} \times 100$, where $n$ = optInTimePeriod

## Inputs

- `inReal` — Input price/data series

## Outputs

- `outReal` — Rate-of-change ratio times 100

## Parameters

- `optInTimePeriod` — Lookback distance (bars back) for the reference price

## Implementation

TA-Lib Definition: [`rocr100.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/rocr100/rocr100.c) · [`rocr100.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/rocr100/rocr100.yaml)

| Native | File |
|--------|------|
| C | [`ta_ROCR100.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_ROCR100.c) |
| Rust | [`rocr100.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/library/src/ta_func/rocr100.rs) |
| Java | [`Core_ROCR100.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/library/fragments/Core_ROCR100.java) |

TA-Lib is also available for Python, R and more using a [wrapper](https://ta-lib.org/wrappers/).

## Aliases

Rate of Change Ratio 100 Scale, MO

## See Also

ROCR · ROC · ROCP · MOM
