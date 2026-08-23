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
| ⚠️ | Deviates, deliberately. Each one is a decision already recorded in the source or in `CLAUDE.md`; the footnote says where. Held as approved on that basis — flip any to ❌ that should not have been. |
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

No member covers *a buffer is too short* (rules B5, B5a, S8), and their RetCode
column reads *(none)*: C is handed bare pointers and has no sizes, so it cannot
detect the condition at all, and the three backends that can detect it raise
rather than return a code (Appendix A).

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
| N8 | Buffers that **partially** overlap — same memory, different start | **Unspecified.** Only *identical* buffers are detected (rule B6). See "Buffer overlap" below before assuming a diagnosis. |

These behaviours are pinned by the value gates and the cross-language hash, over
the whole corpus, on every test run.

---

---

## Part 2 — Error specification

### 2.1 Lookback tier

| Rule | Condition (in order) | RetCode | C | Rust | Java | C# |
|---|---|---|:---:|:---:|:---:|:---:|
| L1 | An optional parameter is outside its documented domain | the lookback out-of-range sentinel | ✅ | ⚠️ [1] | ✅ | ✅ |
| L1a | …and the sentinel is returned **exactly when** the batch tier would reject the same parameters under B4 | the lookback out-of-range sentinel | ✅ | | | |
| L2 | Nothing else in this tier can fail | — | ✅ | ✅ | ✅ | ✅ |

[1] The sentinel is `-1` in C, Java and C#. Rust's lookback returns an unsigned
width in which `-1` is unrepresentable, so it answers the type's maximum;
the generated rustdoc states it on every function. The consequence is real and
worth knowing: a caller who writes `lookback + 1` wraps rather than receiving a
small number. Deliberate — a signed return would put a negative into every
downstream index computation.

**Why L1a is a rule and not a test detail.** A caller sizes its buffers from the
lookback before it trusts the call, so a lookback that answers a plausible number
for parameters the call then rejects is a lie a wrapper cannot detect — and the
reverse, a sentinel for parameters the call accepts, denies a usable call. The two
tiers derive the domain separately, which is exactly what lets them drift. Asserted
for C on every optional parameter of every function by the boundary sweep
(`ta_test_func/test_period_boundary.c`, group `PERIOD1/BOUNDARY`), which counts the
swept cases and asserts the count; not yet probed in the other three.

### 2.2 Batch tier

| Rule | Condition (in order) | RetCode | C | Rust | Java | C# |
|---|---|---|:---:|:---:|:---:|:---:|
| B1 | `startIdx` outside `[0, MAX_INDEX]` | `TA_OUT_OF_RANGE_START_INDEX` | ✅ | ✅ [2] | ✅ | ✅ |
| B2 | `endIdx` outside `[0, MAX_INDEX]`, **or** `endIdx < startIdx` | `TA_OUT_OF_RANGE_END_INDEX` | ✅ | ✅ [2] | ✅ | ✅ |
| B3 | A required input or output buffer was not supplied | `TA_BAD_PARAM` | ❌ [3] | — [4] | ✅ [5] | — [6] |
| B4 | An optional parameter is outside its documented domain | `TA_BAD_PARAM` | ✅ | ✅ | ✅ [7] | ✅ |
| B5 | A buffer is too short for what the call reads or writes | *(none)* | ⚠️ [8] | ⚠️ [9] | ✅ [10] | ✅ [10] |
| B5a | …including on a range shorter than the lookback, where the call produces nothing: an input that does not reach `endIdx` is still rejected | *(none)* | ⚠️ [8] | ⚠️ [9] | ✅ [10] | ✅ [10] |
| B6 | Two outputs are the **same buffer** | `TA_BAD_PARAM` | ✅ | ✅ [12] | ✅ [13] | ✅ |

**Domains.** An optional parameter's domain is its documented range. Every
declared bound is finite, so a NaN or infinite real parameter is outside every
one of them and is rejected here — provided the test is spelled to see it.
Verified: 96 of 96 real-parameter range checks are spelled `!(x >= min && x <=
max)` rather than `x < min || x > max`, which is what makes them reject NaN —
both plain comparisons are false for NaN — and there is no plain spelling left in
the tree. An enum parameter's domain is its declared member set.

**A wide domain is still a domain.** All 24 real optional parameters emit a check;
what differs is the width. **19 across 11 functions** declare a bound tighter than
the default — `[0, 1]`, `[0.01, 0.99]`, `[0, TA_REAL_MAX]` (`ta_T3.c` is the shape;
the emitter is `backends/c.rs`, mirrored in `backends/java.rs` and
`backends/rust_lang.rs`). The other **5** — `BBANDS`'s two deviation multipliers,
`SAREXT`'s start value, and `STDDEV`/`VAR`'s — declare the full
`[TA_REAL_MIN, TA_REAL_MAX]`. That is **not** a no-op: those bounds are `±3e37`,
finite and far inside a `double`'s `1.8e308`, so the emitted check still rejects
NaN, both infinities, and any magnitude above `3e37`. B4 is wide for those five,
never vacuous.

**Capacity (B5), precisely.** Every input the body indexes must reach `endIdx`.
Every output must hold **the count actually produced** — `endIdx - max(startIdx,
lookback) + 1` — which on a range starting below the lookback is shorter than
the requested range, and is `0` when the range is shorter than the lookback
(rule N1), at which point any output length will do — including none, on the
two backends that accept a zero-length output at all (Appendix D item 11).

*The body indexes* is doing work in that sentence. Four candlestick patterns
declare an OHLC series their algorithm never reads — **CDL3OUTSIDE**,
**CDLENGULFING** and **CDLXSIDEGAP3METHODS** (`inHigh`, `inLow`), and
**CDLHIKKAKE** (`inOpen`): seven legs, eight public overloads per managed
backend. Those legs are exempt from B5 **and from B3**, so a short or absent
one is accepted. Measured: `CDLHIKKAKE(0, 99, null, high, low, close, out)`
returns 95 values in Java, while the control — a short `inClose`, which the body
does read — is rejected naming the buffer and both sizes.

Deliberate, and shared: the exemption is computed once
(`backends::common::indexed_input_names`) and consumed by the Rust assert
preamble and the Java and C# checks alike, so all three agree on which legs are
load-bearing. Checking them would refuse a call the algorithm can answer, and
refuse it in the managed backends only. The cost is that "required" in B3 means
*required by this function's body*, not *declared in its signature*.

[2] Negative indices are unrepresentable in Rust's unsigned index type; the
ceiling half of each rule is verified.

[3] Two defects, both measured. (i) C checks the **input** buffers here but the
**output** buffers only *after* B4, so a call with both a bad parameter and an
absent output reports `TA_BAD_PARAM`. (ii) The index and count out-parameters are
not checked at all — passing either as null segfaults. C's own `OpenAndFill`
entry points check exactly those two pointers, and so does `TA_StreamOutRange`,
so this is an omission in the batch prologue rather than a considered position.

[4] Slices cannot be null, and the index/count pair is returned rather than
written through pointers.

[5] Java raises rather than returning a code, so it reports this as a
`NullPointerException` naming the function and the argument — the code is on the
thrown object (Appendix A). The presence check used to run **before** B1 and
B2, so an absent buffer pre-empted an out-of-range index; the wrapper now
evaluates the index rules first (Appendix D item 3). It does not cover the
never-indexed legs described under **Capacity (B5)** above, which are not
checked at all — an argument the algorithm never reads is not required.

[6] A `Span<T>` cannot be absent. A null array converts to an empty span and is
reported by B5 instead, which names the buffer and both sizes.

[7] An enum parameter has no out-of-domain value. A *null* one is rejected as an
absent argument (B3), naming the function and the parameter; it used to reach
the function's own lookback and surface as a raw `NullPointerException` naming
neither (Appendix D item 4). Java is the only backend where this arises — C# enums
are value types, C's are integers, Rust's cannot be absent.

[8] C is handed bare pointers and has no sizes; it cannot make this check. Not a
defect — a property of the ABI, and the reason B5 exists in the other backends.
What happens instead is Part 3.

[9] Rust asserts exactly this bound, but the failure is a **panic**, not a reported
error. It is memory-safe (the crate forbids `unsafe`, so a violated bound can
never be undefined behaviour) and the assert is load-bearing for code generation
— it is the proof that lets LLVM elide the per-access bounds checks. The assert
is skipped when the range is shorter than the lookback, so rule B5a applies to
Rust as it does to C.

[10] Rule B5a is the one bound where **Java and C# check more than C and Rust
do**, and the spec takes the stricter reading as the norm. Measured: a 30-bar
average requested over bars 0..2 with a 2-bar input succeeds with zero values in
C and Rust, and is rejected in Java and C#. C answers `Success` there only
because it has no size to check against, and an empty result reads as "no data
yet" rather than "your `endIdx` is wrong". The rationale is recorded on
`Core.clampedStart` in both backends. The OUTPUT bound genuinely does switch off
on such a range — nothing is produced, so any output length will do — and all
four agree on that. *Except at length zero*, where C and Java accept and Rust
and C# reject a second empty output as aliased: Appendix D item 11.

[11] *(retired — B6 is identity-only by decision; partial overlap is rule N8 and
the "Buffer overlap" section below.)*

[12] The borrow checker forbids a safe caller from aliasing two output slices at
all; the pointer-equality guard in the internal tier covers the FFI boundary.

[13] Two Java arrays are identical or disjoint — there is no partial overlap to
express — so reference equality is complete for this rule.

### Buffer overlap

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

**C# currently detects more than this specifies.** Its generated guard is
`if (outReal.Overlaps(inReal) && outReal != inReal)`, which rejects a partial
input↔output overlap while still allowing whole-buffer in place. That is a superset
of the guarantee, kept because it costs one call on a type that already answers the
question. **Callers must not rely on it**: the same call is unspecified in C, and
inexpressible in Java and Rust. If uniformity is ever preferred over the extra
safety, removing it is the change — not adding the check elsewhere.

### 2.3 Streaming tier — opening (`Open`, `OpenAndFill`)

| Rule | Condition (in order) | RetCode | C | Rust | Java | C# |
|---|---|---|:---:|:---:|:---:|:---:|
| S1 | A required argument (handle, history, output) was not supplied | `TA_BAD_PARAM` | ✅ | — | ❌ [14] | — |
| S2 | The history is empty | `TA_BAD_PARAM` | ✅ | ✅ | ✅ | ✅ [15] |
| S3 | The history is longer than `MAX_INDEX + 1` | `TA_OUT_OF_RANGE_END_INDEX` | ✅ | [16] | [16] | [16] |
| S4 | *(withdrawn — the warm-up history is an input array; see "Non-finite input")* [17] | — | — | — | — | — |
| S5 | An optional parameter is outside its documented domain | `TA_BAD_PARAM` | ✅ | ✅ | ✅ | ✅ |
| S6 | The history holds fewer than `lookback + 1` bars | `TA_INSUFFICIENT_HISTORY` | ✅ [18] | ✅ [18] | ✅ [18] | ✅ [18] |
| S7 | (`OpenAndFill`) an output aliases an input, or another output | `TA_BAD_PARAM` | ✅ | — | ✅ | ✅ |
| S8 | (`OpenAndFill`) an output is too short for the fill | *(none)* | ⚠️ [8] | ❌ [19] | ❌ [19] | ❌ [19] |

Rule S6 is the library's only *recoverable* condition — "feed me more bars", not
"your code is wrong" — so it must be distinguishable from `TA_BAD_PARAM`, which
is always a programming error. It has carried its own code since
`TA_INSUFFICIENT_HISTORY` was appended.

Unlike the batch tier, `OpenAndFill` outputs may **not** alias the inputs at all:
that path writes the outputs and then reads the input tail to seed its rings.

[14] The history's length is read with no guard, so a null history is a raw runtime
`NullPointerException` naming nothing; a null `OpenAndFill` output faults inside
the fill loop.

[15] Reported without the `<NAME> open: ` message prefix that
`docs/streaming-api-design.md` names as a cross-language contract; the class is
right, the message is not.

[16] Not probed — the condition needs a >100 000 001-element allocation.

[17] **Withdrawn.** The warm-up history is an input *array*, and the library
does not scan those. Until this was removed it was the only array in the library
that was checked — 174 of 174 `Open` and 174 of 174 `OpenAndFill` entry points,
in all four backends — which made "arrays are never scanned" a rule with one
exception rather than a rule. The scan cost ≈0.3 ns per bar per array: a corpus
median of 22% of an `Open`, up to 76% of a candlestick `OpenAndFill`. Folding it
into the main loop instead would have traded that cost for a rejection partway
through a fill, with the output already half written. The measurement is in
`docs/streaming-api-design.md`. U3 is untouched: a bar handed to `Update` or
`Peek` is a single value.

[18] All four converge on `TA_INSUFFICIENT_HISTORY`. Before it existed C and
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

[19] No backend validates the fill output's capacity, in contrast with the batch
tier which does (B5). An undersized or absent output faults inside the fill loop
with the buffer already partly written — a raw index exception in Java and C#, a
panic in Rust.

[20] Only C has a bar count: the other three take slices or arrays, which carry
their own lengths, so "negative" is unrepresentable.

[21] C is handed bare pointers and has no sizes — the same blind spot as B5 and
S8. The other three answer both before committing anything, which makes
`UpdateAndFill` the one filling entry point whose output capacity IS validated
outside C; `OpenAndFill`'s (S8) still is not, and closing that is Appendix D
item 9.

[22] Rust cannot express it: `&[f64]` and `&mut [f64]` cannot alias, so the
borrow checker rejects the call at compile time.

### 2.4 Streaming tier — advancing (`Update`, `Peek`, `UpdateAndFill`)

| Rule | Condition (in order) | RetCode | C | Rust | Java | C# |
|---|---|---|:---:|:---:|:---:|:---:|
| U1 | The handle was not supplied | `TA_BAD_PARAM` | ✅ | — | — | — |
| U2 | The output — or, for `UpdateAndFill`, an input series — was not supplied | `TA_BAD_PARAM` | ✅ | — | — | — |
| U3 | Any bar is non-finite | `TA_BAD_PARAM` | ✅ | ✅ | ✅ | ✅ |
| U4 | (`UpdateAndFill`) the bar count is negative | `TA_BAD_PARAM` | ✅ | n/a [20] | n/a [20] | n/a [20] |
| U5 | (`UpdateAndFill`) the input series have different lengths | `TA_BAD_PARAM` | ❌ [21] | ✅ | ✅ | ✅ |
| U6 | (`UpdateAndFill`) an output is shorter than the bar count | `TA_BAD_PARAM` | ❌ [21] | ✅ | ✅ | ✅ |
| U7 | (`UpdateAndFill`) an output aliases an input, or another output | `TA_BAD_PARAM` | ✅ | n/a [22] | ✅ | ✅ |

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
| G1 | A setter that names a **single** target rejects the set-all wildcard, and any out-of-domain target | `TA_BAD_PARAM` | ✅ | ✅ [23] | ✅ [23] | ✅ |
| G2 | The unstable period is within `[0, MAX_INDEX]` | `TA_BAD_PARAM` | ✅ | ✅ | ✅ | ✅ |
| G3 | **Reading** a per-function setting for a target that names no single function | `TA_BAD_PARAM` | ⚠️ [24] | ✅ | ✅ | ✅ |
| G4 | The candlestick range type is in domain | `TA_BAD_PARAM` | ✅ | — | — | ✅ |
| G5 | The candlestick average period is within `[0, MAX_INDEX]` | `TA_BAD_PARAM` | ✅ | ✅ | ✅ | ✅ |
| G6 | The candlestick factor is not NaN [27] | `TA_BAD_PARAM` | ✅ | ✅ | ✅ | ✅ |
| G7 | The compatibility mode is in domain | `TA_BAD_PARAM` | ✅ [25] | — [25] | — [25] | — [25] |
| G8 | A rejected setting leaves the configuration unchanged | — | ✅ | ✅ [26] | ✅ | ✅ |

The bounds in G2 and G5 are not arbitrary. Both values are added to a lookback
which is then used as an index: unbounded, the lookback overflows negative and
the function indexes far past the end of its input, while still reporting
success. `MAX_INDEX` is the ceiling the index domain already enforces, and a
warm-up longer than the largest addressable series could never produce output, so
nothing legitimate is refused.

[23] The wildcard is a declared member, so it is rejected by value; a target outside
the declared set is unrepresentable.

[24] C's getter returns the period itself, so it carries no error channel: a target
that names no single function reads as `0`. Accepted, and pinned by
`test_internals.c`; the other three backends reject it because a getter that can
throw costs them nothing.

[25] C rejects an out-of-domain value with `TA_BAD_PARAM` and leaves the mode
unchanged. It previously accepted anything and echoed it back from the getter, so a
caller could not tell a typo from a setting; the two-line domain check was taken even
though `TA_SetCompatibility` is deprecated, because it was that cheap. Rust, Java and
C# expose no public setter at all, so the mode is pinned and the domain cannot be
violated there.

[26] Rust's setters chain and cannot fail individually; each latches the **first**
rejection, which surfaces when the core is built. Verified that a later valid
setter does not clear an earlier rejection.

[27] NaN only: an infinite factor is accepted, in all four backends. A factor
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
| As a **real optional parameter** | **An error**: the value is outside the parameter's domain, since every declared bound is finite. | B4, S5 |
| As a candlestick **`factor`** — a global setting rather than a call parameter | **An error for NaN.** An infinity is accepted, and G6 says why. | G6 |

Only the first row is undefined. The other three are ordinary errors, and
Part 2 is their specification.

### A buffer too short — in C

Rules B5, B5a and S8 are the check C cannot make: it is handed bare pointers and
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
is rule N4. "Buffer overlap", in Part 2, has the reasoning and the per-backend
table.

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

**One condition has no RetCode**, and raises where a code cannot be returned:

| Condition | C | Rust | Java | C# |
|---|---|---|---|---|
| A buffer is too short (B5) | *cannot detect* | panic | `IllegalArgumentException` | `ArgumentException(paramName)` |

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
   forked child so a crash is an observation rather than the end of the run.

2. **A structural check over the whole generated corpus** — a probe on one
   function says nothing about the other 173. Verified mechanically, from the
   generated sources:

   | Claim | Result |
   |---|---|
   | C batch: `startIdx` guard, then `endIdx` guard, before any other return | 174 / 174 |
   | C batch: every input null-checked, all inputs before any output | 174 / 174 |
   | Rust batch: `startIdx` guard → `endIdx` guard → bounds asserts (following the FMA dispatcher to the real core) | 174 / 174 |
   | Java batch: clamp, then every length check, then the core | 348 / 348 |
   | C# batch: clamp, then every length check, then the core | 348 / 348 |
   | C# cores carrying an overlap guard wherever one is expressible | 110 guarded, 0 unguarded, 64 not expressible |
   | Short-history arm reports the catch-all (C, Rust) / the borrowed code (Java, C#) | 172 streaming functions per backend, no backend mixing the two |

   The 348 counts are 174 functions × the double and float overloads, so the
   float surface is covered by the same evidence.

Probes are not committed. They are throwaway drivers, not gates: the shipped
gates cover values, and these cover the failure paths once, to produce this
table. Re-running them means rewriting them — cheap, and honest about the fact
that a mark is a statement about a moment.

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
divergence on a call every rule says is legal (item 11), and coverage a gate lost
rather than a behaviour a backend got wrong (item 12). Each is measured, not
inferred.

| # | Backend | Rule | Defect |
|---|---|---|---|
| 1 | C | B3 | The index and count out-parameters are not null-checked; passing either as null segfaults. The streaming `OpenAndFill` prologue checks them. |
| 2 | C | B3 | Output-buffer presence is checked *after* parameter validation, so a bad parameter masks an absent output. |
| ~~3~~ | Java | B3 | *Fixed.* Buffer presence was checked *before* the index rules, inverting the specified precedence — a negative `startIdx` with a null input reported the null. The wrapper now evaluates B1 and B2 first. |
| ~~4~~ | Java | B4 | *Fixed.* A null enum parameter yielded a raw JVM `NullPointerException` naming neither function nor parameter; it is now an absent argument (B3), named, and carrying `TA_BAD_PARAM`. |
| ~~5~~ | — | — | *Withdrawn, not fixed.* Partial output↔input overlap in C. Decided in #225: detection stops at buffer identity, and partial overlap is unspecified — rule N8 and the "Buffer overlap" section. Numbering left as-is so existing references to items 8 and 9 keep pointing at the same rows. |
| 6 | Java | S1 | A null history, or a null `OpenAndFill` output, yields a raw JVM exception from inside the algorithm. |
| 7 | C# | S2 | Rule passes; the empty-history *message* omits the cross-language `<NAME> open: ` prefix. |
| ~~8~~ | all | S6 | *Fixed.* `TA_RetCode` had **no member** for "history shorter than the lookback", so C and Rust fell back to the catch-all and Java and C# borrowed `TA_OUT_OF_RANGE_END_INDEX`. `TA_INSUFFICIENT_HISTORY = 17` was appended and all four now report it. The borrowed code took `MAX_INDEX + 1` history (S3) down with it — see footnote [18]. |
| 9 | Rust, Java, C# | S8 | `OpenAndFill` validates no output capacity, unlike the batch tier which does; an undersized output faults inside the fill. (C cannot — no sizes.) |
| ~~10~~ | C | G7 | *Fixed.* `TA_SetCompatibility` now returns `TA_BAD_PARAM` for a value outside the enum instead of latching it. The function stays deprecated — this was taken only because it was a two-line domain check. |
| 11 | C#, Rust | B6 | Two **empty** output buffers are rejected as aliased. C# says so explicitly (`a.IsEmpty && b.IsEmpty` is a clause of the guard); Rust does it incidentally, because the guard compares `as_ptr()` and two zero-capacity allocations answer the same dangling value (a slice of a longer buffer truncated to zero would not, so Rust rejects *some* empty pairs and accepts others — which is worse than either). C and Java accept them. The call is legal by rule N1 and by B5's own wording — on a range shorter than the lookback *any output length will do, including none* — so this is a four-way divergence on a call the specification says all four accept. Measured on `ACCBANDS(0, 251, …, optInTimePeriod 253, …)` with three distinct zero-length outputs: `TA_SUCCESS` in C and Java, `BadParam` in Rust and C#. Unreachable until #236 step 2 sized the harness's output buffers to the produced count, which is why the servers floor that length at one. |
| 12 | Java, C# | B5a | **MA** is **withheld from the phantom-I/O sweep**, the gate that pins rule N1's "reads nothing" half. That sweep hands `<N>_Impl` — the transcribed numerics, which validate parameters but not array lengths — zero-length arrays and reads what happens; since cross-calls go to the callee's public entry point, the callee's *input* bound answers `TA_BAD_PARAM` before any array is reached, so the probe cannot tell "read nothing" from "never ran". The public API is unaffected — reached through the caller's own wrapper the callee's check is provably redundant, same `endIdx`, same array. **Was ten functions**; nine are resolved. Eight never forwarded on such a range once their control arm learned to read the exception *kind* rather than demand one type, and `stddev` gained the "nothing to produce" early return `apo`, `bbands`, `ppo` and `pvo` already carried. `ma` cannot: it is a dispatch function, and the generator admits only decls, comments, the identity path, one switch and a final return at the top level of a dispatch body — the shape the stream planner is built on — so a guard there is a generator change, not an edit to `ma.c`. The one remaining route is giving the input bound the sub-lookback escape the output bound has, converging with C and Rust and giving up the stricter reading footnote [10] adopted. The list is explicit and its size asserted in both suites, so the debt cannot grow silently. |

Item 9 is the one that still changes what a *correct* caller can do: it turns a
sizing mistake into a partly-written buffer. (Item 8 was the other; a caller can
now tell "send more bars" from "your code is wrong" and retry.)

**Sequencing is tracked in #236**, which reworks the tier these rules are checked
in: item 8 went first (it appended the `TA_RetCode` member #236 normalises to),
items 3 and 4 are folded into its first step, and the streaming items 6 and 9
follow it. Items 1, 2 and 7 are independent of it.
