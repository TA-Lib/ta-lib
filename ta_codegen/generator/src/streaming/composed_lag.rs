//! Every series read of a composed body: the definition it reads, its exact
//! delay behind that definition's newest element, and the class the delay
//! lowers to. A class only picks the shape: Open captures every tap depth and
//! checks every presumption on the exact counts.

use std::collections::{BTreeMap, BTreeSet};

use super::{
    after_call_guard, batch_call_out_args, collect_var_decls, cursor_index_offset, exprs_equal,
    for_each_stmt, input_array_names, map_loop_bound, map_loop_cursor, nested_bodies, own_writes,
    producer_shape, recv_read_expr, recv_var, walk_expr, CalleeLookup, ComposedPlan, Consumer,
    DefSite, DelayLine, Edge, EdgeClass, LineTap, OpenAction, OpenAt, SeriesDef, StreamError,
    TapBack, UpdateStep,
};
use crate::ir::{BinOp, Expr, FuncDef, Statement, VarType};
use crate::ring_lag::{ComposedWalk, Val};

/// The delay half of a composed plan.
#[derive(Debug, Clone)]
pub struct Classified {
    /// A must-alias name to its class key.
    pub alias_keys: BTreeMap<String, String>,
    pub defs: Vec<SeriesDef>,
    pub edges: Vec<Edge>,
    pub lines: Vec<DelayLine>,
    /// Snapshotted definitions in body order; position `s` is Open's `lagNb<s>`.
    pub snapshots: Vec<usize>,
    /// In emission order.
    pub open_actions: Vec<(OpenAt, OpenAction)>,
}

const SLIDES: &str = "slides with endIdx";
const OPAQUE: &str = "depends on data or an unmodelled value";
const AHEAD: &str = "reads ahead";
const NOT_AFFINE: &str = "index is not affine in the cursor";
const SELF_OFFSET: &str = "reads the map's own output at an offset";
const NONZERO_RETURN: &str = "non-Zero return edge";
const PRODUCER: &str = "producer series is not provably one element per bar";
const MAY_ALIAS: &str = "a may-alias name was written";
const END_WRITTEN: &str = "endIdx is rewritten";
const NO_PRIOR: &str = "a partial map write has no prior definition";
const START_PAIRED: &str = "pairs bars at a distance that depends on where the batch started";
const BEGIN_SUB: &str = "subtracts a begin index with no guard proving it non-negative";

fn refusal(what: &str, buffer: &str, reason: &str) -> StreamError {
    StreamError::Unsupported(format!(
        "composed {what} reads `{buffer}` at a delay that is not a stream constant ({reason}): it \
         must be a same-bar shift (an element-count difference, as APO's `fastNb - *outNBElement`) \
         or a lag fixed by the parameters"
    ))
}

#[derive(Debug, Clone, Copy, PartialEq, Eq)]
enum Kind {
    Zero,
    Presumed,
    Tap { ge1: bool },
}

// ---------------------------------------------------------------------------
// Aliases
// ---------------------------------------------------------------------------

#[derive(Default)]
struct Aliases {
    /// A must-alias name to its class's first name in body order.
    key: BTreeMap<String, String>,
    /// A key to the other keys of its may-alias class.
    may: BTreeMap<String, BTreeSet<String>>,
}

impl Aliases {
    fn key_of(&self, name: &str) -> String {
        self.key.get(name).cloned().unwrap_or_else(|| name.to_string())
    }
}

/// The series a pointer copy `a = <value>` may leave `a` sharing.
fn copied_series(value: &Expr, series: &BTreeSet<String>) -> BTreeSet<String> {
    let mut v = value;
    while let Expr::Cast(_, inner) = v {
        v = inner;
    }
    let mut out = BTreeSet::new();
    if matches!(v, Expr::FuncCall(..)) {
        return out;
    }
    walk_expr(v, &mut |x| match x {
        Expr::Var(n) if series.contains(n) => {
            out.insert(n.clone());
        }
        Expr::AddressOf(t) => {
            if let Expr::ArrayAccess(n, _) = t.as_ref() {
                if series.contains(n) {
                    out.insert(n.clone());
                }
            }
        }
        _ => {}
    });
    out
}

fn pointer_assign(s: &Statement) -> Option<(&String, &Expr)> {
    match s {
        Statement::Assign { target: Expr::Var(a), value, .. }
        | Statement::VarDecl { name: a, init: Some(value), .. } => Some((a, value)),
        _ => None,
    }
}

fn union(parent: &mut BTreeMap<String, String>, a: &str, b: &str) {
    let ra = find(parent, a);
    let rb = find(parent, b);
    if ra != rb {
        parent.insert(ra, rb);
    }
}

fn find(parent: &mut BTreeMap<String, String>, a: &str) -> String {
    let mut cur = a.to_string();
    while let Some(p) = parent.get(&cur).filter(|p| **p != cur) {
        cur = p.clone();
    }
    cur
}

/// `s` and every statement nested in it, in pre-order; `top` marks `s` itself.
fn push_all<'s>(s: &'s Statement, top: bool, out: &mut Vec<(&'s Statement, bool)>) {
    out.push((s, top));
    for body in nested_bodies(s).0 {
        for st in body {
            push_all(st, false, out);
        }
    }
}

/// Must-alias: an unconditional top-level `a = b` whose `a` is assigned nowhere
/// else and whose `b` is not reassigned after it. Every other pointer copy is
/// may-alias.
#[allow(clippy::too_many_lines)]
fn aliases(func: &FuncDef, stmts: &[&Statement]) -> Aliases {
    let mut decls = BTreeMap::new();
    collect_var_decls(func.stream_source(), &mut decls);
    let series: BTreeSet<String> = decls
        .into_iter()
        .filter(|(_, t)| {
            matches!(
                t,
                VarType::RealPointer | VarType::IntPointer | VarType::RealArray(_) | VarType::IntArray(_)
            )
        })
        .map(|(n, _)| n)
        .chain(input_array_names(func))
        .chain(func.outputs.iter().map(|o| o.name.clone()))
        .collect();

    // Pre-order position of every statement, nested ones included.
    let mut flat: Vec<(&Statement, bool)> = Vec::new();
    for top in stmts {
        push_all(top, true, &mut flat);
    }

    let mut first: BTreeMap<String, usize> = BTreeMap::new();
    for (pos, (s, _)) in flat.iter().enumerate() {
        let mut names = BTreeSet::new();
        if let Statement::VarDecl { name, .. } = s {
            names.insert(name.clone());
        }
        super::walk_stmt_own_exprs(s, &mut |e| super::expr_var_names(e, &mut names));
        for n in names {
            first.entry(n).or_insert(pos);
        }
    }
    let assigned_at = |name: &str| -> Vec<usize> {
        flat.iter()
            .enumerate()
            .filter(|(_, (s, _))| pointer_assign(s).is_some_and(|(a, _)| a == name))
            .map(|(p, _)| p)
            .collect()
    };

    let mut must: BTreeMap<String, String> = BTreeMap::new();
    let mut may_pairs: Vec<(String, String)> = Vec::new();
    for (pos, (s, top)) in flat.iter().enumerate() {
        let Some((a, value)) = pointer_assign(s) else { continue };
        if !series.contains(a) {
            continue;
        }
        let srcs = copied_series(value, &series);
        if srcs.is_empty() {
            continue;
        }
        let mut plain = value;
        while let Expr::Cast(_, inner) = plain {
            plain = inner;
        }
        let is_must = *top
            && matches!(plain, Expr::Var(b) if srcs.contains(b))
            && assigned_at(a) == [pos]
            && match plain {
                Expr::Var(b) => assigned_at(b).iter().all(|p| *p < pos),
                _ => false,
            };
        for b in srcs {
            if is_must {
                union(&mut must, a, &b);
            } else {
                may_pairs.push((a.clone(), b));
            }
        }
    }
    let mut classes: BTreeMap<String, Vec<String>> = BTreeMap::new();
    let names: Vec<String> = must.keys().cloned().chain(must.values().cloned()).collect();
    for n in names {
        let root = find(&mut must, &n);
        let class = classes.entry(root).or_default();
        if !class.contains(&n) {
            class.push(n);
        }
    }
    let mut al = Aliases::default();
    for class in classes.into_values() {
        let key = class
            .iter()
            .min_by_key(|n| (first.get(*n).copied().unwrap_or(usize::MAX), (*n).clone()))
            .cloned()
            .expect("a class has a name");
        for n in class {
            al.key.insert(n, key.clone());
        }
    }
    let mut may: BTreeMap<String, String> = BTreeMap::new();
    for (a, b) in &may_pairs {
        union(&mut may, &al.key_of(a), &al.key_of(b));
    }
    let mut groups: BTreeMap<String, BTreeSet<String>> = BTreeMap::new();
    let keys: Vec<String> = may.keys().cloned().chain(may.values().cloned()).collect();
    for k in keys {
        let root = find(&mut may, &k);
        groups.entry(root).or_default().insert(k);
    }
    for group in groups.into_values() {
        for k in &group {
            let others: BTreeSet<String> = group.iter().filter(|o| *o != k).cloned().collect();
            if !others.is_empty() {
                al.may.insert(k.clone(), others);
            }
        }
    }
    al
}

// ---------------------------------------------------------------------------
// Small syntactic helpers
// ---------------------------------------------------------------------------

/// Every scalar a statement may write, `*p` for a write through `p`.
fn scalar_writes(s: &Statement) -> BTreeSet<String> {
    let mut out = BTreeSet::new();
    for_each_stmt(std::slice::from_ref(s), &mut |st| {
        own_writes(st, &mut |t| match t {
            Expr::Var(v) => {
                out.insert(v.clone());
            }
            Expr::PointerDeref(p) => {
                out.insert(format!("*{p}"));
            }
            _ => {}
        });
    });
    out
}

/// The scalars `e` reads, `*p` for a read through `p`.
fn scalar_reads(e: &Expr) -> BTreeSet<String> {
    let mut out = BTreeSet::new();
    walk_expr(e, &mut |x| match x {
        Expr::Var(v) => {
            out.insert(v.clone());
        }
        Expr::PointerDeref(p) => {
            out.insert(format!("*{p}"));
        }
        _ => {}
    });
    out
}

/// Every `name[idx]` read in `e`, outermost first. `target` marks `e` as an
/// assignment target, whose element is written, not read; `&name[idx]` takes
/// an address and reads nothing.
fn array_reads(e: &Expr, target: bool, out: &mut Vec<(String, Expr)>) {
    match e {
        Expr::ArrayAccess(n, idx) => {
            if !target {
                out.push((n.clone(), idx.as_ref().clone()));
            }
            array_reads(idx, false, out);
        }
        Expr::AddressOf(inner) => match inner.as_ref() {
            Expr::ArrayAccess(_, idx) => array_reads(idx, false, out),
            other => array_reads(other, false, out),
        },
        Expr::Cast(_, x)
        | Expr::Not(x)
        | Expr::Neg(x)
        | Expr::BitwiseNot(x)
        | Expr::PostIncrement(x)
        | Expr::PostDecrement(x)
        | Expr::PreIncrement(x)
        | Expr::PreDecrement(x) => array_reads(x, false, out),
        Expr::BinOp(l, _, r) => {
            array_reads(l, false, out);
            array_reads(r, false, out);
        }
        Expr::Ternary(c, a, b) => {
            array_reads(c, false, out);
            array_reads(a, false, out);
            array_reads(b, false, out);
        }
        Expr::FuncCall(_, args) => {
            for a in args {
                array_reads(a, false, out);
            }
        }
        Expr::Literal(_) | Expr::IntLiteral(_) | Expr::Var(_) | Expr::PointerDeref(_) => {}
    }
}

fn stmt_array_reads(s: &Statement) -> Vec<(String, Expr)> {
    let mut out = Vec::new();
    for_each_stmt(std::slice::from_ref(s), &mut |st| match st {
        Statement::Assign { target, value, compound } => {
            array_reads(value, false, &mut out);
            array_reads(target, !compound, &mut out);
        }
        other => super::walk_stmt_own_exprs(other, &mut |e| array_reads(e, false, &mut out)),
    });
    out
}

fn is_memmove(s: &Statement) -> bool {
    matches!(s, Statement::Expr(Expr::FuncCall(n, a)) if n == "memmove" && a.len() == 3)
}

/// `E` of a memmove byte count `E * sizeof(..)` or `sizeof(..) * E`.
fn byte_count(e: &Expr) -> Option<Expr> {
    let is_sizeof = |x: &Expr| matches!(x, Expr::FuncCall(f, _) if f == "sizeof");
    match e {
        Expr::BinOp(l, BinOp::Mul, r) if is_sizeof(r) => Some(l.as_ref().clone()),
        Expr::BinOp(l, BinOp::Mul, r) if is_sizeof(l) => Some(r.as_ref().clone()),
        _ => None,
    }
}

/// `c++`, `++c` or `c = c + 1`.
fn is_unit_step(update: &Statement, cursor: &str) -> bool {
    let is_c = |e: &Expr| matches!(e, Expr::Var(v) if v == cursor);
    match update {
        Statement::Expr(Expr::PostIncrement(x) | Expr::PreIncrement(x)) => is_c(x),
        Statement::Assign { target, value: Expr::BinOp(l, BinOp::Add, r), .. } => {
            is_c(target) && is_c(l) && matches!(r.as_ref(), Expr::IntLiteral(1))
        }
        _ => false,
    }
}

fn arm_of(c: &Consumer) -> Option<(usize, bool)> {
    match c {
        Consumer::MapRead { step, arm: Some((then, _)), .. }
        | Consumer::MapPrior { step, arm: Some((then, _)), .. } => Some((*step, *then)),
        _ => None,
    }
}

// ---------------------------------------------------------------------------
// The walk
// ---------------------------------------------------------------------------

/// One map loop: a top-level map, or one loop of a variant-If arm.
struct Unit<'s> {
    arm: Option<(bool, usize)>,
    lp: &'s Statement,
    cursor: String,
    bound: Expr,
}

impl Unit<'_> {
    fn branch(&self) -> Option<bool> {
        self.arm.map(|(then, _)| then)
    }
}

/// What one map loop's body scan found.
#[derive(Default)]
struct BodyScan {
    /// Keys of the step read at the cursor before any write on some path.
    before: BTreeSet<String>,
    /// Keys of the step read at the cursor at all.
    self_reads: BTreeSet<String>,
    /// The written-key sets of paths that leave the body early.
    cut: Vec<BTreeSet<String>>,
    /// Distinct reads of series the step does not write, in body order.
    reads: Vec<(String, Expr)>,
}

/// What a map step's body scan sees of the step.
struct StepView<'k> {
    keys: &'k BTreeSet<String>,
    writes: &'k BTreeSet<String>,
}

type Place = Result<OpenAt, (usize, bool)>;

struct Cx<'p, 'a> {
    cp: &'p ComposedPlan<'a>,
    walk: ComposedWalk<'a>,
    al: Aliases,
    defs: Vec<SeriesDef>,
    counts: Vec<Val>,
    ordinals: BTreeMap<String, u32>,
    cur: BTreeMap<String, usize>,
    poisoned: BTreeSet<String>,
    edges: Vec<Edge>,
    delays: Vec<Option<Val>>,
    kinds: Vec<Kind>,
    /// Tail index and `n` of each Align step's memmove.
    aligns: BTreeMap<usize, (usize, Expr)>,
    /// `T` of each map loop, by step and arm.
    bounds: BTreeMap<(usize, Option<(bool, usize)>), Expr>,
}

pub(super) fn classify_plan(cp: &ComposedPlan, lookup: &dyn CalleeLookup) -> Result<Classified, StreamError> {
    let func = cp.func;
    let body = func.stream_source();
    let region_full = &body[..body.len() - cp.tail.len()];
    let stmts: Vec<&Statement> = cp.region.iter().chain(cp.tail.iter()).collect();
    if stmts.iter().any(|s| scalar_writes(s).contains("endIdx")) {
        return Err(refusal("body", "endIdx", END_WRITTEN));
    }
    let mut cx = Cx {
        cp,
        walk: ComposedWalk::new(func, lookup),
        al: aliases(func, &stmts),
        defs: Vec::new(),
        counts: Vec::new(),
        ordinals: BTreeMap::new(),
        cur: BTreeMap::new(),
        poisoned: BTreeSet::new(),
        edges: Vec::new(),
        delays: Vec::new(),
        kinds: Vec::new(),
        aligns: BTreeMap::new(),
        bounds: BTreeMap::new(),
    };
    let bars = cx.walk.end_idx().plus(&Val::konst(1), 1).expect("endIdx + 1");
    let bars_open = Expr::BinOp(
        Box::new(Expr::Var("endIdx".into())),
        BinOp::Add,
        Box::new(Expr::IntLiteral(1)),
    );
    for name in input_array_names(func) {
        cx.define(&name, DefSite::Input, bars.clone(), Some(bars_open.clone()));
    }
    cx.region(region_full);
    cx.tail()?;
    cx.finish()
}

impl Cx<'_, '_> {
    fn exec(&mut self, st: &Statement) {
        if self.walk.exec(std::slice::from_ref(st)).is_err() {
            self.walk.forget_writes(st);
        }
    }

    fn define(&mut self, name: &str, site: DefSite, count: Val, count_open: Option<Expr>) {
        let key = self.al.key_of(name);
        let ordinal = {
            let o = self.ordinals.entry(key.clone()).or_insert(0);
            *o += 1;
            *o
        };
        self.cur.insert(key.clone(), self.defs.len());
        self.defs.push(SeriesDef {
            buffer: key.clone(),
            name: name.to_string(),
            ordinal,
            site,
            count_open,
        });
        self.counts.push(count);
        self.poisoned.remove(&key);
        if let Some(others) = self.al.may.get(&key) {
            self.poisoned.extend(others.iter().cloned());
        }
    }

    fn read(&self, name: &str, what: &str) -> Result<usize, StreamError> {
        let key = self.al.key_of(name);
        if self.poisoned.contains(&key) {
            return Err(refusal(what, name, MAY_ALIAS));
        }
        self.cur.get(&key).copied().ok_or_else(|| refusal(what, name, OPAQUE))
    }

    /// A start argument, memmove offset or tail index: fixed for the stream.
    fn endpoint(v: &Val, what: &str, buffer: &str) -> Result<(), StreamError> {
        if v.subtracts_begin() {
            return Err(refusal(what, buffer, BEGIN_SUB));
        }
        if v.v_coef() != 0 {
            return Err(refusal(what, buffer, SLIDES));
        }
        if v.has_top() {
            return Err(refusal(what, buffer, OPAQUE));
        }
        Ok(())
    }

    fn neg(v: &Val) -> Option<Val> {
        Val::konst(0).plus(v, -1)
    }

    fn zero_interval(&self, v: &Val) -> bool {
        v.same(&Val::konst(0))
            || (v.is_param()
                && self.walk.at_least(v, 0)
                && Self::neg(v).is_some_and(|n| self.walk.at_least(&n, 0)))
    }

    fn ahead(&self, v: &Val) -> bool {
        Self::neg(v).is_some_and(|n| self.walk.at_least(&n, 1))
    }

    fn classify(&self, d: &Val, ret: bool) -> Result<Kind, &'static str> {
        if d.subtracts_begin() {
            return Err(BEGIN_SUB);
        }
        if d.v_coef() != 0 {
            return Err(SLIDES);
        }
        if d.has_top() {
            return Err(OPAQUE);
        }
        if d.is_param() {
            if self.zero_interval(d) {
                return Ok(Kind::Zero);
            }
            if self.ahead(d) {
                return Err(AHEAD);
            }
            if ret {
                return if self.walk.at_least(d, 1) { Err(NONZERO_RETURN) } else { Ok(Kind::Presumed) };
            }
            return Ok(Kind::Tap { ge1: self.walk.at_least(d, 1) });
        }
        let (p, nonpositive) = self.walk.presumed(d).ok_or(OPAQUE)?;
        if p.has_start() {
            return Err(START_PAIRED);
        }
        if !p.is_param() || self.zero_interval(&p) {
            return Ok(Kind::Presumed);
        }
        if self.ahead(&p) {
            return if nonpositive { Err(AHEAD) } else { Ok(Kind::Presumed) };
        }
        if ret {
            return Ok(Kind::Presumed);
        }
        Ok(Kind::Tap { ge1: false })
    }

    /// Records and classifies one read of definition `def`.
    fn edge(
        &mut self,
        def: usize,
        consumer: Consumer,
        delay: Option<Val>,
        what: &str,
        buffer: &str,
    ) -> Result<(), StreamError> {
        let n = &self.counts[def];
        let count_reason = (n.v_coef() != 1 || n.has_top())
            .then(|| if self.defs[def].site == DefSite::Producer { PRODUCER } else { OPAQUE });
        // An opaque delay over an unproven count is the count's failure.
        if let (true, Some(reason)) = (delay.as_ref().is_none_or(Val::has_top), count_reason) {
            return Err(refusal(what, &self.defs[def].buffer, reason));
        }
        let d = delay.ok_or_else(|| refusal(what, buffer, OPAQUE))?;
        let ret = matches!(consumer, Consumer::Return { .. });
        let kind = self.classify(&d, ret).map_err(|r| refusal(what, buffer, r))?;
        if kind != Kind::Zero || ret {
            if let Some(reason) = count_reason {
                return Err(refusal(what, &self.defs[def].buffer, reason));
            }
        }
        self.edges.push(Edge {
            def,
            consumer,
            class: EdgeClass::Zero,
        });
        self.delays.push(Some(d));
        self.kinds.push(kind);
        Ok(())
    }

    fn region(&mut self, region_full: &[Statement]) {
        let cp = self.cp;
        let n = cp.region.len();
        for (i, st) in cp.region.iter().enumerate() {
            if i + 1 == n && cp.producer.is_some() {
                self.producer(st, region_full);
            } else {
                self.exec(st);
            }
        }
    }

    /// The producer series holds `endIdx - cursor0 + 1` elements when its loop
    /// runs the cursor up to `endIdx` writing `series[paced++]` once per pass.
    fn producer(&mut self, lp: &Statement, region_full: &[Statement]) {
        let cp = self.cp;
        let series = cp.series.clone().expect("a producer writes a series");
        let paced: Option<String> = cp.producer.as_ref().and_then(|m| {
            let mut vars = m.out_index_vars.iter();
            match (vars.next(), vars.next()) {
                (Some(one), None) => Some(one.clone()),
                _ => None,
            }
        });
        if let Statement::ForC { init, .. } = lp {
            self.exec(init);
        }
        let end = self.walk.end_idx();
        let mut count = None;
        if let Some((cursor, bound)) = paced.as_deref().and_then(|p| producer_shape(region_full, &series, p)) {
            let c0 = self.walk.value(&Expr::Var(cursor.to_string()));
            if self.walk.value(bound).same(&end) && c0.v_coef() == 0 {
                let n = end.plus(&c0, -1).and_then(|x| x.plus(&Val::konst(1), 1));
                if let (Some(n), Some(next), Some(paced)) = (n, end.plus(&Val::konst(1), 1), &paced) {
                    self.walk.forget_writes(lp);
                    self.walk.assign(paced, n.clone());
                    self.walk.assign(cursor, next);
                    count = Some(n);
                }
            }
        }
        let count = count.unwrap_or_else(|| {
            self.exec(lp);
            self.walk.top()
        });
        self.define(&series, DefSite::Producer, count, paced.map(Expr::Var));
    }

    fn tail(&mut self) -> Result<(), StreamError> {
        let cp = self.cp;
        let sub_at: BTreeMap<usize, usize> = cp.subs.iter().enumerate().map(|(k, s)| (s.tail_idx, k)).collect();
        let mut map_at = BTreeMap::new();
        let mut aligns = Vec::new();
        for (si, st) in cp.steps.iter().enumerate() {
            match st {
                UpdateStep::Map { tail_idx } => {
                    map_at.insert(*tail_idx, si);
                }
                UpdateStep::Align { .. } => aligns.push(si),
                UpdateStep::Sub { .. } => {}
            }
        }
        let mut aligns = aligns.into_iter();
        let last = cp.tail.len().saturating_sub(1);
        for (i, st) in cp.tail.iter().enumerate() {
            if !self.walk.reachable() {
                break;
            }
            if let Some(&k) = sub_at.get(&i) {
                self.sub_call(k, st)?;
            } else if let Some(&si) = map_at.get(&i) {
                self.map_step(si, st)?;
            } else if is_memmove(st) {
                let si = aligns.next().expect("one Align step per memmove");
                self.align(si, i, st)?;
            } else if i == last && matches!(st, Statement::Return { .. }) {
                self.returns()?;
            } else if matches!(st, Statement::ForC { .. }) {
                self.prelude(i, st)?;
            } else {
                self.tail_scalar(i, st)?;
            }
        }
        Ok(())
    }

    fn sub_call(&mut self, k: usize, st: &Statement) -> Result<(), StreamError> {
        let sub = &self.cp.subs[k];
        let what = format!("sub-call `{}`", sub.callee);
        let s = self.walk.value(&sub.s_arg);
        Self::endpoint(&s, &what, sub.srcs.first().map_or("", String::as_str))?;
        let e = self.walk.value(&sub.e_arg);
        for (j, src) in sub.srcs.iter().enumerate() {
            let def = self.read(src, &what)?;
            let d = self.counts[def].plus(&e, -1).and_then(|x| x.plus(&Val::konst(1), -1));
            self.edge(def, Consumer::SubSrc { sub_idx: k, input: j }, d, &what, src)?;
        }
        let facts = self.walk.sub_call(k, st, sub);
        let count_open = batch_call_out_args(st, sub)
            .and_then(|(meta, _)| recv_var(meta[1]))
            .map(|r| recv_read_expr(&r));
        for (o, dst) in sub.dsts.iter().enumerate() {
            self.define(dst, DefSite::Sub { sub_idx: k, out: o }, facts.count.clone(), count_open.clone());
        }
        Ok(())
    }

    fn align(&mut self, si: usize, i: usize, st: &Statement) -> Result<(), StreamError> {
        let what = "memmove";
        let Statement::Expr(Expr::FuncCall(_, args)) = st else { unreachable!("a memmove") };
        let (Expr::Var(dst), Expr::AddressOf(at)) = (&args[0], &args[1]) else {
            return Err(refusal(what, "", OPAQUE));
        };
        let Expr::ArrayAccess(src, k) = at.as_ref() else {
            return Err(refusal(what, dst, OPAQUE));
        };
        let Some(n) = byte_count(&args[2]) else {
            return Err(refusal(what, src, OPAQUE));
        };
        let kv = self.walk.value(k);
        Self::endpoint(&kv, what, src)?;
        let nv = self.walk.value(&n);
        let def = self.read(src, what)?;
        let d = self.counts[def].plus(&kv, -1).and_then(|x| x.plus(&nv, -1));
        self.edge(def, Consumer::Align { step: si }, d, what, src)?;
        self.aligns.insert(si, (i, n.clone()));
        self.exec(st);
        self.define(dst, DefSite::Align { step: si }, nv, Some(n));
        Ok(())
    }

    fn returns(&mut self) -> Result<(), StreamError> {
        let fin = self.walk.value(&Expr::PointerDeref("outNBElement".into()));
        for o in &self.cp.func.outputs {
            let def = self.read(&o.name, "return")?;
            let d = self.counts[def].plus(&fin, -1);
            self.edge(def, Consumer::Return { output: o.name.clone() }, d, "return", &o.name)?;
        }
        Ok(())
    }

    fn tail_read(&mut self, name: &str, what: &str, tail_idx: usize) -> Result<(), StreamError> {
        let def = self.read(name, what)?;
        self.edges.push(Edge {
            def,
            consumer: Consumer::TailScalar { tail_idx },
            class: EdgeClass::Zero,
        });
        self.delays.push(None);
        self.kinds.push(Kind::Zero);
        Ok(())
    }

    /// A tail loop that only sets scalars: Open alone runs it, so every series
    /// element it reads must sit at a bar fixed for the stream.
    fn prelude(&mut self, i: usize, st: &Statement) -> Result<(), StreamError> {
        let what = "tail loop";
        let reads = stmt_array_reads(st);
        if let Some((first, _)) = reads.first().cloned() {
            let Statement::ForC { init, condition, .. } = st else { unreachable!("a for loop") };
            let cursor = map_loop_cursor(st).ok_or_else(|| refusal(what, &first, NOT_AFFINE))?;
            let start = match init.as_ref() {
                Statement::Assign { target: Expr::Var(c), value, .. } if *c == cursor => value,
                _ => return Err(refusal(what, &first, NOT_AFFINE)),
            };
            let Expr::BinOp(l, BinOp::Less | BinOp::LessEq, bound) = condition else {
                return Err(refusal(what, &first, NOT_AFFINE));
            };
            let writes = scalar_writes(st);
            if !matches!(l.as_ref(), Expr::Var(c) if *c == cursor)
                || scalar_reads(bound).iter().any(|n| writes.contains(n))
            {
                return Err(refusal(what, &first, NOT_AFFINE));
            }
            Self::endpoint(&self.walk.value(start), what, &first)?;
            Self::endpoint(&self.walk.value(bound), what, &first)?;
            for (name, idx) in reads {
                if scalar_reads(&idx).iter().any(|n| *n != cursor && writes.contains(n)) {
                    return Err(refusal(what, &name, NOT_AFFINE));
                }
                let off = self
                    .walk
                    .cursor_offset(&cursor, &idx)
                    .ok_or_else(|| refusal(what, &name, NOT_AFFINE))?;
                Self::endpoint(&off, what, &name)?;
                self.tail_read(&name, what, i)?;
            }
        }
        self.exec(st);
        Ok(())
    }

    fn tail_scalar(&mut self, i: usize, st: &Statement) -> Result<(), StreamError> {
        let what = "tail statement";
        let writes = scalar_writes(st);
        for (name, idx) in stmt_array_reads(st) {
            if scalar_reads(&idx).iter().any(|n| writes.contains(n)) {
                return Err(refusal(what, &name, OPAQUE));
            }
            Self::endpoint(&self.walk.value(&idx), what, &name)?;
            self.tail_read(&name, what, i)?;
        }
        self.exec(st);
        Ok(())
    }

    // --- maps -------------------------------------------------------------

    fn units(st: &Statement) -> Result<(Vec<Unit<'_>>, Vec<Option<bool>>), StreamError> {
        let mut units = Vec::new();
        let mut arms = Vec::new();
        if let Statement::If { then_body, else_body, .. } = st {
            for (then, branch) in [(true, then_body), (false, else_body)] {
                arms.push(Some(then));
                for (p, s) in branch.iter().enumerate() {
                    if matches!(s, Statement::ForC { .. }) {
                        units.push(Self::unit(s, Some((then, p)))?);
                    }
                }
            }
        } else {
            arms.push(None);
            units.push(Self::unit(st, None)?);
        }
        Ok((units, arms))
    }

    fn unit(lp: &Statement, arm: Option<(bool, usize)>) -> Result<Unit<'_>, StreamError> {
        let what = "map";
        let Statement::ForC { update, body, .. } = lp else { unreachable!("a map loop") };
        let first = stmt_array_reads(lp).first().map(|r| r.0.clone()).unwrap_or_default();
        let cursor = map_loop_cursor(lp).ok_or_else(|| refusal(what, &first, NOT_AFFINE))?;
        let bound = map_loop_bound(lp).ok_or_else(|| refusal(what, &first, NOT_AFFINE))?;
        if !is_unit_step(update, &cursor) || body.iter().any(|s| scalar_writes(s).contains(&cursor)) {
            return Err(refusal(what, &first, NOT_AFFINE));
        }
        Ok(Unit { arm, lp, cursor, bound })
    }

    fn unit_writes(&self, u: &Unit, key: &str) -> bool {
        let mut hit = false;
        for_each_stmt(std::slice::from_ref(u.lp), &mut |s| {
            if let Statement::Assign { target: Expr::ArrayAccess(n, _), .. } = s {
                hit |= self.al.key_of(n) == key;
            }
        });
        hit
    }

    /// `off` of a read `name[cursor + off]` whose offset the step never moves.
    fn offset(&mut self, u: &Unit, idx: &Expr, writes: &BTreeSet<String>, name: &str) -> Result<Val, StreamError> {
        if scalar_reads(idx).iter().any(|n| *n != u.cursor && writes.contains(n))
            || cursor_index_offset(idx, &u.cursor).is_none()
        {
            return Err(refusal("map", name, NOT_AFFINE));
        }
        self.walk
            .cursor_offset(&u.cursor, idx)
            .ok_or_else(|| refusal("map", name, NOT_AFFINE))
    }

    #[allow(clippy::too_many_lines)]
    fn map_step(&mut self, si: usize, st: &Statement) -> Result<(), StreamError> {
        let what = "map";
        let (units, arms) = Self::units(st)?;
        let writes = scalar_writes(st);
        // Every key the step writes, with the first name it is written through.
        let mut written: Vec<(String, String)> = Vec::new();
        for u in &units {
            let mut targets = Vec::new();
            for_each_stmt(std::slice::from_ref(u.lp), &mut |s| {
                if let Statement::Assign { target: Expr::ArrayAccess(n, idx), .. } = s {
                    targets.push((n.clone(), idx.as_ref().clone()));
                }
            });
            for (n, idx) in targets {
                if !matches!(&idx, Expr::Var(c) if *c == u.cursor) {
                    return Err(refusal(what, &n, NOT_AFFINE));
                }
                let key = self.al.key_of(&n);
                if !written.iter().any(|(k, _)| *k == key) {
                    written.push((key, n));
                }
            }
        }
        let keys: BTreeSet<String> = written.iter().map(|(k, _)| k.clone()).collect();
        let view = StepView { keys: &keys, writes: &writes };
        for u in &units {
            if scalar_reads(&u.bound).iter().any(|n| *n != u.cursor && writes.contains(n)) {
                let first = written.first().map_or("", |w| w.1.as_str());
                return Err(refusal(what, first, NOT_AFFINE));
            }
        }
        let tvals: Vec<Val> = units.iter().map(|u| self.walk.value(&u.bound)).collect();

        // Per arm, each key an earlier loop of the arm writes, with that loop.
        let mut owners: BTreeMap<Option<bool>, BTreeMap<String, usize>> = BTreeMap::new();
        // (unit, key, name, whole arm): a whole-arm carry is the prior itself.
        let mut priors: Vec<(usize, String, String, bool)> = Vec::new();
        for (ui, u) in units.iter().enumerate() {
            let earlier = owners.get(&u.branch()).cloned().unwrap_or_default();
            let Statement::ForC { body, .. } = u.lp else { unreachable!("a map loop") };
            let mut scan = BodyScan::default();
            let end = self.scan(body, earlier.keys().cloned().collect(), u, &view, &mut scan)?;
            let must = scan.cut.iter().fold(end, |acc, c| acc.intersection(c).cloned().collect());
            for (key, name) in &written {
                let writes_key = self.unit_writes(u, key);
                if !writes_key && !scan.self_reads.contains(key) {
                    continue;
                }
                match earlier.get(key) {
                    Some(&owner) if !tvals[owner].same(&tvals[ui]) => {
                        return Err(refusal(what, name, SELF_OFFSET));
                    }
                    None if scan.before.contains(key) || !must.contains(key) => {
                        priors.push((ui, key.clone(), name.clone(), false));
                    }
                    _ => {}
                }
                if writes_key {
                    owners.entry(u.branch()).or_default().entry(key.clone()).or_insert(ui);
                }
            }
            for (name, idx) in scan.reads {
                let def = self.read(&name, what)?;
                let off = self.offset(u, &idx, &writes, &name)?;
                let d = self.counts[def].plus(&tvals[ui], -1).and_then(|x| x.plus(&off, -1));
                let consumer = Consumer::MapRead {
                    step: si,
                    arm: u.arm,
                    buffer: name.clone(),
                    index: idx,
                };
                self.edge(def, consumer, d, what, &name)?;
            }
        }
        // An arm that leaves a key unwritten keeps its prior there.
        for arm in &arms {
            for (key, name) in &written {
                let owned = owners.get(arm).is_some_and(|o| o.contains_key(key));
                let carried = priors.iter().any(|(ui, k, _, _)| k == key && units[*ui].branch() == *arm);
                if owned || carried {
                    continue;
                }
                match units.iter().position(|u| u.branch() == *arm) {
                    Some(ui) => priors.push((ui, key.clone(), name.clone(), true)),
                    None => return Err(refusal(what, name, NO_PRIOR)),
                }
            }
        }
        priors.sort_by_key(|(ui, _, _, _)| *ui);
        for (ui, key, name, whole) in &priors {
            let prior = self.read(name, what).map_err(|_| refusal(what, name, NO_PRIOR))?;
            debug_assert_eq!(self.defs[prior].buffer, *key);
            let d = if *whole {
                Some(Val::konst(0))
            } else {
                self.counts[prior].plus(&tvals[*ui], -1)
            };
            let consumer = Consumer::MapPrior {
                step: si,
                arm: units[*ui].arm,
                buffer: name.clone(),
            };
            self.edge(prior, consumer, d, what, name)?;
        }
        for u in &units {
            self.bounds.insert((si, u.arm), u.bound.clone());
        }
        self.walk.forget_writes(st);
        let cond = match st {
            Statement::If { condition, .. } => Some(condition),
            _ => None,
        };
        for (key, name) in &written {
            let (count, open) = self.arms_result(key, &arms, &owners, &units, &tvals, cond);
            self.define(name, DefSite::Map { step: si }, count, open);
        }
        Ok(())
    }

    /// `key`'s count after a variant map, from each arm's owning loop bound or
    /// carried prior, and the expression Open snapshots it from.
    fn arms_result(
        &mut self,
        key: &str,
        arms: &[Option<bool>],
        owners: &BTreeMap<Option<bool>, BTreeMap<String, usize>>,
        units: &[Unit],
        tvals: &[Val],
        cond: Option<&Expr>,
    ) -> (Val, Option<Expr>) {
        let mut counts: Vec<Val> = Vec::new();
        let mut opens: Vec<Option<Expr>> = Vec::new();
        let mut owned_bound = None;
        for arm in arms {
            if let Some(&ui) = owners.get(arm).and_then(|o| o.get(key)) {
                counts.push(tvals[ui].clone());
                opens.push(Some(units[ui].bound.clone()));
                owned_bound.get_or_insert_with(|| units[ui].bound.clone());
            } else {
                // The prior's count expression would be read here, after the
                // prior, and a later call may have rewritten its receiver.
                counts.push(self.counts[self.cur[key]].clone());
                opens.push(None);
            }
        }
        let same = counts.iter().all(|c| c.same(&counts[0]));
        let count = if same { counts[0].clone() } else { self.walk.top() };
        if let (true, Some(bound)) = (same, owned_bound) {
            for o in &mut opens {
                *o = Some(bound.clone());
            }
        }
        let open = match (&opens[..], cond) {
            ([one], _) => one.clone(),
            ([Some(a), Some(b)], _) if exprs_equal(a, b) => Some(a.clone()),
            ([Some(a), Some(b)], Some(c)) => {
                Some(Expr::Ternary(Box::new(c.clone()), Box::new(a.clone()), Box::new(b.clone())))
            }
            _ => None,
        };
        (count, open)
    }

    /// Scans a map body along its paths; returns the keys written at its end.
    fn scan(
        &mut self,
        stmts: &[Statement],
        mut path: BTreeSet<String>,
        u: &Unit,
        view: &StepView,
        scan: &mut BodyScan,
    ) -> Result<BTreeSet<String>, StreamError> {
        for s in stmts {
            let mut reads = Vec::new();
            match s {
                Statement::Assign { target, value, compound } => {
                    array_reads(value, false, &mut reads);
                    array_reads(target, !compound, &mut reads);
                    self.note_reads(reads, &path, u, view, scan)?;
                    if let Expr::ArrayAccess(n, _) = target {
                        path.insert(self.al.key_of(n));
                    }
                }
                Statement::If { condition, then_body, else_body, .. } => {
                    array_reads(condition, false, &mut reads);
                    self.note_reads(reads, &path, u, view, scan)?;
                    // A guard on a value the step moves orders nothing it reads.
                    let stable = !scalar_reads(condition).iter().any(|n| view.writes.contains(n));
                    let always = Expr::IntLiteral(1);
                    self.walk.push_guard(if stable { condition } else { &always });
                    let t = self.scan(then_body, path.clone(), u, view, scan);
                    self.walk.pop_guard();
                    let e = self.scan(else_body, path.clone(), u, view, scan)?;
                    path = t?.intersection(&e).cloned().collect();
                }
                Statement::Block { body } => {
                    path = self.scan(body, path, u, view, scan)?;
                }
                Statement::Break | Statement::Continue | Statement::Return { .. } => {
                    if let Statement::Return { value: Some(v) } = s {
                        array_reads(v, false, &mut reads);
                        self.note_reads(reads, &path, u, view, scan)?;
                    }
                    scan.cut.push(path.clone());
                    return Ok(path);
                }
                Statement::While { .. }
                | Statement::DoWhile { .. }
                | Statement::For { .. }
                | Statement::ForC { .. }
                | Statement::Switch { .. } => {
                    // Runs any number of times or takes any arm: what it writes
                    // counts on no path.
                    super::walk_stmt_own_exprs(s, &mut |e| array_reads(e, false, &mut reads));
                    self.note_reads(reads, &path, u, view, scan)?;
                    for body in nested_bodies(s).0 {
                        self.scan(body, path.clone(), u, view, scan)?;
                    }
                }
                other => {
                    super::walk_stmt_own_exprs(other, &mut |e| array_reads(e, false, &mut reads));
                    self.note_reads(reads, &path, u, view, scan)?;
                }
            }
        }
        Ok(path)
    }

    fn note_reads(
        &mut self,
        reads: Vec<(String, Expr)>,
        path: &BTreeSet<String>,
        u: &Unit,
        view: &StepView,
        scan: &mut BodyScan,
    ) -> Result<(), StreamError> {
        for (name, idx) in reads {
            let key = self.al.key_of(&name);
            if view.keys.contains(&key) {
                let off = self.offset(u, &idx, view.writes, &name)?;
                if !off.same(&Val::konst(0)) {
                    return Err(refusal("map", &name, SELF_OFFSET));
                }
                if !path.contains(&key) {
                    scan.before.insert(key.clone());
                }
                scan.self_reads.insert(key);
                continue;
            }
            if self.al.may.get(&key).is_some_and(|o| o.iter().any(|k| view.keys.contains(k))) {
                return Err(refusal("map", &name, MAY_ALIAS));
            }
            if !scan.reads.iter().any(|(n, i)| *n == name && exprs_equal(i, &idx)) {
                scan.reads.push((name, idx));
            }
        }
        Ok(())
    }

    // --- lines, keys, Open actions ----------------------------------------

    fn map_ordinal(&self, si: usize) -> usize {
        self.cp.steps[..si].iter().filter(|s| matches!(s, UpdateStep::Map { .. })).count()
    }

    fn align_ordinal(&self, si: usize) -> usize {
        self.cp.steps[..si].iter().filter(|s| matches!(s, UpdateStep::Align { .. })).count()
    }

    fn map_tail(&self, si: usize) -> usize {
        match self.cp.steps[si] {
            UpdateStep::Map { tail_idx } => tail_idx,
            _ => unreachable!("a map step"),
        }
    }

    fn zero_key(&self, e: usize) -> String {
        let edge = &self.edges[e];
        let def = &self.defs[edge.def];
        let who = match &edge.consumer {
            Consumer::SubSrc { sub_idx, .. } => format!("sub{sub_idx}"),
            Consumer::Align { step } => format!("copy{}", self.align_ordinal(*step)),
            Consumer::MapRead { step, .. } | Consumer::MapPrior { step, .. } => {
                let j = self.edges[..e]
                    .iter()
                    .filter(|x| {
                        x.def == edge.def
                            && matches!(&x.consumer,
                                Consumer::MapRead { step: s, .. } | Consumer::MapPrior { step: s, .. } if s == step)
                    })
                    .count();
                format!("map{}.r{j}", self.map_ordinal(*step))
            }
            Consumer::Return { output } => format!("ret.{output}"),
            Consumer::TailScalar { tail_idx } => format!("tail{tail_idx}"),
        };
        format!("lagzero.{}.{}.{who}", def.buffer, def.ordinal)
    }

    /// Where Open checks a consumer's delay: a point, or an arm of a variant If.
    fn place(&self, c: &Consumer) -> Place {
        match c {
            Consumer::SubSrc { sub_idx, .. } => Ok(OpenAt::AfterSubOpen(self.cp.subs[*sub_idx].tail_idx)),
            Consumer::Align { step } => Ok(OpenAt::BeforeStmt(self.aligns[step].0)),
            Consumer::MapRead { step, arm: None, .. } | Consumer::MapPrior { step, arm: None, .. } => {
                Ok(OpenAt::BeforeStmt(self.map_tail(*step)))
            }
            Consumer::MapRead { step, arm: Some((then, _)), .. }
            | Consumer::MapPrior { step, arm: Some((then, _)), .. } => Err((*step, *then)),
            Consumer::Return { .. } => Ok(OpenAt::Epilogue),
            Consumer::TailScalar { .. } => unreachable!("a tail read is never checked"),
        }
    }

    /// The EmptyGuard a consumer's first check needs: `T` or `n` below 1 means
    /// batch never read the series there.
    fn empty_guard(&self, c: &Consumer, done: &mut BTreeSet<(usize, Option<(bool, usize)>)>) -> Option<OpenAction> {
        let (key, count) = match c {
            Consumer::Align { step } => ((*step, None), self.aligns[step].1.clone()),
            Consumer::MapRead { step, arm, .. } | Consumer::MapPrior { step, arm, .. } => {
                ((*step, *arm), self.bounds[&(*step, *arm)].clone())
            }
            _ => return None,
        };
        done.insert(key).then_some(OpenAction::EmptyGuard { count })
    }

    /// Groups tap edges into one line per definition, in body order. A sub-call
    /// reading one definition through several inputs reads one tap.
    fn lines(&mut self) -> (Vec<DelayLine>, Vec<Vec<Vec<usize>>>) {
        let mut lines = Vec::new();
        let mut members_of = Vec::new();
        for def in 0..self.defs.len() {
            let mut taps: Vec<LineTap> = Vec::new();
            let mut members: Vec<Vec<usize>> = Vec::new();
            for e in 0..self.edges.len() {
                let Kind::Tap { ge1 } = self.kinds[e] else { continue };
                if self.edges[e].def != def {
                    continue;
                }
                let shared = match self.edges[e].consumer {
                    Consumer::SubSrc { sub_idx, .. } => members.iter().position(|m| {
                        matches!(self.edges[m[0]].consumer, Consumer::SubSrc { sub_idx: s, .. } if s == sub_idx)
                    }),
                    _ => None,
                };
                let j = shared.unwrap_or_else(|| {
                    let d = &self.defs[def];
                    taps.push(LineTap {
                        edge: e,
                        ge1,
                        back: TapBack::Oldest,
                        key: format!("lagdepth.{}.{}.t{}", d.buffer, d.ordinal, taps.len()),
                    });
                    members.push(Vec::new());
                    taps.len() - 1
                });
                members[j].push(e);
                self.edges[e].class = EdgeClass::Tap { line: lines.len(), tap: j };
            }
            if taps.is_empty() {
                continue;
            }
            if taps.len() > 1 {
                let d0 = self.delays[taps[0].edge].clone().expect("a tap has a delay");
                let consts: Option<Vec<i64>> = taps
                    .iter()
                    .map(|t| {
                        self.delays[t.edge]
                            .as_ref()
                            .and_then(|d| d.plus(&d0, -1))
                            .and_then(|x| x.as_const())
                    })
                    .collect();
                let arm0 = arm_of(&self.edges[taps[0].edge].consumer);
                let one_arm = taps.iter().all(|t| arm_of(&self.edges[t.edge].consumer) == arm0);
                let deepest = consts.as_ref().and_then(|cs| cs.iter().max().copied());
                for (j, t) in taps.iter_mut().enumerate() {
                    t.back = match (&consts, deepest) {
                        (Some(cs), Some(m)) if one_arm => TapBack::Literal(m - cs[j]),
                        _ => TapBack::Field,
                    };
                }
            }
            lines.push(DelayLine { def, taps });
            members_of.push(members);
        }
        (lines, members_of)
    }

    #[allow(clippy::too_many_lines)]
    fn finish(mut self) -> Result<Classified, StreamError> {
        let (lines, members) = self.lines();
        let mut presumed: Vec<(usize, String)> = Vec::new();
        for e in 0..self.edges.len() {
            if self.kinds[e] == Kind::Presumed {
                let key = self.zero_key(e);
                self.edges[e].class = EdgeClass::ZeroPresumed { key: key.clone() };
                presumed.push((e, key));
            }
        }
        let snap: BTreeSet<usize> = lines
            .iter()
            .map(|l| l.def)
            .chain(presumed.iter().map(|(e, _)| self.edges[*e].def))
            .collect();
        for &def in &snap {
            if self.defs[def].count_open.is_none() {
                return Err(refusal("pipeline", &self.defs[def].buffer, OPAQUE));
            }
        }
        let snapshots: Vec<usize> = snap.into_iter().collect();

        let mut acts: Vec<(Place, u8, OpenAction)> = Vec::new();
        for &def in &snapshots {
            let at = match &self.defs[def].site {
                DefSite::Input | DefSite::Producer => OpenAt::BeforeStmt(0),
                DefSite::Sub { sub_idx, .. } => {
                    OpenAt::AfterStmt(after_call_guard(self.cp.tail, self.cp.subs[*sub_idx].tail_idx))
                }
                DefSite::Align { step } => OpenAt::BeforeStmt(self.aligns[step].0),
                DefSite::Map { step } => OpenAt::BeforeStmt(self.map_tail(*step)),
            };
            acts.push((Ok(at), 0, OpenAction::Snapshot { def }));
        }
        let mut guarded = BTreeSet::new();
        let mut keys = BTreeSet::new();
        let checks = presumed
            .iter()
            .filter(|(_, key)| keys.insert(key.clone()))
            .map(|(e, _)| (*e, OpenAction::Presume { edge: *e }, 2))
            .chain(lines.iter().enumerate().flat_map(|(k, line)| {
                line.taps
                    .iter()
                    .enumerate()
                    .map(move |(j, t)| (t.edge, OpenAction::Capture { line: k, tap: j }, 3))
            }))
            .collect::<Vec<_>>();
        for (e, action, rank) in checks {
            let c = &self.edges[e].consumer;
            if let Some(g) = self.empty_guard(c, &mut guarded) {
                acts.push((self.place(c), 1, g));
            }
            acts.push((self.place(c), rank, action));
        }
        let order = |at: &OpenAt| -> (usize, u8) {
            match at {
                OpenAt::AfterStmt(i) => (i + 1, 0),
                OpenAt::BeforeStmt(i) => (*i, 1),
                OpenAt::AfterSubOpen(i) => (*i, 3),
                OpenAt::Epilogue => (usize::MAX, 0),
            }
        };
        for (k, member) in members.iter().enumerate() {
            let last = member
                .iter()
                .flatten()
                .map(|e| match self.place(&self.edges[*e].consumer) {
                    Ok(at) => at,
                    Err((step, _)) => OpenAt::BeforeStmt(self.map_tail(step)),
                })
                .max_by_key(|at| order(at))
                .expect("a line has a tap");
            acts.push((Ok(last), 4, OpenAction::Seed { line: k }));
        }

        let mut top: Vec<(OpenAt, u8, OpenAction)> = Vec::new();
        let mut mirrored: BTreeMap<usize, [Vec<(u8, OpenAction)>; 2]> = BTreeMap::new();
        for (place, rank, action) in acts {
            match place {
                Ok(at) => top.push((at, rank, action)),
                Err((step, then)) => mirrored.entry(step).or_default()[usize::from(!then)].push((rank, action)),
            }
        }
        for (step, [then_, else_]) in mirrored {
            let arm = |mut v: Vec<(u8, OpenAction)>| {
                v.sort_by_key(|(r, _)| *r);
                v.into_iter().map(|(_, a)| a).collect::<Vec<_>>()
            };
            let at = self.map_tail(step);
            let Statement::If { condition, .. } = &self.cp.tail[at] else {
                unreachable!("an arm belongs to a variant If")
            };
            top.push((
                OpenAt::BeforeStmt(at),
                2,
                OpenAction::Mirror {
                    cond: condition.clone(),
                    then_: arm(then_),
                    else_: arm(else_),
                },
            ));
        }
        top.sort_by_key(|(at, rank, _)| (order(at), *rank));
        Ok(Classified {
            alias_keys: self.al.key,
            defs: self.defs,
            edges: self.edges,
            lines,
            snapshots,
            open_actions: top.into_iter().map(|(at, _, a)| (at, a)).collect(),
        })
    }
}
