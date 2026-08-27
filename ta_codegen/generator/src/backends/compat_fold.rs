//! Render-time constant-folding of conditions, shared by the managed backends
//! (Java, C#).
//!
//! Two folds funnel through the one propagation engine here:
//!
//! - **Compatibility fold.** Metastock compatibility is retired project-wide and
//!   the managed `Core`s carry no compatibility field at all, so every
//!   `TA_GetCompatibility()` test in the shared input sources is a compile-time
//!   constant on those backends: `== DEFAULT` is true, `== METASTOCK` is false,
//!   and the surviving `if` arm is spliced in place of the branch. C (which
//!   still ships the setting) renders the same IR untouched.
//!
//! - **Float-overload comparison fold (C# only, statement level).** In the
//!   single-precision variants the inputs are `float[]` while outputs are
//!   `double[]`, so an input↔output identity test can never be true — and C#
//!   additionally rejects `float[] == double[]` outright and promotes the
//!   then-arm of a literal `if (false)` to a CS0162 unreachable-code error
//!   under `-warnaserror`. Folding the whole statement removes both problems.
//!   (Java renders the same comparisons as `false`/`true` literals instead,
//!   which javac accepts; that behaviour is unchanged.)
//!
//! - **Answered cross-call guard fold (Rust, Java, C#).** Since #267 a
//!   cross-indicator call enters the callee's PUBLIC tier, and these three turn
//!   its rejection into an early return or a throw before the assignment to
//!   `retCode` is reached — so the transcribed `if( retCode != TA_SUCCESS )`
//!   that follows can no longer be taken. See
//!   [`drop_answered_cross_call_guards`]. C is untouched: it really does return
//!   a code, and its emitters never call this.
//!
//! The engine handles both operand orders and propagates through `&&` / `||` /
//! `!`, so a compound test such as `unstablePeriod == 0 && COMPATIBILITY() ==
//! METASTOCK` collapses whole.
//!
//! **The dropping is only safe while the sibling operand is a pure read**, and
//! that is a property of the leaf classifiers, not of the engine: an absorbed
//! `&&` discards its LEFT operand, which C evaluates unconditionally. The two
//! compatibility leaves sit beside transcribed reads, and the cross-call leaf
//! beside an out-param count test; none of the corpus writes inside a
//! condition. A leaf that could sit beside an increment or a call would need
//! this engine to refuse rather than absorb.

use crate::ir::{BinOp, Expr, Statement};

/// Result of folding a condition against a backend's compile-time constants.
pub(crate) enum CondFold {
    /// The condition is a compile-time constant on this backend.
    Known(bool),
    /// Not constant, or only partly folded. `changed` is false when nothing
    /// folded, so the caller can take the untouched rendering path.
    Open { expr: Expr, changed: bool },
}

impl CondFold {
    /// An operand that folded away nothing.
    fn unchanged(expr: &Expr) -> Self {
        CondFold::Open { expr: expr.clone(), changed: false }
    }
}

/// Fold `expr` against a leaf classifier: `leaf` returns `Some(truth)` for a
/// sub-expression that is a compile-time constant on the calling backend, and
/// `None` otherwise. The engine owns the `&&`/`||`/`!` propagation.
pub(crate) fn fold_cond(expr: &Expr, leaf: &dyn Fn(&Expr) -> Option<bool>) -> CondFold {
    if let Some(k) = leaf(expr) {
        return CondFold::Known(k);
    }
    match expr {
        Expr::BinOp(lhs, op @ (BinOp::And | BinOp::Or), rhs) => {
            let is_and = matches!(op, BinOp::And);
            let (l, r) = (fold_cond(lhs, leaf), fold_cond(rhs, leaf));
            match (l, r) {
                // `x && false` / `x || true` — the absorbing element wins.
                (CondFold::Known(k), _) | (_, CondFold::Known(k)) if k != is_and => {
                    CondFold::Known(k)
                }
                (CondFold::Known(_), CondFold::Known(_)) => CondFold::Known(is_and),
                // `x && true` / `x || false` — the identity element drops out.
                (CondFold::Known(_), CondFold::Open { expr, .. })
                | (CondFold::Open { expr, .. }, CondFold::Known(_)) => {
                    CondFold::Open { expr, changed: true }
                }
                (
                    CondFold::Open { expr: le, changed: lc },
                    CondFold::Open { expr: re, changed: rc },
                ) => CondFold::Open {
                    expr: Expr::BinOp(Box::new(le), op.clone(), Box::new(re)),
                    changed: lc || rc,
                },
            }
        }
        Expr::Not(inner) => match fold_cond(inner, leaf) {
            CondFold::Known(k) => CondFold::Known(!k),
            CondFold::Open { expr, changed } => {
                CondFold::Open { expr: Expr::Not(Box::new(expr)), changed }
            }
        },
        _ => CondFold::unchanged(expr),
    }
}


/// Fold away the guard on a cross-indicator call whose rejection the calling
/// backend has already answered, over one function body.
///
/// **The fact this rests on is about ONE STATEMENT, not about a variable.** The
/// tempting rule — "`retCode` holds `Success` until it is next assigned" — is a
/// dataflow claim, and it is false at a `switch` arm, at an `if`/`else` join and
/// across a loop back-edge. `MA`'s ten dispatch arms are the standing
/// counter-example: each ends in that assignment and all ten are read by the one
/// `return retCode` after the switch. So the scan is deliberately timid, and
/// each of its four properties kills one of those:
///
/// - forward only, and within a **single** `Vec<Statement>` — never into or out
///   of a nested body, so nothing a re-entry can reach is ever folded;
/// - it stops at the first control-flow statement, and folds only when that
///   statement is the `If` itself;
/// - it skips an intervening statement only while that statement does not
///   mention the variable — `macdext.c` puts two `free()` calls between the call
///   and its guard, so strict adjacency would silently miss those sites;
/// - `admits` is the **caller's own** admission test. Rust declines shapes Java
///   and C# accept, and on a declined shape the call falls through to the plain
///   renderer, where the guard is genuinely live. A shared predicate here would
///   turn that documented safe degradation into a swallowed error code.
///
/// Only `!= SUCCESS` is classified, never `== SUCCESS`: the corpus contains no
/// `== TA_SUCCESS` test, so refusing it costs nothing and makes `Known(true)` —
/// the case that would splice an arm — unreachable by construction rather than
/// merely unobserved.
///
/// **Statements are replaced, never removed.** The streaming emitters address
/// the body BY INDEX (their `inserts` / `replaced` sets), so a length change
/// would desynchronize every one of them. A guard that folds away becomes an
/// empty `Statement::Block`, which renders to nothing.
pub(crate) fn drop_answered_cross_call_guards(
    body: &[Statement],
    admits: &dyn Fn(&str, &[Expr]) -> bool,
) -> Vec<Statement> {
    let mut out: Vec<Statement> = body.iter().map(|s| fold_nested(s, admits)).collect();

    // Collected before any rewrite: folding while scanning would let a rewritten
    // guard become the stopping statement of a later scan.
    let mut targets: Vec<(usize, String)> = Vec::new();
    for i in 0..out.len() {
        if let Some(var) = answered_cross_call_var(&out[i], admits) {
            if let Some(j) = guard_index(&out, i + 1, &var) {
                targets.push((j, var));
            }
        }
    }
    for (j, var) in targets {
        if let Some(folded) = fold_guard(&out[j], &var) {
            out[j] = folded;
        }
    }
    out
}

/// Apply the pass to every nested body, leaving this statement's own shape alone.
fn fold_nested(s: &Statement, admits: &dyn Fn(&str, &[Expr]) -> bool) -> Statement {
    let go = |b: &[Statement]| drop_answered_cross_call_guards(b, admits);
    match s {
        Statement::While { condition, body } => {
            Statement::While { condition: condition.clone(), body: go(body) }
        }
        Statement::DoWhile { condition, body } => {
            Statement::DoWhile { condition: condition.clone(), body: go(body) }
        }
        Statement::For { var, count, body } => Statement::For {
            var: var.clone(),
            count: count.clone(),
            body: go(body),
        },
        Statement::ForC { init, condition, update, body } => Statement::ForC {
            init: Box::new(fold_nested(init, admits)),
            condition: condition.clone(),
            update: Box::new(fold_nested(update, admits)),
            body: go(body),
        },
        Statement::If { condition, then_body, else_body, cond_comments } => Statement::If {
            condition: condition.clone(),
            then_body: go(then_body),
            else_body: go(else_body),
            cond_comments: cond_comments.clone(),
        },
        Statement::Switch { expr, cases, default } => Statement::Switch {
            expr: expr.clone(),
            cases: cases.iter().map(|(v, b)| (v.clone(), go(b))).collect(),
            default: go(default),
        },
        Statement::Block { body } => Statement::Block { body: go(body) },
        other => other.clone(),
    }
}

/// The assignment target of a cross-indicator call this backend admits, i.e. one
/// whose rejection it answers itself.
fn answered_cross_call_var(
    s: &Statement,
    admits: &dyn Fn(&str, &[Expr]) -> bool,
) -> Option<String> {
    match s {
        Statement::Assign { target: Expr::Var(v), value: Expr::FuncCall(f, args), compound: false }
            if admits(f, args) =>
        {
            Some(v.clone())
        }
        _ => None,
    }
}

/// Index of the guard on `var`, scanning forward from `from`. `None` as soon as
/// anything could have intervened — see the four properties on the entry point.
fn guard_index(body: &[Statement], from: usize, var: &str) -> Option<usize> {
    for (k, st) in body.iter().enumerate().skip(from) {
        if let Statement::If { condition, .. } = st {
            return expr_mentions(condition, var).then_some(k);
        }
        if is_control_flow(st) || stmt_mentions(st, var) {
            return None;
        }
    }
    None
}

fn is_control_flow(s: &Statement) -> bool {
    matches!(
        s,
        Statement::While { .. }
            | Statement::DoWhile { .. }
            | Statement::For { .. }
            | Statement::ForC { .. }
            | Statement::Switch { .. }
            | Statement::Return { .. }
            | Statement::Break
            | Statement::Continue
            | Statement::Block { .. }
    )
}

fn expr_mentions(e: &Expr, var: &str) -> bool {
    let mut hit = false;
    crate::streaming::walk_expr(e, &mut |x| {
        if matches!(x, Expr::Var(n) if n == var) {
            hit = true;
        }
    });
    hit
}

fn stmt_mentions(s: &Statement, var: &str) -> bool {
    let mut hit = false;
    crate::streaming::walk_stmt_exprs(s, &mut |e| {
        if expr_mentions(e, var) {
            hit = true;
        }
    });
    hit
}

/// The rewritten guard, or `None` to leave it exactly as it was.
fn fold_guard(s: &Statement, var: &str) -> Option<Statement> {
    let Statement::If { condition, then_body, else_body, .. } = s else {
        return None;
    };
    let leaf = |e: &Expr| is_not_success(e, var).then_some(false);
    match fold_cond(condition, &leaf) {
        // The whole test is dead. Splice the else arm rather than dropping it
        // with the branch -- no shipped site has one, and a rule that silently
        // discarded it would invert the logic the day one appears.
        CondFold::Known(false) => Some(Statement::Block { body: else_body.clone() }),
        CondFold::Open { expr, changed: true } => Some(Statement::If {
            condition: expr,
            then_body: then_body.clone(),
            else_body: else_body.clone(),
            // A shortened chain mis-attaches these against the operand-count
            // check the backends make before rendering them inline.
            cond_comments: Vec::new(),
        }),
        // `Known(true)` is unreachable while only `!=` is classified, and would
        // mean a guard that is ALWAYS taken -- refuse it rather than splice an
        // arm we have not analysed. `Open { changed: false }` folded nothing.
        // Both leave the statement exactly as it was.
        CondFold::Known(true) | CondFold::Open { .. } => None,
    }
}

fn is_not_success(e: &Expr, var: &str) -> bool {
    let Expr::BinOp(l, BinOp::NotEq, r) = e else {
        return false;
    };
    let is_var = |x: &Expr| matches!(x, Expr::Var(n) if n == var);
    let is_success = |x: &Expr| matches!(x, Expr::Var(n) if n == "SUCCESS");
    (is_var(l) && is_success(r)) || (is_success(l) && is_var(r))
}

/// Is this expression the `COMPATIBILITY()` builtin (or its bare-`Var` spelling)?
fn is_compat_read(expr: &Expr) -> bool {
    match expr {
        Expr::Var(n) => n == "COMPATIBILITY",
        Expr::FuncCall(n, args) => n == "COMPATIBILITY" && args.is_empty(),
        _ => false,
    }
}

/// The truth value of `COMPATIBILITY() == <name>` under the Default pin.
fn compat_variant_matches(expr: &Expr) -> Option<bool> {
    match expr {
        Expr::Var(n) if n == "DEFAULT" => Some(true),
        Expr::Var(n) if n == "METASTOCK" => Some(false),
        _ => None,
    }
}

/// Leaf classifier for the compatibility fold: `COMPATIBILITY() == <variant>`
/// (either operand order, `==` or `!=`) is constant under the Default pin.
fn compat_leaf(expr: &Expr) -> Option<bool> {
    let Expr::BinOp(lhs, op @ (BinOp::Eq | BinOp::NotEq), rhs) = expr else {
        return None;
    };
    let matched = if is_compat_read(lhs) {
        compat_variant_matches(rhs)
    } else if is_compat_read(rhs) {
        compat_variant_matches(lhs)
    } else {
        None
    };
    matched.map(|eq| if matches!(op, BinOp::Eq) { eq } else { !eq })
}

/// Fold a condition against the managed backends' pinned-to-Default
/// compatibility. See the module docs; the fold hangs off each backend's
/// `if_stmt` hook, which every render path funnels through.
pub(crate) fn fold_compat_cond(expr: &Expr) -> CondFold {
    fold_cond(expr, &compat_leaf)
}

#[cfg(test)]
mod cross_call_guard_tests {
    use super::*;

    /// `retCode = ma(<n args>)` — admitted by the stub predicate below.
    fn call(n: usize) -> Statement {
        Statement::Assign {
            target: Expr::Var("retCode".into()),
            value: Expr::FuncCall("ma".into(), vec![Expr::Var("x".into()); n]),
            compound: false,
        }
    }
    fn ne_success(v: &str) -> Expr {
        Expr::BinOp(
            Box::new(Expr::Var(v.into())),
            BinOp::NotEq,
            Box::new(Expr::Var("SUCCESS".into())),
        )
    }
    fn count_is_zero() -> Expr {
        Expr::BinOp(
            Box::new(Expr::Var("outNBElement".into())),
            BinOp::Eq,
            Box::new(Expr::IntLiteral(0)),
        )
    }
    fn guard(cond: Expr) -> Statement {
        Statement::If {
            condition: cond,
            then_body: vec![Statement::Return { value: Some(Expr::Var("retCode".into())) }],
            else_body: Vec::new(),
            cond_comments: Vec::new(),
        }
    }
    fn run(body: &[Statement]) -> Vec<Statement> {
        drop_answered_cross_call_guards(body, &|f: &str, a: &[Expr]| f == "ma" && a.len() >= 6)
    }
    fn is_gone(s: &Statement) -> bool {
        matches!(s, Statement::Block { body } if body.is_empty())
    }

    #[test]
    fn a_bare_guard_is_dropped() {
        let out = run(&[call(8), guard(ne_success("retCode"))]);
        assert_eq!(out.len(), 2, "the pass must preserve the statement count");
        assert!(is_gone(&out[1]));
    }

    /// The half that is not about the code survives, and the statement stays an
    /// `If`. This is the property no text scan over the rendered corpus can see.
    #[test]
    fn a_live_disjunct_survives_alone() {
        let cond = Expr::BinOp(
            Box::new(ne_success("retCode")),
            BinOp::Or,
            Box::new(count_is_zero()),
        );
        let out = run(&[call(8), guard(cond)]);
        match &out[1] {
            Statement::If { condition, .. } => assert_eq!(*condition, count_is_zero()),
            other => panic!("the guard was dropped whole, not shortened: {other:?}"),
        }
    }

    /// `macdext.c`'s shape: statements that do not mention the variable are
    /// skipped, so the guard is still found.
    #[test]
    fn an_intervening_free_does_not_stop_the_scan() {
        let free = Statement::Expr(Expr::FuncCall("free".into(), vec![Expr::Var("buf".into())]));
        let out = run(&[call(8), free.clone(), free, guard(ne_success("retCode"))]);
        assert!(is_gone(&out[3]));
    }

    #[test]
    fn a_statement_that_reads_the_variable_stops_the_scan() {
        let read = Statement::Assign {
            target: Expr::Var("saved".into()),
            value: Expr::Var("retCode".into()),
            compound: false,
        };
        let out = run(&[call(8), read, guard(ne_success("retCode"))]);
        assert!(!is_gone(&out[2]), "a guard reachable with another value was folded");
    }

    /// `MA`'s dispatch shape: the assignment ends its switch arm, so the guard
    /// after the switch is reachable with any arm's code.
    #[test]
    fn control_flow_between_the_two_stops_the_scan() {
        let out = run(&[call(8), Statement::Break, guard(ne_success("retCode"))]);
        assert!(!is_gone(&out[2]));
    }

    #[test]
    fn a_guard_on_a_different_variable_is_untouched() {
        let out = run(&[call(8), guard(ne_success("subRc"))]);
        assert!(!is_gone(&out[1]));
    }

    /// A shape the backend's own renderer declines falls through to the plain
    /// call, where the guard is genuinely live.
    #[test]
    fn a_declined_call_shape_keeps_its_guard() {
        let out = run(&[call(3), guard(ne_success("retCode"))]);
        assert!(!is_gone(&out[1]));
    }

    /// Only `!=` is classified, so an always-taken guard is refused rather than
    /// spliced -- `Known(true)` stays unreachable by construction.
    #[test]
    fn an_equality_test_is_refused() {
        let cond = Expr::BinOp(
            Box::new(Expr::Var("retCode".into())),
            BinOp::Eq,
            Box::new(Expr::Var("SUCCESS".into())),
        );
        let out = run(&[call(8), guard(cond)]);
        assert!(!is_gone(&out[1]));
    }

    #[test]
    fn an_else_arm_is_spliced_not_discarded() {
        let els = Statement::Expr(Expr::FuncCall("g".into(), vec![]));
        let g = Statement::If {
            condition: ne_success("retCode"),
            then_body: vec![Statement::Break],
            else_body: vec![els.clone()],
            cond_comments: Vec::new(),
        };
        let out = run(&[call(8), g]);
        match &out[1] {
            Statement::Block { body } => {
                assert_eq!(body.len(), 1, "the else arm went with the branch");
                assert!(matches!(&body[0], Statement::Expr(Expr::FuncCall(n, _)) if n == "g"));
            }
            other => panic!("the else arm went with the branch: {other:?}"),
        }
        let _ = els;
    }
}
