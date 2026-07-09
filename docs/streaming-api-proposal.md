# Proposal: Generated Streaming (Incremental) API for ta_codegen

Status: **proposal only** — no implementation. Written 2026-07-02.

## Design

### Motivation

Live-trading users typically recompute an indicator over its whole history on
every new bar. The Python wrapper's experimental `talib.stream` was a good first
step past that — a lighter "just the latest value" call — but it keeps no state
and diverges from the batch for recursive indicators (see next section).

A true streaming tier — per-indicator state + O(1) amortized
`update(bar) -> latest value`, bit-exact to the full-history batch — closes both
gaps. A *generated* one (one state struct + transition function per indicator,
emitted for all native backends from the same canonical input) is a genuine TA-Lib differentiator, and ta_codegen makes it feasible without hand-writing 161 stream implementations.

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

### Public API (stream lifecycle)

Every stream, in every backend, is the same lifecycle:

1. **`open(params, history[]) → (handle, value)` — once.** Validates the params
   and consumes the provided history in one pass, returning an opaque, typed
   stream **handle** (allocated by the library) plus the current value (the
   indicator for the last history bar). It retains no input array afterward —
   everything needed is kept in the handle. (Needs at least `lookback+1` bars to
   yield a first output; `xxx_lookback(params)` gives that count.)
2. **`update(handle, bar) → value` — once per new bar.** Takes the handle and the
   new input(s) (OHLCV for multi-input functions), advances the O(1) state, and
   returns the latest value.
3. **`close(handle)`.** Releases the stream — explicit in C, implicit (automatic)
   in managed backends.

Parameters and history are fixed at `open`, so `update` carries no
configuration; changing a parameter means a new stream. The handle is opaque and
tied to the library version that created it — don't persist it across versions or
processes. Per-backend signatures are in the *API shape per backend* section below.

Multi-output functions (MACD, BBANDS, STOCH) return a tuple for value.

### Semantic definition (the contract tests enforce)

For every function F and bar stream x[0..t]:

```
stream_F.update(x[t]) == batch_F(startIdx=t, endIdx=t, full history 0..t)
```

i.e. streaming output at bar t equals the batch output over the same data —
where "the data" is whatever `open` was given plus every `update` since, seeded
from its index 0. Priming `open` from bar 0 therefore makes the stream bit-exact
to the full-history batch.

Validated against the existing batch code for robust bit-identical matching:
batch is heavily tested against frozen references (e.g. 0.6.4), and stream is
tested against batch — so stream test coverage is transitively exhaustive.

`TA_SetUnstablePeriod` shifts only *which* bars the batch reports (its
`outBegIdx`), never their values, so the check holds for any global unstable setting.

### API shape per backend (signatures)

C (`open` allocates and returns an opaque handle; `close` frees it):

```c
double out;
/* open consumes warm-up history in one pass; writes the current value */
TA_SMA_Stream *s = TA_SMA_Open(optInTimePeriod, history, historyLen, &out);
for (each new bar)
    TA_SMA_Update(s, close, &out);               /* always writes an output */
TA_SMA_Close(s);
```

Rust:

```rust
let mut s = SmaStream::open(14, &history)?;      // warm from history
for &x in new_bars { let v = s.update(x); /* always a value */ }
```

Java/.NET: small handle objects with the same `open(params, history)` +
`update` shape. Multi-input functions take `(open, high, low, close, volume)` as
their batch signature demands; multi-output return small structs
(`(f64, f64, f64)` for MACD).

### Rust concurrency

The Rust binding is shaped so concurrent batch and per-thread streams are
enforced by the type system, not by convention. This justifies the API shape
above:

- **`Core` is immutable after construction** (built via `Core::builder()`, no
  setters — issue #104). Its fields are plain data, so `Core: Send + Sync`: share
  one `Arc<Core>` read-only across threads and call any batch function
  concurrently — safe by construction, since a call only *reads* the globals
  (compatibility, candle settings, unstable period). Changing a setting means
  building a new `Core` (it is small; cloning is cheap).
- **`open` is a `&self` method on `Core`.** It snapshots the value-affecting
  globals (compatibility, candle settings — not unstable period, which streams
  ignore) into the returned handle, so opening streams concurrently from a shared
  `&Core` is also safe.
- **A stream handle is `Send` but single-writer.** `update(&mut self)` makes
  concurrent updates on one handle a compile error, so the "one thread per
  stream" rule is enforced, not merely advised; being `Send`, a handle can still
  be moved between threads (e.g. a work-stealing pool) as long as it is never
  driven from two at once.

Stricter than the C tier, where `TA_Globals` is a process-wide mutable and
concurrent batch is safe only if nothing calls `TA_SetX` meanwhile.

### Non-goals / risks

- **No behavioral change to the batch tier.** The stream tier is additive.
- API surface doubles per function — gate emission on the YAML flag so we can
  ship tiers incrementally.
- Candle settings: CDL streams snapshot `TA_Globals->candleSettings` at `open`
  (documented), avoiding mid-stream global mutation hazards.
- MAVP and functions with per-bar variable periods stay unsupported
  (documented) rather than pretending.
- Memory policy in C: `open` allocates the handle, `close` frees it.
- Handle lifetime: the handle is valid only within the exact library version
  that created it — not serializable or persistable across versions or
  processes. Cross-version persistence is out of scope for now; it may be
  revisited once the streaming feature stabilizes.

## Implementation

### Streamability tiers (classification of all 161 functions)

| Tier | Shape | State | Examples (not exhaustive) |
|------|-------|-------|---------------------------|
| T1 | pure per-bar map | none | ADD…DIV, all math transforms, price transforms, BOP, TRANGE |
| T2 | scalar recurrence | O(1) scalars | EMA, DEMA/TEMA/TRIX, RSI, CMO, ADX/ADXR/DI/DM, ATR/NATR, KAMA, SAR/SAREXT, OBV, AD, T3 |
| T3 | fixed window | ring buffer O(period) | SMA, WMA, SUM, VAR/STDDEV, MOM/ROC*, TRIMA, CCI, MFI, ULTOSC, LINEARREG family, CDL* body averages |
| T4 | window extrema | monotonic deque O(period) | MIN/MAX/MINMAX(INDEX), MIDPOINT/MIDPRICE, WILLR, AROON*, STOCH/STOCHF |
| T5 | needs restructuring first | — | STOCHRSI, MAMA, HT_* (ring buffers already fixed-size → actually T2/T3 after inspection), MAVP (per-bar period ⇒ no fixed window) |

Two structural notes on the current code:

- **T4 uses a monotonic deque, not the batch's idiom.** The batch extrema
  functions use a cached-extremum-index that rescans arbitrarily far back when
  the extremum leaves the window — a stream cannot rescan without retaining the
  whole window anyway. A monotonic deque gives amortized O(1) per tick with
  exactly the window's contents, so the stream emitter substitutes it for T4.
- **The EMA cascades (DEMA/TEMA/TRIX/MACD) run in lockstep per-bar** — scalar
  `prevEMA1/2/3`, no intermediate buffers — so the steady-state loop body already
  *is* the stream transition function. The stream tier is nearly free for that
  family (hence their T2 classification above).

### How it fits ta_codegen

1. **Metadata, not logic, in YAML**: each function's YAML gains
   `streaming: {tier: T2}` (data only). The tier list is authored once from
   the classification above and validated by the generator (it refuses to
   emit a stream for a function whose IR doesn't match the tier's shape).
2. **IR analysis pass** (`generator/src/streaming.rs`): for the steady-state
   loop of each FuncDef (the `while(today <= endIdx)` body — the parser
   already isolates it), compute:
   - *loop-carried scalars*: locals read before written across iterations →
     state struct fields;
   - *trailing-window reads*: `in[trailingIdx]`-style accesses → ring buffer
     of size `lookback+1` plus the trailing pointer;
   - *rescan loops* (the cached-index `while(++i<=today)` idiom) → tier T4,
     replace with deque ops in the stream emitter.
   The batch IR is untouched; the analysis only *reads* it.
3. **New emitters** (`backends/c_stream.rs`, `rust_stream.rs`, …) render the
   state struct + open/update/close from the analyzed loop body. T1/T2 are a
   direct transcription; T3 swaps `in[trailingIdx]` for `ring[pos]`; T4 swaps
   the cached-index blocks for deque push/pop with the *same comparison
   semantics* (`>=`/`<=` on the incoming side to match batch tie-breaking).
4. **Servers**: `generate-servers` adds a `stream_call` method — same params
   as the batch call; the server `open`s the stream on the warm-up prefix, then
   feeds each remaining bar through `update`, returning the collected outputs.
5. **Verification**: ta_regtest gains a codegen pass that calls both
   `TA_XXX` (batch, `startIdx=0`) and `stream_call` on the same history and
   requires **bit-identical** outputs over the range the batch reports
   (`outBegIdx .. outBegIdx+outNBElement`), per language. Keep the batch at
   `startIdx=0` — a sliced batch (`startIdx > lookback`) seeds mid-series and
   would not match the from-index-0 stream. The frozen reference oracle stays
   the batch baseline; the stream contract is anchored to batch, so no new
   reference data is needed.

### Reuse model

The stream is **derived from, never fused with, the batch.** Auto-analysis (§2)
reads the batch and emits a *separate* stream, so contributors keep writing plain
batch — no stream-specific authoring for the functions analysis can crack (all of
T1/T2, running-sum T3 like SMA). Keeping batch independent is a *verification*
choice, not just hygiene: the stream-vs-batch check (§5) only has teeth if the two
sides don't share code. Each error class then anchors to something that is not a
copy of itself:

| Implementation | Anchored to | Catches |
|----------------|-------------|---------|
| batch | frozen reference oracle | wrong batch math |
| stream | batch, bit-exact (§5) | wrong *transformation* — ring size, trailing offset, seed partition |

Derivation isn't fully independent (a batch-math bug reaches both), but that class
is already covered by batch-vs-reference, and the batch never runs the ring/seed
transform — so stream-vs-batch stays a real differential on the transform itself.

Rejected: a single shared internal (`step()` used by both, or batch =
`init; loop{update}`). It makes the differential tautological, and for T3/T4 forces
batch onto ring/deque storage that regresses its tuned array-index loops.

Where analysis can't derive a passing stream (multi-phase seeds like ADX, T4
extrema templates, T5 restructuring cases), hand-write that one stream — independent by
construction, so its differential is only stronger — or mark it non-streamable.

### Staging

1. T1 + T2 for C and Rust (≈90 functions, mostly mechanical), stream-vs-batch
   regtest pass wired in from day one.
2. T3 ring-buffer machinery (+ LINEARREG family via ring, *not* via running
   sliding sums — those reassociate the floating-point ops, and bit-exactness
   vs batch requires recomputing the window sums the same way batch does,
   O(period) per tick for these; still fine for live use at real-world
   periods).
3. T4 deque extrema.
4. Remaining T5 (STOCHRSI, MAMA, HT_*, MAVP) — per-function restructuring or a
   hand-written stream.
5. Java/.NET emitters once C/Rust stabilize.
