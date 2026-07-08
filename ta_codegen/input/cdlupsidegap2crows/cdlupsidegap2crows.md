# CDLUPSIDEGAP2CROWS

## Summary

A three-candle bearish reversal pattern: a long white candle, then a small black candle gapping up (a gap between the real bodies), then a black candle that engulfs the second candle's real body but still closes above the first candle's close. Signals a bearish reversal. A hit (-100) is a bearish reversal signal, most meaningful in an uptrend.

## Notes

- The pattern classically assumes a prior uptrend, but this function does not verify any trend.

## Inputs

- `inPriceOHLC` — Open, High, Low, Close price series

## Outputs

- `outInteger` — -100 on a pattern bar, 0 otherwise. Bearish-only: this pattern never emits +100

## Implementation

TA-Lib Definition: [`cdlupsidegap2crows.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/cdlupsidegap2crows/cdlupsidegap2crows.c) · [`cdlupsidegap2crows.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/cdlupsidegap2crows/cdlupsidegap2crows.yaml)

| Native | File |
|--------|------|
| C | [`ta_CDLUPSIDEGAP2CROWS.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_CDLUPSIDEGAP2CROWS.c) |
| Rust | [`cdlupsidegap2crows.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/src/ta_func/cdlupsidegap2crows.rs) |
| Java | [`Core_CDLUPSIDEGAP2CROWS.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/Core_CDLUPSIDEGAP2CROWS.java) |

TA-Lib is also available for Python, R and more using a [wrapper](https://ta-lib.org/wrappers/).

## Aliases

Upside Gap Two Crows

## See Also

CDL2CROWS · CDLGAPSIDESIDEWHITE
