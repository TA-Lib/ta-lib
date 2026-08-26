# SYNTH7

## Summary

Synthetic gate function: exposes the value of `ta_candlerange` as real outputs, one per rangeType arm, so the existing bitwise cross-language gates can compare an intermediate no shipped function makes observable. It exists only to verify the code generator end to end across all backends; it is never shipped (see `ta_codegen/generator/input_synth/README.md`).

## Formula

outRealBodyRange[i] = candleRange(BodyLong, bar i); outHighLowRange[i] = candleRange(BodyDoji, bar i); outShadowsRange[i] = candleRange(ShadowShort, bar i)

## Notes

- Covers the VALUE of `ta_candlerange` as a real output. Verified against `ta_codegen/input/`: all 57 shipped consumers of the helper are candlesticks whose output is 3-valued, which a 1-ULP helper difference essentially never straddles — so no shipped function makes the intermediate observable.
- Coverage trap: nothing is clamped. The arms diverge on bars whose low sits below half their high, which is where Sterbenz' lemma stops making the subtractions exact; clamp those bars away and every gate stays green while measuring nothing. The fuzz corpus is entirely finite (bounded near 1e9), so nothing non-finite reaches the bitwise comparison anyway.
- Coverage trap: all three arms are emitted on every bar rather than selected by a parameter, so a divergence in one cannot be masked by another. They come from named settings whose defaults already span `RealBody`, `HighLow` and `Shadows`, so no way to change candle settings is needed to reach them. The switch's fourth arm, the default `0.0`, is unreachable through a named setting at all — `TA_SetCandleSettings` rejects a rangeType above `Shadows` — so three of four is complete coverage here, not a gap to close.
- Coverage trap: the helper result goes to a LOCAL, never inline inside a larger expression. Every shipped candlestick calls it inside an `if`, so SYNTH7 and SYNTH8 are the only coverage of the hoisted-statement rendering — see `tests/candle_range_suite.rs`. Inline the call and that rendering goes untested.
- Real outputs rather than this family's usual integer ones: an integer output would reproduce exactly the blindness the fixture exists to remove.
- Issue #216.

## Inputs

- `inOpen` — Open price of each bar
- `inHigh` — High price of each bar
- `inLow` — Low price of each bar
- `inClose` — Close price of each bar

## Outputs

- `outRealBodyRange` — Candle range under a `RealBody` setting
- `outHighLowRange` — Candle range under a `HighLow` setting
- `outShadowsRange` — Candle range under a `Shadows` setting

## Implementation

TA-Lib Definition: [`synth7.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/generator/input_synth/synth7/synth7.c) · [`synth7.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/generator/input_synth/synth7/synth7.yaml)
