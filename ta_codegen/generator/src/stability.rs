//! Numerical-stability classification, derived rather than declared.
//!
//! The YAML `unstable_period` flag answers the ABI question — *does this function own a
//! tunable unstable period?* — which is what `TA_FUNC_FLG_UNST_PER` has always meant. The
//! documentation asks a different question: *can I compare this value across
//! different-length windows?* Those answers diverge, and the divergence is invisible in the
//! YAML:
//!
//! * `DEMA` declares nothing, but its lookback is `ema_lookback(period) * 2` — raise EMA's
//!   unstable period and DEMA's output moves. Same for TEMA, TRIX, MACD, MACDFIX, ADOSC
//!   (all through EMA), ADXR (through ADX) and STOCHRSI (through RSI).
//! * `BBANDS` is stable with one MA type and unstable with another, because `ma()`
//!   dispatches at run time. No static flag can express that.
//!
//! So stability is computed here from the call graph the parser already produces, and the
//! `ma()` dispatcher is treated as a *barrier*: reaching it means "depends on the MA type
//! selected", not "unstable", because which arm runs is the caller's choice.
//!
//! The walkers below deliberately match every `Statement`/`Expr` variant with no catch-all.
//! A missed variant would silently hide a call and publish "Start-Independent" for a
//! function that is not — so adding an IR variant must break this build, not this claim.

use std::collections::{BTreeSet, HashMap, HashSet};

use crate::ir::{Expr, FuncDef, LookbackExpr, ParamType, Statement};

/// The dynamic moving-average dispatcher. Calls into it are MA-type-conditional.
const MA_DISPATCHER: &str = "ma";

/// Why a function's output depends on how much history precedes it.
#[derive(Debug, Clone, Default, PartialEq, Eq)]
pub struct Stability {
    /// Declares `unstable_period`: it owns a `TA_FUNC_UNST_*` id of its own.
    pub intrinsic: bool,
    /// Reaches an intrinsically-unstable function through hard-coded calls. Names are
    /// upper-case function names (`EMA`), sorted; empty when there is no such path.
    pub inherited_from: Vec<String>,
    /// Takes an MAType parameter, or reaches `ma()`: some MA types carry an unstable
    /// period, so stability is the caller's choice.
    pub matype_dependent: bool,
    /// Declares `path_dependent`: never converges (a running accumulation).
    pub path_dependent: bool,
}

impl Stability {
    /// True when the output is unstable whatever the caller passes.
    #[must_use]
    pub fn unconditional(&self) -> bool {
        self.intrinsic || !self.inherited_from.is_empty()
    }
}

/// Classify every function. Keyed by upper-case function name.
#[must_use]
pub fn classify(funcs: &[FuncDef]) -> HashMap<String, Stability> {
    let intrinsic: HashSet<String> = funcs
        .iter()
        .filter(|f| f.flags.iter().any(|x| x == "unstable_period"))
        .map(|f| f.name.to_lowercase())
        .collect();
    let known: HashSet<String> = funcs.iter().map(|f| f.name.to_lowercase()).collect();
    let analysed: HashMap<String, (BTreeSet<String>, bool)> =
        funcs.iter().map(|f| (f.name.to_lowercase(), lookback_callees(f, &known))).collect();
    let callees: HashMap<String, BTreeSet<String>> =
        analysed.iter().map(|(k, v)| (k.clone(), v.0.clone())).collect();

    let mut out = HashMap::new();
    for f in funcs {
        let me = f.name.to_lowercase();
        let mut st = Stability {
            intrinsic: intrinsic.contains(&me),
            path_dependent: f.flags.iter().any(|x| x == "path_dependent"),
            matype_dependent: takes_matype(f)
                || analysed.get(&f.name.to_lowercase()).is_some_and(|(_, c)| *c),
            inherited_from: Vec::new(),
        };

        // Breadth-first over hard-coded calls. `ma()` is a barrier: it is reached, not
        // entered, because its arms are selected by the caller's MAType argument.
        let mut seen: HashSet<String> = HashSet::from([me.clone()]);
        let mut queue: Vec<String> = callees.get(&me).into_iter().flatten().cloned().collect();
        let mut found: BTreeSet<String> = BTreeSet::new();
        while let Some(next) = queue.pop() {
            if next == MA_DISPATCHER {
                st.matype_dependent = true;
                continue;
            }
            if !seen.insert(next.clone()) {
                continue;
            }
            if intrinsic.contains(&next) {
                found.insert(next.to_uppercase());
                continue; // Naming the nearest cause is enough; do not walk past it.
            }
            queue.extend(callees.get(&next).into_iter().flatten().cloned());
        }
        st.inherited_from = found.into_iter().collect();
        out.insert(f.name.clone(), st);
    }
    out
}

/// Does this function expose an MAType parameter the caller can set?
fn takes_matype(func: &FuncDef) -> bool {
    func.optional_inputs
        .iter()
        .any(|o| matches!(&o.param_type, ParamType::Enum(e) if e == "MAType"))
}

/// The functions this one's **lookback** calls, split into unconditional calls and calls
/// predicated on an MAType parameter.
///
/// The lookback, not the body, is what matters: an unstable period manifests as lookback
/// growth (`ema_lookback` = `period + TA_GetUnstablePeriod(TA_FUNC_UNST_EMA) - 1`), so a
/// function inherits instability exactly when its own lookback defers to another's. Two
/// consequences this gets right that a body scan does not:
///
/// * `SAR` calls `minus_dm(..., 1, ...)` in its body, but `minus_dm_lookback` short-circuits
///   to 1 for `period <= 1` without consulting the unstable period — and SAR's lookback
///   never calls it. No inheritance, which is what measuring the library confirms.
/// * `MACDEXT` delegates to `macd` only when all three MA types are EMA, and `MA` reaches
///   its arms through `switch(optInMAType)`. Both are the caller's choice, so they land in
///   the conditional bucket instead of being reported as hard-coded.
fn lookback_callees(func: &FuncDef, known: &HashSet<String>) -> (BTreeSet<String>, bool) {
    let matype_params: Vec<String> = func
        .optional_inputs
        .iter()
        .filter(|o| matches!(&o.param_type, ParamType::Enum(e) if e == "MAType"))
        .map(|o| o.name.clone())
        .collect();
    let mut names = BTreeSet::new();
    let mut conditional = BTreeSet::new();
    if let Some(LookbackExpr::Code(stmts)) = &func.lookback {
        for s in stmts {
            walk_stmt_predicated(s, &matype_params, false, &mut names, &mut conditional);
        }
    }
    let resolve = |set: &BTreeSet<String>| -> BTreeSet<String> {
        set.iter()
            .filter_map(|raw| {
                let base = raw
                    .strip_suffix("_lookback")
                    .or_else(|| raw.strip_suffix("_private"))
                    .unwrap_or(raw);
                known.contains(base).then(|| base.to_string())
            })
            .collect()
    };
    let unconditional = resolve(&names);
    let has_conditional = !resolve(&conditional).is_empty();
    (unconditional, has_conditional)
}

/// Walk a lookback statement, routing calls into `plain` or `predicated` depending on
/// whether they sit under a condition that tests an MAType parameter.
fn walk_stmt_predicated(
    stmt: &Statement,
    matype_params: &[String],
    under_matype: bool,
    plain: &mut BTreeSet<String>,
    predicated: &mut BTreeSet<String>,
) {
    let mentions_matype = |e: &Expr| {
        let mut vars = BTreeSet::new();
        collect_vars(e, &mut vars);
        matype_params.iter().any(|p| vars.contains(p))
    };
    match stmt {
        Statement::Switch { expr, cases, default } => {
            let cond = under_matype || mentions_matype(expr);
            for s in cases.iter().flat_map(|(_, b)| b).chain(default) {
                walk_stmt_predicated(s, matype_params, cond, plain, predicated);
            }
        }
        Statement::If { condition, then_body, else_body, .. } => {
            let cond = under_matype || mentions_matype(condition);
            for s in then_body.iter().chain(else_body.iter()) {
                walk_stmt_predicated(s, matype_params, cond, plain, predicated);
            }
        }
        other => {
            let mut found = BTreeSet::new();
            walk_stmt(other, &mut found);
            if under_matype {
                predicated.extend(found);
            } else {
                plain.extend(found);
            }
        }
    }
}

/// Every variable name appearing in an expression.
pub(crate) fn collect_vars(expr: &Expr, out: &mut BTreeSet<String>) {
    match expr {
        Expr::Var(v) | Expr::PointerDeref(v) => {
            out.insert(v.clone());
        }
        Expr::ArrayAccess(v, i) => {
            out.insert(v.clone());
            collect_vars(i, out);
        }
        Expr::FuncCall(_, args) => {
            for a in args {
                collect_vars(a, out);
            }
        }
        Expr::BinOp(a, _, b) => {
            collect_vars(a, out);
            collect_vars(b, out);
        }
        Expr::Ternary(a, b, c) => {
            collect_vars(a, out);
            collect_vars(b, out);
            collect_vars(c, out);
        }
        Expr::Cast(_, e)
        | Expr::Not(e)
        | Expr::BitwiseNot(e)
        | Expr::AddressOf(e)
        | Expr::PostIncrement(e)
        | Expr::PostDecrement(e)
        | Expr::PreIncrement(e)
        | Expr::PreDecrement(e) => collect_vars(e, out),
        Expr::Literal(_) | Expr::IntLiteral(_) => {}
    }
}


/// Collect every `FuncCall` name reachable from a statement.
///
/// Exhaustive by construction — see the module note on why there is no `_` arm.
fn walk_stmt(stmt: &Statement, out: &mut BTreeSet<String>) {
    match stmt {
        Statement::VarDecl { init, .. } => {
            if let Some(e) = init {
                walk_expr(e, out);
            }
        }
        Statement::Assign { target, value, .. } => {
            walk_expr(target, out);
            walk_expr(value, out);
        }
        Statement::While { condition, body } | Statement::DoWhile { condition, body } => {
            walk_expr(condition, out);
            for s in body {
                walk_stmt(s, out);
            }
        }
        Statement::For { count, body, .. } => {
            walk_expr(count, out);
            for s in body {
                walk_stmt(s, out);
            }
        }
        Statement::ForC { init, condition, update, body } => {
            walk_stmt(init, out);
            walk_stmt(update, out);
            walk_expr(condition, out);
            for s in body {
                walk_stmt(s, out);
            }
        }
        Statement::If { condition, then_body, else_body, .. } => {
            walk_expr(condition, out);
            for s in then_body.iter().chain(else_body.iter()) {
                walk_stmt(s, out);
            }
        }
        Statement::Switch { expr, cases, default } => {
            walk_expr(expr, out);
            for (_, body) in cases {
                for s in body {
                    walk_stmt(s, out);
                }
            }
            for s in default {
                walk_stmt(s, out);
            }
        }
        Statement::Block { body } => {
            for s in body {
                walk_stmt(s, out);
            }
        }
        Statement::Return { value } => {
            if let Some(e) = value {
                walk_expr(e, out);
            }
        }
        Statement::Expr(e) => walk_expr(e, out),
        Statement::CircBuf(_)
        | Statement::UnrollHint { .. }
        | Statement::Comment(_)
        | Statement::Break
        | Statement::Continue => {}
    }
}

/// Collect every `FuncCall` name reachable from an expression.
fn walk_expr(expr: &Expr, out: &mut BTreeSet<String>) {
    match expr {
        Expr::FuncCall(name, args) => {
            out.insert(name.clone());
            for a in args {
                walk_expr(a, out);
            }
        }
        Expr::BinOp(a, _, b) => {
            walk_expr(a, out);
            walk_expr(b, out);
        }
        Expr::Ternary(a, b, c) => {
            walk_expr(a, out);
            walk_expr(b, out);
            walk_expr(c, out);
        }
        Expr::ArrayAccess(_, i) => walk_expr(i, out),
        Expr::Cast(_, e)
        | Expr::Not(e)
        | Expr::BitwiseNot(e)
        | Expr::AddressOf(e)
        | Expr::PostIncrement(e)
        | Expr::PostDecrement(e)
        | Expr::PreIncrement(e)
        | Expr::PreDecrement(e) => walk_expr(e, out),
        Expr::Literal(_) | Expr::IntLiteral(_) | Expr::Var(_) | Expr::PointerDeref(_) => {}
    }
}
