# CDLHAMMER

## Summary

Single-candle pattern: a small real body at the top of the range with a long lower shadow and little or no upper shadow, sitting at or near the prior candle's low. A hit flags a potential bullish reversal.

## Notes

- Does not verify the preceding downtrend that the pattern classically assumes; confirm the trend context yourself.
- Bulkowski's testing found the Hammer reverses a preceding downtrend about 60% of the time — in his words "not far from random (50%)" — and it ranks a modest 65th of 103 patterns for post-breakout performance. ([thepatternsite.com](https://thepatternsite.com/Hammer.html))

## Inputs

- `inOpen` — Open price of each bar
- `inHigh` — High price of each bar
- `inLow` — Low price of each bar
- `inClose` — Close price of each bar

## Outputs

- `outInteger` — +100 when the hammer is detected, 0 otherwise. Bullish only; never emits -100

## Output Values

| Value | Meaning |
|-------|---------|
| 0 | No pattern |
| 100 | Hammer detected — a bullish reversal signal that classically assumes a preceding downtrend, unverified by the function |

## Aliases

Hammer

## See Also

CDLINVERTEDHAMMER · CDLHANGINGMAN · CDLTAKURI
