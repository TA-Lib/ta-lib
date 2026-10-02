# API Error-Handling Conformance

The caller-facing rules are published at https://ta-lib.org/spec/ (source
`website/src/spec/`) under the `r` ids used below. The public pages hold only
what a caller relies on and a test enforces. This file is the maintainer's side:
per id, whether each backend conforms, what tests it, and why the rule is what
it is. It also states what the public pages leave out because only the tests and
a maintainer need it: the evaluation order. A
change to a published rule edits the public page; a change here edits a mark, a
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

Rows of the batch, opening and advancing tables are in evaluation order: a call
that breaks several conditions reports the first (rationale: Order). Footnote
numbers are append-only: a
retired footnote leaves a gap, so a citation of `[4]` keeps its meaning.

### Lookback tier

| Rule | Topic | C | Rust | Java | C# |
|---|---|:---:|:---:|:---:|:---:|
| rL2 | [rejection signal](https://ta-lib.org/spec/lookback/#rl2) | ✅ | ✅ | ✅ | ✅ |
| rL3 | [agrees with batch and Open](https://ta-lib.org/spec/lookback/#rl3) | ✅ | ✅ | ✅ | ✅ |
| rL4 | [agrees with C](https://ta-lib.org/spec/lookback/#rl4) | — | ✅ | ✅ | ✅ |
| rL2 | [its only failure](https://ta-lib.org/spec/lookback/#rl2) | ✅ | ✅ | ✅ | ✅ |
| rL11 | [display shift rejects what the lookback rejects](https://ta-lib.org/spec/lookback/#rl11) | ✅ | ✅ | ✅ | ✅ |

### Batch tier

| Rule | Topic | C | Rust | Java | C# |
|---|---|:---:|:---:|:---:|:---:|
| rB1 | [start index](https://ta-lib.org/spec/errors/#rb1) | ✅ | ✅ | ✅ | ✅ |
| rB2 | [end index](https://ta-lib.org/spec/errors/#rb2) | ✅ | ✅ | ✅ | ✅ |
| rB3 | [parameter value](https://ta-lib.org/spec/errors/#rb3) | ✅ | ✅ | ✅ | ✅ |
| rB4 | [absent argument](https://ta-lib.org/spec/errors/#rb4) | ✅ | —<br>[1] | ✅ | —<br>[2] |
| rB5 | [buffer length](https://ta-lib.org/spec/errors/#rb5) | ⚠️<br>[3] | ✅ | ✅ | ✅ |
| rB6 | [one buffer, two outputs](https://ta-lib.org/spec/errors/#rb6) | ✅ | ✅ | ✅ | ✅ |
| rB7 | [omitted output](https://ta-lib.org/spec/errors/#rb7) | ✅ | —<br>[20] | ✅ | —<br>[20] |
| rB8 | [allocation failure](https://ta-lib.org/spec/errors/#rb8) | ⚠️<br>[23] | ⚠️<br>[23] | ⚠️<br>[23] | ⚠️<br>[23] |
| rB9 | [internal error](https://ta-lib.org/spec/errors/#rb9) | ⚠️<br>[21] | ⚠️<br>[21] | ⚠️<br>[21] | ⚠️<br>[21] |

[1] Rust slices cannot be null, and what C writes through a pointer (the
index/count pair, a stream handle) is returned instead.

[2] C# `Span<T>` cannot be absent. A null array converts to an empty span, so
absence surfaces as a zero length: rB5 in the batch tier, rS1 for a stream's
history. A zero-length output is rS5's case.

[3] C has no sizes to check against: a property of the ABI, not a defect.

[20] Rust takes a non-declinable output as `&mut [T]`, which cannot be omitted.
C# has no check of its own (footnote [2]).

[21] Implemented, but the individual sites are not tested: no reachable input
fires one (rationale rB9).

[23] Deliberately untested: only C returns it, and nothing past it is defined
(rationale rB8).

### Stream opening

| Rule | Topic | C | Rust | Java | C# |
|---|---|:---:|:---:|:---:|:---:|
| rS1 | [empty history](https://ta-lib.org/spec/streaming/#rs1) | ✅<br>[4] | ✅<br>[4] | ✅<br>[4] | ✅<br>[4] |
| rS2 | [history too long](https://ta-lib.org/spec/streaming/#rs2) | ✅<br>[5] | ✅<br>[5] | ⚠️<br>[5] | ✅<br>[5] |
| rS3 | [parameter value](https://ta-lib.org/spec/streaming/#rs3) | ✅ | ✅ | ✅ | ✅ |
| rS4 | [absent argument](https://ta-lib.org/spec/streaming/#rs4) | ✅ | —<br>[1] | ✅ | —<br>[2] |
| rS5 | [buffer length](https://ta-lib.org/spec/streaming/#rs5) | ⚠️<br>[3] | ✅ | ✅ | ✅ |
| rS6 | [aliasing](https://ta-lib.org/spec/streaming/#rs6) | ✅ | —<br>[22] | ✅ | ✅ |
| rS7 | [declined output](https://ta-lib.org/spec/streaming/#rs7) | ✅ | —<br>[20] | ✅ | —<br>[20] |
| rS8 | [short history](https://ta-lib.org/spec/streaming/#rs8) | ✅ | ✅ | ✅ | ✅ |

The openers do not scan the history for non-finite values (rationale: finite
inputs).

[4] Each backend's check ahead of rS1 differs because each is a precondition for
evaluating the pair rather than an argument competing with it. C publishes "no
handle on any failure" through `*stream`, and has nowhere to publish it without
one. Java cannot read a length from an array that is not there. A Rust slice
cannot be absent, and a C# null array arrives as an empty span, so rS1 is how an
absent history is reported (footnote [2]).

[5] The bound is in each opener's own prologue (Java and C# answer it from the
same frame as rS1). C takes `historyLen` as a bare `int`, so its probe needs no
array. Rust probes it on a zeroed vector of `INDEX_MAX + 2` elements that is
never read (`tests/stream_open_contract.rs`, 64-bit targets), and C# on a span
that claims that length over one element (`StreamApiTest`); both rest on the
length being refused before a bar is read. Java probes `Core.requireHistory`,
the helper every public opener calls (`NoPhantomIoTest`), because an array of
that length needs an 800 MB heap, so no Java opener is driven to it:
`java_public_openers_check_arguments_then_the_index_pair` holds the call. The
legal upper edge through an opener, a history of exactly `INDEX_MAX + 1` bars,
is unprobed in all four.

[22] Cannot be provoked. `OpenAndFill` takes each output as its own `&mut` slice,
so two outputs, or an output and the input, cannot name the same buffer while the
call is live. The batch tier emits rB6's pointer comparison anyway; the streaming
tier emits nothing.

### Stream advancing

| Rule | Topic | C | Rust | Java | C# |
|---|---|:---:|:---:|:---:|:---:|
| rU1 | [absent handle](https://ta-lib.org/spec/streaming/#ru1) | ✅ | — | — | — |
| rU4 | [index ceiling](https://ta-lib.org/spec/streaming/#ru4) | ✅ | ✅ | ✅ | ✅ |
| rU2 | [absent output](https://ta-lib.org/spec/streaming/#ru2) | ✅ | — | ✅ | — |
| rU5 | [declined output](https://ta-lib.org/spec/streaming/#ru5) | ✅ | n/a<br>[10] | n/a<br>[10] | n/a<br>[10] |
| rU3 | [non-finite bar](https://ta-lib.org/spec/streaming/#ru3) | ✅ | ✅ | ✅ | ✅ |

[10] The other three have no component to decline at this tier: Rust and C#
return the value, and Java writes every field of a caller-owned sink.

### Stream release

| Rule | Topic | C | Rust | Java | C# |
|---|---|:---:|:---:|:---:|:---:|
| rH10 | [`Close(NULL)`](https://ta-lib.org/spec/streaming/#rh10) | ✅ | — | — | — |

Only C has an explicit release. The other three reclaim a handle when it becomes
unreachable, so there is no double release or use after release to track.
`sf_null_arguments` (`test_stream_finite.c`) calls `Close(NULL)` on each of its
emitted bodies, and drives a NULL handle and a NULL out-pointer through
`Update`, `Peek`, `Value`, `OutRange` and `Clone` in C.

### Configuration

| Rule | Topic | C | Rust | Java | C# |
|---|---|:---:|:---:|:---:|:---:|
| rT2 | [target](https://ta-lib.org/spec/settings-threads/#rt2) | ✅ | ✅<br>[13] | ✅<br>[13] | ✅ |
| rT3 | [unstable period bound](https://ta-lib.org/spec/settings-threads/#rt3) | ✅ | ✅ | ✅ | ✅ |
| rT4 | [reading](https://ta-lib.org/spec/settings-threads/#rt4) | ⚠️<br>[14] | ✅ | ✅ | ✅ |
| rT5 | [range type](https://ta-lib.org/spec/settings-threads/#rt5) | ✅ | — | — | ✅ |
| rT6 | [average period bound](https://ta-lib.org/spec/settings-threads/#rt6) | ✅ | ✅ | ✅ | ✅ |
| rT7 | [NaN factor](https://ta-lib.org/spec/settings-threads/#rt7) | ✅ | ✅ | ✅ | ✅ |
| rT8 | [no change on refusal](https://ta-lib.org/spec/settings-threads/#rt8) | ✅ | ✅<br>[16] | ✅ | ✅ |
| rT10 | [initial state](https://ta-lib.org/spec/settings-threads/#rt10) | ✅ | — | — | — |

[13] The wildcard is a declared member, so it is rejected by value; a target
outside the declared set is unrepresentable.

[14] C's getter returns the period itself, so it has no error channel. Accepted,
and pinned by `test_internals.c`; the other three reject it because a getter
that can throw costs them nothing.

[16] Verified that a later valid setter does not clear an earlier latched
rejection.

## Test coverage

### Lookback rules

`xlang_lookback_leg`, `xlang_tier_native_check`, `xlang_tier_gold_check`
(regtest `--xlang-hash`); `aNullEnumLookbackIsMinusOne` in `BatchApiTest`
(Java) for a null MA type.

What enters a lookback, in C, in a bare run:

- rL1's first bar: `checkNoBarBeforeTheFirst` (`test_abstract.c`) overwrites
  every bar below `max(startIdx, lookback) - lookback` in every input of every
  function, with NaN and with a large finite value, at unstable periods 0 and
  5, and requires the same answer bit for bit. Its control overwrites one bar
  more, which most functions read.
- rL6: `test_func_unstable_shift` (`test_codegen.c`) raises each owner's id by
  5, at the default parameters and with every integer parameter at its
  minimum, and requires five more bars of lookback and the same remaining
  values (the four DI/DM functions at period 1: no more bars). For every
  function it sets every id above the lookback, at the default parameters,
  with every integer parameter at its minimum and at each value of its list
  parameters: a lookback that does not move must come with outputs that do
  not move. `test_adx.c` pins the DI/DM rows,
  `test_period_boundary.c` the DEMA, TEMA and period-1 MA stage counts, and
  `test_kc.c` KC's longer path over a grid of two unstable periods.
- rL5, rL7 and rL9: `abstract_lookback_under_settings` (`test_abstract.c`), for
  every function at its default parameters: another range type and factor on
  every candle setting moves no lookback; seven more bars on every averaging
  period move a candlestick lookback by seven or not at all, and no other;
  all three candle components plus an unstable period move no display shift;
  with every
  unstable period, then every averaging period, at `TA_INDEX_MAX` the lookback
  only grows, at the integer parameters' maxima too.
- rL2's signal: `abstract_check_display_shift` refuses a negative lookback
  other than -1 on every vector it drives.

The ports' lookback and display-shift calls are compared with C's at default
settings only; under a setting a port's lookback is seen through
the batch `outBegIdx`, and its display shift not at all.

### rL11

`abstract_check_display_shift` (`test_abstract.c`; against each server under
regtest `--codegen`, in-process in a bare run); `aNullEnumLookbackIsMinusOne`
in `BatchApiTest` (Java) for a null MA type.

### Batch conditions

`testIndexRange`, `checkOutputAliasRejected`, `testBatchArgumentContract`,
`c_batch_prologue_orders_parameters_before_presence`,
`rust_public_entry_orders_the_argument_contract`,
`rust_batch_impl_orders_capacity_before_aliasing`,
`every_declared_input_is_checked_in_every_backend`, `BatchApiTest` (Java, C#);
no phantom io and `--xlang-hash` for behaviour matching.

rB6: `checkOutputAliasRejected` (`test_abstract.c`) sweeps every ordered output
pair of every function, cross-typed pairs included, binding both onto one buffer
and requiring `TA_BAD_PARAM`, once over the full range and once over a range
that produces no values: C's form of one zero-length array passed as two outputs
(Appendix D item 15), with the pair rebound apart as that leg's control.
Java's and C#'s `BatchApiTest` pass one zero-length array as two outputs.

rW7: `checkInPlaceAliasCorrect` (`test_abstract.c`),
`anOutputOnItsInputAnswersTheSame` (Java's `MetadataTest`) and
`AnOutputOnItsInputAnswersTheSame` (C#'s `MetadataTest`) place each real output
on each real input component of every function and require the answer of the
separate-buffer call bit for bit.

rP4: the float leg of regtest `--codegen` compares each server's float entry
point with its own double one on the same widened inputs, bit for bit: C over
the parameter sweep, Java and C# for every function at its default parameters
and at the default sentinels. It runs with all three on x86-64 only. C is also
held by `test_variants.c` in a bare run.

rP5: `validate_inputs`, `try_inject_parameters` and `validate_outputs`
(`docs_site.rs`) fail `generate` when a function page's Inputs, Parameters or
Outputs list differs from the call signature in name or order.

rW4's candlestick values: `test_candle_value_set` (`test_candlestick.c`) holds
every value a candlestick function writes on its series to the published set,
and requires each non-zero member to occur and most functions to fire. A
pattern too rare to fire there is held to nothing.

rE3 at a batch call: in C, `abstract_rejected_call_writes_nothing`
(`test_abstract.c`) paints every output buffer, calls each parameter vector of
its sweep that the lookback rejects and requires `TA_BAD_PARAM` with no element
written. Its two floors count the vectors rejected inside their declared range,
the rejections a C body makes itself after the generated checks: at a bound
(MAVP's inverted window) and one step above the lower bound (FRAMA's odd
period). It never reads the range pair. Java's and C#'s `BatchApiTest` hold a
canary after rB5. Rust has no run-time canary: the public entry makes every
argument rejection before its one `_impl` call
(`rust_public_entry_orders_the_argument_contract`), the parameter decision
included, which it takes from the lookback, and no output is lent to anything
ahead of that call. A code `_impl` returns is forwarded after the call; its own
parameter rejections stay unreachable for as long as the lookback decides the
same way (lookback rules above).

rB9's band: `testEnumValueContract` (`test_internals.c`) requires
`TA_SetRetCodeInfo` to name each value from 5000 to 5999 `TA_INTERNAL_ERROR`,
and `TA_UNKNOWN_ERR` for every value from 0 to 0xFFFF that is not a pinned
member and for four values outside that walk.

### Stream opening conditions

rS1, rS2, rS4, rS5, rS7 and rS8 are mapped below; rS3 is driven by the parameter
leg of `test_open_contract.c` over the C openers; rS6 is mapped only for one buffer
passed as two outputs: `testBatchArgumentContract` passes one buffer as
ACCBANDS's first two `OpenAndFill` outputs on a history of lookback bars
(`TA_BAD_PARAM`), with a separate-outputs control answering rS8; Java's and C#'s
`BatchApiTest` do the same with one zero-length array.

`testStreamShortHistory` drives rS1, rS2's rejecting side and rS8 in C: 9, 3 and 8
rejections, with rS8's 16 controls. Three of rS1's cases are *also* an absent
argument, which is what makes them about the order and not only the code.
`testBatchArgumentContract` drives rS4 with rB4's own argument shapes plus the
handle: 12 rejections and 5 controls, counted apart from rB4's. `BatchApiTest`
does the same for Java, and adds rS1's; `StreamApiTest` covers rS4 in C#, where
the condition is a zero-length span and so is rS1. Rust cannot express rS4, and
`tests/stream_open_contract.rs` covers rS1 and rS2 there. Java's `BatchApiTest`
asserts `BAD_PARAM`, not only the exception type, at each rS4 rejection, at an
`OpenAndFill` output placed on its input (rS6) and at a missing non-declinable
output (rS7).

`scripts/check_stream_retcodes.py` carries rS1 and rS8 over the whole generated
corpus in all four backends; a probe names one function, and this is what covers
the rest. It reads the *core's* arm, which in Java and C# the public frame makes
unreachable, so those two frames are covered corpus-wide by
`java_public_openers_check_arguments_then_the_index_pair` and
`csharp_public_openers_reject_an_empty_history_as_an_index_fault` instead.
`an_opener_never_answers_the_code_its_sub_call_handed_back` pins rS8 for composed
openers in the three ported backends; C is outside that sweep (rationale rS8).

rS5 is probed from **both sides** in each of the three backends that can express
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

### rS7, rU5 and rB7

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
nightly. rU5 itself is C's alone: `testBatchArgumentContract` drives all four
open/update combinations, and the emitted shape is held on the PR gate by
`test_a_nullable_output_is_declinable_at_update_in_c`.

The empty triple of Appendix D item 11 is a probe in each backend's own suite.

### rH3, rH6 and rH9

`stream_verify`'s fork leg, in each language server and for every function,
opens its original with `OpenAndFill` on the shortest history, updates it,
forks it, calls `Advance` once on the fork, and feeds both to the end. Every
bar of both must match batch, so a handle that came from `OpenAndFill` is held
to rH1 under `Update` (rH3), and a counted bar reaches nothing a step reads
(rH6); the fork reports one bar more than its original. MININDEX, MAXINDEX and
MINMAXINDEX ride the same leg: their index after the counted bar is still the
batch index over the bars fed.

rH9: `test_open_contract.c` requires `TA_FUNC_FLG_STREAM` on every function
`TA_ForEachFunc` lists and as many entries in the stream table.

A bar whose output is not finite: `dz_stream` (`test_div_zero.c`) and the
streaming halves of the Rust, Java and C# DivZero tests drive DIV, which
carries the flag rW6 names, through `Peek` and `Update` and require success,
the value and the bar count.

### rU3

Checked with an explicit finite test, so it rejects NaN and both infinities
alike. Verified: every `Update` and every `Peek` entry point checks its bar.

### rU4

No cross-language gate can reach rU4 (`stream_verify` runs 240 bars and
`INDEX_MAX` is 100 000 000), so it is pinned by one source gate and one runtime
probe per backend. `only_an_accepted_bar_advances_the_range` asserts the guard's
*position* (first, with only C's handle check allowed in front of it), the code
it names, and its ABSENCE from `Peek`, in all four backends over the whole
corpus. Then each of `test_stream_finite.c`, `stream_out_range.rs`,
`StreamSmokeTest` and `StreamApiTest` drives one handle to the ceiling and
demands the refusal, the code, and that nothing moved. The code is asserted
twice because the type cannot stand in for it: Java renders rS1's code and rU4's
as one exception class.

### Settings rules

rT4's C getter: `test_internals.c`. rT8's Rust latch: verified by probe
(footnote [16]). rT8's candle setter in Java: `aRejectedCandleSettingWritesNothing`
(`CoreApiTest`), through CDLDOJI since Java's `Core` has no candle getter. rT2,
rT4 and rT5 under a cast out of the enum in C#: `CandleMisuseThrows` and
`MisuseThrows` (`CoreBuilderTest`). rT10: `testUnstablePeriodBounds`
(`test_internals.c`) changes the settings and initializes over them.

## Rationale

### Order

The public pages promise one code per rejected call and no order (rE1): no
correct caller depends on which code a doubly wrong call gets. Each tier still
evaluates its conditions in the order of its table above, in all four backends.
That is what makes a multi-fault call predictable for automated tests, and what
keeps rE2 true for it. Conditions answering the same code are unordered in
practice: a caller cannot tell which of them fired, so swapping two is
invisible. The cross-language comparison covers the index codes
(`test_index_range_xlang`), the parameter codes and the opener codes
(`--xlang-hash`); the other conditions are tested per language.

### rE3: checks precede writes

C's `OpenAndFill`, rejected under rS8, writes 0 to `*outBegIdx` and
`*outNBElement` before it returns: the transcribed body's empty-range exit
does, and a composed opener's guard writes the caller's pair explicitly
(`c_stream.rs`, "must ALSO zero the caller's real pair"), and C's MAVP and
FRAMA zero the pair before rejecting a parameter. The public rE3 therefore
promises only that no output buffer is written, and leaves C's range
out-parameters unspecified after a failure. No source says whether a `TA_INTERNAL_ERROR` leaves the
caller's buffers untouched, so rE3 promises nothing after rB9.

### Messages

The public pages say only to branch on the code. The `<N>: ` and
`<N> <verb>: ` prefixes are emitted and tested all the same.

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

C#'s rU4 is a `TALibArgumentException`, not the
`TALibArgumentOutOfRangeException` its index codes take in batch: `Update` and
`Advance` have no `endIdx` parameter to name. An opener's rS1 `ParamName` is the
history series, the argument a caller can change, since an opener has no
`startIdx`.

A backend enum carrying a member it never produces is harmless; one missing a
member it needs is not. Re-check whenever the abstraction layer is specified.

Known gap: Java's and C#'s MA stream throw a plain `IllegalStateException` /
`InvalidOperationException` ("unreachable: open rejects arms without a
sub-stream", `Core_MA.java`, `Core_MA.cs`) with no code and no prefix.
Unreachable by construction.

### rB5, rS5: C

Measured with guard-paged buffers: an input that does not reach `endIdx`, or an
output too short for the count the call produces, is read or written anyway, and
the call faults inside the algorithm with the output already partly written.
Behind Rust's check the same bound is asserted for LLVM: a panic, never memory
corruption, since the crate forbids `unsafe`.

### rB6, rW7: overlap

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
(ruled 2026-10-01, Appendix D item 15): rE2 then holds with no exception, and C
and Java already compared identity whatever the length. Two distinct empty
outputs are different buffers and never collide (item 11, #262). A call that
is both undersized and identical answers rB5, the earlier rule (#261).

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

rW7 is supported, not tolerated: several bodies are written for it and elect
their scratch by testing for exactly that case. That election is why STOCH,
STOCHF and KDJ leave intermediate values past `count` when their first output is
in place.

### rB8: allocation failure

`TA_ALLOC_ERR` is returned, and that is the entire promise. Nothing is tested
past it. The library will not abort on the caller's behalf; that choice is the
caller's. What it will not do is pretend a path it never tests is safe to
continue from. Every `TA_ALLOC_ERR` the library returns is a null check on
`TA_Malloc`; a condition the caller could have avoided is `TA_BAD_PARAM`. In C
the handle out-parameter is still set to NULL on that path, which the public
page does not promise.

### rB9: internal errors

Some batch functions per backend carry a guard, and none is reachable today:
they test a period below 1, which rB3's declared floor already refuses. A stream
opener carries one more for each ring, window, circular buffer and
rolling-extremum span it captures, each checking that span against the history it
was handed, and a dispatching or dual-mode opener one for the arm its own `Open`
has already refused. A composed C `Update` can return one after a sub-stream has
advanced (`TA_APO_StepImpl` steps its first sub-stream before the second can
fail), which is why the public rH5 excludes rB9.

`ta_codegen/input/internal_error_ids.yaml` maps a function-tier id back to the
function and the state field it guards. The abstraction layer hand-allocates
ids 1 to 5, most of them shared by several sites (id 2 by 13 in `ta_abstract.c`),
so an id there names a kind of check, not one guard.

### Input domain

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

### Finite inputs: no history scan

The warm-up history is an input *array*, and the library does not scan those.
Until the openers' history scan was removed it was the only checked array in the library (every `Open`
and every `OpenAndFill`, in all four backends), which made "arrays are never
scanned" a rule with one exception. What the scan cost, and why folding it into
the fill loop was not the alternative, are in `docs/streaming-api-design.md`. rU3
is untouched: a bar handed to `Update` or `Peek` is a single value.

### rW5: declinable outputs

C# declines with an empty span. Its writes and its length check test
emptiness, so a null array or `default` (a span whose reference is null) and an
empty span over a real array decline alike there. That costs nothing: a declined
output is written to no more than one that has nothing to hold, and the length
check is still applied to a supplied output. Only the pair guard tells the two
apart (next paragraph). Rust could have read a zero-length slice the same way,
and does not: `Option` is the shape a Rust caller expects, it makes the
declination visible at the call site, and it keeps `&mut []` for a non-nullable
output a sizing mistake.

An omitted output is not an alias, so rB6's pair guard skips a pair whose
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

### Intermediate overflow

Overflow is a property of `double` rather than of any indicator, so it carries
no flag, and issue #191 settled that it gets no semantics.

### Stream opening order

They follow the batch tier's order, but are not the batch conditions read on
`[0, historyLen - 1]`: an empty history answers `TA_OUT_OF_RANGE_START_INDEX`
where `batch(0, -1)` answers `TA_OUT_OF_RANGE_END_INDEX`, and rS5 requires each
input to equal the history's length where rB5 accepts a longer one.

### rS4

The presence checks sit on the public frame, because `<N>_OpenImpl` reads the
history's length before anything else could look at it.

### rS5

The input half is checked first, on the public frame: the core makes the test
too, but only after the capacity bound would have answered, so a short input
series was reported as an output-capacity fault.

The output half is rB5's produced count read over the same range, which collapses
it to `historyLen - lookback`, never the width of the history. It has no
legitimate zero case the way rB5 does: rS8 refuses a history shorter than
`lookback + 1`, so a fill that runs writes at least one value. A short history is
floored to zero so that it reaches rS8; a lookback of `-1` is not floored but
raised, which is what keeps rS3 ahead of the buffer rules, as the batch tier's
`clampedStart` does.

It sits on the **public** frame, never on `<N>_OpenAndFillInternal`: that seam
takes an anchor and writes `historyLen - max(lookback, startIdx)`, fewer, so the
same bound there would reject the composed sub-calls that pass a non-zero anchor.
It would also be redundant: a composed destination is proved disjoint by
`SubCallStep::is_fusable` and sized by construction. The frame reads the count
from the function's own `<N>_Lookback`, whose default substitution and range
validation come with it. A `nullable` output is bounded only when supplied.

### rS6: no in-place OpenAndFill

Not because in-place would compute the wrong answer: measured, it does not. The
fill's writes stop where the handle's warm-up seeds begin, or overlap them by the
single slot the next `Update` rewrites first. The ban is there because that
margin is an accident of every body's arithmetic that nothing states or asserts,
and supporting in-place would promise it for every function in four backends,
permanently.

### rS8: short history

The warm-up check comes last because it is the one thing the batch tier has no
analogue for. All four converge on `TA_INSUFFICIENT_HISTORY`: across every
streaming function in every backend, each short-history arm reports this code
and no other. What they answered before the code existed is Appendix D item 8.

rS8 is also what a **composed** opener answers when a sub-call succeeds with zero
elements: the batch tier may report that as an empty range, but an opener has no
handle to mint over a range holding nothing. C is outside the sweep that pins it:
no fold pass runs for C, so its transcribed guard still tests both halves and
returns the sub-call's own code. No opener answers `TA_SUCCESS` over zero
elements today, but a composed indicator that made one would hand C's caller
`TA_SUCCESS` and a NULL handle where the other three answer rS8.

### rU4, rH5, rH6: advancing

rU4 is rS2 read one bar at a time: the next bar's index is `begIdx + count`, and
once that leaves `[0, INDEX_MAX]` the handle is being asked for a bar the batch
tier refuses to address, permanently. It is evaluated where the opener
evaluates rS1 and rS2, ahead of every presence check with only the handle's own
null test in front of it, because it is the same fault read on the same domain,
and a caller who fixed the argument the presence check named would only get rU4
back. `Peek` is exempt for performance; it commits no bar.

A composed step drives its sub-handles through their public `Update`, so each of
them re-reads rU4. It cannot fire there first, and that is load-bearing: every
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

### rT3, rT6: bounds

Both values are added to a lookback which is then used as an index: unbounded,
the lookback overflows negative and the function indexes far past the end of its
input while still reporting success. `INDEX_MAX` is the ceiling the index domain
already enforces, and a warm-up longer than the largest addressable series could
never produce output, so nothing legitimate is refused.

### rT7: NaN only

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
   | Rust batch: `startIdx` guard, `endIdx` guard, lookback (which returns rB3), every input, then every output length-checked | every function |
   | Rust numerics: `startIdx` guard, `endIdx` guard, bounds asserts (following the FMA dispatcher to the real core) | every function |
   | Java batch: clamp (which raises rB3), then every length check, then the core | every function, both overloads |
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
  [rM1](https://ta-lib.org/spec/errors/#rm1) and
  [rM2](https://ta-lib.org/spec/errors/#rm2). The rest answers differently today
  (C returns 10 or 11 for an unbound argument, C#'s `TryCall` the same codes,
  C#'s `Call` and Java plain exceptions, Rust `BadParam`); the public page leaves
  it unspecified.
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
| ~~1~~ | C | rB4 | *Fixed.* The `OutRange` pointers were not null-checked, so passing either as null segfaulted. The batch prologue now checks them, as the streaming `OpenAndFill` prologue always has. |
| ~~2~~ | C | rB4 | *Fixed.* Input-buffer presence was checked *before* parameter validation and output presence after, so the prologue straddled rB3. Parameter validation now precedes every presence check. |
| ~~3~~ | Java | rB4 | *Fixed.* Buffer presence was checked *before* the index and parameter rules, inverting the specified precedence: a negative `startIdx` with a null input reported the null. The wrapper now evaluates rB1, rB2 and rB3 first. |
| ~~4~~ | Java | rB3 | *Fixed.* A null enum parameter yielded a raw JVM `NullPointerException` naming neither function nor parameter; it is now a parameter outside its domain, named, and carrying `TA_BAD_PARAM`. |
| ~~5~~ | | | *Withdrawn, not fixed.* Partial output/input overlap in C. Decided in #225: detection stops at buffer identity, and partial overlap is the caller's to avoid. |
| ~~6~~ | Java | rS4 | *Fixed.* A null history, or a null `OpenAndFill` output, yielded a raw JVM exception from inside the algorithm. The public openers now check every argument, and item 13 fixed the order they are checked in. |
| ~~7~~ | C# | rS1 | *Fixed.* The empty-history *message* omitted the cross-language `<N> open: ` prefix. Taken with item 13. |
| ~~8~~ | all | rS8 | *Fixed.* `TA_RetCode` had **no member** for "history shorter than the lookback", so C and Rust fell back to the catch-all and Java and C# borrowed `TA_OUT_OF_RANGE_END_INDEX`. `TA_INSUFFICIENT_HISTORY = 17` was appended and all four now report it, which leaves rS2 as the opening tier's only producer of `TA_OUT_OF_RANGE_END_INDEX`. |
| ~~9~~ | Rust, Java, C# | rS5 | *Fixed.* `OpenAndFill` validated no output capacity, so an undersized output faulted inside the fill with the buffer already partly written. The public frame now bounds every output by `historyLen - <N>_Lookback(...)`. (C still cannot: no sizes.) |
| ~~10~~ | C | | *Obsolete.* `TA_SetCompatibility` accepted any value; #388 removed the behaviour it selected, and the pair is kept declared and inert. |
| ~~11~~ | C#, Rust | rB6 | *Fixed.* Two distinct **empty** output buffers were rejected as aliased in C# and Rust and accepted in C and Java, measured on `ACCBANDS(0, 251, …, optInTimePeriod 253, …)` with three distinct zero-length outputs. Both now accept them (#262). |
| ~~13~~ | all | rS1 | *Fixed.* An empty history answered `TA_BAD_PARAM` where rS1 specifies `TA_OUT_OF_RANGE_START_INDEX`, and C checked argument presence ahead of the index pair. All four openers now answer the pair ahead of every presence check, except for the one check each language makes a precondition of reading the length (footnote [4]). |
| ~~14~~ | Java | rL2, rL3, rL11 | *Fixed.* A lookback call did not check a null MA type: `maLookback(1, null)` returned 0 and `maLookback(2, null)` threw `NullPointerException` from its `switch`, and every lookback with an MA-type parameter did the same. Each now returns -1, as rB3 rejects the batch call. |
| ~~15~~ | C# | rB6, rS6 | *Fixed.* One zero-length array passed as two outputs, on a range that produces no values, answered `TA_SUCCESS` in C# (`Overlaps` is false for an empty span) and `TA_BAD_PARAM` in C and Java. Ruled 2026-10-01: outputs must be different buffers in every language. C#'s batch and `OpenAndFill` guards now also reject two empty outputs on the same non-null reference. |

Next item: 16.
