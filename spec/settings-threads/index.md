---
url: 'https://ta-lib.org/spec/settings-threads/index.md'
description: >-
  C initialization, C's process-wide settings and when they may change, the
  immutable Core of the Rust, Java and C# APIs, setting validation, and what the
  caller must do across threads.
---
# Settings and Threads

*Part of TA-Lib's [specification](/spec/), a reference for AI-agent-driven integration.*

C keeps its settings (unstable periods and candle settings) process-wide; Rust, Java and C# keep them in an immutable `Core`. What each setting means: [Unstable Period](/api/unstable-period/), [Candlestick Settings](/api/candle-settings/) (with the defaults). Names per language: [names](/spec/#names). How a refusal reaches the caller: [failures](/spec/#failures).

## The caller must {#caller}

Assume the library detects none of this: a breach is undefined behaviour, not an error code.

* <a id="initialize"></a>**In C, initialize once.** Call `TA_Initialize` once, and only once, per process, before any other TA call. Call `TA_Shutdown` before the process exits; the library must not be used after it. Rust, Java and C# have no lifecycle call: a `Core` is ready when constructed.
* <a id="idle-settings"></a>**In C, change a setting only while the library is idle.** `TA_SetUnstablePeriod`, `TA_SetCandleSettings` and `TA_RestoreCandleDefaultSettings` change process-wide state: call them only while no TA function is running and no stream is open. `TA_Shutdown` counts as a change. Between changes, the library may be called from any number of threads ([rT11](/spec/settings-threads/#rt11)); it allocates with `malloc` and `free` and assumes both are thread-safe.
* <a id="one-writer"></a>**Give each stream handle one writer.** `Update`, `Advance` and C's `Close` must not run concurrently with any other call on the same handle. Separate handles, a clone included ([rH8](/spec/streaming/#rh8)), may each be driven on their own thread. Rust enforces this at compile time: the writing calls take `&mut self`, and every handle is `Send + Sync + Clone`.
* <a id="confine"></a>**Confine holders, builders and sinks to one thread.** A parameter holder (C `TA_ParamHolder`, Java and C# `ParamHolder`) is not thread-safe, and in Java and C# neither is a `CoreBuilder` nor the caller-owned sink a Java multi-output stream writes into (`Core.MacdOut` for MACD). Make one per thread or per call. In C, the `const` in `TA_CallFunc`'s signature does not make a holder shareable: the call writes the output buffers bound to it.

## Managed cores

<a id="rt1"></a>**rT1** In Rust, Java and C#, settings live on an immutable `Core` made by a builder. One `Core` may be shared by any number of threads with no synchronization; Rust's is `Send + Sync`. To change a setting, build another `Core`, from defaults or seeded from an existing one (`to_builder()`, `toBuilder()`, `ToBuilder()`); the existing one is unchanged. A stream keeps the settings of the `Core` that opened it. A Java or C# builder stays usable after `build()`, and later changes to it never reach a `Core` it built.

## Validation

The rows hold for every setter and getter in all four languages, except where a note says otherwise.

| Rule | Condition | Language notes |
|---|---|---|
| <a id="rt2"></a>**rT2** | A setter refuses a target outside its enum, and a setter that takes a single target refuses the set-all wildcard. | Rust and Java enums cannot hold an out-of-domain target, so only the wildcard is refused there. |
| <a id="rt3"></a>**rT3** | An unstable period is in `[0, TA_INDEX_MAX]`. | C takes an `unsigned int`, so a negative value arrives above `TA_INDEX_MAX` and is refused. Rust's `u32` cannot hold one. |
| <a id="rt4"></a>**rT4** | Reading a setting for the set-all wildcard is refused. | C's `TA_GetUnstablePeriod` cannot refuse: it returns 0, which is also a legal period. The getters on `Core` refuse ([failures](/spec/#failures)): Rust `get_unstable_period`, Java `unstablePeriod`, C# `UnstablePeriod` and `CandleSettings`. |
| <a id="rt5"></a>**rT5** | A candle setting's range type is a `TA_RangeType` member. | Checked in C and C#. Rust and Java cannot express another value. |
| <a id="rt6"></a>**rT6** | A candle setting's `avgPeriod` is in `[0, TA_INDEX_MAX]`. | |
| <a id="rt7"></a>**rT7** | A candle setting's `factor` is finite and not negative. | |
| <a id="rt8"></a>**rT8** | A refused call leaves every setting as it was, a wildcard call included. | Rust's builder latches the first refusal until `build()`: a later valid setter or `restore_candle_default` does not clear it, and `to_builder()` starts with none latched. |

<a id="rt9"></a>**rT9** A wildcard is legal where a call documents one. `TA_FUNC_UNST_ALL` (`FuncUnstId::ALL`, `FuncUnstId.ALL`) sets every unstable period at once. `TA_AllCandleSettings` restores every candle setting's default through `TA_RestoreCandleDefaultSettings` and the builders' restore call (`restore_candle_default`, `restoreCandleDefault`, `RestoreCandleDefault`). Elsewhere rT2 or rT4 applies.

## Initial state

<a id="rt10"></a>**rT10** In C, `TA_Initialize` leaves every unstable period at 0 and every candle setting at its default.

## Threads in C

<a id="rt11"></a>**rT11** Between setting changes, C calls made from several threads at once, each on its own stream handles and parameter holders, return what a single thread returns. On one handle with no writer running, `Peek`, `Value`, `OutRange` and `Clone` may run from several threads at once.
