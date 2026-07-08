# CDLONNECK

## Summary

A two-candle on-neck pattern: a long black candle followed by a white candle that opens below the prior candle's low and closes right at that low. Bearish continuation signal. A hit is bearish (bearish continuation); the code does not verify the assumed prior downtrend.

## Formula

Two candles. 1st: black (close<open) with long real body (realbody > BodyLong average). 2nd: white (close>=open); open < prior low; close within the Equal band of the prior low, i.e. (prior_low - EqualAvg) <= close2 <= (prior_low + EqualAvg).

## Notes

- The bearish-continuation reading assumes a prior downtrend, which is not verified.

## Inputs

- `inPriceOHLC` — Open/High/Low/Close price series

## Outputs

- `outInteger` — -100 on a match, 0 otherwise. Only -100 is ever emitted (never +100); on-neck is always bearish

## Implementation

TA-Lib Definition: [`cdlonneck.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/cdlonneck/cdlonneck.c) · [`cdlonneck.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/cdlonneck/cdlonneck.yaml)

| Native | File |
|--------|------|
| C | [`ta_CDLONNECK.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_CDLONNECK.c) |
| Rust | [`cdlonneck.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/src/ta_func/cdlonneck.rs) |
| Java | [`Core_CDLONNECK.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/Core_CDLONNECK.java) |

TA-Lib is also available for Python, R and more using a [wrapper](https://ta-lib.org/wrappers/).

## Aliases

On-Neck Pattern, On-Neck Line

## See Also

CDLINNECK · CDLTHRUSTING · CDLMEETINGLINES
