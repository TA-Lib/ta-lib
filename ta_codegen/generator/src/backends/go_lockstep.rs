//! Two loop shapes Go lowers differently from the C it transcribes.
//!
//! The lockstep counted loop ([`plan`]): each contract or scratch array the
//! loop reads or writes at `v + c`, `v` an index that moves by one on every
//! pass, is re-sliced once to what the loop touches and indexed by one fresh
//! `int` counter, which is what lets gc's prove drop the bounds checks. The index
//! steps leave the loop: inside it a read of `v` is `v + int32(k)`, and after
//! it `v` takes its last value only where something reads it.
//!
//! The padded constant-cap loop ([`capped`]): `for (v = a; v < K; v++) if (v <
//! B) S` runs `S` for `v` below `min(K, B)` and nothing else, so it stops
//! there.

use std::collections::{BTreeMap, BTreeSet, HashMap, HashSet};

use super::builtins::MathFn;
use super::lockstep::{as_step, each_access, flatten, form, inline, invariant, linear, shifted, substitute, unrollable, Facts, Names, Reach};
use crate::ir::{BinOp, CircBuf, CircBufLayout, Expr, Statement};

#[derive(Clone, Copy, PartialEq, Eq, Debug)]
pub(super) enum Contract {
    Input,
    Output,
    /// Allocated by the body itself and never used as a ring: cut only through
    /// an element some access reaches on every pass.
    Scratch,
}

/// What the Go renderer knows that the IR does not say.
pub(super) struct Env<'a> {
    pub int_local: &'a dyn Fn(&str) -> bool,
    /// An input or output of the batch tier, never reassigned in the body
    /// (the public tier checked its length against the index range), or a
    /// [`scratch`] array.
    pub contract: &'a dyn Fn(&str) -> Option<Contract>,
    /// `endIdx` itself, never assigned in the body.
    pub fixed_end: &'a dyn Fn(&Expr) -> bool,
    pub cross_call: &'a dyn Fn(&str) -> bool,
    /// An expression of C integer type: a real bound would run a different
    /// number of passes once converted to the `int` count.
    pub int_expr: &'a dyn Fn(&Expr) -> bool,
    /// A sum Go renders as `math.FMA`.
    pub fused: &'a dyn Fn(&Expr) -> bool,
    /// The loop is one [`branched_loops`] returns.
    pub branched: bool,
    /// The loop runs inside another, over a window or a block, where no
    /// unrolled loop was measured: [`unroll`] leaves it alone.
    pub nested: bool,
}

/// One re-slice: `array[start:][:n]`, element `k` being what `array[inv + var
/// + elem]` reaches on pass `k`, `var` at its value on loop entry. In a
/// [`Plan::down`] loop it is `array[start-n:][:n]` and pass `k` reaches
/// element `n-1-k`.
pub(super) struct Lane {
    pub array: String,
    pub var: String,
    inv: Vec<String>,
    pub elem: i64,
    pub start: Expr,
}

pub(super) enum Trip {
    /// `counter < bound`, or `<=`.
    Up { bound: Expr, inclusive: bool },
    /// `counter > bound`, or `counter != 0` / `> 0` when `bound` is `None`.
    Down { bound: Option<Expr> },
}

pub(super) struct Plan {
    pub counter: String,
    pub trip: Trip,
    /// The indices, each with its move per pass.
    pub steps: BTreeMap<String, i64>,
    pub facts: Facts,
    pub lanes: Vec<Lane>,
    /// The lanes' indices move down, so the counter runs from `n-1` to 0.
    pub down: bool,
    /// The body, then the `for` update, blocks inlined and every step a
    /// statement of its own.
    pub pass: Vec<Statement>,
    /// Per statement of `pass`, each index's steps taken before it.
    pub before: Vec<BTreeMap<String, i64>>,
    /// Passes per trip, by [`unroll`].
    pub unroll: Option<usize>,
    /// The pass makes a call [`calls_out`] names.
    pub calls_out: bool,
    pub scan: Option<Scan>,
}

impl Plan {
    /// The lane `array[idx]` reads in statement `at` of [`Self::pass`].
    pub(super) fn lane_of(&self, at: usize, array: &str, idx: &Expr) -> Option<usize> {
        let (var, inv, c) = linear(idx, &self.steps, &self.facts)?;
        let elem = c + self.before[at].get(&var).copied().unwrap_or(0);
        self.lanes.iter().position(|l| l.array == array && l.var == var && l.inv == inv && l.elem == elem)
    }

    /// Whether statement `s` of [`Self::pass`] is an index's step.
    pub(super) fn is_step(&self, s: &Statement) -> bool {
        as_step(s).is_some_and(|(v, _)| self.steps.contains_key(v))
    }
}

#[derive(Clone, Copy, PartialEq, Eq, PartialOrd, Ord, Debug)]
pub(super) enum Refusal {
    /// No pass count fixed at entry.
    Header,
    /// A declaration, a switch or a ring operation in the pass.
    Statement,
    /// A `return`, or a `break` / `continue` of this loop.
    Exit,
    CrossCall,
    /// The counter is not an index, or the bound moves or is not an integer.
    Counter,
    /// An index the loop keeps from one pass to the next, taken from a moving
    /// one: a scan that tracks where its extreme is.
    IndexTracking,
    /// Nothing to re-slice.
    NoLane,
    /// Skipped by some pass of a loop over bars ([`branched_loops`]): it would
    /// save its checks on some bars only, while what its re-slices hold stays live
    /// across every bar and pushes that loop's own indices to the stack.
    Branched,
    /// A pass that calls out, carries a scalar to the next pass and indexes an
    /// array no lane cuts: every call clobbers the registers, so the re-slices
    /// are reloaded after each call beside the many values such a loop keeps,
    /// costing more than the checks they remove from its long body.
    CallsOverRing,
    /// A pass that shifts a carried real through two variables, `y2 = y1; y1 =
    /// y`, into a fused multiply-add: each fused site has a call path at the
    /// default GOAMD64, so `y1` goes to the stack every pass and comes back as
    /// `y2`. That round trip bounds the loop, and the re-sliced form lands the
    /// reload worse.
    FusedShift,
}

/// `while( cond ) body`, or with `update` the rest of
/// `for( init; cond; update ) body` once `init` has run.
pub(super) fn plan(cond: &Expr, body: &[Statement], update: Option<&Statement>, env: &Env) -> Result<Plan, Refusal> {
    let any = |_: &Expr| true;
    let never = |_: &Expr| false;
    let none = |_: &str| false;
    let no_ring = |_: &str| None;
    let names = Names { index: env.int_local, usize_expr: &any, castable: &never, sliceable: &none, same_len: &none, ring_storage: &no_ring };
    let f = form(cond, &names).ok_or(Refusal::Header)?;
    let Expr::BinOp(_, op, rhs) = cond else { return Err(Refusal::Header) };
    let trip = match (op, f.step, f.cond_step.is_some()) {
        (BinOp::Less, 1, false) => Trip::Up { bound: f.bound.clone(), inclusive: false },
        (BinOp::LessEq, 1, false) => Trip::Up { bound: f.bound.clone(), inclusive: true },
        (BinOp::Greater | BinOp::NotEq, -1, false) => {
            Trip::Down { bound: (!matches!(rhs.as_ref(), Expr::IntLiteral(0))).then(|| f.bound.clone()) }
        }
        _ => return Err(Refusal::Header),
    };

    let mut pass = Vec::new();
    for s in body.iter().chain(match update {
        Some(Statement::Block { body }) => body.iter().collect::<Vec<_>>(),
        Some(u) => vec![u],
        None => vec![],
    }) {
        inline(s, &mut pass);
    }
    allowed(&pass, false)?;
    let mut crossing = false;
    for s in &pass {
        crate::streaming::walk_stmt_exprs(s, &mut |top| {
            crate::streaming::walk_expr(top, &mut |e| crossing |= matches!(e, Expr::FuncCall(n, _) if (env.cross_call)(n)));
        });
    }
    if crossing {
        return Err(Refusal::CrossCall);
    }

    let facts = Facts::of(&pass);
    let steps: BTreeMap<String, i64> = facts.induction().into_iter().filter(|(v, _)| (env.int_local)(v)).collect();
    if steps.get(&f.counter) != Some(&f.step) || !invariant(&f.bound, &facts) || !(env.int_expr)(&f.bound) {
        return Err(Refusal::Counter);
    }
    let int_assigned: BTreeSet<&String> = facts.other.iter().filter(|v| (env.int_local)(v) && !steps.contains_key(*v)).collect();
    let tracking = int_assigned.iter().any(|t| taken_from_index(&pass, t, &steps) && !fresh(&pass, t));

    let mut before = Vec::with_capacity(pass.len());
    let mut taken: BTreeMap<String, i64> = BTreeMap::new();
    for s in &pass {
        before.push(taken.clone());
        if let Some((v, d)) = as_step(s).filter(|(v, _)| steps.contains_key(*v)) {
            *taken.entry(v.to_string()).or_default() += d;
        }
    }

    let mut down = false;
    let mut lanes = choose_lanes(&pass, &before, &steps, &facts, env, &f.counter, &trip, 1);
    if lanes.is_empty() && !tracking {
        down = true;
        lanes = choose_lanes(&pass, &before, &steps, &facts, env, &f.counter, &trip, -1);
    }
    let mut plan = Plan { counter: f.counter, trip, steps, facts, lanes, down, pass, before, unroll: None, calls_out: false, scan: None };
    if tracking {
        plan.scan = Some(scan_shape(&plan, env).ok_or(Refusal::IndexTracking)?);
    } else if plan.lanes.is_empty() {
        return Err(Refusal::NoLane);
    }
    if env.branched {
        return Err(Refusal::Branched);
    }
    plan.calls_out = calls_out(&plan.pass);
    if plan.calls_out && carries(&plan) && unlaned(&plan.pass, env) {
        return Err(Refusal::CallsOverRing);
    }
    if fused_shift(&plan, env) {
        return Err(Refusal::FusedShift);
    }
    if plan.scan.is_none() {
        plan.unroll = unroll(&plan, env);
    }
    Ok(plan)
}

/// A call gc keeps a call at the default GOAMD64: every value live across the
/// loop is reloaded after it on each pass.
fn calls_out(pass: &[Statement]) -> bool {
    let mut hit = false;
    for s in pass {
        crate::streaming::walk_stmt_exprs(s, &mut |top| {
            crate::streaming::walk_expr(top, &mut |e| {
                if let Expr::FuncCall(n, _) = e {
                    hit |= MathFn::from_name(n).is_some_and(|m| !matches!(m, MathFn::Sqrt | MathFn::Abs | MathFn::Max | MathFn::Min));
                }
            });
        });
    }
    hit
}

/// Some scalar the pass reads holds the value the pass before left in it.
fn carries(plan: &Plan) -> bool {
    plan.facts.assigned.iter().any(|n| !plan.steps.contains_key(n) && !fresh(&plan.pass, n))
}

/// The pass, at any depth, indexes an array that is no contract: a ring, a
/// local array, a scratch array used as a ring.
fn unlaned(pass: &[Statement], env: &Env) -> bool {
    let mut hit = false;
    for s in pass {
        crate::streaming::walk_stmt_exprs(s, &mut |top| {
            crate::streaming::walk_expr(top, &mut |e| hit |= matches!(e, Expr::ArrayAccess(a, _) if (env.contract)(a).is_none()));
        });
    }
    hit
}

/// `p = q` on every pass and `p` assigned nowhere else, both reals carried
/// in, `q` assigned something other than `p` in the same pass, and a sum Go
/// fuses reading `p`.
fn fused_shift(plan: &Plan, env: &Env) -> bool {
    let mut writes: Vec<(String, Expr)> = Vec::new();
    for s in &plan.pass {
        visit_assigns(s, &mut |t, v| {
            if let Expr::Var(n) = t {
                writes.push((n.clone(), v.clone()));
            }
        });
    }
    let carried = |n: &str| !(env.int_local)(n) && !fresh(&plan.pass, n);
    let mut delayed = BTreeSet::new();
    for s in &plan.pass {
        let Statement::Assign { target: Expr::Var(p), value: Expr::Var(q), compound: false } = s else { continue };
        let renewed = writes.iter().any(|(t, w)| t == q && !matches!(w, Expr::Var(n) if n == p));
        let once = writes.iter().filter(|(t, _)| t == p).count() == 1;
        if q != p && renewed && once && carried(p) && carried(q) {
            delayed.insert(p.as_str());
        }
    }
    let mut hit = false;
    for s in &plan.pass {
        crate::streaming::walk_stmt_exprs(s, &mut |top| {
            crate::streaming::walk_expr(top, &mut |e| hit |= (env.fused)(e) && delayed.iter().any(|v| mentions(e, v)));
        });
    }
    hit
}

/// How many times each step variable is named in `s`, and how many of those
/// sit in the index of an access [`Plan::lane_of`] resolves; `arrays` keeps
/// the lanes of the arrays it accepts.
fn step_mentions(plan: &Plan, at: usize, s: &Statement, arrays: &dyn Fn(&str) -> bool) -> (BTreeMap<String, usize>, BTreeMap<String, usize>) {
    let (mut all, mut laned) = (BTreeMap::new(), BTreeMap::new());
    let count = |e: &Expr, into: &mut BTreeMap<String, usize>| {
        crate::streaming::walk_expr(e, &mut |x| {
            if let Expr::Var(n) = x {
                if plan.steps.contains_key(n) {
                    *into.entry(n.clone()).or_default() += 1;
                }
            }
        });
    };
    crate::streaming::walk_stmt_exprs(s, &mut |e| count(e, &mut all));
    each_access(s, &mut |array, idx, _| {
        if arrays(array) && plan.lane_of(at, array, idx).is_some() {
            count(idx, &mut laned);
        }
    });
    (all, laned)
}

/// Passes per trip, when no pass depends on another: every scalar the pass
/// reads it either never writes or writes first on every path, every element
/// it touches is a lane at its own position, an index is read through a lane
/// only, and nothing is called that gc keeps a call
/// ([`super::lockstep::unrollable`]). Eight when the pass is one store of an
/// expression to an element, four otherwise.
fn unroll(plan: &Plan, env: &Env) -> Option<usize> {
    if env.nested || plan.down || plan.calls_out || carries(plan) {
        return None;
    }
    let mut code = Vec::new();
    for (at, s) in plan.pass.iter().enumerate() {
        if plan.is_step(s) || matches!(s, Statement::Comment(_) | Statement::UnrollHint { .. }) {
            continue;
        }
        code.push(s);
        let mut every = true;
        each_access(s, &mut |array, idx, _| every &= plan.lane_of(at, array, idx).is_some());
        let (all, laned) = step_mentions(plan, at, s, &|_| true);
        if !every || all != laned {
            return None;
        }
    }
    let single = unrollable(&code, &|m| matches!(m, MathFn::Sqrt | MathFn::Abs | MathFn::Max | MathFn::Min))?;
    Some(if single { 8 } else { 4 })
}

/// An index-tracking scan as [`scan_shape`] proves it.
pub(super) struct Scan {
    /// Indices held in an `int` across the loop: the counter, the steps read
    /// other than as a lane, and the indices the pass assigns.
    pub ints: BTreeSet<String>,
    /// Counters of the rescans `c = e; while( ++c <= t ) ...`.
    pub rescans: BTreeSet<String>,
    /// The inputs the pass reads, each cut once to `endIdx + 1`.
    pub inputs: Vec<String>,
}

/// A loop over bars to `endIdx` inclusive whose integer locals are indices:
/// each one assigned is assigned an index expression (indices and literals
/// added or subtracted), every input is read at one, and a rescan counter is
/// named only in its own rescan, which follows its start and writes neither
/// it, nor the bound, nor a step. The counter is not read after its step.
fn scan_shape(plan: &Plan, env: &Env) -> Option<Scan> {
    struct Ck<'a> {
        ints: &'a BTreeSet<String>,
        rescans: &'a BTreeSet<String>,
        steps: &'a BTreeMap<String, i64>,
        env: &'a Env<'a>,
        inputs: BTreeSet<String>,
    }
    impl Ck<'_> {
        fn index(&self, e: &Expr, inner: Option<&str>) -> bool {
            match e {
                Expr::IntLiteral(_) => true,
                Expr::Var(n) => self.ints.contains(n) || inner == Some(n.as_str()),
                Expr::BinOp(l, BinOp::Add | BinOp::Sub, r) => self.index(l, inner) && self.index(r, inner),
                _ => false,
            }
        }
        fn reads(&mut self, e: &Expr, inner: Option<&str>) -> bool {
            let mut ok = true;
            crate::streaming::walk_expr(e, &mut |x| match x {
                Expr::ArrayAccess(a, i) if (self.env.contract)(a) == Some(Contract::Input) => {
                    ok &= self.index(i, inner);
                    self.inputs.insert(a.clone());
                }
                Expr::Var(n) if self.rescans.contains(n) => ok &= inner == Some(n.as_str()),
                Expr::PostIncrement(_) | Expr::PreIncrement(_) | Expr::PostDecrement(_) | Expr::PreDecrement(_) | Expr::AddressOf(_) => ok = false,
                _ => {}
            });
            ok
        }
        fn list(&mut self, list: &[Statement], inner: Option<&str>, top: bool) -> bool {
            let code: Vec<&Statement> = list.iter().filter(|s| !matches!(s, Statement::Comment(_) | Statement::UnrollHint { .. })).collect();
            for (n, s) in code.iter().enumerate() {
                let ok = match s {
                    Statement::Assign { .. } if top && as_step(s).is_some_and(|(v, _)| self.steps.contains_key(v)) => true,
                    Statement::Assign { target: Expr::Var(t), value, compound } if self.rescans.contains(t) => {
                        let follows = matches!(code.get(n + 1), Some(Statement::While { condition, .. }) if rescan_of(condition).is_some_and(|(c, _)| c == t.as_str()));
                        inner.is_none() && !compound && follows && self.index(value, None)
                    }
                    Statement::Assign { target: Expr::Var(t), value, compound } if (self.env.int_local)(t) => {
                        self.ints.contains(t) && !self.steps.contains_key(t) && !compound && self.index(value, inner) && self.reads(value, inner)
                    }
                    Statement::Assign { target: Expr::Var(_), value, .. } => self.reads(value, inner),
                    Statement::Assign { target: Expr::ArrayAccess(a, i), value, .. } => {
                        (self.env.contract)(a) != Some(Contract::Input) && self.reads(i, inner) && self.reads(value, inner)
                    }
                    Statement::If { condition, then_body, else_body, .. } => {
                        self.reads(condition, inner) && self.list(then_body, inner, false) && self.list(else_body, inner, false)
                    }
                    Statement::While { condition, body } if inner.is_none() => match rescan_of(condition) {
                        Some((c, bound)) if self.rescans.contains(c) && self.index(bound, None) => {
                            let written = stored(body);
                            !written.contains(c) && !written.iter().any(|w| self.steps.contains_key(w)) && self.list(body, Some(c), false)
                        }
                        _ => false,
                    },
                    _ => false,
                };
                if !ok {
                    return false;
                }
            }
            true
        }
    }
    let Trip::Up { bound, inclusive: true } = &plan.trip else { return None };
    if !(env.fixed_end)(bound) {
        return None;
    }
    let mut rescans = BTreeSet::new();
    rescan_counters(&plan.pass, &mut rescans);
    let output = |a: &str| matches!((env.contract)(a), Some(Contract::Output | Contract::Scratch));
    let mut laned: BTreeSet<String> = plan.steps.keys().filter(|v| **v != plan.counter).cloned().collect();
    for (at, s) in plan.pass.iter().enumerate() {
        if plan.is_step(s) {
            continue;
        }
        let (all, through) = step_mentions(plan, at, s, &output);
        laned.retain(|v| all.get(v) == through.get(v));
    }
    let mut ints: BTreeSet<String> = plan.steps.keys().filter(|v| !laned.contains(*v)).cloned().collect();
    ints.extend(plan.facts.assigned.iter().filter(|v| (env.int_local)(v) && !plan.steps.contains_key(*v) && !rescans.contains(*v)).cloned());
    let mut ck = Ck { ints: &ints, rescans: &rescans, steps: &plan.steps, env, inputs: BTreeSet::new() };
    if !ck.list(&plan.pass, None, true) {
        return None;
    }
    let stepped = plan.pass.iter().position(|s| as_step(s).is_some_and(|(v, _)| v == plan.counter))?;
    if plan.pass[stepped + 1..].iter().any(|s| stmt_mentions(s, &plan.counter)) {
        return None;
    }
    let inputs = ck.inputs.into_iter().collect();
    Some(Scan { ints, rescans, inputs })
}

fn rescan_counters(list: &[Statement], out: &mut BTreeSet<String>) {
    for s in list {
        match s {
            Statement::While { condition, .. } => out.extend(rescan_of(condition).map(|(c, _)| c.to_string())),
            Statement::If { then_body, else_body, .. } => {
                rescan_counters(then_body, out);
                rescan_counters(else_body, out);
            }
            _ => {}
        }
    }
}

/// `++c <= bound`: the counter and the bound.
pub(super) fn rescan_of(cond: &Expr) -> Option<(&str, &Expr)> {
    let Expr::BinOp(l, BinOp::LessEq, bound) = cond else { return None };
    let Expr::PreIncrement(c) = l.as_ref() else { return None };
    let Expr::Var(c) = c.as_ref() else { return None };
    Some((c, bound))
}

/// One access of a contract array at an index plus constants.
struct Occ {
    array: String,
    var: String,
    inv: Vec<String>,
    /// The element it reaches relative to the pass, `var` at its entry value.
    elem: i64,
    every: bool,
    index: Expr,
    c: i64,
}

/// The lanes of the indices that move by `dir`.
#[allow(clippy::too_many_arguments)]
fn choose_lanes(
    pass: &[Statement],
    before: &[BTreeMap<String, i64>],
    steps: &BTreeMap<String, i64>,
    facts: &Facts,
    env: &Env,
    counter: &str,
    trip: &Trip,
    dir: i64,
) -> Vec<Lane> {
    let mut occs = Vec::new();
    for (at, s) in pass.iter().enumerate() {
        let both = every_path(s);
        each_access(s, &mut |array, idx, reach| {
            if (env.contract)(array).is_none() || facts.assigned.contains(array) || facts.bare.contains(array) {
                return;
            }
            let Some((var, inv, c)) = linear(idx, steps, facts) else { return };
            if steps[&var] != dir {
                return;
            }
            let elem = c + before[at].get(&var).copied().unwrap_or(0);
            let every = reach == Reach::Every || both.contains(&(array.to_string(), format!("{idx:?}")));
            occs.push(Occ { array: array.to_string(), var, inv, elem, every, index: idx.clone(), c });
        });
    }
    // An element some access reaches on every pass bounds the lane through it
    // as the loop as written would: cutting the lane panics only where a pass
    // would have. Inputs share their extents: the public tier checked each
    // against `endIdx + 1`, and no body reads an input past `endIdx` (the
    // phantom-I/O sweeps), so an element one input reaches on every pass is
    // in bounds in all of them.
    let input = |a: &str| (env.contract)(a) == Some(Contract::Input);
    let mut span: BTreeMap<(&str, &str, &[String]), (i64, i64)> = BTreeMap::new();
    for o in occs.iter().filter(|o| o.every) {
        let pooled = if input(&o.array) { "" } else { o.array.as_str() };
        for a in [o.array.as_str(), pooled] {
            let s = span.entry((a, &o.var, &o.inv)).or_insert((o.elem, o.elem));
            s.0 = s.0.min(o.elem);
            s.1 = s.1.max(o.elem);
        }
    }
    let within = |a: &str, o: &Occ| span.get(&(a, o.var.as_str(), o.inv.as_slice())).is_some_and(|(lo, hi)| *lo <= o.elem && o.elem <= *hi);
    // An input read on some passes only, at or behind a counter that stops at
    // `endIdx`, from no lower than an element read on every pass.
    let to_end = matches!(trip, Trip::Up { bound, inclusive: true } if (env.fixed_end)(bound));
    let floor = occs.iter().filter(|o| o.every && o.var == counter && o.inv.is_empty()).map(|o| o.elem).min();
    let mut lanes: Vec<Lane> = Vec::new();
    for o in &occs {
        let held = within(&o.array, o) || (input(&o.array) && within("", o));
        let ended = to_end
            && input(&o.array)
            && o.inv.is_empty()
            && o.var == counter
            && o.elem <= 0
            && floor.is_some_and(|lo| lo <= o.elem);
        if (held || ended) && !lanes.iter().any(|l| l.array == o.array && l.var == o.var && l.inv == o.inv && l.elem == o.elem) {
            let top = i64::from(dir < 0);
            let start = folded(substitute(&o.index, &o.var, &shifted(&o.var, o.elem - o.c + top)));
            lanes.push(Lane { array: o.array.clone(), var: o.var.clone(), inv: o.inv.clone(), elem: o.elem, start });
        }
    }
    lanes
}

/// `e` with its integer literals summed into one trailing term, when it has
/// more than one.
fn folded(e: Expr) -> Expr {
    let mut terms = Vec::new();
    flatten(&e, true, &mut terms);
    let mut c = 0;
    let mut rest = Vec::new();
    for (t, plus) in &terms {
        match t {
            Expr::IntLiteral(n) => c += if *plus { *n } else { -*n },
            other => rest.push((*other, *plus)),
        }
    }
    if terms.len() - rest.len() < 2 || rest.is_empty() {
        return e;
    }
    let mut acc = if rest[0].1 { rest[0].0.clone() } else { Expr::Neg(Box::new(rest[0].0.clone())) };
    for (t, plus) in &rest[1..] {
        acc = Expr::BinOp(Box::new(acc), if *plus { BinOp::Add } else { BinOp::Sub }, Box::new((*t).clone()));
    }
    match c {
        0 => acc,
        c if c > 0 => Expr::BinOp(Box::new(acc), BinOp::Add, Box::new(Expr::IntLiteral(c))),
        c => Expr::BinOp(Box::new(acc), BinOp::Sub, Box::new(Expr::IntLiteral(-c))),
    }
}

/// The elements every path through `s` accesses, as (array, index `Debug`
/// text): an index means one element in both arms because no index of the
/// loop steps in an arm.
fn every_path(s: &Statement) -> BTreeSet<(String, String)> {
    let mut out = BTreeSet::new();
    each_access(s, &mut |a, i, r| {
        if r == Reach::Every {
            out.insert((a.to_string(), format!("{i:?}")));
        }
    });
    if let Statement::If { then_body, else_body, .. } = s {
        let arm = |b: &[Statement]| b.iter().flat_map(every_path).collect::<BTreeSet<_>>();
        out.extend(arm(then_body).intersection(&arm(else_body)).cloned());
    }
    out
}

/// Statements the pass may hold; `nested` inside a loop of its own.
fn allowed(list: &[Statement], nested: bool) -> Result<(), Refusal> {
    for s in list {
        match s {
            Statement::Assign { .. }
            | Statement::Expr(_)
            | Statement::Comment(_)
            | Statement::UnrollHint { .. }
            | Statement::CircBuf(CircBuf::Next { .. }) => {}
            Statement::If { then_body, else_body, .. } => {
                allowed(then_body, nested)?;
                allowed(else_body, nested)?;
            }
            Statement::Block { body } => allowed(body, nested)?,
            Statement::While { body, .. } | Statement::DoWhile { body, .. } | Statement::For { body, .. } | Statement::ForC { body, .. } => {
                allowed(body, true)?;
            }
            Statement::Break | Statement::Continue if nested => {}
            Statement::Break | Statement::Continue | Statement::Return { .. } => return Err(Refusal::Exit),
            _ => return Err(Refusal::Statement),
        }
    }
    Ok(())
}

fn mentions(e: &Expr, v: &str) -> bool {
    let mut hit = false;
    crate::streaming::walk_expr(e, &mut |x| hit |= matches!(x, Expr::Var(n) if n == v));
    hit
}

fn stmt_mentions(s: &Statement, v: &str) -> bool {
    let mut hit = matches!(s, Statement::For { var, .. } if var == v);
    crate::streaming::walk_stmt_exprs(s, &mut |e| hit |= mentions(e, v));
    hit
}

/// Whether some assignment to `t` in `list` takes an index's value, other
/// than through an element it subscripts.
fn taken_from_index(list: &[Statement], t: &str, steps: &BTreeMap<String, i64>) -> bool {
    fn value_of(e: &Expr, name: &str) -> bool {
        match e {
            Expr::Var(n) => n == name,
            Expr::ArrayAccess(..) | Expr::Literal(_) | Expr::IntLiteral(_) | Expr::PointerDeref(_) => false,
            Expr::BinOp(l, _, r) => value_of(l, name) || value_of(r, name),
            Expr::Ternary(c, yes, no) => value_of(c, name) || value_of(yes, name) || value_of(no, name),
            Expr::FuncCall(_, args) => args.iter().any(|x| value_of(x, name)),
            Expr::Cast(_, i)
            | Expr::Not(i)
            | Expr::Neg(i)
            | Expr::BitwiseNot(i)
            | Expr::AddressOf(i)
            | Expr::PostIncrement(i)
            | Expr::PostDecrement(i)
            | Expr::PreIncrement(i)
            | Expr::PreDecrement(i) => value_of(i, name),
        }
    }
    list.iter().any(|s| {
        let mut hit = false;
        visit_assigns(s, &mut |target, value| {
            hit |= matches!(target, Expr::Var(n) if n == t) && steps.keys().any(|v| value_of(value, v));
        });
        hit
    })
}

fn visit_assigns(s: &Statement, f: &mut dyn FnMut(&Expr, &Expr)) {
    match s {
        Statement::Assign { target, value, .. } => f(target, value),
        Statement::If { then_body, else_body, .. } => {
            for x in then_body.iter().chain(else_body) {
                visit_assigns(x, f);
            }
        }
        Statement::ForC { init, update, body, .. } => {
            visit_assigns(init, f);
            visit_assigns(update, f);
            for x in body {
                visit_assigns(x, f);
            }
        }
        Statement::While { body, .. } | Statement::DoWhile { body, .. } | Statement::For { body, .. } | Statement::Block { body } => {
            for x in body {
                visit_assigns(x, f);
            }
        }
        _ => {}
    }
}

/// Whether every read of `t` in `list` follows a write of it in the same
/// pass, so no value of `t` crosses from one pass to the next.
fn fresh(list: &[Statement], t: &str) -> bool {
    let Some(at) = list.iter().position(|s| stmt_mentions(s, t)) else { return true };
    let later = || !list[at + 1..].iter().any(|s| stmt_mentions(s, t));
    match &list[at] {
        Statement::Assign { target: Expr::Var(n), value, compound: false } if n == t => !mentions(value, t),
        Statement::ForC { init, .. } if matches!(init.as_ref(), Statement::Assign { target: Expr::Var(n), value, compound: false } if n == t && !mentions(value, t)) => true,
        Statement::If { condition, then_body, else_body, .. } => !mentions(condition, t) && later() && fresh(then_body, t) && fresh(else_body, t),
        Statement::Block { body } => later() && fresh(body, t),
        _ => false,
    }
}

/// Every name `body` stores to other than as an element, at any depth.
pub(super) fn stored(body: &[Statement]) -> BTreeSet<String> {
    Facts::of(body).assigned
}

/// The arrays of each circular buffer `body` sizes once with `CIRCBUF_INIT`,
/// outside any loop, and never uses as a ring: no `CIRCBUF_NEXT`, and its
/// index never named.
pub(super) fn scratch(body: &[Statement]) -> Vec<String> {
    #[derive(Default)]
    struct Seen {
        inits: HashMap<String, (usize, bool, Option<CircBufLayout>)>,
        rings: BTreeSet<String>,
        named: BTreeSet<String>,
    }
    fn walk(list: &[Statement], in_loop: bool, seen: &mut Seen) {
        for s in list {
            match s {
                Statement::CircBuf(CircBuf::Init { id, layout, .. }) => {
                    let e = seen.inits.entry(id.clone()).or_default();
                    e.0 += 1;
                    e.1 |= in_loop;
                    e.2 = Some(layout.clone());
                }
                Statement::CircBuf(CircBuf::Next { id } | CircBuf::InitLocalOnly { id, .. }) => {
                    seen.rings.insert(id.clone());
                }
                _ => {}
            }
            crate::streaming::walk_stmt_exprs(s, &mut |e| {
                crate::streaming::walk_expr(e, &mut |x| {
                    if let Expr::Var(n) = x {
                        seen.named.insert(n.clone());
                    }
                });
            });
            let (nested, looped): (Vec<&[Statement]>, bool) = match s {
                Statement::While { body, .. } | Statement::DoWhile { body, .. } | Statement::For { body, .. } => (vec![body], true),
                Statement::ForC { init, update, body, .. } => (vec![std::slice::from_ref(init), std::slice::from_ref(update), body], true),
                Statement::Block { body } => (vec![body], false),
                Statement::If { then_body, else_body, .. } => (vec![then_body, else_body], false),
                Statement::Switch { cases, default, .. } => (cases.iter().map(|(_, b)| b.as_slice()).chain([default.as_slice()]).collect(), false),
                _ => (Vec::new(), false),
            };
            for b in nested {
                walk(b, in_loop || looped, seen);
            }
        }
    }
    let mut seen = Seen::default();
    walk(body, false, &mut seen);
    let mut out = Vec::new();
    for (id, (count, in_loop, layout)) in &seen.inits {
        let Some(layout) = layout else { continue };
        if *count == 1 && !in_loop && !seen.rings.contains(id) && !seen.named.contains(&format!("{id}_Idx")) {
            out.extend(super::java::circbuf_arrays(id, layout).into_iter().map(|(a, _)| a));
        }
    }
    out.sort();
    out
}

/// The loops of `body` that some pass of an enclosing loop over bars skips,
/// by address: an `if` or `switch` lies between them that is not one whose
/// every arm, `else` or `default` included, runs outside any further branch
/// a loop [`plan`] takes. A loop over bars moves some index by one on every pass; a block
/// scan's pass loop, which moves its indices by a block, is none, so its arms
/// keep their passes.
pub(super) fn branched_loops(body: &[Statement], env: &Env, out: &mut HashSet<usize>) {
    fn over_bars(s: &Statement, env: &Env) -> bool {
        let (body, update) = match s {
            Statement::For { .. } => return true,
            Statement::While { body, .. } | Statement::DoWhile { body, .. } => (body, None),
            Statement::ForC { body, update, .. } => (body, Some(update.as_ref())),
            _ => return false,
        };
        let mut pass = Vec::new();
        for x in body.iter().chain(update) {
            inline(x, &mut pass);
        }
        Facts::of(&pass).induction().keys().any(|v| (env.int_local)(v))
    }
    fn plans(arm: &[Statement], env: &Env) -> bool {
        arm.iter().any(|s| match s {
            Statement::While { condition, body } => plan(condition, body, None, env).is_ok(),
            Statement::ForC { condition, body, update, .. } => plan(condition, body, Some(update), env).is_ok(),
            Statement::Block { body } => plans(body, env),
            _ => false,
        })
    }
    fn walk(list: &[Statement], in_bars: bool, branched: bool, env: &Env, out: &mut HashSet<usize>) {
        for s in list {
            let arms: Vec<&[Statement]> = match s {
                Statement::If { then_body, else_body, .. } => vec![then_body, else_body],
                Statement::Switch { cases, default, .. } => cases.iter().map(|(_, b)| b.as_slice()).chain([default.as_slice()]).collect(),
                Statement::Block { body } => {
                    walk(body, in_bars, branched, env, out);
                    continue;
                }
                _ => {
                    let Some(body) = super::go_select::loop_body(s) else { continue };
                    if branched {
                        out.insert(super::go_select::addr(s));
                    }
                    walk(body, in_bars || over_bars(s, env), branched, env, out);
                    continue;
                }
            };
            let every = arms.iter().all(|a| plans(a, env));
            for a in arms {
                walk(a, in_bars, branched || (in_bars && !every), env, out);
            }
        }
    }
    walk(body, false, false, env, out);
}

// ---------------------------------------------------------------------------
// The padded constant-cap loop
// ---------------------------------------------------------------------------

pub(super) struct Capped<'s> {
    pub var: String,
    pub cap: i64,
    pub bound: &'s Expr,
    pub body: &'s [Statement],
}

/// `for( v = a; v < K; v++ ) { if( v < B ) S }`, `a <= K`, the `if` the whole
/// body with no `else`, `S` writing neither `v` nor what `B` reads, and
/// leaving the loop no other way than by its test, and `B` an integer.
pub(super) fn capped<'s>(init: &Statement, cond: &Expr, update: &Statement, body: &'s [Statement], int_expr: &dyn Fn(&Expr) -> bool) -> Option<Capped<'s>> {
    let Statement::Assign { target: Expr::Var(v), value: Expr::IntLiteral(first), compound: false } = init else { return None };
    let Expr::BinOp(lhs, BinOp::Less, limit) = cond else { return None };
    let (Expr::Var(tested), Expr::IntLiteral(cap)) = (lhs.as_ref(), limit.as_ref()) else { return None };
    let up = as_step(update) == Some((v.as_str(), 1))
        || matches!(update, Statement::Expr(Expr::PostIncrement(stepped) | Expr::PreIncrement(stepped)) if matches!(stepped.as_ref(), Expr::Var(n) if n == v));
    if tested != v || first > cap || !up {
        return None;
    }
    let code: Vec<&Statement> = body.iter().filter(|s| !matches!(s, Statement::Comment(_))).collect();
    let [Statement::If { condition, then_body, else_body, .. }] = code.as_slice() else { return None };
    let Expr::BinOp(gl, BinOp::Less, bound) = condition else { return None };
    if !matches!(gl.as_ref(), Expr::Var(n) if n == v) || !else_body.is_empty() || mentions(bound, v) || !int_expr(bound) {
        return None;
    }
    let facts = Facts::of(then_body);
    if !invariant(bound, &facts) || facts.assigned.contains(v) || leaves(then_body, false) {
        return None;
    }
    Some(Capped { var: v.clone(), cap: *cap, bound, body: then_body })
}

/// A `break` or `continue` of the loop holding `list`, or a `goto`-like exit.
fn leaves(list: &[Statement], nested: bool) -> bool {
    list.iter().any(|s| match s {
        Statement::Break | Statement::Continue => !nested,
        Statement::If { then_body, else_body, .. } => leaves(then_body, nested) || leaves(else_body, nested),
        Statement::Block { body } => leaves(body, nested),
        Statement::While { body, .. } | Statement::DoWhile { body, .. } | Statement::For { body, .. } | Statement::ForC { body, .. } => leaves(body, true),
        Statement::Switch { cases, default, .. } => cases.iter().any(|(_, b)| leaves(b, true)) || leaves(default, true),
        _ => false,
    })
}

// ---------------------------------------------------------------------------
// What is read after a loop
// ---------------------------------------------------------------------------

/// The variables live after each loop of `body`, by the loop's address, and
/// those live on entry. `out` is what is live after `body`. Over-approximate:
/// a variable it misses as live would lose its last value.
pub(super) fn live_after_loops(body: &[Statement], out: BTreeSet<String>, map: &mut HashMap<usize, BTreeSet<String>>) -> BTreeSet<String> {
    Live { map }.block(body, out, &Jumps::default())
}

/// [`live_after_loops`] for the loops of `pass`, run as the body of a loop
/// testing `cond` after which `out` is live.
pub(super) fn live_after_pass_loops(cond: &Expr, pass: &[Statement], out: &BTreeSet<String>, map: &mut HashMap<usize, BTreeSet<String>>) {
    Live { map }.while_loop(None, cond, pass, out);
}

#[derive(Default, Clone)]
struct Jumps {
    brk: Option<BTreeSet<String>>,
    cont: Option<BTreeSet<String>>,
}

struct Live<'m> {
    map: &'m mut HashMap<usize, BTreeSet<String>>,
}

fn uses(e: &Expr, into: &mut BTreeSet<String>) {
    crate::streaming::walk_expr(e, &mut |x| {
        if let Expr::Var(n) | Expr::ArrayAccess(n, _) | Expr::PointerDeref(n) = x {
            into.insert(n.clone());
        }
    });
}

impl Live<'_> {
    fn block(&mut self, list: &[Statement], out: BTreeSet<String>, j: &Jumps) -> BTreeSet<String> {
        list.iter().rev().fold(out, |live, s| self.stmt(s, live, j))
    }

    fn looped(&mut self, s: Option<&Statement>, out: &BTreeSet<String>, mut head: impl FnMut(&mut Self, &BTreeSet<String>) -> BTreeSet<String>) -> BTreeSet<String> {
        let mut h = out.clone();
        loop {
            let next: BTreeSet<String> = head(self, &h).union(out).cloned().collect();
            if next == h {
                break;
            }
            h = next;
        }
        if let Some(s) = s {
            self.map.entry(super::go_select::addr(s)).or_default().extend(out.iter().cloned());
        }
        h
    }

    fn while_loop(&mut self, s: Option<&Statement>, condition: &Expr, body: &[Statement], out: &BTreeSet<String>) -> BTreeSet<String> {
        self.looped(s, out, |me, h| {
            let mut l = me.block(body, h.clone(), &Jumps { brk: Some(out.clone()), cont: Some(h.clone()) });
            uses(condition, &mut l);
            l
        })
    }

    fn stmt(&mut self, s: &Statement, out: BTreeSet<String>, j: &Jumps) -> BTreeSet<String> {
        match s {
            Statement::VarDecl { name, init, .. } => {
                let mut l = out;
                l.remove(name);
                if let Some(i) = init {
                    uses(i, &mut l);
                }
                l
            }
            Statement::Assign { target, value, compound } => {
                let mut l = out;
                match target {
                    Expr::Var(t) if !*compound => {
                        l.remove(t);
                    }
                    other => uses(other, &mut l),
                }
                uses(value, &mut l);
                l
            }
            Statement::Expr(e) => {
                let mut l = out;
                uses(e, &mut l);
                l
            }
            Statement::If { condition, then_body, else_body, .. } => {
                let mut l = self.block(then_body, out.clone(), j);
                l.extend(self.block(else_body, out, j));
                uses(condition, &mut l);
                l
            }
            Statement::While { condition, body } => self.while_loop(Some(s), condition, body, &out),
            Statement::DoWhile { condition, body } => self.looped(Some(s), &out, |me, h| {
                let mut test = h.clone();
                uses(condition, &mut test);
                me.block(body, test.clone(), &Jumps { brk: Some(out.clone()), cont: Some(test) })
            }),
            Statement::ForC { init, condition, update, body } => {
                let head = self.looped(Some(s), &out, |me, h| {
                    let next = me.stmt(update, h.clone(), j);
                    let mut l = me.block(body, next.clone(), &Jumps { brk: Some(out.clone()), cont: Some(next) });
                    uses(condition, &mut l);
                    l
                });
                self.stmt(init, head, j)
            }
            Statement::For { var, count, body } => {
                let mut head = self.looped(Some(s), &out, |me, h| {
                    let mut l = me.block(body, h.clone(), &Jumps { brk: Some(out.clone()), cont: Some(h.clone()) });
                    l.insert(var.clone());
                    l
                });
                head.remove(var);
                uses(count, &mut head);
                head
            }
            Statement::Return { value } => {
                let mut l = BTreeSet::new();
                if let Some(v) = value {
                    uses(v, &mut l);
                }
                l
            }
            Statement::Break => j.brk.clone().unwrap_or(out),
            Statement::Continue => j.cont.clone().unwrap_or(out),
            Statement::Switch { expr, cases, default } => {
                let inner = Jumps { brk: Some(out.clone()), cont: j.cont.clone() };
                let mut l = out.clone();
                for (_, b) in cases {
                    l.extend(self.block(b, out.clone(), &inner));
                }
                l.extend(self.block(default, out, &inner));
                uses(expr, &mut l);
                l
            }
            Statement::Block { body } => self.block(body, out, j),
            Statement::CircBuf(_) => {
                let mut l = out;
                crate::streaming::walk_stmt_exprs(s, &mut |e| uses(e, &mut l));
                l
            }
            Statement::Comment(_) | Statement::UnrollHint { .. } => out,
        }
    }
}

#[cfg(test)]
mod tests {
    use super::{branched_loops, capped, live_after_loops, plan, Contract, Env, Refusal};
    use crate::backends::go_select::addr;
    use crate::backends::lockstep::{bin, int, step, var};
    use crate::ir::{BinOp, Expr, Statement};
    use std::collections::{BTreeSet, HashMap, HashSet};

    fn at(a: &str, i: Expr) -> Expr {
        Expr::ArrayAccess(a.into(), Box::new(i))
    }
    fn set(t: Expr, e: Expr) -> Statement {
        Statement::Assign { target: t, value: e, compound: false }
    }
    fn run(cond: &Expr, body: &[Statement], update: Option<&Statement>) -> Result<super::Plan, Refusal> {
        run_in(cond, body, update, false)
    }
    fn run_in(cond: &Expr, body: &[Statement], update: Option<&Statement>, branched: bool) -> Result<super::Plan, Refusal> {
        with_env(branched, |env| plan(cond, body, update, env))
    }
    fn run_nested(cond: &Expr, body: &[Statement]) -> Result<super::Plan, Refusal> {
        with_env(false, |env| plan(cond, body, None, &Env { nested: true, ..*env }))
    }
    /// `r` and every real literal are reals.
    fn int_expr(e: &Expr) -> bool {
        let mut real = false;
        crate::streaming::walk_expr(e, &mut |x| real |= matches!(x, Expr::Literal(_)) || matches!(x, Expr::Var(n) if n == "r"));
        !real
    }
    fn with_env<R>(branched: bool, f: impl FnOnce(&Env) -> R) -> R {
        let int_local = |n: &str| ["i", "o", "t", "n", "h", "j"].contains(&n);
        let contract = |n: &str| match n {
            "in" | "vol" => Some(Contract::Input),
            "out" => Some(Contract::Output),
            "s" => Some(Contract::Scratch),
            _ => None,
        };
        let fixed_end = |e: &Expr| matches!(e, Expr::Var(n) if n == "endIdx");
        let cross = |n: &str| n == "TA_SMA";
        let product = |e: &Expr| matches!(e, Expr::BinOp(_, BinOp::Mul, _));
        let fused = |e: &Expr| matches!(e, Expr::BinOp(l, BinOp::Add, r) if product(l) || product(r));
        f(&Env { int_local: &int_local, contract: &contract, fixed_end: &fixed_end, cross_call: &cross, int_expr: &int_expr, fused: &fused, branched, nested: false })
    }
    fn lanes(p: &super::Plan) -> Vec<(String, String, i64)> {
        p.lanes.iter().map(|l| (l.array.clone(), l.var.clone(), l.elem)).collect()
    }
    fn le_end() -> Expr {
        bin(var("i"), BinOp::LessEq, var("endIdx"))
    }

    /// `out[o++] = in[i++] - in[t++]` steps three indices in one statement,
    /// and a comma update steps two.
    #[test]
    fn steps_in_one_statement_or_a_comma_update_are_indices() {
        let post = |v: &str| Box::new(Expr::PostIncrement(Box::new(var(v))));
        let mom = set(Expr::ArrayAccess("out".into(), post("o")), bin(Expr::ArrayAccess("in".into(), post("i")), BinOp::Sub, Expr::ArrayAccess("in".into(), post("t"))));
        let p = run(&le_end(), &[mom], None).expect("planned");
        assert_eq!(lanes(&p), vec![("in".into(), "i".into(), 0), ("in".into(), "t".into(), 0), ("out".into(), "o".into(), 0)]);
        let comma = Statement::Block { body: vec![step("i", 1), step("o", 1)] };
        let add = set(at("out", var("o")), bin(at("in", var("i")), BinOp::Add, at("vol", var("i"))));
        let p = run(&le_end(), &[add], Some(&comma)).expect("planned");
        assert_eq!(p.lanes.len(), 3);
        assert_eq!(p.pass.iter().filter(|s| p.is_step(s)).count(), 2);
    }

    /// `while( n != 0 ) { ...; n--; }` counts down; its other indices count up.
    #[test]
    fn a_count_down_test_with_indices_counting_up() {
        let body = [set(at("out", var("o")), at("in", var("h"))), step("o", 1), step("h", 1), step("n", -1)];
        let p = run(&bin(var("n"), BinOp::NotEq, int(0)), &body, None).expect("planned");
        assert!(matches!(p.trip, super::Trip::Down { bound: None }));
        assert_eq!(p.lanes.len(), 2);
    }

    /// A read in a branch is re-sliced where an every-pass read of the same
    /// array, or of any input for an input, bounds it, or where the counter
    /// stops at `endIdx` above an every-pass element; an output written in a
    /// branch, or an input read ahead of the counter, is not.
    #[test]
    fn a_read_in_a_branch_is_cut_only_where_the_public_tier_bounds_it() {
        let branch = |then_body: Vec<Statement>| Statement::If { condition: var("c"), then_body, else_body: vec![], cond_comments: vec![] };
        let pooled = [set(var("x"), at("in", var("i"))), branch(vec![set(var("y"), at("vol", var("i")))]), step("i", 1)];
        let p = run(&le_end(), &pooled, None).expect("planned");
        assert!(lanes(&p).contains(&("vol".into(), "i".into(), 0)));
        let lt = bin(var("i"), BinOp::Less, var("endIdx"));
        assert!(lanes(&run(&lt, &pooled, None).expect("planned")).contains(&("vol".into(), "i".into(), 0)));

        let behind = bin(var("i"), BinOp::Sub, int(1));
        let to_end = [set(at("out", behind), var("x")), branch(vec![set(var("y"), at("vol", var("i")))]), step("i", 1)];
        let p = run(&le_end(), &to_end, None).expect("planned");
        assert!(lanes(&p).contains(&("vol".into(), "i".into(), 0)));
        let p = run(&lt, &to_end, None).expect("planned");
        assert!(!lanes(&p).iter().any(|l| l.0 == "vol"), "`<` does not stop at endIdx");

        let ahead = [set(var("x"), at("in", var("i"))), branch(vec![set(var("y"), at("vol", bin(var("i"), BinOp::Add, int(1))))]), step("i", 1)];
        assert_eq!(lanes(&run(&le_end(), &ahead, None).expect("planned")), vec![("in".into(), "i".into(), 0)]);
        let written = [set(var("x"), at("in", var("i"))), branch(vec![set(at("out", var("i")), var("x"))]), step("i", 1)];
        assert_eq!(lanes(&run(&le_end(), &written, None).expect("planned")), vec![("in".into(), "i".into(), 0)]);
    }

    /// `while( i > b ) { i--; s[i-b] = in[i]; }` counts down over an input and
    /// a scratch array, each cut from one past the top element.
    #[test]
    fn a_count_down_index_cuts_its_lanes_from_the_top() {
        let body = [step("i", -1), set(at("s", bin(var("i"), BinOp::Sub, var("b"))), at("in", var("i")))];
        let p = run(&bin(var("i"), BinOp::Greater, var("b")), &body, None).expect("planned");
        assert!(p.down);
        assert_eq!(lanes(&p), vec![("in".into(), "i".into(), -1), ("s".into(), "i".into(), -1)]);
        assert_eq!(format!("{:?}", p.lanes[0].start), format!("{:?}", var("i")));
    }

    /// A step both arms take last is a step of the pass, and an element both
    /// arms write is one every pass reaches; written on one arm, neither.
    #[test]
    fn a_step_both_arms_take_last_moves_after_the_branch() {
        let arm = |v: Expr| vec![set(Expr::ArrayAccess("out".into(), Box::new(Expr::PostIncrement(Box::new(var("o"))))), v)];
        let branch = |else_body| Statement::If { condition: var("c"), then_body: arm(at("in", var("i"))), else_body, cond_comments: vec![] };
        let p = run(&le_end(), &[branch(arm(Expr::Literal(0.0))), step("i", 1)], None).expect("planned");
        assert_eq!(p.steps.get("o"), Some(&1));
        assert!(lanes(&p).contains(&("out".into(), "o".into(), 0)));
        assert_eq!(run(&le_end(), &[branch(vec![set(var("x"), int(0))]), step("i", 1)], None).err(), Some(Refusal::NoLane));
    }

    /// The refusals, one per class.
    #[test]
    fn what_is_refused() {
        let read = set(var("x"), at("in", var("i")));
        assert_eq!(run(&bin(var("x"), BinOp::LessEq, var("endIdx")), &[read.clone(), step("i", 1)], None).err(), Some(Refusal::Header));
        assert_eq!(run(&bin(var("n"), BinOp::LessEq, var("endIdx")), &[read.clone(), step("i", 1)], None).err(), Some(Refusal::Counter));
        assert_eq!(run(&le_end(), &[read.clone(), Statement::Break, step("i", 1)], None).err(), Some(Refusal::Exit));
        let call = Statement::Expr(Expr::FuncCall("TA_SMA".into(), vec![var("i")]));
        assert_eq!(run(&le_end(), &[read.clone(), call, step("i", 1)], None).err(), Some(Refusal::CrossCall));
        assert_eq!(run(&le_end(), &[read.clone(), step("i", 1), step("i", 1)], None).err(), Some(Refusal::Counter));
        assert_eq!(run(&le_end(), &[set(var("x"), at("scratch", var("i"))), step("i", 1)], None).err(), Some(Refusal::NoLane));
        let moved = set(var("endIdx"), var("x"));
        assert_eq!(run(&le_end(), &[read.clone(), moved, step("i", 1)], None).err(), Some(Refusal::Counter));
        assert_eq!(run(&bin(var("i"), BinOp::LessEq, var("r")), &[read.clone(), step("i", 1)], None).err(), Some(Refusal::Counter));
        // `h` keeps where the extreme was, read before it is written, and
        // takes a value no index expression gives.
        let track = Statement::If {
            condition: bin(var("h"), BinOp::Less, var("t")),
            then_body: vec![set(var("h"), bin(var("i"), BinOp::Mul, int(2)))],
            else_body: vec![],
            cond_comments: vec![],
        };
        let body = [read.clone(), track, set(at("out", var("o")), var("h")), step("o", 1), step("t", 1), step("i", 1)];
        assert_eq!(run(&le_end(), &body, None).err(), Some(Refusal::IndexTracking));
        // A window start taken from an index each pass is no tracking.
        let reseed = Statement::If {
            condition: var("c"),
            then_body: vec![set(var("j"), bin(var("i"), BinOp::Sub, var("p"))), set(var("x"), at("in", var("j")))],
            else_body: vec![],
            cond_comments: vec![],
        };
        assert!(run(&le_end(), &[read.clone(), reseed, step("i", 1)], None).is_ok());
        assert_eq!(run_in(&le_end(), &[read, step("i", 1)], None, true).err(), Some(Refusal::Branched));
    }

    /// A call, a carried scalar and a ring together are refused, a call in a
    /// loop inside the pass included; any two of them are not.
    #[test]
    fn a_stateful_pass_that_calls_out_over_a_ring_is_refused() {
        let call = |e: Expr| Expr::FuncCall("exp".into(), vec![e]);
        let pass = |x: Expr, ring: bool| {
            let mut v = vec![set(var("x"), x), set(at("out", var("o")), var("x"))];
            if ring {
                v.push(set(at("ring", var("h")), var("x")));
            }
            v.extend([step("o", 1), step("i", 1)]);
            v
        };
        let carried = bin(var("x"), BinOp::Add, call(at("in", var("i"))));
        assert_eq!(run(&le_end(), &pass(carried.clone(), true), None).err(), Some(Refusal::CallsOverRing));
        assert!(run(&le_end(), &pass(carried, false), None).is_ok(), "no ring");
        assert!(run(&le_end(), &pass(call(at("in", var("i"))), true), None).is_ok(), "nothing carried");
        let sqrt = bin(var("x"), BinOp::Add, Expr::FuncCall("sqrt".into(), vec![at("in", var("i"))]));
        assert!(run(&le_end(), &pass(sqrt, true), None).is_ok(), "no call gc keeps");
        let inner = Statement::While { condition: bin(var("j"), BinOp::Less, var("e")), body: vec![set(var("y"), call(var("y"))), step("j", 1)] };
        let mut nested = pass(bin(var("x"), BinOp::Add, at("in", var("i"))), true);
        nested.insert(0, inner);
        assert_eq!(run(&le_end(), &nested, None).err(), Some(Refusal::CallsOverRing));
    }

    /// `y = a1*y1 + (a2*y2 + c0*in[i]); y2 = y1; y1 = y` is refused; a
    /// delayed copy no fused sum reads, one assigned on a branch too, and a
    /// one-tap recurrence are not.
    #[test]
    fn a_two_variable_shift_into_a_fused_sum_is_refused() {
        let prod = |a: &str, b: Expr| bin(var(a), BinOp::Mul, b);
        let filter = |older: Expr| bin(prod("a1", var("y1")), BinOp::Add, bin(prod("a2", older), BinOp::Add, prod("c0", at("in", var("i")))));
        let pass = |y: Expr, extra: Vec<Statement>| {
            let mut v = vec![set(var("y"), y), set(var("y2"), var("y1")), set(var("y1"), var("y"))];
            v.extend(extra);
            v.extend([set(at("out", var("o")), bin(var("y"), BinOp::Sub, var("y2"))), step("o", 1), step("i", 1)]);
            v
        };
        assert_eq!(run(&le_end(), &pass(filter(var("y2")), vec![]), None).err(), Some(Refusal::FusedShift));
        assert!(run(&le_end(), &pass(filter(var("z")), vec![]), None).is_ok(), "the delayed copy feeds no fused sum");
        let reset = Statement::If { condition: var("c"), then_body: vec![set(var("y2"), Expr::Literal(0.0))], else_body: vec![], cond_comments: vec![] };
        assert!(run(&le_end(), &pass(filter(var("y2")), vec![reset]), None).is_ok(), "the delayed copy is assigned twice");
        let one_tap = [set(var("y"), bin(prod("a1", var("y")), BinOp::Add, prod("c0", at("in", var("i"))))), set(at("out", var("o")), var("y")), step("o", 1), step("i", 1)];
        assert!(run(&le_end(), &one_tap, None).is_ok(), "a one-tap recurrence");
    }

    /// MAXINDEX's pass: `x = in[i]`; when `h` leaves the window, `h = t`,
    /// `m = in[h]`, `j = h`, `while( ++j <= i )` keeping the best index;
    /// else a new extreme at `i`; `out[o] = h`.
    fn tracking_pass(rescan_tail: Vec<Statement>, after: Vec<Statement>) -> Vec<Statement> {
        let better = |v: &str| Statement::If { condition: bin(var("x"), BinOp::GreaterEq, var("m")), then_body: vec![set(var("h"), var(v)), set(var("m"), var("x"))], else_body: vec![], cond_comments: vec![] };
        let mut rescan = vec![set(var("x"), at("in", var("j"))), better("j")];
        rescan.extend(rescan_tail);
        let cond = bin(Expr::PreIncrement(Box::new(var("j"))), BinOp::LessEq, var("i"));
        let mut then_body = vec![set(var("h"), var("t")), set(var("m"), at("in", var("h"))), set(var("j"), var("h")), Statement::While { condition: cond, body: rescan }];
        then_body.extend(after);
        let track = Statement::If { condition: bin(var("h"), BinOp::Less, var("t")), then_body, else_body: vec![better("i")], cond_comments: vec![] };
        vec![set(var("x"), at("in", var("i"))), track, set(at("out", var("o")), var("h")), step("o", 1), step("t", 1), step("i", 1)]
    }

    /// An index-tracking scan to `endIdx` holds its counter, the window's
    /// trailing index and the tracked index in `int`s, reads the output only
    /// through its lane, and keeps the rescan counter to its rescan.
    #[test]
    fn an_index_tracking_scan_holds_its_indices_in_ints() {
        let p = run(&le_end(), &tracking_pass(vec![], vec![]), None).expect("planned");
        let scan = p.scan.as_ref().expect("a scan");
        assert_eq!(scan.ints, BTreeSet::from(["h".to_string(), "i".into(), "t".into()]));
        assert_eq!(scan.rescans, BTreeSet::from(["j".to_string()]));
        assert_eq!(scan.inputs, vec!["in".to_string()]);
        assert!(p.unroll.is_none());
        assert!(lanes(&p).contains(&("out".into(), "o".into(), 0)));

        let lt = bin(var("i"), BinOp::Less, var("endIdx"));
        assert_eq!(run(&lt, &tracking_pass(vec![], vec![]), None).err(), Some(Refusal::IndexTracking), "a bound short of endIdx");
        let read_after = vec![set(var("y"), var("j"))];
        assert_eq!(run(&le_end(), &tracking_pass(vec![], read_after), None).err(), Some(Refusal::IndexTracking), "the rescan counter read after its rescan");
        let moves_bound = vec![step("i", 1)];
        assert_eq!(run(&le_end(), &tracking_pass(moves_bound, vec![]), None).err(), Some(Refusal::Counter), "the rescan moves the counter");
        let mut scaled = tracking_pass(vec![], vec![]);
        scaled[0] = set(var("x"), at("in", bin(var("i"), BinOp::Mul, int(2))));
        assert_eq!(run(&le_end(), &scaled, None).err(), Some(Refusal::IndexTracking), "an input read at no index expression");
    }

    /// A pass that depends on no other is unrolled: eight per group for one
    /// store to an element, four for more or a branch; a carried scalar, an
    /// index read other than as a lane, a call gc keeps, counting down or a
    /// loop inside another are not.
    #[test]
    fn independent_passes_are_unrolled() {
        let post = |v: &str| Box::new(Expr::PostIncrement(Box::new(var(v))));
        let elem = |a: &str, v: &str| Expr::ArrayAccess(a.into(), post(v));
        let map = |e: Expr| set(Expr::ArrayAccess("out".into(), post("o")), e);
        let add = map(bin(at("in", var("i")), BinOp::Add, at("vol", var("i"))));
        let p = run(&le_end(), &[add.clone(), step("i", 1)], None).expect("planned");
        assert_eq!((p.unroll, p.calls_out), (Some(8), false));

        let fresh = [set(var("x"), at("in", var("i"))), set(var("y"), bin(var("x"), BinOp::Sub, at("vol", var("i")))), set(at("out", var("o")), var("y")), step("o", 1), step("i", 1)];
        assert_eq!(run(&le_end(), &fresh, None).expect("planned").unroll, Some(4));
        let guarded = Statement::If { condition: bin(var("x"), BinOp::Greater, Expr::Literal(0.0)), then_body: vec![set(var("x"), Expr::Literal(0.0))], else_body: vec![], cond_comments: vec![] };
        let branch = [set(var("x"), at("in", var("i"))), guarded, map(var("x")), step("i", 1)];
        assert_eq!(run(&le_end(), &branch, None).expect("planned").unroll, Some(4));

        let carried = [Statement::Assign { target: var("x"), value: bin(var("x"), BinOp::Add, elem("in", "i")), compound: true }, map(var("x"))];
        assert_eq!(run(&le_end(), &carried, None).expect("planned").unroll, None, "a carried sum");
        let index = [map(Expr::Cast(crate::ir::VarType::Real, Box::new(var("i")))), step("i", 1)];
        assert_eq!(run(&le_end(), &index, None).expect("planned").unroll, None, "an index read as a value");
        let exp = run(&le_end(), &[map(Expr::FuncCall("exp".into(), vec![at("in", var("i"))])), step("i", 1)], None).expect("planned");
        assert_eq!((exp.unroll, exp.calls_out), (None, true));
        let sqrt = run(&le_end(), &[map(Expr::FuncCall("sqrt".into(), vec![at("in", var("i"))])), step("i", 1)], None).expect("planned");
        assert_eq!((sqrt.unroll, sqrt.calls_out), (Some(8), false));
        assert_eq!(run_nested(&le_end(), &[add, step("i", 1)]).expect("planned").unroll, None, "inside another loop");
        let down = [step("i", -1), set(at("s", bin(var("i"), BinOp::Sub, var("b"))), at("in", var("i")))];
        let p = run(&bin(var("i"), BinOp::Greater, var("b")), &down, None).expect("planned");
        assert!(p.down && p.unroll.is_none(), "counting down");
    }

    /// Of a loop over bars, a loop on an arm is branched at any depth, unless
    /// every arm of an `if` with an `else` runs a loop the plan takes; one run
    /// on every pass is not; nor is a loop on an arm of a block scan's pass
    /// loop, which moves its index by a block, or of no loop at all.
    #[test]
    fn a_loop_on_an_arm_of_a_loop_over_bars_is_branched() {
        let inner = |v: &str| Statement::While { condition: bin(var(v), BinOp::Less, var("e")), body: vec![set(var("x"), at("in", var(v))), step(v, 1)] };
        let arm = |then_body: Vec<Statement>, else_body: Vec<Statement>| Statement::If { condition: var("c"), then_body, else_body, cond_comments: vec![] };
        let bars = Statement::DoWhile {
            condition: le_end(),
            body: vec![
                inner("j"),
                arm(vec![inner("j"), arm(vec![inner("h")], vec![])], vec![]),
                arm(vec![inner("j")], vec![inner("h")]),
                arm(vec![inner("j")], vec![set(var("x"), int(0))]),
                step("i", 1),
            ],
        };
        let block = Statement::While { condition: bin(var("t"), BinOp::LessEq, var("endIdx")), body: vec![inner("j"), step("t", 1), arm(vec![inner("j")], vec![inner("h"), set(var("t"), bin(var("t"), BinOp::Add, var("n")))])] };
        let body = vec![arm(vec![inner("j")], vec![]), bars, block];
        let mut got = HashSet::new();
        with_env(false, |env| branched_loops(&body, env, &mut got));
        let Statement::DoWhile { body: b, .. } = &body[1] else { unreachable!() };
        let Statement::If { then_body: t, .. } = &b[1] else { unreachable!() };
        let Statement::If { then_body: t2, .. } = &t[1] else { unreachable!() };
        let Statement::If { then_body: half, .. } = &b[3] else { unreachable!() };
        assert_eq!(got, HashSet::from([addr(&t[0]), addr(&t2[0]), addr(&half[0])]));
    }

    #[test]
    fn a_padded_loop_and_its_near_misses() {
        let init = set(var("i"), int(0));
        let cond = bin(var("i"), BinOp::Less, int(50));
        let guarded = |b: Expr, s: Vec<Statement>| vec![Statement::If { condition: bin(var("i"), BinOp::Less, b), then_body: s, else_body: vec![], cond_comments: vec![] }];
        let sum = vec![Statement::Assign { target: var("s"), value: at("in", bin(var("t"), BinOp::Sub, var("i"))), compound: true }];
        let body = guarded(var("p"), sum.clone());
        let c = capped(&init, &cond, &step("i", 1), &body, &int_expr).expect("capped");
        assert_eq!((c.var.as_str(), c.cap), ("i", 50));
        let writes_bound = guarded(var("p"), vec![set(var("p"), int(3))]);
        assert!(capped(&init, &cond, &step("i", 1), &writes_bound, &int_expr).is_none());
        let writes_var = guarded(var("p"), vec![set(var("i"), int(3))]);
        assert!(capped(&init, &cond, &step("i", 1), &writes_var, &int_expr).is_none());
        let mut with_else = guarded(var("p"), sum.clone());
        if let Statement::If { else_body, .. } = &mut with_else[0] {
            else_body.push(Statement::Break);
        }
        assert!(capped(&init, &cond, &step("i", 1), &with_else, &int_expr).is_none());
        let mut two = guarded(var("p"), sum);
        two.push(set(var("x"), int(1)));
        assert!(capped(&init, &cond, &step("i", 1), &two, &int_expr).is_none());
        assert!(capped(&init, &cond, &step("i", 1), &guarded(var("r"), vec![set(var("x"), int(1))]), &int_expr).is_none(), "a real guard");
    }

    #[test]
    fn liveness_sees_reads_after_a_loop_and_in_the_next_outer_pass() {
        let inner = Statement::While { condition: bin(var("i"), BinOp::Less, var("e")), body: vec![step("i", 1)] };
        let body = vec![set(var("i"), int(0)), inner, set(var("r"), var("i"))];
        let mut map = HashMap::new();
        live_after_loops(&body, BTreeSet::new(), &mut map);
        assert!(map[&super::super::go_select::addr(&body[1])].contains("i"));
        let dead = vec![set(var("i"), int(0)), body[1].clone(), set(var("i"), int(4)), set(var("r"), var("i"))];
        let mut map = HashMap::new();
        live_after_loops(&dead, BTreeSet::new(), &mut map);
        assert!(!map[&super::super::go_select::addr(&dead[1])].contains("i"));
        // Read at the top of the next pass of an enclosing loop.
        let outer = vec![Statement::While { condition: var("c"), body: vec![set(var("r"), var("i")), dead[1].clone()] }];
        let mut map = HashMap::new();
        live_after_loops(&outer, BTreeSet::new(), &mut map);
        let Statement::While { body: ob, .. } = &outer[0] else { unreachable!() };
        assert!(map[&super::super::go_select::addr(&ob[1])].contains("i"));
    }
}
