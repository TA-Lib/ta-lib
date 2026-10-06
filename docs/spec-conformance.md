# Spec Conformance

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

`—` is not a pass. It means the language removes the failure mode (safe Rust
cannot pass one buffer twice, a Rust output that is not an `Option` cannot be
omitted, an enum has no out-of-domain value), so there is nothing for the backend
to check and nothing for a caller to hit.

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
| rB4 | [absent argument](https://ta-lib.org/spec/errors/#rb4) | ✅ | ✅<br>[1] | ✅ | ✅<br>[2] |
| rB5 | [buffer length](https://ta-lib.org/spec/errors/#rb5) | ⚠️<br>[3] | ✅ | ✅ | ✅ |
| rB6 | [one buffer, two outputs](https://ta-lib.org/spec/errors/#rb6) | ✅ | ✅ | ✅ | ✅ |
| rB7 | [omitted output](https://ta-lib.org/spec/errors/#rb7) | ✅ | —<br>[20] | ✅ | ✅<br>[2] |
| rB8 | [allocation failure](https://ta-lib.org/spec/errors/#rb8) | ⚠️<br>[23] | ⚠️<br>[23] | ⚠️<br>[23] | ⚠️<br>[23] |
| rB9 | [internal error](https://ta-lib.org/spec/errors/#rb9) | ⚠️<br>[21] | ⚠️<br>[21] | ⚠️<br>[21] | ⚠️<br>[21] |

[1] Rust slices cannot be null: an empty one is the absent argument. What C
writes through a pointer (the index/count pair, a stream handle) is returned
instead.

[2] A C# span cannot be null. A null array converts to an empty span, and an
empty span is the absent argument: rB4 or rS4 for an input or for an output that
cannot be declined, rS1 for a stream's history.

[3] C has no sizes to check against: a property of the ABI, not a defect.

[20] Rust takes a non-declinable output as `&mut [T]`, which cannot be omitted;
an empty one is rB4.

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
| rS4 | [absent argument](https://ta-lib.org/spec/streaming/#rs4) | ✅ | ✅<br>[1] | ✅ | ✅<br>[2] |
| rS5 | [buffer length](https://ta-lib.org/spec/streaming/#rs5) | ⚠️<br>[3] | ✅ | ✅ | ✅ |
| rS6 | [aliasing](https://ta-lib.org/spec/streaming/#rs6) | ✅ | —<br>[22] | ✅ | ✅ |
| rS7 | [declined output](https://ta-lib.org/spec/streaming/#rs7) | ✅ | —<br>[20] | ✅ | ✅<br>[2] |
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
| rT3 | [unstable period values](https://ta-lib.org/spec/settings-threads/#rt3) | ✅ | ✅ | ✅ | ✅ |
| rT4 | [reading](https://ta-lib.org/spec/settings-threads/#rt4) | ⚠️<br>[14] | ✅ | ✅ | ✅ |
| rT5 | [range type](https://ta-lib.org/spec/settings-threads/#rt5) | ✅ | — | — | ✅ |
| rT6 | [average period bound](https://ta-lib.org/spec/settings-threads/#rt6) | ✅ | ✅ | ✅ | ✅ |
| rT7 | [factor domain](https://ta-lib.org/spec/settings-threads/#rt7) | ✅ | ✅ | ✅ | ✅ |
| rT8 | [no change on refusal](https://ta-lib.org/spec/settings-threads/#rt8) | ✅ | ✅<br>[16] | ✅ | ✅ |
| rT10 | [initial state](https://ta-lib.org/spec/settings-threads/#rt10) | ✅ | — | — | — |
| rT11 | [threads in C](https://ta-lib.org/spec/settings-threads/#rt11) | ✅ | — | — | — |
| rT12 | [a getter returns what was set](https://ta-lib.org/spec/settings-threads/#rt12) | ✅ | ✅ | ✅ | ✅ |

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
  5 and under each Auto level, and requires the same answer bit for bit. Its
  control overwrites one bar more, which most functions read. A lookback
  under a level that does not fit the series is a failure, not a skip.
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
- rL8 under an Auto level: `test_auto_stabilization.c` asks every function flagged
  identity at a period of 1, owner of an id or not, for its lookback at a
  period of 1 with every id on each level, and requires 0.
- rL6 under an Auto level: the count leg of `test_auto_stabilization.c`. With one id
  on a level, its owner's lookback minus its lookback at 0 must equal the
  rule, computed from the file's own copy of the rule table, and the owner
  must report the bits it reports at 0, starting that many bars later. It
  runs at the defaults, with every integer parameter at its minimum and
  tripled, and at the real parameters a rule reads (SWAK_BP's delta, MAMA's
  limits). A function flagged as owning an id that the file has no rule for
  fails. `test_stc.c` holds the STC line to `TA_EMA(fast) - TA_EMA(slow)` bit
  for bit under each level: the fast leg is placed by its own lookback, which
  under a level is not the slow leg's.
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

rL4 under settings: `server_verify_lookback_value` compares each server's
lookback with C's, for every function with every unstable period raised
(`test_func_unstable_shift`) and for every candlestick function under each row
of the candle settings matrix (`test_candlestick.c`), under regtest
`--codegen`. Under each Auto level: `xlang_auto_lookback_leg` (regtest
`--xlang-hash`), with every id on the level, on every accepted vector of the
lookback leg plus one with every integer parameter at its maximum, the only
way into VIDYA's saturating arm. An Auto count depends on the parameters, so
this is the one check that the four renderings of every rule agree; it fails
when too few of its cases are lengthened by a level. The ports' display-shift calls are compared with C's at default
settings only.

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
that produces no values (Appendix D item 15), with the pair rebound apart as
that leg's control. Java's and C#'s `BatchApiTest` pass one array as two outputs.

rB4 and rS4, an empty argument: an output that cannot be declined is refused when
empty, on a range that produces nothing and on a history too short to open, with
a one-element control for each. Java `anEmptyOutputIsAnAbsentOne` and C#
`AnEmptyOutputIsAnAbsentOne` (`BatchApiTest`), C# `StreamApiTest`, Rust
`an_empty_output_is_an_absent_one` (`tests/nullable_outputs.rs`),
`a_sub_lookback_range_frees_the_output_bound_and_not_the_input_bound`
(`tests/empty_range.rs`) and the short-history test of
`tests/stream_open_contract.rs`. The same tests refuse an empty declinable
output in Java and Rust, with `null` and `None` as the controls that decline, and
hold an empty span to declining in C#. An empty input was already refused by the
length rule.

rW7: `checkInPlaceAliasCorrect` (`test_abstract.c`),
`anOutputOnItsInputAnswersTheSame` (Java's `MetadataTest`) and
`AnOutputOnItsInputAnswersTheSame` (C#'s `MetadataTest`) place each real output
on each real input component of every function and require the answer of the
separate-buffer call bit for bit. The call ends 16 bars before the series does,
and those bars must keep their input values.

rP4: the float leg of regtest `--codegen` compares each server's float entry
point with its own double one on the same widened inputs, bit for bit: C over
the parameter sweep, Java and C# for every function at its default parameters
and at the default sentinels. It runs with all three on x86-64 only. C is also
held by `test_variants.c` in a bare run.

rP5: `validate_inputs`, `try_inject_parameters` and `validate_outputs`
(`docs_site.rs`) fail `generate` when a function page's Inputs, Parameters or
Outputs list differs from the call signature in name or order.

rW8: `check_flags` (`parser/yaml.rs`) fails `generate` on a pattern output
whose flags break its rules, and `validate_output_values` (`docs_site.rs`) on an
Output Values table that differs from the values the flags declare.
`test_candle_value_set` (`test_candlestick.c`) holds every value a candlestick
function writes on its series to the values its output's flags declare, a color
output's sign to its candle's color, and each +-200 that follows an earlier
value in the range to a live pattern of its sign that no 200 has confirmed; a
+-200 whose pattern may precede the range is held to nothing. An output declares
level 200 exactly when `test_hikkake_predicate_coverage` proves it writes +-200.
It requires each of
+-80, +-100 and +-200 to occur, a color output to fire, a 200 to be matched, and
most functions to fire. That every declared value occurs is held per function
by the MC/DC scenarios (`pb_check_mcdc`), whose firing cases must produce
exactly the values of level 100 and 80 the output's flags declare, and by
`test_hikkake_predicate_coverage` for the +-200 of CDLHIKKAKE and CDLHIKKAKEMOD.

rE3 at a batch call: in C, `abstract_rejected_call_writes_nothing`
(`test_abstract.c`) paints every output buffer, calls each parameter vector of
its sweep that the lookback rejects and requires `TA_BAD_PARAM` with no element
written. Its two floors count the vectors rejected inside their declared range,
the rejections a C body makes itself after the generated checks: at a bound
(MAVP's inverted window) and one step above the lower bound (FRAMA's odd
period). `checkIndexRangeRejected` holds the same paint over every index
rejection, for every function. Java's and C#'s `BatchApiTest` hold a canary
after rB5. In Rust, `a_rejected_call_writes_no_output` (`abstract_api.rs`)
paints the outputs of every function through a holder and requires `BadParam`
with no element written, on an input one bar short and on a last output of one
element with the others bound. It repeats that for each integer-range parameter
one step outside its range and where the lookback refuses it at a bound or one
step above the lower bound. A holder answers the index codes itself, so the
order of the public entry's own index checks stays with
`rust_public_entry_orders_the_argument_contract`.

rB9's band: `testEnumValueContract` (`test_internals.c`) requires
`TA_SetRetCodeInfo` to name each value from 5000 to 5999 `TA_INTERNAL_ERROR`,
and `TA_UNKNOWN_ERR` for every value from 0 to 0xFFFF that is not a pinned
member and for four values outside that walk.

### Stream opening conditions

rS1, rS2, rS4, rS5, rS7 and rS8 are mapped below; rS3 is driven by the parameter
leg of `test_open_contract.c` over the C openers; rS6 is mapped only for one buffer
passed as two outputs: `testBatchArgumentContract` passes one buffer as
ACCBANDS's first two `OpenAndFill` outputs on a history of lookback bars
(`TA_BAD_PARAM`), with a separate-outputs control answering rS8.

`testStreamShortHistory` drives rS1, rS2's rejecting side and rS8 in C: 9, 3 and 8
rejections, with rS8's 16 controls. Three of rS1's cases are *also* an absent
argument, which is what makes them about the order and not only the code.
`testBatchArgumentContract` drives rS4 with rB4's own argument shapes plus the
handle: 12 rejections and 5 controls, counted apart from rB4's. `BatchApiTest`
does the same for Java, and adds rS1's; `StreamApiTest` covers rS4 in C#, where
the condition is a zero-length span and so is rS1. In Rust
`tests/stream_open_contract.rs` covers rS1, rS2 and rS4's empty output. Java's `BatchApiTest`
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
requiring `outFAMA`, or to accepting an empty output, would stay green in
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

### rH3, rH6 and rH9

`stream_verify`'s fork leg, in each language server and for every function,
opens its original with `OpenAndFill` on the shortest history, updates it,
forks it, calls `Advance` twice on the fork, and feeds both to the end. Every
bar of both must match batch, so a handle that came from `OpenAndFill` is held
to rH1 under `Update` (rH3), and a counted bar reaches nothing a step reads
(rH6); the fork reports two bars more than its original, which is the resume
recipe's step of one `Advance` per bar counted without being fed. MININDEX,
MAXINDEX and MINMAXINDEX ride the same leg: their index after the counted bars
is still the batch index over the bars fed.

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

### Abstract API rules

rA1: in C, `checkHolderErrorContract` (`test_abstract.c`) ends on a fully bound
holder whose index or parameter the function refuses, which must come back as
the function's code. In the ports, through a bound holder: a short input and a
short output (`an_undersized_input_is_rejected_not_read_past`,
`an_undersized_output_is_rejected_not_written_past`) and both index codes
(`a_bound_holder_answers_the_batch_index_codes`) in Rust; `holderRejectsMisuse`
(Java) and `BinderRejectsMisuse` (C#) send a short input, a short output, an end
index below the start index and one array bound as two outputs. The generator's `rust_binder_calls_the_public_tier` pins that every
Rust arm calls the public entry point. Across languages: under `--codegen`,
`d2_param_vectors` sends each function's rejected parameter vectors through
`abstract_call`. No output declined: `checkHolderErrorContract` leaves only the
`TA_OUT_NULLABLE` output of each function that has one unbound (C, where an
empty buffer cannot be spelled); `no_output_is_declined_through_a_holder`
(Rust), `holderRejectsMisuse` (Java) and `BinderRejectsMisuse` (C#) bind MAMA's
second output to an empty buffer.

rA2: `testHolderStaysReusable` (C), `a_rejected_setter_leaves_the_holder_as_it_found_it`
(Rust), `aRejectedSetterLeavesTheHolderAsItFoundIt` (Java),
`ARejectedSetterLeavesTheCallAsItFoundIt` (C#), and the generator's
`metadata_price_setter_validates_before_writing` for all four.

rA3 in C: `checkHolderErrorContract` drives, for every function, each setter
with a wrong index, a wrong kind and a NULL, `TA_CallFunc` with the inputs, then
the outputs, unbound, with each NULL argument and with a holder the API did not
make. `test_default_calls` then checks, once, the lookups by name, the two table
frees, a handle the API did not give out, an absent one, and a parameter index
that names nothing. In the ports: `the_setters_refuse_what_does_not_belong`
(Rust), `holderRejectsMisuse` (Java) and `BinderRejectsMisuse` (C#), whose
helpers also require the exception to be a `TALibArgumentException`. Across
languages: under `--codegen`, `test_abstract.c` sends every function's
`abstract_call` with input 0, then output 0, left unbound (`skipInput`,
`skipOutput`) and requires C's code from each server. These tests pin today's
codes, which is more than rA3 asks: a deliberate refinement changes them
together.

rA4: `checkFuncHandleFoldsCase` (C), `lookup_is_ascii_case_insensitive`,
`the_fold_does_not_widen_what_resolves` and `unknown_name_is_none` (Rust),
`byNameFoldsAsciiCase` and `registryIsComplete` (Java), `ByNameFoldsAsciiCase`
and `CatalogueIsComplete` (C#).

rA5: in C, `callWithDefaults` calls every function through a holder with no
optional parameter set and, under `--codegen`, requires each server to give the
same answer with the declared defaults bound. In the ports:
`unset_matches_the_documented_default` (Rust), `callByNameMatchesTheTypedApi`
and `choiceListSentinelMatchesTheDefault` (Java),
`UnboundParametersTakeTheDocumentedDefault` (C#).

rA6: `test_default_calls` (C, over `TA_ForEachFunc`),
`every_function_binds_calls_and_agrees_with_its_lookback` (Rust),
`callByNameMatchesTheTypedApi` (Java), `BothCallPathsAgree` (C#). Each walks the
registry it tests. Across languages, under `--codegen`: `callWithDefaults` sends
an `abstract_call` for every function C enumerates, which each server resolves
in its shipped registry, and `abstract_verify_for_each_func` requires each
server's enumeration to be C's, as a set (Java's server enumerates a table of
its own).

rA7: the generator's `the_spec_flag_catalog_is_complete` requires every flag
member of the three ports to carry the value of the C `#define` its catalog row
names. The same test holds the catalog itself, which is more than rA7 asks: a
row for every flag `#define` of `ta_abstract.h`, no port flag without a row, no
flag family beyond the four it reads, and "No function sets it" exactly on the
flags no function carries.

### Settings rules

rT3's levels and rT12: `testUnstablePeriodBounds` (`test_internals.c`) sets
each level per id and through the wildcard, reads it back as itself, and
requires `TA_INDEX_MAX + 5` and `+ 9` refused with nothing written;
`test_unstable_bounds` (`test_codegen.c`) sends the same values to every
server; the ports' own suites read both levels back from a built `Core`
(the Rust template's tests, `CoreApiTest`, `CoreBuilderTest`).

rT4's C getter: `test_internals.c`. rT8's Rust latch: verified by probe
(footnote [16]). rT8's candle setter in Java: `aRejectedCandleSettingWritesNothing`
(`CoreApiTest`), through CDLDOJI since Java's `Core` has no candle getter. rT2,
rT4 and rT5 under a cast out of the enum in C#: `CandleMisuseThrows` and
`MisuseThrows` (`CoreBuilderTest`). rT10: `testUnstablePeriodBounds`
(`test_internals.c`) changes the settings and initializes over them.

`TA_Initialize` and `TA_Shutdown` are idempotent (ruled 2026-10-02): a repeated
call of either answers `TA_SUCCESS` and leaves every setting at its default,
settings changed in between included. The public contract still asks for one
call of each, so this is held to, not promised: `testUnstablePeriodBounds`
calls each twice over changed settings. Nothing reads an initialized flag, so
`TA_LIB_NOT_INITIALIZE` has no producer.

rT11: `scripts/thread_sanitize.py` (nightly) builds the library and
`scripts/thread_sanitize.c` with clang's ThreadSanitizer and runs eight threads
through, for every function, its double and float batch entry points, its
lookup by name, lookback and `TA_CallFunc`, and its `Open`, `OpenAndFill` and
`Close`; then, for a spread of stream shapes, through `Peek`, `Update` and
`Advance` on a handle of their own, and `Peek`, `Value`, `OutRange` and `Clone`
on handles they share. The sanitizer's report is the test: a function-static
counter, or a `Peek` that stores into its handle, returns the right values on
every thread, so comparing answers finds neither. Not run on several threads:
the `Update`, `Peek`, `Advance` and accessors of the functions outside that
spread, and
the fused clones gcc builds, since under clang the FMA dispatch compiles away.

### Auto levels

What the specification states about a level is mapped above: rT3 and rT12
under Settings rules, rL1, rL4 and rL6 under Lookback rules. Two more legs run
under each level.

Stream against batch: `stream_verify`'s unstable-period leg runs one more pass
per level, in each language server, on the default vector of a function an
unstable id reaches, on every list-parameter vector and on the below-default boundary
vectors. MACDEXT gains an all-EMA vector at periods (7, 8, 2): its batch
delegates to MACD while its stream composes three MAs, each placed by its own
lookback, and under a level the two agree only if MACD places its fast leg the
same way: shorter periods forget a misplaced seed before the first bar
compared at `PREC_8`. The default all-EMA vector reports too few bars on the 240-bar
series to show it. A floor on the output bars compared under a level keeps
the passes from comparing nothing.

Two starts: the second leg of `test_auto_stabilization.c`, in a bare run. It holds
what the [Unstable Period](https://ta-lib.org/api/unstable-period/) page says
a level means, which is not a specification rule. Every function runs on three
synthetic 8192-bar series (a random walk, alternating trends, a range), from
bar 0 and from six later starts, at the defaults, with every integer parameter
at its minimum and tripled, at every MA type, and at the real parameters a
rule reads. Compared at every bar both runs report:

- a call whose lookback no level moves: within `T = max(1e-10, 1e-7 * range)`,
  integer outputs equal;
- a converging call: with `S` the largest difference at a setting of 0, within
  `max(e^-(K-3) * S, T)` at `PREC_4` and within `max(e^-(K-3) * S, F)` at
  `PREC_8`, with `F` a committed floor per function and output, for the
  outputs whose own rounding is above that threshold. A call whose `S` is
  under `T` is not a comparison;
- a function flagged `path_dependent` is not compared, and fails if it meets
  the converging criterion on every series and start.

Not compared: KAMA, FRAMA, VIDYA and the KAMA and VIDYA MA types on the range
series, which their counts are not sized for; STOCHRSI with the MAMA or VIDYA
type, whose FastK rests at 0 or 100; CRSI when a level lengthens its lookback,
its streak being a state machine. MAXINDEX, MININDEX and MINMAXINDEX are
compared after rebasing the later start's index, which counts from the first
bar handed in. One counter per class and a floor on the bars compared, per
series for a window and per call for a converging one, keep the leg from
passing on nothing; a call that fails or reports no lookback is a failure.

### Versions and determinism

rV2: `testEnumValueContract` pins C's numbers. `MAType` reaches every language
from `enums.yaml`, and `FuncUnstId` reaches C, Java and C# from it; Rust's
`FuncUnstId` is a template that `generate` refuses when its member names or
their order differ from `enums.yaml`, with its wildcard's number pinned by
`rust_funcunstid_pins_the_all_sentinel`. `RangeType` and `CandleSettingType` are
written by hand in C's header and in each port:
`every_backend_candle_enum_member_carries_c_s_number` (generator suite) reads
C's numbers from `ta_defs.h` and requires them of every Rust, Java and C#
member.

rV3: `rust_matype_emits_every_yaml_variant_and_its_frozen_shape` and
`rust_template_enums_are_non_exhaustive` (generator suite).

rD2's list of functions: `the_transcendental_list_is_what_the_sources_call`
(generator suite) derives it from the indicator sources and requires the same
set on the versions page and in `CODEGEN_TRANSCENDENTAL[]` (`test_codegen.c`),
the list that moves a Java or C# comparison from bitwise to a tolerance.
`test_elementary_math` (`test_1in_1out.c`) holds each elementary function to
the host math library's routine of that name.

The build-flags caller item: `the_three_build_systems_carry_the_same_flags`
(generator suite) requires the statements that set `-ffp-contract=off`,
`-fno-math-errno` and the two alignment flags in CMake, autotools and the
generator, and `-ffast-math` and `-Ofast` in none.

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

The range out-parameters in C. The public rule is the weak one, to keep room
to change: `*outBegIdx` and `*outNBElement` may or may not have been written
when a call does not return `TA_SUCCESS`, so a caller reads them after
`TA_SUCCESS` only. The library holds itself to more, and the suite tests it:

- `TA_INSUFFICIENT_HISTORY` writes 0 to both. Nothing was output, and a caller
  that skipped the return code reads an empty range instead of stale indices.
- Every other rejection leaves both as the caller had them. The aim is one
  write of the pair, at exit, on success; a body that still writes it earlier
  does so after its last argument check.
- Nothing holds after rB8 or rB9: some bodies zero the pair on those exits,
  others leave it.

Coverage, each call painting the pair with a sentinel first: every `TA_CallFunc`
the suite makes (`regtest_guarded_call`, which `ta_test_priv.h` substitutes for
it, failing the run at `freeLib`); the double and float entry of every function
at each in-range parameter vector its body refuses (`test_variants.c`); every
function's `OpenAndFill`, held to (0, 0) at each too-short history and to the
sentinel at each out-of-range parameter (`test_open_contract.c`); FRAMA's and
MAVP's `OpenAndFill` at the parameters their bodies refuse (`test_frama.c`,
`test_mavp.c`); the absent-argument rows of `test_internals.c`.

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

The settings builders raise plain platform types for their own refusals: they
are not function calls, so they have no `TA_RetCode` a caller would be
recovering. The Abstract API's refusals carry a code (rA3), and its
*dispatch* calls the public entry point, so an indicator's rejection reaches the
caller carrying its own code in every backend.

C#'s rU4 is a `TALibArgumentException`, not the
`TALibArgumentOutOfRangeException` its index codes take in batch: `Update` and
`Advance` have no `endIdx` parameter to name. An opener's rS1 `ParamName` is the
history series, the argument a caller can change, since an opener has no
`startIdx`.

A backend enum carrying a member it never produces is harmless; one missing a
member it needs is not.

Known gap: Java's and C#'s MA stream throw a plain `IllegalStateException` /
`InvalidOperationException` ("unreachable: open rejects arms without a
sub-stream", `Core_MA.java`, `Core_MA.cs`) with no code and no prefix.
Unreachable by construction.

### rB4, rS4: empty is absent

An argument that cannot be declined is absent when it is `NULL` in C, null or
zero-length in Java, zero-length in Rust and C#, and the call answers
`TA_BAD_PARAM` at the absent check: after the index and parameter conditions,
before any length (ruled 2026-10-02). A zero-length buffer is almost always a
mistake, and a caller cannot cheaply tell whether a call will produce nothing.
"No room is owed when nothing is written" protected no real use, and hid a null
buffer behind a success or behind `TA_INSUFFICIENT_HISTORY`. C sees only `NULL`:
a buffer of no capacity is invisible there, as every length is.

Two things are not part of it. A stream's empty history answers rS1, which sits
ahead of rS4 as `historyLen < 1` sits ahead of C's NULL checks. An empty span
is how C# declines an output, the one case that is not refused: Java and Rust
decline with `null` and `None`, so an empty declinable output is absent there
like any other.

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

Outputs must be different buffers in every language. An empty output that is
not a declination is absent (rB4) and never reaches a verdict of the pair guard.
In C# an empty span declines, and the guard still rejects one zero-length array
passed as two declinable outputs (ruled 2026-10-01, Appendix D item 15), which
no shipped function has; a span with a null reference is the only operand it
skips. A call that is both undersized and identical answers rB5, the earlier
rule (#261).

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
call on a type that already answers the question (ruled 2026-10-02), and made
last so that a call with another fault answers the code C and Java give it: in
the batch tier after the same-buffer guard, at an opener after the history
check. The same buffer at an opener is one start address, as C compares it,
and stays ahead of the history check in every language. If uniformity is ever
preferred over the extra safety, removing it is the change, not adding the check
elsewhere.

rW7 is supported, not tolerated: several bodies are written for it and elect
their scratch by testing for exactly that case. That election is why STOCH,
STOCHF and KDJ leave intermediate values past `count` when their first output is
in place (ruled 2026-10-03: accepted). The scratch spans the values the
smoothing consumes and never reaches past `endIdx`, which is the limit the
public rule states.

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
function and the state field it guards. The Abstract API hand-allocates
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
and does not: `Option` is the shape a Rust caller expects, and it makes the
declination visible at the call site.

An omitted output is not an alias, so rB6's pair guard skips a pair whose
operands are not both present. C and Java guard each nullable operand non-null
before comparing (two `NULL`s compare equal), and C# skips a span whose
reference is null, which is what a null array becomes; an empty span over a real
array is still compared. Rust's guard compares the addresses of the slices supplied; safe
code cannot pass one slice twice, so that is unobservable.

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

An Auto level is `INDEX_MAX` plus its digit count. Above `INDEX_MAX`, so no
count is a level. Below 2^31, so a Java or C# `int` holds one and a negative C
`int` stays above it and refused. Not `INT_MAX`, which the C regtest server
uses to saturate an oversized integer on the wire. Close to the ceiling, so
that code reading a level as a count gets no output or
`TA_INSUFFICIENT_HISTORY`, never an index out of range. The offsets between
and beside the levels stay refused. A level's lookback fits an `int` at every
parameter's maximum: VIDYA's rule, the one product of two periods, saturates
at `INDEX_MAX`.

### rT12: the getter returns the level

A getter that answered 0, or the resolved count, under a level would turn the
level off, or into a fixed count, in a caller that saves a setting and restores
it. A caller that adds the getter's value to a lookback of its own gets a
number that is plainly wrong, not one that is quietly short. No call returns
the count a level resolves to: it is the lookback under the level minus the
lookback at 0, and it depends on the call's parameters, which a getter does
not take.

### rT7: finite and not negative

A factor scales a threshold (ruled 2026-10-02). NaN silences every comparison it
feeds, which is indistinguishable from "this shape never occurs". An infinity
does the same on flat bars, where it multiplies a zero average into NaN. A
negative threshold is below every range, so a test that a range exceeds it
always passes and its opposite never does. None of the three is a threshold a
caller means, and zero is the edge that is.

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

- **Which code the Abstract API answers for a given misuse**:
  [rA3](https://ta-lib.org/spec/abstract/#ra3) names the set and leaves the choice
  open, so a backend can become more specific. Today all four answer 10 for an
  unbound input, 11 for an unbound output and 8 for a mistyped setter.
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
| ~~11~~ | C#, Rust | rB6 | *Fixed.* Two distinct **empty** output buffers were rejected as aliased in C# and Rust and accepted in C and Java, measured on `ACCBANDS(0, 251, …, optInTimePeriod 253, …)` with three distinct zero-length outputs. Both then accepted them (#262). Superseded 2026-10-02: an empty output that cannot be declined is absent (rB4), so the call is refused in Rust, Java and C#. |
| ~~13~~ | all | rS1 | *Fixed.* An empty history answered `TA_BAD_PARAM` where rS1 specifies `TA_OUT_OF_RANGE_START_INDEX`, and C checked argument presence ahead of the index pair. All four openers now answer the pair ahead of every presence check, except for the one check each language makes a precondition of reading the length (footnote [4]). |
| ~~14~~ | Java | rL2, rL3, rL11 | *Fixed.* A lookback call did not check a null MA type: `maLookback(1, null)` returned 0 and `maLookback(2, null)` threw `NullPointerException` from its `switch`, and every lookback with an MA-type parameter did the same. Each now returns -1, as rB3 rejects the batch call. |
| ~~15~~ | C# | rB6, rS6 | *Fixed.* One zero-length array passed as two outputs, on a range that produces no values, answered `TA_SUCCESS` in C# (`Overlaps` is false for an empty span) and `TA_BAD_PARAM` in C and Java. Ruled 2026-10-01: outputs must be different buffers in every language. C#'s batch and `OpenAndFill` guards now also reject two empty outputs on the same non-null reference. |

Next item: 16.
