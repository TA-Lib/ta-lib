# CDLSTICKSANDWICH

## Summary

A three-candle bullish reversal pattern: two black candles (1st and 3rd) sandwiching a white candle, where the 3rd black candle closes at the same level as the 1st (the "bread"). A hit signals a bullish reversal (code comment notes it is significant in a downtrend, which the function does not verify).

## Notes

- Although classically a bullish reversal (and TA-Lib only emits +100), Bulkowski's testing found it actually acts as a bearish continuation 62% of the time — despite that, it still ranks a respectable 14th of 103 patterns for overall performance. ([thepatternsite.com](https://thepatternsite.com/StickSandwich.html))

## Inputs

- `inOpen` — Open price of each bar
- `inHigh` — High price of each bar
- `inLow` — Low price of each bar
- `inClose` — Close price of each bar

## Outputs

- `outInteger` — +100 when the pattern is present, 0 otherwise. Never -100 — Stick Sandwich is always bullish

## Output Values

| Value | Meaning |
|-------|---------|
| 0 | No pattern |
| 100 | Stick Sandwich detected — bullish reversal signal |

## Implementation

TA-Lib Definition: [`cdlsticksandwich.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/cdlsticksandwich/cdlsticksandwich.c) · [`cdlsticksandwich.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/cdlsticksandwich/cdlsticksandwich.yaml)

| Native | File |
|--------|------|
| C | [`ta_CDLSTICKSANDWICH.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_CDLSTICKSANDWICH.c) |
| Rust | [`cdlsticksandwich.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/library/src/ta_func/cdlsticksandwich.rs) |
| Java | [`Core_CDLSTICKSANDWICH.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/fragments/Core_CDLSTICKSANDWICH.java) |

TA-Lib is also available for Python, R and more using a [wrapper](/install/#wrappers).

## Aliases

Stick Sandwich

## See Also

CDLMATCHINGLOW · CDLHOMINGPIGEON
