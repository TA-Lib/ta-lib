# CDL3STARSINSOUTH

## Summary

A three-candle bullish reversal pattern of three consecutive black candles that progressively shrink and stabilize: a long black candle with a long lower shadow, a smaller black candle probing lower, then a small black marubozu contained within the second candle's range. A hit signals a bullish reversal; per the code comment it is meaningful in a downtrend, but the function does not verify prior trend.

## Notes

- Does not verify the prior downtrend the pattern classically assumes for significance.
- Thomas Bulkowski's statistical study found this has the best reversal rate of the 103 candlestick patterns he tracked (86% bullish reversal) — but that rests on just 9 occurrences in 4.7 million candle lines, and its overall post-breakout performance ranks dead last, 103rd of 103. ([thepatternsite.com](https://thepatternsite.com/ThreeStarsSouth.html))

## Inputs

- `inOpen` — Open price of each bar
- `inHigh` — High price of each bar
- `inLow` — Low price of each bar
- `inClose` — Close price of each bar

## Outputs

- `outInteger` — +100 on the bar where the pattern completes (always bullish), 0 otherwise. Never emits -100

## Output Values

| Value | Meaning |
|-------|---------|
| 0 | No pattern |
| 100 | Three Stars In The South detected — a bullish bottom-reversal signal, most meaningful after a downtrend (unverified by the function) |

## Aliases

Three Stars In The South

## See Also

CDL3BLACKCROWS · CDLIDENTICAL3CROWS · CDL3WHITESOLDIERS
