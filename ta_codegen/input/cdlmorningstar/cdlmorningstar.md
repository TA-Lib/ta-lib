# CDLMORNINGSTAR

## Summary

A three-candle bottom-reversal pattern: a long black candle, a small-bodied star gapping down, then a white candle closing well up into the first candle's body. Bullish reversal signal. A hit signals a bullish reversal (most meaningful after a downtrend, which the code does not check).

## Notes

- The gap-down is measured between the candles' real bodies, not between their high/low ranges.
- A prior downtrend is not verified.
- Bulkowski ranks the Morning Star unusually high — 6th of 103 for reversal rate (78%) and 12th of 103 for overall post-breakout performance — one of the few classic candle patterns whose textbook reputation his statistics confirm rather than debunk. ([thepatternsite.com](https://thepatternsite.com/MorningStar.html))

## Inputs

- `inOpen` — Open price of each bar
- `inHigh` — High price of each bar
- `inLow` — Low price of each bar
- `inClose` — Close price of each bar

## Outputs

- `outInteger` — +100 when the morning star is detected, 0 otherwise. Never negative (pattern is exclusively bullish)

## Output Values

| Value | Meaning |
|-------|---------|
| 0 | No pattern |
| 100 | Morning Star detected — bullish (bottom) reversal signal |

## Parameters

- `optInPenetration` — Fraction of the 1st candle's body the 3rd close must exceed above the 1st close; larger = deeper penetration required

## Aliases

Morning Star

## See Also

CDLMORNINGDOJISTAR · CDLEVENINGSTAR · CDLABANDONEDBABY · CDLDOJISTAR
