# CDLGAPSIDESIDEWHITE

## Summary

A three-candle pattern: a first candle followed by two white candles of similar body size that both gap the same direction (up or down) from the first candle's real body and open at about the same level. It is a continuation signal whose sign reports the gap direction; the code does not verify a prior trend.

## Notes

- Does not verify the prior trend the continuation signal classically assumes.
- Bulkowski's data shows the bullish form is rare (984 occurrences out of 4.7 million candle lines, frequency rank 73/103) but continues as labeled 66% of the time; the bearish form is rarer still (frequency rank 86/103) and its 56% continuation rate is "near random" — Bulkowski cautions the bearish sample is too thin to trust. ([thepatternsite.com](https://thepatternsite.com/SidebySideWhiteLinesBull.html))

## Inputs

- `inOpen` — Open price of each bar
- `inHigh` — High price of each bar
- `inLow` — Low price of each bar
- `inClose` — Close price of each bar

## Outputs

- `outInteger` — +100 for an up-gap (bullish continuation), -100 for a down-gap (bearish continuation), 0 when no pattern. Sign is set solely by the C2-vs-C1 gap direction (realbodygapup ? 100 : -100)

## Output Values

| Value | Meaning |
|-------|---------|
| -100 | Bearish continuation: two similar white candles gapped down together and held the gap, suggesting the decline will resume |
| 0 | No pattern |
| 100 | Bullish continuation: two similar white candles gapped up together and held the gap, suggesting the advance will resume |

## Aliases

Up/Down-gap side-by-side white lines, Gapping side-by-side white lines

## See Also

CDLTASUKIGAP · CDLXSIDEGAP3METHODS
