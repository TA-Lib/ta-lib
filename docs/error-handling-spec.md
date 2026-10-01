# API Error-Handling Conformance

The error-handling rules are published at https://ta-lib.org/spec/ (source
`website/src/spec/`) under the ids used below. This file states no rule. Per id,
it tracks whether each backend conforms, what tests it, and why the rule is what
it is. A change to a rule edits the public page; a change here edits a mark, a
test or a reason.

The columns are a conformance tracker, one per shipped backend. A new backend
adds a column; it does not change a rule.

## Marks

| Mark | Meaning |
|:---:|---|
| ✅ | Verified: a probe against the built artifact produced the specified behaviour. |
| ⚠️ | Deviates deliberately, **or** is implemented but not verified by the CI test suite. The rule's footnote says which, and where a deviation was decided. |
| ❌ | Measured, and does **not** conform. Needs a fix. Collected in Appendix D. |
| — | Not applicable: the condition cannot be expressed in this backend's types. |
| *(blank)* | Not yet verified. |

`—` is not a pass. It means the language removes the failure mode (a Rust slice
cannot be null, a C# `Span<T>` cannot be absent, an enum has no out-of-domain
value), so there is nothing for the backend to check and nothing for a caller to
hit.

## Conformance

Rows follow the public tables' order. Footnote numbers are append-only: a
retired footnote leaves a gap, so a citation of `[4]` keeps its meaning.

### Lookback tier

| Rule | Topic | C | Rust | Java | C# |
|---|---|:---:|:---:|:---:|:---:|
| L1 | [rejection signal](https://ta-lib.org/spec/lookback/#l1) | ✅ | ✅ | ❌<br>[18] | ✅ |
| L2 | [agrees with batch and Open](https://ta-lib.org/spec/lookback/#l2) | ✅ | ✅ | ❌<br>[18] | ✅ |
| L3 | [agrees with C](https://ta-lib.org/spec/lookback/#l3) | — | ✅ | ✅ | ✅ |
| L4 | [nothing else fails](https://ta-lib.org/spec/lookback/#l4) | ✅ | ✅ | ❌<br>[18] | ✅ |
| L12 | [display shift rejects what the lookback rejects](https://ta-lib.org/spec/lookback/#l12) | ✅ | ✅ | ❌<br>[18] | ✅ |

[18] A null MA type: Appendix D item 14.

### Batch tier

| Rule | Topic | C | Rust | Java | C# |
|---|---|:---:|:---:|:---:|:---:|
| B1 | [start index](https://ta-lib.org/spec/errors/#b1) | ✅ | ✅ | ✅ | ✅ |
| B2 | [end index](https://ta-lib.org/spec/errors/#b2) | ✅ | ✅ | ✅ | ✅ |
| B3 | [parameter value](https://ta-lib.org/spec/errors/#b3) | ✅ | ✅ | ✅ | ✅ |
| B4 | [absent argument](https://ta-lib.org/spec/errors/#b4) | ✅ | —<br>[1] | ✅ | —<br>[2] |
| B5 | [buffer length](https://ta-lib.org/spec/errors/#b5) | ⚠️<br>[3] | ✅ | ✅ | ✅ |
| B6 | [one buffer, two outputs](https://ta-lib.org/spec/errors/#b6) | ✅ | ✅ | ✅ | ✅ |
| B6a | [omitted output](https://ta-lib.org/spec/errors/#b6a) | ✅ | —<br>[20] | ✅ | —<br>[20] |
| B7 | [allocation failure](https://ta-lib.org/spec/errors/#b7) | ⚠️<br>[23] | ⚠️<br>[23] | ⚠️<br>[23] | ⚠️<br>[23] |
| B8 | [internal error](https://ta-lib.org/spec/errors/#b8) | ⚠️<br>[21] | ⚠️<br>[21] | ⚠️<br>[21] | ⚠️<br>[21] |

[1] Rust slices cannot be null, and what C writes through a pointer (the
index/count pair, a stream handle) is returned instead.

[2] C# `Span<T>` cannot be absent. A null array converts to an empty span, so
absence surfaces as a zero length: B5 in the batch tier, S1 for a stream's
history. A zero-length output is S5's case.

[3] C has no sizes to check against: a property of the ABI, not a defect.

[20] Rust takes a non-declinable output as `&mut [T]`, which cannot be omitted.
C# has no check of its own (footnote [2]).

[21] Implemented, but the individual sites are not tested: no reachable input
fires one (rationale B8).

[23] Deliberately untested: only C returns it, and nothing past it is defined
(rationale B7).

### Stream opening

| Rule | Topic | C | Rust | Java | C# |
|---|---|:---:|:---:|:---:|:---:|
| S1 | [empty history](https://ta-lib.org/spec/streaming/#s1) | ✅<br>[4] | ✅<br>[4] | ✅<br>[4] | ✅<br>[4] |
| S2 | [history too long](https://ta-lib.org/spec/streaming/#s2) | ✅<br>[5] | ⚠️<br>[5] | ⚠️<br>[5] | ⚠️<br>[5] |
| S3 | [parameter value](https://ta-lib.org/spec/streaming/#s3) | ✅ | ✅ | ✅ | ✅ |
| S4 | [absent argument](https://ta-lib.org/spec/streaming/#s4) | ✅ | —<br>[1] | ✅ | —<br>[2] |
| S5 | [buffer length](https://ta-lib.org/spec/streaming/#s5) | ⚠️<br>[3] | ✅ | ✅ | ✅ |
| S6 | [aliasing](https://ta-lib.org/spec/streaming/#s6) | ✅ | —<br>[22] | ✅ | ✅ |
| S6a | [declined output](https://ta-lib.org/spec/streaming/#s6a) | ✅ | —<br>[20] | ✅ | —<br>[20] |
| S7 | [short history](https://ta-lib.org/spec/streaming/#s7) | ✅ | ✅ | ✅ | ✅ |

S8 is withdrawn and its id is never reused (rationale I5).

[4] Each backend's check ahead of S1 differs because each is a precondition for
evaluating the pair rather than an argument competing with it. C publishes "no
handle on any failure" through `*stream`, and has nowhere to publish it without
one. Java cannot read a length from an array that is not there. A Rust slice
cannot be absent, and a C# null array arrives as an empty span, so S1 is how an
absent history is reported (footnote [2]).

[5] Implemented in all four (the bound is in the opener's own prologue; Java and
C# answer it from the same frame as S1) but probed only in C, which takes
`historyLen` as a bare `int`, so the rejection answers before a bar is read. The
other three derive the length from the array they are handed, so provoking it
needs a 100 000 001-element allocation; the legal upper edge, a history of
exactly `INDEX_MAX + 1` bars, is out of reach everywhere for the same reason.

[22] Cannot be provoked. `OpenAndFill` takes each output as its own `&mut` slice,
so two outputs, or an output and the input, cannot name the same buffer while the
call is live. The batch tier emits B6's pointer comparison anyway; the streaming
tier emits nothing.

### Stream advancing

| Rule | Topic | C | Rust | Java | C# |
|---|---|:---:|:---:|:---:|:---:|
| U1 | [absent handle](https://ta-lib.org/spec/streaming/#u1) | ✅ | — | — | — |
| U4 | [index ceiling](https://ta-lib.org/spec/streaming/#u4) | ✅ | ✅ | ✅ | ✅ |
| U2 | [absent output](https://ta-lib.org/spec/streaming/#u2) | ✅ | — | ✅ | — |
| U6a | [declined output](https://ta-lib.org/spec/streaming/#u6a) | ✅ | n/a<br>[10] | n/a<br>[10] | n/a<br>[10] |
| U3 | [non-finite bar](https://ta-lib.org/spec/streaming/#u3) | ✅ | ✅ | ✅ | ✅ |

[10] The other three have no component to decline at this tier: Rust and C#
return the value, and Java writes every field of a caller-owned sink.

### Stream release

| Rule | Topic | C | Rust | Java | C# |
|---|---|:---:|:---:|:---:|:---:|
| X1 | [`Close(NULL)`](https://ta-lib.org/spec/streaming/#x1) | ✅ | — | — | — |

Only C has an explicit release. The other three reclaim a handle when it becomes
unreachable, so there is no double release or use after release to track.

### Configuration

| Rule | Topic | C | Rust | Java | C# |
|---|---|:---:|:---:|:---:|:---:|
| G1 | [target](https://ta-lib.org/spec/settings-threads/#g1) | ✅ | ✅<br>[13] | ✅<br>[13] | ✅ |
| G2 | [unstable period bound](https://ta-lib.org/spec/settings-threads/#g2) | ✅ | ✅ | ✅ | ✅ |
| G3 | [reading](https://ta-lib.org/spec/settings-threads/#g3) | ⚠️<br>[14] | ✅ | ✅ | ✅ |
| G4 | [range type](https://ta-lib.org/spec/settings-threads/#g4) | ✅ | — | — | ✅ |
| G5 | [average period bound](https://ta-lib.org/spec/settings-threads/#g5) | ✅ | ✅ | ✅ | ✅ |
| G6 | [NaN factor](https://ta-lib.org/spec/settings-threads/#g6) | ✅ | ✅ | ✅ | ✅ |
| G7 | [no change on refusal](https://ta-lib.org/spec/settings-threads/#g7) | ✅ | ✅<br>[16] | ✅ | ✅ |

[13] The wildcard is a declared member, so it is rejected by value; a target
outside the declared set is unrepresentable.

[14] C's getter returns the period itself, so it has no error channel. Accepted,
and pinned by `test_internals.c`; the other three reject it because a getter
that can throw costs them nothing.

[16] Verified that a later valid setter does not clear an earlier latched
rejection.

## Test coverage

### L1 to L4

`xlang_lookback_leg`, `xlang_tier_native_check`, `xlang_tier_gold_check`
(regtest `--xlang-hash`).

### L12

`abstract_check_display_shift` (`test_abstract.c`; against each server under
regtest `--codegen`, in-process in a bare run).

### B1 to B8

`testIndexRange`, `checkOutputAliasRejected`, `testBatchArgumentContract`,
`c_batch_prologue_orders_parameters_before_presence`,
`rust_public_entry_orders_the_argument_contract`,
`rust_batch_impl_orders_capacity_before_aliasing`,
`every_declared_input_is_checked_in_every_backend`, `BatchApiTest` (Java, C#);
no phantom io and `--xlang-hash` for behaviour matching.

B6: `checkOutputAliasRejected` (`test_abstract.c`) sweeps every ordered output
pair of every function, cross-typed pairs included, binding both onto one buffer
and requiring `TA_BAD_PARAM`, once over the full range and once over a range
that produces no values: C's form of one zero-length array passed as two outputs
(Appendix D item 15), with the pair rebound apart as that leg's control.
Java's and C#'s `BatchApiTest` pass one zero-length array as two outputs.

### S1 to S7

S1, S2, S4, S5, S6a and S7 are mapped; S3 is not yet, and S6 only for one buffer
passed as two outputs: `testBatchArgumentContract` passes one buffer as
ACCBANDS's first two `OpenAndFill` outputs on a history of lookback bars
(`TA_BAD_PARAM`), with a separate-outputs control answering S7; Java's and C#'s
`BatchApiTest` do the same with one zero-length array.

`testStreamShortHistory` drives S1, S2's rejecting side and S7 in C: 9, 3 and 8
rejections, with S7's 16 controls. Three of S1's cases are *also* an absent
argument, which is what makes them about the order and not only the code.
`testBatchArgumentContract` drives S4 through B4's own argument shapes plus the
handle: 12 rejections and 5 controls, counted apart from B4's. `BatchApiTest`
does the same for Java, and adds S1's; `StreamApiTest` covers S4 in C#, where
the condition is a zero-length span and so is S1. Rust cannot express S4, and
`tests/stream_open_contract.rs` covers S1 there.

`scripts/check_stream_retcodes.py` carries S1 and S7 over the whole generated
corpus in all four backends; a probe names one function, and this is what covers
the rest. It reads the *core's* arm, which in Java and C# the public frame makes
unreachable, so those two frames are covered corpus-wide by
`java_public_openers_check_arguments_then_the_index_pair` and
`csharp_public_openers_reject_an_empty_history_as_an_index_fault` instead.
`an_opener_never_answers_the_code_its_sub_call_handed_back` pins S7 for composed
openers in the three ported backends; C is outside that sweep (rationale S7).

S5 is probed from **both sides** in each of the three backends that can express
it (an exactly sized output accepted and filled, one element shorter rejected),
because only the pair pins the arithmetic: a bound of `historyLen` would reject
the first, and no bound at all would accept the second. Each also drives the
tiers that hand-roll their fill (the dispatch tier including its identity arm,
the period bank, and a composed multi-output, whose sub-calls fill scratch of
their own rather than the caller's arrays), and asserts that a rejected fill left
the buffer untouched. Corpus-wide, the width is pinned by the three
`*_public_*fill*` / `*_public_openers_*` gates in `open_validation_suite.rs`,
which require it to be read from the function's own lookback rather than from
the history's length, that the bound REJECT rather than merely exist, and that a
`nullable` output be bounded conditionally while every other output is not.

### S6a, U6a and B6a

No cross-language gate reaches a declination: the JSON-RPC servers bind every
declared output and floor its length at one, so a backend that went back to
requiring `outFAMA`, or to rejecting distinct empty buffers, would stay green in
`--codegen`, `--xlang-hash` and `--ref` alike. Each backend therefore carries
its own probe: `testBatchArgumentContract` (C), `tests/nullable_outputs.rs` and
`tests/stream_open_contract.rs` (Rust), `BatchApiTest` (Java) and `BatchApiTest`
plus `StreamApiTest` (C#).

Each compares a declining call against the same call with the output supplied,
rather than only asserting that it was accepted: `MAMA_OpenAndFill` with
`outFAMA` declined must leave the other output and the reported range
bit-identical to the supplied run, and must still report FAMA through the
handle. That is the check a backend fails if it "supports" declining by not
computing the value. Both bounds stay armed on that call: an undersized
`outMAMA` beside a declined `outFAMA` is still rejected, and so is an undersized
`outFAMA` that was supplied.

`test_mama_nullable_fama_is_declinable_at_the_opener_in_every_backend` pins the
emitted opener shape in all four on the PR gate, because the runtime probes are
nightly. U6a itself is C's alone: `testBatchArgumentContract` drives all four
open/update combinations, and the emitted shape is held on the PR gate by
`test_a_nullable_output_is_declinable_at_update_in_c`.

The empty triple of Appendix D item 11 is a probe in each backend's own suite.

### U3

Checked with an explicit finite test, so it rejects NaN and both infinities
alike. Verified: every `Update` and every `Peek` entry point checks its bar.

### U4

No cross-language gate can reach U4 (`stream_verify` runs 240 bars and
`INDEX_MAX` is 100 000 000), so it is pinned by one source gate and one runtime
probe per backend. `only_an_accepted_bar_advances_the_range` asserts the guard's
*position* (first, with only C's handle check allowed in front of it), the code
it names, and its ABSENCE from `Peek`, in all four backends over the whole
corpus. Then each of `test_stream_finite.c`, `stream_out_range.rs`,
`StreamSmokeTest` and `StreamApiTest` drives one handle to the ceiling and
demands the refusal, the code, and that nothing moved. The code is asserted
twice because the type cannot stand in for it: Java renders S1's code and U4's
as one exception class.

### G1 to G7

G3's C getter: `test_internals.c`. G7's Rust latch: verified by probe
(footnote [16]).

## Rationale

### R2: order

Listing each tier in evaluation order is what makes a multi-fault call
predictable for automated tests. Rules answering the same code are unordered in
practice: a caller cannot tell which of them fired, so swapping two is
invisible. What has to hold is that all four backends answer the same code for
the same call (R3), which `--xlang-hash` compares over the whole corpus.

### R4: checks precede writes

C's `OpenAndFill`, rejected under S7, writes 0 to `*outBegIdx` and
`*outNBElement` before it returns: the transcribed body's empty-range exit
does, and a composed opener's guard writes the caller's pair explicitly
(`c_stream.rs`, "must ALSO zero the caller's real pair"). The public R4 lists
it as an exception. No source says whether a `TA_INTERNAL_ERROR` leaves the
caller's buffers untouched, so R4 promises nothing after B8.

### R5: messages

A composed function calls its callee's public entry point, so a rejection the
callee detects carries the callee's name. Accepted rather than worked around: a
sub-function failure naming the sub-function is understandable, and restoring
the outer name would mean catching and rethrowing at every cross-call, which the
machinery calling the public tier exists to remove. It is narrow in practice:
every outer function validates its own parameters before it cross-calls, so
reaching it needs a fault the outer prologue does not screen for first.
`docs/streaming-api-design.md` fixes the `<N> <verb>: ` prefix as a
cross-language contract.

### Failure mapping ([hub](https://ta-lib.org/spec/#failures))

The platform exception types are many-to-one: one `IndexOutOfBoundsException`
serves both index codes in Java, one `IllegalStateException` or
`InvalidOperationException` the library-side ones. A caller who cannot tell two
conditions apart cannot respond to either, so every exception Java and C# raise
from the batch and stream tiers carries its `TA_RetCode`. They are subclasses of
the platform types, so an existing `catch` is unaffected. An absent argument and
a buffer too short, which have no code of their own (and the second of which C
cannot detect), answer `TA_BAD_PARAM`, which makes the mapping total over those
two tiers.

The settings builders and the abstraction layer raise plain platform types for
their *own* refusals (an unbound slot, a wrong kind, a slot index out of range):
neither is a function call, so neither has a `TA_RetCode` a caller would be
recovering. The layer's *dispatch* calls the public entry point, so an
indicator's rejection reaches the caller carrying its code in every backend.

C#'s U4 is a `TALibArgumentException`, not the
`TALibArgumentOutOfRangeException` its index codes take in batch: `Update` and
`Advance` have no `endIdx` parameter to name. An opener's S1 `ParamName` is the
history series, the argument a caller can change, since an opener has no
`startIdx`.

A backend enum carrying a member it never produces is harmless; one missing a
member it needs is not. Re-check whenever the abstraction layer is specified.

Known gap: Java's and C#'s MA stream throw a plain `IllegalStateException` /
`InvalidOperationException` ("unreachable: open rejects arms without a
sub-stream", `Core_MA.java`, `Core_MA.cs`) with no code and no prefix.
Unreachable by construction.

### B5, S5: C

Measured with guard-paged buffers: an input that does not reach `endIdx`, or an
output too short for the count the call produces, is read or written anyway, and
the call faults inside the algorithm with the output already partly written.
Behind Rust's check the same bound is asserted for LLVM: a panic, never memory
corruption, since the crate forbids `unsafe`.

### B6, N4, N8: overlap

Decided in #225: the library detects buffer identity and nothing finer. The four
backends do not even agree on whether a partial overlap can exist, so a stronger
rule could not be one rule:

| backend | can a caller express a partial overlap? | detected? |
|---|---|---|
| C | Yes: two pointers into one allocation | **No.** Detecting it means ordering pointers into different declared objects, which C leaves undefined; a conforming check would have to launder them through `uintptr_t` and reason about representation |
| Java | **No**: two arrays are the same object or disjoint; there is no offset to differ | n/a, so the run-time reference-equality check is complete |
| Rust | **No** in safe code: `&[T]` and `&mut [T]` over the same data cannot coexist, and two `&mut` cannot either | the `as_ptr()` identity guard never fires in safe code; it is kept for parity, and for the FFI boundary |
| C# | **Yes**: two `Span<T>` slices of one array | **Yes**, incidentally: `Span.Overlaps` exists, so the check is one call |

Measured: `TA_BBANDS` with its three bands one element apart returns
`TA_SUCCESS` and a wrong upper band on every bar.

So the guarantee is set by the weakest member that can express the problem,
which is C, and C is the one language where the check is not merely expensive
but not straightforwardly expressible. Java and Rust satisfy the stronger rule
for free by making the state unreachable, which is not the same as enforcing it.

Outputs must be different buffers in every language, a zero-length one included
(ruled 2026-10-01, Appendix D item 15): R3 then holds with no exception, and C
and Java already compared identity whatever the length. Two distinct empty
outputs are different buffers and never collide (item 11, #262). A call that
is both undersized and identical answers B5, the earlier rule (#261).

Outputs of different element types can only be the same buffer through a
reinterpreting cast. C compares both through `const void *` (well defined, not
the `double * == int *` constraint violation that reading suggests), and C#
compares byte ranges through `MemoryMarshal.AsBytes`, because
`MemoryMarshal.Cast` lets a caller lay a `Span<int>` over a `Span<double>`
without `unsafe`. Java and safe Rust cannot build the case. Java's batch tier
omits the term; its streaming tier spells it `(Object)a == (Object)b`, which
compiles and is always false. Skipping the pair in C would leave a hole: every
same-typed pair answers `TA_BAD_PARAM`, and the one pair a caller has to cast to
build would answer `TA_SUCCESS` and write through both (SUPERTREND mixes the two
types; #272, #386).

C# detects more than the rule: its guard
`if (outReal.Overlaps(inReal) && outReal != inReal)` rejects a partial
input/output overlap while allowing whole-buffer in place, `Overlaps` rejects a
partial output/output overlap, and its `float` overload rejects any overlap
between a `float` input and a `double` output (#386). Kept because it costs one
call on a type that already answers the question. If uniformity is ever
preferred over the extra safety, removing it is the change, not adding the check
elsewhere.

N4 is supported, not tolerated: several bodies are written for it and elect
their scratch by testing for exactly that case. That election is why STOCH,
STOCHF and KDJ leave intermediate values past `count` when their first output is
in place.

### B7: allocation failure

`TA_ALLOC_ERR` is returned, and that is the entire promise. Nothing is tested
past it. The library will not abort on the caller's behalf; that choice is the
caller's. What it will not do is pretend a path it never tests is safe to
continue from. Every `TA_ALLOC_ERR` the library returns is a null check on
`TA_Malloc`; a condition the caller could have avoided is `TA_BAD_PARAM`. In C
the handle out-parameter is still set to NULL on that path, which the public
page does not promise.

### B8: internal errors

Some batch functions per backend carry a guard, and none is reachable today:
they test a period below 1, which B3's declared floor already refuses. A stream
opener carries one more for each ring, window, circular buffer and
rolling-extremum span it captures, each checking that span against the history it
was handed, and a dispatching or dual-mode opener one for the arm its own `Open`
has already refused. A composed C `Update` can return one after a sub-stream has
advanced (`TA_APO_StepImpl` steps its first sub-stream before the second can
fail), which is why the public H4 excludes B8.

`ta_codegen/input/internal_error_ids.yaml` maps a function-tier id back to the
function and the state field it guards. The abstraction layer hand-allocates
ids 1 to 5, most of them shared by several sites (id 2 by 13 in `ta_abstract.c`),
so an id there names a kind of check, not one guard; V4 is scoped to the function
tier for that reason.

### I4: input domain

It licenses an implementation to be correct only inside the domain, so that a
formulation which is exact for every value a caller may legally pass is not given
up for what it would do near `DBL_MAX`. RSI is the worked example: its gain/loss
split derives the loss delta by subtraction, which is exact for every finite
difference and degenerates to `Inf - Inf` only when two adjacent bars of
opposite sign are each near 9e307, some 270 orders of magnitude outside the
domain. Sizing a change against a value outside the domain is
over-engineering, and this is the citation for saying so.

The bound is not enforced: checking it would cost a pass over every input array
on every call, to reject values no caller sends. It is not the range the gates
exercise either: the fuzz corpus tops out around 1e9 (`FUZZ_EXTREME`), so
between 1e9 and 3e37 the domain rests on the arithmetic rather than on
measurement. The constant is reused rather than invented: it already bounds a
real optional parameter, and it sits inside `FLT_MAX`, so a value in the domain
is representable in the `TA_S_*` variants. `DBL_MIN` would mislead, being the
smallest normalised *positive* double.

### I5: S8 withdrawn

The warm-up history is an input *array*, and the library does not scan those.
Until S8 was removed it was the only checked array in the library (every `Open`
and every `OpenAndFill`, in all four backends), which made "arrays are never
scanned" a rule with one exception. What the scan cost, and why folding it into
the fill loop was not the alternative, are in `docs/streaming-api-design.md`. U3
is untouched: a bar handed to `Update` or `Peek` is a single value.

### O5: declinable outputs

C# declines with an empty span. Its writes and its length check test
emptiness, so a null array or `default` (a span whose reference is null) and an
empty span over a real array decline alike there. That costs nothing: a declined
output is written to no more than one that has nothing to hold, and the length
check is still applied to a supplied output. Only the pair guard tells the two
apart (next paragraph). Rust could have read a zero-length slice the same way,
and does not: `Option` is the shape a Rust caller expects, it makes the
declination visible at the call site, and it keeps `&mut []` for a non-nullable
output a sizing mistake.

An omitted output is not an alias, so B6's pair guard skips a pair whose
operands are not both present. C and Java guard each nullable operand non-null
before comparing (two `NULL`s compare equal), and C# skips a span whose
reference is null, which is what a null array becomes; an empty span over a real
array is still compared. Rust's guard requires both slices non-empty; safe code
cannot pass one slice twice, so that is unobservable.

A declined output is still computed, which is what MAMA needs: FAMA feeds the
next bar. `MA`'s MAMA arm declines `outFAMA` outright in all four backends.

Keep `nullable` off the two hand-rolled stream tiers (`Dispatch`, `PeriodBank`):
they copy a bar through an unguarded assignment and so require every output,
declared `nullable` or not. Nothing shipped combines the two.

More than one nullable output per function is generated but unshipped; the pair
guard branches on which of a pair is nullable, so three of its four arms are
reached only by `SYNTH10` (`ta_codegen/generator/input_synth/`), driven through
all four backends by `scripts/synth_gate.py`.

### O7: intermediate overflow

Overflow is a property of `double` rather than of any indicator, so it carries
no flag, and issue #191 settled that it gets no semantics.

### S1 to S6

They follow the batch tier's order, but are not B1 to B6 read on
`[0, historyLen - 1]`: an empty history answers `TA_OUT_OF_RANGE_START_INDEX`
where `batch(0, -1)` answers `TA_OUT_OF_RANGE_END_INDEX`, and S5 requires each
input to equal the history's length where B5 accepts a longer one.

### S4

The presence checks sit on the public frame, because `<N>_OpenImpl` reads the
history's length before anything else could look at it.

### S5

The input half is checked first, on the public frame: the core makes the test
too, but only after the capacity bound would have answered, so a short input
series was reported as an output-capacity fault.

The output half is B5's produced count read over the same range, which collapses
it to `historyLen - lookback`, never the width of the history. It has no
legitimate zero case the way B5 does: S7 refuses a history shorter than
`lookback + 1`, so a fill that runs writes at least one value. A short history is
floored to zero so that it reaches S7; a lookback of `-1` is not floored but
raised, which is what keeps S3 ahead of the buffer rules, as the batch tier's
`clampedStart` does.

It sits on the **public** frame, never on `<N>_OpenAndFillInternal`: that seam
takes an anchor and writes `historyLen - max(lookback, startIdx)`, fewer, so the
same bound there would reject the composed sub-calls that pass a non-zero anchor.
It would also be redundant: a composed destination is proved disjoint by
`SubCallStep::is_fusable` and sized by construction. The frame reads the count
from the function's own `<N>_Lookback`, whose default substitution and range
validation come with it. A `nullable` output is bounded only when supplied.

### S6: no in-place OpenAndFill

Not because in-place would compute the wrong answer: measured, it does not. The
fill's writes stop where the handle's warm-up seeds begin, or overlap them by the
single slot the next `Update` rewrites first. The ban is there because that
margin is an accident of every body's arithmetic that nothing states or asserts,
and supporting in-place would promise it for every function in four backends,
permanently.

### S7: short history

The warm-up check comes last because it is the one thing the batch tier has no
analogue for. All four converge on `TA_INSUFFICIENT_HISTORY`: across every
streaming function in every backend, each short-history arm reports this code
and no other. What they answered before the code existed is Appendix D item 8.

S7 is also what a **composed** opener answers when a sub-call succeeds with zero
elements: the batch tier may report that as an empty range, but an opener has no
handle to mint over a range holding nothing. C is outside the sweep that pins it:
no fold pass runs for C, so its transcribed guard still tests both halves and
returns the sub-call's own code. No opener answers `TA_SUCCESS` over zero
elements today, but a composed indicator that made one would hand C's caller
`TA_SUCCESS` and a NULL handle where the other three answer S7.

### U4, H4, H5: advancing

U4 is S2 read one bar at a time: the next bar's index is `begIdx + count`, and
once that leaves `[0, INDEX_MAX]` the handle is being asked for a bar the batch
tier refuses to address, permanently. It is evaluated where the opener
evaluates S1 and S2, ahead of every presence check with only the handle's own
null test in front of it, because it is the same fault read on the same domain,
and a caller who fixed the argument the presence check named would only get U4
back. `Peek` is exempt for performance; it commits no bar.

A composed step drives its sub-handles through their public `Update`, so each of
them re-reads U4. It cannot fire there first, and that is load-bearing: every
handle over one history satisfies `begIdx + count == historyLen` at open, an
accepted `Update` moves a parent and its sub-handles together, and `Advance`
moves the parent alone, so the parent is never behind and its own guard answers
before any sub-handle is stepped.

Re-feed or `Advance` is the caller's choice and neither is automatic: the error
is in hand at the moment of decision and `OutRange` is readable, so the drift
doing neither causes is detectable. No backend publishes the held value through
the call's own result: Rust's `Result<f64, RetCode>` cannot carry one beside
`Err`, and a rule three backends could honour and one could not would be worse
than the accessor. `Value` exists because a cloned stream is the one caller with
no earlier call to have handed it a value.

### G2, G5: bounds

Both values are added to a lookback which is then used as an index: unbounded,
the lookback overflows negative and the function indexes far past the end of its
input while still reporting success. `INDEX_MAX` is the ceiling the index domain
already enforces, and a warm-up longer than the largest addressable series could
never produce output, so nothing legitimate is refused.

### G6: NaN only

A factor scales a threshold and never indexes anything, so an infinity cannot
take a function off the end of its input. NaN is refused because it silences
every comparison it feeds, which is indistinguishable from "this shape never
occurs". This is the one check on a single value that treats NaN and ±Inf
differently.

---

## Appendix B: How the marks were produced

Each ✅ rests on two independent checks; neither alone is enough.

1. **A runtime probe against the built artifact**: one program per backend,
   driving the scenarios through the *public* API and printing what came back.
   Representative functions: `SMA` (one input, one integer parameter, one
   output), `BBANDS` (three outputs, a real and an enum parameter), `MAMA` (real
   parameters with a narrow domain), `MININDEX` (integer output), `SQRT` (a
   documented output-domain hole). C's memory-unsafe rules are probed with
   guard-paged buffers, so a read or write past a declared length faults instead
   of silently corrupting neighbouring memory, and each such scenario runs in a
   forked child so a crash is an observation rather than the end of the run.

2. **A structural check over the whole generated corpus**: a probe on one
   function says nothing about the rest. Verified mechanically, from the
   generated sources:

   | Claim | Result |
   |---|---|
   | C batch: `startIdx` guard, then `endIdx` guard, before any other return | every function |
   | C batch: parameter validation, then every input, the `OutRange` pointers and every output null-checked, inputs before outputs | every function, both overloads |
   | Rust batch: `startIdx` guard, `endIdx` guard, lookback (which returns B3), every input, then every output length-checked | every function |
   | Rust numerics: `startIdx` guard, `endIdx` guard, bounds asserts (following the FMA dispatcher to the real core) | every function |
   | Java batch: clamp (which raises B3), then every length check, then the core | every function, both overloads |
   | C# batch: clamp, then every length check, then the core | every function, both overloads |
   | C# cores carrying an overlap guard wherever one is expressible | no core unguarded where the type expresses it |
   | Short-history arm reports `TA_INSUFFICIENT_HISTORY` | every streaming function, per backend, no backend mixing it with anything else |
   | Empty-history arm reports `TA_OUT_OF_RANGE_START_INDEX` | every opener arm in all four backends, no backend mixing it with anything else |
   | Java public opener: the history's null test, then the index pair, then every other argument | every `Open` and `OpenAndFill` |
   | C# public opener: an empty history is the index fault, ahead of every other input | every `Open` and `OpenAndFill` |
   | Rust/Java/C# public `OpenAndFill`: every output bounded by `historyLen - <N>_Lookback(...)` | every one, per backend |

   The "both overloads" rows are every definition in `ta_codegen/input/` taken
   twice, once per overload, so the float surface rests on the same evidence.

Most of these probes are not committed. They are throwaway drivers: the shipped
gates cover values, and these cover the failure paths once, to produce this
table. Re-running them means rewriting them: cheap, and honest about the fact
that a mark is a statement about a moment. The exceptions are the tests named
under Test coverage: those cover a condition no value gate reaches, so they are
committed and run.

---

## Appendix C: Not yet specified

- **The abstraction layer's own surface**: handle lookup, unbound and mistyped
  arguments. Two rules about the layer are settled and published,
  [M1](https://ta-lib.org/spec/errors/#m1) and
  [M2](https://ta-lib.org/spec/errors/#m2). The rest answers differently today
  (C returns 10 or 11 for an unbound argument, C#'s `TryCall` the same codes,
  C#'s `Call` and Java plain exceptions, Rust `BadParam`); the public page lists
  that as current behaviour.
- **JSON-RPC servers**: a test harness, not a shipped API. Their error behaviour
  is a property of the harness.

---

## Appendix D: Open items

Every ❌ above, collected, plus message-level deviations and divergences no rule
row can carry: a call two backends answer differently although a rule covers it
(items 11 and 15). Numbering is append-only: a retired item leaves a gap rather
than renumbering the rest. Each was measured, not inferred.

A `⚠️` is not tracked here. It marks a deviation that is deliberate, or a rule
implemented but not covered by a CI probe; the rule's own footnote says which.

| # | Backend | Rule | Defect |
|---|---|---|---|
| ~~1~~ | C | B4 | *Fixed.* The `OutRange` pointers were not null-checked, so passing either as null segfaulted. The batch prologue now checks them, as the streaming `OpenAndFill` prologue always has. |
| ~~2~~ | C | B4 | *Fixed.* Input-buffer presence was checked *before* parameter validation and output presence after, so the prologue straddled B3. Parameter validation now precedes every presence check. |
| ~~3~~ | Java | B4 | *Fixed.* Buffer presence was checked *before* the index and parameter rules, inverting the specified precedence: a negative `startIdx` with a null input reported the null. The wrapper now evaluates B1, B2 and B3 first. |
| ~~4~~ | Java | B3 | *Fixed.* A null enum parameter yielded a raw JVM `NullPointerException` naming neither function nor parameter; it is now a parameter outside its domain, named, and carrying `TA_BAD_PARAM`. |
| ~~5~~ | | | *Withdrawn, not fixed.* Partial output/input overlap in C. Decided in #225: detection stops at buffer identity, and partial overlap is unspecified (N8). |
| ~~6~~ | Java | S4 | *Fixed.* A null history, or a null `OpenAndFill` output, yielded a raw JVM exception from inside the algorithm. The public openers now check every argument, and item 13 fixed the order they are checked in. |
| ~~7~~ | C# | S1 | *Fixed.* The empty-history *message* omitted the cross-language `<N> open: ` prefix. Taken with item 13. |
| ~~8~~ | all | S7 | *Fixed.* `TA_RetCode` had **no member** for "history shorter than the lookback", so C and Rust fell back to the catch-all and Java and C# borrowed `TA_OUT_OF_RANGE_END_INDEX`. `TA_INSUFFICIENT_HISTORY = 17` was appended and all four now report it, which leaves S2 as the opening tier's only producer of `TA_OUT_OF_RANGE_END_INDEX`. |
| ~~9~~ | Rust, Java, C# | S5 | *Fixed.* `OpenAndFill` validated no output capacity, so an undersized output faulted inside the fill with the buffer already partly written. The public frame now bounds every output by `historyLen - <N>_Lookback(...)`. (C still cannot: no sizes.) |
| ~~10~~ | C | | *Obsolete.* `TA_SetCompatibility` accepted any value; #388 removed the behaviour it selected, and the pair is kept declared and inert. |
| ~~11~~ | C#, Rust | B6 | *Fixed.* Two distinct **empty** output buffers were rejected as aliased in C# and Rust and accepted in C and Java, measured on `ACCBANDS(0, 251, …, optInTimePeriod 253, …)` with three distinct zero-length outputs. Both now accept them (#262). |
| ~~13~~ | all | S1 | *Fixed.* An empty history answered `TA_BAD_PARAM` where S1 specifies `TA_OUT_OF_RANGE_START_INDEX`, and C checked argument presence ahead of the index pair. All four openers now answer the pair ahead of every presence check, except for the one check each language makes a precondition of reading the length (footnote [4]). |
| 14 | Java | L1, L2, L4 | *Open.* A lookback call does not check a null MA type: `maLookback(1, null)` returns 0 and `maLookback(2, null)` throws `NullPointerException` from its `switch`, where `ma(..., null)` throws `TALibArgumentException` carrying `TA_BAD_PARAM` (B3). The same holds for every lookback with an MA-type parameter (MA, MAVP, STOCH, STOCHF, STOCHRSI, KDJ, MACDEXT, BBANDS and the rest): a null-typed stage at period 1 returns a lookback, any other throws. |
| ~~15~~ | C# | B6, S6 | *Fixed.* One zero-length array passed as two outputs, on a range that produces no values, answered `TA_SUCCESS` in C# (`Overlaps` is false for an empty span) and `TA_BAD_PARAM` in C and Java. Ruled 2026-10-01: outputs must be different buffers in every language. C#'s batch and `OpenAndFill` guards now also reject two empty outputs on the same non-null reference. |

Next item: 16.
