# CDLDARKCLOUDCOVER

## Summary

A two-candle bearish reversal pattern: a long white candle followed by a black candle that opens above the prior high and closes deep into the prior white body past a penetration threshold. Signals a potential top. A hit (-100) is a bearish reversal signal, most meaningful after an uptrend.

## Notes

- Does not verify the preceding uptrend the bearish reversal classically assumes.

## Inputs

- `inOpen` — Open price of each bar
- `inHigh` — High price of each bar
- `inLow` — Low price of each bar
- `inClose` — Close price of each bar

## Outputs

- `outInteger` — -100 when the pattern is detected (always bearish), 0 otherwise; never emits +100

## Output Values

| Value | Meaning |
|-------|---------|
| -100 | Dark Cloud Cover pattern detected: bearish |
| 0 | No pattern |

## Parameters

- `optInPenetration` — Fraction of candle 1's real body that candle 2's close must penetrate below close[i-1]; larger values require deeper penetration

## Aliases

Dark Cloud Cover

## See Also

CDLPIERCING · CDLENGULFING · CDLONNECK
