# CDLHANGINGMAN

## Summary

Single candle with a small real body, a long lower shadow, and little/no upper shadow, sitting at or near the highs of the prior candle. Bearish reversal signal. A hit is a bearish reversal signal (meaningful at the top of an uptrend).

## Notes

- Does not verify the preceding uptrend that the pattern classically assumes; confirm the trend context yourself.
- Bulkowski's testing found this acts as a bullish continuation 59% of the time — the opposite of the bearish-reversal reading it's named for ("near random") — and it ranks 87th of 103 patterns for post-breakout performance. ([thepatternsite.com](https://thepatternsite.com/HangingMan.html))

## Inputs

- `inOpen` — Open price of each bar
- `inHigh` — High price of each bar
- `inLow` — Low price of each bar
- `inClose` — Close price of each bar

## Outputs

- `outInteger` — -100 when detected (always bearish), 0 otherwise. Never emits +100

## Output Values

| Value | Meaning |
|-------|---------|
| -100 | Hanging Man pattern detected: bearish |
| 0 | No pattern |

## Aliases

Hanging Man

## See Also

CDLHAMMER · CDLINVERTEDHAMMER · CDLSHOOTINGSTAR · CDLTAKURI
