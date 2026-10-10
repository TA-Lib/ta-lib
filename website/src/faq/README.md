---
title: FAQ
description: "Whether TA-Lib is still maintained (yes, actively again since 2025), whether it works on a live feed, whether the Rust, Java and C# libraries need the C library, and where to get support."
---

**Is TA-Lib maintained?**

Yes — and more actively than it has been in years!

Development slowed to a near-hibernation between 2014 and 2024. That changed in 2025: packaging was modernized with automated CI/CD releases, and new feature development is underway — including a native Rust implementation, a streaming API, and more.

The classic C library is as reliable as ever, and the project is once again moving forward.

**Does TA-Lib work on a live feed?**

Yes. Every function has a streaming form: open it on the history, then update it once per bar, without recomputing the history. See the streaming page for [C/C++](/api/stream/), [Rust](/api/rust/stream/), [Java](/api/java/stream/) or [C#](/api/csharp/stream/).

**Can the same code serve a backtest and live trading?**

Yes. A stream fed bar by bar returns the values the batch function returns over the same bars; the [specification](/spec/streaming/) lists the exceptions.

**Do the Rust, Java and C# libraries need the C library?**

No. Each is a native implementation in its own language, with no binding to C.

**How to get support?**

Various ways:

- Open a discussion on [Discord](https://discord.com/invite/Erb6SwsVbH)
- Open a [Github Issue](https://github.com/TA-Lib/ta-lib/issues)
- Check these communities:
    - [https://github.com/ta-lib/ta-lib-python](https://github.com/ta-lib/ta-lib-python)
