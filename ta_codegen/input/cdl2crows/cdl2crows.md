# CDL2CROWS

## Summary

Three-candle bearish reversal pattern: a long white candle, then a black candle gapping up, then a black candle that opens inside the second body and closes down inside the first white body. A hit (-100) signals a bearish reversal; significant in an uptrend, which this function does not verify.

## Notes

- Does not verify the prior uptrend the pattern classically assumes for significance.
- Bulkowski's testing found this reverses bearishly only 54% of the time — "near random" — despite the pattern's classic always-bearish label; the breakout direction cannot be predicted with any real accuracy. ([thepatternsite.com](https://thepatternsite.com/TwoCrows.html))

## Inputs

- `inOpen` — Open price of each bar
- `inHigh` — High price of each bar
- `inLow` — Low price of each bar
- `inClose` — Close price of each bar

## Outputs

- `outInteger` — -100 on a detected pattern (always bearish), 0 otherwise. Never emits +100

## Output Values

| Value | Meaning |
|-------|---------|
| -100 | Two Crows pattern detected: bearish |
| 0 | No pattern |

## Aliases

Two Crows

## See Also

CDLUPSIDEGAP2CROWS · CDLIDENTICAL3CROWS
