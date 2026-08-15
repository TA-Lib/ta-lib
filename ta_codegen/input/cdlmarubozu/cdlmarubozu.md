# CDLMARUBOZU

## Summary

Single candle with a long real body and no/very-short upper and lower shadows, so open and close sit at the range extremes. Bullish (white) or bearish (black) reversal/strength signal per the body color.

## Formula

One candle at i. Match when: realbody(i) > BodyLong average AND upperShadow(i) < ShadowVeryShort average AND lowerShadow(i) < ShadowVeryShort average. If matched emit candlecolor(i)*100 (+100 white when close>=open, -100 black when close<open); else 0.

## Notes

- Despite the shape's strong-conviction reputation, Bulkowski's testing found a Marubozu continues in its expected direction only about 53% (black) to 56% (white) of the time — both "near random." ([thepatternsite.com](https://thepatternsite.com/BlackMarubozu.html))

## Inputs

- `inOpen` — Open price of each bar
- `inHigh` — High price of each bar
- `inLow` — Low price of each bar
- `inClose` — Close price of each bar

## Outputs

- `outInteger` — +100 on a white (bullish) marubozu, -100 on a black (bearish) marubozu, 0 when no pattern. Sign follows the candle color

## Output Values

| Value | Meaning |
|-------|---------|
| -100 | Black marubozu — close near the low with no shadows, signaling strong selling pressure |
| 0 | No pattern |
| 100 | White marubozu — close near the high with no shadows, signaling strong buying pressure |

## Implementation

TA-Lib Definition: [`cdlmarubozu.c`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/cdlmarubozu/cdlmarubozu.c) · [`cdlmarubozu.yaml`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/input/cdlmarubozu/cdlmarubozu.yaml)

| Native | File |
|--------|------|
| C | [`ta_CDLMARUBOZU.c`](https://github.com/TA-Lib/ta-lib/blob/main/src/ta_func/ta_CDLMARUBOZU.c) |
| Rust | [`cdlmarubozu.rs`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/rust/library/src/ta_func/cdlmarubozu.rs) |
| Java | [`Core_CDLMARUBOZU.java`](https://github.com/TA-Lib/ta-lib/blob/main/ta_codegen/output/java/fragments/Core_CDLMARUBOZU.java) |

TA-Lib is also available for Python, R and more using a [wrapper](/install/#wrappers).

## Aliases

Marubozu, Shaven Head/Bottom

## See Also

CDLCLOSINGMARUBOZU · CDLLONGLINE · CDLBELTHOLD
