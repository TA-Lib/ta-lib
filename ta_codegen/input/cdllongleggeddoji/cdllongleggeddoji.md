# CDLLONGLEGGEDDOJI

## Summary

Single-candle doji (open ~ close) with at least one long shadow. Signals market indecision, not a directional bias. Marks indecision/uncertainty; not inherently bullish or bearish despite the positive sign.

## Formula

One candle. Hit when: real body <= BodyDoji average (doji body) AND (lower shadow > ShadowLong average OR upper shadow > ShadowLong average), i.e. at least one long shadow.

## Notes

- Only one long shadow (upper or lower) is required, whereas the classic pattern shows both long upper and lower shadows.
- Bulkowski's testing found this continues in the direction of the prior trend only 51% of the time — statistically random — and ranks 37th of 103 patterns overall; in his words, "it means nothing." ([thepatternsite.com](https://thepatternsite.com/LongLegDoji.html))

## Inputs

- `inOpen` — Open price of each bar
- `inHigh` — High price of each bar
- `inLow` — Low price of each bar
- `inClose` — Close price of each bar

## Outputs

- `outInteger` — +100 when the pattern is present, 0 otherwise. Only +100 is emitted; the code never emits -100, and the positive sign does NOT mean bullish

## Output Values

| Value | Meaning |
|-------|---------|
| 0 | No pattern |
| 100 | Long-legged doji detected — signals market indecision, not a directional (bullish/bearish) bias |

## Aliases

Long Legged Doji

## See Also

CDLDOJI · CDLGRAVESTONEDOJI · CDLDRAGONFLYDOJI · CDLRICKSHAWMAN
