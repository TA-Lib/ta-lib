---
url: 'https://ta-lib.org/spec/settings-threads/index.md'
description: >-
  Settings in an immutable Core that any number of threads share with no
  synchronization (Rust, Java, C#), C's initialization and process-wide settings
  and when they may change, setting validation, and the caller's rules across
  threads.
---
# Settings and Threads

*Part of TA-Lib's [specification](/spec/), a reference for AI-agent-driven integration.*

C keeps its settings (unstable periods and candle settings) process-wide; Rust, Java and C# keep them in an immutable `Core`, so cores with different settings coexist in one process and any number of threads can share one with no synchronization ([rT1](/spec/settings-threads/#rt1)). What each setting means: [Unstable Period](/api/unstable-period/), [Candlestick Settings](/api/candle-settings/) (with the defaults). Names per language: [names](/spec/#names). How a failure reaches the caller: [failures](/spec/#failures).

## The caller must {#caller}

* <a id="initialize"></a>**In C, initialize exactly once per process.** Call `TA_Initialize` before any other TA call. Call `TA_Shutdown` before the process exits; the library must not be used after it. Rust, Java and C# have no lifecycle call: a `Core` is ready when constructed.
* <a id="idle-settings"></a>**In C, change a setting only while no TA function is running and no stream is open.** A change is a call to `TA_SetUnstablePeriod`, `TA_SetCandleSettings`, `TA_RestoreCandleDefaultSettings` or `TA_Shutdown`. Between changes, any number of threads may call the library ([rT11](/spec/settings-threads/#rt11)); it assumes the `malloc` and `free` it allocates with are thread-safe.
* <a id="one-writer"></a>**Run a writer alone on its stream handle.** The writers are `Update`, `Advance` and C's `Close`; not even `Peek` or `Value` may run alongside one. Separate handles, a clone included ([rH8](/spec/streaming/#rh8)), may each be driven on their own thread. Rust enforces this at compile time: the writing calls take `&mut self`, and every handle is `Send + Sync + Clone`.
* <a id="confine"></a>**Confine holders, builders and sinks to one thread.** A [parameter holder](/spec/abstract/#provides) is not thread-safe in C, Java and C#; neither is a Java or C# `CoreBuilder` nor the sink a Java multi-output stream writes into ([names](/spec/#names)). In C, the `const` in `TA_CallFunc`'s signature does not make a holder shareable: the call writes the output buffers bound to it.

## `Core` in Rust, Java and C\#

<a id="rt1"></a>**rT1** In Rust, Java and C#, settings live on an immutable `Core` made by a builder. One `Core` may be shared by any number of threads with no synchronization; Rust's is `Send + Sync`. To change a setting, build another `Core`, from defaults or seeded from an existing one (`to_builder()`, `toBuilder()`, `ToBuilder()`). A stream keeps the settings of the `Core` that opened it. A Java or C# builder stays usable after `build()`.

## Validation

The rows hold for every setter and getter in all four languages, except where a note says otherwise. A Rust or Java enum cannot hold a value outside its members.

| Rule | Condition | Language notes |
|---|---|---|
| <a id="rt2"></a>**rT2** | A setter fails on a target outside its enum. `TA_SetCandleSettings` and the builders' candle setter also fail on `TA_AllCandleSettings`. | In Rust and Java only the wildcard fails. |
| <a id="rt3"></a>**rT3** | An unstable period is a count in `[0, TA_INDEX_MAX]` or an Auto level: `TA_UNSTABLE_AUTO_PREC_4` or `TA_UNSTABLE_AUTO_PREC_8` ([constants](/spec/#names)). | C takes an `unsigned int`, so a negative value arrives above every level and fails. Rust's `u32` cannot hold one. |
| <a id="rt4"></a>**rT4** | Reading a setting for the set-all wildcard fails. | For the wildcard or an out-of-range id, C's `TA_GetUnstablePeriod` returns 0, which is also a legal period. |
| <a id="rt5"></a>**rT5** | A candle setting's range type is a `TA_RangeType` member. | |
| <a id="rt6"></a>**rT6** | A candle setting's `avgPeriod` is in `[0, TA_INDEX_MAX]`. | |
| <a id="rt7"></a>**rT7** | A candle setting's `factor` is finite and not negative. | |
| <a id="rt8"></a>**rT8** | A failed call leaves every setting as it was, a wildcard call included. | Rust's `build()` returns the first failure, even after a later valid setter or `restore_candle_default`. |

<a id="rt9"></a>**rT9** Two wildcards act on every member at once: `TA_FUNC_UNST_ALL` sets every unstable period, and `TA_AllCandleSettings` restores every candle setting's default through `TA_RestoreCandleDefaultSettings` and the builders' restore call (`restore_candle_default`, `restoreCandleDefault`, `RestoreCandleDefault`).

<a id="rt12"></a>**rT12** A getter returns the value the setter stored. For an unstable period on an Auto level that is the level's constant, not the number of bars it adds ([rL6](/spec/lookback/#rl6)).

## Initial state

<a id="rt10"></a>**rT10** In C, `TA_Initialize` leaves every unstable period at 0 and every candle setting at its default.

## Threads in C

<a id="rt11"></a>**rT11** Between setting changes, C calls made from several threads at once, each on its own stream handles and parameter holders, return what a single thread returns. On one handle with no writer running, `Peek`, `Value`, `OutRange` and `Clone` may run from several threads at once.
