//! Backend-selected IR cleanup, run between the streaming decision and
//! rendering.
//!
//! A transcribed body is C, and some of C is not relevant to every backend: a
//! `free()` has no counterpart where a `Vec` drops or a GC owns the memory, and
//! a guard on a code the backend answered at the call site can no longer be
//! taken. Rendering those as the empty string leaves the scaffolding standing —
//! `if bufferIsAllocated != 0 { }`.
//!
//! So each backend states its own sequence, explicitly, at its entry point:
//!
//! ```text
//! let body = ir_cleanup::drop_answered_cross_call_guards(body, &admits, None);
//! let body = ir_cleanup::drop_deallocation(&body);
//! let body = ir_cleanup::drop_inert_guards(&body);   // always last
//! ```
//!
//! The third argument is the code the surviving arm of a folded guard answers:
//! `None` in the batch tiers, where "success with nothing produced" is a legal
//! answer, and the insufficient-history code — in that backend's own spelling —
//! in the stream tiers, where it is not. C states no sequence at all: every
//! pass here would be wrong there.
//!
//! **Every pass is length-preserving.** A removed statement becomes an empty
//! `Statement::Block`, which renders to nothing through the shared default. The
//! streaming emitters address the body BY INDEX — their `inserts` / `replaced`
//! sets are statement positions — so a pass that shortened the list would
//! desynchronize every one of them.
//!
//! `drop_inert_guards` runs last by construction: the earlier passes are what
//! empty the bodies it then finds.

use crate::backends::builtins::StdlibFn;
use crate::backends::compat_fold::{fold_cond, CondFold};
use crate::ir::{BinOp, CircBuf, Expr, Statement};

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
    answered_return: Option<&str>,
) -> Vec<Statement> {
    let mut out: Vec<Statement> = body
        .iter()
        .map(|s| recurse(s, &|b| drop_answered_cross_call_guards(b, admits, answered_return)))
        .collect();

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
        if let Some(folded) = fold_guard(&out[j], &var, answered_return) {
            out[j] = folded;
        }
    }
    out
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

/// Does this expression name `var`, in any of the three spellings that carry a
/// name? `PointerDeref` and `ArrayAccess` hold theirs as a `String`, not as a
/// nested `Expr::Var`, so a `Var`-only test would walk straight past `*retCode`
/// and `buf[i]` -- and the forward scan's whole soundness rests on this being
/// complete, not on today's targets happening to be plain `Var`s.
fn expr_mentions(e: &Expr, var: &str) -> bool {
    let mut hit = false;
    crate::streaming::walk_expr(e, &mut |x| {
        let named = match x {
            Expr::Var(n) | Expr::PointerDeref(n) | Expr::ArrayAccess(n, _) => n == var,
            _ => false,
        };
        if named {
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
fn fold_guard(s: &Statement, var: &str, answered_return: Option<&str>) -> Option<Statement> {
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
            then_body: answered_return
                .map_or_else(|| then_body.clone(), |code| substitute_return(then_body, var, code)),
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

/// Replace `return <var>` with `return <code>` in a surviving guard arm.
///
/// The arm is only reached with `var` holding `SUCCESS` -- the `!= SUCCESS` half
/// is what was just folded away -- so what the C body returns there is "success
/// with nothing produced". A batch call may answer that (an empty `OutRange`);
/// an OPENER may not, and the rule it falls under is S7: a history that cannot
/// produce a value is `TA_INSUFFICIENT_HISTORY`. Without the substitution Rust
/// answered `Err(RetCode::Success)` and Java/C# minted a handle over an empty
/// range (issue #271 item 4).
///
/// So the batch tiers pass `None` here and the stream openers name their code.
/// One level only: a `return` nested inside a loop or a branch of the arm is not
/// the guard's answer, and nothing in the corpus has one.
fn substitute_return(then_body: &[Statement], var: &str, code: &str) -> Vec<Statement> {
    then_body
        .iter()
        .map(|s| match s {
            Statement::Return { value: Some(Expr::Var(v)) } if returns_code_var(v, var) => {
                Statement::Return { value: Some(Expr::Var(code.to_string())) }
            }
            other => other.clone(),
        })
        .collect()
}

/// Is this returned name the guard's code variable?
///
/// Two spellings reach here, because a backend may map its return codes BEFORE
/// running this sequence: the bare variable, and that variable already wrapped
/// in the backend's own return form (Rust's `Err(retCode)`). So the code each
/// backend passes is likewise ITS spelling of the answer, not a C one.
fn returns_code_var(v: &str, var: &str) -> bool {
    v == var
        || v.strip_suffix(')')
            .and_then(|t| t.split_once('('))
            .is_some_and(|(_, inner)| inner == var)
}

fn is_not_success(e: &Expr, var: &str) -> bool {
    let Expr::BinOp(l, BinOp::NotEq, r) = e else {
        return false;
    };
    let is_var = |x: &Expr| matches!(x, Expr::Var(n) if n == var);
    let is_success = |x: &Expr| matches!(x, Expr::Var(n) if n == "SUCCESS");
    (is_var(l) && is_success(r)) || (is_success(l) && is_var(r))
}
/// Drop deallocation, for a backend that has none.
///
/// Two spellings reach the IR and both render to the empty string in Rust, Java
/// and C# today: `free()`, and `CircBuf::Destroy` (whose own doc says "C frees
/// each heap buffer iff allocated; other backends no-op"). Taking both here
/// rather than at the renderer is what lets [`drop_inert_guards`] be a
/// structural rule instead of one that has to know what `free` is.
///
/// `TA_Free` is deliberately absent: it never enters the IR. It is text the C
/// emitters write for their own circbuf and scratch scaffolding, and C runs no
/// sequence from this module.
///
/// `CircBuf::Prolog` also renders empty in those backends, and is deliberately
/// NOT taken — it is a declaration, not a deallocation.
pub(crate) fn drop_deallocation(body: &[Statement]) -> Vec<Statement> {
    body.iter()
        .map(|s| {
            if is_deallocation(s) {
                Statement::Block { body: Vec::new() }
            } else {
                recurse(s, &drop_deallocation)
            }
        })
        .collect()
}

fn is_deallocation(s: &Statement) -> bool {
    match s {
        Statement::Expr(Expr::FuncCall(f, _)) => {
            matches!(StdlibFn::from_name(f), Some(StdlibFn::Free))
        }
        Statement::CircBuf(CircBuf::Destroy { .. }) => true,
        _ => false,
    }
}

/// Drop an `if` that does nothing: an empty body, and a condition that only
/// reads.
///
/// Structural, and it has to stay that way — the rule is "this statement has no
/// effect", not "the body contained a `free`". The earlier passes are what make
/// a body empty; this one never asks why.
///
/// Two preconditions, each of which a naive version gets wrong:
///
/// - **The condition must not write.** `if( i++ > 0 ) { }` does something. A
///   pass keyed only on the body would silently lose the increment.
/// - **BOTH arms must be inert.** An `else` here is not the one
///   `drop_answered_cross_call_guards` splices: there the condition had been
///   PROVED false, so the else was the only reachable arm. Here the condition is
///   arbitrary and live, so splicing would run the else unconditionally --
///   turning `if( c ) { } else { compute(); }` into an unconditional
///   `compute()`, in the ported backends only, while C keeps the branch.
///   Discarding it instead (what the older `contains_alloc_err_return`
///   early-return does) is the same defect mirrored. So: refuse.
///
/// Negating the condition to keep an `else`-only guard is the obvious next step
/// and is deliberately absent -- no such shape exists in the corpus, and an
/// unexercised transform is one nothing would catch going wrong.
pub(crate) fn drop_inert_guards(body: &[Statement]) -> Vec<Statement> {
    body.iter()
        .map(|s| {
            let s = recurse(s, &drop_inert_guards);
            match &s {
                Statement::If { condition, then_body, else_body, .. }
                    if renders_nothing(then_body)
                        && renders_nothing(else_body)
                        && !crate::streaming::expr_has_effect(condition) =>
                {
                    Statement::Block { body: Vec::new() }
                }
                _ => s,
            }
        })
        .collect()
}

/// True when this statement list produces no output: empty, or nothing but the
/// empty `Block`s the earlier passes leave behind and the comments that were
/// narrating what they removed.
///
/// Comments count as nothing on purpose. A comment inside a guard is about that
/// guard — `/* Don't need K anymore, free it if it was allocated here. */` sits
/// beside the `free` it explains — so keeping it after the guard is gone leaves
/// the output narrating code that is not there. C keeps both, having dropped
/// neither.
fn renders_nothing(body: &[Statement]) -> bool {
    body.iter().all(|s| match s {
        Statement::Block { body } => renders_nothing(body),
        Statement::Comment(_) => true,
        _ => false,
    })
}

/// Apply `pass` to every nested body, leaving this statement's own shape alone.
fn recurse(s: &Statement, pass: &dyn Fn(&[Statement]) -> Vec<Statement>) -> Statement {
    match s {
        Statement::While { condition, body } => {
            Statement::While { condition: condition.clone(), body: pass(body) }
        }
        Statement::DoWhile { condition, body } => {
            Statement::DoWhile { condition: condition.clone(), body: pass(body) }
        }
        Statement::For { var, count, body } => {
            Statement::For { var: var.clone(), count: count.clone(), body: pass(body) }
        }
        Statement::ForC { init, condition, update, body } => Statement::ForC {
            init: Box::new(recurse(init, pass)),
            condition: condition.clone(),
            update: Box::new(recurse(update, pass)),
            body: pass(body),
        },
        Statement::If { condition, then_body, else_body, cond_comments } => Statement::If {
            condition: condition.clone(),
            then_body: pass(then_body),
            else_body: pass(else_body),
            cond_comments: cond_comments.clone(),
        },
        Statement::Switch { expr, cases, default } => Statement::Switch {
            expr: expr.clone(),
            cases: cases.iter().map(|(v, b)| (v.clone(), pass(b))).collect(),
            default: pass(default),
        },
        Statement::Block { body } => Statement::Block { body: pass(body) },
        other => other.clone(),
    }
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
        drop_answered_cross_call_guards(body, &ADMITS_MA, None)
    }
    /// The stream tiers' call: the surviving arm answers the opener's code.
    fn run_open(body: &[Statement]) -> Vec<Statement> {
        drop_answered_cross_call_guards(body, &ADMITS_MA, Some("InsufficientHistory"))
    }
    const ADMITS_MA: fn(&str, &[Expr]) -> bool = |f, a| f == "ma" && a.len() >= 6;
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

    /// The surviving arm is reached only with the code holding `SUCCESS`, so what
    /// it returns is "success with nothing produced". An OPENER may not answer
    /// that (issue #271 item 4): the stream tiers name their own code, the batch
    /// tiers pass `None` and keep the variable.
    #[test]
    fn the_surviving_arm_answers_the_openers_code() {
        let cond = Expr::BinOp(
            Box::new(ne_success("retCode")),
            BinOp::Or,
            Box::new(count_is_zero()),
        );
        let body = [call(8), guard(cond)];
        let batch = run(&body);
        let open = run_open(&body);
        let returned = |out: &[Statement]| match &out[1] {
            Statement::If { then_body, .. } => match &then_body[0] {
                Statement::Return { value: Some(Expr::Var(v)) } => v.clone(),
                other => panic!("not a bare return: {other:?}"),
            },
            other => panic!("the guard was dropped whole: {other:?}"),
        };
        assert_eq!(returned(&batch), "retCode");
        assert_eq!(returned(&open), "InsufficientHistory");
    }

    /// A backend that maps its return codes BEFORE running the sequence hands
    /// the pass its own spelling of the same return — Rust's `Err(retCode)`.
    #[test]
    fn an_already_wrapped_return_is_still_the_codes() {
        let cond = Expr::BinOp(
            Box::new(ne_success("retCode")),
            BinOp::Or,
            Box::new(count_is_zero()),
        );
        let wrapped = Statement::If {
            condition: cond,
            then_body: vec![Statement::Return { value: Some(Expr::Var("Err(retCode)".into())) }],
            else_body: Vec::new(),
            cond_comments: Vec::new(),
        };
        let out = drop_answered_cross_call_guards(
            &[call(8), wrapped],
            &ADMITS_MA,
            Some("Err(RetCode::InsufficientHistory)"),
        );
        match &out[1] {
            Statement::If { then_body, .. } => match &then_body[0] {
                Statement::Return { value: Some(Expr::Var(v)) } => {
                    assert_eq!(v, "Err(RetCode::InsufficientHistory)");
                }
                other => panic!("not a bare return: {other:?}"),
            },
            other => panic!("the guard was dropped whole: {other:?}"),
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

#[cfg(test)]
mod cleanup_pipeline_tests {
    use super::*;
    use crate::ir::{CircBufLayout, VarType};

    fn free_of(v: &str) -> Statement {
        Statement::Expr(Expr::FuncCall("free".into(), vec![Expr::Var(v.into())]))
    }
    fn destroy(id: &str) -> Statement {
        Statement::CircBuf(CircBuf::Destroy {
            id: id.into(),
            layout: CircBufLayout::Plain(VarType::Real),
        })
    }
    fn guard(cond: Expr, body: Vec<Statement>) -> Statement {
        Statement::If { condition: cond, then_body: body, else_body: Vec::new(), cond_comments: Vec::new() }
    }
    fn flag(v: &str) -> Expr {
        Expr::Var(v.into())
    }
    fn gone(s: &Statement) -> bool {
        matches!(s, Statement::Block { body } if body.is_empty())
    }

    #[test]
    fn both_deallocator_spellings_are_taken() {
        let out = drop_deallocation(&[free_of("buf"), destroy("ring")]);
        assert_eq!(out.len(), 2, "the pass must preserve the statement count");
        assert!(gone(&out[0]) && gone(&out[1]));
    }

    /// A declaration is not a deallocation, however empty it renders.
    #[test]
    fn a_circbuf_prolog_is_left_alone() {
        let prolog = Statement::CircBuf(CircBuf::Prolog {
            id: "ring".into(),
            layout: CircBufLayout::Plain(VarType::Real),
            static_size: 4,
        });
        let out = drop_deallocation(&[prolog]);
        assert!(!gone(&out[0]));
    }

    /// The whole point of the sequence: the earlier pass empties the body, and
    /// the later one needs no idea why.
    #[test]
    fn the_sequence_removes_a_free_guard_end_to_end() {
        let body = vec![guard(flag("bufferIsAllocated"), vec![free_of("tempBuffer")])];
        let out = drop_inert_guards(&drop_deallocation(&body));
        assert!(gone(&out[0]));
    }

    #[test]
    fn a_guard_is_kept_while_its_body_still_does_something() {
        let keep = Statement::Assign {
            target: Expr::Var("x".into()),
            value: Expr::IntLiteral(1),
            compound: false,
        };
        let body = vec![guard(flag("f"), vec![free_of("b"), keep])];
        let out = drop_inert_guards(&drop_deallocation(&body));
        assert!(!gone(&out[0]));
    }

    /// A comment inside the guard is about the guard, so it goes with it.
    #[test]
    fn a_comment_only_body_is_inert() {
        let body = vec![guard(flag("f"), vec![Statement::Comment(vec!["why".into()])])];
        assert!(gone(&drop_inert_guards(&body)[0]));
    }

    #[test]
    fn a_condition_that_writes_is_never_dropped() {
        for cond in [
            Expr::PostIncrement(Box::new(Expr::Var("i".into()))),
            Expr::PreDecrement(Box::new(Expr::Var("i".into()))),
            Expr::FuncCall("sideEffect".into(), vec![]),
        ] {
            let out = drop_inert_guards(&[guard(cond, Vec::new())]);
            assert!(!gone(&out[0]), "an effectful condition was dropped");
        }
    }

    /// A live else arm means the guard still decides something, so the guard
    /// stays. Splicing the else would run it unconditionally; discarding it
    /// would lose it. Both are wrong, and only in the ported backends -- C would
    /// keep the branch, so the two would silently disagree.
    #[test]
    fn a_guard_with_a_live_else_arm_is_refused() {
        let els = Statement::Assign {
            target: Expr::Var("x".into()),
            value: Expr::IntLiteral(1),
            compound: false,
        };
        let g = Statement::If {
            condition: flag("f"),
            then_body: vec![free_of("b")],
            else_body: vec![els],
            cond_comments: Vec::new(),
        };
        let out = drop_inert_guards(&drop_deallocation(&[g]));
        assert!(
            matches!(&out[0], Statement::If { .. }),
            "a guard whose else arm still does something was folded"
        );
    }

    /// Both arms inert: now it really does nothing.
    #[test]
    fn a_guard_with_two_inert_arms_is_dropped() {
        let g = Statement::If {
            condition: flag("f"),
            then_body: vec![free_of("b")],
            else_body: vec![free_of("c")],
            cond_comments: Vec::new(),
        };
        assert!(gone(&drop_inert_guards(&drop_deallocation(&[g]))[0]));
    }

    /// Nested bodies are reached, and the count at every level is preserved.
    #[test]
    fn the_passes_recurse_and_preserve_length() {
        let inner = guard(flag("f"), vec![free_of("b")]);
        let outer = Statement::While {
            condition: flag("cond"),
            body: vec![inner, free_of("c")],
        };
        let out = drop_inert_guards(&drop_deallocation(&[outer]));
        match &out[0] {
            Statement::While { body, .. } => {
                assert_eq!(body.len(), 2);
                assert!(gone(&body[0]) && gone(&body[1]));
            }
            other => panic!("unexpected: {other:?}"),
        }
    }
}
