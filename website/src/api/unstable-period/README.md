---
title: Unstable Period
description: "How many warm-up bars TA-Lib discards from recursive indicators such as EMA, RSI and ADX before reporting their output, and how to set it in C and Rust."
toc: false
---

Some indicators need a warm-up before their output settles. The *unstable period* setting controls how many of those warm-up bars TA-Lib discards instead of reporting them.

## Why it exists

Some indicators have "memory" — each output depends on the previous one, seeded from the start of the data. An Exponential Moving Average is the classic example: its value at a given bar depends on every bar before it. In practice the influence of the earliest bars decays quickly, so the result becomes **stable** after enough bars.

This is inherent to the algorithms, not something specific to TA-Lib — every implementation has to seed the recursion somewhere, and the earliest outputs are distorted by that seed. What TA-Lib adds is the ability to scrub those early values on every function call, so unstable data never gets injected into your application.

## What to do

There are three distinct approaches, from the most common to the most rigorous:

1. **Ignore the problem.** This is what most users and most charting sites do, and it is relatively OK in practice: most people focus on the most recent bar, which by then has enough history behind it that the value has long since stabilized. The weakness is that nothing warns you when the assumption stops holding — a short series, or a back-test that acts on the earliest bars, will quietly use values that are off.

2. **Provide extra history.** Fetch more bars than you intend to use and treat the leading outputs as throwaway, so that even the first bar you actually act on has stabilized. This is application-level scrubbing: TA-Lib is left at its default (unstable period `0`) and returns everything it can compute, while your code chooses what to drop.

3. **Have TA-Lib drop the unstable data.** Set an unstable period and TA-Lib strips that many extra bars from the front of the output, on top of the function's normal lookback: `outBegIdx` moves forward and those unstable outputs never reach your code.

## API

`id` selects which function to affect. The period sets how many warm-up bars that
function discards — the larger the value, the later the first output. The default,
`0`, discards nothing: you get every value the function can compute.

The setting follows the function wherever it runs: whether you call it directly, or
another indicator uses it internally. The EMA id therefore affects EMA itself and
every indicator built on one, such as MACD and DEMA.

::: code-tabs#lang

@tab C

```c
TA_RetCode   TA_SetUnstablePeriod( TA_FuncUnstId id, unsigned int unstablePeriod );
unsigned int TA_GetUnstablePeriod( TA_FuncUnstId id );

/* Strip 30 extra bars from every EMA-based calculation: */
TA_SetUnstablePeriod( TA_FUNC_UNST_EMA, 30 );

/* Apply the same unstable period to ALL affected functions at once: */
TA_SetUnstablePeriod( TA_FUNC_UNST_ALL, 30 );
```

Ids are spelled `TA_FUNC_UNST_<NAME>`.

Being a global, choose the unstable period **once, from a single thread**, before
making concurrent calls (see [multi-threading](/api/#multithreading)).

@tab Rust

```rust
use ta_lib::{Core, FuncUnstId};

// Strip 30 extra bars from every EMA-based calculation:
let core = Core::builder()
    .unstable_period(FuncUnstId::EMA, 30)
    .build();

// Apply the same unstable period to ALL affected functions at once:
let core = Core::builder()
    .unstable_period(FuncUnstId::ALL, 30)
    .build();

let n = core.get_unstable_period(FuncUnstId::EMA);   // read it back
```

Ids are spelled `FuncUnstId::<NAME>`, so `TA_FUNC_UNST_HT_DCPERIOD` is
`FuncUnstId::HT_DCPERIOD`.

There is no global to guard here: the period is fixed when the [`Core`](/api/rust/)
is built and cannot change afterwards, which is what makes a `Core` `Send + Sync`.
To use a different period, build another `Core` (or derive one with `to_builder()`).

:::

## Functions with an unstable period

These are the functions with an unstable period. Every binding covers the same set
and spells each id the same way; C alone prefixes it with `TA_FUNC_UNST_`. Each
language also has a wildcard that targets all of them at once.

<!-- ta_codegen:begin unstable-func-list -->
`ADX`, `ATR`, `CMO`, `DX`, `EMA`, `HT_DCPERIOD`, `HT_DCPHASE`, `HT_PHASOR`, `HT_SINE`, `HT_TRENDLINE`, `HT_TRENDMODE`, `KAMA`, `MAMA`, `MINUS_DI`, `MINUS_DM`, `NATR`, `PLUS_DI`, `PLUS_DM`, `RSI`, `T3`.
<!-- ta_codegen:end unstable-func-list -->

The C enumeration is in [ta_defs.h](https://github.com/TA-Lib/ta-lib/blob/main/include/ta_defs.h).

## See also

- [C/C++ Core API](/api/) / [Rust Core API](/api/rust/)
- [Candlestick Settings](/api/candle-settings/)
