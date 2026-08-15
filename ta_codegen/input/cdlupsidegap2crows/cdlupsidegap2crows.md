# CDLUPSIDEGAP2CROWS

## Summary

A three-candle bearish reversal pattern: a long white candle, then a small black candle gapping up (a gap between the real bodies), then a black candle that engulfs the second candle's real body but still closes above the first candle's close. Signals a bearish reversal. A hit (-100) is a bearish reversal signal, most meaningful in an uptrend.

## Notes

- The pattern classically assumes a prior uptrend, but this function does not verify any trend.
- Although classically a bearish reversal, Bulkowski's testing found this actually acts as a bullish continuation 60% of the time, and even when it does work "the price move is often lousy." ([thepatternsite.com](https://www.thepatternsite.com/UpGapTwoCrows.html))

## Inputs

- `inOpen` — Open price of each bar
- `inHigh` — High price of each bar
- `inLow` — Low price of each bar
- `inClose` — Close price of each bar

## Outputs

- `outInteger` — -100 on a pattern bar, 0 otherwise. Bearish-only: this pattern never emits +100

## Output Values

| Value | Meaning |
|-------|---------|
| -100 | Upside Gap Two Crows pattern detected: bearish |
| 0 | No pattern |

## Implementation

TA-Lib Definition: [`cdlupsidegap2crows.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/cdlupsidegap2crows/cdlupsidegap2crows.c) · [`cdlupsidegap2crows.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/cdlupsidegap2crows/cdlupsidegap2crows.yaml)

| Native | File |
|--------|------|
| C | [`ta_CDLUPSIDEGAP2CROWS.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_CDLUPSIDEGAP2CROWS.c) |
| Rust | [`cdlupsidegap2crows.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/library/src/ta_func/cdlupsidegap2crows.rs) |
| Java | [`Core_CDLUPSIDEGAP2CROWS.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/fragments/Core_CDLUPSIDEGAP2CROWS.java) |

TA-Lib is also available for Python, R and more using a [wrapper](/install/#wrappers).

## Aliases

Upside Gap Two Crows

## See Also

CDL2CROWS · CDLGAPSIDESIDEWHITE
