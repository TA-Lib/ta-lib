//! Which real picks the Go backend renders through `selF`: in a stream frame,
//! and in a batch or open body's bar loops.
//!
//! A select waits for its condition and both values every bar; a predicted
//! branch waits for neither. So a pick is a select only when its condition is
//! a coin flip on the bar's data: two computed values compared, or a computed
//! value against zero. A guard stays a branch, because it is almost never
//! taken: an equality, a call, a compound, a test against a non-zero literal or
//! a parameter, one that keeps a divisor or a root's argument in its domain
//! with a literal fallback, and one an enclosing condition already decided but
//! for the tie. A pick stored to a handle field is a select only when it is
//! the field's one pick on its path: the field carries the value to the next
//! bar, and selects chained on it put every bar on all of their latencies.
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
//! A real accumulation under the same kind of condition, `if c1 { acc += a }
//! else if c2 { acc -= b }` with each arm that one statement, selects its
//! addend instead: `acc + selF(c1, a, -0.0) - selF(c2, b, +0.0)`. No select is
//! on the accumulator's chain, which is one add or subtract per arm, so the
//! handle-field limit above does not apply. A not-taken arm leaves every
//! quiet value's bits as they were, but quiets a signaling NaN. Two arms
//! must never both be true. The addend is computed on every bar: it may not
//! call, except for an intrinsic Go inlines, nor read an array but an input
//! at an index the bar already read, nor hold a product among its terms
//! (the accumulation may fuse it), and a condition keeping its divisor or
//! root in domain is a guard. An accumulator its own statement list also
//! assigns holds this bar's value, not a sum carried in, and keeps its branch.

use std::collections::{BTreeSet, HashMap};

use crate::ir::{BinOp, Expr, Statement};
use crate::streaming::{expr_var_names, walk_expr, walk_stmt_exprs};

use super::builtins::{strip_unlikely, MathFn};

fn bare(e: &Expr) -> &Expr {
    match e {
        Expr::Cast(_, x) => bare(x),
        other => other,
    }
}

fn is_zero(e: &Expr) -> bool {
    match bare(e) {
        Expr::Literal(v) => *v == 0.0,
        Expr::IntLiteral(0) => true,
        Expr::Neg(x) => is_zero(x),
        _ => false,
    }
}

fn is_literal(e: &Expr) -> bool {
    match bare(e) {
        Expr::Literal(_) | Expr::IntLiteral(_) => true,
        Expr::Neg(x) => is_literal(x),
        _ => false,
    }
}

/// A parameter, or a candle setting's snapshot: fixed for the handle's life.
fn is_parameter(name: &str) -> bool {
    let n = name.strip_prefix("sp.").unwrap_or(name);
    n.starts_with("optIn") || ["_rangeType", "_avgPeriod", "_factor"].iter().any(|s| n.ends_with(s))
}

fn is_constant(e: &Expr) -> bool {
    match bare(e) {
        Expr::Literal(_) | Expr::IntLiteral(_) => true,
        Expr::Var(n) => is_parameter(n),
        Expr::Neg(x) => is_constant(x),
        Expr::BinOp(l, BinOp::Add | BinOp::Sub | BinOp::Mul | BinOp::Div, r) => is_constant(l) && is_constant(r),
        _ => false,
    }
}

/// Whether the bar's data decides `cond`: two computed values compared, or
/// one against zero.
pub(super) fn data_driven(cond: &Expr) -> bool {
    let Expr::BinOp(l, op, r) = bare(strip_unlikely(cond).0) else { return false };
    if !matches!(op, BinOp::Less | BinOp::LessEq | BinOp::Greater | BinOp::GreaterEq) {
        return false;
    }
    match (is_constant(l), is_constant(r)) {
        (false, false) => true,
        (true, false) => is_zero(l),
        (false, true) => is_zero(r),
        (true, true) => false,
    }
}

/// Names under a divisor or a `sqrt`/`log` argument in `e`.
fn domain_names(e: &Expr, out: &mut BTreeSet<String>) {
    walk_expr(e, &mut |x| match x {
        Expr::BinOp(_, BinOp::Div, d) => expr_var_names(d, out),
        Expr::FuncCall(n, args) if matches!(MathFn::from_name(n), Some(MathFn::Sqrt | MathFn::Log | MathFn::Log10)) => {
            for a in args {
                expr_var_names(a, out);
            }
        }
        _ => {}
    });
}

fn definition(s: &Statement) -> Option<(&str, &Expr)> {
    match s {
        Statement::Assign { target: Expr::Var(v), value, compound: false } => Some((v, value)),
        Statement::VarDecl { name, init: Some(e), .. } => Some((name, e)),
        _ => None,
    }
}

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

/// What a pick's statement sees around it in its frame.
#[derive(Default, Debug)]
pub(super) struct Around {
    /// Each name's latest earlier definition.
    defs: HashMap<String, Expr>,
    /// Names a later statement divides by or takes a root or log of, and the
    /// names defining those one step back.
    later: BTreeSet<String>,
    /// `(value, literal)` compared by an enclosing condition.
    tested: Vec<(Expr, Expr)>,
    /// Names the other statements of the pick's own list define.
    siblings: BTreeSet<String>,
}

impl Around {
    /// A domain guard: one picked value is a literal fallback, and the
    /// condition tests a divisor or a root's argument that defines the other,
    /// or the picked value goes on to be one.
    pub(super) fn guard(&self, cond: &Expr, t: &Expr, f: &Expr, target: Option<&str>) -> bool {
        if !is_literal(t) && !is_literal(f) {
            return false;
        }
        let mut ops = BTreeSet::new();
        expr_var_names(cond, &mut ops);
        let mut picked: Vec<&str> = [t, f]
            .into_iter()
            .filter_map(|e| match bare(e) {
                Expr::Var(n) => Some(n.as_str()),
                _ => None,
            })
            .collect();
        picked.extend(target);
        picked.iter().any(|p| {
            self.defs.get(*p).is_some_and(|e| {
                let mut d = BTreeSet::new();
                domain_names(e, &mut d);
                !d.is_disjoint(&ops)
            })
        })
            || target.is_some_and(|v| self.later.contains(v) && picked.iter().any(|p| ops.contains(*p)))
    }

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

    /// The condition compares a value with the literal an enclosing condition
    /// compared it with: on this path it decides only the tie.
    pub(super) fn retest(&self, cond: &Expr) -> bool {
        let mut mine = Vec::new();
        against_literal(cond, &mut mine);
        mine.iter().any(|m| self.tested.contains(m))
    }

    /// Whether the accumulation `arms` on `acc` selects its addends.
    pub(super) fn accrual_free(&self, acc: &str, arms: &[Accrual]) -> bool {
        let names = |e: &Expr| {
            let mut n = BTreeSet::new();
            expr_var_names(e, &mut n);
            n
        };
        let apart = match arms {
            [_] => true,
            [(a, ..), (b, ..)] => exclusive(a, b),
            _ => false,
        };
        apart
            && !self.siblings.contains(acc)
            && arms.iter().all(|(c, _, v)| {
                let mut domain = BTreeSet::new();
                domain_names(v, &mut domain);
                let ops = names(c);
                data_driven(c)
                    && !ops.contains(acc)
                    && !names(v).contains(acc)
                    && self.addend(v)
                    && ops.is_disjoint(&domain)
                    && !self.guard(c, v, &Expr::Literal(0.0), Some(acc))
                    && !self.retest(c)
            })
    }

    fn addend(&self, e: &Expr) -> bool {
        match e {
            Expr::Literal(_) | Expr::IntLiteral(_) | Expr::Var(_) => true,
            Expr::Cast(_, x) | Expr::Neg(x) => self.addend(x),
            Expr::BinOp(l, BinOp::Add | BinOp::Sub | BinOp::Mul | BinOp::Div, r) => self.addend(l) && self.addend(r),
            Expr::FuncCall(n, args) => matches!(MathFn::from_name(n), Some(MathFn::Abs | MathFn::Sqrt)) && args.iter().all(|a| self.addend(a)),
            Expr::ArrayAccess(a, at) => is_input(a) && self.defs.values().any(|d| reads_input_at(d, at)),
            _ => false,
        }
    }
}

/// One arm of an accumulation: its condition, `Add` or `Sub`, and the addend.
pub(super) type Accrual<'s> = (&'s Expr, BinOp, &'s Expr);

/// `if c1 { acc op= a } [else if c2 { acc op= b }]`, each arm that one
/// statement on one variable: the variable and the arms in order.
pub(super) fn accrual<'s>(cond: &'s Expr, then_body: &'s [Statement], else_body: &'s [Statement]) -> Option<(&'s str, Vec<Accrual<'s>>)> {
    let code = |body: &'s [Statement]| -> Vec<&'s Statement> { body.iter().filter(|s| !matches!(s, Statement::Comment(_))).collect() };
    let one = |body: &'s [Statement]| -> Option<(&'s str, BinOp, &'s Expr)> {
        match code(body).as_slice() {
            [Statement::Assign { target: Expr::Var(v), value: Expr::BinOp(l, op @ (BinOp::Add | BinOp::Sub), r), compound: true }] if matches!(l.as_ref(), Expr::Var(n) if n == v) && !product_term(r) => {
                Some((v, op.clone(), r))
            }
            _ => None,
        }
    };
    let (acc, op, a) = one(then_body)?;
    let mut arms = vec![(strip_unlikely(cond).0, op, a)];
    match code(else_body).as_slice() {
        [] => {}
        [Statement::If { condition, then_body, else_body, .. }] if else_body.is_empty() => match one(then_body)? {
            (w, op, b) if w == acc => arms.push((strip_unlikely(condition).0, op, b)),
            _ => return None,
        },
        _ => return None,
    }
    Some((acc, arms))
}

/// A product, or a sum with one among its terms.
fn product_term(e: &Expr) -> bool {
    match e {
        Expr::BinOp(_, BinOp::Mul, _) => true,
        Expr::BinOp(l, BinOp::Add | BinOp::Sub, r) => product_term(l) || product_term(r),
        Expr::Cast(_, x) | Expr::Neg(x) => product_term(x),
        _ => false,
    }
}

/// Two comparisons of the same operands in opposite directions, at most one
/// of them admitting a tie.
fn exclusive(a: &Expr, b: &Expr) -> bool {
    fn side(e: &Expr) -> Option<(&Expr, &Expr, bool, bool)> {
        let Expr::BinOp(l, op, r) = e else { return None };
        let (up, strict) = match op {
            BinOp::Greater => (true, true),
            BinOp::GreaterEq => (true, false),
            BinOp::Less => (false, true),
            BinOp::LessEq => (false, false),
            _ => return None,
        };
        Some((l, r, up, strict))
    }
    let (Some((l1, r1, up1, strict1)), Some((l2, r2, up2, strict2))) = (side(a), side(b)) else { return false };
    let up2 = if l1 == l2 && r1 == r2 {
        up2
    } else if l1 == r2 && r1 == l2 {
        !up2
    } else {
        return false;
    };
    up1 != up2 && (strict1 || strict2)
}

fn is_input(name: &str) -> bool {
    name.strip_prefix("in").is_some_and(|r| r.starts_with(|c: char| c.is_ascii_uppercase() || c.is_ascii_digit()))
}

/// Whether `e` reads an input at `at`, an index free of steps and calls.
fn reads_input_at(e: &Expr, at: &Expr) -> bool {
    let mut pure = true;
    walk_expr(at, &mut |x| pure &= !matches!(x, Expr::PostIncrement(_) | Expr::PostDecrement(_) | Expr::PreIncrement(_) | Expr::PreDecrement(_) | Expr::FuncCall(..)));
    let mut found = false;
    walk_expr(e, &mut |x| found |= matches!(x, Expr::ArrayAccess(a, i) if is_input(a) && i.as_ref() == at));
    pure && found
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

pub(super) fn loop_body(s: &Statement) -> Option<&[Statement]> {
    match s {
        Statement::While { body, .. } | Statement::DoWhile { body, .. } | Statement::For { body, .. } | Statement::ForC { body, .. } => Some(body),
        _ => None,
    }
}

pub(super) fn addr(s: &Statement) -> usize {
    std::ptr::from_ref(s) as usize
}

pub(super) fn scan(body: &[Statement]) -> FrameScan {
    let mut s = FrameScan { chains: chains(body), around: HashMap::new(), live_in: BTreeSet::new(), all_defs: HashMap::new() };
    visit(body, &mut Vec::new(), &mut Vec::new(), &mut s);
    s
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

fn nested(s: &Statement) -> Vec<&[Statement]> {
    match s {
        Statement::If { then_body, else_body, .. } => vec![then_body, else_body],
        Statement::Block { body } => vec![body],
        Statement::Switch { cases, default, .. } => cases.iter().map(|(_, b)| b.as_slice()).chain(std::iter::once(default.as_slice())).collect(),
        _ => Vec::new(),
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

fn visit<'b>(block: &'b [Statement], path: &mut Vec<(&'b [Statement], usize)>, tests: &mut Vec<&'b Expr>, s: &mut FrameScan) {
    for (i, st) in block.iter().enumerate() {
        path.push((block, i));
        if has_pick(st) {
            s.around.insert(addr(st), around(path, tests));
        }
        let cond = match st {
            Statement::If { condition, .. } => Some(condition),
            _ => None,
        };
        tests.extend(cond);
        for b in nested(st) {
            visit(b, path, tests, s);
        }
        if cond.is_some() {
            tests.pop();
        }
        path.pop();
    }
}

fn around(path: &[(&[Statement], usize)], tests: &[&Expr]) -> Around {
    let mut a = Around::default();
    for (blk, idx) in path.iter().rev() {
        for st in blk[..*idx].iter().rev() {
            if let Some((v, e)) = definition(st) {
                a.defs.entry(v.to_string()).or_insert_with(|| e.clone());
            }
        }
    }
    let later: Vec<&Statement> = path.iter().flat_map(|(blk, idx)| &blk[idx + 1..]).collect();
    for st in &later {
        walk_stmt_exprs(st, &mut |e| domain_names(e, &mut a.later));
    }
    let mut step = BTreeSet::new();
    for (v, e) in later.iter().filter_map(|st| definition(st)) {
        if a.later.contains(v) {
            expr_var_names(e, &mut step);
        }
    }
    a.later.extend(step);
    if let Some((blk, idx)) = path.last() {
        a.siblings = blk.iter().enumerate().filter(|(j, _)| j != idx).filter_map(|(_, st)| definition(st)).map(|(v, _)| v.to_string()).collect();
    }
    for c in tests {
        against_literal(c, &mut a.tested);
    }
    a
}

/// The `(value, literal)` pairs `c`'s relational conjuncts compare.
fn against_literal(c: &Expr, out: &mut Vec<(Expr, Expr)>) {
    match bare(strip_unlikely(c).0) {
        Expr::BinOp(l, BinOp::And | BinOp::Or, r) => {
            against_literal(l, out);
            against_literal(r, out);
        }
        Expr::BinOp(l, BinOp::Less | BinOp::LessEq | BinOp::Greater | BinOp::GreaterEq, r) => {
            let (l, r) = (bare(l), bare(r));
            match (is_literal(l), is_literal(r)) {
                (false, true) => out.push((l.clone(), r.clone())),
                (true, false) => out.push((r.clone(), l.clone())),
                _ => {}
            }
        }
        _ => {}
    }
}

/// Whether `body` writes an output array.
pub(super) fn stores_output(body: &[Statement]) -> bool {
    body.iter().any(|st| {
        matches!(st, Statement::Assign { target: Expr::ArrayAccess(n, _), .. } if n.starts_with("out"))
            || nested(st).into_iter().chain(loop_body(st)).any(stores_output)
    })
}

#[cfg(test)]
mod tests {
    use super::accrual;
    use crate::backends::lockstep::{bin, var};
    use crate::ir::{BinOp, Expr, Statement};

    /// `if a > b { acc += e }`.
    fn arm(e: Expr) -> (Expr, Vec<Statement>) {
        let add = Statement::Assign { target: var("acc"), value: bin(var("acc"), BinOp::Add, e), compound: true };
        (bin(var("a"), BinOp::Greater, var("b")), vec![add])
    }

    #[test]
    fn an_addend_with_a_product_on_top_keeps_its_branch() {
        let (c, then_body) = arm(var("x"));
        assert!(accrual(&c, &then_body, &[]).is_some());
        let (c, then_body) = arm(bin(var("x"), BinOp::Div, var("y")));
        assert!(accrual(&c, &then_body, &[]).is_some(), "a quotient does not fuse");
        let (c, then_body) = arm(bin(var("x"), BinOp::Mul, var("y")));
        assert!(accrual(&c, &then_body, &[]).is_none(), "acc += x*y");
        let (c, then_body) = arm(bin(var("z"), BinOp::Sub, bin(var("x"), BinOp::Mul, var("y"))));
        assert!(accrual(&c, &then_body, &[]).is_none(), "acc += z - x*y");
    }
}
