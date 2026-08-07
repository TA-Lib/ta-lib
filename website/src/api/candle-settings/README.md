---
title: Candlestick Settings
description: "Tune the thresholds the CDL* pattern functions judge candles against: body length, shadows, near-equal candles. Defaults, and how to set them in C and Rust."
toc: false
---

The candlestick pattern functions (the `CDL*` family) judge each candle — is its body "long", its shadow "short", two candles "near" — relative to a set of tunable thresholds. These settings control those judgements.

## API

A candle characteristic is measured against an average of a chosen range over the
previous `avgPeriod` bars, scaled by `factor`. For each setting type:

- **range type** — what to measure: the real body (open-to-close), the high-to-low
  range, or the two shadows.
- **`avgPeriod`** — how many prior bars to average (`0` means "use only the current
  candle", no averaging).
- **`factor`** — the multiplier applied to that average to form the threshold.

::: code-tabs#lang

@tab C

```c
TA_RetCode TA_SetCandleSettings( TA_CandleSettingType settingType,
                                 TA_RangeType         rangeType,
                                 int                  avgPeriod,
                                 double               factor );

TA_RetCode TA_RestoreCandleDefaultSettings( TA_CandleSettingType settingType );

/* Treat a "long body" as 1.2x the average real body of the last 10 candles: */
TA_SetCandleSettings( TA_BodyLong, TA_RangeType_RealBody, 10, 1.2 );

/* ...later, restore the default for that one setting: */
TA_RestoreCandleDefaultSettings( TA_BodyLong );
```

Range types are `TA_RangeType_RealBody`, `TA_RangeType_HighLow` and
`TA_RangeType_Shadows`; setting types are `TA_BodyLong` and friends, per the table
below. `TA_RestoreCandleDefaultSettings` reverts one setting, or every one when
passed `TA_AllCandleSettings`.

Being globals, choose candle settings **once, from a single thread**, before making
concurrent calls (see [multi-threading](/api/#multithreading)). They stay in effect
until changed or restored.

@tab Rust

```rust
use ta_lib::{CandleSetting, CandleSettingType, Core};

// Treat a "long body" as 1.2x the average real body of the last 10 candles:
let core = Core::builder()
    .candle_setting(
        CandleSettingType::BodyLong,
        CandleSetting { range_type: 0, avg_period: 10, factor: 1.2 },
    )
    .build();
```

`range_type` is `0` = real body, `1` = high-to-low, `2` = shadows.

There is no restore call and none is needed: a builder starts from the defaults, so
"restoring" a setting means simply not overriding it. Settings are fixed when the
[`Core`](/api/rust/) is built, which is what makes it `Send + Sync`; to change one,
build another `Core`.

`CandleSettingType::AllCandleSettings` is a wildcard for the C restore call, not a
setting. Passing it to `candle_setting` **panics**, where C returns
`TA_BAD_PARAM` — there is no single setting for it to write.

:::

## Setting types and defaults

The setting types, with the defaults every binding starts from. C spells them
`TA_BodyLong`; Rust spells them `CandleSettingType::BodyLong`.

| Setting            | Range type | avgPeriod | factor |
|--------------------|------------|-----------|--------|
| `BodyLong`        | RealBody | 10 | 1.0  |
| `BodyVeryLong`    | RealBody | 10 | 3.0  |
| `BodyShort`       | RealBody | 10 | 1.0  |
| `BodyDoji`        | HighLow  | 10 | 0.1  |
| `ShadowLong`      | RealBody | 0  | 1.0  |
| `ShadowVeryLong`  | RealBody | 0  | 2.0  |
| `ShadowShort`     | Shadows  | 10 | 1.0  |
| `ShadowVeryShort` | HighLow  | 10 | 0.1  |
| `Near`            | HighLow  | 5  | 0.2  |
| `Far`             | HighLow  | 5  | 0.6  |
| `Equal`           | HighLow  | 5  | 0.05 |

`AllCandleSettings` targets every setting at once. It is meaningful only for C's
restore call; see the Rust tab above.

## See also

- [C/C++ Core API](/api/) / [Rust Core API](/api/rust/)
- [Unstable Period](/api/unstable-period/)
- The candlestick pattern functions in the [function reference](/functions/) (the `Pattern Recognition` group).
