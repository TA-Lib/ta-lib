//! Which `if`s guard a division by testing its divisor.

use crate::ir::{BinOp, Expr, Statement};

/// True for an `if` with no else whose condition only tests variables
/// against zero (`d > 0`, `d != 0`, conjoined with `&&`) and whose call-free
/// body divides by one of them. Such a guard almost always passes, so a
/// backend that would otherwise if-convert it keeps it a predicted branch. A
/// body with a call is left alone: it is a branch already, and the hint only
/// moves code around it.
#[must_use]
pub fn is_divisor_guard(condition: &Expr, then_body: &[Statement], else_body: &[Statement]) -> bool {
    if !else_body.is_empty() {
        return false;
    }
    let mut vars = Vec::new();
    if !zero_tests(condition, &mut vars) {
        return false;
    }
    let divides = |e: &Expr| {
        matches!(e, Expr::BinOp(_, BinOp::Div, d) if matches!(strip_casts(d), Expr::Var(v) if vars.contains(&v.as_str())))
    };
    let calls = |e: &Expr| matches!(e, Expr::FuncCall(..));
    then_body.iter().any(|s| stmt_any(s, &divides)) && !then_body.iter().any(|s| stmt_any(s, &calls))
}

fn is_zero(e: &Expr) -> bool {
    match e {
        Expr::Literal(v) => *v == 0.0,
        Expr::IntLiteral(v) => *v == 0,
        Expr::Cast(_, inner) => is_zero(inner),
        _ => false,
    }
}

fn strip_casts(e: &Expr) -> &Expr {
    match e {
        Expr::Cast(_, inner) => strip_casts(inner),
        _ => e,
    }
}

fn zero_tests<'a>(cond: &'a Expr, vars: &mut Vec<&'a str>) -> bool {
    let Expr::BinOp(l, op, r) = cond else { return false };
    match op {
        BinOp::And => zero_tests(l, vars) && zero_tests(r, vars),
        BinOp::Greater | BinOp::NotEq | BinOp::Less => {
            let var = match (strip_casts(l), strip_casts(r), op) {
                (Expr::Var(v), z, BinOp::Greater | BinOp::NotEq) if is_zero(z) => v,
                (z, Expr::Var(v), BinOp::Less | BinOp::NotEq) if is_zero(z) => v,
                _ => return false,
            };
            vars.push(var);
            true
        }
        _ => false,
    }
}

fn expr_any(e: &Expr, pred: &dyn Fn(&Expr) -> bool) -> bool {
    pred(e)
        || match e {
            Expr::BinOp(l, _, r) => expr_any(l, pred) || expr_any(r, pred),
            Expr::ArrayAccess(_, i)
            | Expr::Cast(_, i)
            | Expr::Not(i)
            | Expr::Neg(i)
            | Expr::BitwiseNot(i)
            | Expr::AddressOf(i)
            | Expr::PostIncrement(i)
            | Expr::PostDecrement(i)
            | Expr::PreIncrement(i)
            | Expr::PreDecrement(i) => expr_any(i, pred),
            Expr::FuncCall(_, args) => args.iter().any(|a| expr_any(a, pred)),
            Expr::Ternary(c, a, b) => [c, a, b].iter().any(|x| expr_any(x, pred)),
            Expr::Literal(_) | Expr::IntLiteral(_) | Expr::Var(_) | Expr::PointerDeref(_) => false,
        }
}

/// Anything other than a flat statement counts as a match, so a loop or a
/// switch in the body keeps the guard out of the rule.
fn stmt_any(s: &Statement, pred: &dyn Fn(&Expr) -> bool) -> bool {
    let any = |b: &[Statement]| b.iter().any(|s| stmt_any(s, pred));
    match s {
        Statement::Assign { target, value, .. } => expr_any(target, pred) || expr_any(value, pred),
        Statement::VarDecl { init: Some(e), .. } | Statement::Expr(e) => expr_any(e, pred),
        Statement::VarDecl { init: None, .. } | Statement::Comment(_) => false,
        Statement::If { condition, then_body, else_body, .. } => {
            expr_any(condition, pred) || any(then_body) || any(else_body)
        }
        Statement::Block { body } => any(body),
        _ => true,
    }
}
