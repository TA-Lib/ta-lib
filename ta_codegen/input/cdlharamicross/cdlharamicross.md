# CDLHARAMICROSS

## Summary

A two-candle reversal pattern: a long real body followed by a doji whose real body is contained within the first candle's real body (the doji variant of the Harami). Bullish after a black first candle, bearish after a white first candle.

## Notes

- Does not verify the prior trend (downtrend for bullish, uptrend for bearish) that the reversal signal assumes.
- Bulkowski's testing found the bearish Harami Cross behaves opposite its textbook label even more strongly than the plain Harami: it acts as a bullish CONTINUATION 57% of the time rather than a bearish reversal, and the bullish Harami Cross likewise fails to reverse the downtrend 55% of the time. ([thepatternsite.com](https://thepatternsite.com/HaramiCrossBear.html))

## Inputs

- `inOpen` — Open price of each bar
- `inHigh` — High price of each bar
- `inLow` — Low price of each bar
- `inClose` — Close price of each bar

## Outputs

- `outInteger` — +100/+80 when the first candle is black (bullish), -100/-80 when the first candle is white (bearish), 0 otherwise. Magnitude 100 for strict containment inside the first body, 80 when one real-body end matches

## Output Values

| Value | Meaning |
|-------|---------|
| -100 | Bearish Harami Cross: a doji forms inside the prior long white candle — indecision after an advance, a sharper warning than a plain Harami |
| -80 | Bearish Harami Cross, weaker variant: the doji's edge lines up exactly with one end of the long white candle |
| 0 | No pattern |
| 80 | Bullish Harami Cross, weaker variant: the doji's edge lines up exactly with one end of the long black candle |
| 100 | Bullish Harami Cross: a doji forms inside the prior long black candle — indecision after a decline, a sharper warning than a plain Harami |

## Implementation

TA-Lib Definition: [`cdlharamicross.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/cdlharamicross/cdlharamicross.c) · [`cdlharamicross.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/cdlharamicross/cdlharamicross.yaml)

| Native | File |
|--------|------|
| C | [`ta_CDLHARAMICROSS.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_CDLHARAMICROSS.c) |
| Rust | [`cdlharamicross.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/library/src/ta_func/cdlharamicross.rs) |
| Java | [`Core_CDLHARAMICROSS.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/fragments/Core_CDLHARAMICROSS.java) |

TA-Lib is also available for Python, R and more using a [wrapper](/install/#wrappers).

## Aliases

Harami Cross

## See Also

CDLHARAMI · CDLDOJI
