# CDLTHRUSTING

## Summary

A two-candle pattern: a long black candle followed by a white candle that opens below the prior low and closes back into the prior body but below its midpoint. It is a bearish continuation signal. A hit is bearish: the failed white push back into the black body signals continuation of the down move.

## Notes

- The pattern is classically meaningful only in a downtrend, but this function does not verify any prior trend.
- Although the pattern can be read as bullish in an uptrend or when it recurs, this function ignores trend and always reports it as bearish.
- Bulkowski's testing found this classically-bearish continuation pattern actually acts as a bullish reversal 57% of the time — "near random" — though it ranks a strong 15th of 103 patterns for overall performance. ([thepatternsite.com](https://www.thepatternsite.com/Thrusting.html))

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
| -100 | Thrusting pattern detected: bearish continuation |
| 0 | No pattern |

## Aliases

Thrusting Pattern, Thrusting Line

## See Also

CDLINNECK · CDLONNECK · CDLPIERCING · CDLMEETINGLINES
