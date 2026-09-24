//! Which selects sit on a loop-carried arithmetic recurrence.
//!
//! Read by the C# renderer only: RyuJIT turns every floating-point select into
//! a branch, and the renderer lowers the exact ones to MAXSD/MINSD or a compare
//! mask everywhere except here.

use std::collections::{BTreeSet, HashMap, HashSet};

use crate::ir::{BinOp, Expr, Statement};
use crate::streaming::nested_bodies;

/// Targets of a select (`t = c ? a : b`, or `if( c ) t = v;`) whose result
/// comes back to the select's own operands through arithmetic on a later pass
/// of an enclosing loop. A branchless select there adds its latency to the
/// loop-carried chain every iteration, where a correctly predicted branch adds
/// nothing, so on predictable input it is slower.
///
/// Flow-sensitive, because a scratch variable reused for unrelated values
/// (every `tempReal`) must not tie a leaf select to a recurrence it never
/// feeds. Keyed by name, because the stream tier renders a rewritten copy of
/// the analysed body: one recurrent select keeps every select on its target a
/// branch.
#[must_use]
pub fn recurrent_select_targets(body: &[Statement]) -> HashSet<String> {
    let mut loops = Vec::new();
    collect(body, &mut |s| loop_parts(s).is_some(), &mut loops);
    let mut out = HashSet::new();
    for l in loops {
        let (_, passes, _) = loop_parts(l).unwrap_or_default();
        let mut selects = Vec::new();
        for b in &passes {
            collect(b, &mut |s| as_select(s).is_some(), &mut selects);
        }
        for s in selects {
            let target = as_select(s).map(|sel| sel.target).unwrap_or_default();
            if !out.contains(&target) && closes_through_arithmetic(&passes, s) {
                out.insert(target);
            }
        }
    }
    out
}

/// Name -> whether its dependence on the planted select passed through
/// arithmetic.
type Taint = HashMap<String, bool>;

/// `Seed` plants the select's result; `Check` is a later pass, where the
/// select reading that result back through arithmetic is the recurrence.
#[derive(Clone, Copy)]
enum Lap<'a> {
    Seed(&'a Statement),
    Check(&'a Statement),
}

/// States that leave a loop body early.
#[derive(Default)]
struct Exits {
    cont: Taint,
    brk: Taint,
}

struct Select {
    target: String,
    reads: DefUse,
}

/// The names an expression reads, and those among them read beneath
/// arithmetic. An array index computes an address, not a value on the chain.
#[derive(Default)]
struct DefUse {
    reads: BTreeSet<String>,
    arith: BTreeSet<String>,
}

impl DefUse {
    fn of(e: &Expr) -> Self {
        let mut d = Self::default();
        d.add(e, false);
        d
    }

    fn add(&mut self, e: &Expr, under: bool) {
        match e {
            Expr::Var(n) | Expr::PointerDeref(n) => self.name(n, under),
            Expr::ArrayAccess(n, idx) => {
                self.name(n, under);
                let idx = Self::of(idx);
                self.reads.extend(idx.reads);
            }
            Expr::BinOp(a, op, b) => {
                let under = under
                    || matches!(op, BinOp::Add | BinOp::Sub | BinOp::Mul | BinOp::Div | BinOp::Mod);
                self.add(a, under);
                self.add(b, under);
            }
            Expr::FuncCall(_, args) => {
                for a in args {
                    self.add(a, true);
                }
            }
            Expr::Ternary(c, t, f) => {
                self.add(c, under);
                self.add(t, under);
                self.add(f, under);
            }
            Expr::Cast(_, i)
            | Expr::Not(i)
            | Expr::BitwiseNot(i)
            | Expr::AddressOf(i)
            | Expr::PostIncrement(i)
            | Expr::PostDecrement(i)
            | Expr::PreIncrement(i)
            | Expr::PreDecrement(i) => self.add(i, under),
            Expr::Literal(_) | Expr::IntLiteral(_) => {}
        }
    }

    fn name(&mut self, n: &str, under: bool) {
        self.reads.insert(n.to_string());
        if under {
            self.arith.insert(n.to_string());
        }
    }
}

fn collect<'a>(stmts: &'a [Statement], keep: &mut dyn FnMut(&Statement) -> bool, out: &mut Vec<&'a Statement>) {
    for s in stmts {
        if keep(s) {
            out.push(s);
        }
        for b in nested_bodies(s).0 {
            collect(b, keep, out);
        }
    }
}

/// A loop's run-once prefix, the bodies of one pass, and whether a pass is
/// guaranteed.
fn loop_parts(s: &Statement) -> Option<(Vec<&[Statement]>, Vec<&[Statement]>, bool)> {
    match s {
        Statement::While { body, .. } | Statement::For { body, .. } => {
            Some((Vec::new(), vec![body.as_slice()], false))
        }
        Statement::DoWhile { body, .. } => Some((Vec::new(), vec![body.as_slice()], true)),
        Statement::ForC { init, update, body, .. } => Some((
            vec![std::slice::from_ref(init.as_ref())],
            vec![body.as_slice(), std::slice::from_ref(update.as_ref())],
            false,
        )),
        _ => None,
    }
}

fn as_select(s: &Statement) -> Option<Select> {
    match s {
        Statement::Assign { target: Expr::Var(t), value: value @ Expr::Ternary(..), compound: false } => {
            Some(Select { target: t.clone(), reads: DefUse::of(value) })
        }
        Statement::If { condition, then_body, else_body, .. } if else_body.is_empty() => {
            let code: Vec<&Statement> =
                then_body.iter().filter(|t| !matches!(t, Statement::Comment(_))).collect();
            let [Statement::Assign { target: Expr::Var(t), value, compound: false }] = code.as_slice()
            else {
                return None;
            };
            let mut reads = DefUse::of(condition);
            reads.add(value, false);
            reads.name(t, false);
            Some(Select { target: t.clone(), reads })
        }
        _ => None,
    }
}

fn closes_through_arithmetic(passes: &[&[Statement]], select: &Statement) -> bool {
    let mut taint = Taint::new();
    let mut hit = false;
    one_pass(passes, &mut taint, Lap::Seed(select), &mut hit);
    // Monotone in the accumulated state, so this ends; a later pass catches a
    // recurrence carried through a delay line.
    loop {
        let before = taint.clone();
        let mut pass = taint.clone();
        one_pass(passes, &mut pass, Lap::Check(select), &mut hit);
        merge(&mut taint, pass);
        if hit || taint == before {
            return hit;
        }
    }
}

/// One pass of the analysed loop: `continue` rejoins at the back edge, and a
/// path that leaves the loop cannot come back to the select.
fn one_pass(passes: &[&[Statement]], taint: &mut Taint, lap: Lap, hit: &mut bool) {
    let mut exits = Exits::default();
    for b in passes {
        step(b, taint, lap, hit, &mut exits);
    }
    merge(taint, exits.cont);
}

fn assign(taint: &mut Taint, target: &str, d: &DefUse) {
    let mut hits = d.reads.iter().filter_map(|r| taint.get(r).map(|&a| a || d.arith.contains(r)));
    match hits.next() {
        None => {
            taint.remove(target);
        }
        Some(first) => {
            let arith = first || hits.any(|a| a);
            taint.insert(target.to_string(), arith);
        }
    }
}

fn merge(a: &mut Taint, b: Taint) {
    for (k, v) in b {
        *a.entry(k).or_insert(v) |= v;
    }
}

fn step(stmts: &[Statement], taint: &mut Taint, lap: Lap, hit: &mut bool, exits: &mut Exits) {
    for s in stmts {
        if let Some(sel) = as_select(s) {
            match lap {
                Lap::Seed(p) if std::ptr::eq(s, p) => {
                    taint.insert(sel.target, false);
                }
                Lap::Check(p) if std::ptr::eq(s, p) => {
                    let d = &sel.reads;
                    *hit |= d.reads.iter().any(|r| match taint.get(r) {
                        Some(&arith) => arith || d.arith.contains(r),
                        None => false,
                    });
                    assign(taint, &sel.target, d);
                }
                _ => assign(taint, &sel.target, &sel.reads),
            }
            continue;
        }
        match s {
            Statement::Assign { target, value, compound } => {
                let (Expr::Var(t) | Expr::ArrayAccess(t, _) | Expr::PointerDeref(t)) = target else {
                    continue;
                };
                let mut d = DefUse::of(value);
                // A compound store reads its target through arithmetic; a
                // store into one element leaves the rest of the array as it was.
                if *compound {
                    d.name(t, true);
                } else if let Expr::ArrayAccess(_, idx) = target {
                    d.name(t, false);
                    d.reads.extend(DefUse::of(idx).reads);
                }
                assign(taint, t, &d);
            }
            Statement::VarDecl { name, init: Some(init), .. } => assign(taint, name, &DefUse::of(init)),
            Statement::Continue => merge(&mut exits.cont, std::mem::take(taint)),
            Statement::Break => merge(&mut exits.brk, std::mem::take(taint)),
            Statement::Return { .. } => taint.clear(),
            other => {
                if let Some((init, passes, at_least_once)) = loop_parts(other) {
                    inner_loop(&init, &passes, at_least_once, taint, lap, hit);
                    continue;
                }
                let bodies = nested_bodies(other).0;
                if bodies.is_empty() {
                    continue;
                }
                // Exactly one arm runs; an `if` without else has an empty one.
                let entry = std::mem::take(taint);
                for b in bodies {
                    let mut arm = entry.clone();
                    step(b, &mut arm, lap, hit, exits);
                    merge(taint, arm);
                }
            }
        }
    }
}

fn inner_loop(
    init: &[&[Statement]],
    passes: &[&[Statement]],
    at_least_once: bool,
    taint: &mut Taint,
    lap: Lap,
    hit: &mut bool,
) {
    let mut exits = Exits::default();
    for b in init {
        step(b, taint, lap, hit, &mut exits);
    }
    if at_least_once {
        for b in passes {
            step(b, taint, lap, hit, &mut exits);
        }
    }
    // Zero or more further passes. The accumulated state only grows, so the
    // fixpoint is reached.
    loop {
        let before = taint.clone();
        let mut pass = taint.clone();
        for b in passes {
            step(b, &mut pass, lap, hit, &mut exits);
        }
        merge(taint, pass);
        merge(taint, std::mem::take(&mut exits.cont));
        if *taint == before {
            break;
        }
    }
    merge(taint, exits.brk);
}
