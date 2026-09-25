# CDLEVENINGSTAR

## Summary

A three-candle bearish reversal pattern: a long white candle, a short-bodied star gapping up, then a black candle closing well down into the first candle's body. A hit signals a bearish reversal (most significant in an uptrend).

## Notes

- Does not verify the preceding uptrend the bearish reversal classically assumes.
- The third candle only needs a body longer than short, not the full long body some definitions require.

## Inputs

- `inOpen` — Open price of each bar
- `inHigh` — High price of each bar
- `inLow` — Low price of each bar
- `inClose` — Close price of each bar

## Outputs

- `outInteger` — -100 when detected (always bearish), 0 otherwise. Never emits +100

## Output Values

| Value | Meaning |
|-------|---------|
| -100 | Evening Star pattern detected: bearish |
| 0 | No pattern |

## Parameters

- `optInPenetration` — Fraction of the 1st candle's real body the 3rd close must penetrate below the 1st close; larger requires deeper penetration

## Aliases

Evening Star

## See Also

CDLEVENINGDOJISTAR · CDLMORNINGSTAR · CDLMORNINGDOJISTAR · CDLSTARSINSOUTH
