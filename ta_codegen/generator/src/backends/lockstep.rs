//! The lockstep counted loop, as the batch emitters read it: a header whose
//! trip count is fixed at entry, the variables that move by one once per
//! pass, and how often each element access is made. Shared by the windows
//! Rust and C# render ([`super::window`]) and the Go re-slices
//! ([`super::go_lockstep`]); each backend keeps its own policy on what to cut.

use std::collections::{BTreeMap, BTreeSet};

use super::builtins::{MathFn, SpecialBuiltin};
use super::rust_respell::SELECT_OTHER;
use crate::ir::{BinOp, CircBuf, Expr, Statement, VarType};

/// What the renderer knows about names that the IR does not say. Only
/// `index`, `usize_expr` and `castable` reach [`form`]; the rest is
/// [`super::window`]'s.
pub(crate) struct Names<'a> {
    /// A local or parameter a counter may be: Rust's `usize`s.
    pub index: &'a dyn Fn(&str) -> bool,
    /// An expression a bound may be as written: Rust's `usize`s.
    pub usize_expr: &'a dyn Fn(&Expr) -> bool,
    /// An `i32` a bound may take through the `as usize` cast the loop as
    /// written already applies, so the trip and the guard agree.
    pub castable: &'a dyn Fn(&Expr) -> bool,
    /// An array a window may be cut from.
    pub sliceable: &'a dyn Fn(&str) -> bool,
    /// One of the inputs that all have one length here.
    pub same_len: &'a dyn Fn(&str) -> bool,
    /// The storage a CIRCBUF's wrap reads the length of, by ring id.
    pub ring_storage: &'a dyn Fn(&str) -> Option<String>,
}

pub(crate) fn step(v: &str, d: i64) -> Statement {
    let op = if d > 0 { BinOp::Add } else { BinOp::Sub };
    Statement::Assign { target: var(v), value: bin(var(v), op, int(1)), compound: true }
}
pub(crate) fn var(n: &str) -> Expr {
    Expr::Var(n.to_string())
}
pub(crate) fn int(n: i64) -> Expr {
    Expr::IntLiteral(n)
}
pub(crate) fn bin(l: Expr, op: BinOp, r: Expr) -> Expr {
    Expr::BinOp(Box::new(l), op, Box::new(r))
}

pub(crate) struct Form {
    pub counter: String,
    /// The counter's move per pass.
    pub step: i64,
    pub bound: Expr,
    pub guard: Expr,
    pub trip: Expr,
    pub cond_step: Option<String>,
    pub may_pass_unguarded: bool,
}

pub(crate) fn form(cond: &Expr, names: &Names) -> Option<Form> {
    let Expr::BinOp(lhs, op, rhs) = cond else { return None };
    let bound = rhs.as_ref().clone();
    let bound = if (names.usize_expr)(&bound) {
        bound
    } else if (names.castable)(&bound) {
        Expr::Cast(VarType::Index, Box::new(bound))
    } else {
        return None;
    };
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

/// A call that evaluates each argument exactly once, or whose every argument
/// is safe to evaluate on every pass.
pub(crate) fn eager_call(name: &str) -> bool {
    MathFn::from_name(name).is_some()
        || name == SELECT_OTHER
        || super::eager_select::is_pseudo_call(name)
        || matches!(
            SpecialBuiltin::from_name(name),
            Some(
                SpecialBuiltin::IsZero
                    | SpecialBuiltin::IsZeroScaled
                    | SpecialBuiltin::IsZeroOrNeg
                    | SpecialBuiltin::IsFinite
                    | SpecialBuiltin::Unlikely
            )
        )
}

/// Whether the passes `code` holds may run several to a trip: every statement
/// an assignment or a branch, and every call evaluated eagerly and, for a math
/// function, one `inline` says the backend's compiler keeps in the loop.
/// `Some(true)` when the pass is one store of an expression to an element.
/// Values stay exact under any aliasing only while the copies run in
/// increasing index, each statement whole before the next: interleaving the
/// copies, or hoisting a load above an earlier copy's store, changes them
/// with no error.
pub(crate) fn unrollable(code: &[&Statement], inline: &dyn Fn(MathFn) -> bool) -> Option<bool> {
    fn plain(list: &[Statement]) -> bool {
        list.iter().all(|s| match s {
            Statement::Assign { target, .. } => !matches!(target, Expr::PointerDeref(_)),
            Statement::If { then_body, else_body, .. } => plain(then_body) && plain(else_body),
            Statement::Comment(_) | Statement::UnrollHint { .. } => true,
            _ => false,
        })
    }
    let mut ok = code.iter().all(|s| plain(std::slice::from_ref(*s)));
    for s in code {
        crate::streaming::walk_stmt_exprs(s, &mut |top| {
            crate::streaming::walk_expr(top, &mut |e| {
                if let Expr::FuncCall(n, _) = e {
                    ok &= eager_call(n) && MathFn::from_name(n).is_none_or(inline);
                }
            });
        });
    }
    let code: Vec<&&Statement> = code.iter().filter(|s| !matches!(s, Statement::Comment(_) | Statement::UnrollHint { .. })).collect();
    ok.then_some(matches!(code.as_slice(), [Statement::Assign { target: Expr::ArrayAccess(..), compound: false, .. }]))
}

/// `v += 1`, `v = v - 1`, ...: the variable and its step.
pub(crate) fn as_step(stmt: &Statement) -> Option<(&str, i64)> {
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
pub(crate) fn trailing_step(s: &Statement) -> Option<&str> {
    let Statement::Assign { target, value, compound: false } = s else { return None };
    let v = post_increment(target).or_else(|| post_increment(value))?;
    let mut uses = 0;
    for e in [target, value] {
        crate::streaming::walk_expr(e, &mut |x| uses += usize::from(matches!(x, Expr::Var(n) if n == v)));
    }
    (uses == 1).then_some(v)
}

pub(crate) fn post_increment(e: &Expr) -> Option<&str> {
    let Expr::ArrayAccess(_, i) = e else { return None };
    let Expr::PostIncrement(v) = i.as_ref() else { return None };
    let Expr::Var(v) = v.as_ref() else { return None };
    Some(v)
}

pub(crate) fn step_of(s: &Statement) -> Option<(&str, i64)> {
    as_step(s).or_else(|| trailing_step(s).map(|v| (v, 1)))
}

#[derive(Default)]
pub(crate) struct Facts {
    /// Names stored to other than as an element.
    pub assigned: BTreeSet<String>,
    /// Names stored to by anything but a step.
    pub other: BTreeSet<String>,
    /// Per variable, its steps in pass order.
    pub steps: BTreeMap<String, Vec<i64>>,
    /// Arrays with an element stored.
    pub written: BTreeSet<String>,
    /// Names used other than as the array of an element access.
    pub bare: BTreeSet<String>,
}

impl Facts {
    pub(crate) fn of(pass: &[Statement]) -> Facts {
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
                self.arm(&[then_body.as_slice(), else_body.as_slice()].concat());
            }
            Statement::Block { body } => {
                for s in body {
                    self.statement(s);
                }
            }
            Statement::VarDecl { name, init, .. } => {
                self.store(name);
                if let Some(init) = init {
                    self.expr(init, None);
                }
            }
            // What a nested loop or a switch stores moves on some passes only.
            Statement::While { condition, body } | Statement::DoWhile { condition, body } => {
                self.expr(condition, None);
                self.arm(body);
            }
            Statement::ForC { init, condition, update, body } => {
                self.expr(condition, None);
                let mut all = vec![init.as_ref().clone(), update.as_ref().clone()];
                all.extend(body.iter().cloned());
                self.arm(&all);
            }
            Statement::For { var, count, body } => {
                self.store(var);
                self.expr(count, None);
                self.arm(body);
            }
            Statement::Switch { expr, cases, default } => {
                self.expr(expr, None);
                let mut all: Vec<Statement> = cases.iter().flat_map(|(_, b)| b.iter().cloned()).collect();
                all.extend(default.iter().cloned());
                self.arm(&all);
            }
            _ => {}
        }
    }

    fn arm(&mut self, body: &[Statement]) {
        let arms = Facts::of(body);
        self.other.extend(arms.assigned.iter().cloned());
        self.assigned.extend(arms.assigned);
        self.other.extend(arms.other);
        self.written.extend(arms.written);
        self.bare.extend(arms.bare);
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
    pub(crate) fn induction(&self) -> BTreeMap<String, i64> {
        self.steps
            .iter()
            .filter(|(v, s)| s.len() == 1 && !self.other.contains(*v))
            .map(|(v, s)| (v.clone(), s[0]))
            .collect()
    }
}

/// Side-effect free, and nothing it reads changes within the loop.
pub(crate) fn invariant(e: &Expr, facts: &Facts) -> bool {
    match e {
        Expr::IntLiteral(_) => true,
        Expr::Var(n) | Expr::PointerDeref(n) => !facts.assigned.contains(n) && !n.contains('.'),
        Expr::BinOp(l, _, r) => invariant(l, facts) && invariant(r, facts),
        Expr::Cast(_, i) | Expr::Neg(i) => invariant(i, facts),
        _ => false,
    }
}

/// Calls `f(array, index, every)` for each element access in `s`, in
/// evaluation order.
pub(crate) fn each_access(s: &Statement, f: &mut dyn FnMut(&str, &Expr, Reach)) {
    each_access_in(s, Reach::Every, f);
}

/// How often an access is made.
#[derive(Clone, Copy, PartialEq, Eq, Debug)]
pub(crate) enum Reach {
    Every,
    /// Under `?:`, `&&`, `||` or a call that may skip an argument.
    Some,
    /// In a branch's arm, where it keeps its check: a checked access there is
    /// what keeps LLVM from re-forming the branch, and reads skipped this way
    /// measured no gain.
    Arm,
}

pub(crate) fn each_access_in(s: &Statement, reach: Reach, f: &mut dyn FnMut(&str, &Expr, Reach)) {
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
            | Expr::Neg(inner)
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

pub(crate) fn flatten<'e>(e: &'e Expr, plus: bool, out: &mut Vec<(&'e Expr, bool)>) {
    match e {
        Expr::BinOp(l, BinOp::Add, r) => {
            flatten(l, plus, out);
            flatten(r, plus, out);
        }
        Expr::BinOp(l, BinOp::Sub, r) => {
            flatten(l, plus, out);
            flatten(r, !plus, out);
        }
        Expr::Neg(x) => flatten(x, !plus, out),
        other => out.push((other, plus)),
    }
}

/// `(v, invariant terms, c)` for an index `inv + v + c`, `v` one of `steps`;
/// the invariant terms with their signs, sorted, as `Debug` text.
pub(crate) fn linear(idx: &Expr, steps: &BTreeMap<String, i64>, facts: &Facts) -> Option<(String, Vec<String>, i64)> {
    let mut terms = Vec::new();
    flatten(idx, true, &mut terms);
    let mut var = None;
    let mut inv = Vec::new();
    let mut c = 0i64;
    for (e, plus) in terms {
        match e {
            Expr::IntLiteral(n) => c += if plus { *n } else { -*n },
            Expr::Var(n) if steps.contains_key(n) => {
                if !plus || var.replace(n.clone()).is_some() {
                    return None;
                }
            }
            other if invariant(other, facts) => inv.push(format!("{}{other:?}", if plus { '+' } else { '-' })),
            _ => return None,
        }
    }
    inv.sort();
    Some((var?, inv, c))
}

/// `v`, `v + k` or `v - k`.
pub(crate) fn shifted(v: &str, k: i64) -> Expr {
    match k {
        0 => var(v),
        k if k > 0 => bin(var(v), BinOp::Add, int(k)),
        k => bin(var(v), BinOp::Sub, int(-k)),
    }
}

pub(crate) fn substitute(e: &Expr, v: &str, with: &Expr) -> Expr {
    crate::streaming::rewrite_expr(e, &|x| match &x {
        Expr::Var(n) if n == v => with.clone(),
        _ => x,
    })
}

/// `s` with blocks inlined and each step it takes at a place every
/// evaluation reaches moved into a statement of its own: a pre-step before
/// it, a post-step after. Exact when the statement names the variable nowhere
/// else, which is also what the renderer requires of a step. A step both arms
/// of an `if` take last moves after it.
pub(crate) fn inline(s: &Statement, out: &mut Vec<Statement>) {
    match s {
        Statement::If { condition, then_body, else_body, cond_comments } if !else_body.is_empty() => {
            let flat = |b: &[Statement]| {
                let mut v = Vec::new();
                for x in b {
                    inline(x, &mut v);
                }
                v
            };
            let last = |v: &[Statement]| v.iter().rposition(|x| !matches!(x, Statement::Comment(_))).filter(|&at| as_step(&v[at]).is_some());
            let (mut then2, mut else2) = (flat(then_body), flat(else_body));
            match (last(&then2), last(&else2)) {
                (Some(a), Some(b)) if as_step(&then2[a]) == as_step(&else2[b]) => {
                    let moved = then2.remove(a);
                    else2.remove(b);
                    out.push(Statement::If { condition: condition.clone(), then_body: then2, else_body: else2, cond_comments: cond_comments.clone() });
                    out.push(moved);
                }
                _ => out.push(s.clone()),
            }
        }
        Statement::Block { body } => {
            for x in body {
                inline(x, out);
            }
        }
        Statement::Expr(Expr::PostIncrement(v) | Expr::PreIncrement(v)) if matches!(v.as_ref(), Expr::Var(_)) => {
            let Expr::Var(n) = v.as_ref() else { unreachable!() };
            out.push(step(n, 1));
        }
        Statement::Expr(Expr::PostDecrement(v) | Expr::PreDecrement(v)) if matches!(v.as_ref(), Expr::Var(_)) => {
            let Expr::Var(n) = v.as_ref() else { unreachable!() };
            out.push(step(n, -1));
        }
        Statement::Assign { target, value, compound } if as_step(s).is_none() => {
            let (mut pre, mut post) = (Vec::new(), Vec::new());
            let value2 = eager(value, &mut pre, &mut post);
            let target2 = eager(target, &mut pre, &mut post);
            if pre.is_empty() && post.is_empty() || !named_once(&[target, value], &pre, &post) {
                out.push(s.clone());
                return;
            }
            out.extend(pre);
            out.push(Statement::Assign { target: target2, value: value2, compound: *compound });
            out.extend(post);
        }
        Statement::Expr(e) => {
            let (mut pre, mut post) = (Vec::new(), Vec::new());
            let e2 = eager(e, &mut pre, &mut post);
            if pre.is_empty() && post.is_empty() || !named_once(&[e], &pre, &post) {
                out.push(s.clone());
                return;
            }
            out.extend(pre);
            out.push(Statement::Expr(e2));
            out.extend(post);
        }
        other => out.push(other.clone()),
    }
}

/// Each variable stepped is stepped once and named nowhere else in `exprs`.
fn named_once(exprs: &[&Expr], pre: &[Statement], post: &[Statement]) -> bool {
    let stepped: Vec<&str> = pre.iter().chain(post).filter_map(|s| as_step(s).map(|(v, _)| v)).collect();
    stepped.iter().all(|v| {
        let mut n = 0;
        for e in exprs {
            crate::streaming::walk_expr(e, &mut |x| n += usize::from(matches!(x, Expr::Var(m) if m == v)));
        }
        n == 1
    })
}

/// `e` with each step every evaluation of it takes lifted into `pre` / `post`.
fn eager(e: &Expr, pre: &mut Vec<Statement>, post: &mut Vec<Statement>) -> Expr {
    let bx = Box::new;
    match e {
        Expr::PostIncrement(i) | Expr::PostDecrement(i) | Expr::PreIncrement(i) | Expr::PreDecrement(i) => {
            let Expr::Var(v) = i.as_ref() else { return e.clone() };
            let d = if matches!(e, Expr::PostIncrement(_) | Expr::PreIncrement(_)) { 1 } else { -1 };
            if matches!(e, Expr::PostIncrement(_) | Expr::PostDecrement(_)) {
                post.push(step(v, d));
            } else {
                pre.push(step(v, d));
            }
            Expr::Var(v.clone())
        }
        Expr::ArrayAccess(a, i) => Expr::ArrayAccess(a.clone(), bx(eager(i, pre, post))),
        Expr::BinOp(l, op @ (BinOp::And | BinOp::Or), r) => Expr::BinOp(bx(eager(l, pre, post)), op.clone(), r.clone()),
        Expr::BinOp(l, op, r) => {
            let l = eager(l, pre, post);
            Expr::BinOp(bx(l), op.clone(), bx(eager(r, pre, post)))
        }
        Expr::Ternary(c, t, f) => Expr::Ternary(bx(eager(c, pre, post)), t.clone(), f.clone()),
        Expr::FuncCall(n, args) if eager_call(n) => Expr::FuncCall(n.clone(), args.iter().map(|a| eager(a, pre, post)).collect()),
        Expr::Cast(t, i) => Expr::Cast(t.clone(), bx(eager(i, pre, post))),
        Expr::Not(i) => Expr::Not(bx(eager(i, pre, post))),
        Expr::Neg(i) => Expr::Neg(bx(eager(i, pre, post))),
        Expr::BitwiseNot(i) => Expr::BitwiseNot(bx(eager(i, pre, post))),
        _ => e.clone(),
    }
}
