# CDLRICKSHAWMAN

## Summary

Single-candle doji with two long shadows whose body sits near the midpoint of the high-low range. It is a neutral indecision signal, not a directional (bullish/bearish) reversal. A hit marks market indecision/uncertainty; neutral, neither bullish nor bearish.

## Notes

- Bulkowski's verdict: "The rickshaw man candle may look pretty on the chart but it has no investment implications that I have been able to find" — his testing shows it continues only 51% of the time, statistically random. ([thepatternsite.com](https://thepatternsite.com/RickshawMan.html))

## Inputs

- `inOpen` — Open price of each bar
- `inHigh` — High price of each bar
- `inLow` — Low price of each bar
- `inClose` — Close price of each bar

## Outputs

- `outInteger` — +100 when the pattern is present, 0 otherwise. Never -100; the code notes the positive value does NOT imply bullish, it signals uncertainty

## Output Values

| Value | Meaning |
|-------|---------|
| 0 | No pattern |
| 100 | Rickshaw Man detected — neutral indecision signal, not a directional (bullish/bearish) bias |

## Implementation

TA-Lib Definition: [`cdlrickshawman.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/cdlrickshawman/cdlrickshawman.c) · [`cdlrickshawman.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/cdlrickshawman/cdlrickshawman.yaml)

| Native | File |
|--------|------|
| C | [`ta_CDLRICKSHAWMAN.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_CDLRICKSHAWMAN.c) |
| Rust | [`cdlrickshawman.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/library/src/ta_func/cdlrickshawman.rs) |
| Java | [`Core_CDLRICKSHAWMAN.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/fragments/Core_CDLRICKSHAWMAN.java) |

TA-Lib is also available for Python, R and more using a [wrapper](/install/#wrappers).

## Aliases

Rickshaw Man

## See Also

CDLLONGLEGGEDDOJI · CDLDOJI · CDLHIGHWAVE
