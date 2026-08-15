# CDL3OUTSIDE

## Summary

A three-candle pattern: an engulfing pair (candle 2's body fully engulfs candle 1's body) followed by a third candle that confirms in the engulfing direction. Signals a bullish reversal (Three Outside Up) or bearish reversal (Three Outside Down).

## Notes

- Does not verify the prior trend the pattern classically assumes (three outside up is meaningful in a downtrend, three outside down in an uptrend).
- Bulkowski's testing puts Three Outside Up at a 75% bullish-reversal success rate versus 69% for Three Outside Down — both notably higher than the closely related Three Inside Up/Down (65%/60%), i.e. the engulfing "outside" variant tests as more reliable than the harami "inside" variant. ([thepatternsite.com](https://thepatternsite.com/ThreeOutsideUp.html))

## Inputs

- `inOpen` — Open price of each bar
- `inHigh` — High price of each bar
- `inLow` — Low price of each bar
- `inClose` — Close price of each bar

## Outputs

- `outInteger` — +100 for Three Outside Up (bullish), -100 for Three Outside Down (bearish), 0 when no pattern. Emits both signs; value is candle i-1's color * 100

## Output Values

| Value | Meaning |
|-------|---------|
| -100 | Three Outside Down — bearish reversal, most meaningful after an uptrend |
| 0 | No pattern |
| 100 | Three Outside Up — bullish reversal, most meaningful after a downtrend |

## Implementation

TA-Lib Definition: [`cdl3outside.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/cdl3outside/cdl3outside.c) · [`cdl3outside.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/cdl3outside/cdl3outside.yaml)

| Native | File |
|--------|------|
| C | [`ta_CDL3OUTSIDE.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_CDL3OUTSIDE.c) |
| Rust | [`cdl3outside.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/library/src/ta_func/cdl3outside.rs) |
| Java | [`Core_CDL3OUTSIDE.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/fragments/Core_CDL3OUTSIDE.java) |

TA-Lib is also available for Python, R and more using a [wrapper](/install/#wrappers).

## Aliases

Three Outside Up/Down, Three Outside

## See Also

CDL3INSIDE · CDLENGULFING · CDL3LINESTRIKE
