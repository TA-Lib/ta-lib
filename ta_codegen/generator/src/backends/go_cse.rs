//! Binds a real expression that two neighbouring assignments of one statement
//! list both compute, which gcc merges and Go's compiler does not always.
//!
//! A candidate must be a node the renderer lowers whole in both statements, so
//! the walk must descend exactly where the Go expression renderer does: the
//! product a fused sum takes apart into `math.FMA`'s operands is never one; a
//! whole fused sum can be, since whether it fuses depends on its subtree alone.

use std::collections::BTreeSet;

use crate::ir::{BinOp, Expr, Statement};

use super::builtins::MathFn;
use super::ctype::{CType, TypeEnv};
use super::fma::{self, FmaVarSets};

/// How many statements after the first the second may sit.
const REACH: usize = 4;

/// A binding over `list[first..=last]`.
pub(super) struct Span {
    pub(super) expr: Expr,
    pub(super) first: usize,
    pub(super) last: usize,
}

/// The bindings whose first statement is `list[at]`.
pub(super) fn spans(list: &[Statement], at: usize, env: &TypeEnv, fs: Option<&FmaVarSets>, taken: &[Expr]) -> Vec<Span> {
    let Some(first) = whole_nodes(&list[at], env, fs) else { return Vec::new() };
    if first.is_empty() {
        return Vec::new();
    }
    let mut chosen: Vec<Span> = Vec::new();
    for j in at + 1..list.len().min(at + REACH + 1) {
        if matches!(list[j], Statement::Comment(_)) {
            continue;
        }
        let Some(second) = whole_nodes(&list[j], env, fs) else { break };
        let mut common: Vec<&Expr> = first.iter().copied().filter(|e| second.contains(e)).collect();
        common.sort_by_key(|e| std::cmp::Reverse(size(e)));
        let open = |e: &Expr| {
            !taken.iter().chain(chosen.iter().map(|s| &s.expr)).any(|t| contains(t, e) || contains(e, t)) && !killed(e, &list[at..j])
        };
        if let Some(e) = common.into_iter().find(|e| open(e)) {
            chosen.push(Span { expr: e.clone(), first: at, last: j });
        }
    }
    chosen
}

/// The qualifying nodes `s` renders whole, or `None` when `s` is not a plain
/// assignment free of effects.
fn whole_nodes<'s>(s: &'s Statement, env: &TypeEnv, fs: Option<&FmaVarSets>) -> Option<Vec<&'s Expr>> {
    let Statement::Assign { target, value, compound } = s else { return None };
    if !matches!(target, Expr::Var(_) | Expr::ArrayAccess(..)) || impure(target) || impure(value) {
        return None;
    }
    let mut out = Vec::new();
    let rendered = match (target, value, compound) {
        (Expr::Var(t), Expr::BinOp(l, _, r), true) if matches!(l.as_ref(), Expr::Var(n) if n == t) => r.as_ref(),
        _ => value,
    };
    // The accumulator swap reorders two products, neither of which may be bound.
    let canonical = fma::EMIT_FMA && fma::canonicalize_accumulator_add(target, value) != *value;
    if !canonical {
        walk(rendered, env, fs, &mut out);
    }
    Some(out)
}

fn walk<'e>(e: &'e Expr, env: &TypeEnv, fs: Option<&FmaVarSets>, out: &mut Vec<&'e Expr>) {
    if qualifies(e, env) {
        out.push(e);
    }
    match e {
        Expr::BinOp(l, BinOp::And | BinOp::Or, _) => walk(l, env, fs, out),
        Expr::BinOp(left, op, right) => {
            let fused = fma::EMIT_FMA.then_some(fs).flatten().and_then(|fs| {
                fma::fuse_operands(left, op, right, &fs.view()).filter(|(x, y, _)| !super::go::exact_factor(x) && !super::go::exact_factor(y))
            });
            let operands = match fused {
                Some((x, y, addend)) => vec![x, y, addend],
                None => vec![left.as_ref(), right.as_ref()],
            };
            for o in operands {
                walk(o, env, fs, out);
            }
        }
        Expr::Ternary(c, _, _) => walk(c, env, fs, out),
        Expr::Cast(_, i) | Expr::Neg(i) | Expr::Not(i) => walk(i, env, fs, out),
        Expr::FuncCall(_, args) if math_call(e) => args.iter().for_each(|a| walk(a, env, fs, out)),
        _ => {}
    }
}

/// A real expression of plain operations with at least one real operation
/// and one read: never a bare read, a constant, or an integer value converted.
fn qualifies(e: &Expr, env: &TypeEnv) -> bool {
    let mut real_op = false;
    let mut read = false;
    ty(e, env, &mut real_op, &mut read) == Some(CType::Real) && real_op && read
}

fn ty(e: &Expr, env: &TypeEnv, real_op: &mut bool, read: &mut bool) -> Option<CType> {
    match e {
        Expr::Literal(_) => Some(CType::Real),
        Expr::IntLiteral(_) => Some(CType::Int32),
        Expr::Var(n) => {
            *read = true;
            env.lookup(n).filter(|t| !t.is_pointer())
        }
        Expr::ArrayAccess(n, i) => {
            *read = true;
            (ty(i, env, &mut false, &mut false)? == CType::Int32).then_some(())?;
            match env.lookup(n)? {
                CType::RealPtr => Some(CType::Real),
                CType::IntPtr => Some(CType::Int32),
                _ => None,
            }
        }
        Expr::BinOp(l, BinOp::Add | BinOp::Sub | BinOp::Mul | BinOp::Div, r) => {
            let (lt, rt) = (ty(l, env, real_op, read)?, ty(r, env, real_op, read)?);
            if lt == CType::Bool || rt == CType::Bool {
                return None;
            }
            let t = if lt == CType::Real || rt == CType::Real { CType::Real } else { CType::Int32 };
            *real_op |= t == CType::Real;
            Some(t)
        }
        Expr::Cast(vt, i) => {
            let to = CType::of_var_type(vt);
            let from = ty(i, env, real_op, read)?;
            (matches!(to, CType::Real | CType::Int32) && matches!(from, CType::Real | CType::Int32)).then_some(to)
        }
        Expr::Neg(i) => ty(i, env, real_op, read).filter(|t| matches!(t, CType::Real | CType::Int32)),
        Expr::FuncCall(_, args) if math_call(e) || known_range(e) => {
            for a in args {
                ty(a, env, real_op, read)?;
            }
            *real_op = true;
            Some(CType::Real)
        }
        _ => None,
    }
}

/// A candle range of a known range type, which renders as its arithmetic.
fn known_range(e: &Expr) -> bool {
    matches!(e, Expr::FuncCall(n, args) if n == "ta_candlerange" && args.len() == 5 && matches!(args[0], Expr::IntLiteral(0..=2)))
}

/// A one-argument libm call Go renders as `math.<F>(x)`.
fn math_call(e: &Expr) -> bool {
    matches!(e, Expr::FuncCall(n, args) if args.len() == 1 && MathFn::from_name(n).is_some_and(|m| !matches!(m, MathFn::Max | MathFn::Min)))
}

/// A step, an address taken, or a call other than the math and candle-range
/// forms, at any depth: such a call may write what a binding reads, and its
/// hoisted body would be rendered with the binding substituted.
fn impure(e: &Expr) -> bool {
    let mut any = false;
    crate::streaming::walk_expr(e, &mut |x| {
        any |= match x {
            Expr::FuncCall(..) => !math_call(x) && !known_range(x),
            Expr::AddressOf(_) | Expr::PostIncrement(_) | Expr::PostDecrement(_) | Expr::PreIncrement(_) | Expr::PreDecrement(_) => true,
            _ => false,
        };
    });
    any
}

/// A handle field is read and written under `sp.` and bare.
fn base(n: &str) -> &str {
    n.strip_prefix("sp.").unwrap_or(n)
}

/// Whether a statement of `span`, the first one's own store included, writes a
/// scalar `e` reads, or stores to any element while `e` reads one.
fn killed(e: &Expr, span: &[Statement]) -> bool {
    let mut vars = BTreeSet::new();
    let mut elements = false;
    crate::streaming::walk_expr(e, &mut |x| match x {
        Expr::Var(n) => {
            vars.insert(base(n).to_string());
        }
        Expr::ArrayAccess(n, _) => {
            vars.insert(base(n).to_string());
            elements = true;
        }
        _ => {}
    });
    span.iter().any(|s| match s {
        Statement::Assign { target: Expr::Var(n), .. } => vars.contains(base(n)),
        Statement::Assign { .. } => elements,
        Statement::Comment(_) => false,
        _ => true,
    })
}

fn contains(outer: &Expr, inner: &Expr) -> bool {
    let mut found = false;
    crate::streaming::walk_expr(outer, &mut |x| found |= x == inner);
    found
}

fn size(e: &Expr) -> usize {
    let mut n = 0;
    crate::streaming::walk_expr(e, &mut |_| n += 1);
    n
}
