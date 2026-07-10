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

use crate::ir::{BinOp, Expr, FuncDef, ParamType, Statement, StreamTier, VarType};

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
    /// Countdown loops only: the iteration-count variable (dropped).
    pub counter: Option<String>,
    /// Output-index variables (dropped in the transition).
    pub out_index_vars: BTreeSet<String>,
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
    /// Recognized param==1 identity path, if the batch body has one.
    pub identity: Option<IdentityPath>,
}

impl StreamModel<'_> {
    /// Variables that exist only to walk the batch arrays and are dropped
    /// from the transition (their bookkeeping statements are deleted).
    #[must_use]
    pub fn dropped_vars(&self) -> BTreeSet<String> {
        let mut d: BTreeSet<String> = self.out_index_vars.clone();
        d.insert(self.cursor.clone());
        if let Some(c) = &self.counter {
            d.insert(c.clone());
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

fn check_return_paths(body: &[Statement], skip_idx: Option<usize>) -> Result<(), StreamError> {
    fn is_no_data_guard(cond: &Expr) -> bool {
        match cond {
            Expr::BinOp(l, BinOp::Greater, r) => {
                matches!(l.as_ref(), Expr::Var(v) if v == "startIdx")
                    && matches!(r.as_ref(), Expr::Var(v) if v == "endIdx")
            }
            Expr::BinOp(l, BinOp::Less, r) => {
                matches!(l.as_ref(), Expr::Var(v) if v == "endIdx")
                    && matches!(r.as_ref(), Expr::Var(v) if v == "startIdx")
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
    fn walk(
        stmts: &[Statement],
        in_guard: bool,
        is_top: bool,
        skip_idx: Option<usize>,
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
                }
                Statement::If {
                    condition,
                    then_body,
                    else_body,
                    ..
                } => {
                    let guard = in_guard || is_no_data_guard(condition);
                    walk(then_body, guard, false, None)?;
                    walk(else_body, in_guard, false, None)?;
                }
                Statement::While { body, .. }
                | Statement::DoWhile { body, .. }
                | Statement::For { body, .. }
                | Statement::Block { body } => walk(body, in_guard, false, None)?,
                Statement::Switch { cases, default, .. } => {
                    for (_, sts) in cases {
                        walk(sts, in_guard, false, None)?;
                    }
                    walk(default, in_guard, false, None)?;
                }
                Statement::ForC { init, update, body, .. } => {
                    walk(std::slice::from_ref(init), in_guard, false, None)?;
                    walk(std::slice::from_ref(update), in_guard, false, None)?;
                    walk(body, in_guard, false, None)?;
                }
                _ => {}
            }
        }
        Ok(())
    }
    walk(body, false, true, skip_idx)
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
    lower.contains("lookback")
        || lower.contains("compatibility")
        || lower.contains("unstable")
        || lower.contains("candle")
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
        if o.param_type != ParamType::Real {
            return Err(StreamError::Unsupported(format!(
                "non-real output `{}` is beyond stage 1",
                o.name
            )));
        }
    }

    let param_name_list: Vec<String> = func
        .optional_inputs
        .iter()
        .map(|p| p.name.clone())
        .collect();
    let (identity, identity_idx) =
        detect_identity_path(body, &bar_inputs, &outputs, &param_name_list);
    check_return_paths(body, identity_idx)?;

    let steady = find_steady_loop(body)?;

    // Transition source: loop body (+ `for` increment clause).
    let mut steady_stmts: Vec<Statement> = steady.body.to_vec();
    if let Some(u) = steady.for_update {
        steady_stmts.push(u.clone());
    }

    // --- cursor -----------------------------------------------------------
    let (cursor, counter) = if steady.form == LoopForm::Countdown {
        let counter = countdown_counter(steady.condition).ok_or(StreamError::NoSteadyLoop)?;
        let c = countdown_cursor(&steady_stmts, &bar_inputs, &counter)?;
        (c, Some(counter))
    } else {
        let c = endidx_cursor(steady.condition).ok_or(StreamError::NoSteadyLoop)?;
        (c, None)
    };

    // --- array accesses ----------------------------------------------------
    let (lags, out_index_vars) = scan_accesses(&steady_stmts, &bar_inputs, &outputs, &cursor)?;

    // Outputs may only appear as assignment targets (no read-back).
    for s in &steady_stmts {
        if let Some(name) = output_read_back(s, &outputs) {
            return Err(StreamError::UnsupportedAccess(format!(
                "output `{name}` is read back in the steady loop"
            )));
        }
    }

    // --- locals ------------------------------------------------------------
    let (state, temps) = classify_locals(
        body,
        &steady_stmts,
        func,
        &cursor,
        counter.as_deref(),
        &out_index_vars,
        &bar_inputs,
        &outputs,
    )?;

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

    let tier = if state.is_empty() && lag_slots.is_empty() && counter.is_none() {
        StreamTier::T1
    } else {
        StreamTier::T2
    };

    Ok(StreamModel {
        func,
        body,
        tier,
        loop_form: steady.form,
        steady_stmts,
        cursor,
        counter,
        out_index_vars,
        bar_inputs,
        outputs,
        state,
        temps,
        lags: lag_slots,
        identity,
    })
}

/// Validate that a `streaming: true` function is still analyzable. Any
/// failure is a generation-time error (the maintenance-coupling gate from
/// the proposal): a batch rewrite that breaks stream analyzability fails
/// HERE, not at release. The tier is derived, never declared.
pub fn validate_streamable(func: &FuncDef) -> Result<StreamModel<'_>, String> {
    analyze(func).map_err(|e| {
        format!(
            "{}: YAML declares `streaming: true` but the function is not streamable at stage 1: {e}",
            func.name
        )
    })
}

/// Scan the transition statements for array accesses and stateful calls.
/// Returns (per-array max lag depth, output-index variables).
fn scan_accesses(
    steady_stmts: &[Statement],
    bar_inputs: &[String],
    outputs: &[String],
    cursor: &str,
) -> Result<(BTreeMap<String, i64>, BTreeSet<String>), StreamError> {
    let mut lags: BTreeMap<String, i64> = BTreeMap::new();
    let mut out_index_vars: BTreeSet<String> = BTreeSet::new();
    let mut access_err: Option<StreamError> = None;
    for s in steady_stmts {
        walk_stmt_exprs(s, &mut |e| {
            walk_expr(e, &mut |x| {
                if access_err.is_some() {
                    return;
                }
                if let Expr::ArrayAccess(name, idx) = x {
                    if bar_inputs.iter().any(|i| i == name) {
                        match classify_input_index(idx, cursor) {
                            InputIndex::Current => {}
                            InputIndex::Lag(k) => {
                                let d = lags.entry(name.clone()).or_insert(0);
                                *d = (*d).max(k);
                            }
                            InputIndex::OtherVar(v) => {
                                access_err = Some(StreamError::NeedsRing(v));
                            }
                            InputIndex::Unsupported => {
                                access_err = Some(StreamError::UnsupportedAccess(format!(
                                    "{name}[{idx:?}]"
                                )));
                            }
                        }
                    } else if outputs.iter().any(|o| o == name) {
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
    Ok((lags, out_index_vars))
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
    bar_inputs: &[String],
    outputs: &[String],
) -> Result<(Vec<ScalarField>, Vec<ScalarField>), StreamError> {
    let mut decls = BTreeMap::new();
    collect_var_decls(body, &mut decls);

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
            || param_names.contains(v)
            || bar_inputs.contains(v)
            || outputs.contains(v)
            || v == "endIdx"
            || v == "startIdx"
        {
            continue;
        }
        match decls.get(v) {
            Some(VarType::Real | VarType::Integer | VarType::Index) => candidates.push(v.clone()),
            Some(_) => return Err(StreamError::NonScalarState(v.clone())),
            None => return Err(StreamError::UnknownSymbol(v.clone())),
        }
    }

    // Bookkeeping purity: statements the transition deleter will drop
    // must not read or mutate anything but index vars — a side effect
    // hiding in a dropped statement would be lost silently.
    check_bookkeeping_purity(steady_stmts, cursor, counter, out_index_vars)?;

    // Carried vs temp split (sound): a candidate is a TEMP only if the
    // straight-line prefix of the transition assigns it whole (non-compound)
    // before any read and before the first branching statement; everything
    // else is carried state.
    let temps_set = written_before_read(steady_stmts, &candidates);

    // Deterministic order: follow first-declaration order in the body.
    let mut decl_order: Vec<String> = Vec::new();
    collect_decl_order(body, &mut decl_order);
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
fn output_read_back(s: &Statement, outputs: &[String]) -> Option<String> {
    let mut hit: Option<String> = None;
    let mut check_value = |e: &Expr| {
        walk_expr(e, &mut |x| {
            if let Expr::ArrayAccess(n, _) = x {
                if outputs.iter().any(|o| o == n) && hit.is_none() {
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
        &|s| drop_bookkeeping(s, &dropped, model, names),
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
    if let Some(b) = identity_branch {
        out.insert(0, b);
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
    Ok(out)
}

fn rewrite_expr_for_transition(
    e: Expr,
    model: &StreamModel,
    names: &dyn NameMap,
    state_names: &BTreeSet<String>,
) -> Expr {
    match e {
        Expr::ArrayAccess(n, idx) if model.bar_inputs.contains(&n) => {
            match classify_input_index(&idx, &model.cursor) {
                InputIndex::Current => Expr::Var(names.bar(&n)),
                InputIndex::Lag(k) => Expr::Var(names.state(&StreamModel::lag_field(&n, k))),
                _ => Expr::ArrayAccess(n, idx),
            }
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
    fn trailing_window_rejected_as_ring() {
        // out = in[i] - in[trailingIdx]; trailingIdx++  -> T3, rejected.
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
        assert!(matches!(
            analyze(&f),
            Err(StreamError::NeedsRing(v)) if v == "trailingIdx"
        ));
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
