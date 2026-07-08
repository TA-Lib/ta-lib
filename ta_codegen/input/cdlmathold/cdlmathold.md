# CDLMATHOLD

## Summary

A five-candle bullish continuation pattern: a long white candle, an upside real-body-gapped small black candle, two more small falling candles that hold within the first body, and a final white candle closing above the reaction days' highs. Signals continuation of the prior uptrend. Hit = bullish continuation of the existing uptrend.

## Notes

- The colors of the third and fourth (reaction) candles are not checked, although they are classically black.
- The continuation reading assumes a prior uptrend, which is not verified.

## Inputs

- `inPriceOHLC` — OHLC price series (open, high, low, close)

## Outputs

- `outInteger` — +100 when the bullish Mat Hold is detected, 0 otherwise. Never emits -100

## Parameters

- `optInPenetration` — Max fraction of the 1st white body the reaction days (3rd, 4th) may penetrate; default 0.5

## Implementation

TA-Lib Definition: [`cdlmathold.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/cdlmathold/cdlmathold.c) · [`cdlmathold.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/cdlmathold/cdlmathold.yaml)

| Native | File |
|--------|------|
| C | [`ta_CDLMATHOLD.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_CDLMATHOLD.c) |
| Rust | [`cdlmathold.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/src/ta_func/cdlmathold.rs) |
| Java | [`Core_CDLMATHOLD.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/Core_CDLMATHOLD.java) |

TA-Lib is also available for Python, R and more using a [wrapper](https://ta-lib.org/wrappers/).

## Aliases

Mat Hold

## See Also

CDLRISEFALL3METHODS · CDLXSIDEGAP3METHODS
