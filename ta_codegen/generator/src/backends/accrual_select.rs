//! A coin-flip accumulation in C, Rust, Java and C#: `if( a > b ) acc += x;
//! else if( a < b ) acc -= y;` becomes `acc = acc + ADDEND(a > b, x) -
//! SUBTRAHEND(a < b, y)`, where `ADDEND` is `c ? x : -0.0` and `SUBTRAHEND` is
//! `c ? y : +0.0`, each rendered so the compiler keeps it a select. A not-taken
//! arm adds a negative zero or subtracts a positive one, which leaves every
//! quiet value's bits unchanged; a taken arm runs the original operation on the
//! original operands.
//!
//! Which sites qualify is [`super::go_select::accrual`] and its
//! `accrual_free`, narrowed to a condition comparing two values the bar
//! computes, never one against zero: such a sign test is no coin flip often
//! enough to repay the select. Only in the outermost loops that write an
//! output; a stream step inherits the rewrite from the loop it is derived
//! from, and a peek undoes it ([`branched`]).
//!
//! The rewrite is one statement for one, so it keeps every body's length.

use std::collections::BTreeSet;

use crate::ir::{BinOp, Expr, FuncDef, ParamType, Statement, VarType};

use super::builtins::{strip_unlikely, MathFn};
use super::go_select::{accrual, addr, data_driven, loop_body, scan_loop, stores_output};

/// `(condition, addend)`: the addend where the condition holds, else `-0.0`.
pub(crate) const ADDEND: &str = "ta_addend_if";
/// `(condition, subtrahend)`: the subtrahend where the condition holds, else `+0.0`.
pub(crate) const SUBTRAHEND: &str = "ta_subtrahend_if";

pub(crate) fn is_accrual_pick(name: &str) -> bool {
    name == ADDEND || name == SUBTRAHEND
}

/// `body` with its qualifying accumulations rewritten, or `None` when it has none.
pub(crate) fn rewrite(func: &FuncDef, body: &[Statement]) -> Option<Vec<Statement>> {
    let mut reals = BTreeSet::new();
    declared_reals(body, &mut reals);
    let inputs: BTreeSet<String> = func.inputs.iter().filter(|i| i.param_type == ParamType::Real).map(|i| i.name.clone()).collect();
    let types = Types { reals, inputs };
    let mut out = body.to_vec();
    let changed = outermost(body, &mut out, &types);
    changed.then_some(out)
}

struct Types {
    reals: BTreeSet<String>,
    inputs: BTreeSet<String>,
}

impl Types {
    /// Real at every leaf, so no integer division or conversion hides inside.
    fn real(&self, e: &Expr) -> bool {
        match e {
            Expr::Literal(_) => true,
            Expr::Var(n) => self.reals.contains(n),
            Expr::ArrayAccess(a, _) => self.inputs.contains(a),
            Expr::Cast(VarType::Real, x) | Expr::Neg(x) => self.real(x),
            Expr::BinOp(l, BinOp::Add | BinOp::Sub | BinOp::Mul | BinOp::Div, r) => self.real(l) && self.real(r),
            Expr::FuncCall(n, args) => matches!(MathFn::from_name(n), Some(MathFn::Abs | MathFn::Sqrt)) && args.iter().all(|a| self.real(a)),
            _ => false,
        }
    }

    /// Two real values the bar computes, compared.
    fn coin_flip(&self, cond: &Expr) -> bool {
        let Expr::BinOp(l, _, r) = strip_unlikely(cond).0 else { return false };
        let computed = |e: &Expr| {
            let mut read = false;
            crate::streaming::walk_expr(e, &mut |x| read |= matches!(x, Expr::Var(_) | Expr::ArrayAccess(..)));
            read
        };
        data_driven(cond) && self.real(l) && self.real(r) && computed(l) && computed(r)
    }
}

fn declared_reals(block: &[Statement], out: &mut BTreeSet<String>) {
    for st in block {
        if let Statement::VarDecl { var_type: VarType::Real, name, .. } = st {
            out.insert(name.clone());
        }
        for b in children(st) {
            declared_reals(b, out);
        }
    }
}

fn children(s: &Statement) -> Vec<&[Statement]> {
    match s {
        Statement::If { then_body, else_body, .. } => vec![then_body, else_body],
        Statement::Block { body } => vec![body],
        Statement::Switch { cases, default, .. } => cases.iter().map(|(_, b)| b.as_slice()).chain(std::iter::once(default.as_slice())).collect(),
        other => loop_body(other).into_iter().collect(),
    }
}

fn children_mut(s: &mut Statement) -> Vec<&mut Vec<Statement>> {
    match s {
        Statement::If { then_body, else_body, .. } => vec![then_body, else_body],
        Statement::Block { body } | Statement::While { body, .. } | Statement::DoWhile { body, .. } | Statement::For { body, .. } | Statement::ForC { body, .. } => {
            vec![body]
        }
        Statement::Switch { cases, default, .. } => cases.iter_mut().map(|(_, b)| b).chain(std::iter::once(default)).collect(),
        _ => Vec::new(),
    }
}

/// Finds the outermost loops of `src`, rewriting the matching statements of `dst`.
fn outermost(src: &[Statement], dst: &mut [Statement], types: &Types) -> bool {
    let mut changed = false;
    for (s, d) in src.iter().zip(dst.iter_mut()) {
        if let Some(body) = loop_body(s) {
            if stores_output(body) {
                let scan = scan_loop(body);
                let Some(dbody) = children_mut(d).into_iter().next() else { continue };
                changed |= in_pass(body, dbody, types, &scan);
            }
            continue;
        }
        for (sb, db) in children(s).into_iter().zip(children_mut(d)) {
            changed |= outermost(sb, db, types);
        }
    }
    changed
}

/// One pass of a bar loop, not entering a deeper loop.
fn in_pass(src: &[Statement], dst: &mut [Statement], types: &Types, scan: &super::go_select::FrameScan) -> bool {
    let mut changed = false;
    for (s, d) in src.iter().zip(dst.iter_mut()) {
        if loop_body(s).is_some() {
            continue;
        }
        if let Statement::If { condition, then_body, else_body, .. } = s {
            if let Some(site) = site(condition, then_body, else_body, types, scan.around.get(&addr(s))) {
                *d = site;
                changed = true;
                continue;
            }
        }
        for (sb, db) in children(s).into_iter().zip(children_mut(d)) {
            changed |= in_pass(sb, db, types, scan);
        }
    }
    changed
}

fn site(cond: &Expr, then_body: &[Statement], else_body: &[Statement], types: &Types, around: Option<&super::go_select::Around>) -> Option<Statement> {
    let (acc, arms) = accrual(cond, then_body, else_body)?;
    let fits = types.reals.contains(acc)
        && arms.iter().all(|(c, _, v)| types.coin_flip(c) && types.real(v))
        && around.is_some_and(|a| a.accrual_free(acc, &arms));
    if !fits {
        return None;
    }
    let mut sum = Expr::Var(acc.to_string());
    for (c, op, v) in arms {
        let pick = if op == BinOp::Add { ADDEND } else { SUBTRAHEND };
        sum = Expr::BinOp(Box::new(sum), op, Box::new(Expr::FuncCall(pick.to_string(), vec![c.clone(), v.clone()])));
    }
    Some(Statement::Assign { target: Expr::Var(acc.to_string()), value: sum, compound: false })
}

/// The accumulation `s` selects, branched again, or `s` unchanged.
///
/// A peek keeps the branch: it carries no sum to the next bar, so the select
/// buys no shorter chain and only adds the arms' work.
pub(crate) fn branched(s: Statement) -> Statement {
    let Statement::Assign { target, value, compound: false } = &s else { return s };
    let mut arms = Vec::new();
    let mut e = value;
    while let Expr::BinOp(l, op, r) = e {
        let Expr::FuncCall(pick, args) = r.as_ref() else { break };
        let [c, v] = args.as_slice() else { break };
        if !is_accrual_pick(pick) || (*op == BinOp::Add) != (pick == ADDEND) {
            break;
        }
        arms.push((c.clone(), op.clone(), v.clone()));
        e = l;
    }
    if arms.is_empty() || e != target {
        return s;
    }
    let mut chain: Vec<Statement> = Vec::new();
    for (c, op, v) in arms {
        let step = Statement::Assign { target: target.clone(), value: Expr::BinOp(Box::new(target.clone()), op, Box::new(v)), compound: true };
        chain = vec![Statement::If { condition: c, then_body: vec![step], else_body: chain, cond_comments: Vec::new() }];
    }
    chain.pop().unwrap_or(s)
}
