//! The stride-0 frame of a split opener: the one loop that stores an output
//! runs its bars but the last without deciding them, then decides the last
//! once.
//!
//! A statement of that loop is a decision when all it assigns is the current
//! output element and the store cursor; every other statement is carried and
//! runs on every bar. The rewrite needs no output read anywhere in the body,
//! a loop counter stepped by one exactly once per pass, and, on every path of
//! a pass, a store before the cursor first moves: slot 0 then ends holding
//! exactly what the strided replay leaves there. Anything else answers `None`,
//! and the opener keeps its single strided body.

use crate::helper_registry::HelperRegistry;
use crate::ir::{BinOp, Expr, FuncDef, Statement};
use crate::streaming::{expr_effect, rewrite_stmts, walk_expr, walk_stmt_exprs, StreamModel};

/// An opener whose body is offered to the split.
pub(crate) fn is_framed(func: &FuncDef, model: &StreamModel) -> bool {
    func.is_candlestick() && model.identity.is_none()
}

/// A call every backend may let a decision make.
pub(crate) fn is_pure_call(n: &str, helpers: &HelperRegistry) -> bool {
    super::eager_select::is_pseudo_call(n) || super::builtins::MathFn::from_name(n).is_some() || helpers.get(n).is_some()
}

/// `body` for the stride-0 frame, or `None` when it does not split.
///
/// `body` stores each output at its plain index; the result stores at slot 0.
/// `pure` vouches for a call a decision may make.
pub(crate) fn sink_body(body: &[Statement], outputs: &[String], pure: &dyn Fn(&str) -> bool) -> Option<Vec<Statement>> {
    let is_out = |n: &str| outputs.iter().any(|o| o == n);
    let Split { at, counter, bound, pass, cursor, moves, .. } = split(body, &is_out, pure)?;
    let mut history = Vec::new();
    for (k, s) in pass.iter().enumerate() {
        match moves[k] {
            Some(0) => {}
            Some(n) => history.push(step(&cursor, n)),
            None if matches!(s, Statement::Comment(_)) && next_decides(pass, &moves, k) => {}
            None => history.push(s.clone()),
        }
    }
    let mut out = body[..at].to_vec();
    out.push(Statement::While {
        condition: Expr::BinOp(Box::new(Expr::Var(counter.to_string())), BinOp::Less, Box::new(bound.clone())),
        body: history,
    });
    for (k, s) in pass.iter().enumerate() {
        if moves[k].is_some() {
            out.extend(into_slot_zero(std::slice::from_ref(s), &cursor, &is_out));
        } else {
            out.push(s.clone());
        }
    }
    out.extend_from_slice(&body[at + 1..]);
    Some(out)
}

/// The storing loop of a body that splits, and how each statement of its pass
/// moves the store cursor: `Some(steps)` for a decision, `None` for a carried
/// statement.
struct Split<'a> {
    at: usize,
    counter: &'a str,
    bound: &'a Expr,
    pass: &'a [Statement],
    cursor: String,
    moves: Vec<Option<usize>>,
}

fn split<'a>(body: &'a [Statement], is_out: &dyn Fn(&str) -> bool, pure: &dyn Fn(&str) -> bool) -> Option<Split<'a>> {
    let storing: Vec<usize> = (0..body.len()).filter(|&i| stores_output(&body[i], is_out)).collect();
    let [at] = storing[..] else { return None };
    let Statement::DoWhile { condition, body: pass } = &body[at] else { return None };
    let Expr::BinOp(counter, BinOp::LessEq, bound) = condition else { return None };
    let Expr::Var(counter) = counter.as_ref() else { return None };
    // A declaration in the pass would be hoisted to the frame's head once the
    // last bar's copy of it sits at the top level.
    if reads_output(body, is_out) || returns_or_leaves(pass) || pass.iter().any(|s| matches!(s, Statement::VarDecl { .. })) {
        return None;
    }
    let assigned = assigned_names(pass);
    if mentions(bound, &|n| assigned.iter().any(|a| a == n)) {
        return None;
    }
    let steps = pass.iter().filter(|s| is_unit_step(s, counter)).count();
    if steps != 1 || assigned.iter().filter(|n| *n == counter).count() != 1 {
        return None;
    }
    let cursor = store_cursor(pass, is_out)?;

    let decide = |s: &Statement| stores_output(s, is_out) || stmt_mentions(s, &|n| n == cursor);
    let clean = |e: &Expr| !expr_effect(e, pure) && !mentions(e, &|n| is_out(n) || n == cursor);
    let mut path = Path { moved: 0, stored: false };
    let mut moves = Vec::with_capacity(pass.len());
    for s in pass {
        let before = path.moved;
        if decide(s) {
            scan(std::slice::from_ref(s), &cursor, is_out, &clean, &mut path)?;
            moves.push(Some(path.moved - before));
        } else {
            moves.push(None);
        }
    }
    if !path.stored {
        return None;
    }
    Some(Split { at, counter, bound, pass, cursor, moves })
}

#[derive(Clone, Copy)]
struct Path {
    /// Cursor steps taken so far in this pass.
    moved: usize,
    /// Every way through so far has stored.
    stored: bool,
}

/// One decision's statements, walking `path` through them.
fn scan(
    stmts: &[Statement],
    cursor: &str,
    is_out: &dyn Fn(&str) -> bool,
    clean: &dyn Fn(&Expr) -> bool,
    path: &mut Path,
) -> Option<()> {
    for s in stmts {
        match s {
            Statement::Comment(_) => {}
            Statement::Assign { target: Expr::ArrayAccess(n, idx), value, compound: false } if is_out(n) => {
                let moves = match idx.as_ref() {
                    Expr::Var(v) if v == cursor => false,
                    Expr::PostIncrement(v) if matches!(v.as_ref(), Expr::Var(v) if v == cursor) => true,
                    _ => return None,
                };
                if path.moved != 0 || !clean(value) {
                    return None;
                }
                path.stored = true;
                path.moved += usize::from(moves);
            }
            s if is_unit_step(s, cursor) => path.moved += 1,
            Statement::If { condition, then_body, else_body, .. } => {
                if !clean(condition) {
                    return None;
                }
                let (mut a, mut b) = (*path, *path);
                scan(then_body, cursor, is_out, clean, &mut a)?;
                scan(else_body, cursor, is_out, clean, &mut b)?;
                if a.moved != b.moved {
                    return None;
                }
                *path = Path { moved: a.moved, stored: a.stored && b.stored };
            }
            _ => return None,
        }
    }
    Some(())
}

/// The statement after the comment run at `k` is a decision.
fn next_decides(pass: &[Statement], moves: &[Option<usize>], k: usize) -> bool {
    (k + 1..pass.len()).find(|&j| !matches!(pass[j], Statement::Comment(_))).is_some_and(|j| moves[j].is_some())
}

/// A decision's stores moved to slot 0, each post-increment of the cursor
/// kept as a step right after its store.
fn into_slot_zero(stmts: &[Statement], cursor: &str, is_out: &dyn Fn(&str) -> bool) -> Vec<Statement> {
    let mut out = Vec::new();
    for s in stmts {
        match s {
            Statement::Assign { target: Expr::ArrayAccess(n, idx), value, compound: false } if is_out(n) => {
                out.push(Statement::Assign {
                    target: Expr::ArrayAccess(n.clone(), Box::new(Expr::IntLiteral(0))),
                    value: value.clone(),
                    compound: false,
                });
                if matches!(idx.as_ref(), Expr::PostIncrement(_)) {
                    out.push(step(cursor, 1));
                }
            }
            Statement::If { condition, then_body, else_body, cond_comments } => out.push(Statement::If {
                condition: condition.clone(),
                then_body: into_slot_zero(then_body, cursor, is_out),
                else_body: into_slot_zero(else_body, cursor, is_out),
                cond_comments: cond_comments.clone(),
            }),
            other => out.push(other.clone()),
        }
    }
    out
}

fn step(v: &str, n: usize) -> Statement {
    let n = i64::try_from(n).expect("a pass steps its cursor a few times");
    Statement::Assign {
        target: Expr::Var(v.to_string()),
        value: Expr::BinOp(Box::new(Expr::Var(v.to_string())), BinOp::Add, Box::new(Expr::IntLiteral(n))),
        compound: true,
    }
}

/// `v += 1`, `v = v + 1`, `v++` or `++v`.
fn is_unit_step(s: &Statement, v: &str) -> bool {
    let var = |e: &Expr| matches!(e, Expr::Var(n) if n == v);
    match s {
        Statement::Assign { target, value: Expr::BinOp(l, BinOp::Add, r), .. } => {
            var(target) && var(l) && matches!(r.as_ref(), Expr::IntLiteral(1))
        }
        Statement::Expr(Expr::PostIncrement(e) | Expr::PreIncrement(e)) => var(e),
        _ => false,
    }
}

/// The one variable every output store is indexed by, bare or post-incremented.
fn store_cursor(pass: &[Statement], is_out: &dyn Fn(&str) -> bool) -> Option<String> {
    let mut cursors: Vec<Option<String>> = Vec::new();
    each_stmt(pass, &mut |s| {
        if let Statement::Assign { target: Expr::ArrayAccess(n, idx), .. } = s {
            if is_out(n) {
                cursors.push(match idx.as_ref() {
                    Expr::Var(v) => Some(v.clone()),
                    Expr::PostIncrement(v) => match v.as_ref() {
                        Expr::Var(v) => Some(v.clone()),
                        _ => None,
                    },
                    _ => None,
                });
            }
        }
    });
    let first = cursors.first()?.clone()?;
    cursors.iter().all(|c| c.as_deref() == Some(first.as_str())).then_some(first)
}

fn stores_output(s: &Statement, is_out: &dyn Fn(&str) -> bool) -> bool {
    let mut found = false;
    each_stmt(std::slice::from_ref(s), &mut |s| {
        found |= matches!(s, Statement::Assign { target: Expr::ArrayAccess(n, _), .. } if is_out(n));
    });
    found
}

/// Any mention of an output but as the target of a plain store.
fn reads_output(body: &[Statement], is_out: &dyn Fn(&str) -> bool) -> bool {
    let masked = rewrite_stmts(body, &|e| e, &|s| {
        Some(match s {
            Statement::Assign { target: Expr::ArrayAccess(n, idx), value, compound: false } if is_out(&n) => {
                Statement::Assign { target: Expr::ArrayAccess(String::new(), idx), value, compound: false }
            }
            other => other,
        })
    });
    masked.iter().any(|s| stmt_mentions(s, is_out))
}

/// A `return` anywhere, or a `break` / `continue` that would leave the pass.
fn returns_or_leaves(pass: &[Statement]) -> bool {
    fn walk(stmts: &[Statement], in_loop: bool) -> bool {
        stmts.iter().any(|s| match s {
            Statement::Return { .. } => true,
            Statement::Break | Statement::Continue => !in_loop,
            Statement::While { body, .. }
            | Statement::DoWhile { body, .. }
            | Statement::For { body, .. }
            | Statement::ForC { body, .. } => walk(body, true),
            Statement::If { then_body, else_body, .. } => walk(then_body, in_loop) || walk(else_body, in_loop),
            Statement::Block { body } => walk(body, in_loop),
            // A `break` in a case leaves the switch, not the pass.
            Statement::Switch { cases, default, .. } => {
                cases.iter().any(|(_, b)| walk(b, true)) || walk(default, true)
            }
            _ => false,
        })
    }
    walk(pass, false)
}

/// Every variable assigned in `stmts`, once per assignment. A variable whose
/// address is taken counts as assigned there.
fn assigned_names(stmts: &[Statement]) -> Vec<String> {
    let mut out = Vec::new();
    each_stmt(stmts, &mut |s| {
        if let Statement::Assign { target: Expr::Var(n) | Expr::PointerDeref(n), .. } = s {
            out.push(n.clone());
        }
        walk_stmt_exprs_shallow(s, &mut |e| {
            walk_expr(e, &mut |x| {
                if let Expr::PostIncrement(v)
                | Expr::PreIncrement(v)
                | Expr::PostDecrement(v)
                | Expr::PreDecrement(v)
                | Expr::AddressOf(v) = x
                {
                    if let Expr::Var(n) = v.as_ref() {
                        out.push(n.clone());
                    }
                }
            });
        });
    });
    out
}

fn mentions(e: &Expr, hit: &dyn Fn(&str) -> bool) -> bool {
    let mut found = false;
    walk_expr(e, &mut |x| {
        found |= matches!(x, Expr::Var(n) | Expr::ArrayAccess(n, _) | Expr::PointerDeref(n) if hit(n));
    });
    found
}

fn stmt_mentions(s: &Statement, hit: &dyn Fn(&str) -> bool) -> bool {
    let mut found = false;
    walk_stmt_exprs(s, &mut |e| found |= mentions(e, hit));
    found
}

/// Every statement of `stmts`, nested ones included.
fn each_stmt(stmts: &[Statement], f: &mut dyn FnMut(&Statement)) {
    for s in stmts {
        f(s);
        match s {
            Statement::While { body, .. }
            | Statement::DoWhile { body, .. }
            | Statement::For { body, .. }
            | Statement::Block { body } => each_stmt(body, f),
            Statement::ForC { init, update, body, .. } => {
                each_stmt(std::slice::from_ref(init), f);
                each_stmt(std::slice::from_ref(update), f);
                each_stmt(body, f);
            }
            Statement::If { then_body, else_body, .. } => {
                each_stmt(then_body, f);
                each_stmt(else_body, f);
            }
            Statement::Switch { cases, default, .. } => {
                for (_, b) in cases {
                    each_stmt(b, f);
                }
                each_stmt(default, f);
            }
            _ => {}
        }
    }
}

/// The expressions `s` holds itself, not those of its nested statements.
fn walk_stmt_exprs_shallow(s: &Statement, f: &mut dyn FnMut(&Expr)) {
    match s {
        Statement::VarDecl { init: Some(e), .. }
        | Statement::Return { value: Some(e) }
        | Statement::Expr(e)
        | Statement::While { condition: e, .. }
        | Statement::DoWhile { condition: e, .. }
        | Statement::If { condition: e, .. }
        | Statement::ForC { condition: e, .. }
        | Statement::For { count: e, .. }
        | Statement::Switch { expr: e, .. } => f(e),
        Statement::Assign { target, value, .. } => {
            f(target);
            f(value);
        }
        _ => {}
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    fn var(n: &str) -> Expr {
        Expr::Var(n.to_string())
    }

    /// `do { <pass> } while( i <= endIdx );`, the pass storing `out[outIdx++]`
    /// and stepping `i` before or after the store.
    fn opener(step_first: bool) -> Vec<Statement> {
        let store = Statement::Assign {
            target: Expr::ArrayAccess("out".to_string(), Box::new(Expr::PostIncrement(Box::new(var("outIdx"))))),
            value: Expr::IntLiteral(1),
            compound: false,
        };
        let pass = if step_first { vec![step("i", 1), store] } else { vec![store, step("i", 1)] };
        vec![Statement::DoWhile {
            condition: Expr::BinOp(Box::new(var("i")), BinOp::LessEq, Box::new(var("endIdx"))),
            body: pass,
        }]
    }

    /// A counter or bound written through its address or a pointer keeps the
    /// single body.
    #[test]
    fn a_write_through_an_address_or_a_pointer_does_not_split() {
        let outs = ["out".to_string()];
        let pure = |_: &str| true;
        let by_address = Statement::Expr(Expr::FuncCall("advance".to_string(), vec![Expr::AddressOf(Box::new(var("i")))]));
        let through_pointer =
            Statement::Assign { target: Expr::PointerDeref("endIdx".to_string()), value: Expr::IntLiteral(3), compound: false };
        for write in [by_address, through_pointer] {
            let mut body = opener(false);
            let Some(Statement::DoWhile { body: pass, .. }) = body.first_mut() else { unreachable!() };
            pass.insert(0, write.clone());
            assert!(sink_body(&body, &outs, &pure).is_none(), "sink_body splits around {write:?}");
        }
    }
}
