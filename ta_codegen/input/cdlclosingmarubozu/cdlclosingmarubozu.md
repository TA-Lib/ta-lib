# CDLCLOSINGMARUBOZU

## Summary

Single-candle pattern: a long real body whose closing end has no or very short shadow, so the close sits at the candle's extreme. A strong directional bar, not a defined reversal/continuation signal — white is bullish, black is bearish.

## Formula

One candle. Requires: (1) long real body: real body > the BodyLong average; AND (2) very short shadow at the closing end: if white (close>=open) upper shadow < the ShadowVeryShort average [close at/near high]; if black (close<open) lower shadow < the ShadowVeryShort average [close at/near low].

## Notes

- Bulkowski's testing found Closing Marubozu continues in its expected direction only marginally more than chance — 52% for the black variant — which he calls "near random." ([thepatternsite.com](https://thepatternsite.com/CloseBlkMarubozu.html))

## Inputs

- `inOpen` — Open price of each bar
- `inHigh` — High price of each bar
- `inLow` — Low price of each bar
- `inClose` — Close price of each bar

## Outputs

- `outInteger` — +100 for a white (bullish) closing marubozu, -100 for a black (bearish) one, 0 otherwise

## Output Values

| Value | Meaning |
|-------|---------|
| -100 | Closing Marubozu (bearish) — closes right at the session low, sellers in full control into the close |
| 0 | No pattern |
| 100 | Closing Marubozu (bullish) — closes right at the session high, buyers in full control into the close |

## Implementation

TA-Lib Definition: [`cdlclosingmarubozu.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/cdlclosingmarubozu/cdlclosingmarubozu.c) · [`cdlclosingmarubozu.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/cdlclosingmarubozu/cdlclosingmarubozu.yaml)

| Native | File |
|--------|------|
| C | [`ta_CDLCLOSINGMARUBOZU.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_CDLCLOSINGMARUBOZU.c) |
| Rust | [`cdlclosingmarubozu.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/library/src/ta_func/cdlclosingmarubozu.rs) |
| Java | [`Core_CDLCLOSINGMARUBOZU.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/fragments/Core_CDLCLOSINGMARUBOZU.java) |

TA-Lib is also available for Python, R and more using a [wrapper](/install/#wrappers).

## Aliases

Closing Marubozu

## See Also

CDLMARUBOZU · CDLLONGLINE · CDLBELTHOLD
