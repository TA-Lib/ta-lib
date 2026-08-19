# API Error-Handling Specification

What every TA-Lib backend must do when a call is **wrong**, and which backends
have been verified to do it.

Normal behaviour — what a *correct* call returns — is specified by the tests
(`ta_regtest`, the value gates, `--xlang-hash`). It is summarised in Part 1 only
where it is routinely mistaken for an error. Everything else here is failure
behaviour.

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
| ❌ | Measured, and does **not** conform. Needs a fix. Collected in Part 3. |
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

Two vocabularies, and they are deliberately not the same size.

**Signals — `TA_RetCode`.** C's enum is the reference; every backend mirrors a
subset of it, and no backend invents a code of its own. It has 18 members, but
the **function tier — batch and streaming — produces exactly six**:

| Signal | Meaning |
|---|---|
| `TA_SUCCESS` | the call completed; the reported range says how much was written |
| `TA_BAD_PARAM` | the catch-all rejection |
| `TA_OUT_OF_RANGE_START_INDEX` | `startIdx` outside the addressable index domain |
| `TA_OUT_OF_RANGE_END_INDEX` | `endIdx` outside the domain, or below `startIdx` |
| `TA_ALLOC_ERR` | an allocation failed |
| `TA_INTERNAL_ERROR` | an invariant the library owns was violated |

The other twelve belong to the **abstract tier** (`TA_INVALID_HANDLE`,
`TA_INVALID_PARAM_HOLDER`, `TA_FUNC_NOT_FOUND`, …) or to library
initialisation (`TA_LIB_NOT_INITIALIZE`), and four of them —
`TA_NOT_SUPPORTED`, `TA_INVALID_LIST_TYPE`, `TA_INVALID_PARAM_FUNCTION`,
`TA_UNKNOWN_ERR` — are returned from nowhere in the tree at all. Appendix C
defers the abstract tier, so no rule below produces any of the twelve.

**Conditions — the rule labels.** The signal set is *coarser than the conditions
it has to report*: `TA_BAD_PARAM` is the answer at 4823 of the function tier's
return sites, and at least six distinct caller mistakes collapse onto it — an
absent buffer, an out-of-domain parameter, two buffers that overlap, an empty
history, a history shorter than the lookback, a non-finite bar.

A specification written only in signals therefore could not order those six, and
ordering them is the point of this document (see *Order is normative*). So each
rule names the **condition** it detects, and carries the signal as a separate
column. **Where one signal serves many conditions is where Part 3 lives** — a
caller who cannot tell two conditions apart cannot respond to either.

**Two signals the specification needs and `TA_RetCode` does not have.** A rule
that needs one carries **[A]** or **[B]** in its Signal column:

- **[A] — the history is shorter than the lookback.** The library's only
  *recoverable* condition: it means "send more bars", not "your code is wrong".
  There is no member for it, so C and Rust answer the catch-all and a caller
  cannot separate the two. Java and C# borrow `TA_OUT_OF_RANGE_END_INDEX` as an
  in-band marker and re-type it at the wrapper — distinguishable, but on a
  borrowed code that is wrong on its face, since a stream open has no `endIdx`.
  The fix is additive: append a member. Part 3, item 9.
- **[B] — a buffer is too short.** C cannot detect it at all — it is handed bare
  pointers and has no sizes — so there is nothing for a code to report and the
  backends that *can* detect it raise instead. Not a defect in `TA_RetCode`; a
  property of the C ABI, recorded here so the empty cell is not read as an
  oversight.

`MAX_INDEX` is the addressable index ceiling, 100 000 000, identical in all four
backends.

---

## Part 1 — Normal behaviour, and what is simply unspecified

The conditions most often mistaken for failures. None of them is reported as one.
N-5 is the only row that is *unspecified* rather than defined: the library
neither detects it nor promises anything about the result.

| # | Condition | Result |
|---|---|---|
| N-1 | A **valid range shorter than the lookback** | Success, zero values produced, reported count `0`. Never an error. |
| N-2 | Anywhere outside the reported output range | Not written. The library never pads, and never emits a fill value. |
| N-3 | An optional parameter set to its **default sentinel** | The documented default is substituted, then validated like any other value. |
| N-4 | An output buffer that **is** an input buffer (whole-buffer, in place) | Allowed, in the batch tier. Several bodies are written for it. |
| N-5 | A **non-finite value in a batch input** | **Unspecified.** Not detected, not rejected, and nothing is promised about the result. Do not rely on any particular output. The streaming tier does reject it (S-4, U-3). |
| N-6 | A **negative** candlestick `factor` | Legal. It does not "never match" — it makes the comparison unconditionally true. |
| N-7 | The set-all / restore-all **wildcards**, where a setter documents one | Legal on those setters, and rejected on the ones that name a single target (rule G-1). |
| N-8 | **Peeking** a forming bar, any number of times | Never advances the stream and never writes the handle. It can still be *rejected* (U-3), and a rejected peek changes nothing either. |

Part 1 is not a checklist — these behaviours are pinned by the value gates and
the cross-language hash, over the whole corpus, on every run. N-5 is the
exception, and deliberately so: it is unspecified precisely so that nothing has
to hold it.

---

## Part 2 — Error specification

### 2.1 Lookback tier

| # | Condition (in order) | Signal | C | Rust | Java | C# |
|---|---|---|:---:|:---:|:---:|:---:|
| L-1 | An optional parameter is outside its documented domain | the lookback out-of-range sentinel | ✅ | ⚠️ [1] | ✅ | ✅ |
| L-2 | Nothing else in this tier can fail | — | ✅ | ✅ | ✅ | ✅ |

[1] The sentinel is `-1` in C, Java and C#. Rust's lookback returns an unsigned
width in which `-1` is unrepresentable, so it answers the type's maximum;
the generated rustdoc states it on every function. The consequence is real and
worth knowing: a caller who writes `lookback + 1` wraps rather than receiving a
small number. Deliberate — a signed return would put a negative into every
downstream index computation.

### 2.2 Batch tier

| # | Condition (in order) | Signal | C | Rust | Java | C# |
|---|---|---|:---:|:---:|:---:|:---:|
| B-1 | `startIdx` outside `[0, MAX_INDEX]` | `TA_OUT_OF_RANGE_START_INDEX` | ✅ | ✅ [2] | ✅ | ✅ |
| B-2 | `endIdx` outside `[0, MAX_INDEX]`, **or** `endIdx < startIdx` | `TA_OUT_OF_RANGE_END_INDEX` | ✅ | ✅ [2] | ✅ | ✅ |
| B-3 | A required input or output buffer was not supplied | `TA_BAD_PARAM` | ❌ [3] | — [4] | ❌ [5] | — [6] |
| B-4 | An optional parameter is outside its documented domain | `TA_BAD_PARAM` | ✅ | ✅ | ✅ [7] | ✅ |
| B-5 | A buffer is too short for what the call reads or writes | [B] | ⚠️ [8] | ⚠️ [9] | ✅ [10] | ✅ [10] |
| B-5a | …including on a range shorter than the lookback, where the call produces nothing: an input that does not reach `endIdx` is still rejected | [B] | ⚠️ [8] | ⚠️ [9] | ✅ [10] | ✅ [10] |
| B-6 | Two outputs share memory, or an output partially overlaps an input | `TA_BAD_PARAM` | ❌ [11] | ✅ [12] | ✅ [13] | ✅ |

**Domains.** An optional parameter's domain is its documented range; a real
parameter that is NaN is outside every range and is rejected (a plain `x < min ||
x > max` test does not do this — both comparisons are false for NaN — so the
check is spelled inverted). An enum parameter's domain is its declared member
set.

**Capacity (B-5), precisely.** Every input the body indexes must reach `endIdx`.
Every output must hold **the count actually produced** — `endIdx - max(startIdx,
lookback) + 1` — which on a range starting below the lookback is shorter than
the requested range, and is `0` when the range is shorter than the lookback
(rule N-1), at which point any output length will do, including none.

[2] Negative indices are unrepresentable in Rust's unsigned index type; the
ceiling half of each rule is verified.

[3] Two defects, both measured. (i) C checks the **input** buffers here but the
**output** buffers only *after* B-4, so a call with both a bad parameter and an
absent output reports `TA_BAD_PARAM`. (ii) The index and count out-parameters are
not checked at all — passing either as null segfaults. C's own `OpenAndFill`
entry points do check exactly those two pointers, so this is an omission in the
batch prologue rather than a considered position.

[4] Slices cannot be null, and the index/count pair is returned rather than
written through pointers.

[5] The presence check runs **before** B-1, B-2 and B-4, so an absent buffer
pre-empts an out-of-range index or parameter — the opposite precedence from C.
Measured: a negative `startIdx` with a null input reports the null.

[6] A `Span<T>` cannot be absent. A null array converts to an empty span and is
reported by B-5 instead, which names the buffer and both sizes.

[7] An enum parameter has no out-of-domain value, but a *null* one produces a raw
runtime `NullPointerException` naming neither the function nor the parameter.

[8] C is handed bare pointers and has no sizes; it cannot make this check. Not a
defect — a property of the ABI, and the reason B-5 exists in the other backends.
Measured with guard-paged buffers: an input that does not reach `endIdx`, and an
output too short for the produced count, each fault inside the algorithm with the
output already partly written.

[9] Rust asserts exactly this bound, but the failure is a **panic**, not a reported
error. It is memory-safe (the crate forbids `unsafe`, so a violated bound can
never be undefined behaviour) and the assert is load-bearing for code generation
— it is the proof that lets LLVM elide the per-access bounds checks. The assert
is skipped when the range is shorter than the lookback, so rule B-5a applies to
Rust as it does to C.

[10] Rule B-5a is the one bound where **Java and C# check more than C and Rust
do**, and the spec takes the stricter reading as the norm. Measured: a 30-bar
average requested over bars 0..2 with a 2-bar input succeeds with zero values in
C and Rust, and is rejected in Java and C#. C answers `Success` there only
because it has no size to check against, and an empty result reads as "no data
yet" rather than "your `endIdx` is wrong". The rationale is recorded on
`Core.clampedStart` in both backends. The OUTPUT bound genuinely does switch off
on such a range — nothing is produced, so any output length will do, including
none — and all four agree on that.

[11] Only whole-buffer output↔output aliasing is rejected. A **partial** overlap —
which C can express and the guard cannot see — is undetected: the call returns
success with wrong values. Tracked as issue #225.

[12] The borrow checker forbids a safe caller from aliasing two output slices at
all; the pointer-equality guard in the internal tier covers the FFI boundary.

[13] Two Java arrays are identical or disjoint — there is no partial overlap to
express — so reference equality is complete for this rule.

### 2.3 Streaming tier — opening (`Open`, `OpenAndFill`)

| # | Condition (in order) | Signal | C | Rust | Java | C# |
|---|---|---|:---:|:---:|:---:|:---:|
| S-1 | A required argument (handle, history, output) was not supplied | `TA_BAD_PARAM` | ✅ | — | ❌ [14] | — |
| S-2 | The history is empty | `TA_BAD_PARAM` | ✅ | ✅ | ✅ | ✅ [15] |
| S-3 | The history is longer than `MAX_INDEX + 1` | `TA_OUT_OF_RANGE_END_INDEX` | ✅ | [16] | [16] | [16] |
| S-4 | Any history bar is non-finite | `TA_BAD_PARAM` | ✅ | ❌ [17] | ❌ [17] | ✅ |
| S-5 | An optional parameter is outside its documented domain | `TA_BAD_PARAM` | ✅ | ✅ | ✅ | ✅ |
| S-6 | The history holds fewer than `lookback + 1` bars | [A] | ❌ [18] | ❌ [18] | ⚠️ [18] | ⚠️ [18] |
| S-7 | (`OpenAndFill`) an output aliases an input, or another output | `TA_BAD_PARAM` | ✅ | — | ✅ | ✅ |
| S-8 | (`OpenAndFill`) an output is too short for the fill | [B] | ⚠️ [8] | ❌ [19] | ❌ [19] | ❌ [19] |

Rule S-6 is the library's only *recoverable* condition — "feed me more bars", not
"your code is wrong" — so it must be distinguishable from `TA_BAD_PARAM`, which
is always a programming error. No backend fully manages it; see signal [A].

Unlike the batch tier, `OpenAndFill` outputs may **not** alias the inputs at all:
that path writes the outputs and then reads the input tail to seed its rings.

[14] The history's length is read with no guard, so a null history is a raw runtime
`NullPointerException` naming nothing; a null `OpenAndFill` output faults inside
the fill loop.

[15] Reported without the `<NAME> open: ` message prefix that
`docs/streaming-api-design.md` names as a cross-language contract; the class is
right, the message is not.

[16] Not probed — the condition needs a >100 000 001-element allocation.

[17] The non-finite scan is hoisted above S-2 and S-3. Against S-2 this is
unobservable (an empty history has no bar to be non-finite). Against S-3 it
changes the answer: an over-long history containing a non-finite bar reports
`NON_FINITE` where C reports `END_INDEX_RANGE`.

[18] Nobody fully conforms, because the signal does not exist (see [A]). C and
Rust return the **same** code for "history too short" and "parameter out of
range", so a caller cannot separate the recoverable condition from the
programming error — ❌. Java and C# make the two separable, deliberately, by
borrowing `TA_OUT_OF_RANGE_END_INDEX` in band and re-typing it at the wrapper —
⚠️, because the borrowed code is wrong on its face and C# must carry a second
code-to-exception mapper to keep it from surfacing as an `endIdx` complaint on a
method that has no `endIdx`. Verified as uniform, not incidental: across all 172
streaming functions per backend, every C and Rust short-history arm reports the
catch-all and every Java and C# one reports the borrowed code — no backend mixes
the two.

[19] No backend validates the fill output's capacity, in contrast with the batch
tier which does (B-5). An undersized or absent output faults inside the fill loop
with the buffer already partly written — a raw index exception in Java and C#, a
panic in Rust.

### 2.4 Streaming tier — advancing (`Update`, `Peek`)

| # | Condition (in order) | Signal | C | Rust | Java | C# |
|---|---|---|:---:|:---:|:---:|:---:|
| U-1 | The handle was not supplied | `TA_BAD_PARAM` | ✅ | — | — | — |
| U-2 | The output was not supplied | `TA_BAD_PARAM` | ✅ | — | — | — |
| U-3 | Any bar is non-finite | `TA_BAD_PARAM` | ✅ | ✅ | ✅ | ✅ |

A rejected `Update` leaves the handle usable and unadvanced — that is the whole
point of rejecting rather than computing. `Peek` never writes the handle under
any outcome.

**There is no value accessor in C or Rust**, so "read the current value" is a
rule only two backends could carry, and it has no error surface in either: Java's
`value()` and C#'s `Value` are plain field reads of the last committed bar. In C
the value arrives through the out-parameter of `Open`/`Update`/`Peek`, and in
Rust through their return values — a caller who wants it later keeps it.

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

| # | Condition (in order) | Signal | C | Rust | Java | C# |
|---|---|---|:---:|:---:|:---:|:---:|
| X-1 | Releasing an absent handle is a success no-op | — | ✅ | — | — | — |

Only the C tier has an explicit release. The other three reclaim a handle when it
becomes unreachable, so there is no double-release or use-after-release to
specify.

### 2.6 Configuration tier

Global settings in C; a builder producing an immutable core in Rust, Java and C#.

| # | Condition (in order) | Signal | C | Rust | Java | C# |
|---|---|---|:---:|:---:|:---:|:---:|
| G-1 | A setter that names a **single** target rejects the set-all wildcard, and any out-of-domain target | `TA_BAD_PARAM` | ✅ | ✅ [20] | ✅ [20] | ✅ |
| G-2 | The unstable period is within `[0, MAX_INDEX]` | `TA_BAD_PARAM` | ✅ | ✅ | ✅ | ✅ |
| G-3 | **Reading** a per-function setting for a target that names no single function | `TA_BAD_PARAM` | ❌ [21] | ✅ | ✅ | ✅ |
| G-4 | The candlestick range type is in domain | `TA_BAD_PARAM` | ✅ | — | — | ✅ |
| G-5 | The candlestick average period is within `[0, MAX_INDEX]` | `TA_BAD_PARAM` | ✅ | ✅ | ✅ | ✅ |
| G-6 | The candlestick factor is not NaN | `TA_BAD_PARAM` | ✅ | ✅ | ✅ | ✅ |
| G-7 | The compatibility mode is in domain | `TA_BAD_PARAM` | ❌ [22] | — [22] | — [22] | — [22] |
| G-8 | A rejected setting leaves the configuration unchanged | — | ✅ | ✅ [23] | ✅ | ✅ |

The bounds in G-2 and G-5 are not arbitrary. Both values are added to a lookback
which is then used as an index: unbounded, the lookback overflows negative and
the function indexes far past the end of its input, while still reporting
success. `MAX_INDEX` is the ceiling the index domain already enforces, and a
warm-up longer than the largest addressable series could never produce output, so
nothing legitimate is refused.

[20] The wildcard is a declared member, so it is rejected by value; a target outside
the declared set is unrepresentable.

[21] C's getter has no error channel and answers `0` — itself a legal period — so a
bad target is indistinguishable from a genuine reading. Fixing it is an ABI
break; recorded, not scheduled.

[22] C accepts any value and echoes it back from the getter (measured: set an
out-of-domain mode, read the same value back). Rust, Java and C# expose no public
setter at all, so the mode is pinned and the domain cannot be violated.

[23] Rust's setters chain and cannot fail individually; each latches the **first**
rejection, which surfaces when the core is built. Verified that a later valid
setter does not clear an earlier rejection.

---

## Part 3 — Open items

Every ❌ above, collected, plus the message-level deviations that sit
alongside a passing rule (item 7). Each is measured, not inferred.

| # | Backend | Rule | Defect |
|---|---|---|---|
| 1 | C | B-3 | The index and count out-parameters are not null-checked; passing either as null segfaults. The streaming `OpenAndFill` prologue checks them. |
| 2 | C | B-3 | Output-buffer presence is checked *after* parameter validation, so a bad parameter masks an absent output. |
| 3 | Java | B-3 | Buffer presence is checked *before* the index and parameter rules, inverting the specified precedence. |
| 4 | Java | B-4 | A null enum parameter yields a raw JVM `NullPointerException` naming neither function nor parameter. |
| 5 | C | B-6 | Partial output↔input overlap is undetected: success, wrong values. Issue #225. |
| 6 | Java | S-1 | A null history, or a null `OpenAndFill` output, yields a raw JVM exception from inside the algorithm. |
| 7 | C# | S-2 | Rule passes; the empty-history *message* omits the cross-language `<NAME> open: ` prefix. |
| 8 | Rust, Java | S-3/S-4 | The non-finite scan is ordered above the index-ceiling check. |
| 9 | all | S-6 | `TA_RetCode` has **no member** for "history shorter than the lookback". C and Rust fall back to the catch-all, so the library's only recoverable condition is indistinguishable from a programming error; Java and C# borrow `TA_OUT_OF_RANGE_END_INDEX` and re-type it. The fix is additive — append a member — and would let all four converge. |
| 10 | Rust, Java, C# | S-8 | `OpenAndFill` validates no output capacity, unlike the batch tier which does; an undersized output faults inside the fill. (C cannot — no sizes.) |
| 11 | C | G-7 | The compatibility setter accepts any value and the getter echoes it back. |
| 12 | C | G-3 | The unstable-period getter cannot report a bad target. ABI-locked. |

Items 9 and 10 are the two that change what a *correct* caller can do: 9 denies
them the ability to retry, and 10 turns a sizing mistake into a partly-written
buffer.

---

## Appendix A — How each backend spells the six signals

C and Rust report a code; Java and C# raise. Stated here once so no rule has to.

| `TA_RetCode` | Rust | Java | C# |
|---|---|---|---|
| `TA_SUCCESS` | `Ok(OutRange)` | returns `OutRange` | returns `OutRange` |
| `TA_BAD_PARAM` | `Err(RetCode::BadParam)` | `IllegalArgumentException` | `ArgumentException` |
| `TA_OUT_OF_RANGE_START_INDEX` | `Err(RetCode::OutOfRangeStartIndex)` | `IndexOutOfBoundsException` | `ArgumentOutOfRangeException("startIdx")` |
| `TA_OUT_OF_RANGE_END_INDEX` | `Err(RetCode::OutOfRangeEndIndex)` | `IndexOutOfBoundsException` | `ArgumentOutOfRangeException("endIdx")` |
| `TA_ALLOC_ERR` | `Err(RetCode::AllocErr)` | `IllegalStateException` | `InvalidOperationException` |
| `TA_INTERNAL_ERROR` | `Err(RetCode::InternalError)` | `IllegalStateException` | `InvalidOperationException` |

**Two conditions have no signal**, and each raises where a code cannot be
returned:

| Condition | C | Rust | Java | C# |
|---|---|---|---|---|
| A buffer is too short (B-5) | *cannot detect* | panic | `IllegalArgumentException` | `ArgumentException(paramName)` |
| History shorter than the lookback (S-6) | `TA_BAD_PARAM` | `Err(RetCode::BadParam)` | `InsufficientHistoryException` | `InsufficientHistoryException` |

Java and C# reach the second by **borrowing `TA_OUT_OF_RANGE_END_INDEX`** as an
in-band marker and re-typing it at the wrapper. It works, and it is deliberate,
but the borrowed code is wrong on its face — a stream open has no `endIdx` — and
C# has to keep a *second* code-to-exception mapper (`StreamFailure` beside
`Failure`) precisely so the condition does not surface as
`ArgumentOutOfRangeException("endIdx")` on a method with no such parameter. That
is the cost of the missing member, paid twice.

### The backend enums are not the same subset

| Backend | Members | Notes |
|---|---|---|
| C | 18 | 6 function-tier, 12 abstract-tier, of which 4 are returned nowhere |
| Rust | 6 | exactly the function tier |
| Java | 6 | exactly the function tier |
| C# | 8 | the six, plus `InputNotAllInitialize` / `OutputNotAllInitialize` (C's 10 and 11), which its metadata tier produces |

A backend carrying a member it never produces is harmless; a backend *missing*
one it needs is not. Worth re-checking whenever the abstract tier is specified.

### Message prefixes

Batch tier: `TA_<NAME>: ` in C#, `<NAME>: ` in Java. Streaming tier:
`<NAME> <verb>: ` in both, which `docs/streaming-api-design.md` fixes as a
cross-language contract.

**One surface gap, not an error-handling one:** the parameter-default sentinels
are public API in C (`TA_REAL_DEFAULT`, `TA_INTEGER_DEFAULT`), Java
(`Core.REAL_DEFAULT`, `Core.INTEGER_DEFAULT`) and Rust
(`Core::REAL_DEFAULT`, `Core::INTEGER_DEFAULT`), but `internal` in C#. A C#
caller has no supported way to write rule N-3 for a real parameter.

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
