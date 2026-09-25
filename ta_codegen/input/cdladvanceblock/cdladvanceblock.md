# CDLADVANCEBLOCK

## Summary

Three-candle bearish reversal pattern: three white candles with consecutively higher closes whose advance weakens (progressively smaller bodies and/or lengthening upper shadows). Signals that an uptrend's advance is being blocked. A hit (-100) is bearish: the advance is stalling/blocked; meaningful mainly within an existing uptrend.

## Notes

- Does not verify the prior uptrend the pattern classically assumes for significance.
- Although classically read as a bearish reversal, Bulkowski's testing found the Advance Block actually acts as a bullish continuation 64% of the time. ([thepatternsite.com](https://thepatternsite.com/AdvanceBlock.html))

## Inputs

- `inOpen` — Open price of each bar
- `inHigh` — High price of each bar
- `inLow` — Low price of each bar
- `inClose` — Close price of each bar

## Outputs

- `outInteger` — -100 on a detected pattern (always bearish), 0 otherwise; never emits +100

## Output Values

| Value | Meaning |
|-------|---------|
| -100 | Advance Block pattern detected: bearish |
| 0 | No pattern |

## Aliases

Advance Block

## See Also

CDL3WHITESOLDIERS · CDLDELIBERATION · CDLSTALLEDPATTERN
