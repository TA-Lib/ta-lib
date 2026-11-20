//! Which picks and accumulations a coin flip on the bar's data decides, for
//! the backends that render them without a branch.
//!
//! A select waits for its condition and both values every bar; a predicted
//! branch waits for neither. So a pick is a select only when its condition is
//! a coin flip on the bar's data: two computed values compared, or a computed
//! value against zero. A guard stays a branch, because it is almost never
//! taken: an equality, a call, a compound, a test against a non-zero literal or
//! a parameter, one that keeps a divisor or a root's argument in its domain
//! with a literal fallback, and one an enclosing condition already decided but
//! for the tie.
//!
//! A real accumulation under the same kind of condition, `if c1 { acc += a }
//! else if c2 { acc -= b }` with each arm that one statement, may compute its
//! arms on every bar. Two arms must never both be true. The addend is computed
//! on every bar: it may not call, except `fabs` and `sqrt`, nor read an array
//! but an input at an index the bar already read, nor hold a product among its
//! terms (the accumulation may fuse it), and a condition keeping its divisor or
//! root in domain is a guard. An accumulator its own statement list also
//! assigns holds this bar's value, not a sum carried in, and keeps its branch.

use std::collections::{BTreeSet, HashMap, HashSet};

use crate::ir::{BinOp, Expr, Statement};
use crate::streaming::{expr_var_names, walk_expr, walk_stmt_exprs};

use super::builtins::{strip_unlikely, MathFn};
use super::fma::{self, FmaVarSets};

pub(super) fn bare(e: &Expr) -> &Expr {
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

pub(super) fn definition(s: &Statement) -> Option<(&str, &Expr)> {
    match s {
        Statement::Assign { target: Expr::Var(v), value, compound: false } => Some((v, value)),
        Statement::VarDecl { name, init: Some(e), .. } => Some((name, e)),
        _ => None,
    }
}

/// What a pick's statement sees around it in its frame.
#[derive(Default, Debug)]
pub(super) struct Around {
    /// Each name's latest earlier definition.
    pub(super) defs: HashMap<String, Expr>,
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

    /// The condition compares a value with the literal an enclosing condition
    /// compared it with: on this path it decides only the tie.
    pub(super) fn retest(&self, cond: &Expr) -> bool {
        let mut mine = Vec::new();
        against_literal(cond, &mut mine);
        mine.iter().any(|m| self.tested.contains(m))
    }

    /// Whether the accumulation `arms` on `acc` may compute its arms on every
    /// bar.
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

/// A two-armed accrual whose arms test the two strict orders of the same two
/// computed values: `a < b` takes `below`, `b < a` takes `above`, and a tie or
/// a NaN leaves `acc`. A backend may compute all three values and pick one by
/// the two compares. A test against zero is left out: an oscillator's sign.
pub(super) struct OrderedStep<'s> {
    pub(super) acc: &'s str,
    pub(super) a: &'s Expr,
    pub(super) b: &'s Expr,
    pub(super) below: (BinOp, &'s Expr),
    pub(super) above: (BinOp, &'s Expr),
}

pub(super) fn ordered_step<'s>(cond: &'s Expr, then_body: &'s [Statement], else_body: &'s [Statement]) -> Option<OrderedStep<'s>> {
    let commented = |b: &[Statement]| b.iter().any(|s| matches!(s, Statement::Comment(_)));
    if commented(then_body) || commented(else_body) {
        return None;
    }
    let (acc, arms) = accrual(cond, then_body, else_body)?;
    let [(c1, op1, v1), (c2, op2, v2)] = arms.as_slice() else { return None };
    let less = |c: &'s Expr| match c {
        Expr::BinOp(l, BinOp::Less, r) => Some((l.as_ref(), r.as_ref())),
        Expr::BinOp(l, BinOp::Greater, r) => Some((r.as_ref(), l.as_ref())),
        _ => None,
    };
    let ((a, b), (b2, a2)) = (less(c1)?, less(c2)?);
    if a != a2 || b != b2 || is_constant(a) || is_constant(b) {
        return None;
    }
    Some(OrderedStep { acc, a, b, below: (op1.clone(), v1), above: (op2.clone(), v2) })
}

/// An [`ordered_step`] on a real accumulator that [`ordered_step_targets`]
/// admits, comparing reals: the form a backend may render branch-free. Its
/// operands are evaluated more than once, so they must be pure.
pub(super) fn admitted_step<'s>(cond: &'s Expr, then_body: &'s [Statement], else_body: &'s [Statement], sets: &FmaVarSets) -> Option<OrderedStep<'s>> {
    let step = ordered_step(cond, then_body, else_body)?;
    if !sets.ordered_step_targets.contains(fma::stream_base(step.acc)) {
        return None;
    }
    let fs = sets.view();
    let int = |e: &Expr| matches!(e, Expr::IntLiteral(_)) || fma::is_definitely_integer(e, &fs);
    let real = |e: &Expr| fma::expr_is_float_typed(e, Some(&fs));
    let pure = |e: &Expr| !crate::streaming::expr_effect(e, &|f| MathFn::from_name(f).is_some());
    let (a, b) = (step.a, step.b);
    if !real(&Expr::Var(step.acc.to_string())) || int(a) || int(b) || !(real(a) || real(b)) || ![a, b, step.below.1, step.above.1].into_iter().all(pure) {
        return None;
    }
    Some(step)
}

/// `(condition, addend)`: the addend where the condition holds, else `-0.0`.
pub(crate) const ADDEND: &str = "ta_addend_if";
/// `(condition, subtrahend)`: the subtrahend where the condition holds, else `+0.0`.
pub(crate) const SUBTRAHEND: &str = "ta_subtrahend_if";

pub(crate) fn is_accrual_pick(name: &str) -> bool {
    name == ADDEND || name == SUBTRAHEND
}

/// `step` as `acc = acc op1 PICK(a < b, x) op2 PICK(b < a, y)`, each `PICK` an
/// [`ADDEND`] or a [`SUBTRAHEND`] by its operator. Adding `-0.0` or subtracting
/// `+0.0` leaves the bits of every sum but a signaling NaN as they were, a `-0.0`
/// sum included, and a taken arm runs the branch's own operation on its own
/// operands.
pub(super) fn addend_form(step: &OrderedStep<'_>) -> (Expr, Expr) {
    let acc = Expr::Var(step.acc.to_string());
    let less = |l: &Expr, r: &Expr| Expr::BinOp(Box::new(l.clone()), BinOp::Less, Box::new(r.clone()));
    let mut sum = acc.clone();
    for (c, (op, v)) in [(less(step.a, step.b), &step.below), (less(step.b, step.a), &step.above)] {
        let pick = if *op == BinOp::Add { ADDEND } else { SUBTRAHEND };
        sum = Expr::BinOp(Box::new(sum), op.clone(), Box::new(Expr::FuncCall(pick.to_string(), vec![c, (*v).clone()])));
    }
    (acc, sum)
}

/// The accumulators of the [`ordered_step`]s in a body's bar loops (an
/// outermost loop that writes an output) whose accrual [`Around::accrual_free`]
/// admits. Keyed by name, as the stream tiers render a rewritten copy: a step
/// on the same name anywhere else in the body refuses it.
#[must_use]
pub fn ordered_step_targets(body: &[Statement]) -> HashSet<String> {
    let mut admitted = HashSet::new();
    bar_loops(body, &mut |pass| {
        if !stores_output(pass) {
            return;
        }
        let around = arounds(pass, &|s| matches!(s, Statement::If { .. }));
        each_if(pass, false, &mut |s| {
            let Statement::If { condition, then_body, else_body, .. } = s else { return };
            let Some((acc, arms)) = accrual(condition, then_body, else_body) else { return };
            if ordered_step(condition, then_body, else_body).is_some() && around.get(&addr(s)).is_some_and(|a| a.accrual_free(acc, &arms)) {
                admitted.insert(addr(s));
            }
        });
    });
    let (mut taken, mut refused) = (HashSet::new(), HashSet::new());
    each_if(body, true, &mut |s| {
        let Statement::If { condition, then_body, else_body, .. } = s else { return };
        if let Some(step) = ordered_step(condition, then_body, else_body) {
            (if admitted.contains(&addr(s)) { &mut taken } else { &mut refused }).insert(step.acc.to_string());
        }
    });
    taken.retain(|n| !refused.contains(n));
    taken
}

fn bar_loops<'b>(block: &'b [Statement], f: &mut dyn FnMut(&'b [Statement])) {
    for st in block {
        match loop_body(st) {
            Some(pass) => f(pass),
            None => {
                for b in nested(st) {
                    bar_loops(b, f);
                }
            }
        }
    }
}

/// Every statement of `block` at any depth, into loops only when `loops`.
fn each_if(block: &[Statement], loops: bool, f: &mut dyn FnMut(&Statement)) {
    for st in block {
        f(st);
        for b in nested(st).into_iter().chain(loop_body(st).filter(|_| loops)) {
            each_if(b, loops, f);
        }
    }
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

pub(super) fn loop_body(s: &Statement) -> Option<&[Statement]> {
    match s {
        Statement::While { body, .. } | Statement::DoWhile { body, .. } | Statement::For { body, .. } | Statement::ForC { body, .. } => Some(body),
        _ => None,
    }
}

pub(super) fn addr(s: &Statement) -> usize {
    std::ptr::from_ref(s) as usize
}

pub(super) fn nested(s: &Statement) -> Vec<&[Statement]> {
    match s {
        Statement::If { then_body, else_body, .. } => vec![then_body, else_body],
        Statement::Block { body } => vec![body],
        Statement::Switch { cases, default, .. } => cases.iter().map(|(_, b)| b.as_slice()).chain(std::iter::once(default.as_slice())).collect(),
        _ => Vec::new(),
    }
}

/// The [`Around`] of each statement `keep` takes in `body`, keyed by its
/// address.
pub(super) fn arounds(body: &[Statement], keep: &dyn Fn(&Statement) -> bool) -> HashMap<usize, Around> {
    let mut out = HashMap::new();
    visit(body, &mut Vec::new(), &mut Vec::new(), keep, &mut out);
    out
}

fn visit<'b>(block: &'b [Statement], path: &mut Vec<(&'b [Statement], usize)>, tests: &mut Vec<&'b Expr>, keep: &dyn Fn(&Statement) -> bool, out: &mut HashMap<usize, Around>) {
    for (i, st) in block.iter().enumerate() {
        path.push((block, i));
        if keep(st) {
            out.insert(addr(st), around(path, tests));
        }
        let cond = match st {
            Statement::If { condition, .. } => Some(condition),
            _ => None,
        };
        tests.extend(cond);
        for b in nested(st) {
            visit(b, path, tests, keep, out);
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
    use super::{accrual, ordered_step};
    use crate::backends::lockstep::{bin, var};
    use crate::ir::{BinOp, Expr, Statement};

    fn step(op: BinOp, e: Expr) -> Vec<Statement> {
        vec![Statement::Assign { target: var("acc"), value: bin(var("acc"), op, e), compound: true }]
    }

    /// `if a > b { acc += e }`.
    fn arm(e: Expr) -> (Expr, Vec<Statement>) {
        (bin(var("a"), BinOp::Greater, var("b")), step(BinOp::Add, e))
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

    /// `if c1 { acc += x } else if c2 { acc -= x }`.
    fn two(c1: &Expr, c2: Expr) -> bool {
        let inner = Statement::If { condition: c2, then_body: step(BinOp::Sub, var("x")), else_body: vec![], cond_comments: vec![] };
        ordered_step(c1, &step(BinOp::Add, var("x")), &[inner]).is_some()
    }

    #[test]
    fn an_ordered_step_tests_both_strict_orders_of_two_computed_values() {
        let cmp = |l: &str, op, r: Expr| bin(var(l), op, r);
        assert!(two(&cmp("a", BinOp::Greater, var("b")), cmp("a", BinOp::Less, var("b"))));
        assert!(two(&cmp("a", BinOp::Greater, var("b")), cmp("b", BinOp::Greater, var("a"))));
        assert!(!two(&cmp("a", BinOp::GreaterEq, var("b")), cmp("a", BinOp::Less, var("b"))), "a tie is the first arm's");
        assert!(!two(&cmp("a", BinOp::Greater, var("b")), cmp("a", BinOp::Less, var("c"))), "two other pairs");
        assert!(!two(&cmp("a", BinOp::Greater, Expr::Literal(0.0)), cmp("a", BinOp::Less, Expr::Literal(0.0))), "a sign test");
    }
}
