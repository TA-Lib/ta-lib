# CDLABANDONEDBABY

## Summary

A three-candle reversal pattern: a long body, then a gapped-away doji, then a body of opposite color that gaps back the other way and closes deep into the first body. Bullish (bottom) or bearish (top) reversal signal. Nonzero hit signals a reversal: +100 abandoned baby bottom (bullish), -100 abandoned baby top (bearish).

## Notes

- Does not verify the prior trend the pattern classically assumes for significance.

## Inputs

- `inPriceOHLC` — Open, High, Low, Close price series

## Outputs

- `outInteger` — +100 at a bullish abandoned baby bottom (3rd candle white), -100 at a bearish abandoned baby top (3rd candle black), 0 otherwise; sign = color of the 3rd candle

## Parameters

- `optInPenetration` — Fraction of the 1st candle's real body the 3rd close must penetrate; default 0.3, range [0, TA_REAL_MAX]

## Implementation

TA-Lib Definition: [`cdlabandonedbaby.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/cdlabandonedbaby/cdlabandonedbaby.c) · [`cdlabandonedbaby.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/cdlabandonedbaby/cdlabandonedbaby.yaml)

| Native | File |
|--------|------|
| C | [`ta_CDLABANDONEDBABY.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_CDLABANDONEDBABY.c) |
| Rust | [`cdlabandonedbaby.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/src/ta_func/cdlabandonedbaby.rs) |
| Java | [`Core_CDLABANDONEDBABY.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/Core_CDLABANDONEDBABY.java) |

TA-Lib is also available for Python, R and more using a [wrapper](https://ta-lib.org/wrappers/).

## Aliases

Abandoned Baby

## See Also

CDLEVENINGDOJISTAR · CDLMORNINGDOJISTAR · CDLEVENINGSTAR · CDLMORNINGSTAR
