---
url: 'https://ta-lib.org/spec/streaming/index.md'
description: >-
  The streaming contract in C, Rust, Java and C#: what the caller must do,
  bit-identity with batch, opening and its conditions, Update, Peek and Advance,
  the accessors, and handle lifetime.
---
# Streaming

*Part of TA-Lib's [specification](/spec/), a reference for AI-agent-driven integration.*

A stream is defined as the batch call over every bar fed to it: the same values, bit for bit, and the same range ([rH1](/spec/streaming/#rh1)); a bar counted with [Advance](/spec/streaming/#rh6) enters the range only.

Every function streams ([rH9](/spec/streaming/#rh9)): `Open` or `OpenAndFill` once, then `Update` per closed bar and `Peek` per forming bar; `Value`, `OutRange`, `Advance` and `Clone` at any time; `Close` in C only. Calls, examples and per-language shapes: [C/C++](/api/stream/), [Rust](/api/rust/stream/), [Java](/api/java/stream/), [C#](/api/csharp/stream/). Rules use C's verbs (`TA_<N>_Open` and so on); each language's spelling is in [names](/spec/#names), and how each code reaches the caller in [failures](/spec/#failures).

## The caller must {#caller}

* <a id="history"></a>**Open on at least `lookback + 1` finite bars.** A shorter, non-empty history answers `TA_INSUFFICIENT_HISTORY` ([rS8](/spec/streaming/#rs8)), the one recoverable code: send more bars rather than fix the call. A NaN or infinity inside the history is not detected ([finite inputs](/spec/inputs-outputs/#finite-inputs)).
* <a id="rejected-bar"></a>**Answer every rejected bar.** A rejected `Update` changed nothing ([rH5](/spec/streaming/#rh5)), so the handle is now one bar behind the feed. Re-feed the bar with a corrected value, or count it with `Advance` ([rH6](/spec/streaming/#rh6)).
* <a id="ownership"></a>**In C, close every handle exactly once.** `Open`, `OpenAndFill` and `Clone` each return a handle that `Close` frees. Never open or clone into a variable that holds a live handle: a failed call sets it to NULL. Rust drops a handle; Java and C# collect it.
* <a id="checkpoint"></a>**Keep the bars to resume.** A handle is not serializable and is valid only within the library version that opened it. To restart, reopen on the bars, then call `Advance` once for each bar the old handle counted without being fed ([rH6](/spec/streaming/#rh6)).
* **Give each handle one writer**: [threads](/spec/settings-threads/#one-writer). **In C, change no setting while a stream is open**: [settings](/spec/settings-threads/#idle-settings).

## Definition

<a id="rh1"></a>**rH1** Open a stream on bars 0 to k, then `Update` it with bars k+1 to t, with no `Advance`. At every bar the stream reported (Open's value for bar k, each Update's for its bar), the value is bit-identical to what `batch(0, t)` writes for that bar, and `OutRange` equals the range `batch(0, t)` reports. This holds under the same parameters and settings. One exception: a zero output may differ in sign, `+0.0` against `-0.0`, which compare equal.

<a id="rh2"></a>**rH2** The history given to `Open` defines bar 0. State is carried forward from bar to bar and never re-seeded, so a stream opened on a later start equals the batch call over that shorter series. Which functions' values depend on the start: [/functions/stability](/functions/stability).

## Opening

For `Open` and `OpenAndFill`, the history is the first declared input and `historyLen` its length (a C argument).

| Rule | Condition | Code | Not checked in |
|---|---|---|---|
| <a id="rs1"></a>**rS1** | The history is empty (`historyLen < 1`) | `TA_OUT_OF_RANGE_START_INDEX` | |
| <a id="rs2"></a>**rS2** | The history holds more than `TA_INDEX_MAX + 1` bars | `TA_OUT_OF_RANGE_END_INDEX` | |
| <a id="rs3"></a>**rS3** | An optional parameter is outside its accepted values, or the parameters form a combination the function rejects ([rP2](/spec/inputs-outputs/#rp2)) | `TA_BAD_PARAM` | |
| <a id="rs4"></a>**rS4** | A required argument is absent ([rB4](/spec/errors/#rb4)): an input, an output, or in C the handle out-parameter, `outBegIdx` or `outNBElement` | `TA_BAD_PARAM` | |
| <a id="rs5"></a>**rS5** | An input's length differs from `historyLen`, or an `OpenAndFill` output holds fewer than `historyLen - lookback` values | `TA_BAD_PARAM` | C: cannot check ([input length](/spec/inputs-outputs/#input-length)) |
| <a id="rs6"></a>**rS6** | `OpenAndFill`: an output is the buffer of an input or of another output | `TA_BAD_PARAM` | Rust: cannot alias |
| <a id="rs7"></a>**rS7** | `OpenAndFill`: an output that is not declinable is declined ([rW5](/spec/inputs-outputs/#rw5)) | `TA_BAD_PARAM` | Rust: cannot decline |
| <a id="rs8"></a>**rS8** | The history holds at least one bar, but fewer than [lookback](/spec/lookback/#rl1) `+ 1` | `TA_INSUFFICIENT_HISTORY` | |

A loop that waits for enough history starts at one bar: an empty history is rS1.

<a id="rh3"></a>**rH3** `OpenAndFill` writes what `batch(0, historyLen - 1)` writes, compared as in [rH1](/spec/streaming/#rh1): `historyLen - lookback` values per output from index 0, the first for bar `lookback`, and the same range. It takes no `startIdx`. The handle it returns then updates as the one `Open` returns on the same history.

## Advancing

`Update`, `Peek` and `Advance`.

| Rule | Condition | Code | Calls | Not checked in |
|---|---|---|---|---|
| <a id="ru1"></a>**rU1** | The handle is absent | `TA_BAD_PARAM` | every handle call except Close | Rust, Java, C#: cannot be absent |
| <a id="ru2"></a>**rU2** | A required output is absent: a C out-pointer, a Java multi-output sink | `TA_BAD_PARAM` | Update, Peek; in C also Value, OutRange, Clone | Rust, C#: value returned |
| <a id="ru3"></a>**rU3** | A bar value is NaN, `+Inf` or `-Inf` | `TA_BAD_PARAM` | Update, Peek | |
| <a id="ru4"></a>**rU4** | The bar this call would count leaves the index domain: `begIdx + count > TA_INDEX_MAX` | `TA_OUT_OF_RANGE_END_INDEX` | Update, Advance | |
| <a id="ru5"></a>**rU5** | An output that is not declinable is declined ([rW5](/spec/inputs-outputs/#rw5)) | `TA_BAD_PARAM` | Update, Peek | Rust, Java, C#: nothing to decline |

* rU4 never clears: open a new handle on a shorter history. `Peek` counts no bar, so it keeps answering at the ceiling.
* In C, a declination binds only its own call: the outputs an `Update` or `Peek` declines may differ from the opener's and from the previous call's.
* An accepted bar whose output is not finite ([rW6](/spec/inputs-outputs/#rw6)) is a success: the call returns that value and the bar is counted.

<a id="rh4"></a>**rH4** `Peek` never advances the stream and never changes the handle, whatever its outcome and however often it is called. Its value is bit-identical to what the next `Update` with the same bar returns.

<a id="rh5"></a>**rH5** A rejected `Update` or `Peek` changes nothing: no state, no value, no range, no output variable written. A re-fed bar counts exactly once. An allocation failure ([rB8](/spec/errors/#rb8)) or an internal error ([rB9](/spec/errors/#rb9)) is outside this rule.

<a id="rh6"></a>**rH6** `Advance` counts one bar the handle was not fed: the range grows by one, and that bar's output is the previous one, held. Later Updates compute over the bars fed, as if the counted bar did not exist; only the range includes it. An index output ([rW4](/spec/inputs-outputs/#rw4)) is a position among the bars fed: a bar fed after k `Advance` calls is named k below its place in `OutRange`.

## Accessors

<a id="rh7"></a>**rH7** `Value` returns the value(s) at the last counted bar, the bar the range ends on: after `Open` the last history bar's, after an accepted `Update` that bar's, after `Advance` the held value. `OutRange` is `[begIdx, begIdx + count)` in the input series' coordinates: the batch range over the same bars, Advance-counted bars included. `begIdx + count` never exceeds `TA_INDEX_MAX + 1`.

<a id="rh8"></a>**rH8** `Clone` is a deep, independent fork at the same bar: the same value and range, and updating either never affects the other.

## Discovery

<a id="rh9"></a>**rH9** Every function streams, in every language. The [metadata flag](/spec/abstract/#flags) is `TA_FUNC_FLG_STREAM`: Rust `FuncFlags::STREAM`, Java `FuncFlags.STREAMING`, C# `FuncFlags.Stream`. A stream is opened by its typed `Open`; the Abstract API binds batch calls only.

## Release

<a id="rh10"></a>**rH10** In C, `Close(NULL)` succeeds and does nothing. Rust, Java and C# have no release call.
