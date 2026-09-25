# CDLHIKKAKEMOD

## Summary

A four-candle pattern: two successively narrower inside bars, then a breakout bar, with the second candle closing near one extreme of its range. Bullish or bearish reversal signal. Bullish (+) or bearish (-) reversal; per the code's note it is significant in a downtrend (bull) or uptrend (bear), context the code does not verify.

## Notes

- Does not verify the prior trend (downtrend for bullish, uptrend for bearish) that this reversal pattern assumes.

## Inputs

- `inOpen` — Open price of each bar
- `inHigh` — High price of each bar
- `inLow` — Low price of each bar
- `inClose` — Close price of each bar

## Outputs

- `outInteger` — +100 bullish hikkake bar, -100 bearish; +200 confirmed bullish, -200 confirmed bearish (confirmation adds another +/-100); 0 otherwise

## Output Values

| Value | Meaning |
|-------|---------|
| -200 | Bearish Modified Hikkake confirmed — price breaks down through the setup's low within 3 bars, confirming the reversal lower |
| -100 | Bearish Modified Hikkake — a false upside breakout traps buyers, warning that the uptrend may be topping out |
| 0 | No pattern, and no trap awaiting confirmation |
| 100 | Bullish Modified Hikkake — a false downside breakout traps sellers, warning that the downtrend may be bottoming out |
| 200 | Bullish Modified Hikkake confirmed — price breaks up through the setup's high within 3 bars, confirming the reversal higher |

## Aliases

Modified Hikkake, Modified Hikkake Pattern

## See Also

CDLHIKKAKE
