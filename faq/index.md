---
url: 'https://ta-lib.org/faq/index.md'
description: >-
  Whether TA-Lib is still maintained (yes, actively again since 2025), whether
  it works on a live feed, whether the Rust, Java, C# and TypeScript libraries
  need the C library, and where to get support.
---
# FAQ

## Is TA-Lib maintained?

Yes, and more actively than in years. Development resumed in 2025 after a quiet decade (2014 to 2024), adding automated releases, a streaming API, and native Rust, Java, C# and TypeScript libraries. The classic C library stays as reliable as ever.

## Does TA-Lib work on a live feed?

Yes. Every function has a streaming form: open it on the history, then update it once per bar, without recomputing the history. See the streaming page for [C/C++](/api/stream/), [Rust](/api/rust/stream/), [Java](/api/java/stream/), [C#](/api/csharp/stream/) or [TypeScript](/api/typescript/stream/).

## Can the same code serve a backtest and live trading?

Yes. A stream fed bar by bar returns the values the batch function returns over the same bars, except that a value depending on a [transcendental function](/spec/versions/#transcendental) may differ in the last bits.

## What do the candlestick values mean?

0 means no pattern, a positive value a bullish pattern and a negative value a bearish one. The usual values are +/-100. See [Candlestick Outputs](/api/candlestick-outputs/).

## Why does a candlestick pattern never fire on my data?

Many patterns are rare; see [How often patterns fire](/api/candlestick-outputs/#how-often-patterns-fire).

## Do the Rust, Java, C# and TypeScript libraries need the C library?

No. Each is a native implementation in its own language, with no binding to C.

## How to get support?

Various ways:

* Open a discussion on [Discord](https://discord.com/invite/Erb6SwsVbH)
* Open a [Github Issue](https://github.com/TA-Lib/ta-lib/issues)
* Check these communities:
  * <https://github.com/ta-lib/ta-lib-python>
