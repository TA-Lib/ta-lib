# CDLTASUKIGAP

## Summary

A three-candle pattern: a real-body-gapping candle followed by an opposite-color candle that opens inside its body and closes back into the gap without filling it. An upside gap is a bullish continuation signal; a downside gap is a bearish continuation signal. Hit signals trend continuation: +100 bullish (in an uptrend), -100 bearish (in a downtrend).

## Notes

- This continuation pattern does not verify the prior trend it classically assumes; the caller must confirm the trend.

## Inputs

- `inPriceOHLC` — OHLC price series (open, high, low, close)

## Outputs

- `outInteger` — +100 on a bullish (upside-gap) tasuki gap, -100 on a bearish (downside-gap) tasuki gap, 0 otherwise. Sign equals the color of the gap candle i-1 (candlecolor(i-1)*100)

## Implementation

TA-Lib Definition: [`cdltasukigap.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/cdltasukigap/cdltasukigap.c) · [`cdltasukigap.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/cdltasukigap/cdltasukigap.yaml)

| Native | File |
|--------|------|
| C | [`ta_CDLTASUKIGAP.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_CDLTASUKIGAP.c) |
| Rust | [`cdltasukigap.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/library/src/ta_func/cdltasukigap.rs) |
| Java | [`Core_CDLTASUKIGAP.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/library/fragments/Core_CDLTASUKIGAP.java) |

TA-Lib is also available for Python, R and more using a [wrapper](https://ta-lib.org/wrappers/).

## Aliases

Tasuki Gap, Upside/Downside Tasuki Gap

## See Also

CDLGAPSIDESIDEWHITE · CDLXSIDEGAP3METHODS
