---
title: Streaming
description: "The streaming contract in C, Rust, Java and C#: bit-identity with batch, opening and its errors, Update, Peek and Advance, the accessors, and handle lifetime."
---

*Part of TA-Lib's exhaustive [specifications](/spec/), intended for precise AI-agent-driven integration with TA-Lib, to minimize errors.*

A stream is defined as the batch call over every bar fed to it: the same values, bit for bit up to the sign of a zero, and the same range ([H1](/spec/streaming/#h1)); a bar counted with [Advance](/spec/streaming/#h5) enters the range only. This page owns that definition and the error rules of the stream calls. How to call each language's stream API is on that language's streaming page.

## Calls

Every function streams ([H10](/spec/streaming/#h10)): `Open` or `OpenAndFill` once, then `Update` per closed bar and `Peek` per forming bar; `Value`, `OutRange`, `Advance` and `Clone` at any time; `Close` in C only. Calls, examples and per-language shapes: [C/C++](/api/stream/), [Rust](/api/rust/stream/), [Java](/api/java/stream/), [C#](/api/csharp/stream/). Rules use C's verbs (`TA_<N>_Open` and so on); each language's spelling is in [names](/spec/#names), and how each code reaches the caller in [failures](/spec/#failures).

## Definition

<a id="h1"></a>**H1** Open a stream on bars 0 to k, then `Update` it with bars k+1 to t, with no `Advance`. At every bar the stream reported (Open's value for bar k, each Update's for its bar), the value is bit-identical to what `batch(0, t)` writes for that bar, and `OutRange` equals the range `batch(0, t)` reports. This holds under the same parameters and settings, with no setting changed since `Open` ([T7](/spec/settings-threads/#t7)). One exception: a zero output may differ in sign, `+0.0` against `-0.0`, which compare equal (current behaviour: seen in functions that take a rolling maximum or minimum, such as MAX, MIN and MIDPOINT).

<a id="h2"></a>**H2** The history given to `Open` defines bar 0. State is carried forward from bar to bar and never re-seeded, so a stream opened on a later start equals the batch call over that shorter series. Which functions' values depend on the start: [/functions/stability](/functions/stability).

## Opening

For `Open` and `OpenAndFill`, the history is the first declared input and `historyLen` its length (a C argument). Rows are in evaluation order ([R2](/spec/errors/#r2)).

| Rule | Condition, in evaluation order | Code | Not checked in |
|---|---|---|---|
| <a id="s1"></a>**S1** | The history is empty (`historyLen < 1`) | `TA_OUT_OF_RANGE_START_INDEX` | |
| <a id="s2"></a>**S2** | The history holds more than `TA_INDEX_MAX + 1` bars | `TA_OUT_OF_RANGE_END_INDEX` | |
| <a id="s3"></a>**S3** | An optional parameter is outside its accepted values, or the parameters form a combination the function rejects ([I3](/spec/inputs-outputs/#i3)) | `TA_BAD_PARAM` | |
| <a id="s4"></a>**S4** | A required argument is absent: an input, an output, or C's `outBegIdx` or `outNBElement` | `TA_BAD_PARAM` | Rust, C#: cannot be absent |
| <a id="s5"></a>**S5** | An input's length differs from `historyLen` ([I2](/spec/inputs-outputs/#i2)), or an `OpenAndFill` output holds fewer than `historyLen - lookback` values | `TA_BAD_PARAM` | C: undefined ([B5](/spec/errors/#b5)) |
| <a id="s6"></a>**S6** | `OpenAndFill`: an output aliases an input or another output ([N8](/spec/inputs-outputs/#n8)) | `TA_BAD_PARAM` | Rust: cannot alias |
| <a id="s6a"></a>**S6a** | `OpenAndFill`: an output that is not declinable is declined ([O5](/spec/inputs-outputs/#o5)) | `TA_BAD_PARAM` | Rust: cannot decline. C#: no check of its own (current behaviour) |
| <a id="s7"></a>**S7** | The history holds fewer than [lookback](/spec/lookback/) `+ 1` bars | `TA_INSUFFICIENT_HISTORY` | |

- One check precedes S1. C: the `stream` argument itself, NULL answering `TA_BAD_PARAM` with nothing written. Java: a null first input, answering `TA_BAD_PARAM`. Rust and C# have none.
- S6a: C and Java decline with `NULL` or `null`, which S4 reports first. C# declines with an empty span, which only S5 bounds: when the history holds `lookback` bars or fewer, S5 requires no values and S7 answers `TA_INSUFFICIENT_HISTORY`, where C and Java answer `TA_BAD_PARAM`.
- `TA_INSUFFICIENT_HISTORY` is the one recoverable code: send more bars rather than fix the call. An empty history is S1, not S7, so a loop that waits for enough history starts at one bar.
- On S7, a C `OpenAndFill` sets `*outBegIdx` and `*outNBElement` to 0 and writes no output (current behaviour, an exception to [R4](/spec/errors/#r4)).
- Non-finite history values: [I5](/spec/inputs-outputs/#i5).

<a id="h3"></a>**H3** `OpenAndFill` writes what `batch(0, historyLen - 1)` writes, zero signs aside ([H1](/spec/streaming/#h1)): `historyLen - lookback` values per output from index 0, the first for bar `lookback`, and the same range. The handle it opens is the one `Open` opens on the same history. It takes no `startIdx`.

## Advancing

`Update`, `Peek` and `Advance`.

| Rule | Condition, in evaluation order | Code | Calls | Not checked in |
|---|---|---|---|---|
| <a id="u1"></a>**U1** | The handle is absent | `TA_BAD_PARAM` | all | Rust, Java, C#: cannot be absent |
| <a id="u4"></a>**U4** | The bar this call would count leaves the index domain: `begIdx + count > TA_INDEX_MAX` | `TA_OUT_OF_RANGE_END_INDEX` | Update, Advance | |
| <a id="u2"></a>**U2** | A required output is absent: a C out-pointer, a Java multi-output sink | `TA_BAD_PARAM` | Update, Peek | Rust, C#: value returned |
| <a id="u6a"></a>**U6a** | An output that is not declinable is declined ([O5](/spec/inputs-outputs/#o5)) | `TA_BAD_PARAM` | Update, Peek | Rust, Java, C#: nothing to decline |
| <a id="u3"></a>**U3** | A bar value is NaN, `+Inf` or `-Inf` ([I5](/spec/inputs-outputs/#i5)) | `TA_BAD_PARAM` | Update, Peek | |

- Outside both tables: [B8](/spec/errors/#b8) from an opener, and in C from some functions' `Update` and `Peek`; [B7](/spec/errors/#b7) in C from `Open`, `OpenAndFill`, `Clone`, and MAVP's `Update` and `Peek`.
- `Peek` is exempt from U4: it counts no bar, so it keeps answering at the ceiling.
- <a id="n7"></a>**N7** `Peek` never advances the stream and never writes the handle, whatever its outcome and however often it is called. Its value is bit-identical to what the next `Update` with the same bar returns.
- <a id="h4"></a>**H4** A rejected `Update` or `Peek` changes nothing: no state, no value, no range, no output variable written. Answer a rejected bar by re-feeding it with a corrected value, or by counting it with `Advance` ([H5](/spec/streaming/#h5)); doing neither leaves the handle one bar behind the feed. U4 never clears: open a new handle on a shorter history. [B7](/spec/errors/#b7), [B8](/spec/errors/#b8) and [O7](/spec/inputs-outputs/#o7) are outside this rule.
- <a id="h5"></a>**H5** `Advance` counts one bar the handle was not fed: the range grows by one and nothing else moves. That bar's output is the previous one, held, and `Value` answers it. Later Updates compute over the bars fed, as if the counted bar did not exist; only the range includes it.
- A declination binds only its own call: the set an `Update` or `Peek` declines may differ from the opener's and from the previous call's.
- An accepted bar whose output is not finite ([O6](/spec/inputs-outputs/#o6), such as LN on 0) is a success: the state advances, `Value` answers that value, and the range grows by one.

## Accessors

<a id="h6"></a>**H6** `Value` returns the value(s) at the last counted bar, the bar the range ends on, without recomputing: after `Open` the last history bar's, after an accepted `Update` that bar's, after `Advance` the held value. `OutRange` is `[begIdx, begIdx + count)` in the input series' coordinates: the batch range over the same bars, Advance-counted bars included. `begIdx + count` never exceeds `TA_INDEX_MAX + 1`.

<a id="h7"></a>**H7** `Clone` is a deep, independent fork at the same bar: the same state, value and range, and updating either never affects the other.

The C accessors' error surface:

| C call | `TA_BAD_PARAM` when | Other codes |
|---|---|---|
| `Value` | the stream is NULL, or a required output pointer is NULL; a declinable one may be NULL and is not written | none |
| `OutRange` | the stream or either out-pointer is NULL | none |
| `Advance` | the stream is NULL | U4 |
| `Clone` | the stream or `clone` is NULL | `TA_ALLOC_ERR` |
| `Close` | never | none |

In Rust, Java and C#, only `Advance` (U4) and Java's multi-output `value(out)` (`TA_BAD_PARAM` for a null sink) reject anything.

## Lifetime

- **C ownership.** Close every handle that `Open`, `OpenAndFill` or `Clone` returns, exactly once; `Close` frees it. When one of them fails, it sets its handle out-parameter (`*stream`, `*clone`) to NULL unless that argument is itself NULL ([B7](/spec/errors/#b7) aside), so never open or clone into a variable that holds a live handle. A failed `Clone` leaves the original untouched.
- <a id="x1"></a>**X1** `Close(NULL)` is a success no-op. Only C has a release call; Rust drops a handle and Java and C# collect it.
- <a id="h8"></a>**H8** A handle is not serializable and is valid only within the library version that opened it. To checkpoint, keep the history, the bars fed since and the number of bars counted with `Advance`; reopen on the bars and call `Advance` that many times ([H1](/spec/streaming/#h1), [H5](/spec/streaming/#h5)).
- Threads on one handle: [T4](/spec/settings-threads/#t4). A setting changed while a stream is open: [T7](/spec/settings-threads/#t7).

## Index outputs

<a id="h9"></a>**H9** In a stream, the index outputs of MININDEX, MAXINDEX and MINMAXINDEX count the bars fed to the handle: the history, then each accepted `Update`. A bar counted by `Advance` is not fed, so after an `Advance` an index no longer equals that bar's position in the range. Example in C, MININDEX at period 3: open on 6 bars, `Advance`, then `Update` with a new low; the stream answers 6, where batch over all 8 bars, the skipped one included, answers 7.

## Discovery

<a id="h10"></a>**H10** Every function streams, in every language. The metadata flag is `TA_FUNC_FLG_STREAM`: Rust `FuncFlags::STREAM`, Java `FuncFlags.STREAMING`, C# `FuncFlags.Stream`. A stream is opened by its typed `Open`; the abstraction layer binds batch calls only.
