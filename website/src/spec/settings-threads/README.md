---
title: Settings and Threads
description: "Settings in an immutable Core (Rust, Java, C#, TypeScript), shared across threads with no synchronization in Rust, Java and C#, or in a Go Core configured before goroutines share it, C's initialization and process-wide settings and when they may change, setting validation, the caller's rules across threads, and the EMA variation: how the EMA family is seeded."
---

*Part of TA-Lib's [specification](/spec/), a reference for AI-agent-driven integration.*

C keeps its settings (unstable periods, candle settings and the EMA variation) process-wide; Rust, Java, C# and TypeScript keep them in an immutable `Core`, so cores with different settings coexist in one process, and in Rust, Java and C# any number of threads can share one with no synchronization ([rT1](/spec/settings-threads/#rt1)). Go keeps them in a `Core` that its setters change, shared by goroutines once configured ([Go settings](/spec/settings-threads/#go-settings)). What each setting means: [Unstable Period](/api/unstable-period/), [Candlestick Settings](/api/candle-settings/) (with the defaults), [EMA variation](/spec/settings-threads/#ema-variation). Names per language: [names](/spec/#names). How a failure reaches the caller: [failures](/spec/#failures).

## The caller must {#caller}

- <a id="initialize"></a>**In C, initialize exactly once per process.** Call `TA_Initialize` before any other TA call. Call `TA_Shutdown` before the process exits; the library must not be used after it. Rust, Java, C#, TypeScript and Go have no lifecycle call: a `Core` is ready when constructed.
- <a id="idle-settings"></a>**In C, change a setting only while no TA function is running and no stream is open.** A change is a call to `TA_SetUnstablePeriod`, `TA_SetEMAVariation`, `TA_SetCandleSettings`, `TA_RestoreCandleDefaultSettings` or `TA_Shutdown`. Between changes, any number of threads may call the library ([rT11](/spec/settings-threads/#rt11)); it assumes the `malloc` and `free` it allocates with are thread-safe.
- <a id="one-writer"></a>**Run a writer alone on its stream handle.** The writers are `Update`, `Advance` and C's `Close`; not even `Peek` or `Value` may run alongside one. Separate handles, a clone included ([rH8](/spec/streaming/#rh8)), may each be driven on their own thread. Rust enforces this at compile time: the writing calls take `&mut self`, and every handle is `Send + Sync + Clone`. A Go stream belongs to one goroutine: no call on it may run alongside another.
- <a id="go-settings"></a>**In Go, configure a `Core` before sharing it.** Call its setters only while no other goroutine uses it and no stream opened on it is in use. Between changes, any number of goroutines may call through it.
- <a id="confine"></a>**Confine holders, builders and sinks to one thread.** A [parameter holder](/spec/abstract/#provides) is not thread-safe in C, Java and C#; neither is a Java or C# `CoreBuilder` nor the sink a Java multi-output stream writes into ([names](/spec/#names)). In C, the `const` in `TA_CallFunc`'s signature does not make a holder shareable: the call writes the output buffers bound to it.

## `Core` in Rust, Java, C# and TypeScript

<a id="rt1"></a>**rT1** In Rust, Java, C# and TypeScript, settings live on an immutable `Core` made by a builder. In Rust, Java and C#, one `Core` may be shared by any number of threads with no synchronization; Rust's is `Send + Sync`. To change a setting, build another `Core`, from defaults or seeded from an existing one (`to_builder()`, `toBuilder()`, `ToBuilder()`). A stream keeps the settings of the `Core` that opened it. A Java, C# or TypeScript builder stays usable after `build()`.

## Validation

The rows hold for every setter and getter in every language, except where a note says otherwise. A Rust or Java enum cannot hold a value outside its members.

| Rule | Condition | Language notes |
|---|---|---|
| <a id="rt2"></a>**rT2** | A setter fails on a target or a value outside its enum. `TA_SetCandleSettings`, the builders' candle setter and Go's `SetCandleSetting` also fail on `TA_AllCandleSettings`. | In Rust and Java only the wildcard fails. |
| <a id="rt3"></a>**rT3** | An unstable period is a count in `[0, TA_INDEX_MAX]` or an Auto level: `TA_UNSTABLE_AUTO_PREC_4` or `TA_UNSTABLE_AUTO_PREC_8` ([constants](/spec/#names)). | C takes an `unsigned int`, so a negative value arrives above every level and fails. Rust's `u32` cannot hold one. |
| <a id="rt4"></a>**rT4** | Reading a setting for the set-all wildcard fails. | For the wildcard or an out-of-range id, C's `TA_GetUnstablePeriod` returns 0, which is also a legal period. |
| <a id="rt5"></a>**rT5** | A candle setting's range type is a `TA_RangeType` member. | |
| <a id="rt6"></a>**rT6** | A candle setting's `avgPeriod` is in `[0, TA_INDEX_MAX]`. | |
| <a id="rt7"></a>**rT7** | A candle setting's `factor` is finite and not negative. | |
| <a id="rt8"></a>**rT8** | A failed call leaves every setting as it was, a wildcard call included. | Rust's `build()` returns the first failure, even after a later valid setter or `restore_candle_default`. |

<a id="rt9"></a>**rT9** Two wildcards act on every member at once: `TA_FUNC_UNST_ALL` sets every unstable period, and `TA_AllCandleSettings` restores every candle setting's default through `TA_RestoreCandleDefaultSettings`, the builders' restore call (`restore_candle_default`, `restoreCandleDefault`, `RestoreCandleDefault`) and Go's `RestoreCandleDefault`.

<a id="rt12"></a>**rT12** A getter returns the value the setter stored. For an unstable period on an Auto level that is the level's constant, not the number of bars it adds ([rL6](/spec/lookback/#rl6)).

## Initial state

<a id="rt10"></a>**rT10** In C, `TA_Initialize` leaves every unstable period at 0, every candle setting at its default and the EMA variation at `TA_EMAVariation_SMA_SEED`, and so do Go's `NewCore` and a zero `Core`.

## Threads in C

<a id="rt11"></a>**rT11** Between setting changes, C calls made from several threads at once, each on its own stream handles and parameter holders, return what a single thread returns. On one handle with no writer running, `Peek`, `Value`, `OutRange` and `Clone` may run from several threads at once.

## EMA variation {#ema-variation}

The EMA variation selects how the EMA family seeds its recursion: `SMA_SEED`, the default, or `FIRST_BAR_SEED`.

| Language | Select `FIRST_BAR_SEED` | Read |
|---|---|---|
| C | `TA_SetEMAVariation(TA_EMAVariation_FIRST_BAR_SEED)` | `TA_GetEMAVariation()` |
| Rust | `Core::builder().ema_variation(EMAVariation::FIRST_BAR_SEED)` | `core.ema_variation()` |
| Java | `Core.builder().emaVariation(EMAVariation.FIRST_BAR_SEED)` | `core.emaVariation()` |
| C# | `Core.Builder().EmaVariation(EMAVariation.FIRST_BAR_SEED)` | `core.EmaVariation()` |
| TypeScript | `Core.builder().emaVariation(EMAVariation.FIRST_BAR_SEED)` | `core.emaVariation()` |
| Go | `core.SetEMAVariation(talib.EMAVariationFIRST_BAR_SEED)` | `core.EMAVariation()` |

<a id="rt13"></a>**rT13** A stage is an exponential average with factor `2 / (period + 1)` that `SMA_SEED` seeds with the simple average of its first `period` inputs, other than T3's, or one of MACDFIX's two fixed-factor legs. It counts whether the function computes it itself or reaches it through a call or an MA type. `FIRST_BAR_SEED` changes the seed of every stage and nothing else: each step is the one `SMA_SEED` takes.

<a id="rt14"></a>**rT14** Under `FIRST_BAR_SEED`, a stage is seeded with one value:

- A stage fed by the function's input, or by a per-bar transform of it (EFI's force, CVI's high-low spread, ZLEMA's de-lagged price, the windowed series PSO, SMI and KSTEXT smooth, the Fast-K that STOCH, STOCHF and KDJ smooth through an MA type), takes the first value of that series in the caller's array: bar 0, or the first bar where the transform exists. It runs from there to the requested start, whatever `startIdx` is. Stages of different periods fed by the same series start on that same bar.
- A stage fed by anything else, such as another stage, DOSC's RSI, or STOCHRSI's Fast-K of RSI, takes the first value of its feeder that the call uses: the one on the bar where `SMA_SEED` starts the window it averages.

<a id="rt15"></a>**rT15** The EMA variation changes no lookback ([rL5](/spec/lookback/#rl5)), so no first output bar and no count ([rW1](/spec/inputs-outputs/#rw1)).

<a id="rt16"></a>**rT16** Under `FIRST_BAR_SEED`, a stage fed by the input is stepped from its seed bar to the requested start, so a call reads bars before `max(startIdx, lookback) - lookback`, which [rL1](/spec/lookback/#rl1) otherwise excludes, and its cost grows with `startIdx`. STOCH, STOCHF and KDJ compute Fast-K from its first bar under every MA type, so theirs grows with `startIdx` too. A stream's `Open` steps through the whole history it is given.

<a id="rt17"></a>**rT17** Under `FIRST_BAR_SEED`, the value a stage fed by the input takes at a bar does not depend on `startIdx`, up to rounding, so a function built from such stages alone, such as EMA, MACD's line, or STOCHF's Fast-D and STOCH's Slow-K with an `EMA` MA type, writes the same values for every `startIdx`. The seed bar of a stage fed by anything else moves with `startIdx`: a call to DEMA, TEMA, TRIX, MACD's signal and histogram, STOCH's Slow-D with an `EMA` MA type, and the like, starting past the lookback writes different values from a call starting at 0.

<a id="rt18"></a>**rT18** The EMA variation moves exactly the functions with a stage ([rT13](/spec/settings-threads/#rt13)):

- `EMA`, `DEMA`, `TEMA`, `TRIX`, `MACD`, `MACDFIX`, `MACDV`, `TSI`, `DOSC`, `CVI`, `SMI`, `EFI`, `ERI`, `MASSI`, `STC` (its MACD line), `WAVETREND`, `PSO`, `ZLEMA`, `KC` (its centre line);
- with an `EMA`, `DEMA`, `TEMA` or `ZLEMA` MA type: `APO`, `PPO` and `PVO` (whose default is `EMA`), `MA`, `MAVP`, `MACDEXT`, `BBANDS`, `BBW`, `PERCENTB`, `MAENV`, `KSTEXT`, `STOCH`, `STOCHF`, `STOCHRSI`, `KDJ`.

It does not move `T3` (also as an MA type), `ADOSC` and `KVO` (whose averages start on their first value under both), Wilder smoothing (`RSI`, `ATR`, the `ADX` family, `RMA`), the adaptive averages, STC's two 0.5 smoothers, or any other function.

<a id="metastock"></a>**From TA-Lib 0.7.1 and earlier.** For `EMA`, `DEMA`, `TEMA`, `TRIX`, `MACD`, `MACDFIX`, and the `EMA`, `DEMA` and `TEMA` MA types inside `MA`, `MAVP`, `MACDEXT`, `APO`, `PPO`, `BBANDS`, `STOCH` and `STOCHF`, a call with `startIdx` 0 under `FIRST_BAR_SEED` writes, within rounding, what `TA_SetCompatibility(TA_COMPATIBILITY_METASTOCK)` wrote. That mode also changed `RSI` and `CMO`, which no setting reproduces. `TA_SetCompatibility` and `TA_GetCompatibility` select nothing.
