# CDLGRAVESTONEDOJI

## Summary

Single-candle doji whose open and close sit at the low of the day, leaving a long upper shadow and no lower shadow. A doji variant whose bullish/bearish meaning depends on the surrounding trend, which the code does not judge. A hit marks a gravestone doji; its bullish vs bearish reversal meaning must be read against the prevailing trend, which this function does not check.

## Formula

One candle. Detected when all hold: (1) doji body: realbody |close-open| <= BodyDoji average; (2) very short/absent lower shadow: lowerShadow < ShadowVeryShort average; (3) non-short upper shadow: upperShadow > ShadowVeryShort average (open/close at the low with an upper shadow).

## Notes

- Does not verify the prior trend that determines the pattern's bullish/bearish meaning.
- Bulkowski's testing found the bearish reversal traders expect actually shows up only 51% of the time — essentially random — and it ranks 77th of 103 patterns for post-breakout performance. ([thepatternsite.com](https://thepatternsite.com/Gravestone.html))

## Inputs

- `inOpen` — Open price of each bar
- `inHigh` — High price of each bar
- `inLow` — Low price of each bar
- `inClose` — Close price of each bar

## Outputs

- `outInteger` — +100 on a detected gravestone doji, 0 otherwise. Never negative; the positive sign is not a directional signal (evaluate relative to the trend)

## Output Values

| Value | Meaning |
|-------|---------|
| 0 | No pattern |
| 100 | Gravestone Doji detected — a potential reversal; direction (bullish/bearish) must be read from the prevailing trend, which this function does not check |

## Implementation

TA-Lib Definition: [`cdlgravestonedoji.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/cdlgravestonedoji/cdlgravestonedoji.c) · [`cdlgravestonedoji.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/cdlgravestonedoji/cdlgravestonedoji.yaml)

| Native | File |
|--------|------|
| C | [`ta_CDLGRAVESTONEDOJI.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_CDLGRAVESTONEDOJI.c) |
| Rust | [`cdlgravestonedoji.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/library/src/ta_func/cdlgravestonedoji.rs) |
| Java | [`Core_CDLGRAVESTONEDOJI.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/fragments/Core_CDLGRAVESTONEDOJI.java) |

TA-Lib is also available for Python, R and more using a [wrapper](/install/#wrappers).

## Aliases

Gravestone Doji

## See Also

CDLDOJI · CDLDRAGONFLYDOJI · CDLLONGLEGGEDDOJI · CDLDOJISTAR
