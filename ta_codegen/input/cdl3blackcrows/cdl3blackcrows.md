# CDL3BLACKCROWS

## Summary

A four-bar pattern: a white candle followed by three consecutive black (down) candles with successively lower closes, each opening inside the prior black's real body. It is a bearish reversal signal. A hit (-100) signals a bearish reversal.

## Notes

- Does not verify the prior mature uptrend the pattern classically assumes for significance.

## Inputs

- `inOpen` — Open price of each bar
- `inHigh` — High price of each bar
- `inLow` — Low price of each bar
- `inClose` — Close price of each bar

## Outputs

- `outInteger` — -100 when the bearish pattern is detected, 0 otherwise. Never emits +100

## Output Values

| Value | Meaning |
|-------|---------|
| -100 | Three Black Crows pattern detected: bearish |
| 0 | No pattern |

## Aliases

Three Black Crows, 3 Black Crows

## See Also

CDL3WHITESOLDIERS · CDLIDENTICAL3CROWS · CDLADVANCEBLOCK
