---
url: 'https://ta-lib.org/api/candlestick-outputs/index.md'
description: >-
  What the numbers returned by the CDL* candlestick pattern functions mean: 0
  for no pattern, 100 or -100 for a pattern, and the rarer 80 and 200.
---
# Candlestick Outputs

**TL;DR:** 0 means no pattern. 100 means the pattern is on this bar. A minus sign means bearish.

## What the values mean

Every candlestick function (`CDL*`) returns one whole number per bar:

| Value | Meaning |
|-------|---------|
| 0 | No pattern on this bar |
| 100 | Pattern found, bullish |
| -100 | Pattern found, bearish |

That is all most patterns return. A few add one more level:

| Value | Meaning | For example |
|-------|---------|-------------|
| 80 or -80 | A weaker form of the pattern | [CDLENGULFING](/functions/cdlengulfing), [CDLHARAMI](/functions/cdlharami) |
| 200 or -200 | This bar confirms a pattern found a few bars earlier | [CDLHIKKAKE](/functions/cdlhikkake) |

Some patterns point only one way, so they return only one sign: a hammer is always 100, a shooting star always -100. A few have no direction: a doji is just 100 when found, and a spinning top's sign is only the candle's color. Each function's page lists its values under **Output Values**.

A value describes the candles, not a trade. A pattern does not check the trend before it, so whether it is a signal is up to you.

## Reading them from code

You do not have to hard-code any of this. Through the [Abstract API](/api/abstract/), each candlestick output carries flags that say which of these values it can return and what its sign means, so a charting app or a strategy builder can find out at run time.

## Related pages

* [Abstract API](/api/abstract/): look up any function and its output flags
* [Output flags](/spec/abstract/#flags): every flag, one line each
* [Pattern outputs](/spec/inputs-outputs/#rw7): the exact rules
* [Candlestick Settings](/api/candle-settings/): tune what counts as a long body or a short shadow
