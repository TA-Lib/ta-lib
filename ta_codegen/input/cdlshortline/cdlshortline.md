# CDLSHORTLINE

## Summary

Single-candle pattern: a short real body with short upper and lower shadows (a small-range candle). Not a directional signal — the output sign encodes candle color, not bullish/bearish sentiment.

## Formula

One candle at i, all three:
- short real body: real body < the BodyShort average
- short upper shadow: upper shadow < the ShadowShort average
- short lower shadow: lower shadow < the ShadowShort average
If matched: output = candle color * 100 (+100 white, -100 black); else 0.

## Inputs

- `inOpen` — Open price of each bar
- `inHigh` — High price of each bar
- `inLow` — Low price of each bar
- `inClose` — Close price of each bar

## Outputs

- `outInteger` — +100 for a matching white candle (close>=open), -100 for a matching black candle (close<open), 0 when no pattern. Sign is candle color, NOT bullish/bearish

## Output Values

| Value | Meaning |
|-------|---------|
| -100 | Matching black short-line candle (color only — not a bearish call) |
| 0 | No pattern |
| 100 | Matching white short-line candle (color only — not a bullish call) |

## Implementation

TA-Lib Definition: [`cdlshortline.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/cdlshortline/cdlshortline.c) · [`cdlshortline.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/cdlshortline/cdlshortline.yaml)

| Native | File |
|--------|------|
| C | [`ta_CDLSHORTLINE.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_CDLSHORTLINE.c) |
| Rust | [`cdlshortline.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/library/src/ta_func/cdlshortline.rs) |
| Java | [`Core_CDLSHORTLINE.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/fragments/Core_CDLSHORTLINE.java) |

TA-Lib is also available for Python, R and more using a [wrapper](/install/#wrappers).

## Aliases

Short Line Candle, Short Line

## See Also

CDLLONGLINE · CDLSPINNINGTOP · CDLDOJI
