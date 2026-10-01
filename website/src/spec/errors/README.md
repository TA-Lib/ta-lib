---
title: Errors
description: "Every TA-Lib return code, the order a rejected call is evaluated in, and the batch tier's conditions, for the C, Rust, Java and C# APIs."
---

*Part of TA-Lib's exhaustive [specifications](/spec/), intended for precise AI-agent-driven integration with TA-Lib, to minimize errors.*

A rejected call reports one condition, the first in its tier's order, and leaves the caller's buffers as it found them. This page owns the return codes, the general rules R1 to R5, the batch tier B1 to B8 and the abstraction layer's M1 and M2, in C's spelling; how each code reaches a Rust, Java or C# caller is on the [hub](/spec/#failures).

## Return codes

| Code | `TA_RetCode` | Returned by |
|---:|---|---|
| 0 | `TA_SUCCESS` | Every call that succeeds, including a batch range that ends before the lookback (count 0, [N1](/spec/inputs-outputs/#n1)). |
| 1 | `TA_LIB_NOT_INITIALIZE` | `TA_Shutdown`, called when the library is not initialized ([T1](/spec/settings-threads/#t1)). |
| 2 | `TA_BAD_PARAM` | B3 to B6a, S3 to S6a, U1 to U3, U6a; the checks before [S1](/spec/streaming/#s1) and the stream [accessors](/spec/streaming/#accessors); Rust's lookback ([L1](/spec/lookback/#l1)); the settings refusals G1 to G6 in C and Rust, except C's getter ([G3](/spec/settings-threads/#g3)); the abstraction layer. |
| 3 | `TA_ALLOC_ERR` | Any C call that allocates, when the allocation fails ([B7](/spec/errors/#b7)). |
| 4 | `TA_GROUP_NOT_FOUND` | `TA_FuncTableAlloc`, an unknown group. |
| 5 | `TA_FUNC_NOT_FOUND` | `TA_GetFuncHandle`, an unknown function. |
| 6 | `TA_INVALID_HANDLE` | An invalid function handle. |
| 7 | `TA_INVALID_PARAM_HOLDER` | An invalid parameter holder. |
| 8 | `TA_INVALID_PARAM_HOLDER_TYPE` | A holder setter whose type does not match the parameter. |
| 9 | `TA_INVALID_PARAM_FUNCTION` | Nothing. |
| 10 | `TA_INPUT_NOT_ALL_INITIALIZE` | `TA_CallFunc` (and C#'s `ParamHolder.TryCall`), an input left unbound. |
| 11 | `TA_OUTPUT_NOT_ALL_INITIALIZE` | `TA_CallFunc` (and C#'s `ParamHolder.TryCall`), an output left unbound. |
| 12 | `TA_OUT_OF_RANGE_START_INDEX` | B1, S1. |
| 13 | `TA_OUT_OF_RANGE_END_INDEX` | B2, S2, U4. |
| 14 | `TA_INVALID_LIST_TYPE` | Nothing. |
| 15 | `TA_BAD_OBJECT` | `TA_FuncTableFree` or `TA_GroupTableFree`, an invalid table. |
| 16 | `TA_NOT_SUPPORTED` | Nothing. |
| 17 | `TA_INSUFFICIENT_HISTORY` | Stream openers only ([S7](/spec/streaming/#s7)). A batch call never returns it. |
| 5000 to 5999 | `TA_INTERNAL_ERROR` + id | [B8](/spec/errors/#b8). |
| 65535 | `TA_UNKNOWN_ERR` | Nothing. |

Who returns codes 1, 4 to 11, 14 to 16 and 65535 is current behaviour, and so is the abstraction layer's own `TA_BAD_PARAM` ([abstraction layer](/spec/errors/#abstraction-layer)).

`TA_SetRetCodeInfo` names and describes any value: 5000 to 5999 as `TA_INTERNAL_ERROR`, one it does not know as `TA_UNKNOWN_ERR`.

## General rules

<a id="r1"></a>**R1** One condition per call: a rejected call reports exactly one code (in Java and C#, one exception) and stops.

<a id="r2"></a>**R2** The first listed condition wins: a rejected call reports the code of the first condition it violates, in its tier's table order. Conditions that share a code may be checked in any order, and a Java or C# message, or a C# `ParamName`, may name any of them ([R5](/spec/errors/#r5)).

<a id="r3"></a>**R3** One code across backends. In the batch and stream tiers, a call that two or more backends can express and detect gets the same code from each; the one known exception is in [B6](/spec/errors/#b6). Lookback: [L3](/spec/lookback/#l3). Settings refusals and the abstraction layer's own are outside R3 ([hub](/spec/#failures)).

<a id="r4"></a>**R4** Checks precede writes. A call rejected under any rule of this specification leaves every caller-owned buffer, and C's range out-parameters, as it found them, except for C's writes stated with [S7](/spec/streaming/#s7) (`OpenAndFill`'s range) and with its handle out-parameters ([lifetime](/spec/streaming/#lifetime)). Current behaviour: C's MAVP and FRAMA set the range out-parameters to 0 before rejecting a value [I3](/spec/inputs-outputs/#i3) names. Nothing is promised after [B7](/spec/errors/#b7) or [B8](/spec/errors/#b8).

## Batch tier

A batch call accepts `0 <= startIdx <= endIdx <= TA_INDEX_MAX`; B1 and B2 are the only statement of that bound ([`TA_INDEX_MAX`](/spec/#names), [I1](/spec/inputs-outputs/#i1)). Rows are in evaluation order (R2).

| Rule | Condition | Code | Not expressible in |
|---|---|---|---|
| <a id="b1"></a>**B1** | `startIdx` is below 0 or above `TA_INDEX_MAX`. | `TA_OUT_OF_RANGE_START_INDEX` | Rust, below 0 (`usize`) |
| <a id="b2"></a>**B2** | `endIdx` is below 0, above `TA_INDEX_MAX`, or below `startIdx`. | `TA_OUT_OF_RANGE_END_INDEX` | Rust, below 0 |
| <a id="b3"></a>**B3** | An optional parameter is outside its accepted values, or the parameters form a combination the function rejects ([I3](/spec/inputs-outputs/#i3)). | `TA_BAD_PARAM` | none |
| <a id="b4"></a>**B4** | A required argument is absent: an input, an output, or a range out-parameter. | `TA_BAD_PARAM` | Rust, C# |
| <a id="b5"></a>**B5** | A buffer is too short: an input does not reach `endIdx` ([I2](/spec/inputs-outputs/#i2)), or an output cannot hold the count the call produces ([O2](/spec/inputs-outputs/#o2)). | `TA_BAD_PARAM` | none; C cannot detect it |
| <a id="b6"></a>**B6** | Two outputs are the same buffer. | `TA_BAD_PARAM` | Rust (safe code) |
| <a id="b6a"></a>**B6a** | An output is omitted that the function does not let a caller decline ([O5](/spec/inputs-outputs/#o5)). | `TA_BAD_PARAM` | Rust: cannot decline. C#: no check of its own, [B5](/spec/errors/#b5) applies (current behaviour) |
| <a id="b7"></a>**B7** | A memory allocation failed. | `TA_ALLOC_ERR` (C only) | none |
| <a id="b8"></a>**B8** | The library found an inconsistency in its own state. | `TA_INTERNAL_ERROR` + id | none |

**B4, B6a.** An omitted output that reaches the call as null ([names](/spec/#names)) is reported by B4, which comes first. An empty output, in every language that checks lengths, is held to B5 alone: rejected when the call produces values, accepted when it produces none (current behaviour).

**B5.** C is handed bare pointers and cannot check: a short buffer is read or written past its end, which is undefined.

**B6.** Identity only: partial overlap is [N8](/spec/inputs-outputs/#n8), and an input reused whole as an output is legal ([N4](/spec/inputs-outputs/#n4)). Two distinct empty outputs never collide. Current behaviour, and R3's exception: C and Java reject one buffer passed as two outputs whatever its length; C# never treats an empty output as aliased, so on a range that produces no values one zero-length array passed as two outputs is `TA_BAD_PARAM` in Java and a success in C#. C and C# also compare an integer output with a real one.

**B7.** Fatal in every tier: nothing after it is defined (outputs, range, stream handle), so stop. Rust aborts the process, and Java and C# raise their runtime's out-of-memory error.

**B8.** A bug in TA-Lib, not in the call: report it. C returns a value from 5000 to 5999, `TA_INTERNAL_ERROR` plus an id, so test the band, never `== TA_INTERNAL_ERROR`. In the function tier the id names the guard that fired ([V4](/spec/versions/#v4)). Rust, Java and C# report `TA_INTERNAL_ERROR` without an id.

## Messages

<a id="r5"></a>**R5** In Java and C#, a batch-tier exception's message starts with `<N>: ` (`SMA: `) and a stream-tier one with `<N> <verb>: `, where `<verb>` is the stream call (`open`, `openAndFill`, `update`, `peek`, `advance`, and Java's `value`). Only the prefix is specified: branch on the code, not the text. A composed function may report a function it calls: a caller of `MACDEXT` can see `MA: `. Rust's `Err(RetCode)` carries no message.

## Abstraction layer {#abstraction-layer}

<a id="m1"></a>**M1** A call through the abstraction layer ([per language](/spec/#abstraction)) whose arguments are all bound and accepted by their setters runs the function's public entry point, so B1 to B8 hold with the same codes. The exception is C, whose setters take bare pointers and no length: a bound series shorter than the range goes undetected there, as in B5.

<a id="m2"></a>**M2** A rejected setter leaves the parameter holder as it found it, so a rejected re-bind cannot leave the next call to succeed, silently, over a mix of old and new arguments.

The layer's own surface (lookup, binding, unbound or mistyped arguments) is not specified. Current behaviour:

- Java's `setOptInput` refuses an MA-type value outside the enum, which C, Rust and C# accept and the call answers under B3. Java and C# refuse a null series at the setter. Neither refusal carries a code.
- An unbound input or output is reported before B1 in C, Java and C#, and after B2 in Rust ([hub](/spec/#failures)).

## Rules on other pages

| Tier | Rules |
|---|---|
| Lookback | [L1](/spec/lookback/#l1) rejection signal, [L2](/spec/lookback/#l2) agrees with batch, [L3](/spec/lookback/#l3) agrees with C, [L4](/spec/lookback/#l4) nothing else fails |
| Stream opening | [S1](/spec/streaming/#s1) empty history, [S2](/spec/streaming/#s2) too long, [S3](/spec/streaming/#s3) parameter, [S4](/spec/streaming/#s4) absent, [S5](/spec/streaming/#s5) length, [S6](/spec/streaming/#s6) aliasing, [S6a](/spec/streaming/#s6a) declined output, [S7](/spec/streaming/#s7) short history |
| Stream advancing | [U1](/spec/streaming/#u1) absent handle, [U4](/spec/streaming/#u4) index ceiling, [U2](/spec/streaming/#u2) absent output, [U6a](/spec/streaming/#u6a) declined output, [U3](/spec/streaming/#u3) non-finite bar |
| Stream release | [X1](/spec/streaming/#x1) `Close(NULL)` succeeds |
| Settings | [G1](/spec/settings-threads/#g1) target, [G2](/spec/settings-threads/#g2) unstable period, [G3](/spec/settings-threads/#g3) reading, [G4](/spec/settings-threads/#g4) range type, [G5](/spec/settings-threads/#g5) average, [G6](/spec/settings-threads/#g6) NaN factor, [G7](/spec/settings-threads/#g7) no change on refusal |

## What is not detected

None of these is promised to be reported. The caller avoids them, or treats what follows as undefined.

- [I4](/spec/inputs-outputs/#i4): a real input outside plus or minus 3e37.
- [I5](/spec/inputs-outputs/#i5): NaN or infinity inside an input array or an opener's history.
- [O7](/spec/inputs-outputs/#o7): intermediate overflow on finite input.
- [B5](/spec/errors/#b5): a buffer too short, in C.
- [N8](/spec/inputs-outputs/#n8): shared memory other than identity, such as partial overlap (C# rejects some; not to be relied on).
- [B7](/spec/errors/#b7): the state after an allocation failure.
- [G3](/spec/settings-threads/#g3): C's unstable-period getter given a wildcard or unknown id, which returns 0.
- [T1](/spec/settings-threads/#t1): C used before `TA_Initialize` or after `TA_Shutdown`.
- [T2](/spec/settings-threads/#t2): a C setting changed while a TA function is running or a stream is open.
