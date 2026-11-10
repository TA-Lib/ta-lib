# CDLKICKING

## Summary

Two-candle pattern of two opposite-color marubozu (long bodies with very short shadows) separated by a price gap. A reversal signal whose direction is set by the second candle's color.

## Notes

- Bulkowski's testing found Kicking reverses only 53% (bullish) / 54% (bearish) of the time, both "near random". ([thepatternsite.com](https://thepatternsite.com/KickingBull.html))
- Rare: a few years of one symbol's daily bars may show none. Bulkowski ranks the bullish form 100th and the bearish 102nd of 103 patterns by how often they occur ([bullish](https://thepatternsite.com/KickingBull.html), [bearish](https://thepatternsite.com/KickingBear.html)).

## Inputs

- `inOpen` — Open price of each bar
- `inHigh` — High price of each bar
- `inLow` — Low price of each bar
- `inClose` — Close price of each bar

## Outputs

- `outInteger` — +100 when the second candle is white (bullish), -100 when it is black (bearish), 0 otherwise

## Output Values

| Value | Meaning |
|-------|---------|
| -100 | Bearish Kicking: the second (gapping) marubozu closed black |
| 0 | No pattern |
| 100 | Bullish Kicking: the second (gapping) marubozu closed white |

## Aliases

Kicking

## See Also

CDLKICKINGBYLENGTH · CDLMARUBOZU · CDLGAPSIDESIDEWHITE
