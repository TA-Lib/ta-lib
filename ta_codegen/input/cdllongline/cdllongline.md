# CDLLONGLINE

## Summary

A single-candle pattern: a long real body with short upper and short lower shadow. The signal direction follows the candle color (bullish if white, bearish if black). Signals strong directional conviction on the bar. Not intrinsically a reversal or continuation signal.

## Inputs

- `inOpen` — Open price of each bar
- `inHigh` — High price of each bar
- `inLow` — Low price of each bar
- `inClose` — Close price of each bar

## Outputs

- `outInteger` — +100 on a white (close>=open) long line, -100 on a black long line, 0 when no pattern

## Output Values

| Value | Meaning |
|-------|---------|
| -100 | Black long-line candle (long body, short shadows) |
| 0 | No pattern |
| 100 | White long-line candle (long body, short shadows) |

## Aliases

Long Line Candle, Long Line

## See Also

CDLSHORTLINE · CDLCLOSINGMARUBOZU · CDLMARUBOZU · CDLLONGLEGGEDDOJI
