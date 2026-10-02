---
title: Errors
description: "What a rejected TA-Lib call reports: the return codes a function call can answer, the batch call's conditions, and the abstraction layer, for the C, Rust, Java and C# APIs."
---

*Part of TA-Lib's [specification](/spec/), a reference for AI-agent-driven integration.*

What a rejected call reports, in C's spelling. How each code reaches a Rust, Java or C# caller is on the [hub](/spec/#failures); what the library does not detect at all is listed [there](/spec/#not-detected) too.

## The caller must {#caller}

- <a id="branch-on-code"></a>**Branch on the code, never on a message.** Java's and C#'s exceptions carry a message for people; its text is not specified. Rust's `Err(RetCode)` carries none.
- <a id="one-fault"></a>**Fix one fault at a time.** When a call breaks several conditions, which one it reports is not specified.
- <a id="failed-range"></a>**In C, ignore the range after a failure.** `*outBegIdx` and `*outNBElement` are unspecified when a call does not return `TA_SUCCESS`. A handle out-parameter: [ownership](/spec/streaming/#ownership).
- <a id="internal-band"></a>**In C, test internal errors as a range.** C returns a value from 5000 to 5999 ([rB9](/spec/errors/#rb9)), so compare against the range, never `== TA_INTERNAL_ERROR`.
- <a id="stop-on-alloc"></a>**Stop after an allocation failure.** Nothing after [rB8](/spec/errors/#rb8) is defined: not the outputs, not the range, not a stream handle.

## Return codes {#return-codes}

The codes a batch, lookback or stream call can answer:

| Code | `TA_RetCode` | Returned by |
|---:|---|---|
| 0 | `TA_SUCCESS` | Every call that succeeds, including a batch range that ends before the lookback (count 0, [rW2](/spec/inputs-outputs/#rw2)). |
| 2 | `TA_BAD_PARAM` | A rejected parameter, an absent or too-short buffer, one buffer used twice, a non-finite bar: most [batch](/spec/errors/#batch), [opening](/spec/streaming/#opening) and [advancing](/spec/streaming/#advancing) conditions; Rust's lookback ([rL2](/spec/lookback/#rl2)); the settings refusals in C and Rust ([rT2](/spec/settings-threads/#rt2)). |
| 3 | `TA_ALLOC_ERR` | A C call that allocates, when the allocation fails ([rB8](/spec/errors/#rb8)). |
| 12 | `TA_OUT_OF_RANGE_START_INDEX` | rB1, rS1. |
| 13 | `TA_OUT_OF_RANGE_END_INDEX` | rB2, rS2, rU4. |
| 17 | `TA_INSUFFICIENT_HISTORY` | Stream openers only ([rS8](/spec/streaming/#rs8)). A batch call never returns it. |
| 5000 to 5999 | `TA_INTERNAL_ERROR` + id | [rB9](/spec/errors/#rb9). |

`TA_RetCode` has further members, declared in `ta_defs.h`. Only `TA_Shutdown` and the abstraction layer return them.

## General rules

<a id="re1"></a>**rE1** A rejected call reports exactly one code (in Java and C#, one exception) and stops.

<a id="re2"></a>**rE2** One code across languages. A call that breaks one condition of the batch or stream tables gets that condition's code from every language that can express and detect it.

<a id="re3"></a>**rE3** A rejected call writes no output buffer. Nothing is promised after [rB8](/spec/errors/#rb8) or [rB9](/spec/errors/#rb9).

## Batch conditions {#batch}

A batch call accepts `0 <= startIdx <= endIdx <= TA_INDEX_MAX` ([`TA_INDEX_MAX`](/spec/#names)).

| Rule | Condition | Code | Not expressible in |
|---|---|---|---|
| <a id="rb1"></a>**rB1** | `startIdx` is below 0 or above `TA_INDEX_MAX`. | `TA_OUT_OF_RANGE_START_INDEX` | Rust, below 0 (`usize`) |
| <a id="rb2"></a>**rB2** | `endIdx` is below 0, above `TA_INDEX_MAX`, or below `startIdx`. | `TA_OUT_OF_RANGE_END_INDEX` | Rust, below 0 |
| <a id="rb3"></a>**rB3** | An optional parameter is outside its accepted values, or the parameters form a combination the function rejects ([rP2](/spec/inputs-outputs/#rp2)). | `TA_BAD_PARAM` | none |
| <a id="rb4"></a>**rB4** | A required argument is absent: an input, an output, or in C a range out-parameter. | `TA_BAD_PARAM` | none |
| <a id="rb5"></a>**rB5** | A buffer is too short: an input does not reach `endIdx` ([input length](/spec/inputs-outputs/#input-length)), or an output cannot hold the count the call produces ([output size](/spec/inputs-outputs/#output-size)). | `TA_BAD_PARAM` | none; C cannot detect it |
| <a id="rb6"></a>**rB6** | Two outputs are the same buffer. | `TA_BAD_PARAM` | Rust (safe code) |
| <a id="rb7"></a>**rB7** | An output is omitted that the function does not let a caller decline ([rW5](/spec/inputs-outputs/#rw5)). | `TA_BAD_PARAM` | Rust: cannot decline |
| <a id="rb8"></a>**rB8** | A memory allocation failed. | `TA_ALLOC_ERR` (C only) | none |
| <a id="rb9"></a>**rB9** | The library found an inconsistency in its own state. | `TA_INTERNAL_ERROR` + id | none |

**rB4.** An input or output is absent when it is `NULL` in C, `null` or empty in Java, and empty in Rust and C#, where a `null` array becomes an empty span. It is refused whatever the call would produce, a range that produces no values ([rW2](/spec/inputs-outputs/#rw2)) included. The one exception is how C# declines an output ([rW5](/spec/inputs-outputs/#rw5)): with an empty span.

**rB6.** Identity only: an input reused whole as an output is legal ([rW7](/spec/inputs-outputs/#rw7)), and partial overlap is the caller's to avoid ([no overlap](/spec/inputs-outputs/#no-overlap)). One buffer passed as two outputs is rejected.

**rB8.** Rust aborts the process, and Java and C# raise their runtime's out-of-memory error.

**rB9.** A bug in TA-Lib, not in the call: report it, with the number in C. C's `TA_SetRetCodeInfo` names every value from 5000 to 5999 `TA_INTERNAL_ERROR`, and a value that is not a `TA_RetCode` member `TA_UNKNOWN_ERR`. Rust, Java and C# report `TA_INTERNAL_ERROR` without an id.

## Abstraction layer {#abstraction-layer}

<a id="rm1"></a>**rM1** A call through the abstraction layer ([per language](/spec/#abstraction)) whose arguments are all bound and accepted by their setters is held to the batch conditions, with the same codes. The exception is C, whose setters take bare pointers and no length: a bound series shorter than the range goes undetected there, as in rB5.

<a id="rm2"></a>**rM2** A rejected setter leaves the parameter holder as it found it, so a rejected re-bind cannot leave the next call to succeed, silently, over a mix of old and new arguments.

How the layer answers a misuse of its own surface (an unknown name, an unbound or mistyped argument) is not specified.

## Conditions on other pages

| Call | Rules |
|---|---|
| Lookback, display shift | [rL2](/spec/lookback/#rl2) rejection signal, [rL3](/spec/lookback/#rl3) agrees with batch, [rL11](/spec/lookback/#rl11) display shift |
| Stream opening | [rS1](/spec/streaming/#rs1) empty history, [rS2](/spec/streaming/#rs2) too long, [rS3](/spec/streaming/#rs3) parameter, [rS4](/spec/streaming/#rs4) absent, [rS5](/spec/streaming/#rs5) length, [rS6](/spec/streaming/#rs6) one buffer twice, [rS7](/spec/streaming/#rs7) declined output, [rS8](/spec/streaming/#rs8) short history |
| Stream advancing | [rU1](/spec/streaming/#ru1) absent handle, [rU2](/spec/streaming/#ru2) absent output, [rU3](/spec/streaming/#ru3) non-finite bar, [rU4](/spec/streaming/#ru4) index ceiling, [rU5](/spec/streaming/#ru5) declined output |
| Settings | [rT2](/spec/settings-threads/#rt2) target, [rT3](/spec/settings-threads/#rt3) unstable period, [rT4](/spec/settings-threads/#rt4) reading, [rT5](/spec/settings-threads/#rt5) range type, [rT6](/spec/settings-threads/#rt6) average, [rT7](/spec/settings-threads/#rt7) NaN factor, [rT8](/spec/settings-threads/#rt8) no change on refusal |
