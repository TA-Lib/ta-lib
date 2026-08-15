# CDLSTALLEDPATTERN

## Summary

A three-candle pattern of three white candles with consecutively higher closes where the third loses momentum (a small body riding on the shoulder of the second's long body). It is a bearish reversal signal of a stalling advance. A hit (-100) is bearish: the uptrend is stalling and may reverse.

## Notes

- The pattern classically appears in an uptrend, but this function does not verify a prior uptrend; the caller must confirm it.
- Bulkowski's testing shows this classically-bearish pattern actually acts as a bullish continuation 77% of the time — the reverse of the label — because price tends to close above the pattern's top rather than turning down. ([thepatternsite.com](https://thepatternsite.com/Deliberation.html))

## Inputs

- `inOpen` — Open price of each bar
- `inHigh` — High price of each bar
- `inLow` — Low price of each bar
- `inClose` — Close price of each bar

## Outputs

- `outInteger` — -100 when the pattern is detected (always bearish), 0 otherwise. Never emits +100

## Output Values

| Value | Meaning |
|-------|---------|
| -100 | Stalled Pattern detected: bearish |
| 0 | No pattern |

## Implementation

TA-Lib Definition: [`cdlstalledpattern.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/cdlstalledpattern/cdlstalledpattern.c) · [`cdlstalledpattern.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/cdlstalledpattern/cdlstalledpattern.yaml)

| Native | File |
|--------|------|
| C | [`ta_CDLSTALLEDPATTERN.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_CDLSTALLEDPATTERN.c) |
| Rust | [`cdlstalledpattern.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/library/src/ta_func/cdlstalledpattern.rs) |
| Java | [`Core_CDLSTALLEDPATTERN.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/fragments/Core_CDLSTALLEDPATTERN.java) |

TA-Lib is also available for Python, R and more using a [wrapper](/install/#wrappers).

## Aliases

Stalled Pattern, Deliberation Pattern

## See Also

CDLADVANCEBLOCK · CDL3WHITESOLDIERS · CDLXSIDEGAP3METHODS
