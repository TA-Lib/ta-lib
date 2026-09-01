# Streaming (incremental) API

Every function in the corpus streams, in C, Rust, Java and C#: a handle warmed
on history, then one bar in and that bar's value out, bit-identical to what the
batch tier reports for the same bar. The flag is per function (`stream` in the
YAML `flags:`, surfaced as `TA_FUNC_FLG_STREAM`); everything else — the plan,
the state shape, the tier — is derived from the IR, and
`ta_codegen stream-census` prints what each function derives today.

This file is the contract and the shape. The error model is
`docs/error-handling-spec.md`; the gates are `src/tools/ta_regtest/CLAUDE.md`.

## Lifecycle

1. **`open(history[], params) → (handle, value)`** — validates the parameters
   exactly as the batch guarded entry does, consumes the history in one pass,
   and returns a typed handle plus the value at the last history bar. It
   requires `historyLen >= lookback + 1` (the unstable period in effect
   included); with less there is no defined value yet, and the call fails with
   `TA_INSUFFICIENT_HISTORY` — the library's one recoverable condition, which is
   why it carries its own code. The history may be freed afterwards.
2. **`update(handle, bar) → value`** — once per CLOSED bar. Always produces the
   new value, and **allocates nothing**: the handle is sized at open.
3. **`peek(handle, bar) → value`** — a provisional bar, evaluated without
   committing. Call it as often as the forming bar is revised.
4. **`close(handle)`** — explicit in C, nothing in the managed backends.

Parameters and history are fixed at open; changing a parameter means a new
stream. The handle is opaque and tied to the library version that created it —
it is not serializable and never crosses a process boundary. The sanctioned
checkpoint story is retaining history and re-opening, which is bit-identical by
contract.

**The handle reports its own `OutRange`** — `[begIdx, begIdx + count)`, the bars
it has an output for, in the input series' coordinates: `TA_StreamOutRange` in C
(one accessor for any handle, since every stream struct leads with the same two
ints), `out_range()`, `outRange()`, `OutRange`. Which calls move it is
`docs/error-handling-spec.md` §2.4's business.

Multi-output functions produce one value per output per update: an out-pointer
each in C, a tuple in Rust, a caller-owned sink in Java, a `readonly record
struct` in C#.

## Peek commits nothing, and copies nothing that grows

`peek` returns exactly what `update` would return for that bar, bit-identically,
because it is the same generated transition run as a second frame: a store into
a handle buffer becomes two locals — the slot it targeted and the value it held
— every load that could land on that slot selects them back, and a store no load
can reach this bar is deleted outright. Nothing else about the body changes, so
it is the same numbers in the same order.

What that buys is the cost model. No backend copies a handle's BUFFERS: peek's
overhead is a fixed number of bytes where the buffers are a function of the
period, which is the difference between a peek that is flat in the period and
one that is not. C still copies the struct itself — the fixed-size part, nothing
behind a pointer. Java and C# additionally offer the accumulators to the shadow
rewrite, because a managed array field is a reference and localizing one means
cloning it; what survives is a clone only where the rewrite refuses, which today
is an accumulator the batch body sums inside a loop. The offer is made from all
four backends or from none — bit-identity has no room for a per-backend
difference in what the frame rewrites.

No form writes the handle. That is what keeps Rust's `peek` a `&self` method and
every backend's handles concurrently peekable, including two threads peeking the
same handle. `update` never allocating is the hard constraint; `peek` also
allocates nothing in C and Rust, and in the managed backends only where that
surviving clone is — each generated `peek` doc comment says which of the two it
is rather than claiming the stronger one everywhere.

The property is structural, not observable: a peek that copied and then wrote
the copy would still answer correctly, so no value gate can see the difference.
Each backend therefore carries its own sweep over every streamable function —
`peek_suite` for C, `no_rust_peek_copies_the_handle`,
`no_java_peek_copies_the_handle`, `no_csharp_peek_copies_the_handle` — asserting
that a frame is what runs, that it allocates nothing growing with the period, and
that the accumulators it still copies are the ones the shadow rewrite refused.

Two things Rust needs that C does not, because its renderer keys on how a name is
spelled and a frame's locals drop the `sp.` qualifier: a localized field inherits
the classification the QUALIFIED name carried, and the `while i <= n` → `for i in
..` lowering is off inside a frame, since it rebinds the counter as `usize`.

## `OpenAndFill` — the warm-up output `open` would have thrown away

`open` already computes the whole batch output internally and keeps only the last
value, so a "backfill then go live" bootstrap otherwise pays two passes.
`open_and_fill` keeps the output: everything `open` does, plus the full-history
arrays, bit-identical to `batch(0, historyLen-1)`, in the same single pass.

It is a separate entry point; `open` is byte-for-byte unchanged. The signature is
`open`'s input head followed by `batch`'s output tail — one array per output plus
`outBegIdx`/`outNBElement`. There is no `startIdx`: pinning bar 0 is what makes
the fill bit-exact. Rejection is `open`'s, not `batch`'s, so short history is an
error here rather than a success with empty output.

Outputs may not alias the inputs or each other — not because the fill would
compute the wrong answer, which it does not, but because the margin between its
writes and the ring seeds' reads of the input tail is an accident of each body's
arithmetic that nothing asserts.

The saving is one-time warm-up work, so the case for it is ergonomics, one fewer
full-history allocation, and closing the two-pass footgun — not steady-state
throughput.

## `UpdateAndFill` — n closed bars in one call

Exactly `n` back-to-back `update` calls, in order: same values, same state, same
per-bar rejection. What disappears is the per-call cost around the step — one
call frame and one argument-check set for the whole run instead of one per bar.
There is no out-meta pair; the range rides on the handle.

**A rejected bar commits the bars before it.** The loop stops at the first error,
so bar `k` being non-finite is rejected the way `update` rejects it: bars
`[0, k)` stay committed with their values written, bar `k` and everything after
does not, output slot `k` is untouched, and the handle's range has advanced by
`k + 1` — the committed bars plus the rejected one, which `update` counts too.
The caller reads the range to learn where it stopped and resumes with the rest.
This is the one call in the library that returns a failure AND leaves output
behind, so what it leaves is specified rather than merely allowed.

That advance is gated absolutely — one rejected `Update` counts one bar, a `Peek`
none — by a leg in each backend's stream suite and by `out_range_advance_suite`
over the emitted text of every streamable function in all four. Comparing an
`UpdateAndFill` against a control handle driven one bar at a time cannot gate it:
that compare is symmetric and stays green with the advance deleted from both
arms.

Reading the `n` bars as an input array instead — never scanned, `count += n`
unconditionally — was rejected. The two reasons the warm-up scan was deleted stop
applying here: that scan was an extra pass over caller memory, while this check
is a comparison on a value the loop has already loaded, and a partial fill is
unacceptable in `OpenAndFill` because it leaves no handle and a half-written
array with nothing to describe it, where here it leaves `k` successful updates
and a handle that says so. Under the array reading, `UpdateAndFill` would have
been the one way to poison a handle silently.

Outputs may not alias the inputs or each other. Exact equality happens to be safe
— the step takes bar `i` by value, so output `i` is written after every input `i`
is read — and is rejected anyway, because it is the only case C can see and
admitting it would advertise a guarantee whose immediate neighbourhood (an output
overlapping an input at a non-zero offset) is silent corruption.

## Semantic definition

For every function F, parameters p and series `x[0..t]`: after `open(x[0..k], p)`
for any `k+1 >= lookback + 1`, then `update(x[k+1]) … update(x[t])`, the stream
value at every bar where batch reports an output is **bit-identical** to
`batch_F(0, t, x[0..t])` at that bar — under the same compatibility and candle
settings, which must not change over the stream's lifetime, and the unstable
period in effect at open.

- **The range matches batch too, not just the values.** After a handle has been
  fed `N` bars by any mixture of the openers, `update` and `updateAndFill`, its
  `OutRange` is what the batch call over those same bars reports.
- **The history given to `open` defines bar 0.** For seedings that depend on the
  whole history (EMA under Metastock compatibility), that is the definition, by
  design.
- **State is carried forward, never re-seeded.** Every update continues the
  computation batch would run from bar 0, which is what makes bit-exactness
  possible at all.
- **Unstable period** is honored as in batch, where with full history it only
  delays the first visible output. It is read once at open; changing it later
  affects future opens, never a live stream.
- **Non-finite input: single values are rejected, arrays are not checked.** An
  input array is never scanned in either tier — keeping one free of NaN and ±Inf
  is the caller's responsibility, and passing a non-finite one is undefined
  behaviour. A scan is a whole extra pass over caller memory, and folding it into
  the main loop instead would trade that for a worse contract: a rejection
  partway through a fill, output already half written. A single value is always
  checked, in both tiers, because it is one comparison: every bar in every input
  slot, and every real optional parameter, where the range test would otherwise
  admit NaN (`x < min` and `x > max` are both false for it, so the check is
  spelled inverted, `!(x >= min && x <= max)`).

  The per-bar rejection earns its cost from the retained state. Batch is handed a
  series, computes and forgets, so a NaN reaches only the outputs depending on
  that bar; a handle carries recursive accumulators, so one non-finite bar
  poisons every value it will ever produce afterwards. The finite check runs
  before any STATE is written, so that half of "the handle is unchanged" is
  unconditional and is what the generated docs say; `OutRange` is the one thing
  an `Update` rejection does move.

  Composition is where even the state half fails: a sub-stream re-checks an
  intermediate the library itself computed and rejects it after its siblings have
  advanced. Closing that would mean giving every function an internal, unchecked
  per-bar entry point for composition to call, which buys nothing inside the
  supported input range.
- **A function whose documented output domain includes NaN or ±Inf may not drive
  another function's sub-stream.** Such a callee would make the rejection above
  routine rather than exotic, so `streaming::reject_nonfinite_callees` refuses the
  combination at generation time. Clearing such a failure means either a
  `PRAGMA TA_ALT={STREAM,ALL_LANGUAGES}` body for the callee without the domain
  hole, or teaching the composed emitter to carry a rejection out of the step.
  None of the `nan_inf_output` functions is composed by anything today, so the
  gate is dormant against the shipped corpus; `nan_inf_callee_is_refused` injects
  the flag onto MA so it can be seen firing.

Batch and stream share no emitted code — both come from the same IR through
independent emitters — so a common invisible bug has no shared surface to live
in.

## API shape per language

C: every entry point returns `TA_RetCode`, "Stream" appears only in the handle
type, the handle leads and everything after it is in batch order, every
declaration carries `TA_LIB_API` (the Windows shared build exports nothing
without it), and allocation uses `TA_Malloc`/`TA_Free`.

```c
TA_LIB_API TA_RetCode TA_SMA_Open( TA_SMA_Stream **stream, const double inReal[],
                                   int historyLen, int optInTimePeriod,
                                   double *outReal );
TA_LIB_API TA_RetCode TA_SMA_OpenAndFill( TA_SMA_Stream **stream, const double inReal[],
                                          int historyLen, int optInTimePeriod,
                                          int *outBegIdx, int *outNBElement,
                                          double outReal[] );
TA_LIB_API TA_RetCode TA_SMA_Update( TA_SMA_Stream *stream, double inReal, double *outReal );
TA_LIB_API TA_RetCode TA_SMA_UpdateAndFill( TA_SMA_Stream *stream, const double inReal[],
                                            int barCount, double outReal[] );
TA_LIB_API TA_RetCode TA_SMA_Peek( const TA_SMA_Stream *stream, double inReal, double *outReal );
TA_LIB_API TA_RetCode TA_SMA_Value( const TA_SMA_Stream *stream, double *outReal );
TA_LIB_API TA_RetCode TA_SMA_Clone( const TA_SMA_Stream *stream, TA_SMA_Stream **clone );
TA_LIB_API TA_RetCode TA_SMA_Close( TA_SMA_Stream *stream );

TA_LIB_API TA_RetCode TA_StreamOutRange( const void *stream, int *outBegIdx, int *outNBElement );
```

Multi-input functions take the price scalars in batch order; multi-output ones
take one out-pointer per output in batch order; `CDL*` outputs are
`int *outInteger`. `*stream` is NULL on every `Open` failure.

```rust
let core = Core::builder().build()?;               // immutable settings
let (mut s, _last) = core.sma_open(&history, 14)?; // &self on Core; the handle
                                                   // holds its own Core by value
let v = s.update(x)?;                              // &mut self
s.update_and_fill(&gap_bars, &mut out)?;
let provisional = s.peek(forming)?;                // &self, commits nothing
let r = s.out_range();
```

```java
Core core = new Core();
Core.SmaStream s = core.smaOpen(history, 14);   // throws on reject
double v = s.update(bar);
double p = s.peek(formingBarClose);
s.updateAndFill(gapBars, out);
Core.SmaStream t = s.clone();                   // independent fork
Core.MacdOut mv = new Core.MacdOut();           // allocate once, reuse per bar
m.update(bar, mv);                              // mv.macd / .macdSignal / .macdHist
OutRange r = s.outRange();
```

```csharp
Core.SmaStream s = core.SmaOpen(history, 14);
double v = s.Update(bar);
MacdValue m = macd.Update(bar);                 // readonly record struct
```

Shape rules that are not visible in those lines:

- Java handles are `public static final` classes nested in `Core`
  (`Core.SmaStream`, PascalCase with a single-capitalized acronym), so they ride
  the existing per-function fragment splice into both the shipped `Core.java` and
  the JSON-RPC server with no new build plumbing.
- Java rejections are unchecked and typed: `InsufficientHistoryException` for
  short history (an `IllegalArgumentException` subclass, so it is catchable
  separately), and the `TaLibArgumentException` / `TaLibIndexException` /
  `TaLibStateException` family — each carrying its `RetCode` — for everything
  else. Messages carry the stable prefix `"<NAME> open:"` / `"<NAME> update:"` /
  `"<NAME> peek:"`. `docs/error-handling-spec.md` §2.3–2.5 is the rule-by-rule
  source.
- `value()` re-reads the value(s) at the last bar the stream counted, without
  recomputing.
- A **multi-output Java** handle answers through a caller-owned sink —
  `void update(bars…, <N>Out)` — so a reused sink costs zero bytes per bar
  unconditionally, where a returned object would depend on escape analysis firing
  and measurement shows it does not at the sites that matter. `<N>Out` carries no
  `equals`/`hashCode`: it is a buffer whose contents change under any reference
  kept past the next call, and value equality on that breaks `HashMap`/`HashSet`
  the moment a reused sink becomes a key. **C#** returns a `readonly record
  struct` instead, which is allocation-free by construction — and therefore
  carries .NET's `double` equality, where `NaN` equals `NaN` and `+0.0` equals
  `-0.0`. Compare per component when bit identity is what you mean.
- `clone()` is the universal deep copy — arrays cloned, sub-handles copied
  recursively, the `Core` reference shared — spelled the same in all four
  backends (`TA_<N>_Clone`, `.clone()`, `clone()`, `Clone()`), and it is the only
  path left that copies a handle. Java's spelling needs no `Cloneable` and never
  calls `super.clone()`, which is what the standard objection to Java `clone()`
  actually attaches to; the remedy that objection prescribes is a copy
  constructor, which is what every backend already emits.
- `OpenAndFill` rejects output↔input and output↔output aliasing by reference
  equality in Java (arrays are identical or disjoint, so that is complete) and by
  `ReferenceEquals` in C#, which additionally compiles for cross-typed
  `double[]`/`int[]` output pairs where `==` would not.
- `Integer.MIN_VALUE` keeps its batch meaning — use the documented default — in a
  streaming open, and the gate asserts `open(MIN_VALUE) == open(default)`
  bitwise.
- Handles are deliberately not serializable, and no managed handle implements
  `AutoCloseable` or `IDisposable`: a handle is ordinary heap state, so GC
  suffices. A P/Invoke wrapper over the C handles would own native memory and
  need `SafeHandle`, which is why .NET has a managed emitter instead.

## Concurrency

One rule holds in every language, each enforcing it its own way:

> **A stream's value-affecting settings (compatibility, candle settings) must not
> change over its lifetime.**

- **Rust** enforces it by construction: settings live in the immutable `Core` the
  stream was opened from, so a violation is not expressible. `Core` is
  `Send + Sync`, `open` is `&self`, and the handle holds its own `Core` by value
  (a `&self` method cannot mint a shared `Arc`, and a clone of a small, deeply
  immutable `Core` is observationally identical to a reference while keeping
  handles free of lifetimes). A handle is `Send` but single-writer, because
  `update(&mut self)` makes concurrent updates on one handle a compile error.
- **C** documents it, as an extension of the existing batch-tier caveat: calling
  `TA_SetCompatibility` / `TA_SetCandleSettings` while streams are open is
  undefined, warm-up and ring sizes being derived from the settings in effect at
  open. Where a candle range is BUFFERED — every trailing ring, and the
  rescan-window reads routed into one — its value is the one the range type
  produced when that bar was pushed, up to `back` bars earlier. That sits inside
  the undefined region and is the more self-consistent of the two: a bar now
  enters and leaves a running total as the identical double, so the sum
  telescopes exactly. A single handle is single-writer; concurrent `Peek`s are
  safe, the `const` being load-bearing.
- **Java / C#** keep settings per-`Core`-instance, so there is no process-global
  hazard, and candle settings are additionally snapshotted into CDL handles at
  open, so a mid-stream change cannot produce torn per-bar reads. Single-writer
  per handle; with no concurrent `update`, `peek` / `value()` / `clone()` never
  write the handle and are safe to call concurrently after safe publication. A
  multi-output `<N>Out` is the opposite case: a mutable buffer with no final
  fields, carrying no publication guarantee of its own, so give each thread its
  own sink.

## How it fits ta_codegen

1. **Metadata, not logic.** A function opts in with `stream` in its YAML
   `flags:`, which maps to `TA_FUNC_FLG_STREAM` in ta_abstract like every other
   entry. No flag, no stream code. Everything else is derived from the IR:
   `stream-census` reports what each function derives, and `generate` FAILS if a
   flagged function stops being analyzable or its transition can no longer be
   built. That gate is the answer to the one real maintenance coupling — a batch
   rewrite can change a loop shape and break stream analyzability while leaving
   batch outputs identical, and this catches it at the PR rather than at release.
2. **Analysis** (`generator/src/streaming.rs`) reads the batch IR and never
   changes it. It finds and classifies the steady loop (the parser does not
   pre-isolate one; it appears as `while`, `do/while`, `for` or countdown forms,
   and is absent entirely in composed functions), then derives loop-carried
   scalars → state fields, current-bar reads → update parameters, trailing-window
   reads and bounded `in[i-K]` look-backs → rings, CIRCBUF statements → state
   rings.
3. **The plan is one of five** `StreamPlan` variants, and the emitters key on it
   rather than on any authored tier: `Loop` (the ordinary steady-state
   transition), `DualMode` (a fixed-parameter choice settled at open),
   `Composed` (a producer region plus a strictly-parsed tail pipeline over other
   functions' PUBLIC sub-streams), `Dispatch` (an enum parameter selecting a
   callee's stream — the supported-arm set is derived from the callees' own
   stream flags at generation time) and `PeriodBank` (a bank of sub-streams
   advanced in lockstep, for a per-bar variable period).
4. **The four emitters** (`backends/{c,rust,java,csharp}_stream.rs`) are IR-to-IR
   transforms feeding the existing statement/expression renderers, which is why
   bit-exactness holds by construction and survives batch-code evolution — the
   stream is re-derived from the same IR on every generate.
   - `open` transcribes the ENTIRE batch body at `startIdx = 0` with output
     writes redirected to last-value scalars, then captures the still-live locals
     and ring fills into the state struct. Batch-equal state by construction;
     compatibility-branched seeding and unstable-period skip logic come along
     verbatim.
   - `update` is the steady-loop body with the variable remapping, emitted once
     as the transition; `peek` is the second frame of that same transition.
   - Generation-time invariant checks: no global writes and no compatibility
     reads outside `open` (candle-settings reads in CDL update bodies mirror
     batch's own), no index-variable leakage, and the plan must match the
     analyzed shape.
5. **Composition goes through public stream handles**, never cross-TU internals,
   and its bit-exactness composes by induction: each sub-stream is bit-exact
   against its own batch over the full intermediate series, which is exactly what
   the composed batch computes. `open` opens each sub-stream on its source series
   at the sub-call's own start argument, passed VERBATIM — the callee clamps it
   up to its own lookback, so the composer never computes a callee's lookback —
   immediately before the batch call that consumes it.

Two source-form constraints the analyzer enforces with guiding errors, because
they are what make a combine map streamable: write a combine loop with a single
cursor and a same-bar offset index rather than two cursors, and flatten a
sub-call left inside an `if (rc == TA_SUCCESS) { … }` into a top-level
`if (rc != TA_SUCCESS) return rc;`.

## Cost profile

Per update, by shape rather than by function: O(1) for per-bar maps, scalar
recurrences and rolling sums; O(period) for window recomputers (the LINEARREG
family, BETA, CORREL, AVGDEV, IMI, CCI, ULTOSC, the CDL candle averages); and for
the window extrema, O(1) while the cached extremum sits away from the trailing
edge and O(period) while it sits on it — **not** amortized O(1).

That last one is deliberate. The extrema functions transcribe batch's cached-index
automaton over a ring, so the stream inherits batch's cost profile exactly,
rescans and all. A monotonic deque would be amortized O(1), but batch uses
different tie rules on its two paths (strict `<` on rescan, `<=` on the incoming
side), so the selected INDEX is path-dependent and no single deque discipline
reproduces it — MININDEX at period 2 on `[3,3]` diverges on the first output.
That rules a deque out for every function that outputs an index or computes from
one. It would be legal for the value-output subset, whose ties are bit-identical
input copies, and substituting one there is still open.

One ring-order constraint survives from the same family: some batch code sums its
circular buffer IN BUFFER ORDER, so the FP summation order depends on the ring's
rotation phase. The stream's phase must equal batch's, which `open` gets for free
by capturing the live buffer and its rotation rather than memcpy'ing a trailing
window.

## Verification

Bit-identical comparison cannot ride the ordinary JSON path: inputs cross it at
`%.15g` and the comparator is epsilon-based, so the two sides compute on subtly
different numbers. `stream_verify` therefore runs entirely in-process, in each
language's own server.

- Given `(funcName, params, gen_shape/seed/n, unstablePeriod, compatibility)` the
  server generates the series from the seed (`fuzz_data.h`) and runs both
  `batch(0, n-1)` and, for each warm-up prefix in
  `{lookback+1, lookback+13, n/2, n-1}`, the stream trajectory — the prefixes are
  the stream's analog of `doRangeTest`, exercising the open/update boundary where
  an open-side seeding bug would hide. Comparison is bitwise per bar, and the
  first divergence comes back inline as `%a` hex with its bar and output index;
  the driver never parses a float.
- Unstable period and compatibility are request parameters, pinned for both legs
  and restored afterwards, so neither leg can contaminate the next request.
- `ta_regtest`'s stream pass drives it per function with three parameter vectors
  — defaults, every integer parameter at its true minimum, and min+1 — plus a
  K>0 leg for unstable functions and a Metastock leg. Enum parameters get one
  vector per non-default value, each with its own K and Metastock legs, since the
  selected arm may be unstable or compatibility-seeded where the dispatcher is
  not. An unsupported arm is verified LOUDLY through a generated expect-reject
  precheck, which composes recursively with the caller's argument expressions
  substituted, so a callee's stream landing later narrows every dependent
  precheck on regenerate.
- What each leg family can and cannot see — and why the state-equivalence leg is
  the one that survives a candlestick's 3-valued output — is in
  `src/tools/ta_regtest/CLAUDE.md`.
- The grids run under ASan/UBSan/LSan (LSan has caught real sub-open leaks) and,
  for CDL, under four candle-setting rounds: defaults, bumped, zeroed and
  all-Shadows, the last gating macro arithmetic no default exercises.

## Constraints

- **Double only.** There is no `TA_S_*` stream twin.
- **The batch tier is unchanged.** The stream tier is purely additive.
- **A handle is valid only within the exact library version that created it.**
- **Every `MAType` streams, MAMA included.** When a dispatcher selects it only
  the MAMA line is wanted, and FAMA is a nullable output the dispatcher passes
  NULL for — a supported trailing-NULL delegation, verified bit-exact against
  batch.
- **RSI and CMO under Metastock have a seed boundary**: batch emits a seed output
  and then REWINDS and rebuilds state, so no bit-exact continuation exists from
  the seed exit. `open` requires one bar more than `lookback + 1` in that mode,
  and the verifier knows statically which functions have a seed boundary and
  shifts its boundary leg. STOCHRSI inherits it transitively through its `rsi`
  sub-stream.
