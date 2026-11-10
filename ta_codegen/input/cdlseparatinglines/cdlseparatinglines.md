# CDLSEPARATINGLINES

## Summary

A two-candle continuation pattern: the second candle opposes the first in color, opens at the same price as the first, and is a long-bodied belt hold. Bullish (white second candle) or bearish (black second candle) continuation signal.

## Formula

Two consecutive candles i-1, i: (1) opposite colors: color(i-1) == -color(i); (2) same open: open[i-1]-Equal_avg <= open[i] <= open[i-1]+Equal_avg; (3) long body: realbody(i) > BodyLong_avg; (4) belt hold: if i is white, lowershadow(i) < ShadowVeryShort_avg; if i is black, uppershadow(i) < ShadowVeryShort_avg.

## Notes

- A prior trend is not verified, nor that the pattern aligns with it.
- Rare in Bulkowski's tests: he ranks the bullish form 76th and the bearish 82nd of 103 patterns by how often they occur ([bullish](https://thepatternsite.com/SeparateLinesBull.html), [bearish](https://thepatternsite.com/SeparateLinesBear.html)). It fires more often in crypto: 120 times in [about 16,000 daily bars](/api/candlestick-outputs/#how-often-patterns-fire).

## Inputs

- `inOpen` — Open price of each bar
- `inHigh` — High price of each bar
- `inLow` — Low price of each bar
- `inClose` — Close price of each bar

## Outputs

- `outInteger` — +100 for a bullish (white second candle) hit, -100 for a bearish (black second candle) hit, 0 otherwise

## Output Values

| Value | Meaning |
|-------|---------|
| -100 | Bearish Separating Lines: the second candle (belt hold) closed black |
| 0 | No pattern |
| 100 | Bullish Separating Lines: the second candle (belt hold) closed white |

## Aliases

Separating Lines

## See Also

CDLBELTHOLD · CDLCOUNTERATTACK
