# CDLCONCEALBABYSWALL

## Summary

A four-candle pattern: two black marubozus, then a black candle that gaps down but pokes its upper shadow into the prior body, then a larger black candle fully engulfing the third. A hit signals a bullish reversal.

## Notes

- Does not verify the preceding downtrend the pattern classically assumes.
- Despite the bullish-reversal label, Bulkowski's testing found this pattern actually behaves as a bearish continuation 75% of the time — though the finding rests on just 4 occurrences out of 4.7 million candle lines, and it ranks 101st of 103 patterns overall. ([thepatternsite.com](https://thepatternsite.com/ConcealBaby.html))

## Inputs

- `inOpen` — Open price of each bar
- `inHigh` — High price of each bar
- `inLow` — Low price of each bar
- `inClose` — Close price of each bar

## Outputs

- `outInteger` — +100 on a match, 0 otherwise; never emits -100 (pattern is always bullish)

## Output Values

| Value | Meaning |
|-------|---------|
| 0 | No pattern |
| 100 | Concealing Baby Swallow detected — a bullish reversal signal, most meaningful after a downtrend (unverified by the function) |

## Aliases

Concealing Baby Swallow

## See Also

CDLMARUBOZU · CDLENGULFING
