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

/// Everything the stream emitters need, derived from one [`FuncDef`].
#[derive(Debug)]
pub struct StreamModel<'a> {
    pub func: &'a FuncDef,
    /// The body `open()` transcribes: guarded body, or the private body when
    /// the function has an explicit `_private` variant.
    pub body: &'a [Statement],
    /// Derived tier (must match the YAML declaration).
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
    /// Countdown loops only: the iteration-count variable (dropped).
    pub counter: Option<String>,
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
    /// Bounded look-back lags, in input signature order.
    pub lags: Vec<LagSlot>,
    /// Trailing-window rings (T3), one per trailing index variable.
    pub rings: Vec<RingSpec>,
    /// Bounded rescan windows (T3), one per inner-loop counter variable.
    pub windows: Vec<WindowSpec>,
    /// CIRCBUF-backed buffers referenced by the steady loop.
    pub circs: Vec<CircState>,
    /// Absolute-index automaton (T4). When set, the cursor is carried state
    /// (not dropped) and inputs are read through the automaton ring.
    pub extrema: Option<ExtremaState>,
    /// Recognized param==1 identity path, if the batch body has one.
    pub identity: Option<IdentityPath>,
}

impl StreamModel<'_> {
    /// Variables that exist only to walk the batch arrays and are dropped
    /// from the transition (their bookkeeping statements are deleted).
    #[must_use]
    pub fn dropped_vars(&self) -> BTreeSet<String> {
        let mut d: BTreeSet<String> = self.out_index_vars.clone();
        if self.extrema.is_none() {
            // Extrema automatons carry the cursor as absolute state.
            d.insert(self.cursor.clone());
        }
        if let Some(c) = &self.counter {
            d.insert(c.clone());
        }
        for r in &self.rings {
            d.insert(r.var.clone());
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

fn stmt_var_names(s: &Statement, out: &mut BTreeSet<String>) {
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
    if body.is_empty() {
        return Err(StreamError::Unsupported("empty body".into()));
    }

    let bar_inputs = input_array_names(func);
    let outputs: Vec<String> = func.outputs.iter().map(|o| o.name.clone()).collect();
    for o in &func.outputs {
        if !matches!(o.param_type, ParamType::Real | ParamType::Integer) {
            return Err(StreamError::Unsupported(format!(
                "unsupported output type for `{}`",
                o.name
            )));
        }
    }

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
    let (circs, circ_extra) = discover_circs(body, &steady_stmts);

    let ctx = ClassifyCtx {
        body,
        steady_stmts: &steady_stmts,
        func,
        cursor: &cursor,
        counter: counter.as_deref(),
        out_index_vars: &out_index_vars,
        bar_inputs: &bar_inputs,
        outputs: &outputs,
        circ_extra: &circ_extra,
    };
    let (extrema, (mut state, temps)) =
        classify_or_extrema(&ctx, &ring_vars, &trailing, &windows, &circs)?;
    let rings_effective: Vec<RingSpec> = if extrema.is_some() { Vec::new() } else { rings };
    force_circ_index_state(&circs, &mut state, &temps);

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

    let tier = derive_tier(
        &state,
        &lag_slots,
        &rings_effective,
        &windows,
        &circs,
        counter.as_deref(),
        extrema.as_ref(),
    );

    Ok(StreamModel {
        func,
        seed_boundary,
        out_feedback,
        body,
        tier,
        loop_form,
        steady_stmts,
        cursor,
        counter,
        out_index_vars,
        bar_inputs,
        outputs,
        state,
        temps,
        lags: lag_slots,
        rings: rings_effective,
        windows,
        circs,
        extrema,
        identity,
    })
}

/// Validate that a `streaming: true` function is still analyzable. Any
/// failure is a generation-time error (the maintenance-coupling gate from
/// the proposal): a batch rewrite that breaks stream analyzability fails
/// HERE, not at release. The tier is derived, never declared.
pub fn validate_streamable(func: &FuncDef) -> Result<StreamModel<'_>, String> {
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
    let model = analyze(func).map_err(|e| {
        format!(
            "{}: YAML declares `streaming: true` but the function is not streamable at stage 1: {e}",
            func.name
        )
    })?;
    // The transition must BUILD, too — analysis success alone would let a
    // seeded function pass the gate and then panic in the emitter.
    build_transition(&model, &GateNames).map_err(|e| {
        format!(
            "{}: streamable by analysis but the transition cannot be built: {e}",
            func.name
        )
    })?;
    Ok(model)
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
    // Open and StreamStep (settings-stability rule), so they are neither
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
    for ring in &model.rings {
        push_ring_advance(&mut out, ring, names);
    }
    // Rescan windows: the current bar was written at `pos` before the body;
    // advance the position for the next update.
    for win in &model.windows {
        push_window_advance(&mut out, win, names);
    }
    Ok(out)
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
    if let Some(ex) = &model.extrema {
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
    for win in model.windows.iter().rev() {
        for arr in win.arrays.iter().rev() {
            out.insert(0, window_prewrite(win, arr, names));
        }
    }
    for ring in model.rings.iter().rev() {
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
            if model.extrema.is_some() && model.bar_inputs.contains(&n) =>
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
                    if model.rings.iter().any(|r| r.var == v) =>
                {
                    let ring = model.rings.iter().find(|r| r.var == v).unwrap();
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
                    if model.rings.iter().any(|r| r.var == v) =>
                {
                    let ring = model.rings.iter().find(|r| r.var == v).unwrap();
                    ring_offset_read(ring, &n, Some(k), None, names)
                }
                InputIndex::OtherVarWinLag(v, w)
                    if model.rings.iter().any(|r| r.var == v) =>
                {
                    let ring = model.rings.iter().find(|r| r.var == v).unwrap();
                    ring_offset_read(ring, &n, None, Some(Expr::Var(w)), names)
                }
                InputIndex::WindowVar(w0) => {
                    // Bottom-up rewriting may already have state-mapped the
                    // offset var; resolve back to the window it belongs to.
                    let win = model.windows.iter().find(|win| {
                        win.var == w0 || names.state(&win.var) == w0
                    });
                    match win {
                        Some(win) => {
                            // win[(pos + cap - w) % cap]: bar `w` back from
                            // the current one (slot `pos` holds the current
                            // bar — written before the transition body).
                            let pos = Expr::Var(names.win_pos(&win.var));
                            let cap = Expr::Var(names.win_cap(&win.var));
                            let idx_expr = Expr::BinOp(
                                Box::new(Expr::BinOp(
                                    Box::new(Expr::BinOp(
                                        Box::new(pos),
                                        BinOp::Add,
                                        Box::new(cap.clone()),
                                    )),
                                    BinOp::Sub,
                                    Box::new(Expr::Var(w0)),
                                )),
                                BinOp::Mod,
                                Box::new(cap),
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
                .circs
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
            if model.circs.iter().any(|c| c.id == *id) =>
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
        assert_eq!(m.rings.len(), 1);
        assert_eq!(m.rings[0].var, "trailingIdx");
        assert_eq!(m.rings[0].arrays, ["inReal"]);
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

    #[test]
    fn unanalyzable_declared_function_is_an_error() {
        let ok = func_with_body(t1_body());
        assert!(validate_streamable(&ok).is_ok());
        let bad = func_with_body(vec![Statement::Return { value: None }]);
        let err = validate_streamable(&bad).unwrap_err();
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
        assert_eq!(m.counter.as_deref(), Some("nbBar"));
        assert_eq!(m.tier, StreamTier::T2);
        assert_eq!(m.state, vec![("ad".into(), VarType::Real)]);
        // Transition must drop today/outIdx/nbBar bookkeeping.
        let t = build_transition(&m, &TestNames).unwrap();
        assert_eq!(t.len(), 2); // ad update + output write
    }
}
