# CDLUPSIDEGAP2CROWS

## Summary

A three-candle bearish reversal pattern: a long white candle, then a small black candle gapping up (a gap between the real bodies), then a black candle that engulfs the second candle's real body but still closes above the first candle's close. Signals a bearish reversal. A hit (-100) is a bearish reversal signal, most meaningful in an uptrend.

## Notes

- The pattern classically assumes a prior uptrend, but this function does not verify any trend.
- Although classically a bearish reversal, Bulkowski's testing found this actually acts as a bullish continuation 60% of the time, and even when it does work "the price move is often lousy." ([thepatternsite.com](https://www.thepatternsite.com/UpGapTwoCrows.html))
- Rare: a few years of one symbol's daily bars may show none. Bulkowski ranks it 75th of 103 patterns by how often it occurs ([Upside Gap Two Crows](https://thepatternsite.com/UpGapTwoCrows.html)), and TA-Lib finds none in [about 16,000 daily crypto bars](/api/candlestick-outputs/#how-often-patterns-fire).

## Inputs

- `inOpen` — Open price of each bar
- `inHigh` — High price of each bar
- `inLow` — Low price of each bar
- `inClose` — Close price of each bar

## Outputs

- `outInteger` — -100 on a pattern bar, 0 otherwise. Bearish-only: this pattern never emits +100

## Output Values

| Value | Meaning |
|-------|---------|
| -100 | Upside Gap Two Crows pattern detected: bearish |
| 0 | No pattern |

## Aliases

Upside Gap Two Crows

## See Also

CDL2CROWS · CDLGAPSIDESIDEWHITE
