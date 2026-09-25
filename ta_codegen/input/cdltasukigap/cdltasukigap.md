# CDLTASUKIGAP

## Summary

A three-candle pattern: a real-body-gapping candle followed by an opposite-color candle that opens inside its body and closes back into the gap without filling it. An upside gap is a bullish continuation signal; a downside gap is a bearish continuation signal.

## Notes

- This continuation pattern does not verify the prior trend it classically assumes; the caller must confirm the trend.
- Bulkowski's testing found the downside Tasuki Gap actually acts as a bullish REVERSAL 54% of the time — opposite its textbook bearish-continuation label — while the upside variant does continue as labeled, but only 57% of the time ("near random"). ([thepatternsite.com](https://thepatternsite.com/DownsideTasukiGap.html))

## Inputs

- `inOpen` — Open price of each bar
- `inHigh` — High price of each bar
- `inLow` — Low price of each bar
- `inClose` — Close price of each bar

## Outputs

- `outInteger` — +100 on a bullish (upside-gap) tasuki gap, -100 on a bearish (downside-gap) tasuki gap, 0 otherwise. Sign equals the color of the gap candle i-1 (candlecolor(i-1)*100)

## Output Values

| Value | Meaning |
|-------|---------|
| -100 | Bearish (downside-gap) Tasuki Gap |
| 0 | No pattern |
| 100 | Bullish (upside-gap) Tasuki Gap |

## Aliases

Tasuki Gap, Upside/Downside Tasuki Gap

## See Also

CDLGAPSIDESIDEWHITE · CDLXSIDEGAP3METHODS
