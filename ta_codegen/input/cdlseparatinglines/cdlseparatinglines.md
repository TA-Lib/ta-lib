# CDLSEPARATINGLINES

## Summary

A two-candle continuation pattern: the second candle opposes the first in color, opens at the same price as the first, and is a long-bodied belt hold. Bullish (white second candle) or bearish (black second candle) continuation signal. Trend continuation: +100 = bullish (white belt hold), -100 = bearish (black belt hold).

## Formula

Two consecutive candles i-1, i: (1) opposite colors: color(i-1) == -color(i); (2) same open: open[i-1]-Equal_avg <= open[i] <= open[i-1]+Equal_avg; (3) long body: realbody(i) > BodyLong_avg; (4) belt hold: if i is white, lowershadow(i) < ShadowVeryShort_avg; if i is black, uppershadow(i) < ShadowVeryShort_avg.

## Notes

- A prior trend is not verified, nor that the pattern aligns with it.

## Inputs

- `inPriceOHLC` — Open, High, Low, Close price series

## Outputs

- `outInteger` — +100 for a bullish (white second candle) hit, -100 for a bearish (black second candle) hit, 0 otherwise

## Implementation

TA-Lib Definition: [`cdlseparatinglines.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/cdlseparatinglines/cdlseparatinglines.c) · [`cdlseparatinglines.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/cdlseparatinglines/cdlseparatinglines.yaml)

| Native | File |
|--------|------|
| C | [`ta_CDLSEPARATINGLINES.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_CDLSEPARATINGLINES.c) |
| Rust | [`cdlseparatinglines.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/library/src/ta_func/cdlseparatinglines.rs) |
| Java | [`Core_CDLSEPARATINGLINES.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/library/fragments/Core_CDLSEPARATINGLINES.java) |

TA-Lib is also available for Python, R and more using a [wrapper](https://ta-lib.org/wrappers/).

## Aliases

Separating Lines

## See Also

CDLBELTHOLD · CDLMEETINGLINES
