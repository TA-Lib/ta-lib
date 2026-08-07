# CDLHIKKAKE

## Summary

A 3-bar pattern: an inside bar followed by a false breakout, optionally later confirmed by a follow-through bar. Signals a bullish or bearish reversal/continuation depending on the breakout direction. A false-breakout setup: positive = bullish, negative = bearish; magnitude 200 flags the confirming bar.

## Inputs

- `inOpen` — Open price of each bar
- `inHigh` — High price of each bar
- `inLow` — Low price of each bar
- `inClose` — Close price of each bar

## Outputs

- `outInteger` — +100/-100 at the hikkake (breakout) bar for bull/bear; +200/-200 at a later confirmation bar; 0 otherwise

## Implementation

TA-Lib Definition: [`cdlhikkake.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/cdlhikkake/cdlhikkake.c) · [`cdlhikkake.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/cdlhikkake/cdlhikkake.yaml)

| Native | File |
|--------|------|
| C | [`ta_CDLHIKKAKE.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_CDLHIKKAKE.c) |
| Rust | [`cdlhikkake.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/library/src/ta_func/cdlhikkake.rs) |
| Java | [`Core_CDLHIKKAKE.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/fragments/Core_CDLHIKKAKE.java) |

TA-Lib is also available for Python, R and more using a [wrapper](/install/#wrappers).

## Aliases

Hikkake Pattern, Hikkake

## See Also

CDLHIKKAKEMOD · CDLHARAMI
