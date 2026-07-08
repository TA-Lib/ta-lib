# TYPPRICE

## Summary

Typical Price: the average of the high, low, and close of each bar. A single representative price per period.

## Formula

out[i] = (High[i] + Low[i] + Close[i]) / 3

## Inputs

- `inPriceHLC` — high, low, close of each bar

## Outputs

- `outReal` — typical price per bar

## Implementation

TA-Lib Definition: [`typprice.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/typprice/typprice.c) · [`typprice.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/typprice/typprice.yaml)

| Native | File |
|--------|------|
| C | [`ta_TYPPRICE.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_TYPPRICE.c) |
| Rust | [`typprice.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/src/ta_func/typprice.rs) |
| Java | [`Core_TYPPRICE.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/Core_TYPPRICE.java) |

TA-Lib is also available for Python, R and more using a [wrapper](https://ta-lib.org/wrappers/).

## Aliases

Typical Price

## See Also

MEDPRICE · WCLPRICE · AVGPRICE
