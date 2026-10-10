---
title: TA-Lib
description: "Open-source technical analysis library: 200+ indicators (ADX, MACD, RSI), candlestick patterns, batch and streaming. Native C/C++, Java, C# and Rust, plus Python and R wrappers."
toc: false
---

<div align="center" class="home-hero">

<img src="/assets/images/home.png" alt="TA-Lib">

### Technical analysis, battle-tested since 2001

</div>

- 200+ indicators such as ADX, MACD, RSI, Stochastic, Bollinger Bands etc... [See complete list...](/functions/)
- Candlestick patterns recognition
- Batch and streaming API for every function: compute a whole array, or update one bar at a time without recomputing the history.
- Native implementation in [C/C++](/api/), [Java](/api/java/), [C#](/api/csharp/) and [Rust](/api/rust/). The Java, C# and Rust ones do not need the C library.
- Made for integration and glue-code friendly. Function metadata drives your UI, parameter range, automation and generated bindings; the [Abstract API](/api/abstract/) calls any function by name. New functions show up with an upgrade, with no code change.
- Wrappers for Python, R, and [more](/install/#wrappers).
- Open-Source (BSD License). Can be freely integrated in your own open-source or commercial applications.

TA-Lib implements standard technical analysis algorithms used across the industry — stable, well-tested, and production-proven.

## Example

A 30-bar simple moving average, over an array and then on a live feed.

<!-- @include: ./.vuepress/includes/sma-example.md -->
