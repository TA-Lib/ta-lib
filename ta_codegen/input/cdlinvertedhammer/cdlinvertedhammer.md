# CDLINVERTEDHAMMER

## Summary

Single-candle pattern: a small real body with a long upper shadow and little-to-no lower shadow that gaps down from the prior candle. A hit flags a potential bullish reversal.

## Notes

- Does not verify the preceding downtrend that the pattern classically assumes; it only checks the gap down from the immediately preceding candle.
- Despite the bullish-reversal label, Bulkowski's testing found this actually behaves as a bearish continuation 65% of the time — yet its overall post-breakout performance rank (6th of 103) is among the best of all candlestick patterns he studied. ([thepatternsite.com](https://thepatternsite.com/HammerInv.html))

## Inputs

- `inOpen` — Open price of each bar
- `inHigh` — High price of each bar
- `inLow` — Low price of each bar
- `inClose` — Close price of each bar

## Outputs

- `outInteger` — +100 when the inverted hammer is detected, 0 otherwise. Never emits -100; the pattern is always bullish

## Output Values

| Value | Meaning |
|-------|---------|
| 0 | No pattern |
| 100 | Inverted Hammer detected — a bullish reversal signal that classically assumes a preceding downtrend, unverified by the function |

## Aliases

Inverted Hammer

## See Also

CDLHAMMER · CDLSHOOTINGSTAR · CDLHANGINGMAN
