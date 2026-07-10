# Proposal: Generated Streaming (Incremental) API for ta_codegen

Status: design finalized 2026-07-09 after a design review and a ta_codegen
feasibility spike (spike results in the *Implementation* section; the spike
prototype was retired once the production implementation superseded it).
**Stage 1 is implemented** — T1+T2 C tier (37 functions), `stream_verify`
harness, regtest gate; see *Staging* for what remains proposal-only.

## Design

### Motivation
Live-trading users often want the latest indicator value updated in O(1) per new bar — not a full recompute each tick. This has been requested over the years, and some TA-Lib derivative works have already attempted it (see prior art).

This proposal is to add an official **streaming API** to TA-Lib for C, Rust and Java (other language may be later).

The hard parts are **maintenance** and **validation**, and both are solvable with recent automation added to TA-Lib 0.7.1.

### Prior art: `talib.stream`

The Python wrapper's streaming module returns the latest value by calling the C
function at a single index (`startIdx = endIdx = len-1`), reaching back only
`lookback` bars instead of building the whole array — cheaper than the Function
API. A good experimental first cut; two limitations this proposal removes:

- **No retained state** — every tick redoes the `lookback` reach-back, so it is
  O(lookback)/tick, not O(1); nothing is carried between bars.
- **Diverges from the batch for recursive indicators** — it re-seeds from the
  trailing window, not full history, so for unstable/recursive functions
  (RSI, EMA, ADX, …) its output does not match the full-history batch.

### Prior art: `ta-lib-rt`

[ta-lib-rt](https://github.com/trufanov-nok/ta-lib-rt) is a **fork of the C
TA-Lib** that adds exactly this kind of incremental API.

So the *concept* — per-indicator state + per-bar update — is not novel; ta-lib-rt got there first.

ta-lib-rt shows it can be done. It is a great reference and inspiration to this proposal.

This proposal adds **maintenance automation** to keep an official streaming API updated lockstep with the official TA-Lib batch API.

### Public API (stream lifecycle)

Every stream, in every language, is the same lifecycle:

1. **`open(params, history[]) → (handle, value)` — once.** Validates the params
   (defaults substituted and ranges checked exactly as the batch guarded entry
   does), consumes the provided history in one pass, and returns an opaque,
   typed stream **handle** plus the current value (the output for the last
   history bar). `open` **requires `historyLen >= min_history(params)`** —
   the function's lookback + 1, including any unstable period in effect
   (i.e. exactly `TA_XXX_Lookback() + 1`); with less history there
   is no defined value yet and open fails. This keeps rule 2 unconditionally
   true. Everything needed is kept in the handle (the initial history can be
   "freed").
2. **`update(handle, bar) → value` — once per closed bar.** Takes the handle
   and the new input(s) (OHLCV for multi-input functions) and always produces
   the new current value. `update` performs **no allocation** — the handle is
   sized at open (rings/deques are bounded by the fixed params).
3. **`close(handle)`.** Releases the stream — explicit in C, implicit
   (automatic) in managed backends (Rust/Java).

Parameters and history are fixed at `open`; changing a parameter means a new stream.

The handle is opaque and tied to the library version that created it — don't
persist it across versions.

Per-language signatures are in the *API shape per backend* section below.

Multi-output functions (MACD, BBANDS, STOCH) produce one value per output per
update (a tuple/struct in managed languages, one out-pointer per output in C).
All outputs of a function share the same warm-up; no function has divergent
per-output lookbacks at the API level (verified across the 161).

### Forming bar (peek)

Live feeds revise the *forming* bar many times before it closes; `update` is
for **closed bars only** (it commits state irreversibly). For the forming bar
the surface includes:

- **`peek(handle, bar) → value`** — evaluate a provisional bar without
  committing state. Calling `peek` any number of times between `update`s is
  the intended intra-bar pattern.

`peek` returns exactly the value `update` would return for that bar —
bit-identical, guaranteed by construction: it is the same generated code as
`update`, run without committing (see *How it fits ta_codegen*). Its overhead
is a small state copy.

### Semantic definition (the contract tests enforce)

For every function F, parameters p, and series `x[0..t]`: after
`open(p, x[0..k])` (any `k+1 >= min_history`) and
`update(x[k+1]) … update(x[t])`, the stream value at every bar where batch
reports an output is **bit-identical** to

```
batch_F(startIdx=0, endIdx=t, x[0..t])   at that bar,
```

computed under the same compatibility and candle settings — which must not
change over the stream's lifetime (see *Concurrency across tiers*) — and the
unstable period in effect at `open`.

Notes that make this precise:

- **The history given to `open` defines bar 0.** The stream matches batch
  over exactly the series it has seen; for some seedings (e.g. EMA under
  Metastock compatibility) values depend on the whole history — by design.
- **State is carried forward, never re-seeded.** Every update continues the
  same computation batch would run from bar 0 — that is what makes
  bit-exactness possible.
- **Unstable period.** Honored exactly as in batch, where — with full
  history — it only delays the first visible output: `open` requires
  `TA_XXX_Lookback() + 1` bars; values are unaffected. It is read once at
  `open`; changing it later affects only future opens, never a live stream.
- **NaN.** As in batch: inputs must be finite, and outputs never contain NaN.
  Wrappers (e.g. Python) may layer their own NaN handling.

ta_regtest automation validates the batch API exhaustively including comparison
against frozen references (e.g. 0.6.4). The stream tier is verified against
batch (see *Verification*); what transfers is full-range value correctness.
Batch-only properties (startIdx>0 re-seeding coherency, unstable-period
convergence envelopes) have no stream analog by design.

We purposely avoid code re-use between the generated batch and stream API,
reducing risk of introducing common/invisible bugs. (Both are generated from
the same IR, but through independent emitters — see *How it fits ta_codegen*.)

### API shape per backend (signatures)

C — every entry point returns `TA_RetCode` like the rest of the library;
"Stream" appears only in the handle type, the functions use the lifecycle
verbs `Open`/`Update`/`Peek`/`Close`; every declaration carries `TA_LIB_API`
(the Windows shared build exports nothing without it); allocation uses
`TA_Malloc`/`TA_Free`:

```c
TA_SMA_Stream *s = NULL;
double out;

/* open: validates params, consumes warm-up history in one pass,
 * allocates the handle, writes the value at the last history bar. */
TA_LIB_API TA_RetCode TA_SMA_Open( int            optInTimePeriod,
                                   const double   history[],
                                   int            historyLen,
                                   TA_SMA_Stream **stream,     /* out */
                                   double         *outReal );  /* out */

/* update: always produces a value; cannot fail except on NULL args. */
TA_LIB_API TA_RetCode TA_SMA_Update( TA_SMA_Stream *stream,
                                     double         inReal,
                                     double        *outReal );

/* peek: the same generated transition as update, run on a scratch copy
 * of the state — never commits (the handle is logically const). */
TA_LIB_API TA_RetCode TA_SMA_Peek( const TA_SMA_Stream *stream,
                                   double               inReal,
                                   double              *outReal );

TA_LIB_API TA_RetCode TA_SMA_Close( TA_SMA_Stream *stream );
```

**Error model.** `Open` returns `TA_BAD_PARAM` (param out of range, or
`historyLen < min_history` so no value exists yet) or `TA_ALLOC_ERR`;
`*stream` is NULL on any failure. `Update`/`Peek` return `TA_BAD_PARAM` only
on NULL arguments. `Close(NULL)` is a no-op returning `TA_SUCCESS`.

**Shapes.** Multi-input functions take the price scalars in batch order
(`TA_CDLDOJI_Update(s, open, high, low, close, &outInteger)`).
Multi-output functions take one out-pointer per output in batch order
(`TA_MACD_Update(s, x, &outMACD, &outMACDSignal, &outMACDHist)`).
Integer-output functions (CDL\*) use `int *outInteger`.

Rust:

```rust
let mut s = SmaStream::open(14, &history)?;      // warm from history
for &x in new_bars { let v = s.update(x); /* always a value */ }
let provisional = s.peek(forming_bar_close);      // &self is NOT enough: peek
                                                  // uses the handle's scratch
                                                  // mirror → &mut self, or an
                                                  // interior scratch; decided
                                                  // at implementation time
```

Java/.NET: small handle objects with the same `open(params, history)` +
`update`/`peek` shape. Multi-output return small structs (`(f64, f64, f64)`
for MACD in Rust; a tiny value class in Java/.NET).

**Java/.NET handle lifecycle.** Generated Java is pure Java — a stream handle
is ordinary heap state, so "close" is literally nothing: no `AutoCloseable`,
no finalizer; GC suffices. The same holds for .NET **provided the .NET stream
tier is a managed C# emitter** mirroring the Java one (the plan — staging
step 6). A P/Invoke wrapper over the C handles would instead own native memory
and need `SafeHandle`/`IDisposable` — a worse API; this proposal chooses the
managed emitter and accepts the later delivery date.

### Rust concurrency

The Rust binding is shaped so concurrent batch and per-thread streams are
enforced by the type system, not by convention. This justifies the API shape
above:

- **`Core` is immutable after construction** (built via `Core::builder()`, no
  setters — issue #104, closed). Its
  fields are plain data, so `Core: Send + Sync`: share one `Arc<Core>`
  read-only across threads and call any batch function concurrently — safe by
  construction, since a call only *reads* the globals (compatibility, candle
  settings, unstable period). Changing a setting means building a new `Core`
  (it is small; cloning is cheap).
- **`open` is a `&self` method on `Core`.** Compatibility is consumed during
  seeding; for anything read per-bar (candle settings) the handle keeps a
  reference (`Arc`) to the immutable `Core` it was opened from — nothing is
  copied, and settings cannot change for a `Core` by design. Opening and
  driving streams concurrently from a shared `&Core` is therefore safe.
  (Unstable period is read once at `open` — it only sizes the required
  history — and never consulted again.)
- **A stream handle is `Send` but single-writer.** `update(&mut self)` makes
  concurrent updates on one handle a compile error, so the "one thread per
  stream" rule is enforced, not merely advised; being `Send`, a handle can
  still be moved between threads (e.g. a work-stealing pool) as long as it is
  never driven from two at once.

Stricter than the C tier, where `TA_Globals` is a process-wide mutable and
concurrent batch is safe only if nothing calls `TA_SetX` meanwhile.

### Concurrency across tiers (C / Java / .NET)

The Rust guarantees above rest on one rule that holds in every backend, each
tier enforcing it its own way:

> **A stream's value-affecting settings (compatibility, candle settings) must
> not change over its lifetime.** Nothing is copied into the handle — settings
> remain the shared, effectively-static data they already are, so a user
> maintaining thousands of streams pays zero per-stream settings overhead.

- **Rust.** Enforced by construction: settings live in the immutable `Core`
  (issue #104) the stream was opened from; they cannot change, so a violation
  is not even expressible.
- **C.** Documented, as an extension of the existing batch-tier caveat:
  calling `TA_SetCompatibility`/`TA_SetCandleSettings` while streams are open
  is undefined (CDL\* warm-up and ring sizes are derived from the settings in
  effect at `open`; values read them per bar). Streams add no *new* hazard —
  the C batch tier already requires that nothing calls `TA_SetX` during
  concurrent calls. Distinct handles are otherwise fully independent; a
  single handle is **single-writer**: driving one handle from two threads
  concurrently — `update` *or* `peek`, despite the latter's `const` — is
  undefined behavior (C states as documentation what Rust enforces with
  `&mut`).
- **Java / .NET.** Settings live per-`Core`-instance, so there is no
  process-global hazard; the same documented rule applies per instance: don't
  mutate a `Core`'s settings while streams opened from it are live.
  Single-writer per handle; no synchronization in the generated code (safe
  publication when handing a handle between threads is the caller's usual
  memory-model responsibility).

### Python (future consumer — exploration, not in scope)

The Python wrapper is a separate project, but it is the origin of the
`talib.stream` prior art and the largest consumer of the C library, so the C
tier is checked here against how Python *would* wrap it:

- **Discovery.** `TA_GetFuncInfo()->flags & TA_FUNC_FLG_STREAM` — the same
  bitmask `talib.abstract` already reads — tells the wrapper which functions
  have a stream API.
- **Shape.** A small extension type per function wrapping the C handle:
  `s = talib.stream.SMA(timeperiod=14, history=closes)` → `s.update(price)`,
  `s.peek(price)`. The capsule destructor calls `TA_XXX_Close`. This
  removes both `talib.stream` limitations at the source: retained state makes
  it O(1)/tick, and full-history seeding at open removes the
  recursive-function divergence.
- **Single-bar update is worth it from Python.** A Python→C call costs on the
  order of ~100 ns; the update itself is nanoseconds. That overhead is real
  but replaces today's O(lookback) reach-back *plus* the same call overhead —
  strictly better, and far below tick rates that matter to a Python strategy
  loop.
- **No `TA_XXX_UpdateMany` in the C tier now.** Chunked updates only matter
  for replaying history, and `open(history)` already consumes history in one
  C pass; live use is inherently one bar per call. If profiling of a real
  consumer ever shows per-call overhead dominating, a generic
  `update_many(ndarray) -> ndarray` entry point can be added later without
  changing the handle model.
- **GIL.** Do not release the GIL around a single update — the
  release/reacquire costs more than the sub-microsecond update it unblocks.
  (An eventual `update_many` should release it.)
- **Free-threaded Python (3.13+).** One handle per thread — the same
  single-writer rule the Rust tier enforces — is safe with no GIL, because
  update touches only handle-local state plus read-only settings (the
  settings-stability rule above).
- **multiprocessing / persistence.** Handles are not serializable — restating
  the existing non-goal: a handle never crosses a process boundary or a
  library version; each worker opens its own stream from history.
- **asyncio.** Updates are non-blocking and sub-microsecond; call them
  directly from coroutines. No special support needed.

### Non-goals / risks
- **No behavioral change to the batch tier.** The stream tier is additive.
- In first release, MAVP (per-bar variable periods) stays unsupported
  (documented); functions taking an `MAType` parameter with `MAType=MAMA`
  either follow MAMA's own tier or are documented per the composed-tier notes.
- **Single precision:** no `TA_S_*` stream twin; the stream tier is
  double-only.
- Handle lifetime: the handle is valid only within the exact library version
  that created it — not serializable or persistable across versions or
  processes. Cross-version persistence is out of scope for now; it may be
  revisited once the streaming feature stabilizes.
- Some annotation might be needed in the generator input to guide the stream
  emitter.
- **Maintenance coupling (accepted, gated):** a future batch rewrite of an
  indicator (e.g. a class-A optimization) can change its loop shape and break
  stream *analyzability* even while batch outputs are unchanged. The stream
  tier therefore adds a generator gate: `generate` fails if a
  `streaming: true` function is no longer analyzable, so the coupling is
  caught at the PR that introduces it, not at release.

## Implementation

### Feasibility evidence: spike (2026-07-09)

A ta_codegen spike (branch `spike/streaming-codegen`, ~1,100 LOC
`generator/src/streaming.rs` + a hand-written harness) validated the core
mechanism end-to-end:

- **Bit-exactness works as designed.** Generated C streams for MULT (T1),
  EMA (T2, both compatibility modes) and SMA (T3, including period=1) were
  verified **bit-identical to `batch(startIdx=0)`** over 2,000 bars across
  43 parameter/warm-up/compat cases — 43/43, memcmp on doubles, including
  short-history open rejection.
- **`open` = whole-body transcription.** The spike transcribes the *entire*
  batch body (with `startIdx=0`, `endIdx=historyLen-1`, output writes
  redirected to a scalar) and then captures the still-live locals into the
  state struct. No separate seed analysis is needed, and batch-equal state is
  obtained by construction — including compatibility-branched seeding.
- **Ring lags need no symbolic analysis.** The trailing-pointer lag
  (`cursor − trailingIdx`) is loop-invariant, so `open` captures it
  *numerically* at runtime from the transcribed locals and sizes/fills the
  ring from the history tail. (Caveat for ring-order functions — see the T3
  constraint below.)
- **The emitters reuse the batch renderers.** The stream emitter is an
  IR-to-IR transform (current-bar reads → `bar` param, carried locals →
  state fields, trailing reads → ring slots, index bookkeeping dropped)
  followed by the existing `render_statement`. Operation order is untouched,
  which is *why* bit-exactness holds by construction. This also means the
  bit-exactness argument survives batch-code evolution: the stream is
  re-derived from the same IR on every generate.
- **Analyzer census** (naive spike analyzer over all 161): 62 analyze clean as
  single self-contained loops (incl. RSI, ADX, KAMA, SAR/SAREXT, MACD, T3,
  DEMA/TEMA/TRIX, MIN/MAX family, ADOSC). Every remaining failure falls into
  one of four *mechanical* categories: multi-array trailing rings (the CDL\*
  family — one trailing index over 4 price arrays — plus ULTOSC/WILLR/DX);
  window-rescan reads (`in[today-i]` inner loops: LINEARREG family, AVGDEV,
  CORREL, IMI, TSF); CIRCBUF-backed state (CCI, MFI, HT_\* family, MAMA — the
  IR already models CIRCBUF, it lowers naturally to state-struct rings); and
  no-steady-loop bodies (countdown loops: AD, ATR; plus the composed
  functions below).

### Streamability tiers (classification of all 161 functions)

Corrected after a full 161-function audit (38 corrections to the original
table):

| Tier | Shape | State | Per-update cost | Examples (not exhaustive) |
|------|-------|-------|-----------------|---------------------------|
| T1 | pure per-bar map | none | O(1) | ADD…DIV, math/price transforms, BOP |
| T2 | scalar recurrence | O(1) scalars | O(1) | EMA, DEMA/TEMA/TRIX, MACD/MACDFIX, RSI, CMO, ADX/DI/DM/DX, ATR (steady), OBV, AD (cumulative), ADOSC, SAR/SAREXT, T3, TRANGE (prev-close scalar) |
| T3 | fixed trailing window | ring O(period) | O(1) for rolling sums (SMA, WMA, SUM, VAR/STDDEV, MOM/ROC\*, MFI, TRIMA); **O(period)** for window recomputers (LINEARREG family, TSF, BETA, CORREL, AVGDEV, IMI, CCI, ULTOSC, CDL\* candle averages) | see left | KAMA (sliding ROC-sum ring — *not* T2), HT_\* family + MAMA (single loops, bounded ≤50-slot rings — *not* "needs restructuring") |
| T4a | window extrema, value-output | monotonic deque O(period) | amortized O(1) | MIN/MAX, MIDPOINT/MIDPRICE, WILLR, STOCH's raw range |
| T4b | window extrema, **index-observable** | ring + cached-index automaton (transcribed verbatim) | amortized O(1), worst O(period) | MININDEX/MAXINDEX/MINMAXINDEX, AROON/AROONOSC |
| TC | **composed** — calls other indicators over intermediate arrays | sub-stream handles as state members | sum of parts | BBANDS, STOCH/STOCHF (T4a + MA slowing), STOCHRSI, APO/PPO, MACDEXT, MA (factory over the 9 MA streams), ACCBANDS, ADXR (ADX sub-stream **+ O(period) ring of past ADX outputs** — not T2), STDDEV-as-written (fusable into VAR), NATR (degenerate path) |
| T5 | unsupported v1 | — | — | MAVP (per-bar variable period) |

Structural notes:

- **T4a deque is legal only for value outputs.** Batch extrema use *different*
  tie rules on their two paths (strict `<` on rescan, `<=` on the incoming
  side), so the selected *index* is path-dependent and no single deque
  discipline reproduces it (counterexample: MININDEX period=2 on `[3,3]`
  diverges on the first output). For T4a ties are bit-identical input copies,
  so values match regardless of which index wins. T4b functions output the
  index (or compute from it — AROON), so their stream must transcribe the
  batch cached-index automaton over a ring of the window: same memory as the
  deque, amortized O(1).
- **T3 ring-order constraint (CCI class).** Some batch code sums its circular
  buffer *in buffer order*, so the FP summation order depends on the ring's
  rotation phase. The stream's ring phase must equal batch's — the safe rule
  is that `open` replays the entire provided history through the ring (never
  memcpy'ing the trailing window) for any function whose batch iterates its
  buffer by position. Rolling-sum T3 (SMA-style sequential trailing reads) is
  phase-free; the spike verified the memcpy shortcut is safe there.
- **The EMA cascades (DEMA/TEMA/TRIX/MACD) run in lockstep per-bar** — scalar
  `prevEMA1/2/3`, no intermediate buffers — so the steady-state loop body
  already *is* the stream transition function (verified in the spike: MACD
  analyzes clean as T2). This does **not** extend to MACDEXT (three
  runtime-`MAType` sub-streams → TC).
- **TC composition model:** the state struct holds sub-stream handles;
  `update()` feeds intermediate values through sub-streams in the same order
  batch fills its temp buffers; `MAType` params dispatch at `open` to the
  selected sub-stream (a stream MA handle is a small tagged union over the 9
  MA streams). Sub-stream warm-up alignment must match batch's temp-buffer
  seeding windows — this is the composed tier's one subtle piece and gets its
  own spike (below). ~13 functions plus 4 open-time-only delegations
  (SAR/SAREXT's one-bar `minus_dm` bootstrap, NATR's degenerate path,
  MACDFIX's parameter aliasing).

### How it fits ta_codegen

1. **Metadata, not logic, in YAML**: a function opts in by adding `stream`
   to its existing `flags:` list (`flags: [overlap, stream]`) — the same
   list that already carries `unstable_period` etc., and the flag maps to
   `TA_FUNC_FLG_STREAM` in ta_abstract like every other entry. No flag = no
   stream code, ever. Everything else — the tier, the state shape — is
   **derived from the IR**, never authored: there is nothing for a
   developer to figure out when adding a function. `ta_codegen
   stream-census` reports what each function derives (`--seed-yaml` writes
   the flags), and `generate` **fails** if a flagged function is no longer
   analyzable or its transition can no longer be built (the
   maintenance-coupling gate).
2. **Loop identification + analysis pass** (`generator/src/streaming.rs`).
   The parser does *not* pre-isolate the steady-state loop — `FuncDef.body`
   is a flat statement list and the loop appears as `while(i<=endIdx)`,
   `do{...}while(i<=endIdx)`, `for(i=startIdx;...)`, or countdown forms, and
   is absent entirely in composed functions. The pass therefore: finds and
   classifies the steady loop; computes *loop-carried scalars* (read-before-
   write across iterations) → state fields; *current-bar reads* → update
   parameters; *trailing-window reads* and bounded `in[i-K]` look-backs →
   rings/lag slots; CIRCBUF statements → state-struct rings (the IR already
   models them). The batch IR is untouched; the analysis only *reads* it.
   (Spike-validated, including the census that seeds the YAML tiers.)
3. **Emitters** (`backends/c_stream.rs`, `rust_stream.rs`, …) are IR-to-IR
   transforms feeding the *existing* statement/expression renderers:
   - `open` = transcription of the **entire batch body** (`startIdx=0`,
     output writes → last-value scalars) + a generated epilogue capturing the
     live locals and ring fills into the state struct.
     Batch-equal state by construction; compatibility-branched seeding and
     the unstable-period skip logic come along verbatim — nothing is
     stripped from the input code. (Spike-validated bit-exact.)
   - `update` = the steady-loop body with the variable remapping, emitted
     **once** as the transition function; `peek` is a two-line wrapper — an
     O(state) copy of the state struct (a stack copy for the scalar tiers;
     ring tiers will pre-allocate a scratch mirror in the handle at open),
     then a call to that *same* transition function. One body means there is no
     peek-specific logic to drift; `stream_verify` still asserts
     peek == update. T4a substitutes the deque; T4b
     transcribes the cached-index automaton over a ring; TC emits sub-stream
     calls in batch temp-buffer order.
   - Generation-time invariant checks: no global *writes* and no
     compatibility reads outside `open` (candle-settings reads in CDL\*
     update bodies mirror batch's own); no index-variable leakage; declared
     tier matches analyzed shape.
4. **Servers**: `generate-servers` adds a `stream_verify` method (see
   *Verification*). The warm-up prefix lengths are computed server-side from
   the language's own generated lookback function (which folds in the
   ambient unstable period) + 1. Mismatch diagnostics come back inline
   (first-divergence bar and `%a` values), so no separate trajectory-dump
   method is needed; one can be added later if debugging ever wants full
   trajectories.
5. **ta_abstract / introspection — DONE**: `TA_FUNC_FLG_STREAM`
   (`0x02000000`, a previously free bit in the existing `TA_FuncFlags`
   word) is set for every `streaming: true` function, sourced from the YAML
   bool and mirrored into the Rust/Java abstract tables and
   `ta_func_api.xml` (`<Flag>Streaming</Flag>`). Purely ABI-additive: no
   struct or export changes; wrappers read `TA_GetFuncInfo()->flags` as
   they always have, and warm-up sizing reuses the existing lookback
   metadata (`min_history = lookback + 1`). The regtest stream pass
   enforces set equality — a flagged function without a server stream (or
   the reverse) fails the run.

### Verification

**Bit-identical comparison cannot ride the existing JSON path** — the C server
emits `%.15g`, .NET emits `G15` (doubles need 17 significant digits to
round-trip), inputs are sent at `%.15g`, and the comparator is epsilon-based.
Changing those shared formatters would perturb every existing comparison.
Instead (implemented, riding the fuzz-064 seed-in idea one step further):

- `stream_verify(funcName, params, gen_shape/seed/n, unstablePeriod,
  compatibility)`: the server generates the input series from the seed
  (`fuzz_data.h`), runs **both** `batch(startIdx=0)` and, for each warm-up
  prefix in `{lookback+1, lookback+13, n/2, n-1}` (the stream analog of
  `doRangeTest` — it exercises the open/update boundary, exactly where an
  open-side seeding bug would hide), the stream trajectory (`Open` on the
  prefix, `Update` per remaining bar, `Peek` spot-asserted equal to the
  following `Update`), entirely in-process on identical in-memory inputs, and
  compares **bitwise per bar** (memcmp on doubles) — stronger than a hash,
  and the first divergence comes back inline as `%a` hex with its bar and
  output index. The response carries flat per-leg match flags; the driver
  never parses a float.
- Unstable period and compatibility are **request parameters**: the server
  pins them for both legs and restores them afterwards, so both legs always
  run under identical ambient state (no sticky cross-request contamination).
- ta_regtest’s stream pass (part of `--codegen`) drives it per function with
  three param vectors — defaults, every integer param at its true minimum
  (period==1, the #93/#94 territory the 0.6.4 fuzz must floor away), and
  min+1 — plus a K>0 leg for unstable functions and a Metastock leg. Any
  mismatch fails the run with `TA_CODEGEN_STREAM_MISMATCH`.
- Servers without the method (the foreign languages until their emitters
  land) are detected by a capability probe and skipped, not failed;
  non-streamable functions answer `not_streamable` and count as skips.
- Bit-exactness is verified as a *within-language* property; cross-language
  batch equivalence is already covered at epsilon by the existing pass.
  Follow-ups: sanitizer legs (the server build does not yet take the
  ENABLE_SANITIZERS flags) and a second-compiler leg (bit-exactness is
  currently proven in the gcc-built single-TU server; MSVC/clang builds of
  the shipped library are exercised by CI compiles but not stream-verified).
  Candle-settings variations join when CDL\* streams land (T3).

### Delivery surface (owned by ta_codegen, must ship together)

The stream tier touches every generated deliverable, not just `src/ta_func`:
`include/ta_func.h` declarations (with `TA_LIB_API`), CMake `LIB_SOURCES` +
`src/ta_func/Makefile.am` + `ta_func_list.txt` (the dist-verification CI path
builds from the autotools lists — a missed entry breaks the nightly),
ta_abstract tables (streamable flag), the per-function website/`.md` docs and
rustdoc (a `## Streaming` section per function page), and a `ta_bench` stream
mode (ns/update per function, vs the batch amortized cost — the performance
claim in *Motivation* gets measured, not asserted).

### Staging

0. **Census + flag authoring — DONE.** `ta_codegen stream-census` derives
   each function's streamability from the IR (`--seed-yaml` writes the
   `streaming: true` flags); `generate` fails if a declared function stops
   being analyzable, and the same gate runs in `cargo test`. The Rust tier's
   pre-requisite, issue #104 (immutable `Core`), is closed.
1. **T1 + T2 for C — DONE** (37 functions: 24 T1 maps and price transforms,
   13 T2 recurrences — AD, ADOSC, ADX, DEMA, EMA, MACD, OBV, SAR, SAREXT,
   T3, TEMA, TRANGE, TRIX), with `stream_verify` and the regtest stream pass
   wired in and gating. Includes multi-output (MACD), price-input bars
   (TRANGE, BOP), countdown-loop bodies (AD), and the batch's explicit
   `period==1` identity paths (T3, TEMA) mirrored as a transition
   short-circuit. *Discovered stage-1.5 class (deferred, census-tracked):*
   param-degenerate paths the whole-body transcription cannot yet mirror —
   RSI/CMO (`memmove` identity + a Metastock mid-loop exit),
   MINUS/PLUS_DI/DM (unsmoothed `period<=1` alternate path), ATR/NATR
   (`period<=1` delegates to TRANGE).
2. **T3 ring machinery — DONE** (+18: SMA SUM VAR MOM ROC ROCP ROCR
   ROCR100 KAMA BETA CORREL as trailing rings; LINEARREG ×4 + TSF as
   O(period)/tick rescan windows over a ring in identical FP order; CCI +
   MFI as CIRCBUF state — the batch loop already carries the circular
   buffer, so open captures the LIVE buffer contents and rotation phase and
   the ring-order constraint holds by construction, no replay needed).
   HT_\* and MAMA remain blocked on deeper shapes (cursor-aliased
   `startIdx`, `in[idx--]` reads, fixed-size array locals).
3. **T4 extrema — DONE** (+10: MIN MAX MININDEX MAXINDEX MINMAX
   MINMAXINDEX MIDPOINT WILLR AROON AROONOSC), and **not** via deques: the
   cached-index automaton is transcribed verbatim over an absolute-index
   ring (`in[X]` → `ring[X % cap]`; the cursor and cached indices are
   batch-absolute state ints, periodically rebased by a multiple of the
   capacity long before INT_MAX — index differences and residues are
   invariant, so values never change; index-observable outputs report the
   rebased position beyond ~2^30 bars, where the batch contract is
   inherently out of int range anyway). Ties, indices and values are
   batch-exact by construction. Integer outputs are supported end-to-end.
4. **CDL candlestick family — DONE** (+59 of 61, plus ULTOSC as a
   side-effect = 125 streamable). The candle helpers are pure scalar
   functions, so their calls transcribe directly; the transition renders
   them as `TA_STREAM_CANDLE*` macros that mirror the batch macros'
   arithmetic exactly (the helper source's algebraically-equal-but-
   differently-rounded Shadows form must never be inlined). Settings are
   read where batch reads them (per step, from the globals) under the
   settings-stability rule; the harness re-verifies every CDL under
   bumped and zeroed avgPeriods. New mechanisms, all language-neutral:
   back/forward/counter-offset trailing rings (absolute-mod layout,
   `cap = lag + back + 1`, current bar pre-written at `pos`), fixed-size
   array locals as carried state, ternary-index normalization
   (`in[c ? a : b]` → `c ? in[a] : in[b]`), widest-literal merge for
   multi-bound rescan counters. CDLHIKKAKE/CDLHIKKAKEMOD stay batch-only
   for now (they save bar indices — absolute-index recall beyond the
   extrema automaton).
5. **Stage-1.5 param-degenerate paths — PARTIALLY DONE** (+RSI, CMO, WMA,
   AVGDEV, then DX and IMI = 131 streamable; DX's zero-denominator
   `out[idx-1]` repeat carries as `lastOut_*` state refreshed after each
   update, and IMI's cursor-anchored window
   `for (i = cursor-(p-1); i <= cursor; i++)` normalizes to a descending
   offset counter — bars still visit oldest-first, so FP accumulation
   order is untouched): the period-1 `memmove` form now matches the
   identity fast path, and block-scoped loop locals classify as temps.
   One honest contract nuance came out of RSI/CMO under Metastock: their
   batch emits a seed output and, when continuing, REWINDS and rebuilds
   state — so no bit-exact continuation exists from the seed exit. Open
   returns `TA_BAD_PARAM` at exactly `lookback+1` in that mode (one more
   bar is required); the verifier knows statically which functions have a
   seed boundary and shifts its boundary leg. The remaining members
   (ATR/NATR/MACDEXT/MACDFIX delegate to other functions at period 1,
   DI/DM have a dual unsmoothed loop) belong with the composed tier.
   Also from review: the candle-settings harness now runs FOUR rounds
   (defaults, +3, zeroed, all-Shadows — the last gates the Shadows
   macro arithmetic no default exercises), rounds continue past a
   too-short-history rejection, and assigning a candle-setting local in
   a steady loop is a hard analyzer error. (dual steady loops, memmove
   identity, open-time delegation) — a controlled preview of composition.
6. **TC composed functions** — **MA dispatch + composition spike DONE
   (2026-07-10)**; the rest of the family is in progress. Composition goes
   through the PUBLIC stream handles, not cross-TU internals:
   - **`TA_MA_Stream` (DONE, 132 streamable).** The analyzer recognizes the
     dispatch body shape (identity path + switch over an enum optional param
     whose arms delegate the whole range) and the emitter renders a tagged
     handle over the callees' public streams. The supported-arm set is
     DERIVED from the callees' YAML stream flags at generation time —
     TRIMA's arm joins automatically the moment its stream lands; MAMA's
     arm (discarded-FAMA dummy-buffer shape) stays a documented Open
     `TA_BAD_PARAM`. A stream-flagged callee arm that loses its strict
     whole-range delegation shape is a hard generate error, never a silent
     reject (that would turn a generator regression into a vacuous pass).
     Verification: the driver now sweeps enum (MAType) params — one vector
     per non-default value, each with its own K and Metastock legs (the
     selected arm may be unstable or compatibility-seeded even though the
     dispatcher is not) — and the server carries a generated expect-reject
     precheck for unsupported arms (`unsupportedArm`), so the documented
     rejection is verified loudly. Both a wrong-period arm and a
     wrongly-succeeding TRIMA arm were sabotage-tested; ASan/UBSan clean
     over the full 9-MAType × period × K/compat × 7-shape grid.
   - **STOCH composition spike (DONE, mechanism proven).** A hand-written
     STOCH stream over public `TA_MA_Stream` handles: Open transcribes the
     batch body — and since batch already MATERIALIZES the intermediate
     series (STOCH's tempBuffer) before calling `ma(0, n-1, ...)` over it,
     Open opens the sub-streams on those very arrays at the exact points
     batch consumes them (raw %K before the in-place smoothing, smoothed %K
     after) before they are freed. Update pipelines: transcribed extrema
     step → raw %K → sub-Update(K) → sub-Update(D) → outputs. Peek uses a
     `peekMode` flag in the scratch state copy so the ONE step body calls
     sub-Peek instead of sub-Update (heap sub-handles cannot be cloned by a
     struct copy). Result: 14,580 cases across the full 9×9 MAType grid ×
     6 param sets (incl. period-1 identity-MA composition and fastK=1) ×
     5 data shapes (incl. CONSTANT and TIE_HEAVY) × 3 warm-up prefixes —
     **1,146,950 update legs bit-identical to `batch(0, n-1)`**, peek ==
     update on every bar, TRIMA/MAMA arms reject as documented, 0 failures;
     a deliberate wrong-order sub-open (smoothed instead of raw %K) fails
     4,394 legs, proving the harness. Bit-exactness composes by induction:
     each sub-stream is bit-exact against its own batch over the full
     intermediate series, which is exactly what the composed batch computes.
   - **Remaining:** generalize the pipeline model in the analyzer/emitter
     (sub-open anchor `max(0, sArg − callee_lookback)` at each consuming
     call site; intermediate-series scalars in the transition; combine-loop
     alignment algebra) and roll STOCH, STOCHF, STDDEV, APO/PPO, STOCHRSI,
     MACDEXT, ADXR (sub-output ring), BBANDS. Then the delegating
     param-degenerates (ATR/NATR period 1 = an embedded TRANGE stream;
     MACDFIX = generation-time inlining of macd's IR with substituted
     `0,0` args — those select the FIXED k=0.15/0.075 coefficients inside
     macd's body, so the public `TA_MACD_Open` validation can never accept
     them), while DI/DM, TRIMA and MIDPRICE need a dual transition selected
     once at Open by the fixed param.
   **Strategy: exhaust the C emitter tier by tier — every gotcha is
   language-neutral — and only then port the proven model to Rust; the Rust
   emitter is a re-rendering of settled machinery, not a second discovery.**
7. **T5 leftovers**: MAVP documented-unsupported; HT_\* deep shapes.
8. **Rust emitter** (re-applying the exhausted C model), then Java/.NET
   (managed C# emitter, not P/Invoke — see lifecycle section).
