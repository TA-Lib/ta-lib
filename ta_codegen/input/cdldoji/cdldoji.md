# CDLDOJI

## Summary

Single-candle Doji recognizer: fires when the real body (|close-open|) is at or below the BodyDoji threshold. Market indecision; neither bullish nor bearish on its own.

## Formula

match if $|close-open| \le \text{CandleAverage(BodyDoji)}$

## Inputs

- `inOpen` — Open price of each bar
- `inHigh` — High price of each bar
- `inLow` — Low price of each bar
- `inClose` — Close price of each bar

## Outputs

- `outInteger` — 100 when a doji is detected, else 0

## Output Values

| Value | Meaning |
|-------|---------|
| 0 | No doji |
| 100 | Doji detected — market indecision; neither bullish nor bearish on its own |

## Aliases

Doji

## See Also

CDLDOJISTAR · CDLDRAGONFLYDOJI · CDLGRAVESTONEDOJI · CDLLONGLEGGEDDOJI
