# API Error-Handling Specification

What every TA-Lib backend must do when a call is **wrong**, and which backends
have been verified to do it.

Normal behaviour — what a *correct* call returns — is specified by the tests
(`ta_regtest`, the value gates, `--xlang-hash`). It is summarised in Part 1 only
where it is routinely mistaken for an error.

The rest is failure behaviour: Part 2 is what the library detects and reports, Part 3 what it does not detect (design choices).

## How to read this

### The document is backend-neutral; the tables are the tracker

Each rule is stated once, in language-neutral terms: *this condition produces
this error*. **How** an error reaches the caller is a property of the language,
not of the rule, and is written down exactly once, in Appendix A. Essentially,
C and Rust report a code, Java and C# raise. No rule repeats it.

Identifiers like `<N>_OpenImpl` or `MAMA_OpenAndFill` are C's spelling, used
neutrally as notation for "the tier/function this rule is about" — since #278
dropped cross-language name parity for the streaming family, Rust, Java and C#
each recase `<N>` and the verb to their own idiom (`sma_open_and_fill`,
`mamaOpenAndFill`, `MamaOpenAndFill`); no rule here is about one specific
spelling.

The columns are a conformance tracker, one per shipped backend. A new backend
adds a column; it does not change a rule.

### Marks

| Mark | Meaning |
|:---:|---|
| ✅ | Verified: a probe against the built artifact produced the specified behaviour. |
| ⚠️ | Deviates deliberately, **or** is implemented but not verified by the CI test suite. The rule or its footnote says which, and where a deviation was decided. |
| ❌ | Measured, and does **not** conform. Needs a fix. Collected in Appendix D. |
| — | Not applicable: the condition cannot be expressed in this backend's types. |
| *(blank)* | Not yet verified. |

`—` is not a pass. It means the language removes the failure mode (a Rust slice
cannot be null, a C# `Span<T>` cannot be absent, an enum has no out-of-domain
value), so there is nothing for the backend to check and nothing for a caller to
hit.

### Order is normative

Within a tier the rules are listed **in the order they are evaluated**. A call
that violates several rules reports the **first** one listed. This is what makes
a multi-fault call predictable for automated tests.

Two consequences worth stating outright:

- **One error per call.** A call reports one condition and stops; it never
  accumulates.
- **Checks precede writes**, except where a bar was accepted as data: a U3
  rejection still counts its bar, and `UpdateAndFill` leaves the bars before the
  rejected one committed (§2.4). Every other rejection leaves each caller-owned
  buffer, and `OutRange`, exactly as it found them.
- **Order matters where the codes differ — and where the count does.** Rules
  answering the same code are otherwise mutually unordered in practice: a caller
  cannot tell which of them fired, so swapping two is invisible. U3 is the
  exception, being the one rejection a caller can identify by reading
  `OutRange`. What has to hold is that all four backends answer the **same code
  for the same call**, which `--xlang-hash` compares over the whole corpus.

### Vocabulary

**`TA_RetCode`.** C's enum is the reference; every backend mirrors a subset of
it, and no backend invents a code of its own. It has 20 members, but the
**function tier — batch and streaming — produces exactly seven**:

| RetCode | Meaning |
|---|---|
| `TA_SUCCESS` | the call completed; the reported `OutRange` says what was written |
| `TA_BAD_PARAM` | the catch-all rejection |
| `TA_OUT_OF_RANGE_START_INDEX` | `startIdx` outside the addressable index domain |
| `TA_OUT_OF_RANGE_END_INDEX` | `endIdx` outside the domain, or below `startIdx` |
| `TA_INSUFFICIENT_HISTORY` | a stream opener was given fewer than `lookback + 1` bars |
| `TA_ALLOC_ERR` | an allocation failed |
| `TA_INTERNAL_ERROR` | an invariant the library owns was violated |

The other thirteen belong to the **abstract tier** (`TA_INVALID_HANDLE`,
`TA_INVALID_PARAM_HOLDER`, `TA_FUNC_NOT_FOUND`, …) or to library
initialisation (`TA_LIB_NOT_INITIALIZE`), and four of them —
`TA_NOT_SUPPORTED`, `TA_INVALID_LIST_TYPE`, `TA_INVALID_PARAM_FUNCTION`,
`TA_UNKNOWN_ERR` — are returned from nowhere in the tree at all. Appendix C
defers the abstract tier, so no rule below produces any of the thirteen.

*A buffer is too short* has no member of its own. Where it is detected it is
reported as `TA_BAD_PARAM` (rule B5) — raised rather than returned, in the
backends that raise (Appendix A); Rust returns it — and the ⚠️ on the code says C
cannot detect it at all. Rule S5 carries the same code and the same ⚠️, for the
same reason: three backends bound the fill's output, and C has no size to bound
it against.

**`OutRange`.** What a successful call reports about its own output: the index
of the first value written, in the input series' coordinates, and how many values
follow it. C returns the two numbers through the `*outBegIdx` / `*outNBElement`
out-parameters; the other three carry them as one `OutRange` value.

**`MAX_INDEX`.** The addressable index ceiling, 100 000 000, identical in all
four backends.

**Range.** A real or integer optional parameter's range is the `range:` field
declared in the function .yaml.

---

## Part 1 — Normal behaviour

The conditions most often mistaken for failures. None of them is reported as an
error, and none of them is undefined — the conditions the library leaves
undefined are collected in Part 3.

| Rule | Condition | Result |
|---|---|---|
| N1 | A **valid range shorter than the lookback** | Success, zero values produced, an empty `OutRange`. Never an error. |
| N2 | Anywhere outside the reported `OutRange` | Untouched. The library never pads, and never emits a fill value. The converse — everything *inside* the range was written — holds everywhere but a U3 rejection, which counts a bar without handing its value over: the held value **is** that bar's output and only the write is suppressed, so `Value` still answers for it (§2.4). |
| N3 | An optional parameter set to its **default sentinel** | The documented default is substituted, then validated like any other value. |
| N4 | An output buffer that **is** an input buffer (whole-buffer, in place) | Allowed, in the batch tier. Several bodies are written for it. |
| N5 | A **negative** candlestick `factor` | Legal. It does not "never match" — it makes the comparison unconditionally true. |
| N6 | The set-all / restore-all **wildcards**, where a setter documents one | Legal on those setters, and rejected on the ones that name a single target (rule G1). |
| N7 | **Peeking** a forming bar, any number of times | Never advances the stream and never writes the handle. It can still be *rejected* (U3), and a rejected peek changes nothing either. |
| N8 | Buffers that **partially** overlap — same memory, different start | **Unspecified.** Only *identical* buffers are detected (rule B6). See Appendix E before assuming a diagnosis. |

---

---

## Part 2 — Error specification

### 2.1 Lookback tier

| Rule | Condition (in order) | RetCode | C | Rust | Java | C# |
|---|---|---|:---:|:---:|:---:|:---:|
| L1 | An optional parameter is outside its documented range | the lookback rejection signal | ✅ | ✅ | ✅ | ✅ |
| L2 | …and the signal is returned **exactly when** the batch tier would reject the same parameters under B3, or the streaming opener (`Open`/`OpenAndFill`) would reject them under S3 | the lookback rejection signal | ✅ | ✅ | ✅ | ✅ |
| L3 | …and, for Rust/Java/C#, the same accept-or-reject decision — and, wherever both it and C accept, the Batch/`OpenAndFill` output — matches what C produces for the identical parameters (the "Golden Check") | the lookback rejection signal | — | ✅ | ✅ | ✅ |
| L4 | Nothing else in this tier can fail | — | ✅ | ✅ | ✅ | ✅ |

**Rejection Signal**
For C, Java and C# it is returned as `-1`.
For Rust it is returned with `Result<usize, RetCode>` as `Err(RetCode::BadParam)`.

**Test Coverage**: `xlang_lookback_leg`, `xlang_tier_native_check`, `xlang_tier_gold_check` (regtest `--xlang-hash`).

### 2.2 Batch tier

| Rule | Condition (in order) | RetCode | C | Rust | Java | C# |
|---|---|---|:---:|:---:|:---:|:---:|
| B1 | `startIdx` outside `[0, MAX_INDEX]` | `TA_OUT_OF_RANGE_START_INDEX` | ✅<br>&nbsp; | ✅<br>&nbsp; | ✅<br>&nbsp; | ✅<br>&nbsp; |
| B2 | `endIdx` outside `[0, MAX_INDEX]`, **or** `endIdx < startIdx` | `TA_OUT_OF_RANGE_END_INDEX` | ✅<br>&nbsp; | ✅<br>&nbsp; | ✅<br>&nbsp; | ✅<br>&nbsp; |
| B3 | An optional parameter is outside its documented range (metadata from .yaml). A non-finite value (NaN, ±Inf) always returns an error. Note that non-finites as elements of input arrays are not detected or supported (See Part 3, "Non-finite input") | `TA_BAD_PARAM` | ✅<br>&nbsp; | ✅<br>&nbsp; | ✅<br>&nbsp; | ✅<br>&nbsp; |
| B4 | A required argument was not supplied — any declared input or output buffer, or missing `OutRange` pointer(s) | `TA_BAD_PARAM` | ✅<br>&nbsp; | —<br>[1] | ✅<br>&nbsp; | —<br>[2] |
| B5 | A buffer is too short: every declared input must reach `endIdx`, an output must hold the count actually produced (`endIdx - max(startIdx, lookback) + 1`). On a range shorter than the lookback that count is 0, so no output space is needed — but the input bound still holds | `TA_BAD_PARAM` ⚠️ | ⚠️<br>[3] | ✅<br>&nbsp; | ✅<br>&nbsp; | ✅<br>&nbsp; |
| B6 | Two outputs are the **same buffer** (Appendix E) | `TA_BAD_PARAM` | ✅<br>&nbsp; | ✅<br>&nbsp; | ✅<br>&nbsp; | ✅<br>&nbsp; |
| B6a | An output is unexpectedly **omitted** — null, or zero-length. Omission accepted only where the .yaml marks that output `nullable` (Appendix F) | `TA_BAD_PARAM` | ✅<br>&nbsp; | ✅<br>&nbsp; | ✅<br>&nbsp; | ✅<br>&nbsp; |
| B7 | A memory allocation failed. Only C reports it — Rust aborts, and the managed runtimes raise their own out-of-memory error. **Warning: implemented, but no CI job or probe covers it** — provoking one needs a failable allocator | `TA_ALLOC_ERR` ⚠️ | ⚠️<br>&nbsp; | ⚠️<br>&nbsp; | ⚠️<br>&nbsp; | ⚠️<br>&nbsp; |
| B8 | The library detected an inconsistency in its own state — a likely bug, please report it to the TA-Lib developers (Appendix A, "Internal errors"). **Warning: implemented, but the individual sites are not tested** | `TA_INTERNAL_ERROR` ⚠️ | ⚠️<br>&nbsp; | ⚠️<br>&nbsp; | ⚠️<br>&nbsp; | ⚠️<br>&nbsp; |

**Test Coverage**: `testIndexRange`, `checkOutputAliasRejected`,
`testBatchArgumentContract`, `c_batch_prologue_orders_parameters_before_presence`,
`rust_public_entry_orders_the_argument_contract`, `rust_batch_impl_orders_capacity_before_aliasing`,
`every_declared_input_is_checked_in_every_backend`, `BatchApiTest` (Java, C#); no phantom io and `--xlang-hash` for behavior matching.

[1] Rust slices cannot be null, and what C writes through a pointer — the
index/count pair (`OutRange`), a stream handle — is returned instead.

[2] C# `Span<T>` cannot be absent. A null array converts to an empty span, so
absence surfaces as a zero length: B5 in the batch tier, S1 for a stream's
history. A zero-length output is S5's case.

[3] C has no sizes to check against — a property of the ABI, not a defect. Part 3
has what happens instead.

### 2.3 Streaming tier — opening (`Open`, `OpenAndFill`)

| Rule | Condition (in order) | RetCode | C | Rust | Java | C# |
|---|---|---|:---:|:---:|:---:|:---:|
| S1 | The history is empty — the implied `startIdx` of 0 names no bar (`historyLen < 1`) | `TA_OUT_OF_RANGE_START_INDEX` | ✅<br>[4] | ✅<br>[4] | ✅<br>[4] | ✅<br>[4] |
| S2 | The history is longer than `MAX_INDEX + 1` — the implied `endIdx` of `historyLen - 1` leaves the index domain | `TA_OUT_OF_RANGE_END_INDEX` | ✅<br>[5] | ⚠️<br>[5] | ⚠️<br>[5] | ⚠️<br>[5] |
| S3 | An optional parameter is outside its documented range | `TA_BAD_PARAM` | ✅<br>&nbsp; | ✅<br>&nbsp; | ✅<br>&nbsp; | ✅<br>&nbsp; |
| S4 | A required argument was not supplied — the handle, any declared input, any output | `TA_BAD_PARAM` | ✅<br>&nbsp; | —<br>[1] | ✅<br>[6] | —<br>[2] |
| S5 | A buffer is too short: every declared input must be the history's length, and an `OpenAndFill` output must hold `historyLen - lookback`, the count the fill writes | `TA_BAD_PARAM` ⚠️ | ⚠️<br>[3] | ✅<br>[7] | ✅<br>[7] | ✅<br>[7] |
| S6 | (`OpenAndFill`) an output aliases an input, or another output | `TA_BAD_PARAM` | ✅<br>&nbsp; | —<br>&nbsp; | ✅<br>&nbsp; | ✅<br>&nbsp; |
| S6a | (`OpenAndFill`) an output is **declined** — null, or zero-length where the language cannot spell null. Accepted only where the .yaml marks that output `nullable` (Appendix F) | `TA_BAD_PARAM` | ✅<br>&nbsp; | ✅<br>&nbsp; | ✅<br>&nbsp; | ✅<br>&nbsp; |
| S7 | The history holds fewer than `lookback + 1` bars | `TA_INSUFFICIENT_HISTORY` | ✅<br>[8] | ✅<br>[8] | ✅<br>[8] | ✅<br>[8] |
| S8 | *(withdrawn — the warm-up history is an input array; see "Non-finite input")* [9] | — | —<br>&nbsp; | —<br>&nbsp; | —<br>&nbsp; | —<br>&nbsp; |

**The order is the batch tier's.** An opener is a batch call over
`[0, historyLen - 1]`, so S1–S6 are B1–B6 answering the same code for
the same fault.

**The warm-up check comes last**, because it is the one thing the batch tier has
no analogue for: a *history* shorter than the lookback cannot open a
stream at all (`TA_INSUFFICIENT_HISTORY`).

S7 is also what a **composed** opener answers when a sub-call succeeds with zero
elements: the batch tier may report that as an empty `OutRange`, but an opener
has no handle to mint over a range holding nothing, so the sub-call's own
`TA_SUCCESS` is not the answer.
`an_opener_never_answers_the_code_its_sub_call_handed_back` pins it in the three
ported backends. **C is the exception**, and it is a known one: the guard there
still carries the `retCode != TA_SUCCESS` half — that backend answers a
cross-call rejection by propagating the code rather than at the call site — so
the arm cannot be rewritten without splitting the guard, and C mints the handle.

Unlike the batch tier, `OpenAndFill` outputs may **not** be the same buffer as
the inputs (no in-place execution). Not because it would compute the wrong
answer — measured, it does not: the fill's writes stop where the handle's
warm-up seeds begin, or overlap them by the single slot the next `Update`
rewrites first. The ban is there because that margin is an accident of every
body's arithmetic that nothing states or asserts, and supporting in-place would
promise it for 176 functions in four backends, permanently.

**Test Coverage** (S1, S2, S4, S5, S6a and S7; the rest of this tier is not yet mapped):
`testStreamShortHistory` drives S1, S2's rejecting side and S7 in C — 9, 3 and 8
rejections, with S7's 16 controls. Three of S1's cases are *also* an absent
argument, which is what makes them about the order and not only the code.
`testBatchArgumentContract` drives S4 through B4's own argument shapes plus the
handle — 12 rejections and 5 controls, counted apart from B4's; `BatchApiTest`
does the same for Java, and adds S1's; `StreamApiTest` covers S4 in C#, where
the condition is a zero-length span and so is S1. Rust cannot express S4, and
`tests/stream_open_contract.rs` covers S1 there.
`scripts/check_stream_retcodes.py` carries S1 and S7 over the whole generated
corpus in all four backends — a probe names one function, and this is what
covers the other 175. It reads the *core's* arm, which in Java and C# the public
frame makes unreachable, so those two frames are covered corpus-wide by
`java_public_openers_check_arguments_then_the_index_pair` and
`csharp_public_openers_reject_an_empty_history_as_an_index_fault` instead.

S5 is probed from **both sides** in each of the three backends that can express
it — an exactly-sized output accepted and filled, one element shorter rejected —
because only the pair pins the arithmetic: a bound of `historyLen` would reject
the first, and no bound at all would accept the second. Each also drives the
tiers that hand-roll their fill (the dispatch tier including its identity arm,
the period bank, and a composed multi-output, whose sub-calls fill scratch of
their own rather than the caller's arrays), and asserts that a rejected fill left
the buffer untouched.
Corpus-wide, the width is pinned by the three `*_public_*fill*` /
`*_public_openers_*` gates in `open_validation_suite.rs`, which require it to be read
from the function's own lookback rather than from the history's length, that the
bound REJECT rather than merely exist, and that a `nullable` output be bounded
conditionally while every other output is not.

**A declined output** (rule S6a, Appendix F) has its own probe in each of
the three, beside S5's: `MAMA_OpenAndFill` with `outFAMA` declined must be
accepted, must leave the other output and the reported range bit-identical to
the supplied run, and must still report FAMA through the handle — the check that
fails if a backend "supports" declining by not computing the value. Both bounds
stay armed on that call: an undersized `outMAMA` beside a declined `outFAMA` is
still rejected, and so is an undersized `outFAMA` that WAS supplied.
`test_mama_nullable_fama_is_declinable_at_the_opener_in_every_backend` pins the
emitted shape in all four on the PR gate, because the three runtime probes are
nightly. What the opener declines binds nothing later: rule U6a in 2.4 is the
same choice, made again per call.

[4] **The one check that precedes the pair** is not the same in each backend,
and in each it is a precondition for evaluating the pair rather than an argument
competing with it. C: the handle — `*stream = NULL` is how "no handle on any
failure" is published, and there is nowhere to publish it without one. Java: the
history array's null test, since a length is not readable from an array that is
not there. Rust and C#: nothing — a slice cannot be absent, and in C# a null
array arrives as an empty span, so S1 *is* how an absent history is reported
(footnote [2]).

[5] Implemented in all four — the bound is in the opener's own prologue — but
probed only in C, which takes `historyLen` as a bare `int`, so the rejection
answers before a bar is read. The other three derive the length from the array
they are handed, so provoking it needs a 100 000 001-element allocation; the
legal upper edge, a history of exactly `MAX_INDEX + 1` bars, is out of reach
everywhere for the same reason.

[6] The presence checks sit on the public frame, because `<N>_OpenImpl` reads the
history's length before anything else could look at it. An output marked
`nullable` is the exception, as it is in the batch tier: it may be declined, and
naming it was the whole change — the call used to be rejected either way, by
`NullPointerException`.

[7] **The input half first.** B5 states its two halves as one rule, inputs ahead
of outputs, and this is B5 over `[0, historyLen - 1]`: the history's own length
IS the range, so every other declared input is that length rather than merely
reaching it — which is what the generated per-function docs have always
promised. It is checked on the public frame for the same reason the output half
is: the core makes the test too, but only after the capacity bound would have
answered, so a short input series was reported as an output-capacity fault.

**The output half** is B5's produced count read over the same range, which
collapses it to `historyLen - lookback` — never the width of the history. It has
no legitimate zero case the way B5 does: S7 refuses a history shorter than
`lookback + 1`, so a fill that runs writes at least one value, and there is no
empty output here as there is in the batch tier (rule N1). A short history is
floored to zero so that it reaches S7; a lookback of `-1` is not floored but
raised, which is what keeps S3 ahead of the buffer rules — the same thing the
batch tier's `clampedStart` does.

It sits on the **public** frame, never on `<N>_OpenAndFillInternal`: that seam
takes an anchor and writes `historyLen - max(lookback, startIdx)` — fewer — so
the same bound there would reject the composed sub-calls that pass a non-zero
anchor. It would also be redundant: a composed destination is proved disjoint by
`SubCallStep::is_fusable` and sized by construction, the generator having
allocated it. The frame reads the count from the function's own `<N>_Lookback`,
whose default substitution and range validation come with it.

An output marked `nullable` is bounded only when it is supplied — rule B6a read
on this tier, and the same conditional the batch wrapper emits. Declining one is
not a capacity fault: the value is still computed, the handle still reports it,
and nothing is written out (footnote [6], Appendix F).

[8] All four converge on `TA_INSUFFICIENT_HISTORY`, which leaves a history
*longer* than `MAX_INDEX + 1` (rule S2) as the only producer of
`TA_OUT_OF_RANGE_END_INDEX` in this tier. Verified as uniform, not incidental:
across all 176 streaming functions per backend, every short-history arm reports
this code and no other. What the four answered before the code existed, and why
the borrowed one was wrong on its face, is Appendix D item 8.

[9] **Withdrawn.** The warm-up history is an input *array*, and the library
does not scan those. Until this was removed it was the only array in the library
that was checked — 176 of 176 `Open` and 176 of 176 `OpenAndFill` entry points,
in all four backends — which made "arrays are never scanned" a rule with one
exception rather than a rule. What the scan cost, and why folding it into the
fill loop was not the alternative, are in `docs/streaming-api-design.md`. U3 is
untouched: a bar handed to `Update` or `Peek` is a single value.

[10] Only C has a bar count: the other three take slices or arrays, which carry
their own lengths, so "negative" is unrepresentable.

[11] C is handed bare pointers and has no sizes — the same blind spot as B5 and
S5, and ⚠️ for the same reason they are: a property of the ABI rather than a
defect, so there is nothing here for Appendix D to track. The other three answer
both before committing anything. `OpenAndFill`'s output capacity (S5) is now
validated outside C as well, so the two filling entry points agree; until #268's
follow-up `UpdateAndFill` was the only one that checked.

[12] Rust cannot express it: `&[f64]` and `&mut [f64]` cannot alias, so the
borrow checker rejects the call at compile time.

### 2.4 Streaming tier — advancing (`Update`, `Peek`, `UpdateAndFill`)

| Rule | Condition (in order) | RetCode | C | Rust | Java | C# |
|---|---|---|:---:|:---:|:---:|:---:|
| U1 | The handle was not supplied | `TA_BAD_PARAM` | ✅<br>&nbsp; | —<br>&nbsp; | —<br>&nbsp; | —<br>&nbsp; |
| U2 | The output — or, for `UpdateAndFill`, an input series — was not supplied | `TA_BAD_PARAM` | ✅<br>&nbsp; | —<br>&nbsp; | ✅<br>&nbsp; | —<br>&nbsp; |
| U4 | (`UpdateAndFill`) the bar count is negative | `TA_BAD_PARAM` | ✅<br>&nbsp; | n/a<br>[10] | n/a<br>[10] | n/a<br>[10] |
| U5 | (`UpdateAndFill`) the input series have different lengths | `TA_BAD_PARAM` | ⚠️<br>[11] | ✅<br>&nbsp; | ✅<br>&nbsp; | ✅<br>&nbsp; |
| U6 | (`UpdateAndFill`) an output is shorter than the bar count | `TA_BAD_PARAM` | ⚠️<br>[11] | ✅<br>&nbsp; | ✅<br>&nbsp; | ✅<br>&nbsp; |
| U6a | (`UpdateAndFill`) an output is **declined** — null, or zero-length where the language cannot spell null. Accepted only where the .yaml marks that output `nullable` (Appendix F) | `TA_BAD_PARAM` | ✅<br>&nbsp; | ✅<br>&nbsp; | ✅<br>&nbsp; | ✅<br>&nbsp; |
| U7 | (`UpdateAndFill`) an output aliases an input, or another output | `TA_BAD_PARAM` | ✅<br>&nbsp; | n/a<br>[12] | ✅<br>&nbsp; | ✅<br>&nbsp; |
| U3 | **Per bar**, after the rules above: the bar is non-finite. `UpdateAndFill` tests each of its `n` bars as the loop reaches it | `TA_BAD_PARAM` | ✅<br>&nbsp; | ✅<br>&nbsp; | ✅<br>&nbsp; | ✅<br>&nbsp; |

**A bar the call accepts as data advances the count, whether the step computes on
it or U3 turns it down.** `OutRange` counts the bars this handle has an output
for, and a bar U3 turned down has one: the previous output, held. So `Value`
always answers for bar `begIdx + count - 1`, however many times a value repeats,
and the handle's *state* is what a rejection leaves untouched — no accumulator
moves — which is the whole point of rejecting rather than computing. `Peek`
produces no output and counts nothing, under any outcome.

The failure is still reported, and it is reported *only* as a failure: no backend
publishes the held value through the call's own result, because Rust's
`Result<f64, RetCode>` cannot carry one alongside `Err` and a rule that three
backends could honour and one could not would be worse than the accessor. `Value`
is how a caller reaches the hold, and whether to accept it or override it is the
caller's business, at the caller's layer — never the stream's internal state.

**U3 is the one rejection that advances `OutRange`.** The others do not: U1/U2 and
U4–U7 are faults in the *call* rather than in the data — no bar was ever handed
over — and they leave the handle exactly as it was.

The mirror case is a function whose *output* is legitimately non-finite: the seven
carrying `TA_FUNC_FLG_NAN_INF_OUT` succeed, so the state **is** touched, `Value`
answers that non-finite value, and `OutRange` advances by one exactly as above.
Both directions leave the caller the same job — override the output at their own
layer, or don't — which is why they are one model and not two rules.

Advancing is what keeps two handles driven off the same feed positionally aligned
when one rejects a bar the other accepts. Without it a caller composing indicators
by hand — the upstream one legitimately producing NaN under `TA_FUNC_FLG_NAN_INF_OUT`,
the downstream one rejecting it — ends up with two handles a bar apart, permanently,
with no error to see and plausible numbers still coming out.

What the caller does about the non-finite value is the caller's, and the streaming
tier deliberately does not decide it: the bar is counted, the error is reported,
and the state is intact and usable for the next bar — so a transient bad print
clears itself.

`UpdateAndFill` is `n` back-to-back `Update`s **stopping at the first error**, the
way a hand-written loop that acts on every failure would. U3 is the one rule in
this document whose rejection leaves output behind: bar `k` being non-finite
commits bars `[0, k)` with their values written, leaves bar `k` and everything
after it uncommitted, and advances the handle's `OutRange` by `k + 1` — `k`
committed bars plus the rejected one. The caller reads the range to
learn where it stopped (the rejected bar is the last one counted), decides what to
do about it, and resumes with the remainder of the series. **Output slot `k` is not
written**, exactly as the loop's final `update` would have written nothing before
breaking; it holds whatever the caller put there. A zero bar count is a success
that does nothing.
Reading the `n` bars as an input array instead — never scanned, `count += n`
unconditionally — was considered and rejected; `docs/streaming-api-design.md`,
"Catching up n bars at once", says why.

**A declination is a property of the call.** U6a is S6a at this tier, and the two
are independent: the set an `UpdateAndFill` declines may differ from the set the
opener was given, in either direction, and may differ again on the next call.
Nothing on the handle records it, and no call is rejected for presenting a set
that differs from the one before it. What declining suppresses is the *write*,
never the *computation* — a declined output is still computed and still reported
by the handle, which is what `MAMA` needs: FAMA feeds the next bar. So
`UpdateAndFill` with every `nullable` output declined is an `Update`.

**A declined output is not an absent one**, and U6a only means anything where the
tier can tell them apart. `UpdateAndFill` is the only place U2 has arguments to
check in Java, and it now checks them — the same `requireArgument` the openers
use, ahead of every length, so an absent array is a `BadParam` naming it rather
than a length read off `null`. Rust and C# still answer `—`: a slice cannot be
absent, and in C# a null array is an empty span, which the length bound rejects.

C alone can decline at the **scalar** entry points as well — `Update` and `Peek`
take an out-parameter per output, where the other three return the value and so
have nothing to decline. It holds wherever the step's write to that output is
guarded, which is every transcribed body; the two hand-rolled tiers (`Dispatch`,
`PeriodBank`) copy the bar through an unguarded assignment and so require every
output, declared `nullable` or not. Nothing shipped combines the two, and marking
an output `nullable` on one of those tiers is the thing that would.

No cross-language gate reaches U6a — every server binds every output — so each
backend carries its own probe beside S6a's (`testBatchArgumentContract` in C,
`tests/stream_open_contract.rs` in Rust, `BatchApiTest` in Java, `StreamApiTest`
in C#), and the four are held to the same emitted shape on the PR gate by
`test_mama_nullable_fama_is_declinable_at_update_and_fill_in_every_backend`. Each
probe drives all four open/fill combinations and reads the declined value back
through the handle, which is what a backend that "supported" declining by not
computing would fail.

**All four backends have a value accessor** since #287: `TA_<N>_Value`,
`value()`, `value()` and `Value`. Each reports the value(s) at the last bar the
stream counted — the bar `OutRange` ends on — without recomputing, and each is a
plain read of state the stream already holds — so the only error surface is C's,
which answers `TA_BAD_PARAM` for a NULL stream or a NULL out-pointer for a
required output (a declinable output may be NULL and is then simply not
written). Java, C# and Rust cannot fail: they take no argument
to reject. The accessor exists because a FORKED stream is the one caller with no
earlier call to have handed it a value — `clone()` gives a second stream at the
same bar, and `peek` would answer for a bar that has not been committed.

**The fork has an error surface in C alone.** `TA_<N>_Clone` answers
`TA_BAD_PARAM` for a NULL stream or a NULL `clone`, and `TA_ALLOC_ERR` if any
allocation fails; on either, `*clone` is NULL and the original is untouched.
Java's and C#'s `clone()`/`Clone()` and Rust's derived `Clone` cannot fail
short of the runtime's own allocation failure, which is not a `RetCode`.

U3 is checked with an explicit finite test, so it rejects NaN and both
infinities alike. Verified: 176 of 176 `Update` and `Peek` entry points check
their bar, and every `UpdateAndFill` applies the same test to every bar it is
handed.

**Reading the range** has an error surface in C alone, where it is a function
rather than a field: `TA_StreamOutRange` answers `TA_BAD_PARAM` for a NULL
handle **and** for either NULL out-parameter. The other three read a field on an
object that cannot be absent.

**One documented hole.** A composed function drives its sub-streams through their
*public* entry points, so a sub-stream re-checks a value the library itself
produced. If such an intermediate were ever non-finite the sub-stream would
reject it, and the rejection would surface after earlier sub-streams in the
pipeline had already advanced — leaving the handle partway through a bar, and
the rejecting sub-stream's own `OutRange` counting a bar the parent's does not,
alongside any sibling that already ran.
Reaching it requires an intermediate to overflow to ±Inf, i.e. input magnitudes
around 1e306. Out of scope by the same reasoning as issue #191; recorded so it is
not rediscovered. All four backends agree on the behaviour, and the reported
error names the sub-stage.

### 2.5 Streaming tier — releasing

| Rule | Condition (in order) | RetCode | C | Rust | Java | C# |
|---|---|---|:---:|:---:|:---:|:---:|
| X1 | Releasing an absent handle is a success no-op | — | ✅ | — | — | — |

Only the C tier has an explicit release. The other three reclaim a handle when it
becomes unreachable, so there is no double-release or use-after-release to
specify.

### 2.6 Configuration tier

Global settings in C; a builder producing an immutable core in Rust, Java and C#.

| Rule | Condition (in order) | RetCode | C | Rust | Java | C# |
|---|---|---|:---:|:---:|:---:|:---:|
| G1 | A setter that names a **single** target rejects the set-all wildcard, and any out-of-domain target | `TA_BAD_PARAM` | ✅<br>&nbsp; | ✅<br>[13] | ✅<br>[13] | ✅<br>&nbsp; |
| G2 | The unstable period is within `[0, MAX_INDEX]` | `TA_BAD_PARAM` | ✅<br>&nbsp; | ✅<br>&nbsp; | ✅<br>&nbsp; | ✅<br>&nbsp; |
| G3 | **Reading** a per-function setting for a target that names no single function | `TA_BAD_PARAM` | ⚠️<br>[14] | ✅<br>&nbsp; | ✅<br>&nbsp; | ✅<br>&nbsp; |
| G4 | The candlestick range type is in domain | `TA_BAD_PARAM` | ✅<br>&nbsp; | —<br>&nbsp; | —<br>&nbsp; | ✅<br>&nbsp; |
| G5 | The candlestick average period is within `[0, MAX_INDEX]` | `TA_BAD_PARAM` | ✅<br>&nbsp; | ✅<br>&nbsp; | ✅<br>&nbsp; | ✅<br>&nbsp; |
| G6 | The candlestick factor is not NaN [17] | `TA_BAD_PARAM` | ✅<br>&nbsp; | ✅<br>&nbsp; | ✅<br>&nbsp; | ✅<br>&nbsp; |
| G7 | The compatibility mode is in domain | `TA_BAD_PARAM` | ✅<br>[15] | —<br>[15] | —<br>[15] | —<br>[15] |
| G8 | A rejected setting leaves the configuration unchanged | — | ✅<br>&nbsp; | ✅<br>[16] | ✅<br>&nbsp; | ✅<br>&nbsp; |

The bounds in G2 and G5 are not arbitrary. Both values are added to a lookback
which is then used as an index: unbounded, the lookback overflows negative and
the function indexes far past the end of its input, while still reporting
success. `MAX_INDEX` is the ceiling the index domain already enforces, and a
warm-up longer than the largest addressable series could never produce output, so
nothing legitimate is refused.

[13] The wildcard is a declared member, so it is rejected by value; a target outside
the declared set is unrepresentable.

[14] C's getter returns the period itself, so it carries no error channel: a target
that names no single function reads as `0`. Accepted, and pinned by
`test_internals.c`; the other three backends reject it because a getter that can
throw costs them nothing.

[15] C rejects an out-of-domain value with `TA_BAD_PARAM` and leaves the mode
unchanged. It previously accepted anything and echoed it back from the getter, so a
caller could not tell a typo from a setting; the two-line domain check was taken even
though `TA_SetCompatibility` is deprecated, because it was that cheap. Rust, Java and
C# expose no public setter at all, so the mode is pinned and the domain cannot be
violated there.

[16] Rust's setters chain and cannot fail individually; each latches the **first**
rejection, which surfaces when the core is built. Verified that a later valid
setter does not clear an earlier rejection.

[17] NaN only: an infinite factor is accepted, in all four backends. A factor
scales a threshold and never indexes anything, so an infinity cannot take a
function off the end of its input; NaN is refused because it silences every
comparison it feeds, which is indistinguishable from "this shape never occurs".
This is the one check on a single value that treats NaN and ±Inf differently.

---

## Part 3 — Undefined behaviour

Where the library stops promising. Nothing here is detected, nothing is
reported, and no gate holds anything about it — the caller is the only one who
can prevent it. Each entry also names the neighbouring cases that *are* defined,
because the boundary is the useful part.

### Non-finite input — NaN and ±Inf

What the library does with a non-finite value depends on **where it arrives** —
not on which function was called.

| Where a non-finite value arrives | What the library does | Rule |
|---|---|:---:|
| Inside an **input array**: a batch input series, or the warm-up history handed to `Open` / `OpenAndFill` | Nothing. **Undefined behaviour** — not detected, not rejected, nothing promised about the output or about a handle opened from it. Do not do it. | — |
| As a **bar** handed to `Update` / `Peek`, or as one of the `n` bars handed to `UpdateAndFill` | **An error**: the bar is non-finite. It is still counted, its output being the previous one held; in `UpdateAndFill` the bars before it stay committed. | U3 |
| As a **real optional parameter** | **An error**: outside the parameter's range. | B3, S3 |
| As a candlestick **`factor`** — a global setting rather than a call parameter | **An error for NaN.** An infinity is accepted, and G6 says why. | G6 |

Only the first row is undefined. The other three are ordinary errors, and
Part 2 is their specification.

### A buffer too short — in C

Rules B5 and S5 are the check C cannot make: it is handed bare pointers and
has no sizes. An input that does not reach `endIdx`, or an output too short for
the count the call produces, is read or written anyway, and the call faults
*inside* the algorithm with the output already partly written. Measured with
guard-paged buffers, where the fault is observable rather than silent corruption
of whatever sits next in the heap.

C alone. Rust returns `BadParam` from the public entry point, Java and C#
reject the call naming the buffer and both sizes, and behind Rust's check the
same bound is asserted for LLVM — a panic, never memory corruption, since the
crate forbids `unsafe`. `OpenAndFill`'s output is checked the same way and for
the same reason (rule S5); C is the one backend that cannot, so an undersized
fill output there lands in this section beside the batch case.

### Buffers that partially overlap

Two buffers over the same memory starting at different offsets (rule N8).
*Unspecified* rather than undefined: what happens is a property of the backend,
and only C can reach the undefined end of it. Detection stops at buffer
identity, which is rule B6; whole-buffer in place is legal and supported, which
is rule N4. Appendix E has the reasoning and the per-backend table.

---

## Appendix A — How each backend spells the seven RetCodes

C and Rust report a code; Java and C# raise. Stated here once so no rule has to.

| `TA_RetCode` | Rust | Java | C# |
|---|---|---|---|
| `TA_SUCCESS` | `Ok(OutRange)` | returns `OutRange` | returns `OutRange` |
| `TA_BAD_PARAM` | `Err(RetCode::BadParam)` | `IllegalArgumentException` | `ArgumentException` |
| `TA_OUT_OF_RANGE_START_INDEX` | `Err(RetCode::OutOfRangeStartIndex)` | `IndexOutOfBoundsException` | `ArgumentOutOfRangeException("startIdx")` |
| `TA_OUT_OF_RANGE_END_INDEX` | `Err(RetCode::OutOfRangeEndIndex)` | `IndexOutOfBoundsException` | `ArgumentOutOfRangeException("endIdx")` |
| `TA_INSUFFICIENT_HISTORY` | `Err(RetCode::InsufficientHistory)` | `InsufficientHistoryException` | `InsufficientHistoryException` |
| `TA_ALLOC_ERR` | `Err(RetCode::AllocErr)` | `IllegalStateException` | `InvalidOperationException` |
| `TA_INTERNAL_ERROR` | `Err(RetCode::InternalError)` | `IllegalStateException` | `InvalidOperationException` |

**S1 names a different argument in a stream.** An opener has no `startIdx`
parameter — it is implied, and always 0 — so where the batch tier's C# rejection
carries `ParamName` `"startIdx"`, an opener's carries the history series
(`"inReal"`, `"inHigh"`, …), which is the argument a caller can actually change.
Type and code are the table's.

S2 is the table's in Java and in C#: both openers answer it from the same frame
as S1, and C#'s throws the `ArgumentOutOfRangeException` the table names rather
than leaving it to the core's shared streaming ladder. Unprobed in both for the
reason rule S2 is ⚠️ there (footnote [5]): reaching it needs a
100 000 001-element array.

**Where the `OutRange` arrives** differs by tier as well as by backend:

| Tier | C | Rust | Java | C# |
|---|---|---|---|---|
| batch | `*outBegIdx`, `*outNBElement` | `Ok(OutRange)` | returns `OutRange` | returns `OutRange` |
| `OpenAndFill` | the same two out-parameters | `Ok((Stream, OutRange))` | on the handle | on the handle |
| any live stream | `TA_StreamOutRange(stream, &beg, &nb)` | `out_range()` | `outRange()` | `OutRange` |

The stream accessor answers the same question in all four: the bars this handle
has an output for. An open over `historyLen` bars starts at `(lookback,
historyLen - lookback)`, and the count saturates at `MAX_INDEX` rather than
overflowing. C has one accessor for every function, since every stream struct
leads with the same two ints. `Open`, `Update` and `Peek` still hand back one
value rather than a range. The range's two members are named for each language:
`beg_idx` / `count` in Rust, `begIdx` / `count` in Java, `BegIdx` / `Count` in
C#.

**One condition has no `TA_RetCode` member of its own.** It reports `TA_BAD_PARAM`, and raises it where a code cannot be returned:

| Condition | C | Rust | Java | C# |
|---|---|---|---|---|
| A buffer is too short (B5) | *cannot detect* | `Err(RetCode::BadParam)` | `IllegalArgumentException` | `ArgumentException(paramName)` |

### Internal errors

An internal error means the library found an inconsistency in its **own** state.
It is a bug in TA-Lib, not in the call: nothing the caller changes will avoid it,
and it is worth reporting. Twelve batch functions per backend carry such a guard,
and none is reachable today — they test a period below 1, which B3's declared floor
already refuses. A stream opener carries one more for each ring, window,
circular buffer and rolling-extremum span it captures, each checking that span
against the history it was handed, and a dispatching or dual-mode opener one for
the arm its own `Open` has already refused.

C returns `TA_INTERNAL_ERROR + <site id>`, not the bare member, at **every** one
of them — the id names the guard that fired, and
`ta_codegen/input/internal_error_ids.yaml` maps it back to the function and the
state field it guards. `TA_INTERNAL_ERROR` is 5000 and `TA_UNKNOWN_ERR` is the
only member above it, so a C caller must test `>= TA_INTERNAL_ERROR` rather than
`==`. The other three carry no id: Rust returns `Err(RetCode::InternalError)`,
and Java and C# raise `IllegalStateException` / `InvalidOperationException` with
`TA_INTERNAL_ERROR` recoverable from the thrown object.

### The raised form carries the code

The table above is many-to-one in two places — one `IndexOutOfBoundsException`
serves both index codes, one `IllegalStateException` both library-side ones —
and the same is true of C#'s `InvalidOperationException`. A caller who cannot
tell two conditions apart cannot respond to either, which is what Appendix D
is about, so **every exception Java and C# raise carries its `TA_RetCode`**:
`TaLibFailure.retCode()` in Java, `ITaLibFailure.RetCode` in C#.

The types stay exactly as the table says. They are subclasses
(`TaLibIndexException extends IndexOutOfBoundsException`, and so on), so an
existing `catch` is unaffected; only the recovery of the code is new. Two
conditions C has no code for — an absent argument and a buffer too short —
report `TA_BAD_PARAM`, the code C answers for an argument it *can* detect, which
is what makes the mapping **total** over the tiers this document specifies —
batch (Part 2.2) and streaming (2.3, 2.4): there is no failure either backend
raises from those from which a code cannot be recovered. And it is **lossless**:
no two codes share one thrown representation.

**Two entry points are outside that**, and deliberately: the global-settings
builders (the G-rules) and the metadata/dynamic-binder tier, which Appendix C
defers. Both still raise plain platform types for their *own* refusals — an
unbound slot, a wrong kind, a slot index out of range — and neither of those is
a function call, so neither has a `TA_RetCode` a caller would be recovering. The
binder's *dispatch* is a different matter since #265: it calls the public entry
point, so an indicator's rejection reaches the caller carrying its code in every
backend.

Rust needs none of this — it returns `Err(RetCode)` — and C is the vocabulary.

### The backend enums are not the same subset

| Backend | Members | Notes |
|---|---|---|
| C | 20 | 7 function-tier, 13 abstract-tier, of which 4 are returned nowhere |
| Rust | 7 | exactly the function tier |
| Java | 7 | exactly the function tier |
| C# | 9 | the seven, plus `InputNotAllInitialize` / `OutputNotAllInitialize` (C's 10 and 11), which its metadata tier produces |

A backend carrying a member it never produces is harmless; a backend *missing*
one it needs is not. Worth re-checking whenever the abstract tier is specified.

### Message prefixes

Batch tier: `TA_<NAME>: ` in C#, `<NAME>: ` in Java. Streaming tier:
`<NAME> <verb>: ` in both, which `docs/streaming-api-design.md` fixes as a
cross-language contract.

**`<NAME>` may be an inner function.** A composed function calls its callee's
public entry point — in every backend since #267 — so a rejection the callee
detects carries the *callee's* name: a caller of `MACDEXT` can see
`MA: bad parameter`. (Rust's rejection is a bare `Err(RetCode)` and carries no
name at all, so the attribution question is Java's and C#'s alone.) Accepted rather than
worked around — a sub-function failure naming the sub-function is
understandable, and restoring the outer name would mean catching and rethrowing
at every cross-call, which is the machinery calling the public tier exists to
remove. It is narrow in practice: every outer function validates its own
parameters before it cross-calls, so reaching it needs a fault the outer
prologue does not screen for first.

**One surface gap, not an error-handling one:** the parameter-default sentinels
are public API in C (`TA_REAL_DEFAULT`, `TA_INTEGER_DEFAULT`), Java
(`Core.REAL_DEFAULT`, `Core.INTEGER_DEFAULT`) and Rust
(`Core::REAL_DEFAULT`, `Core::INTEGER_DEFAULT`), but `internal` in C#. A C#
caller has no supported way to write rule N3 for a real parameter.

---

## Appendix B — How the marks were produced

Each ✅ rests on two independent checks; neither alone is enough.

1. **A runtime probe against the built artifact** — one program per backend,
   driving the scenarios above through the *public* API and printing what came
   back. Representative functions: `SMA` (one input, one integer parameter, one
   output), `BBANDS` (three outputs, a real and an enum parameter), `MAMA` (real
   parameters with a narrow domain), `MININDEX` (integer output), `SQRT` (a
   documented output-domain hole). C's memory-unsafe rules are probed with
   guard-paged buffers, so a read or write past a declared length faults instead
   of silently corrupting neighbouring memory, and each such scenario runs in a
   forked child so a crash is an observation rather than the end of the run. An
   allocation failure (B7) is probed with an interposed `malloc` that fails above a
   size threshold, against a non-allocating control that must still succeed.

2. **A structural check over the whole generated corpus** — a probe on one
   function says nothing about the other 173. Verified mechanically, from the
   generated sources:

   | Claim | Result |
   |---|---|
   | C batch: `startIdx` guard, then `endIdx` guard, before any other return | 176 / 176 |
   | C batch: parameter validation, then every input, the `OutRange` pointers and every output null-checked, inputs before outputs | 352 / 352 |
   | Rust batch: `startIdx` guard → `endIdx` guard → lookback (which returns B3) → every input, then every output length-checked | 176 / 176 |
   | Rust numerics: `startIdx` guard → `endIdx` guard → bounds asserts (following the FMA dispatcher to the real core) | 176 / 176 |
   | Java batch: clamp (which raises B3), then every length check, then the core | 352 / 352 |
   | C# batch: clamp, then every length check, then the core | 352 / 352 |
   | C# cores carrying an overlap guard wherever one is expressible | no core unguarded where the type expresses it |
   | Short-history arm reports `TA_INSUFFICIENT_HISTORY` | 176 streaming functions per backend, no backend mixing it with anything else |
   | Empty-history arm reports `TA_OUT_OF_RANGE_START_INDEX` | every opener arm in all four backends, no backend mixing it with anything else |
   | Java public opener: the history's null test, then the index pair, then every other argument | 176 `Open` + 176 `OpenAndFill` |
   | C# public opener: an empty history is the index fault, ahead of every other input | 176 `Open` + 176 `OpenAndFill` |
   | Rust/Java/C# public `OpenAndFill`: every output bounded by `historyLen - <N>_Lookback(...)` | 176 per backend |

   Every 352 is the 176 definitions in `ta_codegen/input/` × the double and
   float overloads, so the float surface is covered by the same evidence.

Most of these probes are not committed. They are throwaway drivers: the shipped
gates cover values, and these cover the failure paths once, to produce this
table. Re-running them means rewriting them — cheap, and honest about the fact
that a mark is a statement about a moment.

The exceptions are the ones a rule names under **Implementation**: those cover a
condition no value gate reaches, so they are committed and run.

---

## Appendix C — Tiers not yet covered

Named so their absence is deliberate rather than an oversight.

- **Abstract / metadata tier** — function-handle lookup, parameter holders, the
  generated catalogues. Has its own error surface in all four backends; not yet
  specified here. One thing about it is settled and worth writing down: **its
  dispatch calls the public entry point in all four**, so every rule above holds
  through it unchanged (#265) — except in C, whose setters take a bare pointer
  and carry no length, so a leg bound to a buffer shorter than the requested
  range goes undetected there where the other three answer `TA_BAD_PARAM`. A
  second thing is settled: **a rejected setter leaves the holder as it found
  it** in all four, the price setter validating every consumed component before
  it writes any (#266). That is §1's "only the range it reports is written"
  applied to a holder's bound state, and the case that makes it worth stating
  is a *re-bind*, where the partial write is not masked by the
  unbound-component report: the next call then succeeded over a mixture of two
  bundles — in C# with no code and no exception. What is still
  unspecified is the tier's *own* surface: unbound and mis-typed arguments, and
  handle lookup.
- **JSON-RPC servers** — a test harness, not a shipped API. Their error
  behaviour is a property of the harness.
- **`Copy` / `Clone` of a stream handle, and concurrent use** — the concurrency
  contract lives in `docs/streaming-api-design.md`; only its *error* surface
  would belong here, and it has none beyond the rules above.

---

## Appendix D — Open items

Every ❌ above, collected — there are none left — plus a message-level deviation
(item 7) and one that no rule row can carry: a divergence on a call every rule
says is legal (item 11). Numbering is append-only — a retired item leaves a gap
rather than renumbering the rest. Each was measured, not inferred.

A `⚠️` is not tracked here. It marks a deviation that is deliberate, or a rule
implemented but not covered by a CI probe; the rule's own footnote says which.
The four places C carries one for a buffer size (B5, S5, U5, U6) are the same
fact each time: C is handed bare pointers and has no sizes to check against.

| # | Backend | Rule | Defect |
|---|---|---|---|
| ~~1~~ | C | B4 | *Fixed.* The `OutRange` pointers were not null-checked, so passing either as null segfaulted. The batch prologue now checks them, as the streaming `OpenAndFill` prologue always has. |
| ~~2~~ | C | B4 | *Fixed.* Input-buffer presence was checked *before* parameter validation and output presence after, so the prologue straddled B3. Parameter validation now precedes every presence check. |
| ~~3~~ | Java | B4 | *Fixed.* Buffer presence was checked *before* the index and parameter rules, inverting the specified precedence — a negative `startIdx` with a null input reported the null. The wrapper now evaluates B1, B2 and B3 first. |
| ~~4~~ | Java | B3 | *Fixed.* A null enum parameter yielded a raw JVM `NullPointerException` naming neither function nor parameter; it is now a parameter outside its domain, named, and carrying `TA_BAD_PARAM`. |
| ~~5~~ | — | — | *Withdrawn, not fixed.* Partial output↔input overlap in C. Decided in #225: detection stops at buffer identity, and partial overlap is unspecified — rule N8 and Appendix E. Numbering left as-is so existing references to items 8 and 9 keep pointing at the same rows. |
| ~~6~~ | Java | S4 | *Fixed.* A null history, or a null `OpenAndFill` output, yielded a raw JVM exception from inside the algorithm — the length was read straight off the array, and an output faulted in the fill loop. The public openers now check every argument, and item 13 fixed the order they are checked in. |
| ~~7~~ | C# | S1 | *Fixed.* The empty-history *message* omitted the cross-language `<NAME> open: ` prefix. Taken with item 13, as predicted: the two were faults of one line. |
| ~~8~~ | all | S7 | *Fixed.* `TA_RetCode` had **no member** for "history shorter than the lookback", so C and Rust fell back to the catch-all and Java and C# borrowed `TA_OUT_OF_RANGE_END_INDEX`. `TA_INSUFFICIENT_HISTORY = 17` was appended and all four now report it. The borrowed code took `MAX_INDEX + 1` history (S2) down with it — see footnote [8]. |
| ~~9~~ | Rust, Java, C# | S5 | *Fixed.* `OpenAndFill` validated no output capacity, unlike the batch tier which does, so an undersized output faulted inside the fill with the buffer already partly written — a raw index exception in Java and C#, a panic in Rust. The public frame now bounds every output by `historyLen - <N>_Lookback(...)`, the count the fill writes. (C still cannot — no sizes.) Rust used to be a partial exception by accident: its `OpenAndFill` distinctness guard rejected two *empty* outputs before the fill could fault, so that one undersized shape answered `BadParam` where every other answered a panic — and where C# faulted, its `Overlaps` being false for an empty span. #262 excluded empty operands from both guards, and now the capacity check answers that shape and every other one alike. |
| ~~10~~ | C | G7 | *Fixed.* `TA_SetCompatibility` now returns `TA_BAD_PARAM` for a value outside the enum instead of latching it. The function stays deprecated — this was taken only because it was a two-line domain check. |
| ~~11~~ | C#, Rust | B6 | *Fixed.* Two **empty** output buffers were rejected as aliased. C# said so explicitly (`a.IsEmpty && b.IsEmpty` was a clause of the guard); Rust did it incidentally, because the guard compared `as_ptr()` and two zero-capacity allocations answer the same dangling value (a slice of a longer buffer truncated to zero would not, so Rust rejected *some* empty pairs and accepted others — which is worse than either). C and Java accepted them. The call is legal by rule N1 and by B5's own wording — on a range shorter than the lookback *any output length will do, including none* — so this was a four-way divergence on a call the specification says all four accept. Measured on `ACCBANDS(0, 251, …, optInTimePeriod 253, …)` with three distinct zero-length outputs: `TA_SUCCESS` in C and Java, `BadParam` in Rust and C#. Both guards now require **both** operands to be non-empty — two zero-length buffers cannot clobber each other — which is also what makes "declined" spellable in C#, where an empty span is the only way to say it (rule B6a, #262). The empty triple is now a probe in each backend's own suite; no cross-language gate can see it, because the servers bind every output and floor its length at one. |
| ~~13~~ | all | S1 | *Fixed.* An empty history answered `TA_BAD_PARAM` where S1 specifies `TA_OUT_OF_RANGE_START_INDEX`, and the index pair was not evaluated first: C checked argument presence (S4) ahead of it, so a call that was both an absent output and an empty history reported S4's code — and a caller who fixed that argument got the same rejection back for a reason nothing had mentioned. All four openers now answer the pair ahead of every presence check, except for the one check each language makes a precondition of reading the length at all (footnote [4]). What was measured on a zero-length history before: `TA_BAD_PARAM` in C, `Err(BadParam)` in Rust, `TaLibArgumentException` carrying `BadParam` in Java, `ArgumentException` in C#. |

**Nothing is left.** Items 6, 7 and 13 went with #268, which took all three at
once because they were the same prologue; item 9 followed it, for the same
reason — the frame the three of them had just been moved onto is the frame that
knows the history's length, and calling `<N>_Lookback` there is what turns that
into a width.

The two that changed what a *correct* caller could do are both closed: item 8
let a caller tell "send more bars" from "your code is wrong" and retry, and item
9 stopped a sizing mistake from becoming a partly-written buffer.

This appendix is empty of open items for the first time. Numbering stays
append-only, so the next one starts at 14.

---

## Appendix E — Buffer overlap

**What is guaranteed.** Two *outputs* that are the same buffer are rejected
(rule B6). That is a caller error with no correct answer — the second write
destroys the first — and it is cheap to see, because it is an identity test.
Two *empty* outputs are not rejected: a zero-length buffer cannot clobber
anything, so both guards that can express the case require both operands to be
non-empty (Appendix D item 11, #262). A call that is both undersized and
identical answers B5, the earlier rule — the question #261 asked.

**What is allowed.** An output that **is** an input, whole buffer, is legal in the
batch tier (rule N4). Several bodies are written for it and elect their scratch by
testing for exactly that case. This is not tolerated-but-discouraged; it is
supported.

**What is unspecified.** Anything in between — the same memory reached through
buffers that start at different offsets (rule N8). Not detected, not promised, and
not a diagnosis you will get. Decided in **#225**: the library detects buffer
identity and nothing finer.

**Why the line is drawn at identity, and not further.** The four backends do not
even *agree on whether a partial overlap can exist*, so a stronger rule could not be
one rule:

| backend | can a caller express a partial overlap? | detected? |
|---|---|---|
| C | Yes — two pointers into one allocation | **No.** Detecting it means ordering pointers into different declared objects, which C leaves undefined; a conforming check would have to launder them through `uintptr_t` and reason about representation |
| Java | **No** — two arrays are the same object or disjoint; there is no offset to differ | n/a, so the run-time reference-equality check is complete |
| Rust | **No** in safe code — `&[T]` and `&mut [T]` over the same data cannot coexist, and two `&mut` cannot either | the `as_ptr()` identity guard never fires in safe code at all; it is kept for parity, and for the FFI boundary |
| C# | **Yes** — two `Span<T>` slices of one array | **Yes**, incidentally: `Span.Overlaps` exists, so the check is one call |

So the guarantee is set by the weakest member that can express the problem, which is
C — and C is the one language where the check is not merely expensive but not
straightforwardly expressible. Java and Rust satisfy the stronger rule for free by
making the state unreachable, which is not the same as enforcing it.

**Outputs of different element types: C compares them, the other three cannot.**
A `double` output and an `int` output can only be the same buffer through a cast,
and three of the four backends cannot express the comparison at all —
`double[] == int[]` is "incomparable types" in Java, `*const f64 == *const i32`
is a type error in Rust, and C#'s `Overlaps` is not defined across element types.
In C the pair is expressible and **is** checked: the guard compares both through
`const void *`, which is well defined and is not the `double * == int *`
constraint violation that reading suggests. C's streaming frames have always done
this; the batch tier joined them with `SUPERTREND` (#272).

**Why C is allowed to detect more here.** Two claims used to close this paragraph
and both are retired: that nothing in the corpus mixes the two types, and that
the frames assert nothing does. `SUPERTREND` mixes them, and the frames carry no
per-function `outIsInteger` flag at all — `ta_variant_frame` indexes `outReal[]`
and `outInteger[]` by the output's declaration position and has handled the mixed
case since the SYNTH12 fixture. So skipping the pair stopped being "what the
emitters must do" and became a hole a C caller could fall into: every *same*-typed
pair answers `TA_BAD_PARAM`, and the one pair a caller has to cast to build
answered `TA_SUCCESS` and wrote through both. Detecting it is the same asymmetry
C# already carries below for a partial input↔output overlap — a superset of the
guarantee, kept because the language can answer the question cheaply. **Callers
must not rely on it**, for the same reason: three backends cannot say it.

**Still not detected, in any backend:** two outputs of different types that
*partially* overlap. That is rule N8, and it is not special to mixed types —
`TA_BBANDS` with its three bands one element apart returns `TA_SUCCESS` and
writes a wrong upper band on every bar, no cast required. B6 catches buffer
identity; everything finer is unspecified.

**Test coverage:** `checkOutputAliasRejected` (`test_abstract.c`) sweeps every
ordered output pair of every function, cross-typed pairs included, binding both
onto one buffer and requiring `TA_BAD_PARAM`.

**C# currently detects more than this specifies.** Its generated guard is
`if (outReal.Overlaps(inReal) && outReal != inReal)`, which rejects a partial
input↔output overlap while still allowing whole-buffer in place. That is a superset
of the guarantee, kept because it costs one call on a type that already answers the
question. **Callers must not rely on it**: the same call is unspecified in C, and
inexpressible in Java and Rust. If uniformity is ever preferred over the extra
safety, removing it is the change — not adding the check elsewhere.

---

## Appendix F — Omitted outputs

An indicator may declare an output a caller can decline. `MAMA`'s `outFAMA` is
the only one today, marked `flags: [nullable]` in `ta_codegen/input/mama/mama.yaml`,
and it surfaces through `ta_abstract` as `TA_OUT_NULLABLE`. Declining it is not
the same as passing a buffer too small: the caller is saying *do not write this*,
so the presence and capacity checks are skipped for that output and the body's
writes to it are guarded instead. Everything else is unchanged — the value is
still computed where the algorithm needs it, and the other outputs are
bit-identical to the same call with the output supplied.

**The opener honours it too.** A streaming `OpenAndFill` accepts a declined output
exactly as the batch call does: rule S5's capacity bound is skipped for it, the
fill's writes to it are guarded, and the handle still reports its value — the
handle caches what the guarded store would have written, so `value()` is the
same whether the output was supplied or not. `MA`'s `MAMA` arm is the caller
that proves it: it declines `outFAMA` outright in all four backends, where three
of them used to allocate a `historyLen`-sized buffer per open and throw it away.
**`UpdateAndFill` honours it on the same terms** (rule U6a): U6's capacity bound
is skipped for a declined output, the fill's store to it is guarded, and the
value is still computed and still on the handle. The choice is the call's — see
2.4 — so the four open/fill combinations are all ordinary calls.

**How a caller spells "omitted" is not the same in every language.** Three of the
four can say it outright; C# cannot, and does not need to:

| backend | "omitted" is | can it be told apart from "empty"? |
|---|---|---|
| C | `NULL` | **Yes** — `NULL` is not a valid zero-length buffer |
| Rust | `None`, the output being `Option<&mut [T]>` | **Yes** |
| Java | `null` | **Yes** — `null` is not `new double[0]` |
| C# | an empty `Span<T>` | **No** — a `Span<T>` is a ref struct and a null array converts to an empty span |

C# is the exception because the type system leaves it no alternative, and the
collapse costs nothing: an output that is declined is written to no more than one
that has nothing to hold. What C# does keep is the distinction that matters — the
length check is skipped for a nullable output that arrives empty and applied to it
otherwise, so a sizing mistake on a *supplied* output is still an error.

Rust could have joined C# by reading a zero-length slice as the declination, and
deliberately does not. `Option` is the shape a Rust caller expects, it makes the
declination visible at the call site, and it keeps `&mut []` for a non-nullable
output what it has always been: a sizing mistake, which B5 answers.

**An omitted output is not an alias.** Two declined outputs are not the same
buffer, and neither are two empty ones — a write to one cannot clobber the other,
because neither is written. So rule B6's pair guard skips a pair whose operands
are not both present and non-empty. C and Java guard each nullable operand
non-null before comparing (two `NULL`s compare *equal*); C#'s `Overlaps` already
answers false for an empty span; Rust requires both slices non-empty, which also
closed the divergence in Appendix D item 11.

**None of this is visible to a cross-language gate.** The JSON-RPC servers bind
every declared output and floor its length at one, so a backend that went back to
requiring `outFAMA` — or to rejecting distinct empty buffers — would stay green in
`--codegen`, `--xlang-hash` and `--fuzz-064` alike. Each backend therefore carries
its own probe: `testBatchArgumentContract` (C), `tests/nullable_outputs.rs` and
`tests/stream_open_contract.rs` (Rust), `BatchApiTest` (Java) and `BatchApiTest`
plus `StreamApiTest` (C#). Each compares a declining call against
the same call with the output supplied, rather than only asserting that it was
accepted — a body that stopped computing the value, or took a different path
without it, would pass the weaker check.

**More than one nullable output per function** is generated but unshipped: the
corpus has one, and the pair guard branches on which of a pair is nullable, so
three of its four arms would be unreachable. `SYNTH10`
(`ta_codegen/generator/input_synth/`) is the fixture that reaches them — three
outputs, two of them declinable, driven end to end through all four backends by
`scripts/synth_gate.py`.
