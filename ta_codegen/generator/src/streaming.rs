//! Streaming (incremental) API analysis — docs/streaming-api-proposal.md.
//!
//! This module derives, from a [`FuncDef`]'s batch IR, everything the stream
//! emitters need: the steady-state loop (the per-bar transition), the state
//! carried between bars, bounded look-back lags, and the streamability tier.
//! The batch IR is never modified — analysis only reads it, and the emitters
//! render *rewritten copies* through the existing per-language renderers so
//! the operation order (and therefore bit-exactness versus the batch API) is
//! preserved by construction.
//!
//! Stage 1 scope: tiers T1 (pure per-bar map) and T2 (O(1) scalar state,
//! including bounded `in[cursor-K]` lag reads). Trailing-window rings (T3),
//! window extrema (T4), and composed functions are rejected with a
//! tier-specific error so the census stays honest.

use std::collections::{BTreeMap, BTreeSet};

use crate::ir::{BinOp, CircBuf, CircBufLayout, Expr, FuncDef, ParamType, Statement, StreamTier, VarType};

// ---------------------------------------------------------------------------
// Public types
// ---------------------------------------------------------------------------

/// Why a function cannot (yet) be streamed. Tier-specific so the census and
/// the YAML validation produce actionable messages.
#[derive(Debug, Clone, PartialEq, Eq)]
pub enum StreamError {
    /// No recognizable steady-state loop (composed functions, delegations).
    NoSteadyLoop,
    /// Reads a trailing window (`in[trailingIdx]`) — needs a T3 ring.
    NeedsRing(String),
    /// Unsupported array-access shape (window rescans, back-patched outputs).
    UnsupportedAccess(String),
    /// Loop state is not scalar (arrays, CIRCBUF) — beyond stage 1.
    NonScalarState(String),
    /// Loop references a symbol that is neither local, param, nor input.
    UnknownSymbol(String),
    /// Loop body calls something stateful (globals, other indicators).
    UnsupportedCall(String),
    /// Structural rule violated (see message).
    Unsupported(String),
}

impl std::fmt::Display for StreamError {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        match self {
            Self::NoSteadyLoop => write!(f, "no steady-state loop (composed or delegating body)"),
            Self::NeedsRing(v) => write!(f, "trailing-window read via `{v}` needs a T3 ring"),
            Self::UnsupportedAccess(m) => write!(f, "unsupported array access: {m}"),
            Self::NonScalarState(v) => write!(f, "non-scalar loop state `{v}`"),
            Self::UnknownSymbol(v) => write!(f, "loop references unknown symbol `{v}`"),
            Self::UnsupportedCall(c) => write!(f, "unsupported call in steady loop: {c}"),
            Self::Unsupported(m) => write!(f, "{m}"),
        }
    }
}

/// One bounded look-back: the transition reads `array[cursor - k]` for
/// `1 <= k <= depth`. Served by `depth` lag scalars shifted once per update.
#[derive(Debug, Clone, PartialEq, Eq)]
pub struct LagSlot {
    pub array: String,
    pub depth: i64,
}

/// A composed combine that reads a sub-output series at two offsets: the
/// current bar (`series[cursor + lag]`, the newest value) and a fixed
/// parameter lag behind it (`series[cursor]`). ADXR is the case:
/// `outReal[k] = (adx[k + (period-1)] + adx[k]) / 2` — the current ADX plus the
/// ADX from `period-1` bars ago. The stream keeps a ring of the last `lag`
/// sub-output values (cap captured at open from `lag`, a param expression), so
/// each update is O(1): read the oldest slot, combine, push the new value.
#[derive(Debug, Clone)]
pub struct SubLagRing {
    /// The sub-output series read at a self-lag.
    pub series: String,
    /// The lag depth `= ring capacity`, a parameter expression (`period-1`).
    pub lag: Expr,
}

/// One trailing-window ring: a batch index variable that walks the inputs a
/// fixed distance behind the cursor (`in[trailingIdx]`, SMA-style). The
/// stream keeps one position/capacity per index variable and one buffer per
/// input array it reads (CDL-style windows read several arrays through the
/// same trailing index). The capacity (`cursor - var`, loop-invariant) is
/// captured NUMERICALLY at the end of open — no symbolic analysis.
///
/// Phase-free only: the transition reads exactly `ring[pos]` (the oldest
/// slot). Batch code that iterates a buffer in storage order (CCI-class
/// CIRCBUF sums) has a rotation-phase-dependent FP order and is handled by
/// the CIRCBUF tranche, not this model.
#[derive(Debug, Clone, PartialEq, Eq)]
pub struct RingSpec {
    /// The batch trailing index variable (dropped from the transition).
    pub var: String,
    /// Input arrays read through it, in signature order.
    pub arrays: Vec<String>,
    /// Max constant back-offset read through the var (`in[var - K]`,
    /// shifted-candle averages). 0 means a plain oldest-slot ring (legacy
    /// layout); positive values switch to the absolute-mod layout (slot =
    /// bar index % cap with cap = runtime lag + back + 1).
    pub back: i64,
    /// Max forward offset (`in[var + K]`). Legal only when the runtime lag
    /// is >= fwd (batch legality guarantees it; Open re-checks and fails
    /// TA_INTERNAL_ERROR otherwise).
    pub fwd: i64,
}

/// One bounded rescan window: an inner loop reads `in[cursor - w]` where `w`
/// is that loop's counter, bounded by a parameter expression (LINEARREG's
/// `for(i=P; i--!=0;)`, AVGDEV's `for(i=0; i<P; i++)`). The stream keeps the
/// last `cap` bars (current bar included) in a ring written BEFORE the
/// transition body; reads become `win[(pos + cap - w) % cap]`, so the loaded
/// value sequence — and therefore the FP summation order — is exactly the
/// batch's. O(cap) work per update, same as the batch's own per-bar rescan.
#[derive(Debug, Clone)]
pub struct WindowSpec {
    /// The inner-loop counter variable (stays a live local in the transition).
    pub var: String,
    /// Window capacity: the loop bound (a parameter expression; max offset
    /// is `cap - 1`, offset 0 is the current bar).
    pub cap: Expr,
    /// Input arrays read through it, in signature order.
    pub arrays: Vec<String>,
}

/// One CIRCBUF-backed batch buffer carried as stream state. The batch loop
/// already maintains the circular buffer across iterations (CCI/MFI class),
/// so the stream captures the LIVE buffer — contents AND rotation phase —
/// at the end of open. Storage-order sums (`for j: sum += buf[j]`) therefore
/// see the exact byte sequence batch would, which is the ring-ORDER
/// constraint from the proposal satisfied by construction.
#[derive(Debug, Clone)]
pub struct CircState {
    /// CIRCBUF id (`circBuffer` in CCI).
    pub id: String,
    /// Storage layout: one buffer (Plain) or one per field (Class).
    pub layout: CircBufLayout,
}

/// A cursor-parity branch carried as stream state — the general handle for a
/// steady loop that selects an arm on the ABSOLUTE bar index (`cursor % 2`).
/// A stream cannot reconstruct the absolute index, so [`carry_cursor_parity`]
/// rewrites the `cursor % 2` sub-expression to read a carried `int` field
/// (registered as ordinary [`StreamModel::state`]); the emitter SEEDS it in
/// open (`historyLen % 2` — the parity of the next bar, established by open's
/// real-batch replay) and FLIPS it each update (`1 - parity`). The branch arms
/// are transcribed verbatim, so a correct seed reproduces the batch's
/// interleaving bit-for-bit; a 1-bar seed offset would swap the arms.
///
/// First consumer: the Hilbert-transform family (HT_DCPERIOD + riders), whose
/// odd/even quadrature arms carry an even-only `hilbertIdx` advance — exactly
/// the interleaving a wrong seed would invert.
#[derive(Debug, Clone)]
pub struct ParitySpec {
    /// State field name carrying `cursor % 2` (an `int` appended to `state`).
    pub field: String,
}

/// Absolute-index automaton (T4 extrema class: MIN/MAX/WILLR/MININDEX/
/// AROON...). The batch keeps a cached extremum INDEX and rescans the
/// window when it expires — tie-breaking differs between the rescan and
/// incoming paths, so no deque can reproduce it; instead the stream
/// transcribes the automaton verbatim: the cursor and every index local
/// become absolute-position state ints, and every input read `in[X]` maps
/// to `ring[X % cap]` over a ring of the last `cap` bars. Indices stay
/// batch-absolute (index outputs match batch bit-for-bit); the absolute
/// counters share batch's own int range (the contract is vacuous past
/// INT_MAX bars for batch too).
#[derive(Debug, Clone)]
pub struct ExtremaState {
    /// The paced window-start variable (advances once per bar with the
    /// cursor; `cursor - trailing + 1` = ring capacity, captured
    /// numerically at open).
    pub trailing: String,
    /// All other absolute-index locals of the automaton (cached extremum
    /// index, rescan cursor, ...). Kept as state ints.
    pub index_vars: Vec<String>,
    /// Input arrays read through absolute indices, in signature order.
    pub arrays: Vec<String>,
}

/// A recognized param-degenerate identity path: `if (<param> == 1) { copy
/// loop out[..] = in[..]; return SUCCESS; }` (T3/TEMA period==1 — explicit in
/// batch because the recurrence coefficients would leave FP drift).
/// The stream mirrors it: `update`/`peek` short-circuit to `out = bar`, and
/// `open` returns the last history bar directly (min history 1 on this path).
#[derive(Debug, Clone)]
pub struct IdentityPath {
    /// The guard condition, verbatim from the batch body (params only).
    pub condition: Expr,
    /// `(output array, input array)` copy pairs, one per output.
    pub pairs: Vec<(String, String)>,
}

/// Signature facts about a potential sub-stream callee, provided by a
/// [`CalleeLookup`]. Derived from YAML metadata only (no `.c` parsing), so
/// any layer that can read the input tree can supply it.
#[derive(Debug, Clone, PartialEq, Eq)]
pub struct CalleeSig {
    /// The callee is YAML `stream`-flagged (its public stream API exists).
    pub streaming: bool,
    /// Number of input ARRAYS (price components expanded).
    pub n_inputs: usize,
    /// Number of optional parameters.
    pub n_opts: usize,
    /// Number of outputs.
    pub n_outputs: usize,
}

/// Cross-function lookup for composed/dispatch analysis: maps an input-level
/// indicator name (`sma`, `ma`, ...) to its [`CalleeSig`]. Implemented by
/// [`crate::registry::Registry`] (emitters) and [`FuncsLookup`] (census,
/// server generation, tests).
pub trait CalleeLookup {
    fn callee(&self, name: &str) -> Option<CalleeSig>;
}

/// Signature facts derived from one [`FuncDef`] (shared by every
/// [`CalleeLookup`] implementation).
#[must_use]
pub fn callee_sig_of(f: &FuncDef) -> CalleeSig {
    CalleeSig {
        streaming: f.streaming,
        n_inputs: input_array_names(f).len(),
        n_opts: f.optional_inputs.len(),
        n_outputs: f.outputs.len(),
    }
}

/// [`CalleeLookup`] over a loaded [`FuncDef`] slice.
pub struct FuncsLookup<'a>(pub &'a [FuncDef]);

impl CalleeLookup for FuncsLookup<'_> {
    fn callee(&self, name: &str) -> Option<CalleeSig> {
        self.0
            .iter()
            .find(|f| f.name.eq_ignore_ascii_case(name))
            .map(callee_sig_of)
    }
}

/// One arm of a recognized dispatch body.
#[derive(Debug, Clone)]
pub struct DispatchArm {
    /// Case label verbatim from the IR switch (e.g. `TA_MAType_SMA`).
    pub label: String,
    /// Callee indicator (input-level lowercase name, e.g. `sma`). Empty for
    /// arms containing no indicator call (pure reject arms).
    pub callee: String,
    /// The callee's optional-input argument expressions, positionally
    /// mapped from the batch call. Meaningful only when `supported`.
    pub opt_args: Vec<Expr>,
    /// The arm delegates whole-range to a stream-flagged callee: the stream
    /// opens the callee's public stream. Unsupported arms reject at Open
    /// with BAD_PARAM (documented capability limitation, e.g. MAType=MAMA).
    pub supported: bool,
}

/// A recognized dispatch body (MA): optional identity path + a switch over
/// an enum optional param whose arms delegate the WHOLE range (`startIdx`,
/// `endIdx`, all inputs, all outputs forwarded) to other indicator
/// functions. The stream is a tagged handle over the callees' PUBLIC
/// streams; the supported-arm set is derived from the callees' YAML stream
/// flags at generation time, so a callee gaining the flag extends the
/// dispatch on the next generate.
#[derive(Debug)]
pub struct DispatchPlan<'a> {
    pub func: &'a FuncDef,
    /// The switch subject: an enum optional param, fixed at open.
    pub param: String,
    pub arms: Vec<DispatchArm>,
    /// Recognized param==1 identity path (checked BEFORE the dispatch,
    /// mirroring the batch body order — it applies to every arm).
    pub identity: Option<IdentityPath>,
}

impl DispatchPlan<'_> {
    /// Labels of the arms Open rejects (unsupported callees).
    #[must_use]
    pub fn unsupported_labels(&self) -> Vec<&str> {
        self.arms
            .iter()
            .filter(|a| !a.supported)
            .map(|a| a.label.as_str())
            .collect()
    }
}

/// One whole-range sub-call in a composed tail: `retCode = callee(sArg,
/// eArg, <srcs>, <opt args>, <begIdx recv>, <nb recv>, <dsts>)`. Open opens
/// the callee's public stream on the materialized sources at this exact
/// point (anchor `max(0, sArg - callee_lookback)`); Update pipes the
/// sources' current scalars through the sub handle into the destinations'.
#[derive(Debug, Clone)]
pub struct SubCallStep {
    /// Index of the call statement within the composed tail.
    pub tail_idx: usize,
    /// Callee indicator (input-level lowercase name, e.g. `ma`).
    pub callee: String,
    /// Callee optional-input argument expressions, positional and pure.
    pub opt_args: Vec<Expr>,
    /// The call's startIdx / endIdx argument expressions (evaluated in the
    /// transcribed Open scope for the sub-open anchor).
    pub s_arg: Expr,
    pub e_arg: Expr,
    /// Source series per callee input, in callee signature order: an
    /// already-materialized series, or the caller's own single real bar
    /// input (STOCHRSI feeds `rsi(inReal)`; STOCHF's three price inputs
    /// all receive the same RSI series).
    pub srcs: Vec<String>,
    /// Destination series per callee output, in callee signature order: an
    /// in-place source, a caller output, or a FRESH intermediate series.
    pub dsts: Vec<String>,
}

/// One per-bar step of a composed Update pipeline, in tail order.
#[derive(Debug, Clone)]
pub enum UpdateStep {
    /// Feed the sources' current scalars through sub handle `sub_idx`.
    Sub { sub_idx: usize },
    /// Tail-aligning copy (`memmove(dst, &src[k], nb)`): the current scalars
    /// coincide, `dst`'s becomes `src`'s.
    Align { dst: String, src: String },
    /// A per-bar combine map (a series-map loop, possibly under a
    /// param-selected variant If): the emitter transforms series reads and
    /// writes into current scalars, drops the loop shells and cursors, and
    /// reads params through the handle. `tail_idx` names the statement.
    Map { tail_idx: usize },
}

/// A non-returning free of an intermediate series in the tail: the
/// series' liveness boundary AND the statement inserted failure returns
/// replay (frees inside returning guards never affect fall-through
/// liveness and are transcribed verbatim anyway).
#[derive(Debug, Clone)]
pub struct SeriesFree {
    pub tail_idx: usize,
    pub stmt: Statement,
}

/// A recognized composed body (STOCH/STOCHF class): a steady producer loop
/// materializing an intermediate series, then a tail of whole-range
/// sub-calls over materialized series + tail-aligning copies + guards/frees.
/// Composition goes through the callees' PUBLIC stream handles; peek uses a
/// `peekMode` flag in the scratch state copy so the ONE step body calls
/// sub-Peek instead of sub-Update (heap sub-handles cannot be cloned by a
/// struct copy).
#[derive(Debug)]
pub struct ComposedPlan<'a> {
    pub func: &'a FuncDef,
    /// Loop model of the producer region (`body[..=loop]`), with the
    /// intermediate series as its output. None for loopless pipelines
    /// (STDDEV, STOCHRSI): the body is prologue + sub-call tail only.
    pub producer: Option<StreamModel<'a>>,
    /// The intermediate series the producer loop writes (when present).
    pub series: Option<String>,
    /// Every intermediate series in creation order (the producer's, then
    /// fresh sub-call destinations). Outputs are not listed.
    pub intermediates: Vec<String>,
    /// Sub-calls in tail order.
    pub subs: Vec<SubCallStep>,
    /// The per-bar pipeline in tail order (subs/aligns/maps interleaved).
    pub steps: Vec<UpdateStep>,
    /// The tail statements (everything after the producer loop, or after
    /// the prologue for loopless pipelines), for the Open transcription.
    pub tail: &'a [Statement],
    /// The region BEFORE the tail (`body[..tail_start]`): the producer loop
    /// and its setup for the producer case, or the pure-scalar prologue for
    /// loopless pipelines. Open transcribes this verbatim, then the tail.
    /// (For the producer case this equals `producer.body`.) Owned because any
    /// leading parameter-guarded fast-path block ([`is_fastpath_block`]) is
    /// filtered out here — the stream composes the general path, not the
    /// specialization.
    pub region: Vec<Statement>,
    /// Non-returning frees of intermediate series (`if (bufferIsAllocated)
    /// free(tempBuffer)` or a bare `free(buf)`), replayed on the emitter's
    /// inserted sub-open failure returns for every series still live there
    /// — the batch's own early returns free the buffers themselves, but an
    /// inserted return must do it explicitly or every honest Open rejection
    /// leaks the series (LeakSanitizer found exactly this class).
    pub series_frees: Vec<SeriesFree>,
    /// Function-local temps referenced by Map steps (step-local decls).
    pub map_temps: Vec<(String, VarType)>,
    /// Sub-output self-lag rings a combine map reads (ADXR's ADX lag). Empty
    /// for the same-bar-only combines (APO/PPO/STDDEV).
    pub sub_lag_rings: Vec<SubLagRing>,
}

/// The derived stream implementation plan for one function: a steady-loop
/// transition model, a dispatch over other functions' public streams, or a
/// composed producer-plus-pipeline over public sub-streams.
// One short-lived plan exists per generated function; the size skew between
// the variants is irrelevant, and boxing would only add deref noise at
// every emitter call site.
#[allow(clippy::large_enum_variant)]
#[derive(Debug)]
pub enum StreamPlan<'a> {
    Loop(StreamModel<'a>),
    Dispatch(DispatchPlan<'a>),
    Composed(ComposedPlan<'a>),
    DualMode(DualModePlan<'a>),
    FastPathSkip(FastPathSkipPlan<'a>),
    PeriodBank(PeriodBankPlan<'a>),
}

/// One optional argument of a period-bank sub-open, in the callee's signature
/// order: either the per-bar variable period (fixed to a distinct value in each
/// bank slot) or the caller's forwarded MAType.
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum PeriodBankArg {
    /// The variable period — bank slot `k` opens the callee at `minPeriod + k`.
    Period,
    /// The caller's MAType param, forwarded to every slot.
    MAType,
}

/// A recognized variable-period moving average (MAVP): a moving average whose
/// period varies PER BAR, read from a second input and clamped to
/// `[minPeriod, maxPeriod]`. The batch computes each distinct period's full MA
/// once and scatters; a stream cannot know future periods, so it maintains a
/// BANK of `maxPeriod - minPeriod + 1` streaming sub-MAs (one per possible
/// period), advances them all in lockstep every bar, and outputs the one the
/// current bar's clamped period selects. Reuses the callee's (`ma`) public
/// stream — so it streams exactly the MATypes the callee streams (MAType_MAMA
/// rejects at Open, as it does through MA's dispatch).
#[derive(Debug)]
pub struct PeriodBankPlan<'a> {
    pub func: &'a FuncDef,
    /// The per-period moving-average callee (input-level name, e.g. `ma`).
    pub callee: String,
    /// The callee's optional arguments, in signature order (period slot + the
    /// forwarded MAType).
    pub callee_opts: Vec<PeriodBankArg>,
    /// Input fed to every sub-MA (the price series).
    pub price_input: String,
    /// Input read per bar as the (clamped) period selector.
    pub period_input: String,
    /// Optional-param names: the inclusive period-bank bounds.
    pub min_param: String,
    pub max_param: String,
    /// The MAType optional-param name forwarded to every slot.
    pub matype_param: String,
    /// The single real output.
    pub output: String,
}

/// A recognized param fast-path split whose two arms are bit-identical (a pure
/// batch perf optimization): `<prologue> if (<param> <= <literal>) { fast-path }
/// else { general } <epilogue>` (MIDPRICE rescans a short window but caches the
/// running extremum for long periods; both paths produce identical output).
/// ONLY the general (else) arm is streamed, for EVERY param — the fast-path
/// `then` arm is a batch-only specialization skipped by the stream, and the
/// stream_verify gate enforces bit-exactness across the threshold. The `<=
/// literal` threshold predicate distinguishes this from a genuine dual-mode
/// branch (TRIMA's `% 2`, whose arms differ and must both be streamed).
#[derive(Debug)]
pub struct FastPathSkipPlan<'a> {
    pub func: &'a FuncDef,
    /// The shared prologue (`body[..if_idx]`).
    pub prologue: &'a [Statement],
    /// The general (else) arm's stream model (`model.body` is the else slice).
    pub model: StreamModel<'a>,
    /// The shared epilogue (`body[if_idx+1..]`): out-meta writes + final return,
    /// transcribed after the general arm.
    pub epilogue: &'a [Statement],
}

/// A recognized param-selected dual-mode body: a shared prologue, then a
/// leading `if (<param predicate>) { <mode-A steady loop>; return SUCCESS; }`
/// arm followed by a fall-through `<mode-B steady loop>` general path
/// (DI/DM: `optInTimePeriod <= 1` selects the raw single-period arm, which
/// deliberately ignores the unstable period, over the Wilder-smoothed general
/// path). Each mode is an independent [`StreamModel`]; the predicate is
/// evaluated once at Open (params only, so fixed for a stream's lifetime) and
/// the step re-selects the mode from the handle's stored param. The two arms'
/// state sets are unioned into one handle; the input `.c` is UNTOUCHED (both
/// arms are transcribed verbatim, so the mode-A quirks — DI's raw ratio, the
/// unstable-period-independent lookback of 1 — are preserved by construction).
#[derive(Debug)]
pub struct DualModePlan<'a> {
    pub func: &'a FuncDef,
    /// The arm predicate, params only (e.g. `optInTimePeriod <= 1`). True
    /// selects mode A; false falls through to mode B.
    pub predicate: Expr,
    /// The shared prologue (`body[..arm_idx]`): lookback computation, the
    /// `startIdx` clamp, the no-data guard, output-index reset. Transcribed
    /// ahead of the selected mode's body in Open (both arms need it).
    pub prologue: &'a [Statement],
    /// Mode A: the `if`-then arm (its `body` is the arm's then-body slice).
    pub mode_a: StreamModel<'a>,
    /// Mode B: the fall-through general path (early-return form) or the `else`
    /// arm (if/else form). Its `body` is the corresponding slice.
    pub mode_b: StreamModel<'a>,
    /// Shared epilogue transcribed after the selected arm. Empty for the
    /// early-return form (DI/DM — mode A returns, mode B is the general tail).
    /// For the if/else form (TRIMA `period % 2`) both arms fall through to this
    /// out-meta + return tail (`body[arm_idx+1..]`).
    pub epilogue: &'a [Statement],
}

/// Syntactic form of the steady-state loop (informational; open() transcribes
/// the whole body verbatim, only update-body extraction depends on it).
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum LoopForm {
    While,
    DoWhile,
    ForC,
    /// `while (counter != 0)`-style loop whose iteration count derives from
    /// `endIdx`; the input cursor is a separate variable.
    Countdown,
}

/// The steady-state kind — the ONE exclusivity the analyzer proves
/// fundamental. `analyze_region` clears any trailing rings when an extrema
/// automaton is present (the `rings_effective` fold) and `build_extrema`
/// refuses to run when windows/circs/counter exist, so a model is never both
/// an ordinary batch-walker and an absolute-index automaton. Encoding that as
/// an enum makes the illegal combination unrepresentable (it deletes only
/// states the analyzer never constructs — no reachable flexibility is lost).
#[derive(Debug)]
pub enum Steady {
    /// Ordinary batch-walking steady state (T1/T2/T3): the cursor is a plain
    /// array walker, dropped from the transition. All-empty/None == a pure
    /// scalar T1/T2 recurrence.
    ///
    /// INVARIANT — do NOT split into finer variants. `rings`, `windows`, and
    /// `circs` are only *incidentally* exclusive: they come from independent
    /// analyzer passes (`assemble_rings` / `assemble_windows` / `discover_circs`)
    /// and can legally co-occur (a trailing ring alongside a rescan window is
    /// representable). Separate arms would silently drop those reachable states.
    Batch {
        /// Trailing-window rings (T3), one per trailing index variable.
        rings: Vec<RingSpec>,
        /// Bounded rescan windows (T3), one per inner-loop counter variable.
        windows: Vec<WindowSpec>,
        /// CIRCBUF-backed buffers referenced by the steady loop.
        circs: Vec<CircState>,
        /// Countdown loops only: the iteration-count variable (dropped).
        counter: Option<String>,
    },
    /// Absolute-index automaton (T4): the cursor is carried state (not dropped)
    /// and inputs are read through the automaton ring. By construction excludes
    /// every ring/window/circ/counter.
    Extrema(ExtremaState),
}

/// Everything the stream emitters need, derived from one [`FuncDef`].
#[derive(Debug)]
pub struct StreamModel<'a> {
    pub func: &'a FuncDef,
    /// The body `open()` transcribes: guarded body, or the private body when
    /// the function has an explicit `_private` variant.
    pub body: &'a [Statement],
    /// Derived tier (must match the YAML declaration). Kept cached (read by the
    /// census + declared-tier gate); the constructor debug-asserts it against
    /// [`derive_tier`] so it can never drift from the data.
    pub tier: StreamTier,
    pub loop_form: LoopForm,
    /// Statements of the per-bar transition, in batch order (loop body plus,
    /// for `for` loops, the increment clause).
    pub steady_stmts: Vec<Statement>,
    /// Variable indexing the inputs at the current bar.
    pub cursor: String,
    /// True when a guarded mid-body success return follows an output write
    /// (Metastock seed boundary): Open rejects at exactly lookback+1 there,
    /// so verification shifts its boundary leg by one bar.
    pub seed_boundary: bool,
    /// Output-index variables (dropped in the transition).
    pub out_index_vars: BTreeSet<String>,
    /// Outputs whose PREVIOUS value the steady loop reads (`out[idx-1]`,
    /// DX's zero-denominator repeat) — carried as `lastOut_<name>` state.
    pub out_feedback: Vec<String>,
    /// Scalar input parameters of `update`, one per input component, in batch
    /// signature order (e.g. `[inHigh, inLow, inClose]` for TRANGE).
    pub bar_inputs: Vec<String>,
    /// Output array names in batch signature order.
    pub outputs: Vec<String>,
    /// Loop-carried scalars: state-struct fields, in declaration order.
    pub state: Vec<(String, VarType)>,
    /// Written-before-read scalars: plain locals of the transition function.
    pub temps: Vec<(String, VarType)>,
    /// Bounded look-back lags, in input signature order. Flat, NOT part of
    /// [`Steady`]: lags co-occur with every steady kind (`lag_slots` is built
    /// unconditionally and never zeroed under extrema), so folding them into a
    /// variant would lose the `extrema + lags` state.
    pub lags: Vec<LagSlot>,
    /// The fundamentally-exclusive steady-state kind (batch-walker vs automaton).
    pub steady: Steady,
    /// Recognized param==1 identity path, if the batch body has one.
    pub identity: Option<IdentityPath>,
    /// Cursor-parity carry (`cursor % 2` odd/even branch), if the batch body
    /// has one. Seeded in open, flipped each update; see [`ParitySpec`].
    pub parity: Option<ParitySpec>,
}

impl StreamModel<'_> {
    /// Trailing rings (empty for the extrema automaton — it reads through its
    /// own ring). Byte-identical to the old `rings` field, which
    /// `analyze_region` force-emptied under extrema.
    #[must_use]
    pub fn rings(&self) -> &[RingSpec] {
        match &self.steady {
            Steady::Batch { rings, .. } => rings,
            Steady::Extrema(_) => &[],
        }
    }

    /// Rescan windows (empty under extrema by construction).
    #[must_use]
    pub fn windows(&self) -> &[WindowSpec] {
        match &self.steady {
            Steady::Batch { windows, .. } => windows,
            Steady::Extrema(_) => &[],
        }
    }

    /// CIRCBUF-backed buffers (empty under extrema by construction).
    #[must_use]
    pub fn circs(&self) -> &[CircState] {
        match &self.steady {
            Steady::Batch { circs, .. } => circs,
            Steady::Extrema(_) => &[],
        }
    }

    /// The countdown iteration-count variable (None under extrema).
    #[must_use]
    pub fn counter(&self) -> Option<&str> {
        match &self.steady {
            Steady::Batch { counter, .. } => counter.as_deref(),
            Steady::Extrema(_) => None,
        }
    }

    /// The absolute-index automaton, if this is the extrema kind.
    #[must_use]
    pub fn extrema(&self) -> Option<&ExtremaState> {
        match &self.steady {
            Steady::Extrema(e) => Some(e),
            Steady::Batch { .. } => None,
        }
    }

    /// True iff the model owns any heap buffer and therefore needs a
    /// `TA_<N>_ReleaseInternal` (per-buffer `TA_Free`) instead of a plain free.
    /// Replaces the OR-chains previously duplicated across the emitter.
    #[must_use]
    pub fn needs_release(&self) -> bool {
        match &self.steady {
            Steady::Extrema(_) => true,
            Steady::Batch {
                rings,
                windows,
                circs,
                ..
            } => !(rings.is_empty() && windows.is_empty() && circs.is_empty()),
        }
    }

    /// Variables that exist only to walk the batch arrays and are dropped
    /// from the transition (their bookkeeping statements are deleted).
    #[must_use]
    pub fn dropped_vars(&self) -> BTreeSet<String> {
        let mut d: BTreeSet<String> = self.out_index_vars.clone();
        match &self.steady {
            Steady::Batch {
                rings, counter, ..
            } => {
                // Ordinary walkers drop the cursor; extrema carries it.
                d.insert(self.cursor.clone());
                if let Some(c) = counter {
                    d.insert(c.clone());
                }
                for r in rings {
                    d.insert(r.var.clone());
                }
            }
            Steady::Extrema(_) => {}
        }
        d
    }

    /// Name of the lag field holding `array`'s value from `k` bars ago.
    #[must_use]
    pub fn lag_field(array: &str, k: i64) -> String {
        format!("lag{k}_{array}")
    }
}

// ---------------------------------------------------------------------------
// IR walking helpers
// ---------------------------------------------------------------------------

/// Visit every expression held by a statement tree (conditions, initializers,
/// targets, values), recursing into nested statements.
pub fn walk_stmt_exprs(s: &Statement, f: &mut dyn FnMut(&Expr)) {
    match s {
        Statement::VarDecl { init, .. } => {
            if let Some(e) = init {
                f(e);
            }
        }
        Statement::Assign { target, value, .. } => {
            f(target);
            f(value);
        }
        Statement::While { condition, body } | Statement::DoWhile { condition, body } => {
            f(condition);
            for st in body {
                walk_stmt_exprs(st, f);
            }
        }
        Statement::For { count, body, .. } => {
            f(count);
            for st in body {
                walk_stmt_exprs(st, f);
            }
        }
        Statement::If {
            condition,
            then_body,
            else_body,
            ..
        } => {
            f(condition);
            for st in then_body.iter().chain(else_body) {
                walk_stmt_exprs(st, f);
            }
        }
        Statement::Return { value } => {
            if let Some(e) = value {
                f(e);
            }
        }
        Statement::Switch {
            expr,
            cases,
            default,
        } => {
            f(expr);
            for (_, sts) in cases {
                for st in sts {
                    walk_stmt_exprs(st, f);
                }
            }
            for st in default {
                walk_stmt_exprs(st, f);
            }
        }
        Statement::ForC {
            init,
            condition,
            update,
            body,
        } => {
            walk_stmt_exprs(init, f);
            f(condition);
            walk_stmt_exprs(update, f);
            for st in body {
                walk_stmt_exprs(st, f);
            }
        }
        Statement::Block { body } => {
            for st in body {
                walk_stmt_exprs(st, f);
            }
        }
        Statement::Expr(e) => f(e),
        Statement::CircBuf(_) | Statement::Comment(_) | Statement::Break | Statement::Continue => {}
    }
}

/// Visit every sub-expression of `e`, including `e` itself.
pub fn walk_expr(e: &Expr, f: &mut dyn FnMut(&Expr)) {
    f(e);
    match e {
        Expr::ArrayAccess(_, i)
        | Expr::Cast(_, i)
        | Expr::Not(i)
        | Expr::AddressOf(i)
        | Expr::PostIncrement(i)
        | Expr::PostDecrement(i)
        | Expr::PreIncrement(i)
        | Expr::PreDecrement(i) => walk_expr(i, f),
        Expr::BinOp(l, _, r) => {
            walk_expr(l, f);
            walk_expr(r, f);
        }
        Expr::FuncCall(_, args) => {
            for a in args {
                walk_expr(a, f);
            }
        }
        Expr::Ternary(c, t, e2) => {
            walk_expr(c, f);
            walk_expr(t, f);
            walk_expr(e2, f);
        }
        Expr::Literal(_)
        | Expr::IntLiteral(_)
        | Expr::Var(_)
        | Expr::PointerDeref(_) => {}
    }
}

fn expr_var_names(e: &Expr, out: &mut BTreeSet<String>) {
    walk_expr(e, &mut |x| match x {
        Expr::Var(n) | Expr::PointerDeref(n) | Expr::ArrayAccess(n, _) => {
            out.insert(n.clone());
        }
        _ => {}
    });
}

pub fn stmt_var_names(s: &Statement, out: &mut BTreeSet<String>) {
    walk_stmt_exprs(s, &mut |e| expr_var_names(e, out));
}

fn expr_mentions(e: &Expr, name: &str) -> bool {
    let mut found = false;
    walk_expr(e, &mut |x| {
        if let Expr::Var(n) = x {
            if n == name {
                found = true;
            }
        }
    });
    found
}

/// Collect `VarDecl`s (name -> type) from a statement tree.
fn collect_var_decls(stmts: &[Statement], out: &mut BTreeMap<String, VarType>) {
    for s in stmts {
        match s {
            Statement::VarDecl { var_type, name, .. } => {
                out.insert(name.clone(), var_type.clone());
            }
            Statement::While { body, .. }
            | Statement::DoWhile { body, .. }
            | Statement::For { body, .. }
            | Statement::Block { body } => collect_var_decls(body, out),
            Statement::If {
                then_body,
                else_body,
                ..
            } => {
                collect_var_decls(then_body, out);
                collect_var_decls(else_body, out);
            }
            Statement::Switch { cases, default, .. } => {
                for (_, sts) in cases {
                    collect_var_decls(sts, out);
                }
                collect_var_decls(default, out);
            }
            Statement::ForC { init, body, .. } => {
                collect_var_decls(std::slice::from_ref(init), out);
                collect_var_decls(body, out);
            }
            _ => {}
        }
    }
}

/// Input array names of a FuncDef in signature order. Price bundles expand to
/// their per-component arrays (`inOpen`, `inHigh`, ...).
#[must_use]
pub fn input_array_names(func: &FuncDef) -> Vec<String> {
    let mut out = Vec::new();
    for i in &func.inputs {
        match &i.param_type {
            ParamType::Price(comps) => {
                for c in comps {
                    let mut n = c.clone();
                    if let Some(first) = n.get_mut(0..1) {
                        first.make_ascii_uppercase();
                    }
                    out.push(format!("in{n}"));
                }
            }
            _ => out.push(i.name.clone()),
        }
    }
    out
}

// ---------------------------------------------------------------------------
// Steady-loop identification
// ---------------------------------------------------------------------------

/// `open()` transcribes the whole batch body, so every return path must be
/// one it can map faithfully:
/// - the FINAL top-level return (success; falls through to state capture);
/// - the no-data guard `if (startIdx > endIdx) ... return TA_SUCCESS`
///   (mapped to TA_BAD_PARAM: not enough history for a value);
/// - `return <retCodeVar>` error propagation (mapped verbatim).
///
/// Anything else — an early success path that produces outputs (T3's
/// period==1 identity loop) or a delegating `return other_func(...)`
/// (ATR/NATR period<=1 → TRANGE) — computes values the transition function
/// cannot reproduce, so the function is rejected until it gets explicit
/// stream support (the "open-time delegation" class in the proposal).
/// Recognize a top-level param==1 identity path (see [`IdentityPath`]).
/// Returns the path and the index of its top-level `If` statement.
fn detect_identity_path(
    body: &[Statement],
    inputs: &[String],
    outputs: &[String],
    params: &[String],
) -> (Option<IdentityPath>, Option<usize>) {
    for (i, s) in body.iter().enumerate() {
        let Statement::If {
            condition,
            then_body,
            else_body,
            ..
        } = s
        else {
            continue;
        };
        if !else_body.is_empty() {
            continue;
        }
        // Guard: <param> == 1 or <param> <= 1.
        let is_param_one_guard = matches!(
            condition,
            Expr::BinOp(l, BinOp::Eq | BinOp::LessEq, r)
                if matches!(l.as_ref(), Expr::Var(v) if params.contains(v))
                    && matches!(r.as_ref(), Expr::IntLiteral(1))
        );
        if !is_param_one_guard {
            continue;
        }
        // Body must end with `return SUCCESS` and contain exactly one loop
        // whose output writes are pure `out[..] = in[..]` copies.
        let ends_in_success = matches!(
            then_body.last(),
            Some(Statement::Return { value: Some(Expr::Var(v)) })
                if matches!(v.as_str(), "SUCCESS" | "TA_SUCCESS")
        );
        if !ends_in_success {
            continue;
        }
        let mut pairs: Vec<(String, String)> = Vec::new();
        let mut clean = true;
        for st in then_body {
            walk_stmt_exprs(st, &mut |_| {});
            // Whole-range block copy: memmove(&out[0], &in[startIdx], n) is
            // the same identity as the element loop (RSI/CMO/WMA period 1).
            if let Statement::Expr(Expr::FuncCall(fname, args)) = st {
                if fname == "memmove" && args.len() == 3 {
                    match identity_memmove_pair(args, inputs, outputs) {
                        Some(pair) => pairs.push(pair),
                        None => clean = false,
                    }
                }
            }
            if let Statement::While { body: lb, .. }
            | Statement::DoWhile { body: lb, .. }
            | Statement::ForC { body: lb, .. } = st
            {
                for ls in lb {
                    if let Statement::Assign {
                        target: Expr::ArrayAccess(out_name, out_idx),
                        value,
                        ..
                    } = ls
                    {
                        if !outputs.contains(out_name) {
                            continue;
                        }
                        // Copy source: in[..], possibly under a cast.
                        let src = match value {
                            Expr::Cast(_, inner) => inner.as_ref(),
                            other => other,
                        };
                        // Both indices must be plain cursors (var or var++):
                        // an arithmetic index (in[i-1]) is a SHIFTED copy,
                        // not an identity, and must not match.
                        let plain = |e: &Expr| {
                            matches!(e, Expr::Var(_))
                                || matches!(e, Expr::PostIncrement(b)
                                    if matches!(b.as_ref(), Expr::Var(_)))
                        };
                        match src {
                            Expr::ArrayAccess(in_name, in_idx)
                                if inputs.contains(in_name)
                                    && plain(in_idx)
                                    && plain(out_idx) =>
                            {
                                pairs.push((out_name.clone(), in_name.clone()));
                            }
                            _ => clean = false,
                        }
                    }
                }
            }
        }
        // One copy per output, no stray shapes.
        if clean
            && pairs.len() == outputs.len()
            && outputs.iter().all(|o| pairs.iter().any(|(po, _)| po == o))
        {
            return (
                Some(IdentityPath {
                    condition: condition.clone(),
                    pairs,
                }),
                Some(i),
            );
        }
    }
    (None, None)
}

/// Validates all return paths and reports whether any guarded success return
/// occurs AFTER an output write (a "seed boundary", e.g. RSI/CMO under
/// Metastock). Such an exit carries state the batch would rewind and rebuild
/// before continuing, so Open rejects there (min-history is one bar stricter)
/// and the verify harness shifts its boundary leg accordingly.
/// Match `memmove(&out[0] | out, &in[startIdx], n)` as an identity copy.
fn identity_memmove_pair(
    args: &[Expr],
    inputs: &[String],
    outputs: &[String],
) -> Option<(String, String)> {
    let dst_out = match &args[0] {
        Expr::AddressOf(d) => match d.as_ref() {
            Expr::ArrayAccess(out_name, out_idx)
                if matches!(out_idx.as_ref(), Expr::IntLiteral(0)) =>
            {
                Some(out_name.clone())
            }
            _ => None,
        },
        Expr::Var(out_name) => Some(out_name.clone()),
        _ => None,
    };
    let src_in = match &args[1] {
        Expr::AddressOf(sr) => match sr.as_ref() {
            Expr::ArrayAccess(in_name, in_idx)
                if matches!(in_idx.as_ref(), Expr::Var(v) if v == "startIdx") =>
            {
                Some(in_name.clone())
            }
            _ => None,
        },
        _ => None,
    };
    match (dst_out, src_in) {
        (Some(o), Some(i2)) if outputs.contains(&o) && inputs.contains(&i2) => Some((o, i2)),
        _ => None,
    }
}

fn check_return_paths(body: &[Statement], skip_idx: Option<usize>) -> Result<bool, StreamError> {
    fn is_no_data_guard(cond: &Expr) -> bool {
        // `startIdx > endIdx`, or the post-clamp cursor form `<var> > endIdx`
        // (AVGDEV clamps startIdx into a local first). Either way a taken
        // guard means the batch emits zero outputs for the full range, so
        // Open maps its success return to BAD_PARAM (no last value exists).
        match cond {
            Expr::BinOp(l, BinOp::Greater, r) => {
                matches!(l.as_ref(), Expr::Var(_))
                    && matches!(r.as_ref(), Expr::Var(v) if v == "endIdx")
            }
            Expr::BinOp(l, BinOp::Less, r) => {
                matches!(l.as_ref(), Expr::Var(v) if v == "endIdx")
                    && matches!(r.as_ref(), Expr::Var(_))
            }
            _ => false,
        }
    }
    fn ret_ok(value: Option<&Expr>, in_no_data_guard: bool) -> bool {
        match value {
            // Error propagation via a ret-code variable is always mappable;
            // a bare success return only inside the no-data guard. (The
            // parser stores ret codes unprefixed: `SUCCESS`, not TA_SUCCESS.)
            Some(Expr::Var(v)) => !matches!(v.as_str(), "SUCCESS" | "TA_SUCCESS") || in_no_data_guard,
            _ => false,
        }
    }
    struct WalkState {
        /// An output-array write was seen before the current point.
        wrote_output: bool,
        /// A guarded success return was seen after an output write.
        seed_boundary: bool,
    }
    fn saw_output_write(s: &Statement, st: &mut WalkState) {
        if st.wrote_output {
            return;
        }
        walk_stmt_exprs(s, &mut |e| {
            if let Expr::ArrayAccess(n, _) = e {
                if n.starts_with("out") {
                    st.wrote_output = true;
                }
            }
        });
    }
    fn walk(
        stmts: &[Statement],
        in_guard: bool,
        is_top: bool,
        skip_idx: Option<usize>,
        st: &mut WalkState,
    ) -> Result<(), StreamError> {
        for (i, s) in stmts.iter().enumerate() {
            if is_top && Some(i) == skip_idx {
                continue; // recognized identity path, handled separately
            }
            let last_top = is_top && i == stmts.len() - 1;
            match s {
                Statement::Return { value } => {
                    if !last_top && !ret_ok(value.as_ref(), in_guard) {
                        return Err(StreamError::Unsupported(
                            "early success/delegating return path (param-degenerate); \
                             needs explicit stream support"
                                .into(),
                        ));
                    }
                    if !last_top && in_guard && st.wrote_output {
                        st.seed_boundary = true;
                    }
                }
                Statement::If {
                    condition,
                    then_body,
                    else_body,
                    ..
                } => {
                    let guard = in_guard || is_no_data_guard(condition);
                    walk(then_body, guard, false, None, st)?;
                    walk(else_body, in_guard, false, None, st)?;
                }
                Statement::While { body, .. }
                | Statement::DoWhile { body, .. }
                | Statement::For { body, .. }
                | Statement::Block { body } => walk(body, in_guard, false, None, st)?,
                Statement::Switch { cases, default, .. } => {
                    for (_, sts) in cases {
                        walk(sts, in_guard, false, None, st)?;
                    }
                    walk(default, in_guard, false, None, st)?;
                }
                Statement::ForC { init, update, body, .. } => {
                    walk(std::slice::from_ref(init), in_guard, false, None, st)?;
                    walk(std::slice::from_ref(update), in_guard, false, None, st)?;
                    walk(body, in_guard, false, None, st)?;
                }
                _ => {}
            }
            saw_output_write(s, st);
        }
        Ok(())
    }
    let mut st = WalkState {
        wrote_output: false,
        seed_boundary: false,
    };
    walk(body, false, true, skip_idx, &mut st)?;
    Ok(st.seed_boundary)
}

struct SteadyLoop<'a> {
    form: LoopForm,
    condition: &'a Expr,
    body: &'a [Statement],
    /// `for` loops: the increment clause, appended to the transition.
    for_update: Option<&'a Statement>,
}

/// `cond` is `var <= endIdx` / `var < endIdx` (endIdx on the right).
fn endidx_cursor(cond: &Expr) -> Option<String> {
    if let Expr::BinOp(l, BinOp::LessEq | BinOp::Less, r) = cond {
        if expr_mentions(r, "endIdx") {
            if let Expr::Var(v) = l.as_ref() {
                return Some(v.clone());
            }
        }
    }
    None
}

/// `cond` is a countdown: `var != 0` or `var > 0` (with optional pre/post
/// decrement on the var).
fn countdown_counter(cond: &Expr) -> Option<String> {
    if let Expr::BinOp(l, BinOp::NotEq | BinOp::Greater, r) = cond {
        if !matches!(r.as_ref(), Expr::IntLiteral(0)) {
            return None;
        }
        return match l.as_ref() {
            Expr::Var(v) => Some(v.clone()),
            Expr::PostDecrement(b) | Expr::PreDecrement(b) => match b.as_ref() {
                Expr::Var(v) => Some(v.clone()),
                _ => None,
            },
            _ => None,
        };
    }
    None
}

fn find_steady_loop(body: &[Statement]) -> Result<SteadyLoop<'_>, StreamError> {
    // Prefer the LAST top-level loop over endIdx; fall back to the last
    // countdown loop (AD-style `while (nbBar != 0)`).
    let is_endidx = |s: &Statement| match s {
        Statement::While { condition, .. }
        | Statement::DoWhile { condition, .. }
        | Statement::ForC { condition, .. } => endidx_cursor(condition).is_some(),
        _ => false,
    };
    let is_countdown = |s: &Statement| match s {
        Statement::While { condition, .. } | Statement::DoWhile { condition, .. } => {
            countdown_counter(condition).is_some()
        }
        _ => false,
    };
    let idx = body
        .iter()
        .rposition(is_endidx)
        .or_else(|| body.iter().rposition(is_countdown))
        .ok_or(StreamError::NoSteadyLoop)?;

    Ok(match &body[idx] {
        Statement::While { condition, body } => SteadyLoop {
            form: if endidx_cursor(condition).is_some() {
                LoopForm::While
            } else {
                LoopForm::Countdown
            },
            condition,
            body,
            for_update: None,
        },
        Statement::DoWhile { condition, body } => SteadyLoop {
            form: if endidx_cursor(condition).is_some() {
                LoopForm::DoWhile
            } else {
                LoopForm::Countdown
            },
            condition,
            body,
            for_update: None,
        },
        Statement::ForC {
            condition,
            update,
            body,
            ..
        } => SteadyLoop {
            form: LoopForm::ForC,
            condition,
            body,
            for_update: Some(update),
        },
        _ => unreachable!(),
    })
}

// ---------------------------------------------------------------------------
// Analysis
// ---------------------------------------------------------------------------

/// Calls that read process/library state or delegate to other indicators —
/// never legal inside a transition body.
fn is_stateful_call(name: &str) -> bool {
    let lower = name.to_ascii_lowercase();
    // NOTE: the candle helpers (ta_candleaverage, ta_candlecolor, ...) are
    // deliberately NOT here — they are pure scalar functions whose candle
    // settings arrive as hoisted-local arguments (the settings-stability
    // rule: streams read settings exactly where batch reads them).
    lower.contains("lookback")
        || lower.contains("compatibility")
        || lower.contains("unstable")
        || lower.starts_with("array_")
        || lower.starts_with("circbuf")
        || name == "TA_Malloc"
        || name == "TA_Free"
}

/// Analyze one function's batch IR into a [`StreamModel`].
///
/// Errors classify *why* the function is outside stage 1 (ring needed,
/// composed body, non-scalar state, ...), which drives both the YAML
/// validation and the census.
pub fn analyze(func: &FuncDef) -> Result<StreamModel<'_>, StreamError> {
    let body: &[Statement] = if func.has_explicit_private {
        &func.private_body
    } else {
        &func.body
    };
    let outputs: Vec<String> = func.outputs.iter().map(|o| o.name.clone()).collect();
    for o in &func.outputs {
        if !matches!(o.param_type, ParamType::Real | ParamType::Integer) {
            return Err(StreamError::Unsupported(format!(
                "unsupported output type for `{}`",
                o.name
            )));
        }
    }
    analyze_region(func, body, outputs)
}

/// [`analyze`] over an explicit body region with an outputs override. The
/// composed tier analyzes its PRODUCER region this way: the statements up to
/// and including the steady loop, with the intermediate series local
/// (STOCH's `tempBuffer`) standing in as the output the loop writes.
pub fn analyze_region<'a>(
    func: &'a FuncDef,
    body: &'a [Statement],
    outputs: Vec<String>,
) -> Result<StreamModel<'a>, StreamError> {
    analyze_region_scoped(func, body, body, outputs)
}

/// [`analyze_region`] with the declaration scope decoupled from the scanned
/// region. `body` is the region scanned for the steady loop, identity path,
/// and return paths; `decl_scope` is where local declarations and CIRCBUF
/// prologs are resolved (`classify_locals` / `discover_circs`). The dual-mode
/// analyzer scans one arm at a time over a NESTED slice of the function body
/// (the `if` then-body, or the general path after it) whose scalars are
/// declared at the FUNCTION top, so it passes the full body as `decl_scope`.
/// Every other caller passes `decl_scope == body` (a `body[..]` prefix already
/// carries every decl), preserving byte-identical output.
#[allow(clippy::too_many_lines)]
pub fn analyze_region_scoped<'a>(
    func: &'a FuncDef,
    body: &'a [Statement],
    decl_scope: &'a [Statement],
    outputs: Vec<String>,
) -> Result<StreamModel<'a>, StreamError> {
    if body.is_empty() {
        return Err(StreamError::Unsupported("empty body".into()));
    }

    let bar_inputs = input_array_names(func);
    let param_name_list: Vec<String> =
        func.optional_inputs.iter().map(|p| p.name.clone()).collect();
    let (identity, identity_idx) =
        detect_identity_path(body, &bar_inputs, &outputs, &param_name_list);
    let seed_boundary = check_return_paths(body, identity_idx)?;

    let (loop_form, steady_stmts, cursor, counter) = extract_steady(body, &bar_inputs)?;
    // Normalize `in[cond ? a : b]` into `cond ? in[a] : in[b]` so index
    // classification sees plain index forms (CDLKICKINGBYLENGTH). Pure
    // expressions only; semantics and FP behavior are identical.
    let steady_stmts = hoist_ternary_indices(&steady_stmts);
    // Normalize cursor-anchored ascending windows
    // `for (i = cursor - E; i <= cursor; i++)` into the descending counter
    // form `for (w = E; w >= 0; w--)` with `i` := `cursor - w` (IMI) — the
    // reads become in[cursor - w], the standard rescan-window shape.
    let steady_stmts = reindex_cursor_windows(&steady_stmts, &cursor);
    // Two more general steady-loop normalizations that decouple the transition
    // from the absolute cursor index (bit-preserving; each a no-op unless its
    // shape is present). First drop a warm-up output gate, then carry any
    // `cursor % 2` branch as state. (HT_DCPERIOD is the first consumer of both.)
    let steady_stmts = strip_cursor_output_gate(steady_stmts, &cursor);
    let (steady_stmts, parity) = carry_cursor_parity(steady_stmts, &cursor);

    // --- array accesses ----------------------------------------------------

    let scanned = scan_accesses(&steady_stmts, &bar_inputs, &outputs, &cursor)?;
    let ScannedAccesses {
        lags,
        trailing,
        ring_back,
        ring_fwd,
        windows: scanned_windows,
        out_index_vars,
    } = scanned;
    let ring_vars: BTreeSet<String> = trailing.keys().cloned().collect();
    check_window_disjoint(&scanned_windows, &ring_vars, &cursor)?;
    let rings = assemble_rings(trailing.clone(), &ring_back, &ring_fwd, &bar_inputs);
    let windows = assemble_windows(scanned_windows, &bar_inputs);

    let out_feedback = collect_out_feedback(&steady_stmts, &outputs);
    check_no_output_read_back(&steady_stmts, &outputs)?;

    // --- locals ------------------------------------------------------------
    // Local decls / CIRCBUF prologs resolve in `decl_scope` (the full function
    // body for a dual-mode arm; identical to `body` for every other caller).
    let (circs, circ_extra) = discover_circs(decl_scope, &steady_stmts);

    let ctx = ClassifyCtx {
        body: decl_scope,
        steady_stmts: &steady_stmts,
        func,
        cursor: &cursor,
        counter: counter.as_deref(),
        out_index_vars: &out_index_vars,
        bar_inputs: &bar_inputs,
        outputs: &outputs,
        circ_extra: &circ_extra,
        parity_field: parity.as_ref().map(|p| p.field.as_str()),
    };
    let (extrema, (mut state, temps)) =
        classify_or_extrema(&ctx, &ring_vars, &trailing, &windows, &circs)?;
    force_circ_index_state(&circs, &mut state, &temps);
    // The carried parity field is synthetic (no VarDecl): classify_locals skips
    // it, so append it as an ordinary int state field here (emitter seeds/flips
    // it — see ParitySpec). Appended last: deterministic, after real locals.
    if let Some(ps) = &parity {
        state.push((ps.field.clone(), VarType::Integer));
    }

    // Lags in input signature order.
    let lag_slots: Vec<LagSlot> = bar_inputs
        .iter()
        .filter_map(|a| {
            lags.get(a).map(|d| LagSlot {
                array: a.clone(),
                depth: *d,
            })
        })
        .collect();

    // The fundamentally-exclusive steady kind. `classify_or_extrema` /
    // `build_extrema` guarantee rings/windows/circs/counter are empty when an
    // extrema automaton is present, so the `Extrema` arm carries none of them
    // and the `Batch` arm takes the trailing rings directly — the enum IS the
    // "effective rings" fold (no separate temporary that could go stale).
    let steady = match extrema {
        Some(e) => Steady::Extrema(e),
        None => Steady::Batch {
            rings,
            windows,
            circs,
            counter,
        },
    };

    // Tier is derived from the steady kind's effective view: an extrema
    // automaton reads through its own ring, so it presents no trailing
    // rings/windows/circs (byte-identical to the old `rings_effective` inputs,
    // since those fields are provably empty on that arm).
    let tier = match &steady {
        Steady::Extrema(e) => derive_tier(&state, &lag_slots, &[], &[], &[], None, Some(e)),
        Steady::Batch {
            rings,
            windows,
            circs,
            counter,
        } => derive_tier(
            &state,
            &lag_slots,
            rings,
            windows,
            circs,
            counter.as_deref(),
            None,
        ),
    };

    let model = StreamModel {
        func,
        seed_boundary,
        out_feedback,
        body,
        tier,
        loop_form,
        steady_stmts,
        cursor,
        out_index_vars,
        bar_inputs,
        outputs,
        state,
        temps,
        lags: lag_slots,
        steady,
        identity,
        parity,
    };
    // Drift-proof the cached tier: re-derive it from the FINAL model (through
    // the accessors) and confirm it matches. Catches any construction bug that
    // drops tier-relevant data into the wrong steady arm.
    debug_assert_eq!(
        model.tier,
        derive_tier(
            &model.state,
            &model.lags,
            model.rings(),
            model.windows(),
            model.circs(),
            model.counter(),
            model.extrema(),
        )
    );
    Ok(model)
}

/// Recognize a param-selected dual-mode body (DI/DM): a shared prologue, a
/// leading `if (<param predicate>) { <steady loop>; return SUCCESS; }` arm, and
/// a fall-through general path with its own steady loop. Each arm is analyzed
/// as an independent [`StreamModel`] over its own body slice, with the FULL
/// body as the declaration scope (both arms' scalars are declared at the
/// function top). Returns [`StreamError::NoSteadyLoop`] when the body is not
/// dual-mode-shaped (the gate then falls through to dispatch/composed); a shape
/// match whose arm is genuinely unstreamable propagates that arm's error.
///
/// Tried only AFTER the single-loop [`analyze`] fails, so an ordinary function
/// with a leading `period == 1` identity path (T3) — which analyzes cleanly as
/// a Loop carrying an [`IdentityPath`] — is never misclassified here.
pub fn analyze_dual_mode(func: &FuncDef) -> Result<DualModePlan<'_>, StreamError> {
    let body: &[Statement] = if func.has_explicit_private {
        &func.private_body
    } else {
        &func.body
    };
    let params: BTreeSet<String> = func.optional_inputs.iter().map(|p| p.name.clone()).collect();
    let outputs: Vec<String> = func.outputs.iter().map(|o| o.name.clone()).collect();

    let ends_in_success = |b: &[Statement]| {
        matches!(
            b.last(),
            Some(Statement::Return { value: Some(Expr::Var(v)) })
                if matches!(v.as_str(), "SUCCESS" | "TA_SUCCESS")
        )
    };
    // A dual-mode arm is a steady loop (over the output range) — not a trivial
    // scalar branch. This excludes the prologue's `if (period > 1) lookbackTotal
    // = ... else lookbackTotal = 1` (both arms plain assignments) from the
    // if/else form.
    let has_loop = |b: &[Statement]| {
        b.iter().any(|s| {
            matches!(
                s,
                Statement::While { .. }
                    | Statement::DoWhile { .. }
                    | Statement::For { .. }
                    | Statement::ForC { .. }
            )
        })
    };
    // Find the FIRST top-level param-guarded branch of either form. The no-data
    // guard `if (startIdx > endIdx) return SUCCESS` is excluded by the params-only
    // condition (it references startIdx/endIdx).
    let mut found: Option<(usize, bool)> = None; // (index, is_if_else)
    for (i, s) in body.iter().enumerate() {
        let Statement::If {
            condition,
            then_body,
            else_body,
            ..
        } = s
        else {
            continue;
        };
        if !expr_is_param_pure(condition, &params) {
            continue;
        }
        // Early-return form (DI/DM): empty else, the then-arm is a steady loop
        // ending in SUCCESS, and a general path follows.
        if else_body.is_empty()
            && ends_in_success(then_body)
            && has_loop(then_body)
            && i + 1 < body.len()
        {
            found = Some((i, false));
            break;
        }
        // If/else form (TRIMA): a real else, each arm is a steady loop, NEITHER
        // returns (both fall through to a shared epilogue), and NOT a `<= literal`
        // threshold — that shape is fast-path-skip's (a bit-identical perf split
        // streams one arm; here the two arms genuinely differ and both stream).
        if !else_body.is_empty()
            && has_loop(then_body)
            && has_loop(else_body)
            && !ends_in_success(then_body)
            && !ends_in_success(else_body)
            && !is_threshold_pred(condition, &params)
        {
            found = Some((i, true));
            break;
        }
    }
    let Some((arm_idx, is_if_else)) = found else {
        return Err(StreamError::NoSteadyLoop);
    };

    // Real borrows of `body` (lifetime tied to `func`) — no synthetic Vecs, so
    // each mode's StreamModel can borrow its region. The full body is the
    // declaration scope for both arms.
    let Statement::If {
        condition,
        then_body,
        else_body,
        ..
    } = &body[arm_idx]
    else {
        unreachable!("arm_idx indexes the recognized If")
    };
    let prologue = &body[..arm_idx];
    // Mode A is always the `then` arm. Mode B is the `else` arm (if/else form) or
    // the fall-through tail after the returning arm (early-return form).
    let (then_region, alt_region, epilogue): (&[Statement], &[Statement], &[Statement]) =
        if is_if_else {
            (then_body, else_body, &body[arm_idx + 1..])
        } else {
            (then_body, &body[arm_idx + 1..], &[])
        };
    let mode_a = analyze_region_scoped(func, then_region, body, outputs.clone())?;
    let mode_b = analyze_region_scoped(func, alt_region, body, outputs)?;

    Ok(DualModePlan {
        func,
        predicate: condition.clone(),
        prologue,
        mode_a,
        mode_b,
        epilogue,
    })
}

/// Recognize a param fast-path split whose two arms are bit-identical (see
/// [`FastPathSkipPlan`]): `<prologue> if (<param> <= <lit>) { fast } else {
/// general } <epilogue>`. Streams the GENERAL (else) arm for every param and
/// skips the fast-path `then` arm (a batch-only perf specialization); the
/// stream_verify gate enforces bit-exactness across the threshold. Tried after
/// [`analyze_dual_mode`], so an early-return degenerate arm is handled there;
/// the `<= literal` threshold predicate excludes a genuine dual-mode branch
/// (e.g. TRIMA's `period % 2`, whose arms differ and are not interchangeable).
pub fn analyze_fastpath_skip(func: &FuncDef) -> Result<FastPathSkipPlan<'_>, StreamError> {
    let body: &[Statement] = if func.has_explicit_private {
        &func.private_body
    } else {
        &func.body
    };
    let params: BTreeSet<String> = func.optional_inputs.iter().map(|p| p.name.clone()).collect();
    let outputs: Vec<String> = func.outputs.iter().map(|o| o.name.clone()).collect();

    let ends_in_return = |b: &[Statement]| matches!(b.last(), Some(Statement::Return { .. }));
    let mut found: Option<usize> = None;
    for (i, s) in body.iter().enumerate() {
        let Statement::If {
            condition,
            then_body,
            else_body,
            ..
        } = s
        else {
            continue;
        };
        // A `param <= literal` (or `<`) threshold, a real else, and NEITHER arm
        // returns (both fall through to the shared epilogue — the early-return
        // form is dual-mode's).
        let is_threshold = matches!(
            condition,
            Expr::BinOp(l, BinOp::LessEq | BinOp::Less, r)
                if matches!(l.as_ref(), Expr::Var(v) if params.contains(v))
                    && matches!(r.as_ref(), Expr::IntLiteral(_))
        );
        if is_threshold
            && !else_body.is_empty()
            && !ends_in_return(then_body)
            && !ends_in_return(else_body)
        {
            found = Some(i);
            break;
        }
    }
    let Some(idx) = found else {
        return Err(StreamError::NoSteadyLoop);
    };
    let Statement::If { else_body, .. } = &body[idx] else {
        unreachable!("idx indexes the recognized If")
    };
    let model = analyze_region_scoped(func, else_body, body, outputs)?;
    Ok(FastPathSkipPlan {
        func,
        prologue: &body[..idx],
        model,
        epilogue: &body[idx + 1..],
    })
}

/// Recognize a dispatch body: optional identity path, then a single switch
/// over an enum optional param whose arms delegate the whole range to other
/// indicator functions, then `return <retcode-var>`.
///
/// Strictness contract: an arm whose callee is stream-flagged MUST match the
/// strict whole-range delegation shape — a mismatch is a hard error, never a
/// silent downgrade to a reject arm (that would turn a generator regression
/// into a vacuous verification pass). Arms with unflagged callees (MAMA,
/// TRIMA until its stream lands) become reject arms regardless of shape.
pub fn analyze_dispatch<'a>(
    func: &'a FuncDef,
    lookup: &dyn CalleeLookup,
) -> Result<DispatchPlan<'a>, StreamError> {
    let body: &[Statement] = if func.has_explicit_private {
        &func.private_body
    } else {
        &func.body
    };
    let bar_inputs = input_array_names(func);
    let outputs: Vec<String> = func.outputs.iter().map(|o| o.name.clone()).collect();
    let params: Vec<String> = func.optional_inputs.iter().map(|p| p.name.clone()).collect();
    let (identity, identity_idx) = detect_identity_path(body, &bar_inputs, &outputs, &params);

    // A body without a top-level switch is not dispatch-shaped AT ALL:
    // report NoSteadyLoop so validate_streamable surfaces the loop-tier
    // error (the actionable one for the 131 loop functions) instead of a
    // dispatch-shape complaint about their first non-decl statement.
    if !body
        .iter()
        .any(|s| matches!(s, Statement::Switch { .. }))
    {
        return Err(StreamError::NoSteadyLoop);
    }

    // Structural scan of the top level: decls/comments, the identity path,
    // exactly one switch, and a final `return <var>`.
    let mut switch_idx: Option<usize> = None;
    let mut ret_var: Option<&str> = None;
    for (i, s) in body.iter().enumerate() {
        if Some(i) == identity_idx {
            continue;
        }
        let is_last = i == body.len() - 1;
        match s {
            Statement::VarDecl { .. } | Statement::Comment(_) => {}
            Statement::Switch { .. } => {
                if switch_idx.is_some() {
                    return Err(StreamError::Unsupported(
                        "dispatch body has more than one switch".into(),
                    ));
                }
                switch_idx = Some(i);
            }
            Statement::Return {
                value: Some(Expr::Var(v)),
            } if is_last => ret_var = Some(v),
            other => {
                return Err(StreamError::Unsupported(format!(
                    "dispatch body has an unrecognized top-level statement ({})",
                    stmt_kind(other)
                )));
            }
        }
    }
    let Some(Statement::Switch {
        expr: subject,
        cases,
        default,
    }) = switch_idx.map(|i| &body[i])
    else {
        return Err(StreamError::NoSteadyLoop);
    };
    let ret_var = ret_var.ok_or_else(|| {
        StreamError::Unsupported("dispatch body does not end in `return <retcode>`".into())
    })?;

    // Subject must be an enum optional param.
    let param = match subject {
        Expr::Var(v)
            if func
                .optional_inputs
                .iter()
                .any(|p| p.name == *v && matches!(p.param_type, ParamType::Enum(_))) =>
        {
            v.clone()
        }
        _ => {
            return Err(StreamError::Unsupported(
                "dispatch switch subject is not an enum optional param".into(),
            ));
        }
    };

    // The default arm must not delegate to an indicator (MA's is a plain
    // `retCode = TA_BAD_PARAM`, which Open mirrors by construction).
    if !find_indicator_calls(default.as_slice(), lookup).is_empty() {
        return Err(StreamError::Unsupported(
            "dispatch default arm calls an indicator".into(),
        ));
    }

    let mut arms = Vec::new();
    for (label, stmts) in cases {
        arms.push(parse_dispatch_arm(
            label, stmts, func, lookup, ret_var, &bar_inputs, &outputs,
        )?);
    }
    if !arms.iter().any(|a| a.supported) {
        return Err(StreamError::Unsupported(
            "dispatch body has no stream-flagged callee arm".into(),
        ));
    }
    Ok(DispatchPlan {
        func,
        param,
        arms,
        identity,
    })
}

/// Recognize a composed body: an optional steady producer loop
/// materializing one intermediate series, then a tail pipeline of
/// whole-range sub-calls to stream-flagged callees over materialized
/// series (or the caller's single real bar input), tail-aligning memmoves
/// into outputs, per-bar combine maps, and guard/free/bookkeeping
/// statements. Strict: any unrecognized tail statement is an error, and a
/// sub-call to an unflagged callee is an error (a composed function only
/// streams when every piece does — there is no per-arm reject here).
#[allow(clippy::too_many_lines, clippy::cognitive_complexity)]
pub fn analyze_composed<'a>(
    func: &'a FuncDef,
    lookup: &dyn CalleeLookup,
) -> Result<ComposedPlan<'a>, StreamError> {
    let body: &[Statement] = if func.has_explicit_private {
        &func.private_body
    } else {
        &func.body
    };
    let bar_inputs = input_array_names(func);
    let outputs: Vec<String> = func.outputs.iter().map(|o| o.name.clone()).collect();

    // Producer loop = the LAST top-level loop over endIdx, if any.
    let is_endidx_loop = |s: &Statement| match s {
        Statement::While { condition, .. }
        | Statement::DoWhile { condition, .. }
        | Statement::ForC { condition, .. } => endidx_cursor(condition).is_some(),
        _ => false,
    };
    let loop_pos = body.iter().rposition(is_endidx_loop);

    // The tail starts after the producer loop, or (loopless pipelines) at
    // the first sub-call statement; everything before is the prologue.
    let is_subcall = |s: &Statement| {
        matches!(s,
            Statement::Assign { value: Expr::FuncCall(name, _), .. }
                if lookup.callee(name).is_some())
    };
    let tail_start = match loop_pos {
        Some(lp) => lp + 1,
        None => body
            .iter()
            .position(is_subcall)
            .ok_or(StreamError::NoSteadyLoop)?,
    };
    let region = &body[..tail_start];
    let tail = &body[tail_start..];
    // A leading parameter-guarded fast-path block (BBANDS's SMA path) is a
    // batch-only specialization: exclude it from the region Open transcribes, so
    // the stream composes the general path below for every parameter value. The
    // sub-calls that make it non-streamable in place are nested inside the block,
    // so the top-level `is_subcall` scan above never selected it as the tail
    // start (`tail` therefore has no fast-path block to filter).
    let params_pre: BTreeSet<String> =
        func.optional_inputs.iter().map(|p| p.name.clone()).collect();
    let region_open: Vec<Statement> = region
        .iter()
        .filter(|st| !is_fastpath_block(st, &params_pre, lookup))
        .cloned()
        .collect();
    // Composed-shaped = the tail delegates to another indicator. A tail of
    // plain bookkeeping (outBegIdx writes, the final return) is an ordinary
    // loop function: report NoSteadyLoop so the loop-tier error surfaces.
    if !tail.iter().any(is_subcall) {
        return Err(StreamError::NoSteadyLoop);
    }

    // Producer analysis (when a loop exists): the ONE non-output local
    // array the loop writes is the intermediate series.
    let mut intermediates: Vec<String> = Vec::new();
    let (producer, series) = if let Some(lp) = loop_pos {
        let loop_body: &[Statement] = match &body[lp] {
            Statement::While { body, .. }
            | Statement::DoWhile { body, .. }
            | Statement::ForC { body, .. } => body,
            _ => unreachable!("matched as loop above"),
        };
        let mut written: BTreeSet<String> = BTreeSet::new();
        for st in loop_body {
            walk_assign_targets(st, &mut |t| {
                if let Expr::ArrayAccess(name, _) = t {
                    written.insert(name.clone());
                }
            });
        }
        written.retain(|n| !outputs.contains(n) && !bar_inputs.contains(n));
        if written.len() != 1 {
            return Err(StreamError::Unsupported(format!(
                "composed producer loop must write exactly one intermediate series, found {}",
                written.len()
            )));
        }
        let series = written.into_iter().next().expect("len checked");
        intermediates.push(series.clone());
        let producer = analyze_region(func, region, vec![series.clone()])?;
        (Some(producer), Some(series))
    } else {
        // Loopless: the prologue must be pure scalar setup (no array
        // writes at all — guards/decls/lookback computations only). The
        // excluded fast-path block is not part of it.
        let mut bad = false;
        for st in &region_open {
            walk_assign_targets(st, &mut |t| {
                if matches!(t, Expr::ArrayAccess(..)) {
                    bad = true;
                }
            });
        }
        if bad {
            return Err(StreamError::Unsupported(
                "loopless composed prologue writes an array".into(),
            ));
        }
        (None, None)
    };

    // The caller's own bar inputs may feed a sub-call directly — a single real
    // input (STOCHRSI's rsi(inReal); STDDEV's var(inReal)) or several price
    // inputs (ADXR's adx(inHigh, inLow, inClose)). In the stream these arrive
    // as the update scalars, so the sub handle is fed them per bar.
    let direct_inputs: BTreeSet<String> = bar_inputs.iter().cloned().collect();

    // --- tail pipeline -------------------------------------------------------
    let params: BTreeSet<String> = func
        .optional_inputs
        .iter()
        .map(|p| p.name.clone())
        .collect();
    let mut subs: Vec<SubCallStep> = Vec::new();
    let mut steps: Vec<UpdateStep> = Vec::new();
    let mut series_frees: Vec<SeriesFree> = Vec::new();
    let mut freed: BTreeSet<String> = BTreeSet::new();
    let mut map_temp_names: BTreeSet<String> = BTreeSet::new();
    let mut sub_lag_rings: Vec<SubLagRing> = Vec::new();
    let mut defined: BTreeSet<String> = intermediates.iter().cloned().collect();
    // Out-meta provenance for the same-bar proof a combine map's `series[cursor
    // + off]` read needs: each series' element-count receiver and producing
    // endIdx, and each scalar local that is an element-count difference of two
    // of them.
    let mut series_nbelem: BTreeMap<String, RecvVar> = BTreeMap::new();
    let mut series_endidx: BTreeMap<String, Expr> = BTreeMap::new();
    let mut diff_locals: BTreeMap<String, (RecvVar, RecvVar)> = BTreeMap::new();
    for (i, st) in tail.iter().enumerate() {
        match st {
            Statement::Comment(_) => {}
            // Whole-range sub-call over materialized series / the bar input.
            Statement::Assign {
                target: Expr::Var(_),
                value: Expr::FuncCall(callee, args),
                compound: false,
            } if lookup.callee(callee).is_some() => {
                let sig = lookup.callee(callee).expect("checked");
                if !sig.streaming {
                    return Err(StreamError::UnsupportedCall(format!(
                        "composed sub-call `{callee}` has no stream"
                    )));
                }
                if args.len() != 2 + sig.n_inputs + sig.n_opts + 2 + sig.n_outputs {
                    return Err(StreamError::Unsupported(format!(
                        "composed sub-call `{callee}` has an unexpected argument count"
                    )));
                }
                let as_series = |e: &Expr| -> Result<String, StreamError> {
                    match e {
                        Expr::Var(v) => Ok(v.clone()),
                        _ => Err(StreamError::Unsupported(format!(
                            "composed sub-call `{callee}` uses a non-plain series argument"
                        ))),
                    }
                };
                let mut srcs = Vec::new();
                for k in 0..sig.n_inputs {
                    let src = as_series(&args[2 + k])?;
                    if !defined.contains(&src) && !direct_inputs.contains(&src) {
                        return Err(StreamError::Unsupported(format!(
                            "composed sub-call `{callee}` reads `{src}` before it is materialized"
                        )));
                    }
                    srcs.push(src);
                }
                let dst_arg_base = 2 + sig.n_inputs + sig.n_opts + 2;
                let mut dsts = Vec::new();
                for k in 0..sig.n_outputs {
                    let dst = as_series(&args[dst_arg_base + k])?;
                    if !srcs.contains(&dst)
                        && !outputs.contains(&dst)
                        && !defined.contains(&dst)
                    {
                        // A fresh intermediate series (STOCHRSI's RSI buffer).
                        intermediates.push(dst.clone());
                    }
                    defined.insert(dst.clone());
                    dsts.push(dst);
                }
                let first_opt_arg = 2 + sig.n_inputs;
                let opt_args: Vec<Expr> =
                    args[first_opt_arg..first_opt_arg + sig.n_opts].to_vec();
                for a in &opt_args {
                    let mut names = BTreeSet::new();
                    expr_var_names(a, &mut names);
                    if !names.iter().all(|nm| params.contains(nm)) {
                        return Err(StreamError::Unsupported(format!(
                            "composed sub-call `{callee}` has an impure optional argument"
                        )));
                    }
                }
                // Record each destination's element-count receiver and its
                // producing endIdx (all of a callee's outputs share the one
                // `outNBElement`, the second of the two out-meta pointers).
                let nb_recv = recv_var(&args[2 + sig.n_inputs + sig.n_opts + 1]);
                for d in &dsts {
                    if let Some(nb) = &nb_recv {
                        series_nbelem.insert(d.clone(), nb.clone());
                    }
                    series_endidx.insert(d.clone(), args[1].clone());
                }
                steps.push(UpdateStep::Sub {
                    sub_idx: subs.len(),
                });
                subs.push(SubCallStep {
                    tail_idx: i,
                    callee: callee.clone(),
                    opt_args,
                    s_arg: args[0].clone(),
                    e_arg: args[1].clone(),
                    srcs,
                    dsts,
                });
            }
            // Tail-aligning copy into an output.
            Statement::Expr(Expr::FuncCall(name, args))
                if name == "memmove" && args.len() == 3 =>
            {
                let dst = match &args[0] {
                    Expr::Var(v) => v.clone(),
                    _ => {
                        return Err(StreamError::Unsupported(
                            "composed memmove destination is not a plain series".into(),
                        ));
                    }
                };
                let src = match &args[1] {
                    Expr::AddressOf(inner) => match inner.as_ref() {
                        Expr::ArrayAccess(sname, _) => sname.clone(),
                        _ => {
                            return Err(StreamError::Unsupported(
                                "composed memmove source is not a series tail".into(),
                            ));
                        }
                    },
                    _ => {
                        return Err(StreamError::Unsupported(
                            "composed memmove source is not a series tail".into(),
                        ));
                    }
                };
                if !defined.contains(&src) || !outputs.contains(&dst) {
                    return Err(StreamError::Unsupported(
                        "composed memmove does not align a series into an output".into(),
                    ));
                }
                defined.insert(dst.clone());
                steps.push(UpdateStep::Align { dst, src });
            }
            // Per-bar combine map over materialized series (STDDEV's sqrt
            // variants), possibly wrapped in a param-selected If.
            Statement::ForC { .. } => {
                check_map_step(
                    st,
                    &defined,
                    &outputs,
                    &params,
                    lookup,
                    &mut map_temp_names,
                    &series_nbelem,
                    &series_endidx,
                    &diff_locals,
                    &mut sub_lag_rings,
                )?;
                for o in map_output_writes(st, &outputs) {
                    defined.insert(o);
                }
                steps.push(UpdateStep::Map { tail_idx: i });
            }
            Statement::If {
                condition,
                then_body,
                else_body,
                ..
            } if is_map_variant_if(then_body, else_body) => {
                let mut names = BTreeSet::new();
                expr_var_names(condition, &mut names);
                if !names.iter().all(|nm| params.contains(nm)) {
                    return Err(StreamError::Unsupported(
                        "composed map variant condition is not param-pure".into(),
                    ));
                }
                for branch in [then_body, else_body] {
                    for bst in branch.iter().filter(|x| !matches!(x, Statement::Comment(_))) {
                        check_map_step(
                            bst,
                            &defined,
                            &outputs,
                            &params,
                            lookup,
                            &mut map_temp_names,
                            &series_nbelem,
                            &series_endidx,
                            &diff_locals,
                            &mut sub_lag_rings,
                        )?;
                    }
                }
                for o in map_output_writes(st, &outputs) {
                    defined.insert(o);
                }
                steps.push(UpdateStep::Map { tail_idx: i });
            }
            // Guards (retCode / nbElement checks with early returns) and the
            // bufferIsAllocated free: transcribed verbatim in Open, absent
            // from the per-bar pipeline. Strictly bounded shape.
            Statement::If { .. } => {
                check_composed_guard(st, &defined, lookup)?;
                for ser in intermediates.clone() {
                    if !freed.contains(&ser) && guard_frees_series(st, &ser) {
                        freed.insert(ser);
                        series_frees.push(SeriesFree {
                            tail_idx: i,
                            stmt: st.clone(),
                        });
                    }
                }
            }
            // Out-meta bookkeeping (Open maps them to dummies).
            Statement::Assign {
                target: Expr::PointerDeref(p),
                ..
            } if p == "outBegIdx" || p == "outNBElement" => {}
            // Scalar tail locals (APO/PPO's alignment offset): Open-only. When
            // one is an element-count difference (`off = fastNb - *outNBElement`),
            // record its provenance so a later combine map can prove `series[
            // cursor + off]` is same-bar; any other write to it clears the record.
            Statement::Assign {
                target: Expr::Var(v),
                value,
                ..
            } if !defined.contains(v) && !outputs.contains(v) => {
                let known: Vec<RecvVar> = series_nbelem.values().cloned().collect();
                match nb_difference(value, &known) {
                    Some(prov) => {
                        diff_locals.insert(v.clone(), prov);
                    }
                    None => {
                        diff_locals.remove(v);
                    }
                }
            }
            Statement::Expr(Expr::FuncCall(name, args)) if name == "free" => {
                // A bare unconditional free of an intermediate series is
                // just as replayable on inserted failure returns as the
                // guarded form (a sub-call can never legally read a series
                // after batch freed it, so the replay is always pre-free).
                if let Some(Expr::Var(v)) = args.first() {
                    if intermediates.contains(v) && !freed.contains(v) {
                        freed.insert(v.clone());
                        series_frees.push(SeriesFree {
                            tail_idx: i,
                            stmt: st.clone(),
                        });
                    }
                }
            }
            Statement::Return { .. } if i == tail.len() - 1 => {}
            other => {
                return Err(StreamError::Unsupported(format!(
                    "composed tail has an unrecognized statement ({})",
                    stmt_kind(other)
                )));
            }
        }
    }
    if subs.is_empty() {
        return Err(StreamError::NoSteadyLoop);
    }
    // A heap-allocated series with no replayable free would make every
    // inserted failure return (honest Open rejections included) leak it —
    // the exact class LeakSanitizer caught during this tranche. Refuse to
    // build such a plan.
    for ser in &intermediates {
        if !freed.contains(ser) && region_mallocs_series(body, ser) {
            return Err(StreamError::Unsupported(format!(
                "composed series `{ser}` is heap-allocated but the tail has no \
                 replayable free for the inserted failure returns"
            )));
        }
    }
    for out in &outputs {
        if !defined.contains(out) {
            return Err(StreamError::Unsupported(format!(
                "composed pipeline never defines output `{out}`"
            )));
        }
    }
    // Map temps: function-local scalars the maps reference; resolve types
    // from the body's declarations.
    let mut decls: BTreeMap<String, VarType> = BTreeMap::new();
    collect_var_decls(body, &mut decls);
    let mut map_temps: Vec<(String, VarType)> = Vec::new();
    for name in &map_temp_names {
        let Some(ty) = decls.get(name) else {
            return Err(StreamError::Unsupported(format!(
                "composed map references `{name}` with no visible declaration"
            )));
        };
        map_temps.push((name.clone(), ty.clone()));
    }
    Ok(ComposedPlan {
        func,
        producer,
        series,
        intermediates,
        subs,
        steps,
        tail,
        region: region_open,
        series_frees,
        map_temps,
        sub_lag_rings,
    })
}

/// Replay the emitter's cur-map over a composed plan: every sub source and
/// align source must already be materialized when consumed, and every output
/// must end up produced. This mirrors [`crate::backends::c_stream`]'s step
/// emission exactly, so any plan it accepts here the emitter can render (and
/// any plan it rejects would have panicked the emitter — the loud gate the
/// loopless tier needs in place of a producer transition build).
fn check_composed_emittable(plan: &ComposedPlan) -> Result<(), StreamError> {
    let bar_inputs = input_array_names(plan.func);
    let outputs: Vec<String> = plan.func.outputs.iter().map(|o| o.name.clone()).collect();
    let mut cur: BTreeSet<String> = bar_inputs.into_iter().collect();
    if let Some(series) = &plan.series {
        cur.insert(series.clone());
    }
    for step in &plan.steps {
        match step {
            UpdateStep::Sub { sub_idx } => {
                let sub = &plan.subs[*sub_idx];
                for s in &sub.srcs {
                    if !cur.contains(s) {
                        return Err(StreamError::Unsupported(format!(
                            "composed sub `{}` reads `{s}` before it is materialized",
                            sub.callee
                        )));
                    }
                }
                for d in &sub.dsts {
                    cur.insert(d.clone());
                }
            }
            UpdateStep::Align { dst, src } => {
                if !cur.contains(src) {
                    return Err(StreamError::Unsupported(format!(
                        "composed align reads `{src}` before it is materialized"
                    )));
                }
                cur.insert(dst.clone());
            }
            UpdateStep::Map { tail_idx } => {
                for o in map_output_writes(&plan.tail[*tail_idx], &outputs) {
                    cur.insert(o);
                }
            }
        }
    }
    for out in &outputs {
        if !cur.contains(out) {
            return Err(StreamError::Unsupported(format!(
                "composed pipeline never produces output `{out}`"
            )));
        }
    }
    Ok(())
}

/// True when an If's branches consist solely of map loops (and comments):
/// STDDEV's `if (optInNbDev != 1.0) { for... } else { for... }` variant
/// selector, as opposed to a retCode guard.
fn is_map_variant_if(then_body: &[Statement], else_body: &[Statement]) -> bool {
    let only_maps = |stmts: &[Statement]| {
        let meaningful: Vec<&Statement> = stmts
            .iter()
            .filter(|s| !matches!(s, Statement::Comment(_)))
            .collect();
        !meaningful.is_empty() && meaningful.iter().all(|s| matches!(s, Statement::ForC { .. }))
    };
    only_maps(then_body) && (else_body.is_empty() || only_maps(else_body))
}

/// An out-meta receiver — where a sub-call writes its `outBegIdx` or
/// `outNBElement`. Two spellings occur and the read form is part of the
/// identity: `&fastNb` (an int local, read back as `fastNb`) versus the
/// function's own out-pointer `outNBElement` (read back as `*outNBElement`).
#[derive(Clone, PartialEq, Eq, Debug)]
enum RecvVar {
    /// Passed as `&local`; read as `Var(local)`.
    Local(String),
    /// Passed as the out-pointer `p`; read as `PointerDeref(p)`.
    Pointer(String),
}

/// The out-meta receiver a sub-call argument names, or None for shapes we do
/// not track (`&x` → `Local(x)`; a plain pointer `p` → `Pointer(p)`; anything
/// else → None).
fn recv_var(arg: &Expr) -> Option<RecvVar> {
    match arg {
        Expr::AddressOf(inner) => match inner.as_ref() {
            Expr::Var(v) => Some(RecvVar::Local(v.clone())),
            _ => None,
        },
        Expr::Var(v) => Some(RecvVar::Pointer(v.clone())),
        _ => None,
    }
}

/// How a difference operand reads an out-meta receiver: `x` → `Local(x)`,
/// `*p` → `Pointer(p)`. Any other expression is not a receiver read.
fn recv_read(e: &Expr) -> Option<RecvVar> {
    match e {
        Expr::Var(v) => Some(RecvVar::Local(v.clone())),
        Expr::PointerDeref(p) => Some(RecvVar::Pointer(p.clone())),
        _ => None,
    }
}

/// If `value` is `a - b` where both operands read *known* out-element-count
/// receivers, return their provenance `(a, b)`. This is what proves an APO/PPO
/// alignment offset (`fastNb - *outNBElement`) is an element-count difference.
///
/// The element-count form is used rather than the begIdx difference
/// (`*outBegIdx - fastBeg`) because it is identical in value — for two sub-calls
/// sharing an endIdx, `nb(a) - nb(b) == begIdx(b) - begIdx(a)` — yet cannot
/// underflow: the wider window (the fast MA here) always has at least as many
/// outputs, so the subtraction is non-negative. The begIdx form underflows as a
/// Rust `usize` when the narrower output is empty (issue: Rust-debug-only
/// underflow class). A genuine lag (ADXR's `period - 1`) is a param expression,
/// not a receiver difference, so it returns None.
fn nb_difference(value: &Expr, known: &[RecvVar]) -> Option<(RecvVar, RecvVar)> {
    let Expr::BinOp(l, BinOp::Sub, r) = value else {
        return None;
    };
    let a = recv_read(l)?;
    let b = recv_read(r)?;
    if known.contains(&a) && known.contains(&b) {
        Some((a, b))
    } else {
        None
    }
}

/// Structural expression equality — used to confirm two sub-calls share an
/// endIdx argument (so an element-count difference really is a same-bar shift).
/// BinOp has no `PartialEq`, so operators compare by discriminant.
fn exprs_equal(a: &Expr, b: &Expr) -> bool {
    match (a, b) {
        (Expr::Var(x), Expr::Var(y))
        | (Expr::PointerDeref(x), Expr::PointerDeref(y)) => x == y,
        (Expr::IntLiteral(x), Expr::IntLiteral(y)) => x == y,
        // Structural (bit-exact) equality, not numeric — and float_cmp-clean.
        (Expr::Literal(x), Expr::Literal(y)) => x.to_bits() == y.to_bits(),
        (Expr::ArrayAccess(n1, i1), Expr::ArrayAccess(n2, i2)) => {
            n1 == n2 && exprs_equal(i1, i2)
        }
        (Expr::BinOp(l1, o1, r1), Expr::BinOp(l2, o2, r2)) => {
            std::mem::discriminant(o1) == std::mem::discriminant(o2)
                && exprs_equal(l1, l2)
                && exprs_equal(r1, r2)
        }
        (Expr::Cast(t1, e1), Expr::Cast(t2, e2)) => t1 == t2 && exprs_equal(e1, e2),
        (Expr::Not(e1), Expr::Not(e2)) | (Expr::AddressOf(e1), Expr::AddressOf(e2)) => {
            exprs_equal(e1, e2)
        }
        (Expr::FuncCall(n1, a1), Expr::FuncCall(n2, a2)) => {
            n1 == n2 && a1.len() == a2.len() && a1.iter().zip(a2).all(|(x, y)| exprs_equal(x, y))
        }
        _ => false,
    }
}

/// The index shape of a combine-map array access.
enum IndexForm {
    /// `series[cursor]` — the current bar.
    PlainCursor,
    /// `series[cursor + off]` — a candidate same-bar shift; `off` must still be
    /// proven a begIdx difference before it is accepted.
    CursorPlus(String),
    /// Anything else (a literal lag, a nested expression): never same-bar.
    Other,
}

/// Classify a `series[idx]` access index against the loop's single cursor.
/// Accepts `cursor + off` in either operand order; rejects `cursor - off`,
/// `cursor + off + 1`, literal offsets, and anything more nested.
fn offset_index_form(idx: &Expr, cursors: &BTreeSet<String>) -> IndexForm {
    if let Expr::Var(v) = idx {
        return if cursors.contains(v) {
            IndexForm::PlainCursor
        } else {
            IndexForm::Other
        };
    }
    if let Expr::BinOp(l, BinOp::Add, r) = idx {
        for (a, b) in [(l.as_ref(), r.as_ref()), (r.as_ref(), l.as_ref())] {
            if let (Expr::Var(c), Expr::Var(off)) = (a, b) {
                if cursors.contains(c) && !cursors.contains(off) {
                    return IndexForm::CursorPlus(off.clone());
                }
            }
        }
    }
    IndexForm::Other
}

/// The output series a combine map writes (at any index): a map may DEFINE an
/// output from sub-outputs (ADXR writes `outReal` from the ADX lag ring), so
/// those outputs become materialized once the map runs.
pub fn map_output_writes(stmt: &Statement, outputs: &[String]) -> Vec<String> {
    let mut out: Vec<String> = Vec::new();
    walk_assign_targets(stmt, &mut |t| {
        if let Expr::ArrayAccess(name, _) = t {
            if outputs.contains(name) && !out.contains(name) {
                out.push(name.clone());
            }
        }
    });
    out
}

/// If `idx` is `cursor + <expr>` in either operand order, return the offset
/// expression — unlike [`offset_index_form`] this accepts a compound offset
/// (a parameter expression like `optInTimePeriod - 1`), used to recognize a
/// sub-output self-lag read `series[cursor + (period-1)]`.
fn cursor_plus_expr(idx: &Expr, cursors: &BTreeSet<String>) -> Option<Expr> {
    let Expr::BinOp(l, BinOp::Add, r) = idx else {
        return None;
    };
    match (l.as_ref(), r.as_ref()) {
        (Expr::Var(c), other) | (other, Expr::Var(c)) if cursors.contains(c) => {
            // Reject a plain cursor+cursor (both operands cursors).
            if matches!(other, Expr::Var(v) if cursors.contains(v)) {
                None
            } else {
                Some(other.clone())
            }
        }
        _ => None,
    }
}

/// True when `e` reads only parameters, literals and arithmetic of them — no
/// series, pointers, cursors or calls. A sub-output lag depth must be such a
/// constant-per-stream parameter expression (`optInTimePeriod - 1`).
/// A `<param> <= <literal>` (or `<`) threshold predicate — the shape of a batch
/// perf fast-path split ([`analyze_fastpath_skip`], MIDPRICE), as opposed to a
/// genuine dual-mode branch (TRIMA's `period % 2 == 1`). Used to route the
/// two if/else recognizers apart.
fn is_threshold_pred(cond: &Expr, params: &BTreeSet<String>) -> bool {
    matches!(
        cond,
        Expr::BinOp(l, BinOp::LessEq | BinOp::Less, r)
            if matches!(l.as_ref(), Expr::Var(v) if params.contains(v))
                && matches!(r.as_ref(), Expr::IntLiteral(_))
    )
}

fn expr_is_param_pure(e: &Expr, params: &BTreeSet<String>) -> bool {
    let mut ok = true;
    walk_expr(e, &mut |x| match x {
        Expr::Var(v) if !params.contains(v) => ok = false,
        Expr::ArrayAccess(..) | Expr::PointerDeref(_) | Expr::FuncCall(..) => ok = false,
        _ => {}
    });
    ok
}

/// Deep variant of [`walk_stmt_exprs`]: `f` sees every sub-expression of every
/// statement expression, not only the top-level target/value/condition nodes
/// (the plain walker stops there, so a `tempBuffer[i+offset]` nested inside a
/// `BinOp` would slip past an offset check — this recurses in).
fn walk_stmt_exprs_deep(s: &Statement, f: &mut dyn FnMut(&Expr)) {
    walk_stmt_exprs(s, &mut |top| walk_expr(top, f));
}

/// Validate one map loop: `for (i = 0; i < NB; i++) { <per-bar body> }` whose
/// body reads/writes ONLY `series[cursor]`, params, and scalar temps — plus
/// the one same-bar-shifted form `series[cursor + off]` where `off` is a
/// proven begIdx difference (APO/PPO's `tempBuffer[i + offset]`). The emitter
/// later drops the shell and turns EVERY series access into a current scalar
/// (it is index-blind), so the soundness that the shifted read really is
/// same-bar has to be proven HERE; everything checked makes that faithful.
#[allow(clippy::too_many_lines)]
fn check_map_step(
    st: &Statement,
    defined: &BTreeSet<String>,
    outputs: &[String],
    params: &BTreeSet<String>,
    lookup: &dyn CalleeLookup,
    temps: &mut BTreeSet<String>,
    series_nbelem: &BTreeMap<String, RecvVar>,
    series_endidx: &BTreeMap<String, Expr>,
    diff_locals: &BTreeMap<String, (RecvVar, RecvVar)>,
    sub_lag_rings: &mut Vec<SubLagRing>,
) -> Result<(), StreamError> {
    let Statement::ForC {
        init,
        condition: _,
        update: _,
        body,
    } = st
    else {
        return Err(StreamError::Unsupported(
            "composed map variant contains a non-loop statement".into(),
        ));
    };
    // One cursor only. A multi-cursor init (the pre-Flat-B APO/PPO `i=0,j=off`
    // form) is the streamable-source-form violation G1 names: guide the author
    // to fold the second cursor into a begIdx-offset index.
    let inits: Vec<&Statement> = match init.as_ref() {
        Statement::Block { body } => body
            .iter()
            .filter(|s| !matches!(s, Statement::Comment(_)))
            .collect(),
        one => vec![one],
    };
    if inits.len() > 1 {
        return Err(StreamError::Unsupported(
            "multi-cursor combine loop; rewrite as a single cursor with a \
             begIdx-offset index (see APO)"
                .into(),
        ));
    }
    let mut cursors: BTreeSet<String> = BTreeSet::new();
    for ist in inits {
        match ist {
            Statement::Assign {
                target: Expr::Var(v),
                value: Expr::IntLiteral(0),
                ..
            } => {
                cursors.insert(v.clone());
            }
            _ => {
                return Err(StreamError::Unsupported(
                    "composed map cursor does not start at 0 (lagged reads are a later shape)"
                        .into(),
                ));
            }
        }
    }
    if !find_indicator_calls(std::slice::from_ref(st), lookup).is_empty() {
        return Err(StreamError::Unsupported(
            "composed map calls an indicator".into(),
        ));
    }

    // The map's primary output = the one series it writes at the plain cursor
    // (APO/PPO write `outReal[i]`). It anchors the same-bar proof for any offset
    // read: `series[cursor + off]` is same-bar iff `off` is the element-count
    // difference `nb(series) - nb(primary_out)` AND the two producers share an
    // endIdx (then that difference equals `begIdx(primary_out) - begIdx(series)`
    // exactly — the shift that aligns the two windows).
    let mut written_series: BTreeSet<String> = BTreeSet::new();
    for bst in body {
        walk_assign_targets(bst, &mut |t| {
            if let Expr::ArrayAccess(name, idx) = t {
                if matches!(idx.as_ref(), Expr::Var(v) if cursors.contains(v)) {
                    written_series.insert(name.clone());
                }
            }
        });
    }
    let primary_out: Option<&String> = if written_series.len() == 1 {
        written_series.iter().next()
    } else {
        None
    };

    // Detect sub-output self-lag rings (ADXR): a defined series read BOTH at the
    // plain cursor (the lagged value) AND at `cursor + <param-pure expr>` (the
    // current, newest value). The offset expression is the lag depth / ring cap.
    let mut has_plain: BTreeSet<String> = BTreeSet::new();
    let mut lag_rings: BTreeMap<String, Expr> = BTreeMap::new();
    for bst in body {
        walk_stmt_exprs_deep(bst, &mut |e| {
            if let Expr::ArrayAccess(name, idx) = e {
                if defined.contains(name) {
                    if matches!(offset_index_form(idx, &cursors), IndexForm::PlainCursor) {
                        has_plain.insert(name.clone());
                    } else if let Some(off) = cursor_plus_expr(idx, &cursors) {
                        if expr_is_param_pure(&off, params) {
                            lag_rings.insert(name.clone(), off);
                        }
                    }
                }
            }
        });
    }
    lag_rings.retain(|name, _| has_plain.contains(name));
    for (series, lag) in &lag_rings {
        sub_lag_rings.push(SubLagRing {
            series: series.clone(),
            lag: lag.clone(),
        });
    }

    let mut recognized_off: BTreeSet<String> = BTreeSet::new();
    let mut err: Option<StreamError> = None;
    for bst in body {
        walk_stmt_exprs_deep(bst, &mut |e| {
            if err.is_some() {
                return;
            }
            match e {
                Expr::ArrayAccess(name, idx) => {
                    if let Some(lag) = lag_rings.get(name) {
                        // Lag-ring series: only the current read (cursor + lag)
                        // or the lagged read (plain cursor) are allowed.
                        let ok = matches!(
                            offset_index_form(idx, &cursors),
                            IndexForm::PlainCursor
                        ) || cursor_plus_expr(idx, &cursors)
                            .is_some_and(|off| exprs_equal(&off, lag));
                        if !ok {
                            err = Some(StreamError::Unsupported(format!(
                                "composed map reads lag-ring series `{name}` at an offset other \
                                 than the current bar or its fixed lag"
                            )));
                        }
                    } else {
                        match offset_index_form(idx, &cursors) {
                            IndexForm::PlainCursor => {
                                if !defined.contains(name) && !outputs.contains(name) {
                                    err = Some(StreamError::Unsupported(format!(
                                        "composed map accesses `{name}` outside series[cursor] form"
                                    )));
                                }
                            }
                            IndexForm::CursorPlus(off) => {
                                // Same-bar iff `off == nb(name) - nb(primary_out)`
                                // and the two producers share an endIdx.
                                let same_bar = defined.contains(name)
                                    && primary_out.is_some_and(|po| {
                                        let prov_ok = matches!(
                                            (
                                                diff_locals.get(&off),
                                                series_nbelem.get(name),
                                                series_nbelem.get(po),
                                            ),
                                            (Some((a, b)), Some(this), Some(prim))
                                                if a == this && b == prim
                                        );
                                        let end_ok = matches!(
                                            (series_endidx.get(name), series_endidx.get(po)),
                                            (Some(e1), Some(e2)) if exprs_equal(e1, e2)
                                        );
                                        prov_ok && end_ok
                                    });
                                if same_bar {
                                    recognized_off.insert(off);
                                } else {
                                    err = Some(StreamError::Unsupported(format!(
                                        "composed map reads `{name}[cursor+{off}]` but `{off}` is not a \
                                         proven same-bar shift — it must be the element-count difference \
                                         of the two sub-outputs sharing an endIdx (as in APO's \
                                         `fastNb - *outNBElement`); a genuine lag needs a ring, not a \
                                         combine map"
                                    )));
                                }
                            }
                            IndexForm::Other => {
                                err = Some(StreamError::Unsupported(format!(
                                    "composed map accesses `{name}` outside series[cursor] form"
                                )));
                            }
                        }
                    }
                }
                Expr::FuncCall(name, _) => {
                    if is_stateful_call(name) {
                        err = Some(StreamError::UnsupportedCall(name.clone()));
                    }
                }
                Expr::Var(v)
                    if !cursors.contains(v)
                        && !params.contains(v)
                        && !defined.contains(v)
                        && !recognized_off.contains(v) =>
                {
                    temps.insert(v.clone());
                }
                _ => {}
            }
        });
    }
    if let Some(e) = err {
        return Err(e);
    }
    Ok(())
}

/// True when an expression heap-allocates: a `malloc`/`TA_Malloc` call
/// anywhere inside it, so cast-wrapped forms (`(double *)malloc(...)`) and the
/// library allocator are recognized, not just a bare top-level `malloc`.
pub fn expr_allocates(e: &Expr) -> bool {
    let mut found = false;
    walk_expr(e, &mut |x| {
        if let Expr::FuncCall(name, _) = x {
            if name == "malloc" || name == "TA_Malloc" {
                found = true;
            }
        }
    });
    found
}

/// True when the producer region heap-allocates the series
/// (`series = malloc(...)` anywhere in the region, including branches). Both
/// the assignment and declaration-with-initializer forms count, and the
/// allocation may be cast-wrapped or a `TA_Malloc` — the leak-refusal gate
/// must fire on ANY heap intermediate without a replayable free.
fn region_mallocs_series(region: &[Statement], series: &str) -> bool {
    fn walk(stmts: &[Statement], series: &str, found: &mut bool) {
        for st in stmts {
            match st {
                Statement::Assign {
                    target: Expr::Var(v),
                    value,
                    ..
                } if v == series && expr_allocates(value) => *found = true,
                // Declaration-with-initializer form (STOCHRSI's
                // `double *tempRSIBuffer = malloc(...)`): the same heap
                // allocation the Assign form catches, spelled as a VarDecl.
                Statement::VarDecl {
                    name,
                    init: Some(init),
                    ..
                } if name == series && expr_allocates(init) => *found = true,
                Statement::If {
                    then_body,
                    else_body,
                    ..
                } => {
                    walk(then_body, series, found);
                    walk(else_body, series, found);
                }
                Statement::While { body, .. }
                | Statement::DoWhile { body, .. }
                | Statement::ForC { body, .. }
                | Statement::For { body, .. }
                | Statement::Block { body } => walk(body, series, found),
                _ => {}
            }
        }
    }
    let mut found = false;
    walk(region, series, &mut found);
    found
}

/// True for the MINIMAL guarded free of the series — an If whose then-body
/// is exactly `free(<series>)` (comments aside) with no else branch. The
/// larger retCode guards also free the series on their way out, but they
/// return as well; only the standalone form is replayable on an inserted
/// failure path.
fn guard_frees_series(st: &Statement, series: &str) -> bool {
    let Statement::If {
        then_body,
        else_body,
        ..
    } = st
    else {
        return false;
    };
    if !else_body.is_empty() {
        return false;
    }
    let meaningful: Vec<&Statement> = then_body
        .iter()
        .filter(|s| !matches!(s, Statement::Comment(_)))
        .collect();
    matches!(meaningful.as_slice(),
        [Statement::Expr(Expr::FuncCall(name, args))]
            if name == "free"
                && matches!(args.first(), Some(Expr::Var(v)) if v == series))
}

/// A parameter-guarded fast-path block: `if( <param test> ) { ...; return; }`
/// with an empty else whose body calls a sub-indicator. This is a batch-only
/// specialization — BBANDS's SMA path reuses the moving average as the mean
/// instead of a separate STDDEV pass. It is EXCLUDED from the composed pipeline:
/// the stream composes the GENERAL path (below the block) for every parameter
/// value, and `stream_verify` proves that path bit-exact against the batch
/// fast-path across the swept parameters (SMA included). The sub-call requirement
/// tells it apart from a plain error guard (which never calls an indicator — the
/// G2 rule) and from a `period == 1` identity path (a plain copy, no sub-call).
///
/// The condition must be a compile-time parameter test: it names at least one
/// optional parameter and reads no series, pointers or calls (enum constants such
/// as `TA_MAType_SMA` are plain `Var`s and are allowed).
fn is_fastpath_block(
    st: &Statement,
    params: &BTreeSet<String>,
    lookup: &dyn CalleeLookup,
) -> bool {
    let Statement::If {
        condition,
        then_body,
        else_body,
        ..
    } = st
    else {
        return false;
    };
    if !else_body.is_empty() {
        return false;
    }
    let mut refs_param = false;
    let mut data_dependent = false;
    walk_expr(condition, &mut |e| match e {
        Expr::Var(v) if params.contains(v) => refs_param = true,
        Expr::ArrayAccess(..) | Expr::PointerDeref(_) | Expr::FuncCall(..) => {
            data_dependent = true;
        }
        _ => {}
    });
    if !refs_param || data_dependent {
        return false;
    }
    let ends_in_return = matches!(
        then_body
            .iter()
            .rev()
            .find(|s| !matches!(s, Statement::Comment(_))),
        Some(Statement::Return { .. })
    );
    ends_in_return && !find_indicator_calls(then_body, lookup).is_empty()
}

/// A composed-tail guard must be pure control flow over scalars: no
/// indicator calls, no writes to any materialized series or output array,
/// only frees / out-meta writes / returns / nested guards inside.
fn check_composed_guard(
    st: &Statement,
    defined: &BTreeSet<String>,
    lookup: &dyn CalleeLookup,
) -> Result<(), StreamError> {
    if !find_indicator_calls(std::slice::from_ref(st), lookup).is_empty() {
        return Err(StreamError::Unsupported(
            "sub-call nested inside an `if (rc == TA_SUCCESS) { ... }` success-guard; \
             flatten to a top-level `if (rc != TA_SUCCESS) return rc;` error-guard (see STDDEV)"
                .into(),
        ));
    }
    let mut bad = false;
    walk_assign_targets(st, &mut |t| {
        if let Expr::ArrayAccess(name, _) = t {
            if defined.contains(name) {
                bad = true;
            }
        }
    });
    if bad {
        return Err(StreamError::Unsupported(
            "composed tail guard writes a materialized series".into(),
        ));
    }
    Ok(())
}

/// Compact statement-kind label for diagnostics (a full IR Debug dump is
/// unreadable in a gate error).
fn stmt_kind(s: &Statement) -> &'static str {
    match s {
        Statement::VarDecl { .. } => "declaration",
        Statement::Assign { .. } => "assignment",
        Statement::If { .. } => "if",
        Statement::While { .. } => "while loop",
        Statement::DoWhile { .. } => "do-while loop",
        Statement::For { .. } | Statement::ForC { .. } => "for loop",
        Statement::Switch { .. } => "switch",
        Statement::Return { .. } => "return",
        Statement::Break => "break",
        Statement::Continue => "continue",
        Statement::Block { .. } => "block",
        Statement::Expr(_) => "expression statement",
        Statement::CircBuf(_) => "circbuf op",
        Statement::Comment(_) => "comment",
    }
}

/// Every distinct `Expr::FuncCall` that is an actual cross-indicator
/// INVOCATION in `stmts`, in first-seen order. A call counts only when the
/// lookup knows the name AND the argument count matches the callee's full TA
/// signature shape (`startIdx, endIdx, <inputs>, <opts>, outBegIdx,
/// outNBElement, <outputs>`). This is what separates a scalar math builtin
/// from a same-named indicator: `sqrt(tempReal)` (one scalar arg) is libm's
/// sqrt, not the `TA_SQRT` vector indicator (whose input-level name is also
/// `sqrt` but takes six args). Nested calls count too (walk_stmt_exprs
/// recurses into every expression).
fn find_indicator_calls(stmts: &[Statement], lookup: &dyn CalleeLookup) -> Vec<String> {
    let mut found: Vec<String> = Vec::new();
    for s in stmts {
        walk_stmt_exprs(s, &mut |e| {
            if let Expr::FuncCall(name, args) = e {
                if let Some(sig) = lookup.callee(name) {
                    let ta_arity = 2 + sig.n_inputs + sig.n_opts + 2 + sig.n_outputs;
                    if args.len() == ta_arity && !found.contains(name) {
                        found.push(name.clone());
                    }
                }
            }
        });
    }
    found
}

/// Classify one case arm. See `analyze_dispatch` for the strictness rules.
fn parse_dispatch_arm(
    label: &str,
    stmts: &[Statement],
    func: &FuncDef,
    lookup: &dyn CalleeLookup,
    ret_var: &str,
    bar_inputs: &[String],
    outputs: &[String],
) -> Result<DispatchArm, StreamError> {
    let reject = |callee: &str| DispatchArm {
        label: label.to_string(),
        callee: callee.to_string(),
        opt_args: Vec::new(),
        supported: false,
    };
    let callees = find_indicator_calls(stmts, lookup);
    let Some(callee) = callees.first().cloned() else {
        return Ok(reject("")); // no indicator call: pure reject arm
    };
    let sig = lookup.callee(&callee).expect("looked up above");

    // Strict whole-range delegation shape: exactly one statement,
    // `<retvar> = callee(startIdx, endIdx, <own inputs>, <pure opt args>,
    //                    outBegIdx, outNBElement, <own outputs>)`.
    let meaningful: Vec<&Statement> = stmts
        .iter()
        .filter(|s| !matches!(s, Statement::Comment(_)))
        .collect();
    let strict: Option<Vec<Expr>> = match meaningful.as_slice() {
        [Statement::Assign {
            target: Expr::Var(t),
            value: Expr::FuncCall(name, args),
            compound: false,
        }] if t == ret_var && *name == callee => {
            delegation_opt_args(args, func, &sig, bar_inputs, outputs)
        }
        _ => None,
    };
    if let (Some(opt_args), true) = (strict, sig.streaming) {
        return Ok(DispatchArm {
            label: label.to_string(),
            callee,
            opt_args,
            supported: true,
        });
    }
    // Not a supported delegation.
    let flagged: Vec<&str> = callees
        .iter()
        .filter(|c| lookup.callee(c).is_some_and(|s| s.streaming))
        .map(String::as_str)
        .collect();
    // A single flagged callee whose OUTPUT arity differs from this dispatch
    // func's can never be a 1:1 whole-range delegation: MA's MAMA arm feeds
    // mama's second (FAMA) output into a discarded scratch buffer, so it emits
    // one output from a two-output callee. That is an honest reject arm (MA
    // asks for batch on MAType_MAMA) even though `mama` itself streams —
    // structurally distinct from the hidden-delegation bug below.
    if let [only] = flagged.as_slice() {
        let osig = lookup.callee(only).expect("flagged callee resolves");
        if osig.n_outputs != outputs.len() {
            return Ok(reject(only));
        }
    }
    // If ANY indicator called in the arm is stream-flagged — not just the first
    // one seen — this is a hard gate error: an arm like `trima(...); dema(...)`
    // must never silently become a reject arm the verify precheck then blesses
    // (the strictness contract above).
    if let Some(f) = flagged.first() {
        return Err(StreamError::Unsupported(format!(
            "dispatch arm `{label}` calls stream-flagged `{f}` but is not a \
             whole-range delegation"
        )));
    }
    // Only unflagged callees: honest reject arm (TRIMA until it streamed).
    Ok(reject(&callee))
}

/// Validate a whole-range delegation call's argument list and return the
/// callee's optional-input argument expressions (positional).
fn delegation_opt_args(
    args: &[Expr],
    func: &FuncDef,
    sig: &CalleeSig,
    bar_inputs: &[String],
    outputs: &[String],
) -> Option<Vec<Expr>> {
    let n = args.len();
    if sig.n_inputs != bar_inputs.len()
        || sig.n_outputs != outputs.len()
        || n != 2 + sig.n_inputs + sig.n_opts + 2 + sig.n_outputs
    {
        return None;
    }
    let is_var = |e: &Expr, want: &str| matches!(e, Expr::Var(v) if v == want);
    if !is_var(&args[0], "startIdx") || !is_var(&args[1], "endIdx") {
        return None;
    }
    for (k, arr) in bar_inputs.iter().enumerate() {
        if !is_var(&args[2 + k], arr) {
            return None;
        }
    }
    let opt_base = 2 + sig.n_inputs;
    let out_meta = opt_base + sig.n_opts;
    if !is_var(&args[out_meta], "outBegIdx") || !is_var(&args[out_meta + 1], "outNBElement") {
        return None;
    }
    for (k, out) in outputs.iter().enumerate() {
        if !is_var(&args[out_meta + 2 + k], out) {
            return None;
        }
    }
    // Opt args must be pure over the caller's own params (or literals): they
    // are re-evaluated at Open time to open the sub-stream.
    let params: BTreeSet<String> = func
        .optional_inputs
        .iter()
        .map(|p| p.name.clone())
        .collect();
    let opt_args: Vec<Expr> = args[opt_base..out_meta].to_vec();
    for a in &opt_args {
        let mut names = BTreeSet::new();
        expr_var_names(a, &mut names);
        if !names.iter().all(|nm| params.contains(nm)) {
            return None;
        }
    }
    Some(opt_args)
}

/// The full argument list of the first `callee` invocation in `stmts` matching
/// its TA signature arity, or `None` if absent.
fn find_call_args(stmts: &[Statement], callee: &str, sig: &CalleeSig) -> Option<Vec<Expr>> {
    let arity = 2 + sig.n_inputs + sig.n_opts + 2 + sig.n_outputs;
    let mut result: Option<Vec<Expr>> = None;
    for s in stmts {
        walk_stmt_exprs(s, &mut |e| {
            walk_expr(e, &mut |x| {
                if result.is_none() {
                    if let Expr::FuncCall(name, args) = x {
                        if name == callee && args.len() == arity {
                            result = Some(args.clone());
                        }
                    }
                }
            });
        });
    }
    result
}

/// Recognize a variable-period moving average (MAVP): two real inputs (a price
/// series and a per-bar period selector), a `MAType` enum param, exactly two
/// integer bound params (min/max period), one real output, and a whole-range
/// delegation to a single streaming 1-input/1-output MAType-dispatch callee
/// (`ma`) whose period argument varies per bar. Returns [`StreamError::NoSteadyLoop`]
/// when the body is not this shape (the gate then falls through to composed).
/// See [`PeriodBankPlan`] for the streaming model.
pub fn analyze_period_bank<'a>(
    func: &'a FuncDef,
    lookup: &dyn CalleeLookup,
) -> Result<PeriodBankPlan<'a>, StreamError> {
    let inputs = input_array_names(func);
    if inputs.len() != 2 || func.outputs.len() != 1 {
        return Err(StreamError::NoSteadyLoop);
    }
    // Exactly one MAType enum param + exactly two integer (period bound) params.
    let Some(matype) = func
        .optional_inputs
        .iter()
        .find(|p| matches!(&p.param_type, ParamType::Enum(e) if e == "MAType"))
    else {
        return Err(StreamError::NoSteadyLoop);
    };
    let ints: Vec<&str> = func
        .optional_inputs
        .iter()
        .filter(|p| matches!(p.param_type, ParamType::Integer))
        .map(|p| p.name.as_str())
        .collect();
    if ints.len() != 2 {
        return Err(StreamError::NoSteadyLoop);
    }
    // A single streaming callee, itself a 1-input / 1-output / 2-opt MAType
    // dispatch (so a per-period sub-MA can be opened from its public stream).
    let streaming_names: Vec<String> = find_indicator_calls(&func.body, lookup)
        .into_iter()
        .filter(|c| lookup.callee(c).is_some_and(|s| s.streaming))
        .collect();
    let [callee] = streaming_names.as_slice() else {
        return Err(StreamError::NoSteadyLoop);
    };
    let sig = lookup.callee(callee).expect("streaming callee resolves");
    if sig.n_inputs != 1 || sig.n_outputs != 1 || sig.n_opts != 2 {
        return Err(StreamError::NoSteadyLoop);
    }
    // Extract the callee's actual args to learn its price input and opt roles.
    let args = find_call_args(&func.body, callee, &sig).ok_or(StreamError::NoSteadyLoop)?;
    let Expr::Var(price_input) = &args[2] else {
        return Err(StreamError::NoSteadyLoop);
    };
    if !inputs.iter().any(|i| i == price_input) {
        return Err(StreamError::NoSteadyLoop);
    }
    let Some(period_input) = inputs.iter().find(|i| *i != price_input) else {
        return Err(StreamError::NoSteadyLoop);
    };
    let opt_base = 2 + sig.n_inputs;
    let callee_opts: Vec<PeriodBankArg> = args[opt_base..opt_base + sig.n_opts]
        .iter()
        .map(|e| {
            if matches!(e, Expr::Var(v) if *v == matype.name) {
                PeriodBankArg::MAType
            } else {
                PeriodBankArg::Period
            }
        })
        .collect();
    // Exactly one period slot and one MAType slot.
    if callee_opts.iter().filter(|a| **a == PeriodBankArg::MAType).count() != 1
        || callee_opts.iter().filter(|a| **a == PeriodBankArg::Period).count() != 1
    {
        return Err(StreamError::NoSteadyLoop);
    }
    Ok(PeriodBankPlan {
        func,
        callee: callee.clone(),
        callee_opts,
        price_input: price_input.clone(),
        period_input: period_input.clone(),
        min_param: ints[0].to_string(),
        max_param: ints[1].to_string(),
        matype_param: matype.name.clone(),
        output: func.outputs[0].name.clone(),
    })
}

/// Validate that a `streaming: true` function is still analyzable. Any
/// failure is a generation-time error (the maintenance-coupling gate from
/// the proposal): a batch rewrite that breaks stream analyzability fails
/// HERE, not at release. The tier is derived, never declared.
#[allow(clippy::too_many_lines)]
pub fn validate_streamable<'a>(
    func: &'a FuncDef,
    lookup: &dyn CalleeLookup,
) -> Result<StreamPlan<'a>, String> {
    struct GateNames;
    impl NameMap for GateNames {
        fn state(&self, name: &str) -> String {
            format!("sp->{name}")
        }
        fn bar(&self, array: &str) -> String {
            array.to_string()
        }
        fn output(&self, name: &str) -> Expr {
            Expr::PointerDeref(name.to_string())
        }
        fn ring_buf(&self, var: &str, array: &str) -> String {
            format!("sp->ring_{var}_{array}")
        }
        fn ring_pos(&self, var: &str) -> String {
            format!("sp->ringPos_{var}")
        }
        fn ring_lag(&self, var: &str) -> String {
            format!("sp->ringLag_{var}")
        }
        fn ring_cap(&self, var: &str) -> String {
            format!("sp->ringCap_{var}")
        }
        fn win_buf(&self, var: &str, array: &str) -> String {
            format!("sp->win_{var}_{array}")
        }
        fn win_pos(&self, var: &str) -> String {
            format!("sp->winPos_{var}")
        }
        fn win_cap(&self, var: &str) -> String {
            format!("sp->winCap_{var}")
        }
        fn circ_buf(&self, storage: &str) -> String {
            format!("sp->cb_{storage}")
        }
        fn extrema_buf(&self, array: &str) -> String {
            format!("sp->x_{array}")
        }
        fn extrema_cap(&self) -> String {
            "sp->xCap".to_string()
        }
    }
    // Loop tier first (the established 131), dispatch second: a body with a
    // steady loop is never a dispatch, so the order only decides which error
    // is reported when both fail.
    let loop_err = match analyze(func) {
        Ok(model) => {
            // The transition must BUILD, too — analysis success alone would
            // let a seeded function pass the gate and then panic in the
            // emitter.
            build_transition(&model, &GateNames).map_err(|e| {
                format!(
                    "{}: streamable by analysis but the transition cannot be built: {e}",
                    func.name
                )
            })?;
            return Ok(StreamPlan::Loop(model));
        }
        Err(e) => e,
    };
    // Dual-mode (DI/DM): a param-selected pair of inline steady loops. Tried
    // after the single-loop analysis fails (so a T3-style identity path stays a
    // Loop), before dispatch/composed (DI/DM are neither). NoSteadyLoop = "not
    // dual-mode-shaped": fall through. Any other error means the shape matched
    // but an arm is unstreamable — surface it loudly (dispatch-strictness parity).
    match analyze_dual_mode(func) {
        Ok(plan) => {
            build_transition(&plan.mode_a, &GateNames).map_err(|e| {
                format!(
                    "{}: dual-mode mode-A streamable by analysis but the transition cannot be built: {e}",
                    func.name
                )
            })?;
            build_transition(&plan.mode_b, &GateNames).map_err(|e| {
                format!(
                    "{}: dual-mode mode-B streamable by analysis but the transition cannot be built: {e}",
                    func.name
                )
            })?;
            return Ok(StreamPlan::DualMode(plan));
        }
        Err(StreamError::NoSteadyLoop) => {}
        Err(dual_err) => {
            return Err(format!(
                "{}: YAML declares `streaming: true` but the dual-mode body is not streamable: {dual_err}",
                func.name
            ));
        }
    }
    // General-arm (MIDPRICE): a `param <= literal` fast-path split whose arms are
    // bit-identical — stream only the general (else) arm. After dual-mode (the
    // early-return / genuine-branch forms), before dispatch/composed.
    match analyze_fastpath_skip(func) {
        Ok(plan) => {
            build_transition(&plan.model, &GateNames).map_err(|e| {
                format!(
                    "{}: general-arm streamable by analysis but the transition cannot be built: {e}",
                    func.name
                )
            })?;
            return Ok(StreamPlan::FastPathSkip(plan));
        }
        Err(StreamError::NoSteadyLoop) => {}
        Err(arm_err) => {
            return Err(format!(
                "{}: YAML declares `streaming: true` but the general-arm body is not streamable: {arm_err}",
                func.name
            ));
        }
    }
    match analyze_dispatch(func, lookup) {
        Ok(plan) => return Ok(StreamPlan::Dispatch(plan)),
        // NoSteadyLoop = "no switch found": the body is not a dispatch;
        // fall through to the composed analysis. Any other dispatch error
        // means the body IS switch-shaped and that error is the actionable
        // one (e.g. a stream-flagged callee arm losing its delegation shape
        // must fail loudly, never fall back to the generic loop error).
        Err(StreamError::NoSteadyLoop) => {}
        Err(dispatch_err) => {
            return Err(format!(
                "{}: YAML declares `streaming: true` but the dispatch body is not streamable: {dispatch_err}",
                func.name
            ));
        }
    }
    // Period-bank (MAVP): a variable-per-bar-period moving average, streamed as a
    // bank of the callee's per-period sub-streams. Tried after dispatch (it has no
    // switch) and before composed (its `ma` call inside a loop is not the composed
    // producer-plus-tail shape). NoSteadyLoop = "not period-bank-shaped".
    match analyze_period_bank(func, lookup) {
        Ok(plan) => return Ok(StreamPlan::PeriodBank(plan)),
        Err(StreamError::NoSteadyLoop) => {}
        Err(bank_err) => {
            return Err(format!(
                "{}: YAML declares `streaming: true` but the period-bank body is not streamable: {bank_err}",
                func.name
            ));
        }
    }
    match analyze_composed(func, lookup) {
        Ok(plan) => {
            // The producer transition must BUILD, too (same chicken-egg gate
            // as the loop tier); loopless pipelines have no producer.
            if let Some(producer) = &plan.producer {
                build_transition(producer, &GateNames).map_err(|e| {
                    format!(
                        "{}: composed by analysis but the producer transition cannot be built: {e}",
                        func.name
                    )
                })?;
            }
            // The per-bar pipeline must resolve every sub source, align source,
            // and output to a materialized series — the same cur-map the
            // emitter walks, so a plan the emitter would panic on is rejected
            // loudly here instead (the loopless tier has no producer transition
            // to lean on, so this is its build gate).
            check_composed_emittable(&plan).map_err(|e| {
                format!(
                    "{}: composed by analysis but the per-bar pipeline cannot be emitted: {e}",
                    func.name
                )
            })?;
            Ok(StreamPlan::Composed(plan))
        }
        // Not composed-shaped either: the loop error names the blocker.
        Err(StreamError::NoSteadyLoop) => Err(format!(
            "{}: YAML declares `streaming: true` but the function is not streamable at stage 1: {loop_err}",
            func.name
        )),
        Err(composed_err) => Err(format!(
            "{}: YAML declares `streaming: true` but the composed body is not streamable: {composed_err}",
            func.name
        )),
    }
}

/// Scan the transition statements for array accesses and stateful calls.
/// Returns (per-array max lag depth, output-index variables).
fn scan_accesses(
    steady_stmts: &[Statement],
    bar_inputs: &[String],
    outputs: &[String],
    cursor: &str,
) -> Result<ScannedAccesses, StreamError> {
    let mut acc = AccessAcc::default();
    let mut out_index_vars: BTreeSet<String> = BTreeSet::new();
    let mut access_err: Option<StreamError> = None;

    // Inner-loop counters usable as rescan-window offsets (var -> bound).
    let window_bounds = merge_window_bounds(steady_stmts)?;
    for s in steady_stmts {
        walk_stmt_exprs(s, &mut |e| {
            walk_expr(e, &mut |x| {
                if access_err.is_some() {
                    return;
                }
                if let Expr::ArrayAccess(name, idx) = x {
                    if bar_inputs.iter().any(|i| i == name) {
                        access_err =
                            record_input_access(name, idx, cursor, &window_bounds, &mut acc);
                    } else if outputs.iter().any(|o| o == name) {
                        if is_prev_output_read(idx) {
                            // Previous-output feedback read (lastOut state).
                            return;
                        }
                        let idx_var = match idx.as_ref() {
                            Expr::Var(v) => Some(v.clone()),
                            Expr::PostIncrement(b) => match b.as_ref() {
                                Expr::Var(v) => Some(v.clone()),
                                _ => None,
                            },
                            _ => None,
                        };
                        match idx_var {
                            Some(v) => {
                                out_index_vars.insert(v);
                            }
                            None => {
                                access_err = Some(StreamError::UnsupportedAccess(format!(
                                    "{name}[non-var index]"
                                )));
                            }
                        }
                    }
                } else if let Expr::FuncCall(fname, _) = x {
                    if is_stateful_call(fname) {
                        access_err = Some(StreamError::UnsupportedCall(fname.clone()));
                    }
                }
            });
        });
        if let Some(e) = access_err {
            return Err(e);
        }
    }
    let window_specs: Vec<(String, Expr, BTreeSet<String>)> = acc
        .windows
        .into_iter()
        .map(|(v, arrs)| {
            let cap = window_bounds[&v].clone();
            (v, cap, arrs)
        })
        .collect();
    Ok(ScannedAccesses {
        lags: acc.lags,
        trailing: acc.trailing,
        ring_back: acc.ring_back,
        ring_fwd: acc.ring_fwd,
        windows: window_specs,
        out_index_vars,
    })
}

/// Accumulators for the input-array access scan.
#[derive(Default)]
struct AccessAcc {
    lags: BTreeMap<String, i64>,
    trailing: BTreeMap<String, BTreeSet<String>>,
    ring_back: BTreeMap<String, i64>,
    ring_fwd: BTreeMap<String, i64>,
    windows: BTreeMap<String, BTreeSet<String>>,
}

/// Record one bar-input array access into the scan accumulators; returns an
/// error to raise when the index form is unsupported.
fn record_input_access(
    name: &str,
    idx: &Expr,
    cursor: &str,
    window_bounds: &BTreeMap<String, Expr>,
    acc: &mut AccessAcc,
) -> Option<StreamError> {
    match classify_input_index(idx, cursor) {
        InputIndex::Current => None,
        InputIndex::Lag(k) => {
            let d = acc.lags.entry(name.to_string()).or_insert(0);
            *d = (*d).max(k);
            None
        }
        InputIndex::OtherVar(v) => {
            acc.trailing.entry(v).or_default().insert(name.to_string());
            None
        }
        InputIndex::OtherVarLag(v, k) => {
            acc.trailing
                .entry(v.clone())
                .or_default()
                .insert(name.to_string());
            if k >= 0 {
                let b = acc.ring_back.entry(v).or_insert(0);
                *b = (*b).max(k);
            } else {
                // Forward read: capacity is unchanged but the layout must be
                // absolute-mod.
                acc.ring_back.entry(v.clone()).or_insert(0);
                let f = acc.ring_fwd.entry(v).or_insert(0);
                *f = (*f).max(-k);
            }
            None
        }
        InputIndex::OtherVarWinLag(v, w) => match window_bounds.get(&w) {
            Some(Expr::IntLiteral(bound)) => {
                acc.trailing
                    .entry(v.clone())
                    .or_default()
                    .insert(name.to_string());
                let b = acc.ring_back.entry(v).or_insert(0);
                // Counter values span [0, bound-1].
                *b = (*b).max(bound - 1).max(1);
                None
            }
            _ => Some(StreamError::UnsupportedAccess(format!(
                "{name}[{v} - {w}] without a literal loop bound"
            ))),
        },
        InputIndex::WindowVar(w) => {
            if window_bounds.contains_key(&w) {
                acc.windows.entry(w).or_default().insert(name.to_string());
                None
            } else {
                Some(StreamError::UnsupportedAccess(format!(
                    "{name}[cursor - {w}] with unbounded offset var"
                )))
            }
        }
        InputIndex::Unsupported => Some(StreamError::UnsupportedAccess(format!("{name}[{idx:?}]"))),
    }
}

/// Collect and merge inner-loop counter bounds: a counter bound by several
/// loops keeps the widest literal bound; mixed literal/expression bounds are
/// unsupported.
fn merge_window_bounds(steady_stmts: &[Statement]) -> Result<BTreeMap<String, Expr>, StreamError> {
    let mut bound_list: Vec<(String, Expr)> = Vec::new();
    collect_window_bounds(steady_stmts, &mut bound_list);
    let mut window_bounds: BTreeMap<String, Expr> = BTreeMap::new();
    for (v, e) in bound_list {
        match window_bounds.get(&v) {
            None => {
                window_bounds.insert(v, e);
            }
            Some(prev) if format!("{prev:?}") == format!("{e:?}") => {}
            Some(Expr::IntLiteral(p)) => {
                if let Expr::IntLiteral(nn) = e {
                    let widest = (*p).max(nn);
                    window_bounds.insert(v, Expr::IntLiteral(widest));
                } else {
                    return Err(StreamError::Unsupported(format!(
                        "window counter `{v}` has inconsistent loop bounds"
                    )));
                }
            }
            Some(_) => {
                return Err(StreamError::Unsupported(format!(
                    "window counter `{v}` has inconsistent loop bounds"
                )));
            }
        }
    }
    Ok(window_bounds)
}

/// Collect inner-loop counter bounds usable as rescan windows: For-countdown
/// (`for(i=E; i--!=0;)` — body sees i in [0, E-1]) and ForC ascending from 0
/// (`for(i=0; i<E; i++)`). Returns var -> bound expr; a var bound twice with
/// a different expr is rejected by the caller via the Debug-format compare.
fn collect_window_bounds(stmts: &[Statement], out: &mut Vec<(String, Expr)>) {
    for st in stmts {
        match st {
            // NOTE: Statement::For (the countdown-only IR node) is never
            // produced by the C parser (all for-loops parse as ForC); its
            // body range is [1, count], NOT [0, count-1], so registering it
            // here would be an off-by-one. Recurse only.
            Statement::For { body, .. } => {
                collect_window_bounds(body, out);
            }
            Statement::ForC {
                init,
                condition,
                body,
                ..
            } => {
                // Ascending: for (v = 0; v < E; v++) — offsets 0..E-1.
                if let (
                    Statement::Assign {
                        target: Expr::Var(v),
                        value: Expr::IntLiteral(0),
                        ..
                    },
                    Expr::BinOp(l, BinOp::Less, r),
                ) = (init.as_ref(), condition)
                {
                    if matches!(l.as_ref(), Expr::Var(lv) if lv == v) {
                        out.push((v.clone(), (**r).clone()));
                    }
                }
                // Descending inclusive: for (v = E; v >= B; v--) with a
                // literal floor B >= 0 — offsets stay within 0..E, so the
                // window bound (exclusive) is E + 1.
                if let (
                    Statement::Assign {
                        target: Expr::Var(v),
                        value: bound,
                        ..
                    },
                    Expr::BinOp(l, BinOp::GreaterEq, r),
                ) = (init.as_ref(), condition)
                {
                    if matches!(r.as_ref(), Expr::IntLiteral(b) if *b >= 0)
                        && matches!(l.as_ref(), Expr::Var(lv) if lv == v)
                    {
                        let excl = match bound {
                            Expr::IntLiteral(k) => Expr::IntLiteral(k + 1),
                            other => Expr::BinOp(
                                Box::new(other.clone()),
                                BinOp::Add,
                                Box::new(Expr::IntLiteral(1)),
                            ),
                        };
                        out.push((v.clone(), excl));
                    }
                }
                // Countdown: for (v = E; v-- != 0;) — body sees v in 0..E-1.
                if let (
                    Statement::Assign {
                        target: Expr::Var(v),
                        value: bound,
                        ..
                    },
                    Expr::BinOp(l, BinOp::NotEq | BinOp::Greater, r),
                ) = (init.as_ref(), condition)
                {
                    if matches!(r.as_ref(), Expr::IntLiteral(0))
                        && matches!(
                            l.as_ref(),
                            Expr::PostDecrement(b) if matches!(b.as_ref(), Expr::Var(lv) if lv == v)
                        )
                        && !matches!(bound, Expr::IntLiteral(0))
                    {
                        out.push((v.clone(), bound.clone()));
                    }
                }
                collect_window_bounds(body, out);
            }
            Statement::While { body, .. }
            | Statement::DoWhile { body, .. }
            | Statement::Block { body } => collect_window_bounds(body, out),
            Statement::If {
                then_body,
                else_body,
                ..
            } => {
                collect_window_bounds(then_body, out);
                collect_window_bounds(else_body, out);
            }
            Statement::Switch { cases, default, .. } => {
                for (_, sts) in cases {
                    collect_window_bounds(sts, out);
                }
                collect_window_bounds(default, out);
            }
            _ => {}
        }
    }
}

/// Locate the steady loop and derive (form, transition statements, cursor,
/// countdown counter).
fn extract_steady(
    body: &[Statement],
    bar_inputs: &[String],
) -> Result<(LoopForm, Vec<Statement>, String, Option<String>), StreamError> {
    let steady = find_steady_loop(body)?;

    // Transition source: loop body (+ `for` increment clause).
    let mut steady_stmts: Vec<Statement> = steady.body.to_vec();
    if let Some(u) = steady.for_update {
        steady_stmts.push(u.clone());
    }

    let (cursor, counter) = if steady.form == LoopForm::Countdown {
        let counter = countdown_counter(steady.condition).ok_or(StreamError::NoSteadyLoop)?;
        let c = countdown_cursor(&steady_stmts, bar_inputs, &counter)?;
        (c, Some(counter))
    } else {
        let c = endidx_cursor(steady.condition).ok_or(StreamError::NoSteadyLoop)?;
        (c, None)
    };
    Ok((steady.form, steady_stmts, cursor, counter))
}

/// Bundled inputs for local classification (analyze() internals).
struct ClassifyCtx<'a> {
    body: &'a [Statement],
    steady_stmts: &'a [Statement],
    func: &'a FuncDef,
    cursor: &'a str,
    counter: Option<&'a str>,
    out_index_vars: &'a BTreeSet<String>,
    bar_inputs: &'a [String],
    outputs: &'a [String],
    circ_extra: &'a [(String, VarType)],
    /// Synthetic carried parity field, if any: skipped by the candidate scan
    /// (it has no VarDecl; `analyze_region_scoped` injects it into state).
    parity_field: Option<&'a str>,
}

type Classified = (Vec<ScalarField>, Vec<ScalarField>);

/// Classify locals; when the bookkeeping-purity gate trips on a cached-index
/// automaton, retry in absolute-index (extrema) mode: the cursor and every
/// index local become carried state and inputs read through the automaton
/// ring.
fn classify_or_extrema(
    ctx: &ClassifyCtx,
    ring_vars: &BTreeSet<String>,
    trailing: &BTreeMap<String, BTreeSet<String>>,
    windows: &[WindowSpec],
    circs: &[CircState],
) -> Result<(Option<ExtremaState>, Classified), StreamError> {
    let run = |rings: &BTreeSet<String>| {
        classify_locals(
            ctx.body,
            ctx.steady_stmts,
            ctx.func,
            ctx.cursor,
            ctx.counter,
            ctx.out_index_vars,
            rings,
            ctx.bar_inputs,
            ctx.outputs,
            ctx.circ_extra,
            ctx.parity_field,
        )
    };
    match run(ring_vars) {
        Ok(st) => Ok((None, st)),
        Err(StreamError::Unsupported(ref msg)) if msg.contains("index bookkeeping") => {
            if !windows.is_empty() || !circs.is_empty() || ctx.counter.is_some() {
                return Err(StreamError::Unsupported(
                    "extrema automaton mixed with other buffer forms".into(),
                ));
            }
            let ex = build_extrema(
                ctx.steady_stmts,
                ctx.cursor,
                ctx.out_index_vars,
                trailing,
                ctx.bar_inputs,
            )?;
            let mut st = run(&BTreeSet::new())?; // index locals = plain int state
            if !st.0.iter().any(|(n2, _)| n2 == ctx.cursor) {
                st.0.push((ctx.cursor.to_string(), VarType::Integer));
            }
            Ok((Some(ex), st))
        }
        Err(e) => Err(e),
    }
}

/// Build the absolute-index automaton description. The paced variable is
/// the one advanced exactly once per bar at the loop's top level (the
/// window start); everything else that indexes the inputs is a free
/// automaton index. Reads through those indices are trusted to stay inside
/// `[trailing, cursor]` — the batch automaton guarantees it, and any
/// violation reads a stale ring slot and fails the bitwise gate loudly.
fn build_extrema(
    steady_stmts: &[Statement],
    cursor: &str,
    out_index_vars: &BTreeSet<String>,
    trailing: &BTreeMap<String, BTreeSet<String>>,
    bar_inputs: &[String],
) -> Result<ExtremaState, StreamError> {
    // Paced vars: top-level `v = v + 1` or `v++` (excluding cursor/outIdx).
    let mut paced: Vec<String> = Vec::new();
    for st in steady_stmts {
        let v = match st {
            Statement::Assign {
                target: Expr::Var(v),
                value:
                    Expr::BinOp(l, BinOp::Add, r),
                ..
            } if matches!(l.as_ref(), Expr::Var(lv) if lv == v)
                && matches!(r.as_ref(), Expr::IntLiteral(1)) =>
            {
                Some(v.clone())
            }
            Statement::Expr(Expr::PostIncrement(b) | Expr::PreIncrement(b)) => match b.as_ref() {
                Expr::Var(v) => Some(v.clone()),
                _ => None,
            },
            _ => None,
        };
        if let Some(v) = v {
            if v != cursor && !out_index_vars.contains(&v) && !paced.contains(&v) {
                paced.push(v);
            }
        }
    }
    // The window start is the paced var that is NOT itself an input reader
    // (cached indices read inputs; the trailing bound usually does not) —
    // when ambiguous, a single paced var wins.
    // Strict: exactly one paced variable. A heuristic pick among several
    // could silently choose a wrong window start (wrong ring capacity);
    // nothing in the corpus needs more than one.
    if paced.len() != 1 {
        return Err(StreamError::Unsupported(format!(
            "extrema automaton: expected exactly one window-start variable, found {paced:?}"
        )));
    }
    let trailing_var = paced.remove(0);
    let _ = &trailing; // arrays derived below; map retained for callers
    let index_vars: Vec<String> = trailing
        .keys()
        .filter(|v| **v != trailing_var)
        .cloned()
        .collect();
    // The ring must cover every input array the loop reads.
    let mut used: BTreeSet<String> = BTreeSet::new();
    for st in steady_stmts {
        walk_stmt_exprs(st, &mut |e| {
            walk_expr(e, &mut |x| {
                if let Expr::ArrayAccess(n2, _) = x {
                    if bar_inputs.iter().any(|a| a == n2) {
                        used.insert(n2.clone());
                    }
                }
            });
        });
    }
    Ok(ExtremaState {
        trailing: trailing_var,
        index_vars,
        arrays: bar_inputs.iter().filter(|a| used.contains(*a)).cloned().collect(),
    })
}

/// The CIRCBUF index scalars are read/written inside the opaque
/// CIRCBUF_NEXT statement (invisible to the expression walkers): force both
/// into carried state so the transition's expansion has its fields.
fn force_circ_index_state(
    circs: &[CircState],
    state: &mut Vec<ScalarField>,
    temps: &[ScalarField],
) {
    for c in circs {
        for n2 in [format!("{}_Idx", c.id), format!("maxIdx_{}", c.id)] {
            if !state.iter().any(|(sn, _)| *sn == n2)
                && !temps.iter().any(|(tn, _)| *tn == n2)
            {
                state.push((n2, VarType::Integer));
            }
        }
    }
}

/// CIRCBUF buffers the steady loop touches, plus the synthetic decls for
/// the index scalars / storage pointers the macros introduce.
fn discover_circs(
    body: &[Statement],
    steady_stmts: &[Statement],
) -> (Vec<CircState>, Vec<(String, VarType)>) {
    let mut all_circs: Vec<CircState> = Vec::new();
    collect_circ_prologs(body, &mut all_circs);
    let mut loop_names = BTreeSet::new();
    for st in steady_stmts {
        stmt_var_names(st, &mut loop_names);
    }
    let circs: Vec<CircState> = all_circs
        .into_iter()
        .filter(|c| {
            circ_storages(c).iter().any(|(n, _)| loop_names.contains(n))
                || loop_names.contains(&format!("{}_Idx", c.id))
        })
        .collect();
    let circ_extra: Vec<(String, VarType)> = circs
        .iter()
        .flat_map(|c| {
            let mut v = vec![
                (format!("{}_Idx", c.id), VarType::Integer),
                (format!("maxIdx_{}", c.id), VarType::Integer),
            ];
            for (storage, _) in circ_storages(c) {
                v.push((storage, VarType::RealPointer)); // excluded from scalars
            }
            v
        })
        .collect();
    (circs, circ_extra)
}

/// Storage buffer names of a CIRCBUF (mirrors backends::c::circbuf_fields):
/// Plain -> [`id`]; Class -> [`id_field`, ...].
pub fn circ_storages(c: &CircState) -> Vec<(String, VarType)> {
    match &c.layout {
        CircBufLayout::Plain(t) => vec![(c.id.clone(), t.clone())],
        CircBufLayout::Class(fields) => fields
            .iter()
            .map(|(f, t)| (format!("{}_{f}", c.id), t.clone()))
            .collect(),
    }
}

/// Collect CIRCBUF prologs anywhere in the body (they are hoisted decls).
fn collect_circ_prologs(stmts: &[Statement], out: &mut Vec<CircState>) {
    for st in stmts {
        match st {
            Statement::CircBuf(CircBuf::Prolog { id, layout, .. }) => {
                if !out.iter().any(|c| c.id == *id) {
                    out.push(CircState {
                        id: id.clone(),
                        layout: layout.clone(),
                    });
                }
            }
            Statement::While { body, .. }
            | Statement::DoWhile { body, .. }
            | Statement::For { body, .. }
            | Statement::Block { body } => collect_circ_prologs(body, out),
            Statement::If {
                then_body,
                else_body,
                ..
            } => {
                collect_circ_prologs(then_body, out);
                collect_circ_prologs(else_body, out);
            }
            Statement::Switch { cases, default, .. } => {
                for (_, sts) in cases {
                    collect_circ_prologs(sts, out);
                }
                collect_circ_prologs(default, out);
            }
            Statement::ForC { init, body, .. } => {
                collect_circ_prologs(std::slice::from_ref(init), out);
                collect_circ_prologs(body, out);
            }
            _ => {}
        }
    }
}

/// A window counter cannot double as trailing index or cursor.
fn check_window_disjoint(
    scanned_windows: &[(String, Expr, BTreeSet<String>)],
    ring_vars: &BTreeSet<String>,
    cursor: &str,
) -> Result<(), StreamError> {
    for (w, _, _) in scanned_windows {
        if ring_vars.contains(w) || w == cursor {
            return Err(StreamError::Unsupported(format!(
                "`{w}` is both a window counter and another index kind"
            )));
        }
    }
    Ok(())
}

/// Windows in a stable order, arrays in signature order.
fn assemble_windows(
    scanned: Vec<(String, Expr, BTreeSet<String>)>,
    bar_inputs: &[String],
) -> Vec<WindowSpec> {
    scanned
        .into_iter()
        .map(|(var, cap, arrs)| WindowSpec {
            var,
            cap,
            arrays: bar_inputs
                .iter()
                .filter(|a| arrs.contains(*a))
                .cloned()
                .collect(),
        })
        .collect()
}

/// T3 when any buffer state exists; T1 for stateless maps; T2 otherwise.
#[allow(clippy::too_many_arguments)]
fn derive_tier(
    state: &[ScalarField],
    lags: &[LagSlot],
    rings: &[RingSpec],
    windows: &[WindowSpec],
    circs: &[CircState],
    counter: Option<&str>,
    extrema: Option<&ExtremaState>,
) -> StreamTier {
    if extrema.is_some() {
        StreamTier::T4
    } else if !rings.is_empty() || !windows.is_empty() || !circs.is_empty() {
        StreamTier::T3
    } else if state.is_empty() && lags.is_empty() && counter.is_none() {
        StreamTier::T1
    } else {
        StreamTier::T2
    }
}

/// Rings in a stable order: by variable name, arrays in signature order.
fn assemble_rings(
    trailing: BTreeMap<String, BTreeSet<String>>,
    ring_back: &BTreeMap<String, i64>,
    ring_fwd: &BTreeMap<String, i64>,
    bar_inputs: &[String],
) -> Vec<RingSpec> {
    trailing
        .into_iter()
        .map(|(var, arrs)| RingSpec {
            back: ring_back
                .get(&var)
                .copied()
                .unwrap_or(0)
                .max(i64::from(ring_fwd.contains_key(&var))),
            fwd: ring_fwd.get(&var).copied().unwrap_or(0),
            var,
            arrays: bar_inputs
                .iter()
                .filter(|a| arrs.contains(*a))
                .cloned()
                .collect(),
        })
        .collect()
}

/// Result of scanning the transition statements for array accesses.
struct ScannedAccesses {
    /// Per-array max bounded look-back depth (`in[cursor - K]`).
    lags: BTreeMap<String, i64>,
    /// Trailing index variables and the input arrays each one reads.
    trailing: BTreeMap<String, BTreeSet<String>>,
    /// Per trailing var: max constant back-offset (`in[var - K]`).
    ring_back: BTreeMap<String, i64>,
    /// Per trailing var: max forward offset (`in[var + K]`).
    ring_fwd: BTreeMap<String, i64>,
    /// Rescan windows: (counter var, bound expr, arrays read).
    windows: Vec<(String, Expr, BTreeSet<String>)>,
    /// Output-index variables (dropped in the transition).
    out_index_vars: BTreeSet<String>,
}

/// A named scalar with its declared type — one state field or temp local.
type ScalarField = (String, VarType);

/// Classify the loop-referenced locals into carried state and transition
/// temps (both in declaration order).
#[allow(clippy::too_many_arguments)]
fn classify_locals(
    body: &[Statement],
    steady_stmts: &[Statement],
    func: &FuncDef,
    cursor: &str,
    counter: Option<&str>,
    out_index_vars: &BTreeSet<String>,
    ring_vars: &BTreeSet<String>,
    bar_inputs: &[String],
    outputs: &[String],
    circ_extra: &[(String, VarType)],
    parity_field: Option<&str>,
) -> Result<(Vec<ScalarField>, Vec<ScalarField>), StreamError> {
    let mut decls = BTreeMap::new();
    collect_var_decls(body, &mut decls);
    // CIRCBUF macros introduce names without VarDecl statements: the shared
    // index scalars (state candidates) and the storage pointers (excluded —
    // the buffers themselves are captured separately).
    let mut circ_buffers: BTreeSet<String> = BTreeSet::new();
    for (n2, t) in circ_extra {
        if matches!(t, VarType::RealPointer) {
            circ_buffers.insert(n2.clone());
        } else {
            decls.insert(n2.clone(), t.clone());
        }
    }

    let mut loop_vars = BTreeSet::new();
    for s in steady_stmts {
        stmt_var_names(s, &mut loop_vars);
    }

    let param_names: BTreeSet<String> = func
        .optional_inputs
        .iter()
        .map(|p| p.name.clone())
        .chain(func.private_extra_params.iter().map(|(n, _)| n.clone()))
        .collect();

    // Candle-setting locals (BodyDoji_rangeType, ...) are hoisted per-call
    // constants in batch; the stream emitters unpack them at the top of both
    // Open and StepInternal (settings-stability rule), so they are neither
    // state nor temps here.
    let candle_locals: BTreeSet<String> = {
        let mut set = BTreeSet::new();
        for base in crate::candle_settings::detect_candle_settings(steady_stmts) {
            for prop in ["rangeType", "avgPeriod", "factor"] {
                set.insert(format!("{base}_{prop}"));
            }
        }
        set
    };
    reject_candle_local_writes(steady_stmts, &candle_locals)?;

    // The batch index space must not leak into the transition.
    for n in ["endIdx", "startIdx"] {
        if loop_vars.contains(n) && n != cursor && Some(n) != counter {
            return Err(StreamError::Unsupported(format!(
                "steady loop references `{n}`"
            )));
        }
    }

    let mut candidates: Vec<String> = Vec::new();
    for v in &loop_vars {
        if *v == cursor
            || Some(v.as_str()) == counter
            || out_index_vars.contains(v)
            || ring_vars.contains(v)
            || circ_buffers.contains(v)
            || param_names.contains(v)
            || bar_inputs.contains(v)
            || outputs.contains(v)
            || candle_locals.contains(v)
            || Some(v.as_str()) == parity_field
            || v == "endIdx"
            || v == "startIdx"
        {
            continue;
        }
        match decls.get(v) {
            Some(VarType::Real | VarType::Integer | VarType::Index) => candidates.push(v.clone()),
            // Fixed-size local arrays (CDL per-candle period totals) carry
            // whole; the emitter copies them by memcpy / struct assignment.
            Some(VarType::RealArray(sz) | VarType::IntArray(sz))
                if sz.parse::<u32>().is_ok() =>
            {
                candidates.push(v.clone());
            }
            Some(_) => return Err(StreamError::NonScalarState(v.clone())),
            None => return Err(StreamError::UnknownSymbol(v.clone())),
        }
    }

    // Bookkeeping purity: statements the transition deleter will drop
    // must not read or mutate anything but index vars — a side effect
    // hiding in a dropped statement would be lost silently.
    let mut index_vars: BTreeSet<String> = out_index_vars.clone();
    index_vars.extend(ring_vars.iter().cloned());
    check_bookkeeping_purity(steady_stmts, cursor, counter, &index_vars)?;

    // Carried vs temp split (sound): a candidate is a TEMP only if the
    // straight-line prefix of the transition assigns it whole (non-compound)
    // before any read and before the first branching statement; everything
    // else is carried state.
    let mut temps_set = written_before_read(steady_stmts, &candidates);
    // Any variable DECLARED inside the steady loop body is per-iteration by
    // C scoping (indeterminate on scope re-entry unless assigned) — always a
    // temp, never carried state (AVGDEV declares its window accumulators
    // in-loop).
    {
        let mut loop_decls: Vec<String> = Vec::new();
        collect_decl_order(steady_stmts, &mut loop_decls);
        for n2 in loop_decls {
            if candidates.contains(&n2) {
                temps_set.insert(n2);
            }
        }
    }

    // Deterministic order: follow first-declaration order in the body;
    // synthetic CIRCBUF index names append after (stable: circ_extra order).
    let mut decl_order: Vec<String> = Vec::new();
    collect_decl_order(body, &mut decl_order);
    for (n2, t) in circ_extra {
        if !matches!(t, VarType::RealPointer) && !decl_order.contains(n2) {
            decl_order.push(n2.clone());
        }
    }
    let ordered = |names: &BTreeSet<String>| -> Vec<(String, VarType)> {
        decl_order
            .iter()
            .filter(|n| names.contains(*n))
            .map(|n| (n.clone(), decls[n].clone()))
            .collect()
    };
    let candidate_set: BTreeSet<String> = candidates.iter().cloned().collect();
    let carried_set: BTreeSet<String> = candidate_set.difference(&temps_set).cloned().collect();

    Ok((ordered(&carried_set), ordered(&temps_set)))
}

/// Every statement the transition deleter drops (assignments to the cursor,
/// counter, or output-index vars) must read only index vars — a side effect
/// hiding in a dropped statement would be lost silently.
fn check_bookkeeping_purity(
    steady_stmts: &[Statement],
    cursor: &str,
    counter: Option<&str>,
    out_index_vars: &BTreeSet<String>,
) -> Result<(), StreamError> {
    fn walk_all(stmts: &[Statement], f: &mut dyn FnMut(&Statement)) {
        for st in stmts {
            f(st);
            match st {
                Statement::While { body, .. }
                | Statement::DoWhile { body, .. }
                | Statement::For { body, .. }
                | Statement::Block { body } => walk_all(body, f),
                Statement::If {
                    then_body,
                    else_body,
                    ..
                } => {
                    walk_all(then_body, f);
                    walk_all(else_body, f);
                }
                Statement::Switch { cases, default, .. } => {
                    for (_, sts) in cases {
                        walk_all(sts, f);
                    }
                    walk_all(default, f);
                }
                Statement::ForC { init, update, body, .. } => {
                    walk_all(std::slice::from_ref(init), f);
                    walk_all(std::slice::from_ref(update), f);
                    walk_all(body, f);
                }
                _ => {}
            }
        }
    }

    let mut dropped: BTreeSet<String> = out_index_vars.clone();
    dropped.insert(cursor.to_string());
    if let Some(c) = counter {
        dropped.insert(c.to_string());
    }
    let mut purity_err: Option<StreamError> = None;
    walk_all(steady_stmts, &mut |st| {
        if let Statement::Assign {
            target: Expr::Var(v),
            value,
            ..
        } = st
        {
            if dropped.contains(v) && purity_err.is_none() {
                let mut reads = BTreeSet::new();
                expr_var_names(value, &mut reads);
                for r in reads {
                    if !dropped.contains(&r) && r != "endIdx" && r != "startIdx" {
                        purity_err = Some(StreamError::Unsupported(format!(
                            "index bookkeeping for `{v}` reads non-index symbol `{r}`"
                        )));
                    }
                }
            }
        }
    });
    match purity_err {
        Some(e) => Err(e),
        None => Ok(()),
    }
}

enum InputIndex {
    Current,
    Lag(i64),
    OtherVar(String),
    /// `in[v - K]` behind a non-cursor index var (ring back-offset;
    /// K < 0 encodes a forward read `in[v + |K|]`).
    OtherVarLag(String, i64),
    /// `in[v - w]` where `w` is a literal-bounded inner-loop counter.
    OtherVarWinLag(String, String),
    /// `in[cursor - w]` with a variable offset (rescan window candidate).
    WindowVar(String),
    Unsupported,
}

fn classify_input_index(idx: &Expr, cursor: &str) -> InputIndex {
    match idx {
        Expr::Var(v) if v == cursor => InputIndex::Current,
        Expr::Var(v) => InputIndex::OtherVar(v.clone()),
        Expr::PostIncrement(b) => match b.as_ref() {
            Expr::Var(v) if v == cursor => InputIndex::Current,
            Expr::Var(v) => InputIndex::OtherVar(v.clone()),
            _ => InputIndex::Unsupported,
        },
        Expr::BinOp(l, BinOp::Sub, r) => match (l.as_ref(), r.as_ref()) {
            (Expr::Var(v), Expr::IntLiteral(k)) if v == cursor && *k >= 1 => InputIndex::Lag(*k),
            (Expr::Var(v), Expr::IntLiteral(k)) if *k >= 1 => {
                InputIndex::OtherVarLag(v.clone(), *k)
            }
            (Expr::Var(v), Expr::Var(w)) if v == cursor => InputIndex::WindowVar(w.clone()),
            (Expr::Var(v), Expr::Var(w)) => InputIndex::OtherVarWinLag(v.clone(), w.clone()),
            _ => InputIndex::Unsupported,
        },
        Expr::BinOp(l, BinOp::Add, r) => match (l.as_ref(), r.as_ref()) {
            (Expr::Var(v), Expr::IntLiteral(k)) if v != cursor && *k >= 1 => {
                InputIndex::OtherVarLag(v.clone(), -k)
            }
            _ => InputIndex::Unsupported,
        },
        _ => InputIndex::Unsupported,
    }
}

/// Countdown loops: the cursor is the single variable that indexes the input
/// arrays at the current bar.
fn countdown_cursor(
    steady: &[Statement],
    inputs: &[String],
    counter: &str,
) -> Result<String, StreamError> {
    let mut idx_vars: BTreeSet<String> = BTreeSet::new();
    for s in steady {
        walk_stmt_exprs(s, &mut |e| {
            walk_expr(e, &mut |x| {
                if let Expr::ArrayAccess(name, idx) = x {
                    if inputs.iter().any(|i| i == name) {
                        match idx.as_ref() {
                            Expr::Var(v) => {
                                idx_vars.insert(v.clone());
                            }
                            Expr::PostIncrement(b) => {
                                if let Expr::Var(v) = b.as_ref() {
                                    idx_vars.insert(v.clone());
                                }
                            }
                            // Lags resolve against the cursor later.
                            Expr::BinOp(l, BinOp::Sub, _) => {
                                if let Expr::Var(v) = l.as_ref() {
                                    idx_vars.insert(v.clone());
                                }
                            }
                            _ => {}
                        }
                    }
                }
            });
        });
    }
    idx_vars.remove(counter);
    match idx_vars.len() {
        1 => Ok(idx_vars.into_iter().next().unwrap()),
        0 => Err(StreamError::Unsupported(
            "countdown loop reads no input at a cursor".into(),
        )),
        _ => Err(StreamError::Unsupported(format!(
            "countdown loop has multiple input cursors: {idx_vars:?}"
        ))),
    }
}

/// Does any statement read an output array (outputs are write-only in a
/// transition)? Returns the offending array name.
/// Outputs may only appear as assignment targets (no read-back).
fn check_no_output_read_back(
    steady_stmts: &[Statement],
    outputs: &[String],
) -> Result<(), StreamError> {
    for s in steady_stmts {
        if let Some(name) = output_read_back(s, outputs) {
            return Err(StreamError::UnsupportedAccess(format!(
                "output `{name}` is read back in the steady loop"
            )));
        }
    }
    Ok(())
}

/// Outputs whose previous value the steady loop reads (`out[idx-1]`).
fn collect_out_feedback(steady_stmts: &[Statement], outputs: &[String]) -> Vec<String> {
    let mut out_feedback: Vec<String> = Vec::new();
    for st in steady_stmts {
        walk_stmt_exprs(st, &mut |e| {
            walk_expr(e, &mut |x| {
                if let Expr::ArrayAccess(n, idx) = x {
                    if outputs.iter().any(|o| o == n)
                        && is_prev_output_read(idx)
                        && !out_feedback.contains(n)
                    {
                        out_feedback.push(n.clone());
                    }
                }
            });
        });
    }
    out_feedback
}

/// `out[idx - 1]`: the previous bar's output (DX repeats it on a zero
/// denominator). Carried as `lastOut_*` state in the transition.
pub fn is_prev_output_read(idx: &Expr) -> bool {
    matches!(
        idx,
        Expr::BinOp(l, BinOp::Sub, r)
            if matches!(l.as_ref(), Expr::Var(_))
                && matches!(r.as_ref(), Expr::IntLiteral(1))
    )
}

fn output_read_back(s: &Statement, outputs: &[String]) -> Option<String> {
    let mut hit: Option<String> = None;
    let mut check_value = |e: &Expr| {
        walk_expr(e, &mut |x| {
            if let Expr::ArrayAccess(n, idx) = x {
                if outputs.iter().any(|o| o == n)
                    && hit.is_none()
                    && !is_prev_output_read(idx)
                {
                    hit = Some(n.clone());
                }
            }
        });
    };
    match s {
        Statement::Assign { target, value, compound } => {
            // The target itself may be `out[i]` (legal write); its INDEX and
            // the value must not read outputs. Compound assigns read the
            // target too.
            if let Expr::ArrayAccess(n, idx) = target {
                if *compound && outputs.iter().any(|o| o == n) {
                    return Some(n.clone());
                }
                check_value(idx);
            } else {
                check_value(target);
            }
            check_value(value);
            hit
        }
        Statement::While { condition, body }
        | Statement::DoWhile { condition, body } => {
            check_value(condition);
            hit.or_else(|| body.iter().find_map(|st| output_read_back(st, outputs)))
        }
        Statement::For { count, body, .. } => {
            check_value(count);
            hit.or_else(|| body.iter().find_map(|st| output_read_back(st, outputs)))
        }
        Statement::If {
            condition,
            then_body,
            else_body,
            ..
        } => {
            check_value(condition);
            hit.or_else(|| {
                then_body
                    .iter()
                    .chain(else_body)
                    .find_map(|st| output_read_back(st, outputs))
            })
        }
        Statement::Switch { expr, cases, default } => {
            check_value(expr);
            hit.or_else(|| {
                cases
                    .iter()
                    .flat_map(|(_, sts)| sts.iter())
                    .chain(default)
                    .find_map(|st| output_read_back(st, outputs))
            })
        }
        Statement::ForC {
            init,
            condition,
            update,
            body,
        } => {
            check_value(condition);
            hit.or_else(|| output_read_back(init, outputs))
                .or_else(|| output_read_back(update, outputs))
                .or_else(|| body.iter().find_map(|st| output_read_back(st, outputs)))
        }
        Statement::Block { body } => body.iter().find_map(|st| output_read_back(st, outputs)),
        Statement::Expr(e)
        | Statement::VarDecl { init: Some(e), .. }
        | Statement::Return { value: Some(e) } => {
            check_value(e);
            hit
        }
        _ => None,
    }
}

/// Candidates assigned whole (plain, non-compound `v = expr`) in the
/// straight-line prefix of the transition, before any read of `v` and before
/// the first branching statement. Sound under-approximation of
/// "written-before-read on every path".
fn written_before_read(steady: &[Statement], candidates: &[String]) -> BTreeSet<String> {
    let cand: BTreeSet<&String> = candidates.iter().collect();
    let mut initialized: BTreeSet<String> = BTreeSet::new();
    let mut read_first: BTreeSet<String> = BTreeSet::new();

    let note_reads = |e: &Expr, initialized: &BTreeSet<String>, read_first: &mut BTreeSet<String>| {
        walk_expr(e, &mut |x| {
            if let Expr::Var(n) = x {
                if cand.contains(n) && !initialized.contains(n) {
                    read_first.insert(n.clone());
                }
            }
        });
    };

    let mut in_prefix = true;
    for s in steady {
        if in_prefix {
            match s {
                Statement::Comment(_) => continue,
                Statement::Assign {
                    target: Expr::Var(v),
                    value,
                    compound: false,
                } => {
                    note_reads(value, &initialized, &mut read_first);
                    if cand.contains(v) && !read_first.contains(v) {
                        initialized.insert(v.clone());
                    }
                    continue;
                }
                Statement::VarDecl {
                    name,
                    init: Some(value),
                    ..
                } => {
                    note_reads(value, &initialized, &mut read_first);
                    if cand.contains(name) && !read_first.contains(name) {
                        initialized.insert(name.clone());
                    }
                    continue;
                }
                _ => in_prefix = false,
            }
        }
        // Tail (and the statement that ended the prefix): any reference to a
        // not-yet-initialized candidate counts as a read.
        let mut names = BTreeSet::new();
        stmt_var_names(s, &mut names);
        for n in names {
            if cand.contains(&n) && !initialized.contains(&n) {
                read_first.insert(n);
            }
        }
    }
    initialized
}

fn collect_decl_order(stmts: &[Statement], out: &mut Vec<String>) {
    for s in stmts {
        match s {
            Statement::VarDecl { name, .. } => {
                if !out.contains(name) {
                    out.push(name.clone());
                }
            }
            Statement::While { body, .. }
            | Statement::DoWhile { body, .. }
            | Statement::For { body, .. }
            | Statement::Block { body } => collect_decl_order(body, out),
            Statement::If {
                then_body,
                else_body,
                ..
            } => {
                collect_decl_order(then_body, out);
                collect_decl_order(else_body, out);
            }
            Statement::Switch { cases, default, .. } => {
                for (_, sts) in cases {
                    collect_decl_order(sts, out);
                }
                collect_decl_order(default, out);
            }
            Statement::ForC { init, body, .. } => {
                collect_decl_order(std::slice::from_ref(init), out);
                collect_decl_order(body, out);
            }
            _ => {}
        }
    }
}

// ---------------------------------------------------------------------------
// Transition-IR construction (language-neutral)
// ---------------------------------------------------------------------------

/// How a rewritten variable is addressed by the target language.
pub trait NameMap {
    /// A state-struct field (carried scalar, param, or lag slot).
    fn state(&self, name: &str) -> String;
    /// The scalar `update` parameter carrying `array`'s current bar.
    fn bar(&self, array: &str) -> String;
    /// The out-pointer for output array `name`.
    fn output(&self, name: &str) -> Expr;
    /// The ring buffer holding `array`'s window behind trailing var `var`.
    fn ring_buf(&self, var: &str, array: &str) -> String;
    /// The shared ring position for trailing var `var`.
    fn ring_pos(&self, var: &str) -> String;
    /// Runtime trailing lag of a back-offset ring (`back > 0` only).
    fn ring_lag(&self, var: &str) -> String;
    /// The shared ring capacity for trailing var `var`.
    fn ring_cap(&self, var: &str) -> String;
    /// The rescan-window buffer for counter `var` over `array`.
    fn win_buf(&self, var: &str, array: &str) -> String;
    /// The rescan-window write position for counter `var`.
    fn win_pos(&self, var: &str) -> String;
    /// The rescan-window capacity for counter `var`.
    fn win_cap(&self, var: &str) -> String;
    /// The captured CIRCBUF storage buffer named `storage`.
    fn circ_buf(&self, storage: &str) -> String;
    /// The extrema-automaton ring buffer for `array`.
    fn extrema_buf(&self, array: &str) -> String;
    /// The extrema-automaton ring capacity.
    fn extrema_cap(&self) -> String;
}

/// Build the transition statements: the steady-loop body with
/// - `in[cursor]`            -> the bar parameter,
/// - `in[cursor-k]`          -> the lag state field,
/// - carried locals & params -> state fields,
/// - `out[idx] = e`          -> the mapped output target,
/// - cursor/counter/out-index bookkeeping deleted,
/// - lag-shift assignments appended (deepest first).
///
/// Returns an error if a dropped variable survives the rewrite (a bookkeeping
/// statement the deleter did not recognize).
#[allow(clippy::too_many_lines)]
pub fn build_transition(model: &StreamModel, names: &dyn NameMap) -> Result<Vec<Statement>, String> {
    let dropped = model.dropped_vars();
    let state_names: BTreeSet<String> = model
        .state
        .iter()
        .map(|(n, _)| n.clone())
        .chain(model.func.optional_inputs.iter().map(|p| p.name.clone()))
        .chain(
            model
                .func
                .private_extra_params
                .iter()
                .map(|(n, _)| n.clone()),
        )
        .collect();

    let rewritten = rewrite_stmts(
        &model.steady_stmts,
        &|e| rewrite_expr_for_transition(e, model, names, &state_names),
        &|s| {
            // In-loop VarDecls: the flattened step declares all temps at the
            // top, so re-declaring here would shadow (and mid-body decls are
            // not C89). Keep the initializer as an assignment.
            let s = match s {
                Statement::VarDecl {
                    name,
                    init: Some(init),
                    ..
                } => Statement::Assign {
                    target: Expr::Var(name),
                    value: init,
                    compound: false,
                },
                Statement::VarDecl { init: None, .. } => return None,
                other => other,
            };
            drop_bookkeeping(s, &dropped, model, names)
        },
    );

    // param==1 identity short-circuit, mirroring the batch's explicit path
    // (bit-exact: both sides copy the input).
    let identity_branch = model.identity.as_ref().map(|idp| {
        let condition = rewrite_expr(&idp.condition, &|e| match e {
            Expr::Var(n) if state_names.contains(&n) => Expr::Var(names.state(&n)),
            other => other,
        });
        let mut then_body: Vec<Statement> = idp
            .pairs
            .iter()
            .map(|(out, inp)| Statement::Assign {
                target: names.output(out),
                value: Expr::Var(names.bar(inp)),
                compound: false,
            })
            .collect();
        then_body.push(Statement::Return { value: None });
        Statement::If {
            condition,
            then_body,
            else_body: vec![],
            cond_comments: vec![],
        }
    });

    // Safety: no dropped variable may survive.
    let mut leaked = BTreeSet::new();
    for s in &rewritten {
        stmt_var_names(s, &mut leaked);
    }
    for v in &dropped {
        if leaked.contains(v) {
            return Err(format!(
                "{}: index variable `{v}` leaks into the transition body",
                model.func.name
            ));
        }
    }

    let mut out = rewritten;
    insert_transition_prologue(&mut out, model, names, identity_branch);
    // Previous-output feedback: refresh lastOut_* AFTER the body computed
    // this bar's output (reads of out[idx-1] were rewritten to the state
    // field, which still held the prior bar's value during the body).
    for name in &model.out_feedback {
        out.push(Statement::Assign {
            target: Expr::Var(names.state(&format!("lastOut_{name}"))),
            value: names.output(name),
            compound: false,
        });
    }
    // Lag shifts, deepest slot first, then the new bar into lag1.
    for lag in &model.lags {
        for k in (2..=lag.depth).rev() {
            out.push(Statement::Assign {
                target: Expr::Var(names.state(&StreamModel::lag_field(&lag.array, k))),
                value: Expr::Var(names.state(&StreamModel::lag_field(&lag.array, k - 1))),
                compound: false,
            });
        }
        out.push(Statement::Assign {
            target: Expr::Var(names.state(&StreamModel::lag_field(&lag.array, 1))),
            value: Expr::Var(names.bar(&lag.array)),
            compound: false,
        });
    }
    // Ring pushes: newest bar replaces the oldest slot, then the shared
    // position advances with the conditional-reset idiom (house style; a
    // modulo costs ~10 cycles on ARM). Order preserved vs the batch reads
    // above: reads happened on the OLD slot contents.
    for ring in model.rings() {
        push_ring_advance(&mut out, ring, names);
    }
    // Rescan windows: the current bar was written at `pos` before the body;
    // advance the position for the next update.
    for win in model.windows() {
        push_window_advance(&mut out, win, names);
    }
    // Cursor-parity flip: this bar's `cursor % 2` was consumed by the branch
    // predicate; advance to the next bar's parity.
    if let Some(ps) = &model.parity {
        push_parity_flip(&mut out, ps, names);
    }
    Ok(out)
}

/// `parity = 1 - parity;` — advance the carried parity to the next bar (toggles
/// {0,1}) after the branch predicate has consumed this bar's value.
fn push_parity_flip(out: &mut Vec<Statement>, parity: &ParitySpec, names: &dyn NameMap) {
    let field = names.state(&parity.field);
    out.push(Statement::Assign {
        target: Expr::Var(field.clone()),
        value: Expr::BinOp(
            Box::new(Expr::IntLiteral(1)),
            BinOp::Sub,
            Box::new(Expr::Var(field)),
        ),
        compound: false,
    });
}

/// `win[pos] = bar;` — the current bar enters the window before the body.
fn window_prewrite(win: &WindowSpec, arr: &str, names: &dyn NameMap) -> Statement {
    Statement::Assign {
        target: Expr::ArrayAccess(
            names.win_buf(&win.var, arr),
            Box::new(Expr::Var(names.win_pos(&win.var))),
        ),
        value: Expr::Var(names.bar(arr)),
        compound: false,
    }
}

/// Advance a window's write position (wrap at capacity).
fn push_window_advance(out: &mut Vec<Statement>, win: &WindowSpec, names: &dyn NameMap) {
    out.push(Statement::Assign {
        target: Expr::Var(names.win_pos(&win.var)),
        value: Expr::BinOp(
            Box::new(Expr::Var(names.win_pos(&win.var))),
            BinOp::Add,
            Box::new(Expr::IntLiteral(1)),
        ),
        compound: false,
    });
    out.push(Statement::If {
        condition: Expr::BinOp(
            Box::new(Expr::Var(names.win_pos(&win.var))),
            BinOp::GreaterEq,
            Box::new(Expr::Var(names.win_cap(&win.var))),
        ),
        then_body: vec![Statement::Assign {
            target: Expr::Var(names.win_pos(&win.var)),
            value: Expr::IntLiteral(0),
            compound: false,
        }],
        else_body: vec![],
        cond_comments: vec![],
    });
}

/// Prepend the transition prologue (in reverse insertion order): identity
/// short-circuit, ring cap-0 guards, window pre-writes, and the extrema
/// automaton's absolute-slot bar store (the body's own tail increments
/// advanced the cursor state after the previous bar, so it already holds
/// this bar's index).
fn insert_transition_prologue(
    out: &mut Vec<Statement>,
    model: &StreamModel,
    names: &dyn NameMap,
    identity_branch: Option<Statement>,
) {
    if let Some(ex) = model.extrema() {
        for arr in ex.arrays.iter().rev() {
            out.insert(
                0,
                Statement::Assign {
                    target: Expr::ArrayAccess(
                        names.extrema_buf(arr),
                        Box::new(Expr::BinOp(
                            Box::new(Expr::Var(names.state(&model.cursor))),
                            BinOp::Mod,
                            Box::new(Expr::Var(names.extrema_cap())),
                        )),
                    ),
                    value: Expr::Var(names.bar(arr)),
                    compound: false,
                },
            );
        }
    }
    for win in model.windows().iter().rev() {
        for arr in win.arrays.iter().rev() {
            out.insert(0, window_prewrite(win, arr, names));
        }
    }
    for ring in model.rings().iter().rev() {
        if ring.back > 0 {
            // Absolute-mod layout: slot `pos` (== bar index % cap) holds the
            // current bar so the runtime-lag-0 case reads it through the
            // same formula. cap = lag + back + 1, so this slot never
            // collides with any `in[var - K]` read.
            for arr in ring.arrays.iter().rev() {
                out.insert(
                    0,
                    Statement::Assign {
                        target: Expr::ArrayAccess(
                            names.ring_buf(&ring.var, arr),
                            Box::new(Expr::Var(names.ring_pos(&ring.var))),
                        ),
                        value: Expr::Var(names.bar(arr)),
                        compound: false,
                    },
                );
            }
        } else {
            out.insert(0, ring_cap0_guard(ring, names));
        }
    }
    if let Some(b) = identity_branch {
        out.insert(0, b);
    }
}

/// The state field name synthesized by [`carry_cursor_parity`] to hold
/// `cursor % 2`. Reserved: the parity carry only fires when a steady loop
/// branches on `cursor % 2`, so this name never coexists with a same-named
/// local (no library source declares one).
const PARITY_FIELD: &str = "streamParity";

/// STEADY-LOOP NORMALIZATION (general; sits beside [`hoist_ternary_indices`]
/// and [`reindex_cursor_windows`], applied to every steady body): strip a
/// warm-up OUTPUT GATE `if (cursor >= startIdx) { <output writes> }` by splicing
/// its then-body to the loop top level.
///
/// A batch loop that walks from before `startIdx` guards its output write so
/// warm-up bars emit nothing. A stream never sees warm-up in the transition:
/// open's full-batch replay consumes it (open transcribes the ORIGINAL body
/// over [`StreamModel::body`], gate intact), and every update processes exactly
/// one bar at index `>= startIdx`. So the transition writes UNCONDITIONALLY;
/// promoting the guarded block removes `startIdx` from the steady loop (which
/// `classify_locals` otherwise rejects) and the cursor reference in the gate.
///
/// Keyed purely on the structural shape `cursor >= startIdx` with an empty
/// `else`, so any non-matching steady body passes through byte-identically
/// (regen-safe). RECURSES into nested bodies and fires on every match: the gate
/// can sit at the loop top level (HT_DCPERIOD) or inside another branch — e.g.
/// HT_PHASOR writes its two outputs under a gate INSIDE each odd/even arm.
fn strip_cursor_output_gate(stmts: Vec<Statement>, cursor: &str) -> Vec<Statement> {
    let mut out: Vec<Statement> = Vec::with_capacity(stmts.len());
    for st in stmts {
        // Descend first so a gate nested in this statement's own bodies is
        // stripped before we test the statement itself.
        let st = strip_gate_in_children(st, cursor);
        if let Statement::If {
            condition: Expr::BinOp(ref l, BinOp::GreaterEq, ref r),
            ref then_body,
            ref else_body,
            ..
        } = st
        {
            if else_body.is_empty()
                && matches!(l.as_ref(), Expr::Var(v) if v == cursor)
                && matches!(r.as_ref(), Expr::Var(v) if v == "startIdx")
            {
                out.extend(then_body.iter().cloned());
                continue;
            }
        }
        out.push(st);
    }
    out
}

/// Apply [`strip_cursor_output_gate`] to every nested statement body of `st`
/// (branch arms, loop bodies, switch cases), leaving the statement's own shape
/// intact. Statements without nested bodies pass through unchanged.
fn strip_gate_in_children(st: Statement, cursor: &str) -> Statement {
    let strip = |b: Vec<Statement>| strip_cursor_output_gate(b, cursor);
    match st {
        Statement::If {
            condition,
            then_body,
            else_body,
            cond_comments,
        } => Statement::If {
            condition,
            then_body: strip(then_body),
            else_body: strip(else_body),
            cond_comments,
        },
        Statement::While { condition, body } => Statement::While {
            condition,
            body: strip(body),
        },
        Statement::DoWhile { condition, body } => Statement::DoWhile {
            condition,
            body: strip(body),
        },
        Statement::For { var, count, body } => Statement::For {
            var,
            count,
            body: strip(body),
        },
        Statement::ForC {
            init,
            condition,
            update,
            body,
        } => Statement::ForC {
            init,
            condition,
            update,
            body: strip(body),
        },
        Statement::Block { body } => Statement::Block { body: strip(body) },
        Statement::Switch {
            expr,
            cases,
            default,
        } => Statement::Switch {
            expr,
            cases: cases.into_iter().map(|(l, b)| (l, strip(b))).collect(),
            default: strip(default),
        },
        other => other,
    }
}

/// STEADY-LOOP NORMALIZATION (general; see [`strip_cursor_output_gate`]): carry
/// a `cursor % 2` (odd/even absolute-bar-index) branch as state.
///
/// A stream cannot reconstruct the absolute bar index, so a steady loop that
/// branches on `cursor % 2` cannot be transcribed verbatim. This rewrites the
/// `cursor % 2` sub-expression to read a carried `int` field ([`PARITY_FIELD`])
/// and returns a [`ParitySpec`] telling the emitter to SEED it in open
/// (`historyLen % 2`, the next bar's parity, established by open's real-batch
/// replay) and FLIP it (`1 - parity`) each update. Returns `None` when the body
/// has no `cursor % 2` — a byte-identical no-op for every other function.
///
/// Keyed on shape via [`is_cursor_parity`] (the modulus's LEFT operand must be
/// the cursor), which distinguishes `cursor % 2` from a param modulus like
/// TRIMA's `optInTimePeriod % 2` — the latter is never rewritten.
fn carry_cursor_parity(
    stmts: Vec<Statement>,
    cursor: &str,
) -> (Vec<Statement>, Option<ParitySpec>) {
    let mut found = false;
    for s in &stmts {
        walk_stmt_exprs(s, &mut |e| {
            walk_expr(e, &mut |x| {
                if is_cursor_parity(x, cursor) {
                    found = true;
                }
            });
        });
    }
    if !found {
        return (stmts, None);
    }
    let fe = |e: Expr| -> Expr {
        if is_cursor_parity(&e, cursor) {
            Expr::Var(PARITY_FIELD.to_string())
        } else {
            e
        }
    };
    let rewritten = rewrite_stmts(&stmts, &fe, &Some);
    (
        rewritten,
        Some(ParitySpec {
            field: PARITY_FIELD.to_string(),
        }),
    )
}

/// `cursor % 2` — a steady loop's odd/even absolute-bar-index parity test. The
/// LEFT operand must be the cursor (excludes a param modulus like `period % 2`).
fn is_cursor_parity(e: &Expr, cursor: &str) -> bool {
    matches!(
        e,
        Expr::BinOp(l, BinOp::Mod, r)
            if matches!(l.as_ref(), Expr::Var(v) if v == cursor)
                && matches!(r.as_ref(), Expr::IntLiteral(2))
    )
}

/// Rewrite `arr[cond ? a : b]` to `cond ? arr[a] : arr[b]` bottom-up,
/// wherever all three sub-expressions are side-effect-free.
fn hoist_ternary_indices(stmts: &[Statement]) -> Vec<Statement> {
    let fe = |e: Expr| -> Expr {
        match e {
            Expr::ArrayAccess(n, idx) => match *idx {
                Expr::Ternary(c, a, b)
                    if expr_is_pure(&c) && expr_is_pure(&a) && expr_is_pure(&b) =>
                {
                    Expr::Ternary(
                        c,
                        Box::new(Expr::ArrayAccess(n.clone(), a)),
                        Box::new(Expr::ArrayAccess(n, b)),
                    )
                }
                other => Expr::ArrayAccess(n, Box::new(other)),
            },
            other => other,
        }
    };
    rewrite_stmts(stmts, &fe, &Some)
}

/// Rewrite `for (i = cursor - E; i <= cursor; i++) BODY` into
/// `for (i = E; i >= 0; i--) BODY[i := cursor - i]`. Iteration order over
/// bars is reversed relative to batch, so this applies ONLY when the body
/// is order-independent for FP purposes — conservatively: never. Instead
/// the counter keeps ascending bar order by iterating the OFFSET downward:
/// offset w runs E..0, so `cursor - w` still visits bars oldest-first and
/// the FP accumulation order is untouched.
fn reindex_cursor_windows(stmts: &[Statement], cursor: &str) -> Vec<Statement> {
    stmts
        .iter()
        .map(|st| reindex_one(st, cursor))
        .collect()
}

/// Match `for (i = cursor - E; i <= cursor; i++)` and return `(i, E)`.
fn match_cursor_anchored_loop(
    init: &Statement,
    condition: &Expr,
    update: &Statement,
    cursor: &str,
) -> Option<(String, Expr)> {
    match (init, condition) {
        (
            Statement::Assign {
                target: Expr::Var(iv),
                value: Expr::BinOp(l, BinOp::Sub, e),
                ..
            },
            Expr::BinOp(cl, BinOp::LessEq, cr),
        ) => {
            let init_from_cursor = matches!(l.as_ref(), Expr::Var(v) if v == cursor);
            let cond_i_le_cursor = matches!(cl.as_ref(), Expr::Var(v) if v == iv)
                && matches!(cr.as_ref(), Expr::Var(v) if v == cursor);
            let inc = match update {
                Statement::Expr(Expr::PostIncrement(b) | Expr::PreIncrement(b)) => {
                    matches!(b.as_ref(), Expr::Var(v) if v == iv)
                }
                // `i++` also parses as the compound `i = i + 1`.
                Statement::Assign {
                    target: Expr::Var(tv),
                    value: Expr::BinOp(al, BinOp::Add, ar),
                    ..
                } => {
                    tv == iv
                        && matches!(al.as_ref(), Expr::Var(v) if v == iv)
                        && matches!(ar.as_ref(), Expr::IntLiteral(1))
                }
                _ => false,
            };
            if init_from_cursor && cond_i_le_cursor && inc && expr_is_pure(e) {
                Some((iv.clone(), (**e).clone()))
            } else {
                None
            }
        }
        _ => None,
    }
}

fn reindex_one(st: &Statement, cursor: &str) -> Statement {
    if let Statement::ForC {
        init,
        condition,
        update,
        body,
    } = st
    {
        if let Some((iv, bound)) = match_cursor_anchored_loop(init, condition, update, cursor) {
            // Substitute i := cursor - i in the body (bars still visit
            // oldest-first: offset counts E down to 0).
            let cur = cursor.to_string();
            let iv2 = iv.clone();
            let fe = move |e: Expr| -> Expr {
                match e {
                    Expr::Var(v) if v == iv2 => Expr::BinOp(
                        Box::new(Expr::Var(cur.clone())),
                        BinOp::Sub,
                        Box::new(Expr::Var(iv2.clone())),
                    ),
                    other => other,
                }
            };
            let new_body = rewrite_stmts(body, &fe, &Some);
            return Statement::ForC {
                init: Box::new(Statement::Assign {
                    target: Expr::Var(iv.clone()),
                    value: bound,
                    compound: false,
                }),
                condition: Expr::BinOp(
                    Box::new(Expr::Var(iv.clone())),
                    BinOp::GreaterEq,
                    Box::new(Expr::IntLiteral(0)),
                ),
                // The canonical IR form for `i--` (the parser's own shape;
                // Statement::Expr(PostDecrement) renders empty in C).
                update: Box::new(Statement::Assign {
                    target: Expr::Var(iv.clone()),
                    value: Expr::BinOp(
                        Box::new(Expr::Var(iv)),
                        BinOp::Sub,
                        Box::new(Expr::IntLiteral(1)),
                    ),
                    compound: true,
                }),
                body: new_body,
            };
        }
    }
    // Recurse into compound statements.
    match st {
        Statement::While { condition, body } => Statement::While {
            condition: condition.clone(),
            body: reindex_cursor_windows(body, cursor),
        },
        Statement::DoWhile { condition, body } => Statement::DoWhile {
            condition: condition.clone(),
            body: reindex_cursor_windows(body, cursor),
        },
        Statement::If {
            condition,
            then_body,
            else_body,
            cond_comments,
        } => Statement::If {
            condition: condition.clone(),
            then_body: reindex_cursor_windows(then_body, cursor),
            else_body: reindex_cursor_windows(else_body, cursor),
            cond_comments: cond_comments.clone(),
        },
        Statement::Block { body } => Statement::Block {
            body: reindex_cursor_windows(body, cursor),
        },
        other => other.clone(),
    }
}

/// True when evaluating the expression has no side effects.
fn expr_is_pure(e: &Expr) -> bool {
    match e {
        Expr::PostIncrement(_)
        | Expr::PostDecrement(_)
        | Expr::PreIncrement(_)
        | Expr::PreDecrement(_) => false,
        Expr::FuncCall(n, args) => !is_stateful_call(n) && args.iter().all(expr_is_pure),
        Expr::ArrayAccess(_, i)
        | Expr::Cast(_, i)
        | Expr::Not(i)
        | Expr::AddressOf(i) => expr_is_pure(i),
        Expr::BinOp(l, _, r) => expr_is_pure(l) && expr_is_pure(r),
        Expr::Ternary(c, a, b) => expr_is_pure(c) && expr_is_pure(a) && expr_is_pure(b),
        _ => true,
    }
}

/// Read `in[var - k]` from an absolute-mod ring: `ring[(pos + cap - lag - k) % cap]`.
/// `pos` is the current bar's slot (bar index % cap), `lag` the runtime
/// distance cursor-var, and `lag + k < cap` by construction, so the single
/// mod suffices and the slot never collides with the current bar's.
fn ring_offset_read(
    ring: &RingSpec,
    array: &str,
    k: Option<i64>,
    var_off: Option<Expr>,
    names: &dyn NameMap,
) -> Expr {
    let mut idx = Expr::BinOp(
        Box::new(Expr::BinOp(
            Box::new(Expr::Var(names.ring_pos(&ring.var))),
            BinOp::Add,
            Box::new(Expr::Var(names.ring_cap(&ring.var))),
        )),
        BinOp::Sub,
        Box::new(Expr::Var(names.ring_lag(&ring.var))),
    );
    match k {
        Some(k) if k > 0 => {
            idx = Expr::BinOp(Box::new(idx), BinOp::Sub, Box::new(Expr::IntLiteral(k)));
        }
        Some(k) if k < 0 => {
            // Forward read in[var + |k|]; runtime lag >= fwd keeps the slot
            // behind the current bar.
            idx = Expr::BinOp(Box::new(idx), BinOp::Add, Box::new(Expr::IntLiteral(-k)));
        }
        _ => {}
    }
    if let Some(off) = var_off {
        idx = Expr::BinOp(Box::new(idx), BinOp::Sub, Box::new(off));
    }
    Expr::ArrayAccess(
        names.ring_buf(&ring.var, array),
        Box::new(Expr::BinOp(
            Box::new(idx),
            BinOp::Mod,
            Box::new(Expr::Var(names.ring_cap(&ring.var))),
        )),
    )
}

/// `if (cap == 0) ring[0] = bar;` for every array of a ring — makes the
/// zero-lag degenerate case read the current bar through the same slot.
fn ring_cap0_guard(ring: &RingSpec, names: &dyn NameMap) -> Statement {
    let then_body = ring
        .arrays
        .iter()
        .map(|arr| Statement::Assign {
            target: Expr::ArrayAccess(
                names.ring_buf(&ring.var, arr),
                Box::new(Expr::IntLiteral(0)),
            ),
            value: Expr::Var(names.bar(arr)),
            compound: false,
        })
        .collect();
    Statement::If {
        condition: Expr::BinOp(
            Box::new(Expr::Var(names.ring_cap(&ring.var))),
            BinOp::Eq,
            Box::new(Expr::IntLiteral(0)),
        ),
        then_body,
        else_body: vec![],
        cond_comments: vec![],
    }
}

/// Append the end-of-update ring maintenance: store the new bar(s) into the
/// current slot, advance the shared position, wrap at capacity.
fn push_ring_advance(out: &mut Vec<Statement>, ring: &RingSpec, names: &dyn NameMap) {
    for arr in &ring.arrays {
        out.push(Statement::Assign {
            target: Expr::ArrayAccess(
                names.ring_buf(&ring.var, arr),
                Box::new(Expr::Var(names.ring_pos(&ring.var))),
            ),
            value: Expr::Var(names.bar(arr)),
            compound: false,
        });
    }
    out.push(Statement::Assign {
        target: Expr::Var(names.ring_pos(&ring.var)),
        value: Expr::BinOp(
            Box::new(Expr::Var(names.ring_pos(&ring.var))),
            BinOp::Add,
            Box::new(Expr::IntLiteral(1)),
        ),
        compound: false,
    });
    out.push(Statement::If {
        condition: Expr::BinOp(
            Box::new(Expr::Var(names.ring_pos(&ring.var))),
            BinOp::GreaterEq,
            Box::new(Expr::Var(names.ring_cap(&ring.var))),
        ),
        then_body: vec![Statement::Assign {
            target: Expr::Var(names.ring_pos(&ring.var)),
            value: Expr::IntLiteral(0),
            compound: false,
        }],
        else_body: vec![],
        cond_comments: vec![],
    });
}

fn rewrite_expr_for_transition(
    e: Expr,
    model: &StreamModel,
    names: &dyn NameMap,
    state_names: &BTreeSet<String>,
) -> Expr {
    match e {
        Expr::ArrayAccess(n, idx)
            if model.extrema().is_some() && model.bar_inputs.contains(&n) =>
        {
            // Absolute-index automaton: every input read maps to the ring
            // slot of its absolute position (the index expression's vars
            // were already state-mapped bottom-up).
            Expr::ArrayAccess(
                names.extrema_buf(&n),
                Box::new(Expr::BinOp(idx, BinOp::Mod, Box::new(Expr::Var(names.extrema_cap())))),
            )
        }
        Expr::ArrayAccess(n, idx)
            if model.out_feedback.contains(&n) && is_prev_output_read(&idx) =>
        {
            Expr::Var(names.state(&format!("lastOut_{n}")))
        }
        Expr::ArrayAccess(n, idx) if state_names.contains(&n) => {
            Expr::ArrayAccess(names.state(&n), idx)
        }
        Expr::ArrayAccess(n, idx) if model.bar_inputs.contains(&n) => {
            match classify_input_index(&idx, &model.cursor) {
                InputIndex::Current => Expr::Var(names.bar(&n)),
                InputIndex::Lag(k) => Expr::Var(names.state(&StreamModel::lag_field(&n, k))),
                InputIndex::OtherVar(v)
                    if model.rings().iter().any(|r| r.var == v) =>
                {
                    let ring = model.rings().iter().find(|r| r.var == v).unwrap();
                    if ring.back > 0 {
                        ring_offset_read(ring, &n, Some(0), None, names)
                    } else {
                        // Oldest slot of the trailing window: ring[pos].
                        Expr::ArrayAccess(
                            names.ring_buf(&v, &n),
                            Box::new(Expr::Var(names.ring_pos(&v))),
                        )
                    }
                }
                InputIndex::OtherVarLag(v, k)
                    if model.rings().iter().any(|r| r.var == v) =>
                {
                    let ring = model.rings().iter().find(|r| r.var == v).unwrap();
                    ring_offset_read(ring, &n, Some(k), None, names)
                }
                InputIndex::OtherVarWinLag(v, w)
                    if model.rings().iter().any(|r| r.var == v) =>
                {
                    let ring = model.rings().iter().find(|r| r.var == v).unwrap();
                    ring_offset_read(ring, &n, None, Some(Expr::Var(w)), names)
                }
                InputIndex::WindowVar(w0) => {
                    // Bottom-up rewriting may already have state-mapped the
                    // offset var; resolve back to the window it belongs to.
                    let win = model.windows().iter().find(|win| {
                        win.var == w0 || names.state(&win.var) == w0
                    });
                    match win {
                        Some(win) => {
                            // Bar `w` back from the current one (slot `pos` holds
                            // the current bar, written before the transition body).
                            // The logical index is (pos + cap - w) % cap; but `w`
                            // is a window offset in [0, cap-1], so that operand is
                            // in [1, 2*cap) and the modulo is a single conditional
                            // subtract. Emit `X >= cap ? X - cap : X` instead of a
                            // per-element runtime integer division -- brings the
                            // stream window-rescan to batch@last's cost (#110).
                            let pos = Expr::Var(names.win_pos(&win.var));
                            let cap = Expr::Var(names.win_cap(&win.var));
                            let x = Expr::BinOp(
                                Box::new(Expr::BinOp(
                                    Box::new(pos),
                                    BinOp::Add,
                                    Box::new(cap.clone()),
                                )),
                                BinOp::Sub,
                                Box::new(Expr::Var(w0)),
                            );
                            let idx_expr = Expr::Ternary(
                                Box::new(Expr::BinOp(
                                    Box::new(x.clone()),
                                    BinOp::GreaterEq,
                                    Box::new(cap.clone()),
                                )),
                                Box::new(Expr::BinOp(
                                    Box::new(x.clone()),
                                    BinOp::Sub,
                                    Box::new(cap),
                                )),
                                Box::new(x),
                            );
                            Expr::ArrayAccess(
                                names.win_buf(&win.var, &n),
                                Box::new(idx_expr),
                            )
                        }
                        None => Expr::ArrayAccess(n, idx),
                    }
                }
                _ => Expr::ArrayAccess(n, idx),
            }
        }
        Expr::ArrayAccess(n, idx)
            if model
                .circs()
                .iter()
                .flat_map(circ_storages)
                .any(|(st, _)| st == n) =>
        {
            Expr::ArrayAccess(names.circ_buf(&n), idx)
        }
        Expr::Var(n) if state_names.contains(&n) => Expr::Var(names.state(&n)),
        other => other,
    }
}

/// Delete pure index-bookkeeping statements; rewrite output writes.
fn drop_bookkeeping(
    s: Statement,
    dropped: &BTreeSet<String>,
    model: &StreamModel,
    names: &dyn NameMap,
) -> Option<Statement> {
    match s {
        // CIRCBUF_NEXT: expand to the exact macro semantics on state names
        // (idx++; if (idx > maxIdx) idx = 0 — conditional reset, not modulo).
        Statement::CircBuf(CircBuf::Next { ref id })
            if model.circs().iter().any(|c| c.id == *id) =>
        {
            let idx = names.state(&format!("{id}_Idx"));
            let max = names.state(&format!("maxIdx_{id}"));
            Some(Statement::Block {
                body: vec![
                    Statement::Assign {
                        target: Expr::Var(idx.clone()),
                        value: Expr::BinOp(
                            Box::new(Expr::Var(idx.clone())),
                            BinOp::Add,
                            Box::new(Expr::IntLiteral(1)),
                        ),
                        compound: false,
                    },
                    Statement::If {
                        condition: Expr::BinOp(
                            Box::new(Expr::Var(idx.clone())),
                            BinOp::Greater,
                            Box::new(Expr::Var(max)),
                        ),
                        then_body: vec![Statement::Assign {
                            target: Expr::Var(idx),
                            value: Expr::IntLiteral(0),
                            compound: false,
                        }],
                        else_body: vec![],
                        cond_comments: vec![],
                    },
                ],
            })
        }
        // out[idx] = expr  ->  <output target> = expr
        Statement::Assign {
            target: Expr::ArrayAccess(n, _),
            value,
            compound,
        } if model.outputs.contains(&n) => Some(Statement::Assign {
            target: names.output(&n),
            value,
            compound,
        }),
        // idx = ... / idx += ...  ->  deleted
        Statement::Assign {
            target: Expr::Var(v),
            ..
        } if dropped.contains(&v) => None,
        // standalone idx++ / idx-- / ++idx / --idx  ->  deleted
        Statement::Expr(
            Expr::PostIncrement(ref b)
            | Expr::PreIncrement(ref b)
            | Expr::PostDecrement(ref b)
            | Expr::PreDecrement(ref b),
        ) if matches!(b.as_ref(), Expr::Var(v) if dropped.contains(v)) => None,
        // empty blocks left by deletions
        Statement::Block { ref body } if body.is_empty() => None,
        other => Some(other),
    }
}

/// Rewrite a statement tree: expressions bottom-up via `fe`, statements via
/// `fs` (returning `None` deletes a statement).
pub fn rewrite_stmts(
    stmts: &[Statement],
    fe: &dyn Fn(Expr) -> Expr,
    fs: &dyn Fn(Statement) -> Option<Statement>,
) -> Vec<Statement> {
    let mut out = Vec::new();
    for s in stmts {
        let mapped: Statement = match s {
            Statement::VarDecl {
                var_type,
                name,
                init,
            } => Statement::VarDecl {
                var_type: var_type.clone(),
                name: name.clone(),
                init: init.as_ref().map(|e| rewrite_expr(e, fe)),
            },
            Statement::Assign {
                target,
                value,
                compound,
            } => Statement::Assign {
                target: rewrite_expr(target, fe),
                value: rewrite_expr(value, fe),
                compound: *compound,
            },
            Statement::While { condition, body } => Statement::While {
                condition: rewrite_expr(condition, fe),
                body: rewrite_stmts(body, fe, fs),
            },
            Statement::DoWhile { condition, body } => Statement::DoWhile {
                condition: rewrite_expr(condition, fe),
                body: rewrite_stmts(body, fe, fs),
            },
            Statement::For { var, count, body } => Statement::For {
                var: var.clone(),
                count: rewrite_expr(count, fe),
                body: rewrite_stmts(body, fe, fs),
            },
            Statement::If {
                condition,
                then_body,
                else_body,
                cond_comments,
            } => Statement::If {
                condition: rewrite_expr(condition, fe),
                then_body: rewrite_stmts(then_body, fe, fs),
                else_body: rewrite_stmts(else_body, fe, fs),
                cond_comments: cond_comments.clone(),
            },
            Statement::Return { value } => Statement::Return {
                value: value.as_ref().map(|e| rewrite_expr(e, fe)),
            },
            Statement::Switch {
                expr,
                cases,
                default,
            } => Statement::Switch {
                expr: rewrite_expr(expr, fe),
                cases: cases
                    .iter()
                    .map(|(v, sts)| (v.clone(), rewrite_stmts(sts, fe, fs)))
                    .collect(),
                default: rewrite_stmts(default, fe, fs),
            },
            Statement::ForC {
                init,
                condition,
                update,
                body,
            } => Statement::ForC {
                init: Box::new(
                    rewrite_stmts(std::slice::from_ref(init), fe, fs)
                        .pop()
                        .unwrap_or(Statement::Block { body: vec![] }),
                ),
                condition: rewrite_expr(condition, fe),
                update: Box::new(
                    rewrite_stmts(std::slice::from_ref(update), fe, fs)
                        .pop()
                        .unwrap_or(Statement::Block { body: vec![] }),
                ),
                body: rewrite_stmts(body, fe, fs),
            },
            Statement::Block { body } => Statement::Block {
                body: rewrite_stmts(body, fe, fs),
            },
            Statement::Expr(e) => Statement::Expr(rewrite_expr(e, fe)),
            other => other.clone(),
        };
        if let Some(kept) = fs(mapped) {
            out.push(kept);
        }
    }
    out
}

/// A batch body that ASSIGNS a settings local would carry the mutation
/// across bars, while the step re-unpacks from the globals every bar —
/// silent divergence. No input does this today; reject if one ever does.
fn reject_candle_local_writes(
    steady_stmts: &[Statement],
    candle_locals: &BTreeSet<String>,
) -> Result<(), StreamError> {
    for st in steady_stmts {
        let mut err: Option<String> = None;
        walk_assign_targets(st, &mut |t| {
            if let Expr::Var(v) = t {
                if candle_locals.contains(v) && err.is_none() {
                    err = Some(v.clone());
                }
            }
        });
        if let Some(v) = err {
            return Err(StreamError::Unsupported(format!(
                "steady loop assigns candle-setting local `{v}`"
            )));
        }
    }
    Ok(())
}

/// Walk every `Assign` target in a statement tree.
fn walk_assign_targets(s: &Statement, f: &mut dyn FnMut(&Expr)) {
    match s {
        Statement::Assign { target, .. } => f(target),
        Statement::While { body, .. }
        | Statement::DoWhile { body, .. }
        | Statement::For { body, .. }
        | Statement::Block { body } => {
            for st in body {
                walk_assign_targets(st, f);
            }
        }
        Statement::If {
            then_body,
            else_body,
            ..
        } => {
            for st in then_body.iter().chain(else_body) {
                walk_assign_targets(st, f);
            }
        }
        Statement::Switch { cases, default, .. } => {
            for (_, sts) in cases {
                for st in sts {
                    walk_assign_targets(st, f);
                }
            }
            for st in default {
                walk_assign_targets(st, f);
            }
        }
        Statement::ForC {
            init, update, body, ..
        } => {
            walk_assign_targets(init, f);
            walk_assign_targets(update, f);
            for st in body {
                walk_assign_targets(st, f);
            }
        }
        _ => {}
    }
}

/// Rewrite an expression bottom-up (`f` sees each node after its children).
pub fn rewrite_expr(e: &Expr, f: &dyn Fn(Expr) -> Expr) -> Expr {
    let rebuilt = match e {
        Expr::ArrayAccess(n, idx) => Expr::ArrayAccess(n.clone(), Box::new(rewrite_expr(idx, f))),
        Expr::BinOp(l, op, r) => Expr::BinOp(
            Box::new(rewrite_expr(l, f)),
            op.clone(),
            Box::new(rewrite_expr(r, f)),
        ),
        Expr::Cast(t, i) => Expr::Cast(t.clone(), Box::new(rewrite_expr(i, f))),
        Expr::Not(i) => Expr::Not(Box::new(rewrite_expr(i, f))),
        Expr::AddressOf(i) => Expr::AddressOf(Box::new(rewrite_expr(i, f))),
        Expr::PostIncrement(i) => Expr::PostIncrement(Box::new(rewrite_expr(i, f))),
        Expr::PostDecrement(i) => Expr::PostDecrement(Box::new(rewrite_expr(i, f))),
        Expr::PreIncrement(i) => Expr::PreIncrement(Box::new(rewrite_expr(i, f))),
        Expr::PreDecrement(i) => Expr::PreDecrement(Box::new(rewrite_expr(i, f))),
        Expr::FuncCall(n, args) => {
            Expr::FuncCall(n.clone(), args.iter().map(|a| rewrite_expr(a, f)).collect())
        }
        Expr::Ternary(c, t, e2) => Expr::Ternary(
            Box::new(rewrite_expr(c, f)),
            Box::new(rewrite_expr(t, f)),
            Box::new(rewrite_expr(e2, f)),
        ),
        other => other.clone(),
    };
    f(rebuilt)
}

// ---------------------------------------------------------------------------
// Tests
// ---------------------------------------------------------------------------

#[cfg(test)]
mod tests {
    use super::*;
    use crate::ir::{Input, Output};

    fn var(n: &str) -> Expr {
        Expr::Var(n.into())
    }
    fn acc(a: &str, i: Expr) -> Expr {
        Expr::ArrayAccess(a.into(), Box::new(i))
    }
    fn assign(t: Expr, v: Expr) -> Statement {
        Statement::Assign {
            target: t,
            value: v,
            compound: false,
        }
    }
    fn decl(n: &str, t: VarType) -> Statement {
        Statement::VarDecl {
            var_type: t,
            name: n.into(),
            init: None,
        }
    }
    fn le(l: Expr, r: Expr) -> Expr {
        Expr::BinOp(Box::new(l), BinOp::LessEq, Box::new(r))
    }
    fn add(l: Expr, r: Expr) -> Expr {
        Expr::BinOp(Box::new(l), BinOp::Add, Box::new(r))
    }
    fn sub(l: Expr, r: Expr) -> Expr {
        Expr::BinOp(Box::new(l), BinOp::Sub, Box::new(r))
    }

    /// Minimal single-real-input, single-output FuncDef with the given body.
    fn func_with_body(body: Vec<Statement>) -> FuncDef {
        FuncDef {
            name: "TEST".into(),
            group: "Test".into(),
            description: None,
            camel_case: None,
            hint: None,
            flags: vec![],
            inputs: vec![Input {
                name: "inReal".into(),
                param_type: ParamType::Real,
            }],
            optional_inputs: vec![],
            outputs: vec![Output {
                name: "outReal".into(),
                param_type: ParamType::Real,
                flags: vec![],
            }],
            lookback: None,
            body,
            private_body: vec![],
            private_extra_params: vec![],
            private_param_init: vec![],
            has_explicit_private: false,
            header_comments: vec![],
            doc: None,
            streaming: false,
        }
    }

    /// `while(i <= endIdx) { out[outIdx] = in[i]*2; outIdx = outIdx+1; i = i+1; }`
    fn t1_body() -> Vec<Statement> {
        vec![
            decl("i", VarType::Integer),
            decl("outIdx", VarType::Integer),
            assign(var("outIdx"), Expr::IntLiteral(0)),
            assign(var("i"), var("startIdx")),
            Statement::While {
                condition: le(var("i"), var("endIdx")),
                body: vec![
                    assign(
                        acc("outReal", var("outIdx")),
                        Expr::BinOp(
                            Box::new(acc("inReal", var("i"))),
                            BinOp::Mul,
                            Box::new(Expr::Literal(2.0)),
                        ),
                    ),
                    assign(var("outIdx"), add(var("outIdx"), Expr::IntLiteral(1))),
                    assign(var("i"), add(var("i"), Expr::IntLiteral(1))),
                ],
            },
            Statement::Return { value: None },
        ]
    }

    #[test]
    fn t1_map_derives_t1() {
        let f = func_with_body(t1_body());
        let m = analyze(&f).expect("analyzes");
        assert_eq!(m.tier, StreamTier::T1);
        assert_eq!(m.cursor, "i");
        assert!(m.state.is_empty() && m.temps.is_empty() && m.lags.is_empty());
        assert_eq!(m.out_index_vars.iter().collect::<Vec<_>>(), ["outIdx"]);
    }

    #[test]
    fn recurrence_derives_t2_with_carried_state() {
        // prev = (in[i] - prev) + prev  (reads prev before writing -> carried)
        let f = func_with_body(vec![
            decl("i", VarType::Integer),
            decl("outIdx", VarType::Integer),
            decl("prev", VarType::Real),
            Statement::While {
                condition: le(var("i"), var("endIdx")),
                body: vec![
                    assign(
                        var("prev"),
                        add(sub(acc("inReal", var("i")), var("prev")), var("prev")),
                    ),
                    assign(acc("outReal", var("outIdx")), var("prev")),
                    assign(var("outIdx"), add(var("outIdx"), Expr::IntLiteral(1))),
                    assign(var("i"), add(var("i"), Expr::IntLiteral(1))),
                ],
            },
        ]);
        let m = analyze(&f).expect("analyzes");
        assert_eq!(m.tier, StreamTier::T2);
        assert_eq!(m.state, vec![("prev".into(), VarType::Real)]);
        assert!(m.temps.is_empty());
    }

    #[test]
    fn straight_line_temp_is_not_state() {
        // tmp = in[i]*2; out = tmp  -> tmp is a temp, tier T1.
        let f = func_with_body(vec![
            decl("i", VarType::Integer),
            decl("outIdx", VarType::Integer),
            decl("tmp", VarType::Real),
            Statement::While {
                condition: le(var("i"), var("endIdx")),
                body: vec![
                    assign(
                        var("tmp"),
                        Expr::BinOp(
                            Box::new(acc("inReal", var("i"))),
                            BinOp::Mul,
                            Box::new(Expr::Literal(2.0)),
                        ),
                    ),
                    assign(acc("outReal", var("outIdx")), var("tmp")),
                    assign(var("outIdx"), add(var("outIdx"), Expr::IntLiteral(1))),
                    assign(var("i"), add(var("i"), Expr::IntLiteral(1))),
                ],
            },
        ]);
        let m = analyze(&f).expect("analyzes");
        assert_eq!(m.tier, StreamTier::T1);
        assert_eq!(m.temps, vec![("tmp".into(), VarType::Real)]);
        assert!(m.state.is_empty());
    }

    #[test]
    fn conditional_write_is_carried_state() {
        // if (in[i] > 0) tmp = in[i];  out = tmp;  -> tmp carried (branchy).
        let f = func_with_body(vec![
            decl("i", VarType::Integer),
            decl("outIdx", VarType::Integer),
            decl("tmp", VarType::Real),
            Statement::While {
                condition: le(var("i"), var("endIdx")),
                body: vec![
                    Statement::If {
                        condition: Expr::BinOp(
                            Box::new(acc("inReal", var("i"))),
                            BinOp::Greater,
                            Box::new(Expr::Literal(0.0)),
                        ),
                        then_body: vec![assign(var("tmp"), acc("inReal", var("i")))],
                        else_body: vec![],
                        cond_comments: vec![],
                    },
                    assign(acc("outReal", var("outIdx")), var("tmp")),
                    assign(var("outIdx"), add(var("outIdx"), Expr::IntLiteral(1))),
                    assign(var("i"), add(var("i"), Expr::IntLiteral(1))),
                ],
            },
        ]);
        let m = analyze(&f).expect("analyzes");
        assert_eq!(m.tier, StreamTier::T2);
        assert_eq!(m.state, vec![("tmp".into(), VarType::Real)]);
    }

    #[test]
    fn lag_read_derives_t2_lag_slot() {
        // out = in[i] - in[i-1]
        let f = func_with_body(vec![
            decl("i", VarType::Integer),
            decl("outIdx", VarType::Integer),
            Statement::While {
                condition: le(var("i"), var("endIdx")),
                body: vec![
                    assign(
                        acc("outReal", var("outIdx")),
                        sub(
                            acc("inReal", var("i")),
                            acc("inReal", sub(var("i"), Expr::IntLiteral(1))),
                        ),
                    ),
                    assign(var("outIdx"), add(var("outIdx"), Expr::IntLiteral(1))),
                    assign(var("i"), add(var("i"), Expr::IntLiteral(1))),
                ],
            },
        ]);
        let m = analyze(&f).expect("analyzes");
        assert_eq!(m.tier, StreamTier::T2);
        assert_eq!(
            m.lags,
            vec![LagSlot {
                array: "inReal".into(),
                depth: 1
            }]
        );
    }

    #[test]
    fn trailing_window_derives_t3_ring() {
        // out = in[i] - in[trailingIdx]; trailingIdx++  -> T3 ring.
        let f = func_with_body(vec![
            decl("i", VarType::Integer),
            decl("outIdx", VarType::Integer),
            decl("trailingIdx", VarType::Integer),
            Statement::While {
                condition: le(var("i"), var("endIdx")),
                body: vec![
                    assign(
                        acc("outReal", var("outIdx")),
                        sub(acc("inReal", var("i")), acc("inReal", var("trailingIdx"))),
                    ),
                    assign(
                        var("trailingIdx"),
                        add(var("trailingIdx"), Expr::IntLiteral(1)),
                    ),
                    assign(var("outIdx"), add(var("outIdx"), Expr::IntLiteral(1))),
                    assign(var("i"), add(var("i"), Expr::IntLiteral(1))),
                ],
            },
        ]);
        let m = analyze(&f).expect("analyzes as T3");
        assert_eq!(m.tier, StreamTier::T3);
        assert_eq!(m.rings().len(), 1);
        assert_eq!(m.rings()[0].var, "trailingIdx");
        assert_eq!(m.rings()[0].arrays, ["inReal"]);
        // Transition: cap-0 guard first, ring read in the body, push+advance
        // at the end, and no trailingIdx leakage.
        let t = build_transition(&m, &TestNames).unwrap();
        let mut names = BTreeSet::new();
        for st in &t {
            stmt_var_names(st, &mut names);
        }
        assert!(!names.contains("trailingIdx"));
        assert!(names.contains("sp->ring_trailingIdx_inReal"));
        assert!(names.contains("sp->ringPos_trailingIdx"));
    }

    #[test]
    fn early_success_path_rejected() {
        // if (p == 1) { out[...] = in[...]; return TA_SUCCESS; }  <steady loop>
        let mut body = vec![
            decl("i", VarType::Integer),
            decl("outIdx", VarType::Integer),
            Statement::If {
                condition: Expr::BinOp(
                    Box::new(var("optInTimePeriod")),
                    BinOp::Eq,
                    Box::new(Expr::IntLiteral(1)),
                ),
                then_body: vec![
                    assign(acc("outReal", var("outIdx")), acc("inReal", var("i"))),
                    Statement::Return {
                        value: Some(var("TA_SUCCESS")),
                    },
                ],
                else_body: vec![],
                cond_comments: vec![],
            },
        ];
        body.extend(t1_body().into_iter().skip(2)); // reuse the steady loop tail
        let f = func_with_body(body);
        assert!(matches!(analyze(&f), Err(StreamError::Unsupported(_))));
    }

    #[test]
    fn no_data_guard_and_error_propagation_allowed() {
        // if (startIdx > endIdx) return TA_SUCCESS;  +  return retCode error path
        let mut body = vec![
            decl("i", VarType::Integer),
            decl("outIdx", VarType::Integer),
            decl("retCode", VarType::RetCodeType),
            Statement::If {
                condition: Expr::BinOp(
                    Box::new(var("startIdx")),
                    BinOp::Greater,
                    Box::new(var("endIdx")),
                ),
                then_body: vec![Statement::Return {
                    value: Some(var("TA_SUCCESS")),
                }],
                else_body: vec![],
                cond_comments: vec![],
            },
            Statement::If {
                condition: Expr::BinOp(
                    Box::new(var("retCode")),
                    BinOp::NotEq,
                    Box::new(var("TA_SUCCESS")),
                ),
                then_body: vec![Statement::Return {
                    value: Some(var("retCode")),
                }],
                else_body: vec![],
                cond_comments: vec![],
            },
        ];
        body.extend(t1_body().into_iter().skip(2));
        let f = func_with_body(body);
        assert!(analyze(&f).is_ok());
    }

    #[test]
    fn no_loop_rejected() {
        let f = func_with_body(vec![Statement::Return { value: None }]);
        assert!(matches!(analyze(&f), Err(StreamError::NoSteadyLoop)));
    }

    #[test]
    fn stateful_call_rejected() {
        let f = func_with_body(vec![
            decl("i", VarType::Integer),
            decl("outIdx", VarType::Integer),
            Statement::While {
                condition: le(var("i"), var("endIdx")),
                body: vec![
                    assign(
                        acc("outReal", var("outIdx")),
                        Expr::FuncCall("TA_GetCompatibility".into(), vec![]),
                    ),
                    assign(var("i"), add(var("i"), Expr::IntLiteral(1))),
                ],
            },
        ]);
        assert!(matches!(analyze(&f), Err(StreamError::UnsupportedCall(_))));
    }

    /// A lookup that knows no indicators (loop-tier unit tests).
    struct NoCallees;
    impl CalleeLookup for NoCallees {
        fn callee(&self, _name: &str) -> Option<CalleeSig> {
            None
        }
    }

    #[test]
    fn unanalyzable_declared_function_is_an_error() {
        let ok = func_with_body(t1_body());
        assert!(validate_streamable(&ok, &NoCallees).is_ok());
        let bad = func_with_body(vec![Statement::Return { value: None }]);
        let err = validate_streamable(&bad, &NoCallees).unwrap_err();
        assert!(err.contains("not streamable"), "{err}");
    }

    struct TestNames;
    impl NameMap for TestNames {
        fn state(&self, name: &str) -> String {
            format!("sp->{name}")
        }
        fn bar(&self, array: &str) -> String {
            array.to_string()
        }
        fn output(&self, name: &str) -> Expr {
            Expr::PointerDeref(format!("out_{name}"))
        }
        fn ring_buf(&self, var: &str, array: &str) -> String {
            format!("sp->ring_{var}_{array}")
        }
        fn ring_pos(&self, var: &str) -> String {
            format!("sp->ringPos_{var}")
        }
        fn ring_lag(&self, var: &str) -> String {
            format!("sp->ringLag_{var}")
        }
        fn ring_cap(&self, var: &str) -> String {
            format!("sp->ringCap_{var}")
        }
        fn win_buf(&self, var: &str, array: &str) -> String {
            format!("sp->win_{var}_{array}")
        }
        fn win_pos(&self, var: &str) -> String {
            format!("sp->winPos_{var}")
        }
        fn win_cap(&self, var: &str) -> String {
            format!("sp->winCap_{var}")
        }
        fn circ_buf(&self, storage: &str) -> String {
            format!("sp->cb_{storage}")
        }
        fn extrema_buf(&self, array: &str) -> String {
            format!("sp->x_{array}")
        }
        fn extrema_cap(&self) -> String {
            "sp->xCap".to_string()
        }
    }

    #[test]
    fn transition_drops_bookkeeping_and_rewrites() {
        let f = func_with_body(t1_body());
        let m = analyze(&f).unwrap();
        let t = build_transition(&m, &TestNames).unwrap();
        // Only the output write survives: *out_outReal = inReal * 2.0
        assert_eq!(t.len(), 1);
        match &t[0] {
            Statement::Assign { target, value, .. } => {
                assert!(matches!(target, Expr::PointerDeref(p) if p == "out_outReal"));
                let mut names = BTreeSet::new();
                expr_var_names(value, &mut names);
                assert!(names.contains("inReal"));
                assert!(!names.contains("i") && !names.contains("outIdx"));
            }
            other => panic!("unexpected stmt: {other:?}"),
        }
    }

    #[test]
    fn transition_appends_lag_shift() {
        let f = func_with_body(vec![
            decl("i", VarType::Integer),
            decl("outIdx", VarType::Integer),
            Statement::While {
                condition: le(var("i"), var("endIdx")),
                body: vec![
                    assign(
                        acc("outReal", var("outIdx")),
                        sub(
                            acc("inReal", var("i")),
                            acc("inReal", sub(var("i"), Expr::IntLiteral(2))),
                        ),
                    ),
                    assign(var("outIdx"), add(var("outIdx"), Expr::IntLiteral(1))),
                    assign(var("i"), add(var("i"), Expr::IntLiteral(1))),
                ],
            },
        ]);
        let m = analyze(&f).unwrap();
        let t = build_transition(&m, &TestNames).unwrap();
        // output write + lag2=lag1 + lag1=bar
        assert_eq!(t.len(), 3);
        match &t[1] {
            Statement::Assign { target, value, .. } => {
                assert!(matches!(target, Expr::Var(v) if v == "sp->lag2_inReal"));
                assert!(matches!(value, Expr::Var(v) if v == "sp->lag1_inReal"));
            }
            other => panic!("unexpected stmt: {other:?}"),
        }
        match &t[2] {
            Statement::Assign { target, value, .. } => {
                assert!(matches!(target, Expr::Var(v) if v == "sp->lag1_inReal"));
                assert!(matches!(value, Expr::Var(v) if v == "inReal"));
            }
            other => panic!("unexpected stmt: {other:?}"),
        }
    }

    #[test]
    fn countdown_loop_finds_input_cursor() {
        // nbBar countdown, today cursor (AD-style).
        let f = func_with_body(vec![
            decl("today", VarType::Integer),
            decl("outIdx", VarType::Integer),
            decl("nbBar", VarType::Integer),
            decl("ad", VarType::Real),
            Statement::While {
                condition: Expr::BinOp(
                    Box::new(var("nbBar")),
                    BinOp::NotEq,
                    Box::new(Expr::IntLiteral(0)),
                ),
                body: vec![
                    assign(var("ad"), add(var("ad"), acc("inReal", var("today")))),
                    assign(acc("outReal", var("outIdx")), var("ad")),
                    assign(var("outIdx"), add(var("outIdx"), Expr::IntLiteral(1))),
                    assign(var("today"), add(var("today"), Expr::IntLiteral(1))),
                    assign(var("nbBar"), sub(var("nbBar"), Expr::IntLiteral(1))),
                ],
            },
        ]);
        let m = analyze(&f).expect("analyzes");
        assert_eq!(m.loop_form, LoopForm::Countdown);
        assert_eq!(m.cursor, "today");
        assert_eq!(m.counter(), Some("nbBar"));
        assert_eq!(m.tier, StreamTier::T2);
        assert_eq!(m.state, vec![("ad".into(), VarType::Real)]);
        // Transition must drop today/outIdx/nbBar bookkeeping.
        let t = build_transition(&m, &TestNames).unwrap();
        assert_eq!(t.len(), 2); // ad update + output write
    }
}
