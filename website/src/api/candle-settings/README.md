---
title: Candlestick Settings
toc: false
---

# Candlestick Settings

Part of the [C/C++ Core API](/api/). The candlestick pattern functions (`TA_CDL*`) judge each candle — is its body "long", its shadow "short", two candles "near" — relative to a set of tunable thresholds. These settings control those judgements globally.

## API

```c
TA_RetCode TA_SetCandleSettings( TA_CandleSettingType settingType,
                                 TA_RangeType         rangeType,
                                 int                  avgPeriod,
                                 double               factor );

TA_RetCode TA_RestoreCandleDefaultSettings( TA_CandleSettingType settingType );
```

A candle characteristic is measured against an average of a chosen range over the previous `avgPeriod` bars, scaled by `factor`. For each `settingType`:

- **`rangeType`** — what to measure: `TA_RangeType_RealBody` (open-to-close), `TA_RangeType_HighLow` (high-to-low), or `TA_RangeType_Shadows` (the two shadows).
- **`avgPeriod`** — how many prior bars to average (`0` means "use only the current candle", no averaging).
- **`factor`** — the multiplier applied to that average to form the threshold.

`TA_RestoreCandleDefaultSettings` reverts one setting (or `TA_AllCandleSettings` for every one) to the built-in default below.

```c
/* Treat a "long body" as 1.2x the average real body of the last 10 candles: */
TA_SetCandleSettings( TA_BodyLong, TA_RangeType_RealBody, 10, 1.2 );

/* ...later, restore the default for that one setting: */
TA_RestoreCandleDefaultSettings( TA_BodyLong );
```

Choose candle settings **once, from a single thread**, before making concurrent calls (see [multi-threading](/api/#multithreading)). They stay in effect until changed or restored.

## Setting types and defaults

`TA_CandleSettingType` values, with the defaults `TA_Initialize` installs:

| Setting            | Range type | avgPeriod | factor |
|--------------------|------------|-----------|--------|
| `TA_BodyLong`        | RealBody | 10 | 1.0  |
| `TA_BodyVeryLong`    | RealBody | 10 | 3.0  |
| `TA_BodyShort`       | RealBody | 10 | 1.0  |
| `TA_BodyDoji`        | HighLow  | 10 | 0.1  |
| `TA_ShadowLong`      | RealBody | 0  | 1.0  |
| `TA_ShadowVeryLong`  | RealBody | 0  | 2.0  |
| `TA_ShadowShort`     | Shadows  | 10 | 1.0  |
| `TA_ShadowVeryShort` | HighLow  | 10 | 0.1  |
| `TA_Near`            | HighLow  | 5  | 0.2  |
| `TA_Far`             | HighLow  | 5  | 0.6  |
| `TA_Equal`           | HighLow  | 5  | 0.05 |

`TA_AllCandleSettings` targets every setting at once (for `TA_RestoreCandleDefaultSettings`).

## See also

- [Initialize and Shutdown](/api/#init)
- [Unstable Period](/api/unstable-period/)
- The candlestick pattern functions in the [function reference](/functions/) (the `Pattern Recognition` group).
