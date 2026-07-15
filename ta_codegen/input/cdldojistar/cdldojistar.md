# CDLDOJISTAR

## Summary

A two-candle reversal pattern: a long real body followed by a doji whose real body gaps away from it (up after a white body, down after a black body). Signals a potential reversal of the prevailing trend. A hit flags a likely trend reversal; true direction depends on the prevailing trend (bullish in a downtrend, bearish in an uptrend), which the code does not itself verify.

## Formula

Two candles. Candle 1: long real body (realbody > BodyLong average). Candle 2: doji (realbody <= BodyDoji average). Gap: either candle 1 white (color==1) AND candle 2 real body gaps up above it (the real bodies gap up), or candle 1 black (color==-1) AND candle 2 real body gaps down below it (the real bodies gap down).

## Notes

- Does not verify the prior trend the reversal signal classically assumes.

## Inputs

- `inPriceOHLC` — OHLC price series (open, high, low, close)

## Outputs

- `outInteger` — Emits +100 or -100 on a hit, 0 otherwise. Value is -candlecolor(candle1)*100: -100 when candle 1 is white (gap up), +100 when candle 1 is black (gap down)

## Implementation

TA-Lib Definition: [`cdldojistar.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/cdldojistar/cdldojistar.c) · [`cdldojistar.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/cdldojistar/cdldojistar.yaml)

| Native | File |
|--------|------|
| C | [`ta_CDLDOJISTAR.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_CDLDOJISTAR.c) |
| Rust | [`cdldojistar.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/library/src/ta_func/cdldojistar.rs) |
| Java | [`Core_CDLDOJISTAR.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/library/fragments/Core_CDLDOJISTAR.java) |

TA-Lib is also available for Python, R and more using a [wrapper](https://ta-lib.org/wrappers/).

## Aliases

Doji Star

## See Also

CDLMORNINGDOJISTAR · CDLEVENINGDOJISTAR · CDLDOJI · CDLMORNINGSTAR · CDLEVENINGSTAR
