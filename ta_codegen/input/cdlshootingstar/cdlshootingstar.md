# CDLSHOOTINGSTAR

## Summary

Single-candle pattern: a small real body with a long upper shadow and little-to-no lower shadow that gaps up from the prior candle's real body. Bearish reversal signal. A hit (-100) flags a bearish reversal at the top of an uptrend.

## Notes

- A preceding uptrend is not verified.
- Bulkowski found this reverses only 59% of the time — "near random," summarized in his words as "this candle looks better than it performs" — ranking 55th of 103 patterns. ([thepatternsite.com](https://thepatternsite.com/ShootingStar.html))

## Inputs

- `inOpen` — Open price of each bar
- `inHigh` — High price of each bar
- `inLow` — Low price of each bar
- `inClose` — Close price of each bar

## Outputs

- `outInteger` — -100 when the shooting star is detected, 0 otherwise. Only ever emits negative (bearish); never +100

## Output Values

| Value | Meaning |
|-------|---------|
| -100 | Shooting Star pattern detected: bearish |
| 0 | No pattern |

## Aliases

Shooting Star

## See Also

CDLINVERTEDHAMMER · CDLHANGINGMAN · CDLHAMMER · CDLGRAVESTONEDOJI
