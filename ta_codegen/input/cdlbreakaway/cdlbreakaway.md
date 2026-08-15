# CDLBREAKAWAY

## Summary

A five-candle reversal pattern: a long first candle, a same-colored second candle that gaps away from it by its real body, two more candles extending the move, and an opposite-colored fifth candle that closes back inside the gap. Emits a bullish signal (bottom reversal) or bearish signal (top reversal).

## Notes

- Does not verify the prior trend the pattern classically assumes (a breakaway matters most against a preceding move).
- Bulkowski's data shows a directional asymmetry TA-Lib's symmetric output doesn't capture: bullish Breakaway reverses only 59% of the time ("near random"), while bearish Breakaway reverses 63% of the time overall. ([thepatternsite.com](https://thepatternsite.com/BullBreakaway.html))

## Inputs

- `inOpen` — Open price of each bar
- `inHigh` — High price of each bar
- `inLow` — Low price of each bar
- `inClose` — Close price of each bar

## Outputs

- `outInteger` — +100 when the fifth candle is white (bullish breakaway), -100 when it is black (bearish breakaway), 0 otherwise

## Output Values

| Value | Meaning |
|-------|---------|
| -100 | Bearish Breakaway — a gap-up advance gets erased by a sharp reversal candle, signaling a top |
| 0 | No pattern |
| 100 | Bullish Breakaway — a gap-down decline gets erased by a sharp reversal candle, signaling a bottom |

## Implementation

TA-Lib Definition: [`cdlbreakaway.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/cdlbreakaway/cdlbreakaway.c) · [`cdlbreakaway.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/cdlbreakaway/cdlbreakaway.yaml)

| Native | File |
|--------|------|
| C | [`ta_CDLBREAKAWAY.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_CDLBREAKAWAY.c) |
| Rust | [`cdlbreakaway.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/library/src/ta_func/cdlbreakaway.rs) |
| Java | [`Core_CDLBREAKAWAY.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/fragments/Core_CDLBREAKAWAY.java) |

TA-Lib is also available for Python, R and more using a [wrapper](/install/#wrappers).

## Aliases

Breakaway

## See Also

CDLGAPSIDESIDEWHITE · CDLRISEFALL3METHODS · CDL3LINESTRIKE
