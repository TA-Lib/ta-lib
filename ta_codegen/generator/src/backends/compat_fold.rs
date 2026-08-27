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
//! [`ir_cleanup`](super::ir_cleanup) is the engine's third consumer, for the
//! guard on a cross-call the backend has already answered.
//!
//! The engine handles both operand orders and propagates through `&&` / `||` /
//! `!`, so a compound test such as `unstablePeriod == 0 && COMPATIBILITY() ==
//! METASTOCK` collapses whole.
//!
//! An absorbed operand is DISCARDED, and C evaluates a left operand
//! unconditionally — short-circuiting protects only the right. So the engine
//! refuses to absorb when the surviving-but-dropped side could do something
//! (`expr_is_pure`), rather than resting on the leaf classifiers happening to
//! sit beside pure reads. No condition in the corpus writes, so this costs
//! nothing today and cannot be traded away by a future leaf.

use crate::ir::{BinOp, Expr};

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
                // `x && false` / `x || true` — the absorbing element wins, and
                // `x` is DISCARDED. C evaluates a left operand unconditionally
                // (short-circuiting protects only the right), so refuse when it
                // could do something. Costs nothing today: no condition in the
                // corpus writes.
                (CondFold::Known(k), _) if k != is_and && !expr_is_pure(rhs) => {
                    CondFold::Open { expr: expr.clone(), changed: false }
                }
                (_, CondFold::Known(k)) if k != is_and && !expr_is_pure(lhs) => {
                    CondFold::Open { expr: expr.clone(), changed: false }
                }
                (CondFold::Known(k), _) | (_, CondFold::Known(k)) if k != is_and => {
                    CondFold::Known(k)
                }
                (CondFold::Known(_), CondFold::Known(_)) => CondFold::Known(is_and),
                // `x && true` / `x || false` — the identity element drops out.
                // Only the folded LEAF disappears here, and a leaf is a
                // comparison against a backend constant, so nothing is lost.
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



/// Does evaluating this expression change nothing? A call counts as impure:
/// this module cannot know whether it is.
fn expr_is_pure(e: &Expr) -> bool {
    let mut pure = true;
    crate::streaming::walk_expr(e, &mut |x| {
        if matches!(
            x,
            Expr::PostIncrement(_)
                | Expr::PreIncrement(_)
                | Expr::PostDecrement(_)
                | Expr::PreDecrement(_)
                | Expr::FuncCall(..)
        ) {
            pure = false;
        }
    });
    pure
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
mod absorb_purity_tests {
    use super::*;

    fn known_false(e: &Expr) -> Option<bool> {
        matches!(e, Expr::Var(n) if n == "FALSE").then_some(false)
    }

    /// `x && false` discards `x`, and C evaluates `x` regardless — so an `x`
    /// that does something must block the fold.
    #[test]
    fn an_impure_sibling_blocks_the_absorb() {
        let cond = Expr::BinOp(
            Box::new(Expr::PostIncrement(Box::new(Expr::Var("i".into())))),
            BinOp::And,
            Box::new(Expr::Var("FALSE".into())),
        );
        assert!(matches!(
            fold_cond(&cond, &known_false),
            CondFold::Open { changed: false, .. }
        ));
    }

    #[test]
    fn a_call_counts_as_impure() {
        let cond = Expr::BinOp(
            Box::new(Expr::FuncCall("f".into(), vec![])),
            BinOp::And,
            Box::new(Expr::Var("FALSE".into())),
        );
        assert!(matches!(
            fold_cond(&cond, &known_false),
            CondFold::Open { changed: false, .. }
        ));
    }

    /// The corpus shape: a pure read beside the leaf still folds whole.
    #[test]
    fn a_pure_sibling_still_absorbs() {
        let cond = Expr::BinOp(
            Box::new(Expr::Var("flag".into())),
            BinOp::And,
            Box::new(Expr::Var("FALSE".into())),
        );
        assert!(matches!(fold_cond(&cond, &known_false), CondFold::Known(false)));
    }
}
