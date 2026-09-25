# CDLKICKINGBYLENGTH

## Summary

A two-candle pattern of two opposite-color marubozu (long body, very short shadows on both ends) separated by a gap. A strong directional/reversal signal whose bull/bear bias is set by the longer of the two marubozu.

## Inputs

- `inOpen` — Open price of each bar
- `inHigh` — High price of each bar
- `inLow` — Low price of each bar
- `inClose` — Close price of each bar

## Outputs

- `outInteger` — +100 or -100 on a hit, 0 otherwise. Sign = candlecolor of the candle with the larger realbody (i if realbody(i) > realbody(i-1), else i-1; tie goes to i-1): +100 if that marubozu is white, -100 if black

## Output Values

| Value | Meaning |
|-------|---------|
| -100 | Bearish Kicking by Length: the longer of the two marubozu closed black |
| 0 | No pattern |
| 100 | Bullish Kicking by Length: the longer of the two marubozu closed white |

## Aliases

Kicking by Length, Kicking - bull/bear decided by the longer marubozu

## See Also

CDLKICKING · CDLMARUBOZU · CDLGAPSIDESIDEWHITE
