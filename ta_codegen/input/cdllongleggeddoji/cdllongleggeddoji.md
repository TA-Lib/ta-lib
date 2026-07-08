# CDLLONGLEGGEDDOJI

## Summary

Single-candle doji (open ~ close) with at least one long shadow. Signals market indecision, not a directional bias. Marks indecision/uncertainty; not inherently bullish or bearish despite the positive sign.

## Formula

One candle. Hit when: real body <= BodyDoji average (doji body) AND (lower shadow > ShadowLong average OR upper shadow > ShadowLong average), i.e. at least one long shadow.

## Notes

- Only one long shadow (upper or lower) is required, whereas the classic pattern shows both long upper and lower shadows.

## Inputs

- `inPriceOHLC` — OHLC price series (open, high, low, close)

## Outputs

- `outInteger` — +100 when the pattern is present, 0 otherwise. Only +100 is emitted; the code never emits -100, and the positive sign does NOT mean bullish

## Implementation

TA-Lib Definition: [`cdllongleggeddoji.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/cdllongleggeddoji/cdllongleggeddoji.c) · [`cdllongleggeddoji.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/cdllongleggeddoji/cdllongleggeddoji.yaml)

| Native | File |
|--------|------|
| C | [`ta_CDLLONGLEGGEDDOJI.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_CDLLONGLEGGEDDOJI.c) |
| Rust | [`cdllongleggeddoji.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/src/ta_func/cdllongleggeddoji.rs) |
| Java | [`Core_CDLLONGLEGGEDDOJI.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/Core_CDLLONGLEGGEDDOJI.java) |

TA-Lib is also available for Python, R and more using a [wrapper](https://ta-lib.org/wrappers/).

## Aliases

Long Legged Doji

## See Also

CDLDOJI · CDLGRAVESTONEDOJI · CDLDRAGONFLYDOJI · CDLRICKSHAWMAN
