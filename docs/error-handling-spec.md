# API Error-Handling Specification

What every TA-Lib backend must do when a call is **wrong**, and which backends
have been verified to do it.

Normal behaviour — what a *correct* call returns — is specified by the tests
(`ta_regtest`, the value gates, `--xlang-hash`). It is summarised in Part 1 only
where it is routinely mistaken for an error. The rest is failure behaviour:
Part 2 is what the library detects and reports, Part 3 what it does not detect.

## How to read this

### The document is backend-neutral; the tables are the tracker

Each rule is stated once, in language-neutral terms: *this condition produces
this error*. **How** an error reaches the caller is a property of the language,
not of the rule, and is written down exactly once, in Appendix A: C and Rust
report a code, Java and C# raise. No rule repeats it.

The columns are a conformance tracker, one per shipped backend. A new backend
adds a column; it does not change a rule.

### Marks

| Mark | Meaning |
|:---:|---|
| ✅ | Verified: a probe against the built artifact produced the specified behaviour. |
| ⚠️ | Deviates deliberately, **or** is implemented but not verified by the CI test suite. The rule or its footnote says which, and where a deviation was decided. Held as approved on that basis — flip any to ❌ that should not have been. |
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
a multi-fault call predictable, and it is the part most easily broken by adding a
check in the convenient place rather than the specified one.

Two consequences worth stating outright:

- **One error per call.** A call reports one condition and stops; it never
  accumulates.
- **Checks precede writes.** A rejected call leaves every caller-owned buffer,
  and every out-parameter, exactly as it found them. Verified for C at each
  rejection class (index, parameter, absent argument): output buffer untouched,
  index and count out-parameters untouched.
- **Order matters only where the codes differ.** Rules that answer the same code
  are mutually unordered in practice: a caller cannot tell which of them fired,
  so swapping two is invisible. What has to hold is that all four backends answer
  the **same code for the same call**, which `--xlang-hash` compares over the
  whole corpus. A per-backend ordering gate is therefore not owed for rules that
  share a code.

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
backends that raise (Appendix A) — and the ⚠️ on the code says C cannot detect it
at all. Rule S8 reads *(none)* because no backend detects it.

**`OutRange`.** What a successful call reports about its own output: the index
of the first value written, in the input series' coordinates, and how many values
follow it. Only that range is written — see N2 — and a call that produces nothing
reports an empty one, which is a success (N1). C returns the two numbers through
the `*outBegIdx` / `*outNBElement` out-parameters; the other three carry them as
one `OutRange` value. A live stream handle carries one as well — the bars it has
produced a value for, which is what the batch call over those same bars reports.
Appendix A has where it arrives, per backend and per tier.

**`MAX_INDEX`.** The addressable index ceiling, 100 000 000, identical in all
four backends.

---

## Part 1 — Normal behaviour

The conditions most often mistaken for failures. None of them is reported as an
error, and none of them is undefined — the conditions the library leaves
undefined are collected in Part 3.

| Rule | Condition | Result |
|---|---|---|
| N1 | A **valid range shorter than the lookback** | Success, zero values produced, an empty `OutRange`. Never an error. |
| N2 | Anywhere outside the reported `OutRange` | Untouched. The library never pads, and never emits a fill value: only the reported `OutRange` is written. |
| N3 | An optional parameter set to its **default sentinel** | The documented default is substituted, then validated like any other value. |
| N4 | An output buffer that **is** an input buffer (whole-buffer, in place) | Allowed, in the batch tier. Several bodies are written for it. |
| N5 | A **negative** candlestick `factor` | Legal. It does not "never match" — it makes the comparison unconditionally true. |
| N6 | The set-all / restore-all **wildcards**, where a setter documents one | Legal on those setters, and rejected on the ones that name a single target (rule G1). |
| N7 | **Peeking** a forming bar, any number of times | Never advances the stream and never writes the handle. It can still be *rejected* (U3), and a rejected peek changes nothing either. |
| N8 | Buffers that **partially** overlap — same memory, different start | **Unspecified.** Only *identical* buffers are detected (rule B6). See Appendix E before assuming a diagnosis. |

These behaviours are pinned by the value gates and the cross-language hash, over
the whole corpus, on every test run.

---

---

## Part 2 — Error specification

### 2.1 Lookback tier

| Rule | Condition (in order) | RetCode | C | Rust | Java | C# |
|---|---|---|:---:|:---:|:---:|:---:|
| L1 | An optional parameter is outside its documented range | the lookback rejection signal | ✅ | ✅ | ✅ | ✅ |
| L2 | …and the signal is returned **exactly when** the batch tier would reject the same parameters under B3, or the streaming opener (`Open`/`OpenAndFill`) would reject them under S5 | the lookback rejection signal | ✅ | ✅ | ✅ | ✅ |
| L3 | …and, for Rust/Java/C#, the same accept-or-reject decision — and, wherever both it and C accept, the Batch/`OpenAndFill` output — matches what C produces for the identical parameters (the "Golden Check") | the lookback rejection signal | — | ✅ | ✅ | ✅ |
| L4 | Nothing else in this tier can fail | — | ✅ | ✅ | ✅ | ✅ |

**Rejection Signal**
For C, Java and C# it is returned as `-1`.
For Rust it is returned with `Result<usize, RetCode>` as `Err(RetCode::BadParam)`.

**Implementation**: `xlang_lookback_leg`, `xlang_tier_native_check`,
`xlang_tier_gold_check` (`test_codegen.c`, under `--xlang-hash`). Issue #256.

### 2.2 Batch tier

| Rule | Condition (in order) | RetCode | C | Rust | Java | C# |
|---|---|---|:---:|:---:|:---:|:---:|
| B1 | `startIdx` outside `[0, MAX_INDEX]` | `TA_OUT_OF_RANGE_START_INDEX` | ✅<br>&nbsp; | ✅<br>&nbsp; | ✅<br>&nbsp; | ✅<br>&nbsp; |
| B2 | `endIdx` outside `[0, MAX_INDEX]`, **or** `endIdx < startIdx` | `TA_OUT_OF_RANGE_END_INDEX` | ✅<br>&nbsp; | ✅<br>&nbsp; | ✅<br>&nbsp; | ✅<br>&nbsp; |
| B3 | An optional parameter is outside its documented range (metadata from .yaml). A non-finite value (NaN, ±Inf) always returns an error. Note that non-finites as elements of input arrays are not detected or supported (See Part 3, "Non-finite input") | `TA_BAD_PARAM` | ✅<br>&nbsp; | ✅<br>&nbsp; | ✅<br>&nbsp; | ✅<br>&nbsp; |
| B4 | A required argument was not supplied — any declared input or output buffer, or missing `OutRange` pointer(s) | `TA_BAD_PARAM` | ✅<br>&nbsp; | —<br>[1] | ✅<br>[2] | —<br>[3] |
| B5 | A buffer is too short: every declared input must reach `endIdx`, an output must hold the count actually produced (`endIdx - max(startIdx, lookback) + 1`). On a range shorter than the lookback that count is 0, so no output space is needed — but the input bound still holds | `TA_BAD_PARAM` ⚠️ | ⚠️<br>[4] | ⚠️<br>[5] | ✅<br>&nbsp; | ✅<br>&nbsp; |
| B6 | Two outputs are the **same buffer** (Appendix E) | `TA_BAD_PARAM` | ✅<br>&nbsp; | ✅<br>[6] | ✅<br>[7] | ✅<br>&nbsp; |
| B6a | An output is **omitted** — null, or zero-length where the language cannot spell null. Accepted only where the .yaml marks that output `nullable` (Appendix F) | `TA_BAD_PARAM` | ✅<br>&nbsp; | ✅<br>&nbsp; | ✅<br>&nbsp; | ✅<br>&nbsp; |
| B7 | A memory allocation failed. Only C reports it — Rust aborts, and the managed runtimes raise their own out-of-memory error. **Warning: implemented, but no CI job or probe covers it** — provoking one needs a failable allocator | `TA_ALLOC_ERR` ⚠️ | ⚠️<br>&nbsp; | ⚠️<br>&nbsp; | ⚠️<br>&nbsp; | ⚠️<br>&nbsp; |
| B8 | The library detected an inconsistency in its own state — a likely bug, please report it to the TA-Lib developers (Appendix A, "Internal errors"). **Warning: implemented, but the individual sites are not tested** | `TA_INTERNAL_ERROR` ⚠️ | ⚠️<br>&nbsp; | ⚠️<br>&nbsp; | ⚠️<br>&nbsp; | ⚠️<br>&nbsp; |

**Range.** A real or integer parameter's range is the `range:` its .yaml
declares; an enum parameter's is the declared member set of its type.

**Every declared input.** B4 and B5 cover every input a function declares — the
seven candlestick legs whose algorithm never reads them included: `inHigh` and
`inLow` on CDL3OUTSIDE, CDLENGULFING and CDLXSIDEGAP3METHODS, `inOpen` on
CDLHIKKAKE. Rust, Java and C# used to exempt exactly those, computing the set
from the body, while C checked them like any other input, so
`TA_CDL3OUTSIDE(0, 251, open, NULL, NULL, close, …)` was `TA_BAD_PARAM` in C and
a success in the other three. Closed by #260: a declared input must be supplied,
and that rule needs no exception list. What survives is not an exception but a
range: Rust's assert preamble is switched off on a sub-lookback range, uniformly
for every input of every function — footnote [5].

**Implementation**: `testIndexRange`, `checkOutputAliasRejected`,
`testBatchArgumentContract` (`test_abstract.c`, `test_internals.c`);
`c_batch_prologue_orders_parameters_before_presence`,
`rust_batch_impl_orders_capacity_before_aliasing`,
`every_declared_input_is_checked_in_every_backend` (ta_codegen's suite);
`BatchApiTest` (Java, C#); `no_phantom_io` (Rust); `--xlang-hash` for
same-code-everywhere.

[1] Slices cannot be null, and the index/count pair (`OutRange`) is returned
rather than written through pointers.

[2] Reported like any other `TA_BAD_PARAM`, naming the function and the
argument. Java used to spell an absent argument as a `NullPointerException`,
which made B3 and B4 distinguishable in Java and nowhere else.

[3] A `Span<T>` cannot be absent. A null array converts to an empty span and is
reported by B5 instead, which names the buffer and both sizes.

[4] C has no sizes to check against — a property of the ABI, not a defect. Part 3
has what happens instead.

[5] Rust panics rather than reporting: the assert is what lets LLVM elide the
per-access bounds checks — and, since #260, also what states B4 and B5 for the
seven legs no body indexes, where it proves LLVM nothing and rejects the caller
anyway. It is skipped on a sub-lookback range, so there the input bound goes
unchecked in Rust as it does in C — and unlike C, so does the presence half:
Rust is the one backend that accepts an omitted input on a range that produces
nothing. Removing that escape is not free — it is what lets `no_phantom_io`'s
sub-lookback sweep hand the body zero-length slices and observe what it touches.

[6] **The condition cannot be written**: two `&mut [f64]` cannot alias, so the
borrow checker rejects it at compile time. The `as_ptr()` guard behind the public
entry point therefore never fires at all in safe code; it is kept for parity, and
for the FFI boundary. It requires **both** operands to be non-empty, because two
zero-length slices cannot clobber each other — without that it fired on distinct
empty `Vec`s, which share a dangling pointer, while accepting distinct empty
*subslices*, which do not (Appendix D item 11, #262). It is emitted *after* the
capacity asserts: B5 panics in Rust where B6 returns, so this is the one pair of
batch rules whose order a caller can see in one backend and not the others. A
call that is both undersized and "aliased" answers the panic; it used to answer
`BadParam` — #261.

[7] **A run-time error.** Two Java arrays are either the same object or
disjoint, so reference equality catches every case this rule covers.

### 2.3 Streaming tier — opening (`Open`, `OpenAndFill`)

| Rule | Condition (in order) | RetCode | C | Rust | Java | C# |
|---|---|---|:---:|:---:|:---:|:---:|
| S1 | A required argument (handle, history, output) was not supplied | `TA_BAD_PARAM` | ✅<br>&nbsp; | —<br>&nbsp; | ❌<br>[8] | —<br>&nbsp; |
| S2 | The history is empty | `TA_BAD_PARAM` | ✅<br>&nbsp; | ✅<br>&nbsp; | ✅<br>&nbsp; | ✅<br>[9] |
| S3 | The history is longer than `MAX_INDEX + 1` | `TA_OUT_OF_RANGE_END_INDEX` | ✅<br>&nbsp; | &nbsp;<br>[10] | &nbsp;<br>[10] | &nbsp;<br>[10] |
| S4 | *(withdrawn — the warm-up history is an input array; see "Non-finite input")* [11] | — | —<br>&nbsp; | —<br>&nbsp; | —<br>&nbsp; | —<br>&nbsp; |
| S5 | An optional parameter is outside its documented range | `TA_BAD_PARAM` | ✅<br>&nbsp; | ✅<br>&nbsp; | ✅<br>&nbsp; | ✅<br>&nbsp; |
| S6 | The history holds fewer than `lookback + 1` bars | `TA_INSUFFICIENT_HISTORY` | ✅<br>[12] | ✅<br>[12] | ✅<br>[12] | ✅<br>[12] |
| S7 | (`OpenAndFill`) an output aliases an input, or another output | `TA_BAD_PARAM` | ✅<br>&nbsp; | —<br>&nbsp; | ✅<br>&nbsp; | ✅<br>&nbsp; |
| S8 | (`OpenAndFill`) an output is too short for the fill | *(none)* | ⚠️<br>[4] | ❌<br>[13] | ❌<br>[13] | ❌<br>[13] |

Rule S6 is the library's only *recoverable* condition — "feed me more bars", not
"your code is wrong" — so it must be distinguishable from `TA_BAD_PARAM`, which
is always a programming error. It has carried its own code since
`TA_INSUFFICIENT_HISTORY` was appended.

Unlike the batch tier, `OpenAndFill` outputs may **not** alias the inputs at all:
that path writes the outputs and then reads the input tail to seed its rings.

[8] The history's length is read with no guard, so a null history is a raw runtime
`NullPointerException` naming nothing; a null `OpenAndFill` output faults inside
the fill loop.

[9] Reported without the `<NAME> open: ` message prefix that
`docs/streaming-api-design.md` names as a cross-language contract; the class is
right, the message is not.

[10] Not probed — the condition needs a >100 000 001-element allocation.

[11] **Withdrawn.** The warm-up history is an input *array*, and the library
does not scan those. Until this was removed it was the only array in the library
that was checked — 174 of 174 `Open` and 174 of 174 `OpenAndFill` entry points,
in all four backends — which made "arrays are never scanned" a rule with one
exception rather than a rule. The scan cost ≈0.3 ns per bar per array: a corpus
median of 22% of an `Open`, up to 76% of a candlestick `OpenAndFill`. Folding it
into the main loop instead would have traded that cost for a rejection partway
through a fill, with the output already half written. The measurement is in
`docs/streaming-api-design.md`. U3 is untouched: a bar handed to `Update` or
`Peek` is a single value.

[12] All four converge on `TA_INSUFFICIENT_HISTORY`. Before it existed C and
Rust returned the **same** code for "history too short" and "parameter out of
range", so a caller could not separate the recoverable condition from the
programming error, and Java and C# borrowed `TA_OUT_OF_RANGE_END_INDEX` in band
and re-typed it at the wrapper — separable, but on a code that is wrong on its
face, since a stream open has no `endIdx`. Two things followed from the borrow
and are gone with it: a history *longer* than `MAX_INDEX + 1` (rule S3, the
only other producer of that code) surfaced as the typed
`InsufficientHistoryException`, telling a caller its history was too short when
it was too long; and C# needed a second code-to-exception mapper
(`StreamFailure`) to keep short history off `ArgumentOutOfRangeException("endIdx")`.
`StreamFailure` remains — the `"<NAME> <verb>: "` message prefix is a
cross-language contract — but it no longer exists to hide a borrowed code.
Verified as uniform, not incidental: across all 172 streaming functions per
backend, every short-history arm in every backend reports this code and no other.

[13] No backend validates the fill output's capacity, in contrast with the batch
tier which does (B5). An undersized or absent output faults inside the fill loop
with the buffer already partly written — a raw index exception in Java and C#, a
panic in Rust. A *zero-length* output is the same story: the opener's own S6
check means the fill always has at least one value to write, so there is no
legitimate empty output here as there is in the batch tier (rule N1), and all
three fault rather than reject. Appendix D item 9.

[14] Only C has a bar count: the other three take slices or arrays, which carry
their own lengths, so "negative" is unrepresentable.

[15] C is handed bare pointers and has no sizes — the same blind spot as B5 and
S8. The other three answer both before committing anything, which makes
`UpdateAndFill` the one filling entry point whose output capacity IS validated
outside C; `OpenAndFill`'s (S8) still is not, and closing that is Appendix D
item 9.

[16] Rust cannot express it: `&[f64]` and `&mut [f64]` cannot alias, so the
borrow checker rejects the call at compile time.

### 2.4 Streaming tier — advancing (`Update`, `Peek`, `UpdateAndFill`)

| Rule | Condition (in order) | RetCode | C | Rust | Java | C# |
|---|---|---|:---:|:---:|:---:|:---:|
| U1 | The handle was not supplied | `TA_BAD_PARAM` | ✅<br>&nbsp; | —<br>&nbsp; | —<br>&nbsp; | —<br>&nbsp; |
| U2 | The output — or, for `UpdateAndFill`, an input series — was not supplied | `TA_BAD_PARAM` | ✅<br>&nbsp; | —<br>&nbsp; | —<br>&nbsp; | —<br>&nbsp; |
| U3 | Any bar is non-finite | `TA_BAD_PARAM` | ✅<br>&nbsp; | ✅<br>&nbsp; | ✅<br>&nbsp; | ✅<br>&nbsp; |
| U4 | (`UpdateAndFill`) the bar count is negative | `TA_BAD_PARAM` | ✅<br>&nbsp; | n/a<br>[14] | n/a<br>[14] | n/a<br>[14] |
| U5 | (`UpdateAndFill`) the input series have different lengths | `TA_BAD_PARAM` | ❌<br>[15] | ✅<br>&nbsp; | ✅<br>&nbsp; | ✅<br>&nbsp; |
| U6 | (`UpdateAndFill`) an output is shorter than the bar count | `TA_BAD_PARAM` | ❌<br>[15] | ✅<br>&nbsp; | ✅<br>&nbsp; | ✅<br>&nbsp; |
| U7 | (`UpdateAndFill`) an output aliases an input, or another output | `TA_BAD_PARAM` | ✅<br>&nbsp; | n/a<br>[16] | ✅<br>&nbsp; | ✅<br>&nbsp; |

A rejected `Update` leaves the handle usable and unadvanced, its `OutRange`
included — that is the whole point of rejecting rather than computing. `Peek`
never writes the handle under any outcome.

`UpdateAndFill` is `n` back-to-back `Update`s, so U3 applies **per bar** and is
the one rule in this document whose rejection leaves output behind: bar `k`
being non-finite commits bars `[0, k)` with their values written, leaves bar `k`
and everything after it uncommitted, and advances the handle's `OutRange` by
exactly `k`. U4–U7 are checked before any bar is committed, so those four leave
the handle where it was, and a zero bar count is a success that does nothing.
Reading the `n` bars as an input array instead — never scanned, `count += n`
unconditionally — was considered and rejected; `docs/streaming-api-design.md`,
"Catching up n bars at once", says why.

**There is no value accessor in C or Rust**, so "read the current value" is a
rule only two backends could carry, and it has no error surface in either: Java's
`value()` and C#'s `Value` are plain field reads of the last committed bar. In C
the value arrives through the out-parameter of `Open`/`Update`/`Peek`, and in
Rust through their return values — a caller who wants it later keeps it.

U3 is checked with an explicit finite test, so it rejects NaN and both
infinities alike. Verified: 174 of 174 `Update` and `Peek` entry points check
their bar, and every `UpdateAndFill` re-emits the same test inside its loop —
re-emitted rather than reached by calling `Update` per bar, because the check
lives in the public entry point and routing through it would buy the check at
the price of a call per bar, which is the cost this entry point exists to
remove.

**Reading the range** has an error surface in C alone, where it is a function
rather than a field: `TA_StreamOutRange` answers `TA_BAD_PARAM` for a NULL
handle **and** for either NULL out-parameter. The other three read a field on an
object that cannot be absent.

**One documented hole.** A composed function drives its sub-streams through their
*public* entry points, so a sub-stream re-checks a value the library itself
produced. If such an intermediate were ever non-finite the sub-stream would
reject it, and the rejection would surface after earlier sub-streams in the
pipeline had already advanced — leaving the handle partway through a bar.
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
| G1 | A setter that names a **single** target rejects the set-all wildcard, and any out-of-domain target | `TA_BAD_PARAM` | ✅<br>&nbsp; | ✅<br>[17] | ✅<br>[17] | ✅<br>&nbsp; |
| G2 | The unstable period is within `[0, MAX_INDEX]` | `TA_BAD_PARAM` | ✅<br>&nbsp; | ✅<br>&nbsp; | ✅<br>&nbsp; | ✅<br>&nbsp; |
| G3 | **Reading** a per-function setting for a target that names no single function | `TA_BAD_PARAM` | ⚠️<br>[18] | ✅<br>&nbsp; | ✅<br>&nbsp; | ✅<br>&nbsp; |
| G4 | The candlestick range type is in domain | `TA_BAD_PARAM` | ✅<br>&nbsp; | —<br>&nbsp; | —<br>&nbsp; | ✅<br>&nbsp; |
| G5 | The candlestick average period is within `[0, MAX_INDEX]` | `TA_BAD_PARAM` | ✅<br>&nbsp; | ✅<br>&nbsp; | ✅<br>&nbsp; | ✅<br>&nbsp; |
| G6 | The candlestick factor is not NaN [21] | `TA_BAD_PARAM` | ✅<br>&nbsp; | ✅<br>&nbsp; | ✅<br>&nbsp; | ✅<br>&nbsp; |
| G7 | The compatibility mode is in domain | `TA_BAD_PARAM` | ✅<br>[19] | —<br>[19] | —<br>[19] | —<br>[19] |
| G8 | A rejected setting leaves the configuration unchanged | — | ✅<br>&nbsp; | ✅<br>[20] | ✅<br>&nbsp; | ✅<br>&nbsp; |

The bounds in G2 and G5 are not arbitrary. Both values are added to a lookback
which is then used as an index: unbounded, the lookback overflows negative and
the function indexes far past the end of its input, while still reporting
success. `MAX_INDEX` is the ceiling the index domain already enforces, and a
warm-up longer than the largest addressable series could never produce output, so
nothing legitimate is refused.

[17] The wildcard is a declared member, so it is rejected by value; a target outside
the declared set is unrepresentable.

[18] C's getter returns the period itself, so it carries no error channel: a target
that names no single function reads as `0`. Accepted, and pinned by
`test_internals.c`; the other three backends reject it because a getter that can
throw costs them nothing.

[19] C rejects an out-of-domain value with `TA_BAD_PARAM` and leaves the mode
unchanged. It previously accepted anything and echoed it back from the getter, so a
caller could not tell a typo from a setting; the two-line domain check was taken even
though `TA_SetCompatibility` is deprecated, because it was that cheap. Rust, Java and
C# expose no public setter at all, so the mode is pinned and the domain cannot be
violated there.

[20] Rust's setters chain and cannot fail individually; each latches the **first**
rejection, which surfaces when the core is built. Verified that a later valid
setter does not clear an earlier rejection.

[21] NaN only: an infinite factor is accepted, in all four backends. A factor
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
| As a **bar** handed to `Update` / `Peek`, or as one of the `n` bars handed to `UpdateAndFill` | **An error**: the bar is non-finite. In `UpdateAndFill` the bars before it stay committed. | U3 |
| As a **real optional parameter** | **An error**: outside the parameter's range. | B3, S5 |
| As a candlestick **`factor`** — a global setting rather than a call parameter | **An error for NaN.** An infinity is accepted, and G6 says why. | G6 |

Only the first row is undefined. The other three are ordinary errors, and
Part 2 is their specification.

### A buffer too short — in C

Rules B5 and S8 are the check C cannot make: it is handed bare pointers and
has no sizes. An input that does not reach `endIdx`, or an output too short for
the count the call produces, is read or written anyway, and the call faults
*inside* the algorithm with the output already partly written. Measured with
guard-paged buffers, where the fault is observable rather than silent corruption
of whatever sits next in the heap.

C alone. Rust asserts the same bound — a panic, never memory corruption, since
the crate forbids `unsafe` — and Java and C# reject the call naming the buffer
and both sizes. The one gap the three share is `OpenAndFill`'s output (S8),
which none of them checks either; there the failure is a raw index exception or
a panic rather than undefined behaviour (Appendix D item 9).

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

**Where the `OutRange` arrives** differs by tier as well as by backend:

| Tier | C | Rust | Java | C# |
|---|---|---|---|---|
| batch | `*outBegIdx`, `*outNBElement` | `Ok(OutRange)` | returns `OutRange` | returns `OutRange` |
| `OpenAndFill` | the same two out-parameters | `Ok((Stream, OutRange))` | on the handle | on the handle |
| any live stream | `TA_StreamOutRange(stream, &beg, &nb)` | `out_range()` | `outRange()` | `OutRange` |

The stream accessor answers the same question in all four: the bars this handle
has produced a value for. An open over `historyLen` bars starts at `(lookback,
historyLen - lookback)`, each accepted `Update` adds one and saturates the count
at `MAX_INDEX` rather than overflowing, and `Peek` — like a rejected bar —
changes nothing. C has one accessor for every function, since every stream struct
leads with the same two ints. `Open`, `Update` and `Peek` still hand back one
value rather than a range. The range's two members are named for each language:
`beg_idx` / `count` in Rust, `begIdx` / `count` in Java, `BegIdx` / `Count` in
C#.

**One condition has no `TA_RetCode` member of its own.** It reports `TA_BAD_PARAM`, and raises it where a code cannot be returned:

| Condition | C | Rust | Java | C# |
|---|---|---|---|---|
| A buffer is too short (B5) | *cannot detect* | panic | `IllegalArgumentException` | `ArgumentException(paramName)` |

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
builders (Part 2.5's G-rules) and the metadata/dynamic-binder tier, which
Appendix C defers. Both still raise plain platform types. Neither is a function
call, so neither has a `TA_RetCode` a caller would be recovering.

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
public entry point, so a rejection the callee detects carries the *callee's*
name: a caller of `MACDEXT` can see `MA: bad parameter`. Accepted rather than
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
   | C batch: `startIdx` guard, then `endIdx` guard, before any other return | 174 / 174 |
   | C batch: parameter validation, then every input, the `OutRange` pointers and every output null-checked, inputs before outputs | 352 / 352 |
   | Rust batch: `startIdx` guard → `endIdx` guard → bounds asserts (following the FMA dispatcher to the real core) | 174 / 174 |
   | Java batch: clamp (which raises B3), then every length check, then the core | 348 / 348 |
   | C# batch: clamp, then every length check, then the core | 348 / 348 |
   | C# cores carrying an overlap guard wherever one is expressible | no core unguarded where the type expresses it |
   | Short-history arm reports the catch-all (C, Rust) / the borrowed code (Java, C#) | 172 streaming functions per backend, no backend mixing the two |

   The 348 counts are 174 functions × the double and float overloads, so the
   float surface is covered by the same evidence. The 352 is the same
   two-per-function sweep over the 176 definitions in `ta_codegen/input/`.

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
  specified here.
- **JSON-RPC servers** — a test harness, not a shipped API. Their error
  behaviour is a property of the harness.
- **`Copy` / `Clone` of a stream handle, and concurrent use** — the concurrency
  contract lives in `docs/streaming-api-design.md`; only its *error* surface
  would belong here, and it has none beyond the rules above.

---

## Appendix D — Open items

Every ❌ above, collected, plus the message-level deviations that sit
alongside a passing rule (item 7), plus two that no rule row can carry: a
divergence on a call every rule says is legal (item 11, now fixed), and coverage
a gate lost rather than a behaviour a backend got wrong (item 12). Each is
measured, not inferred.

| # | Backend | Rule | Defect |
|---|---|---|---|
| ~~1~~ | C | B4 | *Fixed.* The `OutRange` pointers were not null-checked, so passing either as null segfaulted. The batch prologue now checks them, as the streaming `OpenAndFill` prologue always has. |
| ~~2~~ | C | B4 | *Fixed.* Input-buffer presence was checked *before* parameter validation and output presence after, so the prologue straddled B3. Parameter validation now precedes every presence check. |
| ~~3~~ | Java | B4 | *Fixed.* Buffer presence was checked *before* the index and parameter rules, inverting the specified precedence — a negative `startIdx` with a null input reported the null. The wrapper now evaluates B1, B2 and B3 first. |
| ~~4~~ | Java | B3 | *Fixed.* A null enum parameter yielded a raw JVM `NullPointerException` naming neither function nor parameter; it is now a parameter outside its domain, named, and carrying `TA_BAD_PARAM`. |
| ~~5~~ | — | — | *Withdrawn, not fixed.* Partial output↔input overlap in C. Decided in #225: detection stops at buffer identity, and partial overlap is unspecified — rule N8 and Appendix E. Numbering left as-is so existing references to items 8 and 9 keep pointing at the same rows. |
| 6 | Java | S1 | A null history, or a null `OpenAndFill` output, yields a raw JVM exception from inside the algorithm. |
| 7 | C# | S2 | Rule passes; the empty-history *message* omits the cross-language `<NAME> open: ` prefix. |
| ~~8~~ | all | S6 | *Fixed.* `TA_RetCode` had **no member** for "history shorter than the lookback", so C and Rust fell back to the catch-all and Java and C# borrowed `TA_OUT_OF_RANGE_END_INDEX`. `TA_INSUFFICIENT_HISTORY = 17` was appended and all four now report it. The borrowed code took `MAX_INDEX + 1` history (S3) down with it — see footnote [12]. |
| 9 | Rust, Java, C# | S8 | `OpenAndFill` validates no output capacity, unlike the batch tier which does; an undersized output faults inside the fill. (C cannot — no sizes.) Rust used to be a partial exception by accident: its `OpenAndFill` distinctness guard rejected two *empty* outputs before the fill could fault, so that one undersized shape answered `BadParam` where every other answered a panic — and where C# faulted, its `Overlaps` being false for an empty span. The guard now excludes empty operands in both tiers (#262), so Rust faults here like the rest. One fewer answer to a call this item already says nobody validates; the item itself is unchanged. |
| ~~10~~ | C | G7 | *Fixed.* `TA_SetCompatibility` now returns `TA_BAD_PARAM` for a value outside the enum instead of latching it. The function stays deprecated — this was taken only because it was a two-line domain check. |
| ~~11~~ | C#, Rust | B6 | *Fixed.* Two **empty** output buffers were rejected as aliased. C# said so explicitly (`a.IsEmpty && b.IsEmpty` was a clause of the guard); Rust did it incidentally, because the guard compared `as_ptr()` and two zero-capacity allocations answer the same dangling value (a slice of a longer buffer truncated to zero would not, so Rust rejected *some* empty pairs and accepted others — which is worse than either). C and Java accepted them. The call is legal by rule N1 and by B5's own wording — on a range shorter than the lookback *any output length will do, including none* — so this was a four-way divergence on a call the specification says all four accept. Measured on `ACCBANDS(0, 251, …, optInTimePeriod 253, …)` with three distinct zero-length outputs: `TA_SUCCESS` in C and Java, `BadParam` in Rust and C#. Both guards now require **both** operands to be non-empty — two zero-length buffers cannot clobber each other — which is also what makes "declined" spellable in C#, where an empty span is the only way to say it (rule B6a, #262). The empty triple is now a probe in each backend's own suite; no cross-language gate can see it, because the servers bind every output and floor its length at one. |
| 12 | Java, C# | B5 | **MA** is **withheld from the phantom-I/O sweep**, the gate that pins rule N1's "reads nothing" half. That sweep hands `<N>_Impl` — the transcribed numerics, which validate parameters but not array lengths — zero-length arrays and reads what happens; since cross-calls go to the callee's public entry point, the callee's *input* bound answers `TA_BAD_PARAM` before any array is reached, so the probe cannot tell "read nothing" from "never ran". The public API is unaffected — reached through the caller's own wrapper the callee's check is provably redundant, same `endIdx`, same array. **Was ten functions**; nine are resolved. Eight never forwarded on such a range once their control arm learned to read the exception *kind* rather than demand one type, and `stddev` gained the "nothing to produce" early return `apo`, `bbands`, `ppo` and `pvo` already carried. `ma` cannot: it is a dispatch function, and the generator admits only decls, comments, the identity path, one switch and a final return at the top level of a dispatch body — the shape the stream planner is built on — so a guard there is a generator change, not an edit to `ma.c`. The one remaining route is giving the input bound the sub-lookback escape the output bound has, converging with C and Rust and giving up the stricter reading B5 adopted. The list is explicit and its size asserted in both suites, so the debt cannot grow silently. |

Item 9 is the one that still changes what a *correct* caller can do: it turns a
sizing mistake into a partly-written buffer. (Item 8 was the other; a caller can
now tell "send more bars" from "your code is wrong" and retry.)

**Sequencing is tracked in #236**, which reworks the tier these rules are checked
in: item 8 went first (it appended the `TA_RetCode` member #236 normalises to),
items 3 and 4 are folded into its first step, and the streaming items 6 and 9
follow it. Item 7 is independent of it.

---

## Appendix E — Buffer overlap

**What is guaranteed.** Two *outputs* that are the same buffer are rejected
(rule B6). That is a caller error with no correct answer — the second write
destroys the first — and it is cheap to see, because it is an identity test.

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
| Java | **No** — two arrays are the same object or disjoint; there is no offset to differ | n/a, so reference equality is complete |
| Rust | **No** in safe code — `&[T]` and `&mut [T]` over the same data cannot coexist, and two `&mut` cannot either | the identity guard covers the FFI boundary |
| C# | **Yes** — two `Span<T>` slices of one array | **Yes**, incidentally: `Span.Overlaps` exists, so the check is one call |

So the guarantee is set by the weakest member that can express the problem, which is
C — and C is the one language where the check is not merely expensive but not
straightforwardly expressible. Java and Rust satisfy the stronger rule for free by
making the state unreachable, which is not the same as enforcing it.

**Outputs of different element types are not compared.** A `double` output and
an `int` output are never checked against each other, in any backend. Three of
the four cannot express the comparison at all — `double * == int *` is a
constraint violation in C, `double[] == int[]` is "incomparable types" in Java,
`*const f64 == *const i32` is a type error in Rust — and C#'s `Overlaps` is not
defined across element types. So the rule is again set by what the weakest
member can say, and here that is nothing.

Nothing in the corpus mixes them: `ta_variant_frame` and `ta_stream_frame` carry
one `outIsInteger` flag per **function** and assert that no function mixes, so a
mixed-output definition fails the generator before any guard is reached. Skipping
the pair is therefore what the emitters must do rather than a hole a caller can
fall into — but it is what they must do, and until #262 three of them emitted a
term that would not compile.

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
its own probe: `testBatchArgumentContract` (C), `tests/nullable_outputs.rs`
(Rust), and `BatchApiTest` (Java and C#). Each compares a declining call against
the same call with the output supplied, rather than only asserting that it was
accepted — a body that stopped computing the value, or took a different path
without it, would pass the weaker check.

**More than one nullable output per function** is generated but unshipped: the
corpus has one, and the pair guard branches on which of a pair is nullable, so
three of its four arms would be unreachable. `SYNTH10`
(`ta_codegen/generator/input_synth/`) is the fixture that reaches them — three
outputs, two of them declinable, driven end to end through all four backends by
`scripts/synth_gate.py`.
