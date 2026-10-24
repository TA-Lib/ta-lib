---
title: Errors
description: "A rejected batch or stream call reports one code and writes no output, the same code in every language that can detect the fault: the codes a function call can return and the batch call's conditions, in C, Rust, Java and C#."
---

*Part of TA-Lib's [specification](/spec/), a reference for AI-agent-driven integration.*

The rules for what a rejected call reports, in C's spelling. Error handling written against one language maps onto the others ([rE2](/spec/errors/#re2)). How each code reaches a Rust, Java or C# caller is on the [hub](/spec/#failures).

## The caller must {#caller}

- <a id="branch-on-code"></a>**Branch on the code, never on a message.** Java's and C#'s exceptions carry a message for people; its text is not specified.
- <a id="one-fault"></a>**Fix one fault at a time.** When a call breaks several conditions, which one it reports ([rE1](/spec/errors/#re1)) is not specified.
- <a id="failed-range"></a>**In C, ignore the range after a failure.** `*outBegIdx` and `*outNBElement` are then unspecified. A handle out-parameter: [ownership](/spec/streaming/#ownership).
- <a id="internal-band"></a>**In C, test internal errors as a range**, never `== TA_INTERNAL_ERROR` ([rB8](/spec/errors/#rb8)).
- <a id="stop-on-alloc"></a>**Stop after an allocation failure** ([rE3](/spec/errors/#re3)).

## Return codes {#return-codes}

The codes a batch, lookback or stream call can return:

| Code | `TA_RetCode` | Returned by |
|---:|---|---|
| 0 | `TA_SUCCESS` | Every call that succeeds. |
| 2 | `TA_BAD_PARAM` | Most [batch](/spec/errors/#batch), [opening](/spec/streaming/#opening) and [advancing](/spec/streaming/#advancing) conditions; a failed lookback, display shift or settings call, in a language where it reports a code ([failures](/spec/#failures)). |
| 3 | `TA_ALLOC_ERR` | [rB7](/spec/errors/#rb7). |
| 12 | `TA_OUT_OF_RANGE_START_INDEX` | [rB1](/spec/errors/#rb1), [rS1](/spec/streaming/#rs1). |
| 13 | `TA_OUT_OF_RANGE_END_INDEX` | [rB2](/spec/errors/#rb2), [rS2](/spec/streaming/#rs2), [rU4](/spec/streaming/#ru4). |
| 17 | `TA_INSUFFICIENT_HISTORY` | Stream openers only ([rS7](/spec/streaming/#rs7)). |
| 5000 to 5999 | `TA_INTERNAL_ERROR` + id | [rB8](/spec/errors/#rb8). |

The Abstract API returns codes of its own: [Abstract API and Metadata](/spec/abstract/#codes). The remaining members of `TA_RetCode`, declared in `ta_defs.h`, are returned by nothing.

## General rules

<a id="re1"></a>**rE1** A rejected call reports exactly one code (in Java and C#, one exception) and stops.

<a id="re2"></a>**rE2** A call that breaks one condition of the batch or stream tables gets that condition's code from every language that can express and detect it.

<a id="re3"></a>**rE3** A rejected call writes no output buffer. Nothing is promised after [rB7](/spec/errors/#rb7) or [rB8](/spec/errors/#rb8).

## Batch conditions {#batch}

| Rule | Condition | Code | Not expressible in |
|---|---|---|---|
| <a id="rb1"></a>**rB1** | `startIdx` is below 0 or above [`TA_INDEX_MAX`](/spec/#names). | `TA_OUT_OF_RANGE_START_INDEX` | Rust, below 0 (`usize`) |
| <a id="rb2"></a>**rB2** | `endIdx` is below 0, above `TA_INDEX_MAX`, or below `startIdx`. | `TA_OUT_OF_RANGE_END_INDEX` | Rust, below 0 |
| <a id="rb3"></a>**rB3** | [rP2](/spec/inputs-outputs/#rp2) rejects an optional parameter value or a combination of values. | `TA_BAD_PARAM` | none |
| <a id="rb4"></a>**rB4** | A required argument is absent: an input, an output, or in C a range out-parameter. | `TA_BAD_PARAM` | none |
| <a id="rb5"></a>**rB5** | A buffer is too short: an input does not reach `endIdx` ([input length](/spec/inputs-outputs/#input-length)), or an output cannot hold the count the call produces ([output size](/spec/inputs-outputs/#output-size)). | `TA_BAD_PARAM` | none; C cannot detect it |
| <a id="rb6"></a>**rB6** | Two outputs are the same buffer. | `TA_BAD_PARAM` | Rust (safe code) |
| <a id="rb7"></a>**rB7** | A memory allocation failed. | `TA_ALLOC_ERR` (C only) | none |
| <a id="rb8"></a>**rB8** | The library found an inconsistency in its own state. | `TA_INTERNAL_ERROR` + id | none |

**rB4.** An input or output is absent when it is `NULL` in C, `null` or an empty array in Java, an empty slice in Rust, and an empty span in C# (a `null` array becomes one). An absent one is rejected even when the range produces no values ([rW2](/spec/inputs-outputs/#rw2)).

**rB7.** Rust, Java and C# allocate through their runtime.

**rB8.** Report it as a TA-Lib bug, with the number in C. C's `TA_SetRetCodeInfo` names every value from 5000 to 5999 `TA_INTERNAL_ERROR`, and a value that is not a `TA_RetCode` member `TA_UNKNOWN_ERR`. Rust, Java and C# report `TA_INTERNAL_ERROR` without an id.
