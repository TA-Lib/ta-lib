# CDLRISEFALL3METHODS

## Summary

A five-candle continuation pattern: a long candle, three small counter-color candles that stay partly within the first candle's high-low range, then a long same-color candle that resumes the trend. Bullish (rising) or bearish (falling) continuation signal. A hit signals trend continuation: +100 = bullish (rising three methods), -100 = bearish (falling three methods).

## Notes

- Only the three-small-candle variant is detected; the classic pattern allowing two or more small candles is not supported.
- The middle candles need only partially overlap the first candle's range, not be fully contained within it.
- The prior trend the continuation reading assumes is not verified.

## Inputs

- `inPriceOHLC` — OHLC price series (open, high, low, close)

## Outputs

- `outInteger` — +100 when candle 1 is white (rising/bullish continuation), -100 when candle 1 is black (falling/bearish continuation), 0 otherwise. Sign = 100 * color of candle 1

## Implementation

TA-Lib Definition: [`cdlrisefall3methods.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/cdlrisefall3methods/cdlrisefall3methods.c) · [`cdlrisefall3methods.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/cdlrisefall3methods/cdlrisefall3methods.yaml)

| Native | File |
|--------|------|
| C | [`ta_CDLRISEFALL3METHODS.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_CDLRISEFALL3METHODS.c) |
| Rust | [`cdlrisefall3methods.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/src/ta_func/cdlrisefall3methods.rs) |
| Java | [`Core_CDLRISEFALL3METHODS.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/Core_CDLRISEFALL3METHODS.java) |

TA-Lib is also available for Python, R and more using a [wrapper](https://ta-lib.org/wrappers/).

## Aliases

Rising/Falling Three Methods, Rising Three Methods, Falling Three Methods

## See Also

CDLXSIDEGAP3METHODS · CDL3INSIDE · CDL3OUTSIDE
