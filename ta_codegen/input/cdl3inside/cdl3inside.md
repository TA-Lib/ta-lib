# CDL3INSIDE

## Summary

A three-candle reversal pattern: a long real body, then a short real body totally engulfed by it (a harami), then a third candle of opposite color to the first that closes past the first candle's open. Signals a bullish reversal (three inside up, significant in a downtrend) or a bearish reversal (three inside down, significant in an uptrend).

## Notes

- Does not verify the prior trend the pattern classically assumes (three inside up is meaningful in a downtrend, three inside down in an uptrend).
- Bulkowski's testing found Three Inside Up succeeds as a bullish reversal 65% of the time (rank 20 of 103 overall) and Three Inside Down succeeds as a bearish reversal 60% of the time (rank 56 of 103) — both meaningfully better than a coin flip. ([thepatternsite.com](https://thepatternsite.com/ThreeInsideUp.html))

## Inputs

- `inOpen` — Open price of each bar
- `inHigh` — High price of each bar
- `inLow` — Low price of each bar
- `inClose` — Close price of each bar

## Outputs

- `outInteger` — +100 for three inside up (bullish reversal, first candle black), -100 for three inside down (bearish reversal, first candle white), 0 when no pattern. Computed as -candlecolor(1st)*100

## Output Values

| Value | Meaning |
|-------|---------|
| -100 | Three Inside Down — bearish reversal, most meaningful after an uptrend |
| 0 | No pattern |
| 100 | Three Inside Up — bullish reversal, most meaningful after a downtrend |

## Implementation

TA-Lib Definition: [`cdl3inside.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/cdl3inside/cdl3inside.c) · [`cdl3inside.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/cdl3inside/cdl3inside.yaml)

| Native | File |
|--------|------|
| C | [`ta_CDL3INSIDE.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_CDL3INSIDE.c) |
| Rust | [`cdl3inside.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/library/src/ta_func/cdl3inside.rs) |
| Java | [`Core_CDL3INSIDE.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/fragments/Core_CDL3INSIDE.java) |

TA-Lib is also available for Python, R and more using a [wrapper](/install/#wrappers).

## Aliases

Three Inside Up/Down, Three Inside, Three Inside Up, Three Inside Down

## See Also

CDLHARAMI · CDL3OUTSIDE · CDLENGULFING
