# CDL3BLACKCROWS

## Summary

A four-bar pattern: a white candle followed by three consecutive black (down) candles with successively lower closes, each opening inside the prior black's real body. It is a bearish reversal signal. A hit (-100) signals a bearish reversal.

## Notes

- Does not verify the prior mature uptrend the pattern classically assumes for significance.
- Rare: a few years of one symbol's daily bars may show none. Bulkowski ranks it 60th of 103 patterns by how often it occurs ([Three Black Crows](https://thepatternsite.com/ThreeBlackCrows.html)), and TA-Lib finds it 13 times in [about 16,000 daily crypto bars](/api/candlestick-outputs/#how-often-patterns-fire).

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
