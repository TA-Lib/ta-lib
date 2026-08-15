# CDLBELTHOLD

## Summary

Single-candle pattern with a long real body that opens at (or near) its extreme. A bullish belt-hold is a long white candle with no/very short lower shadow; a bearish belt-hold is a long black candle with no/very short upper shadow. A white hit is bullish (opens at the low, closes strong); a black hit is bearish (opens at the high, closes weak).

## Formula

One candle. Requires real body > BodyLong average (long body), then either: white body (close>=open) AND lower shadow < ShadowVeryShort average -> bullish; OR black body (close<open) AND upper shadow < ShadowVeryShort average -> bearish. No prior-trend or gap conditions are checked.

## Notes

- Does not verify the prior trend that the pattern's bullish/bearish reading classically assumes.
- Bulkowski's testing ranks the bullish Belt-Hold's 71% reversal rate 11th of 103 patterns for pure reversal reliability (bearish reverses 68% of the time) — though its overall post-breakout performance rank is a more middling 62nd/63rd of 103. ([thepatternsite.com](https://thepatternsite.com/BeltHoldBull.html))

## Inputs

- `inOpen` — Open price of each bar
- `inHigh` — High price of each bar
- `inLow` — Low price of each bar
- `inClose` — Close price of each bar

## Outputs

- `outInteger` — +100 for a bullish (white) belt-hold, -100 for a bearish (black) belt-hold, 0 otherwise

## Output Values

| Value | Meaning |
|-------|---------|
| -100 | Bearish belt-hold — opens at (or near) the high and sells off hard into the close, showing strong seller conviction |
| 0 | No pattern |
| 100 | Bullish belt-hold — opens at (or near) the low and rallies hard into the close, showing strong buyer conviction |

## Implementation

TA-Lib Definition: [`cdlbelthold.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/cdlbelthold/cdlbelthold.c) · [`cdlbelthold.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/cdlbelthold/cdlbelthold.yaml)

| Native | File |
|--------|------|
| C | [`ta_CDLBELTHOLD.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_CDLBELTHOLD.c) |
| Rust | [`cdlbelthold.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/library/src/ta_func/cdlbelthold.rs) |
| Java | [`Core_CDLBELTHOLD.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/fragments/Core_CDLBELTHOLD.java) |

TA-Lib is also available for Python, R and more using a [wrapper](/install/#wrappers).

## Aliases

Belt-hold, Belt Hold Line

## See Also

CDLCLOSINGMARUBOZU · CDLMARUBOZU · CDLLONGLINE
