# CDLTAKURI

## Summary

Single-candle pattern: a doji whose open and close sit at the high (no/very short upper shadow) with a very long lower shadow, i.e. a dragonfly doji with an exceptionally long lower shadow. Emitted as a positive signal, but its directional meaning depends on the prevailing trend, which the code does not check. A hit marks a takuri (dragonfly-doji) line; a potential reversal only when read against the trend (typically a bottom/bullish reversal after a downtrend), which the code itself does not verify.

## Inputs

- `inOpen` — Open price of each bar
- `inHigh` — High price of each bar
- `inLow` — Low price of each bar
- `inClose` — Close price of each bar

## Outputs

- `outInteger` — +100 when the takuri pattern is detected, 0 otherwise. Never negative; the positive sign is a convention and does not by itself imply bullishness

## Output Values

| Value | Meaning |
|-------|---------|
| 0 | No pattern |
| 100 | Takuri (dragonfly doji with an exceptionally long lower shadow) detected — directional meaning depends on the prevailing trend, which the code does not check |

## Aliases

Takuri, Takuri line

## See Also

CDLDRAGONFLYDOJI · CDLDOJI · CDLHAMMER · CDLGRAVESTONEDOJI
