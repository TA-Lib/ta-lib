# CDLHARAMI

## Summary

Two-candle pattern: a long real body followed by a short real body contained within the first candle's real body. A reversal signal whose direction is the opposite of the first candle's color.

## Notes

- Does not verify the prior trend (downtrend for bullish, uptrend for bearish) that the reversal signal assumes.
- Bulkowski's testing found the bearish Harami actually acts as a bullish CONTINUATION 53% of the time — more often than it reverses the prior uptrend — rating the pattern "near random" overall (rank 72 of 103). ([thepatternsite.com](https://thepatternsite.com/HaramiBear.html))

## Inputs

- `inOpen` — Open price of each bar
- `inHigh` — High price of each bar
- `inLow` — Low price of each bar
- `inClose` — Close price of each bar

## Outputs

- `outInteger` — +100/+80 when the long 1st candle is black (bullish), -100/-80 when it is white (bearish), 0 otherwise; 80 when the two real bodies share an end, 100 when the 1st body strictly overhangs both ends of the 2nd

## Output Values

| Value | Meaning |
|-------|---------|
| -100 | Bearish Harami: a small real body forms inside the prior long white candle — momentum stalling after an advance |
| -80 | Bearish Harami, weaker variant: the small body's edge lines up exactly with one end of the long white candle |
| 0 | No pattern |
| 80 | Bullish Harami, weaker variant: the small body's edge lines up exactly with one end of the long black candle |
| 100 | Bullish Harami: a small real body forms inside the prior long black candle — momentum stalling after a decline |

## Implementation

TA-Lib Definition: [`cdlharami.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/cdlharami/cdlharami.c) · [`cdlharami.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/cdlharami/cdlharami.yaml)

| Native | File |
|--------|------|
| C | [`ta_CDLHARAMI.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_CDLHARAMI.c) |
| Rust | [`cdlharami.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/library/src/ta_func/cdlharami.rs) |
| Java | [`Core_CDLHARAMI.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/fragments/Core_CDLHARAMI.java) |

TA-Lib is also available for Python, R and more using a [wrapper](/install/#wrappers).

## Aliases

Harami, Harami Pattern

## See Also

CDLHARAMICROSS · CDLENGULFING
