# CDLKICKING

## Summary

Two-candle pattern of two opposite-color marubozu (long bodies with very short shadows) separated by a price gap. A reversal signal whose direction is set by the second candle's color.

## Notes

- Bulkowski's testing found Kicking reverses only 53% (bullish) / 54% (bearish) of the time — both "near random" — and it's also one of the rarest patterns he tracked (frequency rank 100/103 bullish, 102/103 bearish). ([thepatternsite.com](https://thepatternsite.com/KickingBull.html))

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
