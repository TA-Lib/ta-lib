# CDLPIERCING

## Summary

Two-candle pattern: a long black candle followed by a long white candle that opens below the prior low and closes back above the midpoint of the prior black body. Bullish reversal signal. A hit (+100) is a bullish reversal signal.

## Notes

- A prior downtrend is not verified.

## Inputs

- `inOpen` — Open price of each bar
- `inHigh` — High price of each bar
- `inLow` — Low price of each bar
- `inClose` — Close price of each bar

## Outputs

- `outInteger` — +100 when the piercing pattern is detected; 0 otherwise. Always bullish, never emits -100

## Output Values

| Value | Meaning |
|-------|---------|
| 0 | No pattern |
| 100 | Piercing pattern detected — bullish reversal signal |

## Aliases

Piercing Pattern, Piercing Line

## See Also

CDLDARKCLOUDCOVER · CDLENGULFING · CDLMORNINGSTAR
