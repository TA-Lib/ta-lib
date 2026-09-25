# CDLRICKSHAWMAN

## Summary

Single-candle doji with two long shadows whose body sits near the midpoint of the high-low range. It is a neutral indecision signal, not a directional (bullish/bearish) reversal. A hit marks market indecision/uncertainty; neutral, neither bullish nor bearish.

## Notes

- Bulkowski's verdict: "The rickshaw man candle may look pretty on the chart but it has no investment implications that I have been able to find" — his testing shows it continues only 51% of the time, statistically random. ([thepatternsite.com](https://thepatternsite.com/RickshawMan.html))

## Inputs

- `inOpen` — Open price of each bar
- `inHigh` — High price of each bar
- `inLow` — Low price of each bar
- `inClose` — Close price of each bar

## Outputs

- `outInteger` — +100 when the pattern is present, 0 otherwise. Never -100; the code notes the positive value does NOT imply bullish, it signals uncertainty

## Output Values

| Value | Meaning |
|-------|---------|
| 0 | No pattern |
| 100 | Rickshaw Man detected — neutral indecision signal, not a directional (bullish/bearish) bias |

## Aliases

Rickshaw Man

## See Also

CDLLONGLEGGEDDOJI · CDLDOJI · CDLHIGHWAVE
