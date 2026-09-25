# CDLHIKKAKE

## Summary

A 3-bar pattern: an inside bar followed by a false breakout, optionally later confirmed by a follow-through bar. Signals a bullish or bearish reversal/continuation depending on the breakout direction. A false-breakout setup: positive = bullish, negative = bearish; magnitude 200 flags the confirming bar.

## Notes

- The name comes from the Japanese word for a deceptive move or "trap" — fitting, since the pattern exists to catch traders acting on a false breakout. Bulkowski's testing of the confirmed pattern found the trap itself barely beats a coin flip: the bullish variant continues as expected only 52% of the time and the bearish variant exactly 50% ("random"), both ranking in the bottom fifth (83rd-84th of 105) for post-breakout performance. ([thepatternsite.com](https://thepatternsite.com/HikkakeBull.html))

## Inputs

- `inOpen` — Open price of each bar
- `inHigh` — High price of each bar
- `inLow` — Low price of each bar
- `inClose` — Close price of each bar

## Outputs

- `outInteger` — +100/-100 at the hikkake (breakout) bar for bull/bear; +200/-200 at a later confirmation bar; 0 otherwise

## Output Values

| Value | Meaning |
|-------|---------|
| -200 | Bearish Hikkake confirmed — price breaks down through the setup's low within 3 bars, validating the trap and the move lower |
| -100 | Bearish Hikkake — a false upside breakout from a tight, inside-bar range traps buyers before price turns back down |
| 0 | No pattern, and no trap awaiting confirmation |
| 100 | Bullish Hikkake — a false downside breakout from a tight, inside-bar range traps sellers before price turns back up |
| 200 | Bullish Hikkake confirmed — price breaks up through the setup's high within 3 bars, validating the trap and the move higher |

## Aliases

Hikkake Pattern, Hikkake

## See Also

CDLHIKKAKEMOD · CDLHARAMI
