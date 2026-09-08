---
title: Candlestick Settings
description: "Tune the thresholds the CDL* pattern functions judge candles against: body length, shadows, near-equal candles. Defaults, and how to set them in C, Rust, Java and C#."
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

@tab Rust

```rust
use ta_lib::{CandleSetting, CandleSettingType, Core, RangeType};

// Treat a "long body" as 1.2x the average real body of the last 10 candles:
let core = Core::builder()
    .candle_setting(
        CandleSettingType::BodyLong,
        CandleSetting { range_type: RangeType::RealBody, avg_period: 10, factor: 1.2 },
    )
    .build()?;
```

@tab Java

```java
import io.github.talib.CandleSettingType;
import io.github.talib.Core;
import io.github.talib.RangeType;

// Treat a "long body" as 1.2x the average real body of the last 10 candles:
Core core = Core.builder()
    .candleSetting(CandleSettingType.BodyLong, RangeType.RealBody, 10, 1.2)
    .build();

// ...later, restore the default for that one setting:
Core restored = core.toBuilder()
    .restoreCandleDefault(CandleSettingType.BodyLong)
    .build();
```

@tab C\#

```csharp
using TALib;

// Treat a "long body" as 1.2x the average real body of the last 10 candles:
Core core = Core.Builder()
    .CandleSetting(CandleSettingType.BodyLong, RangeType.RealBody, 10, 1.2)
    .Build();

// ...restore one setting, or every one with AllCandleSettings:
Core restored = core.ToBuilder()
    .RestoreCandleDefault(CandleSettingType.BodyLong)
    .Build();
```

:::

## Setting types and defaults

The setting types, with the defaults every binding starts from. C spells them
`TA_BodyLong`; Rust spells them `CandleSettingType::BodyLong`; Java and C# spell
them `CandleSettingType.BodyLong`.

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
restore call.

## See also

- [C/C++ Core API](/api/) / [Rust Core API](/api/rust/)
- [Unstable Period](/api/unstable-period/)
- The candlestick pattern functions in the [function reference](/functions/) (the `Pattern Recognition` group).
