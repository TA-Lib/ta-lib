---
url: 'https://ta-lib.org/api/candlestick-outputs/index.md'
description: >-
  What the CDL* candlestick pattern functions return: 0 for no pattern,
  otherwise +/-100, +/-80 for a weaker form or +/-200 for a confirmation, the
  sign giving the direction where the pattern has one, the flags that tell a
  program which values each function returns, and how often the rare patterns
  fire.
---
# Candlestick Outputs

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

## How often patterns fire

Many patterns are rare: a few years of one symbol's daily bars may show none, and that is not a bug. The page of each rare pattern says so under **Notes**:

* **Bulkowski's frequency rank.** Thomas Bulkowski ranks the 103 patterns he tested, over about 4.7 million candles, by how often each occurs, 1 being the most frequent. The note links his page for the pattern on [thepatternsite.com](https://thepatternsite.com/CandleEntry.html). His definitions are his own, so TA-Lib can find a pattern more or less often than his rank suggests.
* **How often TA-Lib's function fires** over about 16,000 daily bars of crypto, May 2021 to November 2026: eight trading pairs on Uniswap v3, Ethereum (WBTC/USDC, WETH/USDC, WETH/USDT, WBTC/WETH, LINK/WETH, UNI/WETH, AAVE/WETH, MKR/WETH), built from the pools' on-chain swaps. Crypto trades around the clock, so a day seldom opens away from the previous close, and a pattern that needs a gap between whole candles almost never fires there.

A pattern counts as rare when Bulkowski ranks either of its forms 78th to 103rd, or when its function fires fewer than 16 times in those bars (1 per 1,000).

## Related pages

* [Abstract API](/api/abstract/): look up any function and its flags
* [Output flags](/spec/abstract/#flags): every flag, one line each
* [Specification](/spec/inputs-outputs/#rw7): the exact rules
* [Candlestick Settings](/api/candle-settings/): tune what counts as a long body or a short shadow
