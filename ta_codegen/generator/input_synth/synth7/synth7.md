# SYNTH7

## Summary

Synthetic gate function: exposes the value of `ta_candlerange` as real outputs, one per rangeType arm, so the existing bitwise cross-language gates can compare them. Every shipped consumer of this helper is a candlestick with a 3-valued output, which cannot show a 1-ULP helper difference; this makes the intermediate observable by construction. It exists only to verify the code generator end to end across all backends; it is never shipped (see `ta_codegen/generator/input_synth/README.md`).

## Formula

outRealBodyRange[i] = candleRange(BodyLong, bar i); outHighLowRange[i] = candleRange(BodyDoji, bar i); outShadowsRange[i] = candleRange(ShadowShort, bar i)

## Notes

- All three arms are emitted on every bar rather than selected by a parameter, so every swept bar covers every arm and a divergence in one cannot be masked by another.
- The arms come from named settings whose defaults already span `RealBody`, `HighLow` and `Shadows`, so no way to change candle settings is needed to reach them.
- Named settings are mandatory, not stylistic: the C backend maps this helper onto the `TA_CANDLERANGE` macro and recovers the setting name from the literal `<NAME>_rangeType` argument.
- This is the one synth family with real outputs rather than integer ones. An integer output would reproduce exactly the blindness the gate exists to remove.
- Nothing is clamped. The divergent region is bars whose low sits below half their high, which is where Sterbenz' lemma stops making the subtractions exact; clamping those away would hide what the gate measures.

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
