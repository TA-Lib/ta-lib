# CDLMORNINGDOJISTAR

## Summary

A three-candle bullish reversal pattern: a long black candle, then a doji that gaps down, then a white candle closing well up into the first candle's body. It is the doji-star variant of the morning star. A hit (+100) signals a bullish reversal; most meaningful after a downtrend, which this function does not verify.

## Notes

- The gap-down is measured between the candles' real bodies, not between their high/low ranges.
- A prior downtrend is not verified.

## Inputs

- `inOpen` — Open price of each bar
- `inHigh` — High price of each bar
- `inLow` — Low price of each bar
- `inClose` — Close price of each bar

## Outputs

- `outInteger` — +100 when the pattern is detected, 0 otherwise. Always bullish; never emits -100

## Output Values

| Value | Meaning |
|-------|---------|
| 0 | No pattern |
| 100 | Morning Doji Star detected — bullish reversal signal |

## Parameters

- `optInPenetration` — Fraction of the 1st candle's real body the 3rd close must exceed above close[i-2]; larger values demand deeper penetration into the black body

## Aliases

Morning Doji Star

## See Also

CDLMORNINGSTAR · CDLEVENINGDOJISTAR · CDLEVENINGSTAR · CDLDOJISTAR
