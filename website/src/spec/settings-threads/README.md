---
title: Settings and Threads
description: "C initialization and process-global settings, the immutable Core of the Rust, Java and C# APIs, setting validation, and what is safe across threads and under a live stream."
---

*Part of TA-Lib's exhaustive [specifications](/spec/), intended for precise AI-agent-driven integration with TA-Lib, to minimize errors.*

C keeps its settings (unstable periods and candle settings) in process globals that are set before any concurrent use; Rust, Java and C# keep them in an immutable `Core`. This page owns the C lifecycle, the rules every setter and getter enforces, and what may run concurrently.

What each setting means: [Unstable Period](/api/unstable-period/), [Candlestick Settings](/api/candle-settings/) (with the defaults). Names per language: [names](/spec/#names). How a refusal reaches the caller: [failures](/spec/#failures).

## C lifecycle

<a id="t1"></a>**T1** Call `TA_Initialize` once, from one thread, before any other TA call, and again before using the library after `TA_Shutdown`. It sets every unstable period to 0 and every candle setting to its default. Call `TA_Shutdown` from one thread while no other TA call runs. Rust, Java and C# have no lifecycle call: a `Core` is ready when constructed.

Current behaviour in C:

- Nothing checks that `TA_Initialize` was called, and every setter works without it. Without it, or after `TA_Shutdown`, every setting the caller has not set is zero: CDL functions drop the averaging period from their lookback (`TA_CDL3BLACKCROWS_Lookback` gives 3 instead of 13) and return `TA_SUCCESS` with values judged against zero thresholds. Zero is also an unstable period's initialized value, so only CDL functions are affected.
- Every `TA_Initialize`, the first included, returns `TA_SUCCESS` and discards whatever was set before it.
- When it succeeds, `TA_Shutdown` sets every unstable period and every candle setting to zero and releases nothing. It returns `TA_LIB_NOT_INITIALIZE`, and changes nothing, when `TA_Initialize` has not been called since the process started or since the last successful `TA_Shutdown`.

## C settings

<a id="t2"></a>**T2** Unstable periods and candle settings are process-global, unsynchronized memory. Change them (`TA_SetUnstablePeriod`, `TA_SetCandleSettings`, `TA_RestoreCandleDefaultSettings`, and `TA_Initialize` or `TA_Shutdown`, which reset them) from one thread while no other TA call runs. Once they are set, batch calls, lookbacks, streams (under [T4](/spec/settings-threads/#t4)) and the abstraction layer, each thread with its own `TA_ParamHolder` ([T6](/spec/settings-threads/#t6)), may run on any number of threads. The library allocates with `malloc` and `free` and assumes both are thread-safe; an allocation failure is [B7](/spec/errors/#b7). How the settings enter a result: [L7](/spec/lookback/#l7) (unstable period), [L8](/spec/lookback/#l8) (candle averaging), [T7](/spec/settings-threads/#t7) (open streams).

## Managed cores

<a id="t3"></a>**T3** In Rust, Java and C#, settings live on an immutable `Core` made by a builder. There are no process-global settings. One `Core` may be shared by any number of threads with no synchronization: Rust's is `Send + Sync`, Java's has only final, deeply immutable fields and is safe even when published racily, and C#'s cannot change once built. To change a setting, build another `Core`, from defaults or seeded from an existing one (`to_builder()`, `toBuilder()`, `ToBuilder()`); the existing one is unchanged. A Java or C# builder stays usable after `build()`, and later changes to it never reach a `Core` it built.

Java's and C#'s `build()` cannot fail. How a refused setter call reaches the caller in each language: [failures](/spec/#failures) and [G7](/spec/settings-threads/#g7).

## Validation

The rows hold for every setter and getter in all four languages, except where a note says otherwise. Within one language every row refuses the same way, so [R2](/spec/errors/#r2) leaves their order open. Java tests for a null argument before any row.

| Rule | Condition | Language notes |
|---|---|---|
| <a id="g1"></a>**G1** | A setter refuses a target outside its enum, and a setter that takes a single target refuses the set-all wildcard. | Rust and Java enums cannot hold an out-of-domain target, so only the wildcard is refused there. |
| <a id="g2"></a>**G2** | An unstable period is in `[0, TA_INDEX_MAX]`. | C takes an `unsigned int`, so a negative value arrives above `TA_INDEX_MAX` and is refused. Rust's `u32` cannot hold one. |
| <a id="g3"></a>**G3** | Reading a setting for a target that names no single function or setting is refused. | C's `TA_GetUnstablePeriod` cannot refuse: it returns 0, which is also a legal period. The getters on `Core` refuse ([failures](/spec/#failures)): Rust `get_unstable_period`, Java `unstablePeriod`, C# `UnstablePeriod` and `CandleSettings`, the only candle-setting getter. |
| <a id="g4"></a>**G4** | A candle setting's range type is a `TA_RangeType` member. | Checked in C and C#. Rust and Java cannot express another value. |
| <a id="g5"></a>**G5** | A candle setting's `avgPeriod` is in `[0, TA_INDEX_MAX]`. | |
| <a id="g6"></a>**G6** | A candle setting's `factor` is not NaN. An infinite factor is accepted. | |
| <a id="g7"></a>**G7** | A refused call leaves every setting as it was, a wildcard call included. | Rust's builder latches the first refusal until `build()`: a later valid setter or `restore_candle_default` does not clear it, and `to_builder()` starts with none latched. |

<a id="n6"></a>**N6** A wildcard is legal where a call documents one. `TA_FUNC_UNST_ALL` (`FuncUnstId::ALL`, `FuncUnstId.ALL`) sets every unstable period at once. `TA_AllCandleSettings` restores every candle setting's default through `TA_RestoreCandleDefaultSettings` and the builders' restore call (`restore_candle_default`, `restoreCandleDefault`, `RestoreCandleDefault`). Elsewhere G1 or G3 applies. A reserved `UNUSED_n` id is in domain: every setter and getter accepts it, and it moves no function (current behaviour).

<a id="n5"></a>**N5** A negative `factor` is accepted in all four languages. On bars with `low <= min(open, close)` and `max(open, close) <= high` every candle range is non-negative, so with a positive average a negative factor gives a negative threshold ([model](/api/candle-settings/)): every test that a range is above it passes, and every test that a range is at or below it fails. For example, with a negative `BodyDoji` factor, CDLDOJI, whose test is at or below, fires on no bar with a positive average. Check a pattern's tests before relying on a negative factor.

## Threads

<a id="t4"></a>**T4** A stream handle has one writer. `Update`, `Advance` and C's `Close` must not run concurrently with any other call on the same handle. With no writer running, `Peek`, `Value`, `OutRange` and `Clone` may run concurrently on it, since none of them writes the handle (for `Peek`, also [N7](/spec/streaming/#n7)): C declares all four on a `const` handle; Rust on `&self`, so the compiler enforces this rule, and every Rust handle is `Send + Sync + Clone`; Java and C# allow it once the handle has been safely published to the reading threads. Separate handles, a clone included, share nothing writable, so each may be driven on its own thread.

<a id="t5"></a>**T5** A Java multi-output stream writes its results into a caller-owned sink (`Core.MacdOut` for MACD) on `update`, `peek` and `value`. The sink carries no publication guarantee: give each thread its own. The other languages' carriers: [names](/spec/#names).

<a id="t6"></a>**T6** A parameter holder (C `TA_ParamHolder`, Java and C# `ParamHolder`) is not thread-safe, and in Java and C# neither is a `CoreBuilder`: confine each to one thread, or make one per call. In C, the `const` in `TA_CallFunc`'s signature does not make a holder shareable: the call writes the output buffers bound to the holder, and the setters write the holder itself, with no synchronization. The function catalogs (Java `Functions`, C# `FunctionCatalog`, Rust `FUNCS`) are immutable and shared freely. Rust's builder setters take the builder by value and its `ParamHolder` setters and `call` take `&mut self`, so the compiler confines both.

## Settings under a live stream

<a id="t7"></a>**T7** What a stream sees when a setting changes after Open:

| Setting | C | Rust, Java, C# |
|---|---|---|
| Unstable period | Read once, at Open. A later change applies to later opens, batch calls and lookbacks, never to an open handle, so `TA_<N>_Lookback` may then disagree with that handle's range (current behaviour). | The `Core` that opened the handle cannot change. To stream with other settings, open from another `Core`. |
| Candle settings | Changing one while a CDL stream is open, by any call including `TA_Initialize` and `TA_Shutdown`, is undefined. Close the stream and open a new one. | The `Core` that opened the handle cannot change. |

What an open handle's values equal: [H1](/spec/streaming/#h1).
