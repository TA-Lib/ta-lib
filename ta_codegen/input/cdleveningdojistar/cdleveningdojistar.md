# CDLEVENINGDOJISTAR

## Summary

A three-candle bearish reversal pattern: a long white candle, a doji that gaps up (the star), then a black candle closing well down into the first candle's body. A stricter Evening Star whose middle candle must be a doji. Hit (-100) signals a bearish top reversal.

## Notes

- Does not verify the preceding uptrend the bearish reversal classically assumes.

## Inputs

- `inOpen` — Open price of each bar
- `inHigh` — High price of each bar
- `inLow` — Low price of each bar
- `inClose` — Close price of each bar

## Outputs

- `outInteger` — -100 when the pattern is detected, 0 otherwise. Always bearish; never emits +100

## Output Values

| Value | Meaning |
|-------|---------|
| -100 | Evening Doji Star pattern detected: bearish |
| 0 | No pattern |

## Parameters

- `optInPenetration` — Fraction of the 1st real body the 3rd candle's close must penetrate; larger demands a deeper close into the first body

## Aliases

Evening Doji Star

## See Also

CDLEVENINGSTAR · CDLMORNINGDOJISTAR · CDLDOJISTAR · CDLABANDONEDBABY
