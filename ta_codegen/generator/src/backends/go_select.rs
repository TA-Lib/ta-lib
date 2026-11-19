//! Which real picks the Go backend renders through `selF`, by the rule in
//! [`super::select_rule`]: in a stream frame, and in a batch or open body's bar
//! loops. A pick stored to a handle field is a select only when it is the
//! field's one pick on its path: the field carries the value to the next bar,
//! and selects chained on it put every bar on all of their latencies.
//!
//! A batch or open body applies the same rule to a pick in an outermost loop,
//! whose iterations are the bars; a deeper loop scans a window and keeps its
//! branches. There a value the next iteration reads stands for the handle
//! field, and is a select only in a loop that writes an output: a pick on the
//! carried value of a loop that writes none is a scan's or a seed's reduction.
//! A true range's raises (a local defined as a difference, raised to the
//! absolute value of a difference) and a true high or low (this bar's high or
//! low against the previous close) stay branches there: the own-bar term
//! almost always wins. So do a true range's raises in a stream Update, which
//! sees consecutive bars, but not in a Peek, whose bar need not follow the
//! handle's last one.
//!
//! An accumulation the rule admits selects its addend instead: `acc + selF(c1,
//! a, -0.0) - selF(c2, b, +0.0)`. No select is on the accumulator's chain,
//! which is one add or subtract per arm, so the handle-field limit above does
//! not apply. A not-taken arm leaves every quiet value's bits as they were,
//! but quiets a signaling NaN.

use std::collections::{BTreeSet, HashMap};

use crate::ir::{BinOp, Expr, Statement};
use crate::streaming::{expr_var_names, walk_expr, walk_stmt_exprs};

use super::builtins::{strip_unlikely, MathFn};
use super::select_rule::{arounds, bare, definition, nested, Around};
pub(super) use super::select_rule::{accrual, addr, data_driven, loop_body, stores_output};

/// `if c { v = a } [else { v = b }]`: `v`, `a`, and `b` when there is an else.
fn if_pick<'s>(then_body: &'s [Statement], else_body: &'s [Statement]) -> Option<(&'s str, &'s Expr, Option<&'s Expr>)> {
    let one = |body: &'s [Statement]| -> Option<(&'s str, &'s Expr)> {
        let mut code = body.iter().filter(|s| !matches!(s, Statement::Comment(_)));
        match (code.next(), code.next()) {
            (Some(Statement::Assign { target: Expr::Var(v), value, compound: false }), None) => Some((v, value)),
            _ => None,
        }
    };
    let (v, a) = one(then_body)?;
    if else_body.is_empty() {
        return Some((v, a, None));
    }
    match one(else_body)? {
        (w, b) if w == v => Some((v, a, Some(b))),
        _ => None,
    }
}

fn is_pick_value(e: &Expr) -> bool {
    match bare(e) {
        Expr::Ternary(..) => true,
        Expr::FuncCall(n, args) => args.len() == 2 && matches!(MathFn::from_name(n), Some(MathFn::Max | MathFn::Min)),
        _ => false,
    }
}

fn has_pick(s: &Statement) -> bool {
    let mut found = matches!(s, Statement::If { .. });
    walk_stmt_exprs(s, &mut |e| walk_expr(e, &mut |x| found |= is_pick_value(x)));
    found
}

impl Around {
    /// A true range's raise: `target`, defined as a difference, takes the
    /// challenger it is compared with when that is defined as the absolute
    /// value of a difference.
    pub(super) fn true_range(&self, cond: &Expr, t: &Expr, f: &Expr, target: Option<&str>) -> bool {
        let Some(range) = target else { return false };
        let Expr::BinOp(lhs, _, rhs) = bare(strip_unlikely(cond).0) else { return false };
        let (lhs, rhs) = (bare(lhs), bare(rhs));
        if !((lhs == bare(t) && rhs == bare(f)) || (lhs == bare(f) && rhs == bare(t))) {
            return false;
        }
        let is_range = |e: &Expr| matches!(bare(e), Expr::Var(name) if name == range);
        let challenger = match (is_range(t), is_range(f)) {
            (false, true) => t,
            (true, false) => f,
            _ => return false,
        };
        let is_diff = |e: Option<&Expr>| matches!(e.map(bare), Some(Expr::BinOp(_, BinOp::Sub, _)));
        let gap = match bare(challenger) {
            Expr::Var(name) => self.defs.get(name).map(bare),
            _ => None,
        };
        let is_abs_diff = matches!(gap, Some(Expr::FuncCall(call, args)) if matches!(MathFn::from_name(call), Some(MathFn::Abs)) && is_diff(args.first()));
        is_abs_diff && is_diff(self.defs.get(range))
    }
}

/// A frame body's picks, keyed by the address of the statement holding each.
#[derive(Default, Debug)]
pub(super) struct FrameScan {
    /// Per target, the most picks one pass through the body makes on it.
    pub(super) chains: HashMap<String, usize>,
    pub(super) around: HashMap<usize, Around>,
    /// In a loop body: the names an iteration reads before it writes them,
    /// which carry the previous iteration's values.
    pub(super) live_in: BTreeSet<String>,
    /// In a loop body: every definition of each name, at any depth.
    all_defs: HashMap<String, Vec<Expr>>,
}

pub(super) fn scan(body: &[Statement]) -> FrameScan {
    FrameScan { chains: chains(body), around: arounds(body, &has_pick), live_in: BTreeSet::new(), all_defs: HashMap::new() }
}

/// [`scan`] of a loop body, with the names its iterations carry.
pub(super) fn scan_loop(body: &[Statement]) -> FrameScan {
    let mut s = scan(body);
    exposed(body, &mut BTreeSet::new(), &mut s.live_in);
    all_defs(body, &mut s.all_defs);
    s
}

fn all_defs(block: &[Statement], out: &mut HashMap<String, Vec<Expr>>) {
    for st in block {
        if let Some((v, e)) = definition(st) {
            out.entry(v.to_string()).or_default().push(e.clone());
        }
        for b in nested(st) {
            all_defs(b, out);
        }
    }
}

impl FrameScan {
    /// A true high or low: `target`, loaded from this bar's high or low, takes
    /// the previous bar's close it is compared with. That close is loaded at
    /// the bar index minus one, or carried from the previous iteration, which
    /// loads it only from this bar's close.
    pub(super) fn true_extreme(&self, around: &Around, cond: &Expr, t: &Expr, f: &Expr, target: Option<&str>) -> bool {
        let Some(extreme) = target else { return false };
        let Expr::BinOp(lhs, _, rhs) = bare(strip_unlikely(cond).0) else { return false };
        let (lhs, rhs) = (bare(lhs), bare(rhs));
        if !((lhs == bare(t) && rhs == bare(f)) || (lhs == bare(f) && rhs == bare(t))) {
            return false;
        }
        let is_extreme = |e: &Expr| matches!(bare(e), Expr::Var(name) if name == extreme);
        let close = match (is_extreme(t), is_extreme(f)) {
            (false, true) => t,
            (true, false) => f,
            _ => return false,
        };
        let Some(Expr::ArrayAccess(arr, idx)) = around.defs.get(extreme).map(bare) else { return false };
        if arr != "inHigh" && arr != "inLow" {
            return false;
        }
        let Expr::Var(close) = bare(close) else { return false };
        let at = |e: &Expr, back: bool| match bare(e) {
            Expr::ArrayAccess(arr, at_idx) if arr == "inClose" => {
                if back {
                    matches!(bare(at_idx), Expr::BinOp(base, BinOp::Sub, one) if bare(base) == bare(idx) && matches!(bare(one), Expr::IntLiteral(1)))
                } else {
                    bare(at_idx) == bare(idx)
                }
            }
            _ => false,
        };
        if around.defs.get(close).is_some_and(|e| at(e, true)) {
            return true;
        }
        let this_close = |e: &Expr| {
            at(e, false) || matches!(bare(e), Expr::Var(n) if self.all_defs.get(n).is_some_and(|d| d.iter().all(|e| at(e, false))))
        };
        self.live_in.contains(close) && self.all_defs.get(close).is_some_and(|d| !d.is_empty() && d.iter().all(this_close))
    }
}

/// The names `block` reads before writing them, a write in a branch killing
/// only inside that branch.
fn exposed(block: &[Statement], killed: &mut BTreeSet<String>, out: &mut BTreeSet<String>) {
    let read = |e: &Expr, killed: &BTreeSet<String>, out: &mut BTreeSet<String>| {
        let mut names = BTreeSet::new();
        expr_var_names(e, &mut names);
        out.extend(names.into_iter().filter(|n| !killed.contains(n)));
    };
    for st in block {
        match st {
            Statement::Assign { target, value, compound } => {
                read(value, killed, out);
                match target {
                    Expr::Var(v) if !compound => {
                        killed.insert(v.clone());
                    }
                    other => read(other, killed, out),
                }
            }
            Statement::VarDecl { name, init, .. } => {
                if let Some(e) = init {
                    read(e, killed, out);
                }
                killed.insert(name.clone());
            }
            Statement::Block { body } => exposed(body, killed, out),
            _ => {
                let mut own = Vec::new();
                match st {
                    Statement::If { condition, .. } | Statement::While { condition, .. } | Statement::DoWhile { condition, .. } => own.push(condition),
                    Statement::For { count, .. } => own.push(count),
                    Statement::Switch { expr, .. } => own.push(expr),
                    Statement::Return { value: Some(e) } | Statement::Expr(e) => own.push(e),
                    _ => {}
                }
                for e in own {
                    read(e, killed, out);
                }
                if matches!(st, Statement::ForC { .. }) {
                    walk_stmt_exprs(st, &mut |e| read(e, killed, out));
                }
                for b in nested(st).into_iter().chain(loop_body(st)) {
                    exposed(b, &mut killed.clone(), out);
                }
            }
        }
    }
}

fn chains(block: &[Statement]) -> HashMap<String, usize> {
    let mut acc: HashMap<String, usize> = HashMap::new();
    for st in block {
        if let Statement::If { then_body, else_body, .. } = st {
            if let Some((v, _, _)) = if_pick(then_body, else_body) {
                *acc.entry(v.to_string()).or_default() += 1;
                continue;
            }
        }
        if let Some((v, e)) = definition(st) {
            if is_pick_value(e) {
                *acc.entry(v.to_string()).or_default() += 1;
            }
        }
        let mut most: HashMap<String, usize> = HashMap::new();
        for b in nested(st) {
            for (k, n) in chains(b) {
                let m = most.entry(k).or_default();
                *m = (*m).max(n);
            }
        }
        for (k, n) in most {
            *acc.entry(k).or_default() += n;
        }
    }
    acc
}
