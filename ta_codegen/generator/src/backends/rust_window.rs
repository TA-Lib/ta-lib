//! Innermost batch loops as a counted `for` over slices cut once at entry
//! (#442).
//!
//! C's loops advance several indices in lockstep (`today`, `trailingIdx`,
//! `outIdx`) and test only one of them, so LLVM keeps a bounds check on every
//! access. Here each access `a[inv + v + c]`, where `v` moves by one per pass
//! and `inv` not at all, becomes `_wJ[_wk + d]` over
//! `let _wJ = &a[start..][..len]` inside `for _wk in 0.._wn`, and the checks
//! go.
//!
//! Rust-only and render-time: [`plan`] reads the loop as written and returns
//! its body with those accesses re-indexed. Every statement stays, the index
//! updates included, so each variable holds C's value inside and after the
//! loop.
//!
//! A window spans only elements that some access reaches on every pass, so
//! cutting it panics only where the loop as written would. When a window must
//! also hold elements read only on some passes, every window is cut with
//! `get`, and the loop runs as written when one does not fit.

use std::collections::{BTreeMap, BTreeSet};

use super::builtins::{MathFn, SpecialBuiltin};
use super::rust_respell::SELECT_OTHER;
use crate::ir::{BinOp, CircBuf, Expr, Statement};

/// The pass count and the pass index in the emitted code.
pub(crate) const TRIP: &str = "_wn";
pub(crate) const PASS: &str = "_wk";

pub(crate) fn window_name(j: usize) -> String {
    format!("_w{j}")
}

const MARK: &str = "\u{1}w";

fn marker(j: usize, d: i64) -> Expr {
    Expr::Var(format!("{MARK}{j}:{d}"))
}

/// A re-indexed access's rendering, window name included; `None` for any
/// other index.
pub(crate) fn render_marker(idx: &Expr) -> Option<String> {
    let Expr::Var(n) = idx else { return None };
    let (j, d) = n.strip_prefix(MARK)?.split_once(':')?;
    let (j, d): (usize, i64) = (j.parse().ok()?, d.parse().ok()?);
    let at = if d == 0 { PASS.to_string() } else { format!("{PASS} + {d}") };
    Some(format!("{}[{at}]", window_name(j)))
}

pub(crate) struct Window {
    pub array: String,
    pub mutable: bool,
    /// Evaluated at loop entry, where it may read [`TRIP`].
    pub start: Expr,
    /// The window is [`TRIP`]` + extra` elements long.
    pub extra: i64,
}

pub(crate) struct Plan {
    /// When true, the loop makes exactly `trip` passes, at least one.
    pub guard: Expr,
    pub trip: Expr,
    pub windows: Vec<Window>,
    pub reverse: bool,
    /// Opens with the condition's own step, when it takes one.
    pub body: Vec<Statement>,
    /// A counter the condition decrements on every test, the failing one
    /// included.
    pub cond_step: Option<String>,
    /// When `guard` fails the loop as written may still pass, so it runs in
    /// place of that failing test's step.
    pub may_pass_unguarded: bool,
    /// Some window holds an element that no access reaches on every pass: the
    /// windows are cut only if all of them fit, and the loop as written runs
    /// otherwise. Their starts may then be below zero, so they wrap.
    pub checked: bool,
}

/// What the renderer knows about names that the IR does not say.
pub(crate) struct Names<'a> {
    /// A `usize` local or parameter.
    pub index: &'a dyn Fn(&str) -> bool,
    /// An expression that renders as a `usize`.
    pub usize_expr: &'a dyn Fn(&Expr) -> bool,
    /// An array a window may be cut from.
    pub sliceable: &'a dyn Fn(&str) -> bool,
    /// One of the inputs that all have one length here.
    pub same_len: &'a dyn Fn(&str) -> bool,
    /// The storage a CIRCBUF's wrap reads the length of, by ring id.
    pub ring_storage: &'a dyn Fn(&str) -> Option<String>,
}

/// `while( cond ) body`, or with `update`, the rest of
/// `for( init; cond; update ) body` once `init` has run.
pub(crate) fn plan(cond: &Expr, body: &[Statement], update: Option<&Statement>, names: &Names) -> Option<Plan> {
    let form = form(cond, names)?;
    // The condition's own step comes first in every pass.
    let mut pass: Vec<Statement> = form.cond_step.iter().map(|c| step(c, -1)).collect();
    pass.extend(body.iter().cloned());
    pass.extend(update.cloned());
    let pass = flat(&pass)?;
    if mentions_reserved(cond, &pass) {
        return None;
    }

    let mut facts = Facts::of(&pass);
    // A ring's wrap reads its storage, which a `&mut` window would lock.
    fn advanced(pass: &[Statement], out: &mut Vec<String>) {
        for s in pass {
            match s {
                Statement::CircBuf(CircBuf::Next { id }) => out.push(id.clone()),
                Statement::If { then_body, else_body, .. } => {
                    advanced(then_body, out);
                    advanced(else_body, out);
                }
                _ => {}
            }
        }
    }
    let mut rings = Vec::new();
    advanced(&pass, &mut rings);
    for id in &rings {
        facts.bare.extend((names.ring_storage)(id));
    }
    let steps: BTreeMap<String, i64> = facts.induction().into_iter().filter(|(v, _)| (names.index)(v)).collect();
    // A counter that moves once per pass, the condition's own step included,
    // against a bound that does not move.
    if steps.get(&form.counter) != Some(&form.step) || !invariant(&form.bound, &facts) {
        return None;
    }

    let branches = pass.iter().any(|s| matches!(s, Statement::If { .. }));
    let an = Analysis { steps: &steps, facts: &facts, names, branches };
    let occs = an.occurrences(&pass);
    let (windows, reverse, assign, checked) = an.windows(&occs, &form.counter)?;
    let body = rewrite(&pass, &assign, &mut 0);
    Some(Plan {
        guard: form.guard,
        trip: form.trip,
        windows,
        reverse,
        body,
        cond_step: form.cond_step,
        may_pass_unguarded: form.may_pass_unguarded,
        checked,
    })
}

fn step(v: &str, d: i64) -> Statement {
    let op = if d > 0 { BinOp::Add } else { BinOp::Sub };
    Statement::Assign { target: var(v), value: bin(var(v), op, int(1)), compound: true }
}
fn var(n: &str) -> Expr {
    Expr::Var(n.to_string())
}
fn int(n: i64) -> Expr {
    Expr::IntLiteral(n)
}
fn bin(l: Expr, op: BinOp, r: Expr) -> Expr {
    Expr::BinOp(Box::new(l), op, Box::new(r))
}

// ---------------------------------------------------------------------------
// The header
// ---------------------------------------------------------------------------

struct Form {
    counter: String,
    /// The counter's move per pass.
    step: i64,
    bound: Expr,
    guard: Expr,
    trip: Expr,
    cond_step: Option<String>,
    may_pass_unguarded: bool,
}

fn form(cond: &Expr, names: &Names) -> Option<Form> {
    let Expr::BinOp(lhs, op, rhs) = cond else { return None };
    let bound = rhs.as_ref().clone();
    if !(names.usize_expr)(&bound) {
        return None;
    }
    let zero = matches!(bound, Expr::IntLiteral(0));
    let limit = || bound.clone();
    let counted = |c: &str, d: i64, trip: Expr| Form {
        counter: c.into(),
        step: d,
        bound: limit(),
        guard: cond.clone(),
        trip,
        cond_step: None,
        may_pass_unguarded: false,
    };
    let tested = |c: &str, guard: Expr, trip: Expr| Form {
        counter: c.into(),
        step: -1,
        bound: limit(),
        guard,
        trip,
        cond_step: Some(c.into()),
        may_pass_unguarded: false,
    };
    let name = |x: &Expr| match x {
        Expr::Var(n) => Some(n.clone()),
        _ => None,
    };
    let form = match (lhs.as_ref(), op) {
        (Expr::Var(c), BinOp::Less) => counted(c, 1, bin(limit(), BinOp::Sub, var(c))),
        (Expr::Var(c), BinOp::LessEq) => counted(c, 1, bin(bin(limit(), BinOp::Sub, var(c)), BinOp::Add, int(1))),
        (Expr::Var(c), BinOp::Greater) if zero => counted(c, -1, var(c)),
        (Expr::Var(c), BinOp::Greater) => counted(c, -1, bin(var(c), BinOp::Sub, limit())),
        (Expr::Var(c), BinOp::NotEq) if zero => counted(c, -1, var(c)),
        // `c-- > 0`, `c-- != 0`
        (Expr::PostDecrement(c), BinOp::Greater | BinOp::NotEq) if zero => {
            let c = name(c)?;
            tested(&c, bin(var(&c), BinOp::Greater, int(0)), var(&c))
        }
        // `--c != 0`, which at `c == 0` wraps and passes.
        (Expr::PreDecrement(c), BinOp::NotEq) if zero => {
            let c = name(c)?;
            let f = tested(&c, bin(var(&c), BinOp::Greater, int(1)), bin(var(&c), BinOp::Sub, int(1)));
            Form { may_pass_unguarded: true, ..f }
        }
        _ => return None,
    };
    (names.index)(&form.counter).then_some(form)
}

// ---------------------------------------------------------------------------
// What a pass does
// ---------------------------------------------------------------------------

/// A pass with no nested loop and no exit but the condition, its blocks
/// inlined and its short countdowns unrolled; `None` for any other.
fn flat(body: &[Statement]) -> Option<Vec<Statement>> {
    let mut out = Vec::new();
    for s in body {
        match s {
            Statement::Assign { .. }
            | Statement::Expr(_)
            | Statement::CircBuf(CircBuf::Next { .. })
            | Statement::Comment(_)
            | Statement::UnrollHint { .. } => out.push(s.clone()),
            Statement::Block { body } => out.extend(flat(body)?),
            Statement::If { condition, then_body, else_body, cond_comments } => out.push(Statement::If {
                condition: condition.clone(),
                then_body: flat(then_body)?,
                else_body: flat(else_body)?,
                cond_comments: cond_comments.clone(),
            }),
            Statement::ForC { init, condition, update, body } => out.extend(unrolled(init, condition, update, body)?),
            _ => return None,
        }
    }
    Some(out)
}

/// `for( v = a; v >= b; v-- ) body`, literal `a` and `b` a few apart and a
/// body that leaves `v` alone, as one copy of the body per value, then
/// `v = b`: where the Rust countdown leaves `v`.
fn unrolled(init: &Statement, cond: &Expr, update: &Statement, body: &[Statement]) -> Option<Vec<Statement>> {
    const MAX_PASSES: i64 = 8;
    let Statement::Assign { target: Expr::Var(v), value: Expr::IntLiteral(a), compound: false } = init else {
        return None;
    };
    let Expr::BinOp(l, BinOp::GreaterEq, r) = cond else { return None };
    let (Expr::Var(c), Expr::IntLiteral(b)) = (l.as_ref(), r.as_ref()) else { return None };
    let body = flat(body)?;
    if c != v || as_step(update) != Some((v.as_str(), -1)) || a < b || a - b >= MAX_PASSES || Facts::of(&body).assigned.contains(v) {
        return None;
    }
    let mut out = Vec::new();
    for k in (*b..=*a).rev() {
        out.extend(body.iter().map(|s| substitute_stmt(s, v, &int(k))));
    }
    out.push(Statement::Assign { target: var(v), value: int(*b), compound: false });
    Some(out)
}

fn substitute_stmt(s: &Statement, v: &str, with: &Expr) -> Statement {
    let sub = |e: &Expr| substitute(e, v, with);
    let all = |b: &[Statement]| b.iter().map(|x| substitute_stmt(x, v, with)).collect();
    match s {
        Statement::Assign { target, value, compound } => {
            Statement::Assign { target: sub(target), value: sub(value), compound: *compound }
        }
        Statement::Expr(e) => Statement::Expr(sub(e)),
        Statement::If { condition, then_body, else_body, cond_comments } => Statement::If {
            condition: sub(condition),
            then_body: all(then_body),
            else_body: all(else_body),
            cond_comments: cond_comments.clone(),
        },
        other => other.clone(),
    }
}

/// A body naming one of the emitted names would read ours.
fn mentions_reserved(cond: &Expr, body: &[Statement]) -> bool {
    let emitted = |n: &str| {
        n == TRIP || n == PASS || n.strip_prefix("_w").is_some_and(|j| !j.is_empty() && j.bytes().all(|b| b.is_ascii_digit()))
    };
    let mut found = false;
    let mut check = |e: &Expr| {
        crate::streaming::walk_expr(e, &mut |x| {
            if let Expr::Var(n) | Expr::ArrayAccess(n, _) | Expr::PointerDeref(n) = x {
                found |= emitted(n);
            }
        });
    };
    fn walk(body: &[Statement], check: &mut dyn FnMut(&Expr)) {
        for s in body {
            match s {
                Statement::Assign { target, value, .. } => {
                    check(target);
                    check(value);
                }
                Statement::Expr(e) => check(e),
                Statement::If { condition, then_body, else_body, .. } => {
                    check(condition);
                    walk(then_body, check);
                    walk(else_body, check);
                }
                _ => {}
            }
        }
    }
    check(cond);
    walk(body, &mut check);
    found
}

/// A call that evaluates each argument exactly once.
fn eager_call(name: &str) -> bool {
    MathFn::from_name(name).is_some()
        || name == SELECT_OTHER
        || matches!(
            SpecialBuiltin::from_name(name),
            Some(SpecialBuiltin::IsZero | SpecialBuiltin::IsZeroScaled | SpecialBuiltin::IsZeroOrNeg | SpecialBuiltin::IsFinite)
        )
}

/// `v += 1`, `v = v - 1`, ...: the variable and its step.
fn as_step(stmt: &Statement) -> Option<(&str, i64)> {
    let Statement::Assign { target: Expr::Var(target), value: Expr::BinOp(lhs, op, rhs), .. } = stmt else {
        return None;
    };
    let delta = match op {
        BinOp::Add => 1,
        BinOp::Sub => -1,
        _ => return None,
    };
    let is_self = matches!(lhs.as_ref(), Expr::Var(n) if n == target);
    (is_self && matches!(rhs.as_ref(), Expr::IntLiteral(1))).then_some((target.as_str(), delta))
}

/// `a[v++] = x` or `x = a[v++]`, naming `v` nowhere else: the access, then
/// `v += 1`, as the renderer splits it.
fn trailing_step(s: &Statement) -> Option<&str> {
    let Statement::Assign { target, value, compound: false } = s else { return None };
    let v = post_increment(target).or_else(|| post_increment(value))?;
    let mut uses = 0;
    for e in [target, value] {
        crate::streaming::walk_expr(e, &mut |x| uses += usize::from(matches!(x, Expr::Var(n) if n == v)));
    }
    (uses == 1).then_some(v)
}

fn post_increment(e: &Expr) -> Option<&str> {
    let Expr::ArrayAccess(_, i) = e else { return None };
    let Expr::PostIncrement(v) = i.as_ref() else { return None };
    let Expr::Var(v) = v.as_ref() else { return None };
    Some(v)
}

fn step_of(s: &Statement) -> Option<(&str, i64)> {
    as_step(s).or_else(|| trailing_step(s).map(|v| (v, 1)))
}

#[derive(Default)]
struct Facts {
    /// Names stored to other than as an element.
    assigned: BTreeSet<String>,
    /// Names stored to by anything but a step.
    other: BTreeSet<String>,
    /// Per variable, its steps in pass order.
    steps: BTreeMap<String, Vec<i64>>,
    /// Arrays with an element stored.
    written: BTreeSet<String>,
    /// Names used other than as the array of an element access.
    bare: BTreeSet<String>,
}

impl Facts {
    fn of(pass: &[Statement]) -> Facts {
        let mut f = Facts::default();
        for s in pass {
            f.statement(s);
        }
        f
    }

    fn store(&mut self, n: &str) {
        self.assigned.insert(n.to_string());
        self.other.insert(n.to_string());
    }

    fn statement(&mut self, s: &Statement) {
        match s {
            Statement::Assign { target, value, .. } => {
                let stepped = step_of(s);
                if let Some((v, d)) = stepped {
                    self.assigned.insert(v.to_string());
                    self.steps.entry(v.to_string()).or_default().push(d);
                }
                match target {
                    Expr::Var(t) if stepped.map(|(v, _)| v) != Some(t.as_str()) => self.store(t),
                    Expr::PointerDeref(t) => self.store(t),
                    Expr::ArrayAccess(a, i) => {
                        self.written.insert(a.clone());
                        self.expr(i, trailing_step(s));
                    }
                    _ => {}
                }
                if as_step(s).is_none() {
                    self.expr(value, trailing_step(s));
                }
            }
            Statement::Expr(e) => {
                // A call made for its effect: whatever it names may change.
                if !matches!(e, Expr::FuncCall(n, _) if eager_call(n)) {
                    crate::streaming::walk_expr(e, &mut |x| {
                        if let Expr::Var(n) | Expr::PointerDeref(n) | Expr::ArrayAccess(n, _) = x {
                            self.assigned.insert(n.clone());
                            self.other.insert(n.clone());
                            self.bare.insert(n.clone());
                        }
                    });
                }
                self.expr(e, None);
            }
            Statement::CircBuf(CircBuf::Next { id }) => self.store(&format!("{id}_Idx")),
            // A step taken in an arm is not taken on every pass: what an arm
            // assigns is no induction variable.
            Statement::If { condition, then_body, else_body, .. } => {
                self.expr(condition, None);
                let arms = Facts::of(&[then_body.as_slice(), else_body.as_slice()].concat());
                self.other.extend(arms.assigned.iter().cloned());
                self.assigned.extend(arms.assigned);
                self.other.extend(arms.other);
                self.written.extend(arms.written);
                self.bare.extend(arms.bare);
            }
            _ => {}
        }
    }

    /// Stores hidden in an expression, and names used bare. `split` is the
    /// statement's own `v++`, which is a step.
    fn expr(&mut self, e: &Expr, split: Option<&str>) {
        let mut stores = Vec::new();
        crate::streaming::walk_expr(e, &mut |x| match x {
            Expr::PostIncrement(i) | Expr::PreIncrement(i) | Expr::PostDecrement(i) | Expr::PreDecrement(i) => {
                if let Expr::Var(n) = i.as_ref() {
                    if !(matches!(x, Expr::PostIncrement(_)) && split == Some(n.as_str())) {
                        stores.push(n.clone());
                    }
                }
            }
            Expr::AddressOf(i) => {
                if let Expr::Var(n) | Expr::ArrayAccess(n, _) = i.as_ref() {
                    stores.push(n.clone());
                    self.bare.insert(n.clone());
                }
            }
            Expr::Var(n) | Expr::PointerDeref(n) => {
                self.bare.insert(n.clone());
            }
            _ => {}
        });
        for n in stores {
            self.store(&n);
        }
    }

    /// Variables that move by one, one way, once per pass.
    fn induction(&self) -> BTreeMap<String, i64> {
        self.steps
            .iter()
            .filter(|(v, s)| s.len() == 1 && !self.other.contains(*v))
            .map(|(v, s)| (v.clone(), s[0]))
            .collect()
    }
}

/// Side-effect free, and nothing it reads changes within the loop.
fn invariant(e: &Expr, facts: &Facts) -> bool {
    match e {
        Expr::IntLiteral(_) => true,
        Expr::Var(n) | Expr::PointerDeref(n) => !facts.assigned.contains(n) && !n.contains('.'),
        Expr::BinOp(l, _, r) => invariant(l, facts) && invariant(r, facts),
        Expr::Cast(_, i) => invariant(i, facts),
        _ => false,
    }
}

// ---------------------------------------------------------------------------
// Accesses
// ---------------------------------------------------------------------------

/// An access `array[inv + var + c]`, less `c`: what a window is cut by.
#[derive(Clone, PartialEq, Eq, PartialOrd, Ord, Debug)]
struct Group {
    array: String,
    var: String,
    /// The invariant terms with their signs, sorted, as `Debug` text.
    inv: Vec<String>,
}

struct Occ {
    array: String,
    /// The index as written, less any `v++`.
    index: Expr,
    /// Its group, and `c` plus the variable's steps so far in the pass: the
    /// element it reaches, relative to the pass.
    elem: Option<(Group, i64)>,
    /// The variable's steps so far in the pass.
    offset: i64,
    /// Made on every pass: [`Reach::Every`].
    every: bool,
}

struct Analysis<'a> {
    steps: &'a BTreeMap<String, i64>,
    facts: &'a Facts,
    names: &'a Names<'a>,
    branches: bool,
}

impl Analysis<'_> {
    /// `(v, invariant terms, c)` for an index `inv + v + c`.
    fn linear(&self, idx: &Expr) -> Option<(String, Vec<String>, i64)> {
        let mut terms = Vec::new();
        flatten(idx, true, &mut terms);
        let mut var = None;
        let mut inv = Vec::new();
        let mut c = 0i64;
        for (e, plus) in terms {
            match e {
                Expr::IntLiteral(n) => c += if plus { *n } else { -*n },
                Expr::Var(n) if self.steps.contains_key(n) => {
                    if !plus || var.replace(n.clone()).is_some() {
                        return None;
                    }
                }
                other if invariant(other, self.facts) => inv.push(format!("{}{other:?}", if plus { '+' } else { '-' })),
                _ => return None,
            }
        }
        inv.sort();
        Some((var?, inv, c))
    }

    /// Every element access, in the order [`rewrite`] meets them.
    fn occurrences(&self, pass: &[Statement]) -> Vec<Occ> {
        let mut out = Vec::new();
        let mut off: BTreeMap<String, i64> = BTreeMap::new();
        for s in pass {
            each_access(s, &mut |array, idx, reach| {
                let index = match idx {
                    Expr::PostIncrement(v) => v.as_ref().clone(),
                    other => other.clone(),
                };
                let usable = reach != Reach::Arm
                    && !array.contains('.')
                    && (self.names.sliceable)(array)
                    && !self.facts.assigned.contains(array)
                    && !self.facts.bare.contains(array);
                let lin = if usable { self.linear(&index) } else { None };
                let offset = lin.as_ref().map_or(0, |(v, _, _)| off.get(v).copied().unwrap_or(0));
                let elem = lin.map(|(var, inv, c)| (Group { array: array.to_string(), var, inv }, c + offset));
                out.push(Occ { array: array.to_string(), index, elem, offset, every: reach == Reach::Every });
            });
            if let Some((v, d)) = step_of(s) {
                *off.entry(v.to_string()).or_default() += d;
            }
        }
        out
    }

    /// The windows, the direction, per access its window and offset in it, and
    /// whether the windows are cut only when they fit. `None` when no window
    /// would lift a check.
    #[allow(clippy::type_complexity)]
    fn windows(&self, occs: &[Occ], counter: &str) -> Option<(Vec<Window>, bool, Vec<Option<(usize, i64)>>, bool)> {
        // Inputs of one length share their extents: an element one of them
        // reaches on every pass is in bounds in all of them.
        let pooled = |g: &Group| {
            let array = if (self.names.same_len)(&g.array) { String::new() } else { g.array.clone() };
            Group { array, ..g.clone() }
        };
        let extent = |every_only: bool| {
            let mut span: BTreeMap<Group, (i64, i64)> = BTreeMap::new();
            for (g, e) in occs.iter().filter(|o| o.every || !every_only).filter_map(|o| o.elem.as_ref()) {
                let s = span.entry(pooled(g)).or_insert((*e, *e));
                s.0 = s.0.min(*e);
                s.1 = s.1.max(*e);
            }
            span
        };
        let (mut sure, mut all) = (extent(true), extent(false));
        let led = if sure.is_empty() { &all } else { &sure };
        let dir = if led.keys().any(|g| self.steps[&g.var] > 0) { 1 } else { -1 };
        sure.retain(|g, _| self.steps[&g.var] == dir);
        all.retain(|g, _| self.steps[&g.var] == dir);
        let fits = |span: &BTreeMap<Group, (i64, i64)>, o: &Occ| {
            o.elem.clone().filter(|(g, e)| span.get(&pooled(g)).is_some_and(|(lo, hi)| lo <= e && e <= hi))
        };

        // Each array's own extent within its pool's.
        let own_in = |span: &BTreeMap<Group, (i64, i64)>| {
            let mut own: BTreeMap<Group, (i64, i64)> = BTreeMap::new();
            for (g, e) in occs.iter().filter_map(|o| fits(span, o)) {
                let s = own.entry(g).or_insert((e, e));
                s.0 = s.0.min(e);
                s.1 = s.1.max(e);
            }
            // An array stored to takes one mutable window, holding every access.
            for a in &self.facts.written {
                let groups = own.keys().filter(|g| &g.array == a).count();
                if groups > 1 || occs.iter().any(|o| &o.array == a && fits(span, o).is_none()) {
                    own.retain(|g, _| &g.array != a);
                }
            }
            own
        };
        let narrow = own_in(&sure);
        let wide = own_in(&all);
        let (mut span, mut own) = (sure, narrow);
        let checked = wide != own;
        if checked {
            (span, own) = (all, wide);
        }
        // An input read at the counter's own value is usually bounded already,
        // and windowing a loop of nothing else measured slower. In a body that
        // branches, so does one read at a fixed step behind the counter.
        let free = |g: &Group| {
            let (lo, hi) = own[g];
            let reach = if self.branches { lo <= 0 && hi <= 0 } else { (lo, hi) == (0, 0) };
            (self.names.same_len)(&g.array) && g.var == counter && g.inv.is_empty() && reach
        };
        if own.keys().all(free) {
            return None;
        }
        // A branching body is windowed only for reads some passes skip: for
        // every-pass reads alone, LLVM re-forms the branch once their checks go,
        // which measured slower.
        if self.branches && !checked {
            return None;
        }

        let order: Vec<&Group> = own.keys().collect();
        let mut windows = Vec::new();
        for g in &order {
            let (lo_own, hi_own) = own[*g];
            let key = pooled(g);
            let lo = span[&key].0;
            // An access reaching the pool's lowest element, on every pass unless
            // the windows are checked: the window starts a fixed step above what
            // it computes.
            let rep = occs
                .iter()
                .find(|o| (o.every || checked) && o.elem.as_ref().is_some_and(|(x, e)| pooled(x) == key && *e == lo))?;
            let at = if dir > 0 {
                shifted(&g.var, rep.offset)
            } else {
                // The last pass, where a descending index is lowest: never below
                // a value the variable takes.
                match rep.offset + 1 {
                    0 => bin(var(&g.var), BinOp::Sub, var(TRIP)),
                    k => bin(var(&g.var), BinOp::Sub, bin(var(TRIP), BinOp::Sub, int(k))),
                }
            };
            let first = substitute(&rep.index, &g.var, &at);
            // A checked start may come from an access the loop never makes, so
            // it must not be able to fault on its own: sums and differences of
            // names and literals only, which wrap.
            if checked && !wraps(&first) {
                return None;
            }
            windows.push(Window {
                array: g.array.clone(),
                mutable: self.facts.written.contains(&g.array),
                start: if lo_own == lo { first } else { bin(first, BinOp::Add, int(lo_own - lo)) },
                extra: hi_own - lo_own,
            });
        }
        let assign = occs
            .iter()
            .map(|o| {
                let (g, e) = fits(&span, o)?;
                let j = order.iter().position(|x| **x == g)?;
                Some((j, e - own[&g].0))
            })
            .collect();
        Some((windows, dir < 0, assign, checked))
    }
}

/// `pass` with each access `assign` places re-indexed into its window.
/// `next` counts the accesses already placed.
fn rewrite(pass: &[Statement], assign: &[Option<(usize, i64)>], next: &mut usize) -> Vec<Statement> {
    let mut out = Vec::new();
    for s in pass {
        if let Statement::If { condition, then_body, else_body, cond_comments } = s {
            let head = Statement::Expr(condition.clone());
            let Statement::Expr(condition) = rewrite(std::slice::from_ref(&head), assign, next).remove(0) else {
                unreachable!()
            };
            let then_body = rewrite(then_body, assign, next);
            let else_body = rewrite(else_body, assign, next);
            out.push(Statement::If { condition, then_body, else_body, cond_comments: cond_comments.clone() });
            continue;
        }
        let mapped = map_accesses(s, &mut |e| {
            let Expr::ArrayAccess(a, i) = e else { return e };
            let m = assign[*next].map(|(j, d)| marker(j, d));
            *next += 1;
            Expr::ArrayAccess(a, Box::new(m.unwrap_or(*i)))
        });
        // A `v++` whose access now reads a window leaves its step behind.
        let split = trailing_step(s).filter(|v| !mentions_post_increment(&mapped, v));
        out.push(mapped);
        if let Some(v) = split {
            out.push(step(v, 1));
        }
    }
    out
}

/// Calls `f(array, index, every)` for each element access in `s`, in
/// evaluation order.
fn each_access(s: &Statement, f: &mut dyn FnMut(&str, &Expr, Reach)) {
    each_access_in(s, Reach::Every, f);
}

/// How often an access is made.
#[derive(Clone, Copy, PartialEq, Eq, Debug)]
enum Reach {
    Every,
    /// Under `?:`, `&&`, `||` or a call that may skip an argument.
    Some,
    /// In a branch's arm, where it keeps its check: a checked access there is
    /// what keeps LLVM from re-forming the branch, and reads skipped this way
    /// measured no gain.
    Arm,
}

fn each_access_in(s: &Statement, reach: Reach, f: &mut dyn FnMut(&str, &Expr, Reach)) {
    fn go(expr: &Expr, reach: Reach, visit: &mut dyn FnMut(&str, &Expr, Reach)) {
        let some = if reach == Reach::Every { Reach::Some } else { reach };
        match expr {
            Expr::ArrayAccess(array, idx) => {
                go(idx, reach, visit);
                visit(array, idx, reach);
            }
            Expr::BinOp(lhs, op, rhs) => {
                go(lhs, reach, visit);
                go(rhs, if matches!(op, BinOp::And | BinOp::Or) { some } else { reach }, visit);
            }
            Expr::Ternary(cond, then, other) => {
                go(cond, reach, visit);
                go(then, some, visit);
                go(other, some, visit);
            }
            Expr::FuncCall(name, args) => {
                let reach = if eager_call(name) { reach } else { some };
                for arg in args {
                    go(arg, reach, visit);
                }
            }
            Expr::Cast(_, inner)
            | Expr::Not(inner)
            | Expr::BitwiseNot(inner)
            | Expr::AddressOf(inner)
            | Expr::PostIncrement(inner)
            | Expr::PostDecrement(inner)
            | Expr::PreIncrement(inner)
            | Expr::PreDecrement(inner) => go(inner, reach, visit),
            Expr::Literal(_) | Expr::IntLiteral(_) | Expr::Var(_) | Expr::PointerDeref(_) => {}
        }
    }
    match s {
        Statement::Assign { target, value, .. } => {
            go(value, reach, f);
            go(target, reach, f);
        }
        Statement::Expr(e) => go(e, reach, f),
        Statement::If { condition, then_body, else_body, .. } => {
            go(condition, reach, f);
            for arm in then_body.iter().chain(else_body) {
                each_access_in(arm, Reach::Arm, f);
            }
        }
        _ => {}
    }
}

/// `s` with every element access passed through `f`, in [`each_access`]'s
/// order.
fn map_accesses(s: &Statement, f: &mut dyn FnMut(Expr) -> Expr) -> Statement {
    fn go(expr: &Expr, map: &mut dyn FnMut(Expr) -> Expr) -> Expr {
        let bx = Box::new;
        match expr {
            Expr::ArrayAccess(array, idx) => {
                let idx = go(idx, map);
                map(Expr::ArrayAccess(array.clone(), bx(idx)))
            }
            Expr::BinOp(lhs, op, rhs) => {
                let lhs = go(lhs, map);
                Expr::BinOp(bx(lhs), op.clone(), bx(go(rhs, map)))
            }
            Expr::Ternary(cond, then, other) => {
                let cond = go(cond, map);
                let then = go(then, map);
                Expr::Ternary(bx(cond), bx(then), bx(go(other, map)))
            }
            Expr::FuncCall(name, args) => Expr::FuncCall(name.clone(), args.iter().map(|arg| go(arg, map)).collect()),
            Expr::Cast(ty, inner) => Expr::Cast(ty.clone(), bx(go(inner, map))),
            Expr::Not(inner) => Expr::Not(bx(go(inner, map))),
            Expr::BitwiseNot(inner) => Expr::BitwiseNot(bx(go(inner, map))),
            Expr::AddressOf(inner) => Expr::AddressOf(bx(go(inner, map))),
            Expr::PostIncrement(inner) => Expr::PostIncrement(bx(go(inner, map))),
            Expr::PostDecrement(inner) => Expr::PostDecrement(bx(go(inner, map))),
            Expr::PreIncrement(inner) => Expr::PreIncrement(bx(go(inner, map))),
            Expr::PreDecrement(inner) => Expr::PreDecrement(bx(go(inner, map))),
            Expr::Literal(_) | Expr::IntLiteral(_) | Expr::Var(_) | Expr::PointerDeref(_) => expr.clone(),
        }
    }
    match s {
        Statement::Assign { target, value, compound } => {
            let value = go(value, f);
            Statement::Assign { target: go(target, f), value, compound: *compound }
        }
        Statement::Expr(e) => Statement::Expr(go(e, f)),
        Statement::If { condition, then_body, else_body, cond_comments } => {
            let condition = go(condition, f);
            let then_body = then_body.iter().map(|x| map_accesses(x, f)).collect();
            let else_body = else_body.iter().map(|x| map_accesses(x, f)).collect();
            Statement::If { condition, then_body, else_body, cond_comments: cond_comments.clone() }
        }
        other => other.clone(),
    }
}

fn mentions_post_increment(s: &Statement, v: &str) -> bool {
    let mut found = false;
    if let Statement::Assign { target, value, .. } = s {
        for e in [target, value] {
            crate::streaming::walk_expr(e, &mut |x| {
                found |= matches!(x, Expr::PostIncrement(i) if matches!(i.as_ref(), Expr::Var(n) if n == v));
            });
        }
    }
    found
}

/// `v`, `v + k` or `v - k`.
fn shifted(v: &str, k: i64) -> Expr {
    match k {
        0 => var(v),
        k if k > 0 => bin(var(v), BinOp::Add, int(k)),
        k => bin(var(v), BinOp::Sub, int(-k)),
    }
}

fn wraps(e: &Expr) -> bool {
    let mut terms = Vec::new();
    flatten(e, true, &mut terms);
    terms.iter().all(|(t, _)| match t {
        Expr::IntLiteral(_) | Expr::Var(_) | Expr::PointerDeref(_) => true,
        Expr::Cast(_, inner) => matches!(inner.as_ref(), Expr::Var(_) | Expr::PointerDeref(_)),
        _ => false,
    })
}

fn flatten<'e>(e: &'e Expr, plus: bool, out: &mut Vec<(&'e Expr, bool)>) {
    match e {
        Expr::BinOp(l, BinOp::Add, r) => {
            flatten(l, plus, out);
            flatten(r, plus, out);
        }
        Expr::BinOp(l, BinOp::Sub, r) => {
            flatten(l, plus, out);
            flatten(r, !plus, out);
        }
        other => out.push((other, plus)),
    }
}

fn substitute(e: &Expr, v: &str, with: &Expr) -> Expr {
    crate::streaming::rewrite_expr(e, &|x| match &x {
        Expr::Var(n) if n == v => with.clone(),
        _ => x,
    })
}

#[cfg(test)]
mod tests {
    use super::*;

    fn v(n: &str) -> Expr {
        var(n)
    }
    fn at(a: &str, i: Expr) -> Expr {
        Expr::ArrayAccess(a.into(), Box::new(i))
    }
    fn set(t: Expr, x: Expr) -> Statement {
        Statement::Assign { target: t, value: x, compound: false }
    }
    fn inc(n: &str) -> Statement {
        step(n, 1)
    }
    fn dec(n: &str) -> Statement {
        step(n, -1)
    }
    fn plus(l: Expr, r: Expr) -> Expr {
        bin(l, BinOp::Add, r)
    }

    /// `i`, `j`, `k`, `n`, `e`, `b` are `usize`; `in`, `in2` inputs of one
    /// length; `out` an output; `ring` the storage of ring `r`.
    fn run_full(cond: &Expr, body: &[Statement], update: Option<&Statement>, same_len: bool) -> Option<Plan> {
        let index = |n: &str| ["i", "j", "k", "n", "e", "b"].contains(&n);
        let usize_expr = |x: &Expr| {
            matches!(x, Expr::IntLiteral(_) | Expr::PointerDeref(_) | Expr::FuncCall(..))
                || matches!(x, Expr::Var(n) if index(n))
        };
        let sliceable = |_: &str| true;
        let pool = |a: &str| same_len && (a == "in" || a == "in2");
        let ring = |id: &str| (id == "r").then(|| "ring".to_string());
        let names =
            Names { index: &index, usize_expr: &usize_expr, sliceable: &sliceable, same_len: &pool, ring_storage: &ring };
        plan(cond, body, update, &names)
    }
    fn run_with(cond: &Expr, body: &[Statement], same_len: bool) -> Option<Plan> {
        run_full(cond, body, None, same_len)
    }
    fn run(cond: &Expr, body: &[Statement]) -> Option<Plan> {
        run_with(cond, body, false)
    }
    fn text(e: &Expr) -> String {
        format!("{e:?}")
    }
    /// Each rewritten access as `array[rendering]`, in order.
    fn accesses(p: &Plan) -> Vec<String> {
        let mut out = Vec::new();
        for s in &p.body {
            each_access(s, &mut |a, i, _| out.push(format!("{a}[{}]", render_marker(i).unwrap_or_else(|| text(i)))));
        }
        out
    }

    #[test]
    fn a_lockstep_loop_reads_and_writes_through_windows() {
        // while( i <= e ) { s += in[i]; out[k] = in[j]; i++; j++; k++; }
        let body = [
            Statement::Assign { target: v("s"), value: plus(v("s"), at("in", v("i"))), compound: true },
            set(at("out", v("k")), at("in", v("j"))),
            inc("i"),
            inc("j"),
            inc("k"),
        ];
        let p = run(&bin(v("i"), BinOp::LessEq, v("e")), &body).expect("windowed");
        assert_eq!(text(&p.trip), text(&plus(bin(v("e"), BinOp::Sub, v("i")), int(1))));
        assert!(!p.reverse);
        let cut: Vec<(String, bool, String)> = p.windows.iter().map(|w| (w.array.clone(), w.mutable, text(&w.start))).collect();
        assert_eq!(
            cut,
            vec![
                ("in".into(), false, text(&v("i"))),
                ("in".into(), false, text(&v("j"))),
                ("out".into(), true, text(&v("k"))),
            ]
        );
        assert_eq!(accesses(&p), ["in[_w0[_wk]]", "in[_w1[_wk]]", "out[_w2[_wk]]"]);
        // The updates stay: each variable still holds C's value.
        assert_eq!(p.body.len(), body.len());
    }

    #[test]
    fn an_access_after_its_step_starts_one_later() {
        // while( i < e ) { i++; x = in[i]; }
        let body = [inc("i"), set(v("x"), at("in", v("i")))];
        let p = run(&bin(v("i"), BinOp::Less, v("e")), &body).expect("windowed");
        assert_eq!(text(&p.windows[0].start), text(&plus(v("i"), int(1))));
    }

    #[test]
    fn a_for_update_is_the_last_statement_of_every_pass() {
        // for( ; i < e; i++ ) { out[k] = in[i]; k++; }
        let body = [set(at("out", v("k")), at("in", v("i"))), inc("k")];
        let p = run_full(&bin(v("i"), BinOp::Less, v("e")), &body, Some(&inc("i")), false).expect("windowed");
        assert_eq!(p.body.len(), 3);
        assert_eq!(format!("{:?}", p.body[2]), format!("{:?}", inc("i")));
    }

    #[test]
    fn a_descending_read_before_its_step_starts_at_the_last_value() {
        // while( i > b ) { x = in[i]; i--; }: reads i0 down to b + 1.
        let body = [set(v("x"), at("in", v("i"))), set(at("out", v("i")), v("x")), dec("i")];
        let p = run(&bin(v("i"), BinOp::Greater, v("b")), &body).expect("windowed");
        let last = bin(v("i"), BinOp::Sub, bin(v(TRIP), BinOp::Sub, int(1)));
        assert!(p.windows.iter().all(|w| text(&w.start) == text(&last)), "{:?}", p.windows.iter().map(|w| text(&w.start)).collect::<Vec<_>>());
    }

    #[test]
    fn a_descending_loop_cuts_from_its_last_pass() {
        // while( i > b ) { i--; x = in[i]; out[i - b] = x; }
        let body = [dec("i"), set(v("x"), at("in", v("i"))), set(at("out", bin(v("i"), BinOp::Sub, v("b"))), v("x"))];
        let p = run(&bin(v("i"), BinOp::Greater, v("b")), &body).expect("windowed");
        assert!(p.reverse);
        assert_eq!(text(&p.trip), text(&bin(v("i"), BinOp::Sub, v("b"))));
        // `i` is `i0 - 1 - pass` at both reads; its last value is `i0 - _wn`.
        let last = bin(v("i"), BinOp::Sub, v(TRIP));
        assert_eq!(text(&p.windows[0].start), text(&last));
        assert_eq!(text(&p.windows[1].start), text(&bin(last, BinOp::Sub, v("b"))));
    }

    #[test]
    fn offsets_of_one_group_share_a_window() {
        // while( i < e ) { out[i] = in[i + 1] - in[i - 1]; i++; }
        let body = [
            set(at("out", v("i")), bin(at("in", plus(v("i"), int(1))), BinOp::Sub, at("in", bin(v("i"), BinOp::Sub, int(1))))),
            inc("i"),
        ];
        let p = run(&bin(v("i"), BinOp::Less, v("e")), &body).expect("windowed");
        let w = p.windows.iter().find(|w| w.array == "in").expect("input window");
        assert_eq!((text(&w.start), w.extra), (text(&bin(v("i"), BinOp::Sub, int(1))), 2));
        assert_eq!(accesses(&p)[..2], ["in[_w0[_wk + 2]]", "in[_w0[_wk]]"]);
    }

    #[test]
    fn a_conditional_read_outside_every_pass_extent_makes_the_windows_checked() {
        // while( i < e ) { x = in[i]; y = c ? in[i + 1] : 0; out[i] = x + y; i++; }
        let body = [
            set(v("x"), at("in", v("i"))),
            set(v("y"), Expr::Ternary(Box::new(v("c")), Box::new(at("in", plus(v("i"), int(1)))), Box::new(int(0)))),
            set(at("out", v("i")), plus(v("x"), v("y"))),
            inc("i"),
        ];
        let p = run(&bin(v("i"), BinOp::Less, v("e")), &body).expect("windowed");
        assert!(p.checked);
        assert_eq!(accesses(&p)[..2], ["in[_w0[_wk]]", "in[_w0[_wk + 1]]"]);
        // Within the extent, a conditional read is windowed too, unchecked.
        let body2 = [
            set(v("x"), at("in", plus(v("i"), int(1)))),
            set(v("y"), Expr::Ternary(Box::new(v("c")), Box::new(at("in", v("i"))), Box::new(int(0)))),
            set(v("z"), at("in", v("i"))),
            set(at("out", v("i")), plus(v("x"), v("y"))),
            inc("i"),
        ];
        let p2 = run(&bin(v("i"), BinOp::Less, v("e")), &body2).expect("windowed");
        assert!(!p2.checked);
        assert_eq!(accesses(&p2)[1], "in[_w0[_wk]]");
    }

    #[test]
    fn a_window_with_no_every_pass_access_is_checked_and_starts_where_any_access_does() {
        // while( i < e ) { x = c ? in[i - 2] : 0; out[i] = x; i++; }
        let body = [
            set(v("x"), Expr::Ternary(Box::new(v("c")), Box::new(at("in", bin(v("i"), BinOp::Sub, int(2)))), Box::new(int(0)))),
            set(at("out", v("i")), v("x")),
            inc("i"),
        ];
        let p = run(&bin(v("i"), BinOp::Less, v("e")), &body).expect("windowed");
        assert!(p.checked);
        let w = p.windows.iter().find(|w| w.array == "in").expect("in windows");
        assert_eq!(text(&w.start), text(&bin(v("i"), BinOp::Sub, int(2))));
    }

    #[test]
    fn one_input_bounds_another_of_its_length() {
        // while( i < e ) { x = in[i]; y = c ? in2[i] : 0; out[k] = x + y; i++; k++; }
        let body = [
            set(v("x"), at("in", v("i"))),
            set(v("y"), Expr::Ternary(Box::new(v("c")), Box::new(at("in2", v("i"))), Box::new(int(0)))),
            set(at("out", v("k")), plus(v("x"), v("y"))),
            inc("i"),
            inc("k"),
        ];
        let cond = bin(v("i"), BinOp::Less, v("e"));
        let pooled = run_with(&cond, &body, true).unwrap();
        assert!(accesses(&pooled)[1].starts_with("in2[_w") && !pooled.checked);
        let apart = run_with(&cond, &body, false).unwrap();
        assert!(accesses(&apart)[1].starts_with("in2[_w") && apart.checked);
    }

    #[test]
    fn an_array_stored_to_takes_one_window_or_none() {
        let lt = bin(v("i"), BinOp::Less, v("e"));
        // out[i] and out[k] are two groups: neither may be a window.
        let body = [set(at("out", v("i")), at("in", v("j"))), set(at("out", v("k")), int(0)), inc("i"), inc("j"), inc("k")];
        let p = run(&lt, &body).expect("the input still windows");
        assert!(p.windows.iter().all(|w| w.array != "out"));
        assert!(accesses(&p).iter().all(|a| !a.starts_with("out[_w")));
        // out[i + 5], read only conditionally: one checked window holds both.
        let beyond = [
            set(at("out", v("i")), at("in", v("i"))),
            set(v("y"), Expr::Ternary(Box::new(v("c")), Box::new(at("out", plus(v("i"), int(5)))), Box::new(int(0)))),
            inc("i"),
        ];
        let p = run(&lt, &beyond).expect("windowed");
        let outs: Vec<i64> = p.windows.iter().filter(|w| w.array == "out").map(|w| w.extra).collect();
        assert!(p.checked && outs == [5], "{outs:?}");
    }

    #[test]
    fn reads_the_counter_already_bounds_are_left_alone() {
        // while( i < e ) { s += in[i]; i++; }, `in` resliced to the range.
        let sum = |i: Expr| Statement::Assign { target: v("s"), value: plus(v("s"), at("in", i)), compound: true };
        let cond = bin(v("i"), BinOp::Less, v("e"));
        assert!(run_with(&cond, &[sum(v("i")), inc("i")], true).is_none());
        assert!(run_with(&cond, &[sum(v("i")), inc("i")], false).is_some());
        // One past the counter is not bounded by the test on it.
        assert!(run_with(&cond, &[sum(plus(v("i"), int(1))), inc("i")], true).is_some());
    }

    #[test]
    fn an_index_that_is_not_one_variable_plus_invariants_keeps_its_check() {
        let lt = bin(v("i"), BinOp::Less, v("e"));
        // in[i + k] moves by two per pass.
        let two = [set(at("out", v("k")), at("in", plus(v("i"), v("k")))), inc("i"), inc("k")];
        assert!(accesses(&run(&lt, &two).expect("out windows"))[0].starts_with("in[BinOp"));
        // in[j] descends in an ascending loop.
        let down = [set(at("out", v("k")), at("in", v("j"))), inc("i"), inc("k"), dec("j")];
        let p = run(&lt, &down).expect("out windows");
        assert!(!p.reverse && p.windows.iter().all(|w| w.array != "in"));
        // in[b + i] and in[i - b] are two elements, not one.
        let signs = [
            set(v("x"), at("in", plus(v("b"), v("i")))),
            set(v("y"), at("in", bin(v("i"), BinOp::Sub, v("b")))),
            set(at("out", v("i")), plus(v("x"), v("y"))),
            inc("i"),
        ];
        let a = accesses(&run(&lt, &signs).expect("windowed"));
        assert_ne!(a[0], a[1], "{a:?}");
    }

    #[test]
    fn a_short_circuit_operand_widens_a_window_only_checked() {
        // while( i < e ) { x = in[i]; y = c && in[i + 1]; out[i] = x + y; i++; }
        let body = [
            set(v("x"), at("in", v("i"))),
            set(v("y"), bin(v("c"), BinOp::And, at("in", plus(v("i"), int(1))))),
            set(at("out", v("i")), plus(v("x"), v("y"))),
            inc("i"),
        ];
        let p = run(&bin(v("i"), BinOp::Less, v("e")), &body).expect("windowed");
        assert_eq!(p.windows.iter().find(|w| w.array == "in").map(|w| w.extra), Some(1));
        assert!(p.checked);
    }

    #[test]
    fn a_ring_the_loop_advances_is_not_windowed() {
        // while( i < e ) { ring[i] = in[i]; CIRCBUF_NEXT(r); i++; }: the wrap reads ring.len().
        let body = [set(at("ring", v("i")), at("in", v("i"))), Statement::CircBuf(CircBuf::Next { id: "r".into() }), inc("i")];
        let p = run(&bin(v("i"), BinOp::Less, v("e")), &body).expect("in windows");
        assert!(p.windows.iter().all(|w| w.array != "ring"));
    }

    #[test]
    fn a_post_increment_index_leaves_its_step_behind() {
        // while( i < e ) { out[k++] = in[i]; i++; }
        let body = [set(at("out", Expr::PostIncrement(Box::new(v("k")))), at("in", v("i"))), inc("i")];
        let p = run(&bin(v("i"), BinOp::Less, v("e")), &body).expect("windowed");
        assert_eq!(p.body.len(), 3);
        assert_eq!(format!("{:?}", p.body[1]), format!("{:?}", inc("k")));
        assert_eq!(accesses(&p)[1], "out[_w1[_wk]]");
    }

    #[test]
    fn condition_steps_run_every_pass_and_once_more() {
        let body = [set(at("out", v("k")), int(0)), inc("k")];
        // `n-- > 0`: `n` passes, each opening with the decrement.
        let p = run(&bin(Expr::PostDecrement(Box::new(v("n"))), BinOp::Greater, int(0)), &body).expect("windowed");
        assert_eq!((text(&p.trip), p.cond_step.clone(), p.may_pass_unguarded), (text(&v("n")), Some("n".into()), false));
        assert_eq!(format!("{:?}", p.body[0]), format!("{:?}", dec("n")));
        // `--n != 0` can pass on a wrapped `n`, so its unguarded path is the loop.
        let p = run(&bin(Expr::PreDecrement(Box::new(v("n"))), BinOp::NotEq, int(0)), &body).expect("windowed");
        assert_eq!(text(&p.trip), text(&bin(v("n"), BinOp::Sub, int(1))));
        assert!(p.may_pass_unguarded);
        // A body moving the counter as well has no fixed trip count.
        let twice = [set(at("out", v("k")), int(0)), inc("k"), dec("n")];
        assert!(run(&bin(Expr::PostDecrement(Box::new(v("n"))), BinOp::Greater, int(0)), &twice).is_none());
    }

    #[test]
    fn loops_without_a_fixed_trip_count_stay_as_written() {
        let body = [set(at("out", v("k")), at("in", v("i"))), inc("i"), inc("k")];
        let lt = bin(v("i"), BinOp::Less, v("e"));
        assert!(run(&lt, &body).is_some(), "the control must window");
        // The counter stepped twice, stepped the wrong way, or stepped and stored.
        assert!(run(&lt, &[body[0].clone(), inc("i"), inc("i"), inc("k")]).is_none());
        assert!(run(&lt, &[body[0].clone(), dec("i"), inc("k")]).is_none());
        assert!(run(&lt, &[body[0].clone(), inc("i"), inc("k"), set(v("i"), v("j"))]).is_none());
        // An index stepped and stored keeps its check.
        let p = run(&lt, &[body[0].clone(), inc("i"), inc("k"), set(v("k"), v("j"))]).expect("in windows");
        assert!(p.windows.iter().all(|w| w.array != "out"));
        // A bound that moves, by a step, a store through a pointer, or a call.
        assert!(run(&lt, &[body[0].clone(), inc("i"), inc("k"), inc("e")]).is_none());
        let deref = Expr::PointerDeref("n".into());
        let stored = [body[0].clone(), inc("i"), inc("k"), set(deref.clone(), v("j"))];
        assert!(run(&bin(v("i"), BinOp::Less, deref), &stored).is_none());
        let call = Expr::FuncCall("f".into(), vec![v("e")]);
        assert!(run(&bin(v("i"), BinOp::Less, call), &body).is_none());
        // `>=` would step a `usize` bound of 0 past zero.
        assert!(run(&bin(v("i"), BinOp::GreaterEq, v("b")), &[body[0].clone(), dec("i"), inc("k")]).is_none());
    }

    fn branch(c: Expr, then_body: Vec<Statement>, else_body: Vec<Statement>) -> Statement {
        Statement::If { condition: c, then_body, else_body, cond_comments: vec![] }
    }

    #[test]
    fn a_branch_is_windowed_but_its_arms_keep_their_checks() {
        // while( i < e ) { if( in[i] > 0 && in[j] > 0 ) out[k] = in[j]; else out[k] = 0; k++; i++; j++; }
        let body = [
            branch(
                bin(bin(at("in", v("i")), BinOp::Greater, int(0)), BinOp::And, bin(at("in", v("j")), BinOp::Greater, int(0))),
                vec![set(at("out", v("k")), at("in", v("j")))],
                vec![set(at("out", v("k")), int(0))],
            ),
            inc("k"),
            inc("i"),
            inc("j"),
        ];
        let p = run_with(&bin(v("i"), BinOp::Less, v("e")), &body, true).expect("windowed");
        let a = accesses(&p);
        assert!(a[0].starts_with("in[_w") && a[1].starts_with("in[_w"), "{a:?}");
        assert!(a[2].starts_with("in[Var") && a[3].starts_with("out[Var") && a[4].starts_with("out[Var"), "{a:?}");
        assert!(p.checked, "in[j] is read only past the `&&`");
        // Read only in an arm, it is no reason to window the loop.
        let arm_only = [
            branch(
                bin(at("in", v("i")), BinOp::Greater, int(0)),
                vec![set(at("out", v("k")), at("in", v("j")))],
                vec![set(at("out", v("k")), int(0))],
            ),
            inc("k"),
            inc("i"),
            inc("j"),
        ];
        assert!(run_with(&bin(v("i"), BinOp::Less, v("e")), &arm_only, true).is_none());
    }

    #[test]
    fn a_branch_whose_windows_need_no_check_stays_as_written() {
        // while( i < e ) { x = in[j]; if( x > 0 ) n++; else n = 0; out[i] = x; i++; j++; }
        let body = [
            set(v("x"), at("in", v("j"))),
            branch(bin(v("x"), BinOp::Greater, int(0)), vec![inc("n")], vec![set(v("n"), int(0))]),
            set(at("out", v("i")), v("x")),
            inc("i"),
            inc("j"),
        ];
        assert!(run(&bin(v("i"), BinOp::Less, v("e")), &body).is_none());
        // Without the branch the same reads are windowed.
        let straight = [body[0].clone(), body[2].clone(), body[3].clone(), body[4].clone()];
        assert!(run(&bin(v("i"), BinOp::Less, v("e")), &straight).is_some());
    }

    #[test]
    fn a_branch_reading_only_at_or_behind_the_counter_stays_as_written() {
        // while( i < e ) { if( in[i] > in[i - 1] && c > in[i - 2] ) out[k] = 1; else out[k] = 0; k++; i++; }
        let at_i = |d: i64| at("in", if d == 0 { v("i") } else if d > 0 { plus(v("i"), int(d)) } else { bin(v("i"), BinOp::Sub, int(-d)) });
        let body = |far: i64| {
            [
                branch(
                    bin(bin(at_i(0), BinOp::Greater, at_i(-1)), BinOp::And, bin(v("c"), BinOp::Greater, at_i(far))),
                    vec![set(at("out", v("k")), int(1))],
                    vec![set(at("out", v("k")), int(0))],
                ),
                inc("k"),
                inc("i"),
            ]
        };
        let lt = bin(v("i"), BinOp::Less, v("e"));
        assert!(run_with(&lt, &body(-2), true).is_none());
        // One bar ahead is not bounded by the test on the counter.
        assert!(run_with(&lt, &body(1), true).is_some());
    }

    #[test]
    fn a_short_countdown_in_a_pass_is_unrolled() {
        // while( i < e ) { for( t = 2; t >= 0; t-- ) x += in[i - t]; out[i] = x; i++; }
        let countdown = Statement::ForC {
            init: Box::new(set(v("t"), int(2))),
            condition: bin(v("t"), BinOp::GreaterEq, int(0)),
            update: Box::new(dec("t")),
            body: vec![Statement::Assign { target: v("x"), value: at("in", bin(v("i"), BinOp::Sub, v("t"))), compound: true }],
        };
        let body = [countdown, set(at("out", v("i")), v("x")), inc("i")];
        let p = run(&bin(v("i"), BinOp::Less, v("e")), &body).expect("windowed");
        assert_eq!(accesses(&p)[..3], ["in[_w0[_wk]]", "in[_w0[_wk + 1]]", "in[_w0[_wk + 2]]"]);
        assert_eq!(format!("{:?}", p.body[3]), format!("{:?}", set(v("t"), int(0))));
        // A countdown that moves its own counter stays a loop, and so the pass.
        let Statement::ForC { init, condition, update, body: mut inner } = body[0].clone() else { unreachable!() };
        inner.push(dec("t"));
        let moved = [Statement::ForC { init, condition, update, body: inner }, body[1].clone(), body[2].clone()];
        assert!(run(&bin(v("i"), BinOp::Less, v("e")), &moved).is_none());
    }

    fn countdown(v: &str, from: i64, body: Vec<Statement>) -> Statement {
        Statement::ForC {
            init: Box::new(set(var(v), int(from))),
            condition: bin(var(v), BinOp::GreaterEq, int(0)),
            update: Box::new(dec(v)),
            body,
        }
    }

    #[test]
    fn a_countdown_nested_in_a_countdown_substitutes_both_counters() {
        // while( i < e ) { for( a = 1; a >= 0; a-- ) for( b = 1; b >= 0; b-- ) x += in[i - a - b]; out[i] = x; i++; }
        let read = Statement::Assign {
            target: v("x"),
            value: at("in", bin(bin(v("i"), BinOp::Sub, v("a")), BinOp::Sub, v("b"))),
            compound: true,
        };
        let body = [countdown("a", 1, vec![countdown("b", 1, vec![read])]), set(at("out", v("i")), v("x")), inc("i")];
        let p = run(&bin(v("i"), BinOp::Less, v("e")), &body).expect("windowed");
        assert_eq!(accesses(&p)[..4], ["in[_w0[_wk]]", "in[_w0[_wk + 1]]", "in[_w0[_wk + 1]]", "in[_w0[_wk + 2]]"]);
    }

    #[test]
    fn a_checked_start_that_could_fault_is_not_cut() {
        // while( i < e ) { x = in[i]; if( d != 0 ) y = in[i + n / d]; out[i] = x; i++; }
        let body = [
            set(v("x"), at("in", v("i"))),
            branch(
                bin(v("d"), BinOp::NotEq, int(0)),
                vec![set(v("y"), at("in", plus(v("i"), bin(v("n"), BinOp::Div, v("d")))))],
                vec![],
            ),
            set(at("out", v("i")), v("x")),
            inc("i"),
        ];
        assert!(run(&bin(v("i"), BinOp::Less, v("e")), &body).is_none());
    }

    #[test]
    fn a_step_taken_in_an_arm_is_no_induction() {
        // while( i < e ) { if( c && in[j] > 0 ) k++; out[k] = in[i]; i++; j++; }
        let body = [
            branch(bin(v("c"), BinOp::And, bin(at("in", v("j")), BinOp::Greater, int(0))), vec![inc("k")], vec![]),
            set(at("out", v("k")), at("in", v("i"))),
            inc("i"),
            inc("j"),
        ];
        let p = run(&bin(v("i"), BinOp::Less, v("e")), &body).expect("in windows");
        assert!(p.windows.iter().all(|w| w.array != "out"));
        // Nor may the counter step in one.
        let counter = [branch(v("c"), vec![inc("i")], vec![]), set(at("out", v("k")), at("in", v("i"))), inc("i"), inc("k")];
        assert!(run(&bin(v("i"), BinOp::Less, v("e")), &counter).is_none());
    }

    #[test]
    fn a_body_naming_an_emitted_name_stays_as_written() {
        let lt = bin(v("i"), BinOp::Less, v("e"));
        for name in ["_wn", "_wk", "_w0", "_w12"] {
            let body = [set(at("out", v("k")), v(name)), inc("i"), inc("k")];
            assert!(run(&lt, &body).is_none(), "{name}");
        }
        let body = [set(at("out", v("k")), v("_windowStart")), inc("i"), inc("k")];
        assert!(run(&lt, &body).is_some());
    }
}
