# CDLABANDONEDBABY

## Summary

A three-candle reversal pattern: a long body, then a gapped-away doji, then a body of opposite color that gaps back the other way and closes deep into the first body. Bullish (bottom) or bearish (top) reversal signal.

## Notes

- Does not verify the prior trend the pattern classically assumes for significance.
- Bulkowski found the Abandoned Baby both very rare (293 occurrences out of 4.7 million candle lines, frequency rank 92 of 103) and unusually reliable when it does occur (70% success as a reversal, overall performance rank 9 of 103). ([thepatternsite.com](https://thepatternsite.com/AbandonBabyBull.html))

## Inputs

- `inOpen` — Open price of each bar
- `inHigh` — High price of each bar
- `inLow` — Low price of each bar
- `inClose` — Close price of each bar

## Outputs

- `outInteger` — +100 at a bullish abandoned baby bottom (3rd candle white), -100 at a bearish abandoned baby top (3rd candle black), 0 otherwise; sign = color of the 3rd candle

## Output Values

| Value | Meaning |
|-------|---------|
| -100 | Abandoned Baby Top — an isolated doji stranded by gaps on both sides at a high, a rare but strong bearish reversal |
| 0 | No pattern |
| 100 | Abandoned Baby Bottom — an isolated doji stranded by gaps on both sides at a low, a rare but strong bullish reversal |

## Parameters

- `optInPenetration` — Fraction of the 1st candle's real body the 3rd close must penetrate

## Implementation

TA-Lib Definition: [`cdlabandonedbaby.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/cdlabandonedbaby/cdlabandonedbaby.c) · [`cdlabandonedbaby.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/cdlabandonedbaby/cdlabandonedbaby.yaml)

| Native | File |
|--------|------|
| C | [`ta_CDLABANDONEDBABY.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_CDLABANDONEDBABY.c) |
| Rust | [`cdlabandonedbaby.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/library/src/ta_func/cdlabandonedbaby.rs) |
| Java | [`Core_CDLABANDONEDBABY.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/fragments/Core_CDLABANDONEDBABY.java) |

TA-Lib is also available for Python, R and more using a [wrapper](/install/#wrappers).

## Aliases

Abandoned Baby

## See Also

CDLEVENINGDOJISTAR · CDLMORNINGDOJISTAR · CDLEVENINGSTAR · CDLMORNINGSTAR
