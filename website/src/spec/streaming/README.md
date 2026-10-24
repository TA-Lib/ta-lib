---
title: Streaming
description: "Every function streams in C, Rust, Java and C#, bit for bit with the batch call outside stated exceptions: open on history, update per closed bar, peek at a forming bar without changing state, count a skipped bar, clone to fork, and release a handle; a rejected bar changes nothing."
---

*Part of TA-Lib's [specification](/spec/), a reference for AI-agent-driven integration.*

A stream is defined as the batch call over every bar fed to it, bit for bit outside the exceptions in [rH1](/spec/streaming/#rh1), so a live feed reproduces a backtest over the same bars.

Every function streams ([rH9](/spec/streaming/#rh9)): `Open` or `OpenAndFill` once, then `Update` per closed bar and `Peek` per forming bar; `Value`, `OutRange`, `Advance` and `Clone` at any time; `Close` in C only. Calls, examples and per-language shapes: [C/C++](/api/stream/), [Rust](/api/rust/stream/), [Java](/api/java/stream/), [C#](/api/csharp/stream/). Rules use C's verbs ([names](/spec/#names)) and codes ([failures](/spec/#failures)).

## The caller must {#caller}

- <a id="history"></a>**Open on at least `lookback + 1` finite bars** ([finite inputs](/spec/inputs-outputs/#finite-inputs)). `TA_INSUFFICIENT_HISTORY` ([rS7](/spec/streaming/#rs7)) is the one recoverable code: send more bars rather than fix the call.
- <a id="rejected-bar"></a>**Account for every rejected bar.** After a rejected `Update` the handle is one bar behind the feed ([rH5](/spec/streaming/#rh5)): re-feed the bar with a corrected value, or count it with `Advance` ([rH6](/spec/streaming/#rh6)).
- <a id="ownership"></a>**In C, close every handle exactly once.** `Open`, `OpenAndFill` and `Clone` each return a handle that `Close` frees. Never open or clone into a variable that holds a live handle: a failed call sets it to NULL.
- <a id="checkpoint"></a>**Keep the bars to resume.** A handle cannot be serialized: reopen on the old handle's history and every bar it accepted since ([rH2](/spec/streaming/#rh2)), then call `Advance` once for each bar it counted without being fed ([rH6](/spec/streaming/#rh6)).
- **Run a writer alone on its handle**: [threads](/spec/settings-threads/#one-writer). **In C, change no setting while a stream is open**: [settings](/spec/settings-threads/#idle-settings).

## Definition

<a id="rh1"></a>**rH1** Open a stream on bars 0 to k, then `Update` it with bars k+1 to t, with no `Advance`. At every bar the stream reported (Open's value for bar k, each Update's for its bar), the value is bit-identical to what `batch(0, t)` writes for that bar, and `OutRange` equals the range `batch(0, t)` reports. This holds under the same parameters and settings. Exceptions: a zero output may differ in sign, `+0.0` against `-0.0`, which compare equal; and a value that depends on a transcendental function may differ ([transcendental functions](/spec/versions/#transcendental)).

<a id="rh2"></a>**rH2** The history given to `Open` defines bar 0. State is carried forward from bar to bar and never re-seeded, so a stream opened on a later start equals the batch call over that shorter series. Which functions' values depend on the start: [/functions/stability](/functions/stability).

## Opening

For `Open` and `OpenAndFill`, the history is the bars the stream opens on, and `historyLen` their count: a C argument, and in Rust, Java and C# the length of the first declared input.

| Rule | Condition | Code | Not checked in |
|---|---|---|---|
| <a id="rs1"></a>**rS1** | The history is empty (`historyLen < 1`) | `TA_OUT_OF_RANGE_START_INDEX` | |
| <a id="rs2"></a>**rS2** | The history holds more than `TA_INDEX_MAX + 1` bars | `TA_OUT_OF_RANGE_END_INDEX` | |
| <a id="rs3"></a>**rS3** | As [rB3](/spec/errors/#rb3) | `TA_BAD_PARAM` | |
| <a id="rs4"></a>**rS4** | A required argument is absent ([rB4](/spec/errors/#rb4)), including the C handle out-parameter. An empty history is rS1 | `TA_BAD_PARAM` | |
| <a id="rs5"></a>**rS5** | An input's length differs from `historyLen`, or an `OpenAndFill` output holds fewer than `historyLen - lookback` values | `TA_BAD_PARAM` | C: cannot check ([rB5](/spec/errors/#rb5)) |
| <a id="rs6"></a>**rS6** | `OpenAndFill`: an output is the buffer of an input or of another output; in C#, a span that starts where the other does | `TA_BAD_PARAM` | Rust: cannot alias |
| <a id="rs7"></a>**rS7** | The history holds at least one bar, but fewer than [lookback](/spec/lookback/#rl1) `+ 1` | `TA_INSUFFICIENT_HISTORY` | |

<a id="rh3"></a>**rH3** `OpenAndFill` writes what `batch(0, historyLen - 1)` writes ([rW1](/spec/inputs-outputs/#rw1)), compared as in [rH1](/spec/streaming/#rh1), and reports the same range. The handle it returns then updates as the one `Open` returns on the same history.

## Advancing

A bar is counted when it enters the range, by an accepted `Update` or by `Advance` ([rH6](/spec/streaming/#rh6)).

| Rule | Condition | Code | Calls | Not checked in |
|---|---|---|---|---|
| <a id="ru1"></a>**rU1** | The handle is absent | `TA_BAD_PARAM` | every handle call except Close | Rust, Java, C#: cannot be absent |
| <a id="ru2"></a>**rU2** | A required output is absent: a C out-pointer, a Java multi-output sink | `TA_BAD_PARAM` | Update, Peek; in C also Value, OutRange, Clone; in Java also value | Rust, C#: value returned |
| <a id="ru3"></a>**rU3** | A bar value is NaN, `+Inf` or `-Inf` | `TA_BAD_PARAM` | Update, Peek | |
| <a id="ru4"></a>**rU4** | The bar this call would count leaves the index domain: `OutRange`'s `begIdx + count > TA_INDEX_MAX` | `TA_OUT_OF_RANGE_END_INDEX` | Update, Advance | |

- rU4 never clears: open a new handle on a shorter history.
- A bar whose output is not finite ([rW5](/spec/inputs-outputs/#rw5)) is accepted.

<a id="rh4"></a>**rH4** `Peek` never changes the handle, however often it is called. Its value is bit-identical to what the next `Update` with the same bar returns.

<a id="rh5"></a>**rH5** A rejected `Update` or `Peek` changes nothing: no state, no value, no range, no output variable written. An allocation failure ([rB7](/spec/errors/#rb7)) or an internal error ([rB8](/spec/errors/#rb8)) is outside this rule.

<a id="rh6"></a>**rH6** `Advance` counts one bar the handle was not fed, and that bar's output is the previous one, held. Later Updates compute over the bars fed, as if the counted bar did not exist; only the range includes it. An index output ([rW4](/spec/inputs-outputs/#rw4)) is a position among the bars fed: a bar fed after k `Advance` calls has an index k less than its place in `OutRange`.

## Accessors

<a id="rh7"></a>**rH7** `Value` returns the value(s) at the last bar of `OutRange`.

<a id="rh8"></a>**rH8** `Clone` is a deep fork at the same bar: the same value and range, and updating either never affects the other.

## Discovery

<a id="rh9"></a>**rH9** Every function streams, in every language, and carries the [metadata flag](/spec/abstract/#flags) `TA_FUNC_FLG_STREAM`. The Abstract API opens no stream.

## Release

<a id="rh10"></a>**rH10** In C, `Close(NULL)` succeeds and does nothing.
