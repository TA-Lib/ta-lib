# CDLCLOSINGMARUBOZU

## Summary

Single-candle pattern: a long real body whose closing end has no or very short shadow, so the close sits at the candle's extreme. Non-directional strong bar that emits +100 for a white body and -100 for a black body. White (+100) is bullish, black (-100) is bearish; a strong directional bar, not a defined reversal/continuation signal.

## Formula

One candle. Requires: (1) long real body: real body > the BodyLong average; AND (2) very short shadow at the closing end: if white (close>=open) upper shadow < the ShadowVeryShort average [close at/near high]; if black (close<open) lower shadow < the ShadowVeryShort average [close at/near low].

## Inputs

- `inPriceOHLC` — Open, High, Low, Close price series

## Outputs

- `outInteger` — +100 for a white (bullish) closing marubozu, -100 for a black (bearish) one, 0 otherwise

## Implementation

TA-Lib Definition: [`cdlclosingmarubozu.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/cdlclosingmarubozu/cdlclosingmarubozu.c) · [`cdlclosingmarubozu.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/cdlclosingmarubozu/cdlclosingmarubozu.yaml)

| Native | File |
|--------|------|
| C | [`ta_CDLCLOSINGMARUBOZU.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_CDLCLOSINGMARUBOZU.c) |
| Rust | [`cdlclosingmarubozu.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/library/src/ta_func/cdlclosingmarubozu.rs) |
| Java | [`Core_CDLCLOSINGMARUBOZU.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/library/fragments/Core_CDLCLOSINGMARUBOZU.java) |

TA-Lib is also available for Python, R and more using a [wrapper](https://ta-lib.org/wrappers/).

## Aliases

Closing Marubozu

## See Also

CDLMARUBOZU · CDLLONGLINE · CDLBELTHOLD
