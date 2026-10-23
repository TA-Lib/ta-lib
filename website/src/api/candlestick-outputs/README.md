---
title: Candlestick Outputs
description: "What the numbers returned by the CDL* candlestick pattern functions mean: 0 for no pattern, positive for bullish, negative for bearish."
toc: false
---

**TL;DR:** 0 means no pattern. A positive value means a bullish pattern, a negative value a bearish one.

## What the values mean

Every candlestick function (`CDL*`) returns one whole number per bar. Read the sign first: zero, positive or negative. The size adds detail:

| Value | Meaning |
|-------|---------|
| 0 | No pattern on this bar |
| +/-100 | The usual case: the pattern is on this bar |
| +/-80 | A weaker form of the pattern, for example [CDLENGULFING](/functions/cdlengulfing) |
| +/-200 | This bar confirms a pattern found a few bars earlier, for example [CDLHIKKAKE](/functions/cdlhikkake) |

Most patterns only ever return 0 and +/-100.

Some patterns point only one way, so they return only one sign: a hammer is always 100, a shooting star always -100. A few have no direction: a doji is just 100 when found, and a spinning top's sign is only the candle's color. Each function's page lists its values under **Output Values**.

A value describes the candles, not a trade. A pattern does not check the trend before it, so whether it is a signal is up to you.

## Reading them from code

You do not have to hard-code any of this. Through the [Abstract API](/api/abstract/), each candlestick function carries flags that say which of these values it can return, so a charting app or a strategy builder can find out at run time.

## Related pages

- [Abstract API](/api/abstract/): look up any function and its flags
- [Output flags](/spec/abstract/#flags): every flag, one line each
- [Specification](/spec/inputs-outputs/#rw7): the exact rules
- [Candlestick Settings](/api/candle-settings/): tune what counts as a long body or a short shadow
