//! Lower bound of a trailing ring's lag over the optional inputs' declared
//! ranges.
//!
//! The lag is `cursor - var` at steady-loop entry, the value Open captures as
//! a `back == 0` ring's capacity. A ring proved to lag by at least one bar
//! never holds the current bar, so its transition needs no zero-capacity
//! guard and Peek no select on the guard's slot.
//!
//! Soundness over coverage: every construct the tracker does not model makes
//! the variables it touches fresh unknowns, and an unproven ring keeps its
//! guard. Open re-checks a proven ring's capacity at run time, so a wrong
//! proof fails loudly rather than reading a stale slot.
//!
//! Preconditions the proof relies on, both already required for the batch body
//! to read only valid indices: every integer parameter lies in its declared
//! range (Open validates it first) and Open's anchor `startIdx` is not
//! negative.

use std::collections::{BTreeMap, BTreeSet};
use std::sync::{Mutex, OnceLock};

use crate::ir::{BinOp, CircBuf, Expr, FuncDef, LookbackExpr, ParamType, Statement, VarType};
use crate::streaming::{CalleeLookback, CalleeLookup};

/// A single-parameter group is enumerated only up to this many values; wider
/// ranges fall back to interval bounds.
const ENUM_LIMIT: i64 = 1 << 18;
/// Stands in for an unbounded interval end. Products of two stay inside i128.
const INF: i128 = 1 << 62;
/// Lookbacks calling lookbacks nest only a few levels deep.
const MAX_INLINE_DEPTH: u32 = 8;

// ---------------------------------------------------------------------------
// Linear forms over parameter atoms
// ---------------------------------------------------------------------------

/// `k + sum(coef * atom)`, each atom a parameter-pure expression.
#[derive(Clone, Debug, PartialEq, Default)]
struct Lin {
    terms: BTreeMap<String, (Expr, i64)>,
    k: i64,
}

impl Lin {
    fn konst(k: i64) -> Self {
        Lin {
            terms: BTreeMap::new(),
            k,
        }
    }

    fn atom(e: Expr) -> Self {
        let mut terms = BTreeMap::new();
        terms.insert(format!("{e:?}"), (e, 1));
        Lin { terms, k: 0 }
    }

    fn as_const(&self) -> Option<i64> {
        self.terms.is_empty().then_some(self.k)
    }

    fn add(&self, other: &Lin, sign: i64) -> Option<Lin> {
        let mut out = self.clone();
        out.k = out.k.checked_add(other.k.checked_mul(sign)?)?;
        for (key, (e, c)) in &other.terms {
            let c = c.checked_mul(sign)?;
            let slot = out.terms.entry(key.clone()).or_insert((e.clone(), 0));
            slot.1 = slot.1.checked_add(c)?;
        }
        out.terms.retain(|_, (_, c)| *c != 0);
        Some(out)
    }

    fn scale(&self, s: i64) -> Option<Lin> {
        if s == 0 {
            return Some(Lin::konst(0));
        }
        let mut out = self.clone();
        out.k = out.k.checked_mul(s)?;
        for (_, c) in out.terms.values_mut() {
            *c = c.checked_mul(s)?;
        }
        Some(out)
    }

    fn to_expr(&self) -> Expr {
        let mut acc: Option<Expr> = None;
        for (e, c) in self.terms.values() {
            let term = if *c == 1 {
                e.clone()
            } else {
                Expr::BinOp(Box::new(Expr::IntLiteral(*c)), BinOp::Mul, Box::new(e.clone()))
            };
            acc = Some(match acc {
                None => term,
                Some(a) => Expr::BinOp(Box::new(a), BinOp::Add, Box::new(term)),
            });
        }
        match acc {
            None => Expr::IntLiteral(self.k),
            Some(a) if self.k == 0 => a,
            Some(a) => Expr::BinOp(Box::new(a), BinOp::Add, Box::new(Expr::IntLiteral(self.k))),
        }
    }
}

// ---------------------------------------------------------------------------
// Parameter box, facts, bounds
// ---------------------------------------------------------------------------

struct Ctx<'a> {
    func: &'a FuncDef,
    params: BTreeSet<String>,
    /// Declared `[lo, hi]` per integer parameter with a range.
    boxes: BTreeMap<String, (i64, i64)>,
    /// Fact conjuncts that mention exactly one parameter, by that parameter.
    facts: BTreeMap<String, Vec<Expr>>,
    types: BTreeMap<String, Option<VarType>>,
    /// Other functions' lookbacks, by lowercase name.
    callees: BTreeMap<String, CalleeLookback>,
}

fn conjuncts(e: &Expr, out: &mut Vec<Expr>) {
    match e {
        Expr::BinOp(l, BinOp::And, r) => {
            conjuncts(l, out);
            conjuncts(r, out);
        }
        Expr::Not(inner) => match inner.as_ref() {
            Expr::BinOp(l, BinOp::Or, r) => {
                conjuncts(&Expr::Not(l.clone()), out);
                conjuncts(&Expr::Not(r.clone()), out);
            }
            Expr::Not(x) => conjuncts(x, out),
            _ => out.push(e.clone()),
        },
        _ => out.push(e.clone()),
    }
}

fn mentioned_params(e: &Expr, params: &BTreeSet<String>) -> BTreeSet<String> {
    let mut out = BTreeSet::new();
    crate::streaming::walk_expr(e, &mut |x| {
        if let Expr::Var(v) = x {
            if params.contains(v) {
                out.insert(v.clone());
            }
        }
    });
    out
}

fn collect_decl_types(stmts: &[Statement], out: &mut BTreeMap<String, Option<VarType>>) {
    for s in stmts {
        if let Statement::VarDecl { var_type, name, .. } = s {
            match out.get(name) {
                Some(Some(t)) if t == var_type => {}
                Some(_) => {
                    out.insert(name.clone(), None);
                }
                None => {
                    out.insert(name.clone(), Some(var_type.clone()));
                }
            }
        }
        let (bodies, _) = crate::streaming::nested_bodies(s);
        for b in bodies {
            collect_decl_types(b, out);
        }
    }
}

impl<'a> Ctx<'a> {
    fn new(func: &'a FuncDef, facts: &[Expr], callees: BTreeMap<String, CalleeLookback>) -> Self {
        let params: BTreeSet<String> = func.optional_inputs.iter().map(|p| p.name.clone()).collect();
        let mut types: BTreeMap<String, Option<VarType>> = BTreeMap::new();
        collect_decl_types(func.stream_source(), &mut types);
        for n in ["startIdx", "endIdx"] {
            types.insert(n.to_string(), Some(VarType::Integer));
        }
        let mut ctx = Ctx {
            func,
            params: params.clone(),
            boxes: BTreeMap::new(),
            facts: BTreeMap::new(),
            types,
            callees,
        };
        for p in &func.optional_inputs {
            if p.param_type == ParamType::Integer {
                ctx.types.insert(p.name.clone(), Some(VarType::Integer));
                if let Some((lo, hi)) = p.range {
                    #[allow(clippy::cast_possible_truncation)]
                    let (lo, hi) = (lo.ceil() as i64, hi.floor() as i64);
                    if lo <= hi {
                        ctx.boxes.insert(p.name.clone(), (lo, hi));
                    }
                }
            }
        }
        let mut cs = Vec::new();
        for f in facts {
            conjuncts(f, &mut cs);
        }
        for c in cs {
            let ps = mentioned_params(&c, &params);
            if ps.len() == 1 {
                let p = ps.into_iter().next().expect("one parameter");
                ctx.facts.entry(p).or_default().push(c);
            }
        }
        ctx
    }

    /// A fact that cannot be evaluated excludes nothing.
    fn admissible(&self, p: &str, v: i64) -> bool {
        let env: BTreeMap<String, i64> = std::iter::once((p.to_string(), v)).collect();
        self.facts
            .get(p)
            .is_none_or(|fs| fs.iter().all(|f| self.eval(f, &env, 0).is_none_or(|x| x != 0)))
    }

    // --- lookback inlining ------------------------------------------------

    /// The body of `<f>_lookback(args)`, only when every argument provably
    /// lies in the range the generated lookback accepts (it answers -1
    /// otherwise).
    fn inline_lookback(&self, name: &str, args: &[Expr], depth: u32) -> Option<Expr> {
        if depth > MAX_INLINE_DEPTH {
            return None;
        }
        let callee = name.strip_suffix("_lookback")?;
        let def = if callee.eq_ignore_ascii_case(&self.func.name) {
            let LookbackExpr::Code(body) = self.func.lookback.as_ref()? else {
                return None;
            };
            CalleeLookback {
                params: self.func.optional_inputs.clone(),
                body: body.clone(),
            }
        } else {
            self.callees.get(&callee.to_ascii_lowercase())?.clone()
        };
        if def.params.len() != args.len() {
            return None;
        }
        for (p, a) in def.params.iter().zip(args) {
            let (Some((lo, hi)), ParamType::Integer) = (p.range, &p.param_type) else {
                return None;
            };
            let (a0, a1) = self.interval(a, depth + 1)?;
            #[allow(clippy::cast_possible_truncation)]
            let inside = a0 >= i128::from(lo.ceil() as i64) && a1 <= i128::from(hi.floor() as i64);
            if !inside {
                return None;
            }
        }
        let env: BTreeMap<String, Expr> =
            def.params.iter().map(|p| p.name.clone()).zip(args.iter().cloned()).collect();
        lookback_body_expr(&def.body, env)
    }

    // --- concrete evaluation (C `int` semantics) ----------------------------

    fn eval(&self, e: &Expr, env: &BTreeMap<String, i64>, depth: u32) -> Option<i64> {
        let int = |x: i64| (i64::from(i32::MIN)..=i64::from(i32::MAX)).contains(&x).then_some(x);
        match e {
            Expr::IntLiteral(k) => Some(*k),
            Expr::Var(v) => env.get(v).copied(),
            Expr::Neg(x) => int(self.eval(x, env, depth)?.checked_neg()?),
            Expr::Not(x) => Some(i64::from(self.eval(x, env, depth)? == 0)),
            Expr::BinOp(lhs, op, rhs) => {
                let va = self.eval(lhs, env, depth)?;
                match op {
                    BinOp::And => {
                        return if va == 0 { Some(0) } else { Some(i64::from(self.eval(rhs, env, depth)? != 0)) };
                    }
                    BinOp::Or => {
                        return if va != 0 { Some(1) } else { Some(i64::from(self.eval(rhs, env, depth)? != 0)) };
                    }
                    _ => {}
                }
                let vb = self.eval(rhs, env, depth)?;
                // Past `+ - *`, a negative value of an unsigned operand is
                // not what C computes with.
                if !matches!(op, BinOp::Add | BinOp::Sub | BinOp::Mul)
                    && (va < 0 || vb < 0)
                    && (is_unsigned(lhs) || is_unsigned(rhs))
                {
                    return None;
                }
                match op {
                    BinOp::Add => int(va.checked_add(vb)?),
                    BinOp::Sub => int(va.checked_sub(vb)?),
                    BinOp::Mul => int(va.checked_mul(vb)?),
                    // Rust's `/` and `%` truncate toward zero, as C99 does.
                    BinOp::Div => int(va.checked_div(vb)?),
                    BinOp::Mod => int(va.checked_rem(vb)?),
                    BinOp::Shr if va >= 0 && (0..31).contains(&vb) => Some(va >> vb),
                    BinOp::Shl if va >= 0 && (0..31).contains(&vb) => int(va << vb),
                    BinOp::Less => Some(i64::from(va < vb)),
                    BinOp::LessEq => Some(i64::from(va <= vb)),
                    BinOp::Greater => Some(i64::from(va > vb)),
                    BinOp::GreaterEq => Some(i64::from(va >= vb)),
                    BinOp::Eq => Some(i64::from(va == vb)),
                    BinOp::NotEq => Some(i64::from(va != vb)),
                    _ => None,
                }
            }
            Expr::Cast(VarType::Integer, x) => {
                if let Some(arg) = sqrt_of_real(x) {
                    let v = self.eval(arg, env, depth)?;
                    #[allow(clippy::cast_possible_truncation, clippy::cast_precision_loss)]
                    return (v >= 0).then(|| (v as f64).sqrt() as i64);
                }
                int(self.eval(x, env, depth)?)
            }
            Expr::Cast(VarType::Index, x) => {
                let v = self.eval(x, env, depth)?;
                (v >= 0).then_some(v)
            }
            Expr::Ternary(c, a, b) => {
                if self.eval(c, env, depth)? != 0 {
                    self.eval(a, env, depth)
                } else {
                    self.eval(b, env, depth)
                }
            }
            Expr::FuncCall(name, args) if (name == "max" || name == "min") && args.len() == 2 => {
                let a = self.eval(&args[0], env, depth)?;
                let b = self.eval(&args[1], env, depth)?;
                Some(if name == "max" { a.max(b) } else { a.min(b) })
            }
            Expr::FuncCall(name, args) => {
                let body = self.inline_lookback(name, args, depth + 1)?;
                self.eval(&body, env, depth + 1)
            }
            _ => None,
        }
    }

    // --- interval evaluation ------------------------------------------------

    fn interval(&self, e: &Expr, depth: u32) -> Option<(i128, i128)> {
        let sat = |x: i128| x.clamp(-INF, INF);
        match e {
            Expr::IntLiteral(k) => Some((i128::from(*k), i128::from(*k))),
            Expr::Var(v) => self.boxes.get(v).map(|&(lo, hi)| (i128::from(lo), i128::from(hi))),
            Expr::Neg(x) => {
                let (lo, hi) = self.interval(x, depth)?;
                Some((-hi, -lo))
            }
            Expr::Not(_) => Some((0, 1)),
            Expr::BinOp(l, op, r) => {
                if matches!(op, BinOp::And | BinOp::Or) {
                    return Some((0, 1));
                }
                let (a0, a1) = self.interval(l, depth)?;
                let (b0, b1) = self.interval(r, depth)?;
                if !matches!(op, BinOp::Add | BinOp::Sub | BinOp::Mul)
                    && (a0 < 0 || b0 < 0)
                    && (is_unsigned(l) || is_unsigned(r))
                {
                    return None;
                }
                match op {
                    BinOp::Add => Some((sat(a0 + b0), sat(a1 + b1))),
                    BinOp::Sub => Some((sat(a0 - b1), sat(a1 - b0))),
                    BinOp::Mul => {
                        let c = [a0 * b0, a0 * b1, a1 * b0, a1 * b1];
                        Some((sat(*c.iter().min()?), sat(*c.iter().max()?)))
                    }
                    // Truncating division by a positive divisor is monotone in
                    // the dividend; a non-negative dividend makes it monotone
                    // (decreasing) in the divisor too.
                    BinOp::Div if b0 >= 1 && b0 == b1 => Some((a0 / b0, a1 / b0)),
                    BinOp::Div if b0 >= 1 && a0 >= 0 => Some((a0 / b1, a1 / b0)),
                    BinOp::Shr if a0 >= 0 && b0 == b1 && (0..31).contains(&b0) => {
                        Some((a0 >> b0, a1 >> b0))
                    }
                    BinOp::Less => Some(decided(a1 < b0, a0 >= b1)),
                    BinOp::LessEq => Some(decided(a1 <= b0, a0 > b1)),
                    BinOp::Greater => Some(decided(a0 > b1, a1 <= b0)),
                    BinOp::GreaterEq => Some(decided(a0 >= b1, a1 < b0)),
                    BinOp::Eq => Some(decided(a0 == a1 && b0 == b1 && a0 == b0, a1 < b0 || b1 < a0)),
                    BinOp::NotEq => Some(decided(a1 < b0 || b1 < a0, a0 == a1 && b0 == b1 && a0 == b0)),
                    _ => None,
                }
            }
            Expr::Cast(VarType::Integer, x) => {
                if let Some(arg) = sqrt_of_real(x) {
                    let (lo, hi) = self.interval(arg, depth)?;
                    return (lo >= 0).then(|| (isqrt(lo), isqrt(hi)));
                }
                self.interval(x, depth)
            }
            Expr::Cast(VarType::Index, x) => {
                let (lo, hi) = self.interval(x, depth)?;
                (lo >= 0).then_some((lo, hi))
            }
            Expr::Ternary(_, a, b) => {
                let (a0, a1) = self.interval(a, depth)?;
                let (b0, b1) = self.interval(b, depth)?;
                Some((a0.min(b0), a1.max(b1)))
            }
            Expr::FuncCall(name, args) if (name == "max" || name == "min") && args.len() == 2 => {
                let (a0, a1) = self.interval(&args[0], depth)?;
                let (b0, b1) = self.interval(&args[1], depth)?;
                Some(if name == "max" {
                    (a0.max(b0), a1.max(b1))
                } else {
                    (a0.min(b0), a1.min(b1))
                })
            }
            Expr::FuncCall(name, _) if is_unstable_call(name) => Some((0, INF)),
            Expr::FuncCall(name, args) => {
                let body = self.inline_lookback(name, args, depth + 1)?;
                self.interval(&body, depth + 1)
            }
            _ => None,
        }
    }

    // --- lower bound of a linear form ---------------------------------------

    /// A sound lower bound of `lin` over the box, `None` when some
    /// atom is unbounded in the direction that matters. `exact` enumerates
    /// each single-parameter group of atoms under that parameter's facts.
    fn lower_bound(&self, lin: &Lin, exact: bool) -> Option<i128> {
        let mut groups: BTreeMap<Vec<String>, Vec<(&Expr, i64)>> = BTreeMap::new();
        for (e, c) in lin.terms.values() {
            let ps: Vec<String> = mentioned_params(e, &self.params).into_iter().collect();
            groups.entry(ps).or_default().push((e, *c));
        }
        let mut total = i128::from(lin.k);
        for (ps, atoms) in &groups {
            let interval_lb = || -> Option<i128> {
                let mut s: i128 = 0;
                for (e, c) in atoms {
                    let (lo, hi) = self.interval(e, 0)?;
                    let c = i128::from(*c);
                    let part = if c > 0 { c * lo } else { c * hi };
                    if part <= -INF {
                        return None;
                    }
                    s += part;
                }
                Some(s)
            };
            let g = match (exact, ps.as_slice()) {
                (true, [p]) => self.enumerate_min(p, atoms).or_else(interval_lb),
                _ => interval_lb(),
            }?;
            total += g;
        }
        (total > -INF).then_some(total)
    }

    fn enumerate_min(&self, p: &str, atoms: &[(&Expr, i64)]) -> Option<i128> {
        let &(lo, hi) = self.boxes.get(p)?;
        if hi - lo > ENUM_LIMIT {
            return None;
        }
        let mut best: Option<i128> = None;
        let mut env = BTreeMap::new();
        for v in lo..=hi {
            if !self.admissible(p, v) {
                continue;
            }
            env.insert(p.to_string(), v);
            let mut s: i128 = 0;
            for (e, c) in atoms {
                s += i128::from(*c) * i128::from(self.eval(e, &env, 0)?);
            }
            best = Some(best.map_or(s, |b| b.min(s)));
        }
        best
    }

    fn at_least(&self, lin: &Lin, need: i64) -> bool {
        let need = i128::from(need);
        self.lower_bound(lin, false).is_some_and(|b| b >= need)
            || self.lower_bound(lin, true).is_some_and(|b| b >= need)
    }

    /// The truth of a parameter-pure condition on every admissible point.
    fn decide(&self, cond: &Expr) -> Option<bool> {
        if let Some((lo, hi)) = self.interval(cond, 0) {
            if lo == hi {
                return Some(lo != 0);
            }
            if lo > 0 || hi < 0 {
                return Some(true);
            }
        }
        let ps: Vec<String> = mentioned_params(cond, &self.params).into_iter().collect();
        let [p] = ps.as_slice() else { return None };
        let &(lo, hi) = self.boxes.get(p)?;
        if hi - lo > ENUM_LIMIT {
            return None;
        }
        let mut seen: Option<bool> = None;
        let mut env = BTreeMap::new();
        for v in lo..=hi {
            if !self.admissible(p, v) {
                continue;
            }
            env.insert(p.clone(), v);
            let t = self.eval(cond, &env, 0)? != 0;
            if seen.is_some_and(|s| s != t) {
                return None;
            }
            seen = Some(t);
        }
        seen
    }

    fn is_int(&self, name: &str) -> bool {
        matches!(self.types.get(name), Some(Some(VarType::Integer)))
    }
}

fn decided(always: bool, never: bool) -> (i128, i128) {
    if always {
        (1, 1)
    } else if never {
        (0, 0)
    } else {
        (0, 1)
    }
}

fn isqrt(x: i128) -> i128 {
    if x >= INF {
        return INF;
    }
    #[allow(clippy::cast_possible_truncation, clippy::cast_precision_loss)]
    let r = (x as f64).sqrt() as i128;
    r
}

/// `x` of `(int)sqrt((double)x)`, the only real-valued shape the bounds model.
fn sqrt_of_real(e: &Expr) -> Option<&Expr> {
    if let Expr::FuncCall(name, args) = e {
        if name == "sqrt" && args.len() == 1 {
            if let Expr::Cast(VarType::Real, x) = &args[0] {
                return Some(x);
            }
        }
    }
    None
}

fn is_unsigned(e: &Expr) -> bool {
    let mut found = false;
    crate::streaming::walk_expr(e, &mut |x| {
        if matches!(x, Expr::Cast(VarType::Index, _)) {
            found = true;
        }
    });
    found
}

fn is_unstable_call(name: &str) -> bool {
    name == "UNSTABLE_PERIOD" || name == "UNSTABLE_AUTO"
}

fn subst(e: &Expr, env: &BTreeMap<String, Expr>) -> Expr {
    crate::streaming::rewrite_expr(e, &|x| match &x {
        Expr::Var(v) => env.get(v).cloned().unwrap_or(x),
        _ => x,
    })
}

/// A lookback body as one expression: straight-line local assignments and a
/// `return`.
fn lookback_body_expr(stmts: &[Statement], mut env: BTreeMap<String, Expr>) -> Option<Expr> {
    for s in stmts {
        match s {
            Statement::Comment(_) | Statement::VarDecl { init: None, .. } => {}
            Statement::Expr(Expr::Cast(_, x)) if matches!(x.as_ref(), Expr::Var(_)) => {}
            Statement::VarDecl { name, init: Some(e), .. } => {
                let v = subst(e, &env);
                env.insert(name.clone(), v);
            }
            Statement::Assign {
                target: Expr::Var(name),
                value,
                ..
            } => {
                let v = subst(value, &env);
                env.insert(name.clone(), v);
            }
            Statement::Return { value: Some(e) } => return Some(subst(e, &env)),
            _ => return None,
        }
    }
    None
}

// ---------------------------------------------------------------------------
// Relational tracker
// ---------------------------------------------------------------------------

/// `base + off`. Base 0 is the constant zero, so `(0, off)` is a
/// parameter-pure value; any other base is an unknown minted once, and two
/// values share a base only when one was computed from the other.
#[derive(Clone, Debug, PartialEq)]
struct Val {
    base: u32,
    off: Lin,
}

type Env = BTreeMap<String, Val>;

/// An unmodelled construct on the path to the steady loop.
struct Unsupported;

/// Per-iteration net change of each scalar a loop body writes. `None` marks a
/// write that is not an unconditional constant step.
type Deltas = BTreeMap<String, Option<i64>>;

struct Tracker<'c, 'a> {
    ctx: &'c Ctx<'a>,
    next: u32,
    /// Proven lower bounds of minted bases (`base >= lin`).
    floors: BTreeMap<u32, Vec<Lin>>,
}

impl Tracker<'_, '_> {
    fn fresh(&mut self) -> Val {
        self.next += 1;
        Val {
            base: self.next,
            off: Lin::default(),
        }
    }

    fn param(lin: Lin) -> Val {
        Val { base: 0, off: lin }
    }

    fn read(&mut self, v: &str, env: &Env) -> Val {
        if let Some(x) = env.get(v) {
            return x.clone();
        }
        if self.ctx.params.contains(v) && self.ctx.is_int(v) {
            return Self::param(Lin::atom(Expr::Var(v.to_string())));
        }
        self.fresh()
    }

    /// A parameter-pure value converted to `size_t` keeps the conversion as an
    /// atom, so a negative operand fails evaluation instead of wrapping. A
    /// relative value only ever meets `+` and `-` before the capture's `int`,
    /// which the wrap commutes with.
    fn unsigned(v: Val) -> Val {
        if v.base != 0 {
            return v;
        }
        let already = v.off.k == 0
            && v.off.terms.len() == 1
            && v.off
                .terms
                .values()
                .all(|(e, c)| *c == 1 && matches!(e, Expr::Cast(VarType::Index, _)));
        if already {
            return v;
        }
        Self::param(Lin::atom(Expr::Cast(VarType::Index, Box::new(v.off.to_expr()))))
    }

    /// The value a store into `name` leaves, after C's implicit conversion to
    /// its declared type. Anything but `int` and `size_t` is not tracked.
    fn store(&mut self, name: &str, v: Val, env: &mut Env) {
        let v = match self.ctx.types.get(name) {
            Some(Some(VarType::Integer)) => v,
            Some(Some(VarType::Index)) => Self::unsigned(v),
            _ => self.fresh(),
        };
        env.insert(name.to_string(), v);
    }

    fn pure_expr(&mut self, e: &Expr, env: &Env) -> Option<Expr> {
        let v = self.aval(e, env);
        (v.base == 0).then(|| v.off.to_expr())
    }

    fn nonneg(&self, v: &Val) -> bool {
        if v.base == 0 {
            return self.ctx.at_least(&v.off, 0);
        }
        self.floors.get(&v.base).is_some_and(|fl| {
            fl.iter()
                .any(|f| f.add(&v.off, 1).is_some_and(|s| self.ctx.at_least(&s, 0)))
        })
    }

    /// The value of `e` in `env`, reads taken before any of its side effects.
    #[allow(clippy::too_many_lines)]
    fn aval(&mut self, e: &Expr, env: &Env) -> Val {
        match e {
            Expr::IntLiteral(k) => Self::param(Lin::konst(*k)),
            Expr::Var(v) => self.read(v, env),
            Expr::PostIncrement(x) | Expr::PostDecrement(x) => match x.as_ref() {
                Expr::Var(v) => self.read(v, env),
                _ => self.fresh(),
            },
            Expr::PreIncrement(x) | Expr::PreDecrement(x) => {
                let Expr::Var(v) = x.as_ref() else { return self.fresh() };
                let d = if matches!(e, Expr::PreIncrement(_)) { 1 } else { -1 };
                let cur = self.read(v, env);
                self.shift(&cur, &Lin::konst(d))
            }
            Expr::Cast(VarType::Integer, x) => {
                if let Some(arg) = sqrt_of_real(x) {
                    return match self.pure_expr(arg, env) {
                        Some(a) => Self::param(Lin::atom(Expr::Cast(
                            VarType::Integer,
                            Box::new(Expr::FuncCall(
                                "sqrt".into(),
                                vec![Expr::Cast(VarType::Real, Box::new(a))],
                            )),
                        ))),
                        None => self.fresh(),
                    };
                }
                // `+`, `-` and constant `*` commute with the wrap to the
                // capture's `int`, and an in-range value survives the cast.
                self.aval(x, env)
            }
            Expr::Cast(VarType::Index, x) => {
                let v = self.aval(x, env);
                Self::unsigned(v)
            }
            Expr::BinOp(lhs, op @ (BinOp::Add | BinOp::Sub), rhs) => {
                let va = self.aval(lhs, env);
                let vb = self.aval(rhs, env);
                let sign = if *op == BinOp::Add { 1 } else { -1 };
                let out = if vb.base == 0 {
                    va.off.add(&vb.off, sign).map(|off| Val { base: va.base, off })
                } else if va.base == 0 && sign == 1 {
                    va.off.add(&vb.off, 1).map(|off| Val { base: vb.base, off })
                } else if va.base == vb.base && sign == -1 {
                    va.off.add(&vb.off, -1).map(Self::param)
                } else {
                    None
                };
                out.unwrap_or_else(|| self.fresh())
            }
            Expr::BinOp(lhs, BinOp::Mul, rhs) => {
                let va = self.aval(lhs, env);
                let vb = self.aval(rhs, env);
                if va.base == 0 && vb.base == 0 {
                    let lin = match (va.off.as_const(), vb.off.as_const()) {
                        (Some(c), _) => vb.off.scale(c),
                        (_, Some(c)) => va.off.scale(c),
                        _ => Some(Lin::atom(Expr::BinOp(
                            Box::new(va.off.to_expr()),
                            BinOp::Mul,
                            Box::new(vb.off.to_expr()),
                        ))),
                    };
                    return lin.map_or_else(|| self.fresh(), Self::param);
                }
                self.fresh()
            }
            Expr::Neg(x) => {
                let a = self.aval(x, env);
                match (a.base, a.off.scale(-1)) {
                    (0, Some(off)) => Self::param(off),
                    _ => self.fresh(),
                }
            }
            Expr::FuncCall(name, _) if is_unstable_call(name) => Self::param(Lin::atom(e.clone())),
            Expr::FuncCall(name, args) => {
                let mut pure = Vec::with_capacity(args.len());
                for a in args {
                    match self.pure_expr(a, env) {
                        Some(x) => pure.push(x),
                        None => return self.fresh(),
                    }
                }
                // Only integer-valued calls may become exact atoms.
                let int_valued = name.ends_with("_lookback")
                    || (matches!(name.as_str(), "max" | "min") && pure.len() == 2);
                if !int_valued {
                    return self.fresh();
                }
                if let Some(body) = self.ctx.inline_lookback(name, &pure, 1) {
                    let empty = Env::new();
                    let v = self.aval(&body, &empty);
                    if v.base == 0 {
                        return v;
                    }
                }
                Self::param(Lin::atom(Expr::FuncCall(name.clone(), pure)))
            }
            Expr::BinOp(lhs, op, rhs) => match (self.pure_expr(lhs, env), self.pure_expr(rhs, env)) {
                (Some(va), Some(vb)) => {
                    Self::param(Lin::atom(Expr::BinOp(Box::new(va), op.clone(), Box::new(vb))))
                }
                _ => self.fresh(),
            },
            Expr::Not(x) => match self.pure_expr(x, env) {
                Some(a) => Self::param(Lin::atom(Expr::Not(Box::new(a)))),
                None => self.fresh(),
            },
            Expr::Ternary(c, a, b) => {
                match (self.pure_expr(c, env), self.pure_expr(a, env), self.pure_expr(b, env)) {
                    (Some(c), Some(a), Some(b)) => Self::param(Lin::atom(Expr::Ternary(
                        Box::new(c),
                        Box::new(a),
                        Box::new(b),
                    ))),
                    _ => self.fresh(),
                }
            }
            _ => self.fresh(),
        }
    }

    fn shift(&mut self, v: &Val, by: &Lin) -> Val {
        match v.off.add(by, 1) {
            Some(off) => Val { base: v.base, off },
            None => self.fresh(),
        }
    }

    fn apply_effects(&mut self, e: &Expr, env: &mut Env) {
        let mut steps = Vec::new();
        let mut clobbered = BTreeSet::new();
        effects(e, false, &mut steps, &mut clobbered);
        for (v, d) in steps {
            let cur = self.read(&v, env);
            let next = self.shift(&cur, &Lin::konst(d));
            env.insert(v, next);
        }
        for v in clobbered {
            let f = self.fresh();
            env.insert(v, f);
        }
    }

    fn exec_seq(&mut self, stmts: &[Statement], mut env: Env) -> Result<Option<Env>, Unsupported> {
        for s in stmts {
            match self.exec(s, env)? {
                Some(e) => env = e,
                None => return Ok(None),
            }
        }
        Ok(Some(env))
    }

    /// A nested body: names it declares shadow the enclosing ones only inside.
    fn exec_scoped(&mut self, stmts: &[Statement], env: &Env) -> Result<Option<Env>, Unsupported> {
        let out = self.exec_seq(stmts, env.clone())?;
        Ok(out.map(|mut inner| {
            for s in stmts {
                if let Statement::VarDecl { name, .. } = s {
                    match env.get(name) {
                        Some(v) => inner.insert(name.clone(), v.clone()),
                        None => inner.remove(name),
                    };
                }
            }
            inner
        }))
    }

    fn join(&mut self, a: Option<Env>, b: Option<Env>) -> Option<Env> {
        let (a, b) = match (a, b) {
            (None, x) | (x, None) => return x,
            (Some(a), Some(b)) => (a, b),
        };
        let names: BTreeSet<String> = a.keys().chain(b.keys()).cloned().collect();
        let mut out = Env::new();
        for n in names {
            let v = match (a.get(&n), b.get(&n)) {
                (Some(x), Some(y)) if x == y => x.clone(),
                _ => self.fresh(),
            };
            out.insert(n, v);
        }
        Some(out)
    }

    #[allow(clippy::too_many_lines)]
    fn exec(&mut self, s: &Statement, mut env: Env) -> Result<Option<Env>, Unsupported> {
        match s {
            Statement::Comment(_) | Statement::UnrollHint { .. } => {}
            Statement::VarDecl { name, init, .. } => {
                let v = match init {
                    Some(e) => {
                        let v = self.aval(e, &env);
                        self.apply_effects(e, &mut env);
                        v
                    }
                    None => self.fresh(),
                };
                self.store(name, v, &mut env);
            }
            Statement::Assign { target, value, .. } => {
                let v = self.aval(value, &env);
                self.apply_effects(value, &mut env);
                if let Expr::Var(name) = target {
                    let mut touched = Vec::new();
                    let mut clobbered = BTreeSet::new();
                    effects(value, false, &mut touched, &mut clobbered);
                    let v = if touched.iter().any(|(t, _)| t == name) || clobbered.contains(name) {
                        self.fresh()
                    } else {
                        v
                    };
                    self.store(name, v, &mut env);
                } else {
                    self.apply_effects(target, &mut env);
                }
            }
            Statement::Expr(e) => self.apply_effects(e, &mut env),
            Statement::Return { .. } => return Ok(None),
            Statement::Break | Statement::Continue => return Err(Unsupported),
            Statement::Block { body } => return self.exec_scoped(body, &env),
            Statement::If {
                condition,
                then_body,
                else_body,
                ..
            } => {
                if let Some(e) = self.clamp(condition, then_body, else_body, &env) {
                    return Ok(Some(e));
                }
                let verdict = self.pure_expr(condition, &env).and_then(|c| self.ctx.decide(&c));
                self.apply_effects(condition, &mut env);
                return match verdict {
                    Some(true) => self.exec_scoped(then_body, &env),
                    Some(false) => self.exec_scoped(else_body, &env),
                    None => {
                        let t = self.exec_scoped(then_body, &env)?;
                        let e = self.exec_scoped(else_body, &env)?;
                        Ok(self.join(t, e))
                    }
                };
            }
            Statement::While { .. }
            | Statement::DoWhile { .. }
            | Statement::For { .. }
            | Statement::ForC { .. } => {
                if let Statement::ForC { init, .. } = s {
                    match self.exec(init, env)? {
                        Some(e) => env = e,
                        None => return Ok(None),
                    }
                }
                return Ok(Some(self.exec_loop(s, env)));
            }
            Statement::Switch { .. } | Statement::CircBuf(_) => {
                for n in written_names(std::slice::from_ref(s)) {
                    let f = self.fresh();
                    env.insert(n, f);
                }
            }
        }
        Ok(Some(env))
    }

    /// `if (x < E) x = E;`: afterwards `x >= E`. With an unsigned comparison
    /// that holds only when neither side is negative.
    fn clamp(&mut self, cond: &Expr, then_body: &[Statement], else_body: &[Statement], env: &Env) -> Option<Env> {
        let Expr::BinOp(l, BinOp::Less, r) = cond else { return None };
        let Expr::Var(x) = l.as_ref() else { return None };
        let code: Vec<&Statement> = then_body
            .iter()
            .filter(|s| !matches!(s, Statement::Comment(_)))
            .collect();
        let [Statement::Assign {
            target: Expr::Var(tx),
            value,
            ..
        }] = code.as_slice()
        else {
            return None;
        };
        if tx != x || !else_body.is_empty() || !crate::streaming::exprs_equal(value, r) || has_effects(r) {
            return None;
        }
        let bound = self.aval(r, env);
        if bound.base != 0 {
            return None;
        }
        let signed = self.ctx.is_int(x) && expr_is_signed(r, self.ctx);
        let x0 = self.read(x, env);
        if !(signed || self.nonneg(&x0) && self.ctx.at_least(&bound.off, 0)) {
            return None;
        }
        let fresh = self.fresh();
        let mut floors = vec![bound.off];
        if self.nonneg(&x0) {
            floors.push(Lin::konst(0));
        }
        self.floors.insert(fresh.base, floors);
        let mut out = env.clone();
        out.insert(x.clone(), fresh);
        Some(out)
    }

    fn exec_loop(&mut self, s: &Statement, mut env: Env) -> Env {
        let (cond, body, update): (Option<&Expr>, &[Statement], Option<&Statement>) = match s {
            Statement::While { condition, body } | Statement::DoWhile { condition, body } => {
                (Some(condition), body, None)
            }
            Statement::ForC {
                condition,
                update,
                body,
                ..
            } => (Some(condition), body, Some(update.as_ref())),
            Statement::For { body, .. } => (None, body, None),
            _ => unreachable!("exec_loop takes a loop"),
        };
        let mut all: Vec<Statement> = body.to_vec();
        if let Some(u) = update {
            all.push(u.clone());
        }
        let mut written = written_names(&all);
        let mut cond_written = BTreeSet::new();
        if let Some(c) = cond {
            let mut steps = Vec::new();
            effects(c, false, &mut steps, &mut cond_written);
            cond_written.extend(steps.into_iter().map(|(v, _)| v));
        }
        if let Statement::For { var, .. } = s {
            cond_written.insert(var.clone());
        }
        written.extend(cond_written.iter().cloned());
        let declared = declared_names(&all);
        let jumps = has_jump(&all);
        let deltas = delta_seq(&all);
        let trips = if jumps {
            None
        } else {
            self.trip_count(s, &env, &deltas, &written, &cond_written)
        };

        let mut groups: BTreeMap<(u32, i64), u32> = BTreeMap::new();
        for v in &written {
            let step = match deltas.get(v) {
                _ if jumps || declared.contains(v) || cond_written.contains(v) => None,
                Some(Some(d)) => Some(*d),
                Some(None) | None => None,
            };
            let cur = self.read(v, &env);
            let next = match (step, &trips) {
                (Some(0), _) => cur,
                (Some(d), Some(t)) => match t.scale(d) {
                    Some(by) => self.shift(&cur, &by),
                    None => self.fresh(),
                },
                (Some(d), None) => {
                    // Unknown trip count t: every value with the same base
                    // and step moves together, onto a base standing for
                    // `base + d * t`.
                    let base = *groups
                        .entry((cur.base, d))
                        .or_insert_with(|| self.fresh().base);
                    Val { base, off: cur.off }
                }
                (None, _) => self.fresh(),
            };
            env.insert(v.clone(), next);
        }
        env
    }

    /// The number of body executions as a parameter-pure form, only when
    /// it is exact.
    fn trip_count(
        &mut self,
        s: &Statement,
        env: &Env,
        deltas: &Deltas,
        written: &BTreeSet<String>,
        cond_written: &BTreeSet<String>,
    ) -> Option<Lin> {
        let max0 = |t: &Self, lin: Lin| t.ctx.at_least(&lin, 0).then_some(lin);
        match s {
            Statement::While { condition, .. } => {
                if let Expr::BinOp(lhs, BinOp::Greater, rhs) = condition {
                    if let (Expr::PostDecrement(x), Expr::IntLiteral(0)) = (lhs.as_ref(), rhs.as_ref()) {
                        let Expr::Var(i) = x.as_ref() else { return None };
                        if deltas.contains_key(i) {
                            return None;
                        }
                        let i0 = self.read(i, env);
                        return (i0.base == 0).then_some(i0.off).and_then(|lin| max0(self, lin));
                    }
                }
                self.run_to_bound(condition, env, deltas, written, cond_written)
            }
            Statement::ForC { condition, .. } => {
                self.run_to_bound(condition, env, deltas, written, cond_written)
            }
            _ => None,
        }
    }

    /// `i < B`, `i <= B` (step +1) or `i > B`, `i >= B` (step -1), with `B`
    /// fixed across the loop: `max(0, B - i0)` executions, give or take one.
    fn run_to_bound(
        &mut self,
        cond: &Expr,
        env: &Env,
        deltas: &Deltas,
        written: &BTreeSet<String>,
        cond_written: &BTreeSet<String>,
    ) -> Option<Lin> {
        let Expr::BinOp(l, op, bound) = cond else { return None };
        let Expr::Var(i) = l.as_ref() else { return None };
        if !cond_written.is_empty() || has_effects(bound) {
            return None;
        }
        let mut bound_vars = BTreeSet::new();
        crate::streaming::expr_var_names(bound, &mut bound_vars);
        if bound_vars.iter().any(|v| written.contains(v)) {
            return None;
        }
        let (step, extra) = match op {
            BinOp::Less => (1, 0),
            BinOp::LessEq => (1, 1),
            BinOp::Greater => (-1, 0),
            BinOp::GreaterEq => (-1, 1),
            _ => return None,
        };
        if deltas.get(i) != Some(&Some(step)) {
            return None;
        }
        let i0 = self.read(i, env);
        let b = self.aval(bound, env);
        let signed = self.ctx.is_int(i) && expr_is_signed(bound, self.ctx);
        if !(signed || self.nonneg(&i0) && self.nonneg(&b)) {
            return None;
        }
        let (hi, lo) = if step == 1 { (&b, &i0) } else { (&i0, &b) };
        if hi.base != lo.base {
            return None;
        }
        let span = hi.off.add(&lo.off, -1)?.add(&Lin::konst(extra), 1)?;
        self.ctx.at_least(&span, 0).then_some(span)
    }
}

/// Whether `e` is evaluated in signed `int` arithmetic: every name in it an
/// `int` and no unsigned cast.
fn expr_is_signed(e: &Expr, ctx: &Ctx) -> bool {
    let mut ok = true;
    crate::streaming::walk_expr(e, &mut |x| match x {
        Expr::Var(v) if !ctx.is_int(v) => ok = false,
        Expr::Cast(t, _) if *t != VarType::Integer => ok = false,
        Expr::Literal(_) | Expr::ArrayAccess(..) | Expr::PointerDeref(_) => ok = false,
        _ => {}
    });
    ok
}

// ---------------------------------------------------------------------------
// Side effects and per-iteration deltas
// ---------------------------------------------------------------------------

/// Increments and decrements `e` performs on named scalars. A step that may
/// not run (under `&&`, `||` or `?:`) and any `&x` land in `clobbered`.
fn effects(e: &Expr, cond: bool, steps: &mut Vec<(String, i64)>, clobbered: &mut BTreeSet<String>) {
    match e {
        Expr::PostIncrement(x) | Expr::PreIncrement(x) | Expr::PostDecrement(x) | Expr::PreDecrement(x) => {
            if let Expr::Var(v) = x.as_ref() {
                let d = if matches!(e, Expr::PostIncrement(_) | Expr::PreIncrement(_)) { 1 } else { -1 };
                if cond {
                    clobbered.insert(v.clone());
                } else {
                    steps.push((v.clone(), d));
                }
            } else {
                effects(x, cond, steps, clobbered);
            }
        }
        Expr::AddressOf(x) => {
            match x.as_ref() {
                Expr::Var(v) | Expr::ArrayAccess(v, _) => {
                    clobbered.insert(v.clone());
                }
                _ => {}
            }
            effects(x, cond, steps, clobbered);
        }
        Expr::BinOp(l, op, r) => {
            effects(l, cond, steps, clobbered);
            effects(r, cond || matches!(op, BinOp::And | BinOp::Or), steps, clobbered);
        }
        Expr::Ternary(c, a, b) => {
            effects(c, cond, steps, clobbered);
            effects(a, true, steps, clobbered);
            effects(b, true, steps, clobbered);
        }
        Expr::ArrayAccess(_, x) | Expr::Cast(_, x) | Expr::Not(x) | Expr::Neg(x) | Expr::BitwiseNot(x) => {
            effects(x, cond, steps, clobbered);
        }
        Expr::FuncCall(_, args) => {
            for a in args {
                effects(a, cond, steps, clobbered);
            }
        }
        Expr::Literal(_) | Expr::IntLiteral(_) | Expr::Var(_) | Expr::PointerDeref(_) => {}
    }
}

fn has_effects(e: &Expr) -> bool {
    let mut steps = Vec::new();
    let mut clobbered = BTreeSet::new();
    effects(e, false, &mut steps, &mut clobbered);
    !steps.is_empty() || !clobbered.is_empty()
}

fn expr_writes(e: &Expr, out: &mut BTreeSet<String>) {
    let mut steps = Vec::new();
    effects(e, false, &mut steps, out);
    out.extend(steps.into_iter().map(|(v, _)| v));
}

/// Every scalar name a statement list may write, at any depth.
fn written_names(stmts: &[Statement]) -> BTreeSet<String> {
    let mut out = BTreeSet::new();
    for s in stmts {
        crate::streaming::walk_stmt_exprs(s, &mut |e| expr_writes(e, &mut out));
        walk_stmts(s, &mut |st| match st {
            Statement::Assign { target: Expr::Var(v), .. } | Statement::VarDecl { name: v, .. } => {
                out.insert(v.clone());
            }
            Statement::For { var, .. } => {
                out.insert(var.clone());
            }
            Statement::CircBuf(cb) => {
                let id = match cb {
                    CircBuf::Prolog { id, .. }
                    | CircBuf::Init { id, .. }
                    | CircBuf::InitLocalOnly { id, .. }
                    | CircBuf::Next { id }
                    | CircBuf::Destroy { id, .. } => id,
                };
                out.insert(format!("{id}_Idx"));
                out.insert(format!("maxIdx_{id}"));
                out.insert(id.clone());
            }
            _ => {}
        });
    }
    out
}

fn walk_stmts(s: &Statement, f: &mut dyn FnMut(&Statement)) {
    f(s);
    if let Statement::ForC { init, update, .. } = s {
        walk_stmts(init, f);
        walk_stmts(update, f);
    }
    let (bodies, _) = crate::streaming::nested_bodies(s);
    for b in bodies {
        for st in b {
            walk_stmts(st, f);
        }
    }
}

fn declared_names(stmts: &[Statement]) -> BTreeSet<String> {
    let mut out = BTreeSet::new();
    for s in stmts {
        walk_stmts(s, &mut |st| {
            if let Statement::VarDecl { name, .. } = st {
                out.insert(name.clone());
            }
        });
    }
    out
}

fn has_jump(stmts: &[Statement]) -> bool {
    let mut found = false;
    for s in stmts {
        walk_stmts(s, &mut |st| {
            if matches!(st, Statement::Break | Statement::Continue | Statement::Return { .. }) {
                found = true;
            }
        });
    }
    found
}

fn add_delta(map: &mut Deltas, v: &str, d: Option<i64>) {
    let slot = map.entry(v.to_string()).or_insert(Some(0));
    *slot = match (*slot, d) {
        (Some(a), Some(b)) => a.checked_add(b),
        _ => None,
    };
}

fn expr_deltas(e: &Expr, map: &mut Deltas) {
    let mut steps = Vec::new();
    let mut clobbered = BTreeSet::new();
    effects(e, false, &mut steps, &mut clobbered);
    for (v, d) in steps {
        add_delta(map, &v, Some(d));
    }
    for v in clobbered {
        add_delta(map, &v, None);
    }
}

/// Net change per pass through `stmts`, for every scalar they write.
fn delta_seq(stmts: &[Statement]) -> Deltas {
    let mut map = Deltas::new();
    for s in stmts {
        match s {
            Statement::Assign { target, value, .. } => {
                expr_deltas(value, &mut map);
                match target {
                    Expr::Var(v) => {
                        let step = match value {
                            Expr::BinOp(l, op @ (BinOp::Add | BinOp::Sub), r)
                                if matches!(l.as_ref(), Expr::Var(x) if x == v) =>
                            {
                                match r.as_ref() {
                                    Expr::IntLiteral(k) => Some(if *op == BinOp::Add { *k } else { -*k }),
                                    _ => None,
                                }
                            }
                            _ => None,
                        };
                        let mut own = Deltas::new();
                        expr_deltas(value, &mut own);
                        add_delta(&mut map, v, if own.contains_key(v) { None } else { step });
                    }
                    other => expr_deltas(other, &mut map),
                }
            }
            Statement::VarDecl { name, init, .. } => {
                if let Some(e) = init {
                    expr_deltas(e, &mut map);
                }
                add_delta(&mut map, name, None);
            }
            Statement::Expr(e) => expr_deltas(e, &mut map),
            Statement::If {
                condition,
                then_body,
                else_body,
                ..
            } => {
                expr_deltas(condition, &mut map);
                let t = delta_seq(then_body);
                let e = delta_seq(else_body);
                let names: BTreeSet<&String> = t.keys().chain(e.keys()).collect();
                for v in names {
                    let a = t.get(v).copied().unwrap_or(Some(0));
                    let b = e.get(v).copied().unwrap_or(Some(0));
                    add_delta(&mut map, v, if a == b { a } else { None });
                }
            }
            Statement::Block { body } => {
                for (v, d) in delta_seq(body) {
                    add_delta(&mut map, &v, d);
                }
            }
            Statement::Comment(_)
            | Statement::UnrollHint { .. }
            | Statement::Break
            | Statement::Continue
            | Statement::Return { .. } => {}
            _ => {
                for v in written_names(std::slice::from_ref(s)) {
                    add_delta(&mut map, &v, None);
                }
            }
        }
    }
    map
}

// ---------------------------------------------------------------------------
// Entry point
// ---------------------------------------------------------------------------

/// The body Open transcribes for one stream model, split where the proof
/// needs it.
pub struct LagProofInput<'a> {
    /// Statements ahead of `region` (the dual-mode shared prologue).
    pub prologue: &'a [Statement],
    /// The body holding the steady loop at its top level.
    pub region: &'a [Statement],
    /// Statements after `region` (the dual-mode shared epilogue).
    pub epilogue: &'a [Statement],
    pub cursor: &'a str,
    /// Conditions that hold on every path reaching the ring's transition.
    pub facts: Vec<Expr>,
    /// Resolves the lookbacks of other functions the body calls.
    pub lookup: &'a dyn CalleeLookup,
}

/// Every other function's lookback reachable from `func`'s body through
/// lookback calls, resolved once so the proof reads only its arguments.
fn resolve_callees(func: &FuncDef, lookup: &dyn CalleeLookup) -> BTreeMap<String, CalleeLookback> {
    fn called(stmts: &[Statement], out: &mut Vec<String>) {
        for s in stmts {
            crate::streaming::walk_stmt_exprs(s, &mut |e| {
                crate::streaming::walk_expr(e, &mut |x| {
                    if let Expr::FuncCall(name, _) = x {
                        if let Some(c) = name.strip_suffix("_lookback") {
                            out.push(c.to_ascii_lowercase());
                        }
                    }
                });
            });
        }
    }
    let own = func.name.to_ascii_lowercase();
    let mut todo = Vec::new();
    called(func.stream_source(), &mut todo);
    if let Some(LookbackExpr::Code(body)) = &func.lookback {
        called(body, &mut todo);
    }
    let mut out = BTreeMap::new();
    let mut seen = BTreeSet::new();
    while let Some(name) = todo.pop() {
        if name == own || !seen.insert(name.clone()) {
            continue;
        }
        if let Some(def) = lookup.lookback(&name) {
            called(&def.body, &mut todo);
            out.insert(name, def);
        }
    }
    out
}

/// Lower bound of `cursor - var` at steady-loop entry for each ring variable,
/// `None` where nothing could be proved.
///
/// Memoized: every emitter re-analyzes each function. The key covers every
/// input the proof reads, including the callee lookbacks it resolved.
pub fn ring_lag_lower_bounds(func: &FuncDef, input: &LagProofInput, vars: &[&str]) -> Vec<Option<i64>> {
    use std::hash::{Hash, Hasher};
    type Memo = Mutex<BTreeMap<(String, u64), Vec<Option<i64>>>>;
    static MEMO: OnceLock<Memo> = OnceLock::new();
    let callees = resolve_callees(func, input.lookup);
    let mut h = std::collections::hash_map::DefaultHasher::new();
    format!(
        "{:?}",
        (
            func.stream_source(),
            &func.optional_inputs,
            &func.lookback,
            input.prologue,
            input.region,
            input.epilogue,
            input.cursor,
            &input.facts,
            vars,
            &callees,
        )
    )
    .hash(&mut h);
    let key = (func.name.clone(), h.finish());
    let memo = MEMO.get_or_init(Default::default);
    if let Some(hit) = memo.lock().expect("lag memo").get(&key) {
        return hit.clone();
    }
    let out = prove(func, input, vars, callees);
    memo.lock().expect("lag memo").insert(key, out.clone());
    out
}

fn prove(
    func: &FuncDef,
    input: &LagProofInput,
    vars: &[&str],
    callees: BTreeMap<String, CalleeLookback>,
) -> Vec<Option<i64>> {
    let none = vec![None; vars.len()];
    let ctx = Ctx::new(func, &input.facts, callees);
    let all: Vec<&Statement> = input
        .prologue
        .iter()
        .chain(input.region)
        .chain(input.epilogue)
        .collect();
    let mut w = BTreeSet::new();
    for s in &all {
        w.extend(written_names(std::slice::from_ref(*s)));
    }
    // A reassigned parameter no longer means its validated value.
    if w.iter().any(|n| ctx.params.contains(n)) {
        return none;
    }
    let Some(steady_idx) = crate::streaming::steady_loop_index(input.region) else {
        return none;
    };
    let steady = &input.region[steady_idx];
    let after = written_names(&input.region[steady_idx + 1..])
        .into_iter()
        .chain(written_names(input.epilogue))
        .collect::<BTreeSet<_>>();

    let mut t = Tracker {
        ctx: &ctx,
        next: 0,
        floors: BTreeMap::new(),
    };
    let mut env = Env::new();
    let start = t.fresh();
    t.floors.insert(start.base, vec![Lin::konst(0)]);
    env.insert("startIdx".into(), start);
    let end = t.fresh();
    env.insert("endIdx".into(), end);
    let Ok(Some(mut env)) = t.exec_seq(input.prologue, env) else { return none };
    let Ok(Some(e)) = t.exec_seq(&input.region[..steady_idx], env) else { return none };
    env = e;
    let (per_bar, cond): (Vec<Statement>, Option<&Expr>) = match steady {
        Statement::ForC {
            init,
            update,
            body,
            condition,
        } => {
            let Ok(Some(e)) = t.exec(init, env) else { return none };
            env = e;
            let mut v = body.clone();
            v.push(update.as_ref().clone());
            (v, Some(condition))
        }
        Statement::While { condition, body } | Statement::DoWhile { condition, body } => {
            (body.clone(), Some(condition))
        }
        _ => return none,
    };
    let steps = delta_seq(&per_bar);
    let mut cond_written = BTreeSet::new();
    if let Some(c) = cond {
        expr_writes(c, &mut cond_written);
    }
    let paced = |v: &str| -> Option<i64> {
        if cond_written.contains(v) || after.contains(v) || has_jump(&per_bar) {
            return None;
        }
        steps.get(v).copied().flatten().filter(|d| *d != 0)
    };
    let Some(cursor_step) = paced(input.cursor) else { return none };
    let cursor = t.read(input.cursor, &env);
    vars.iter()
        .map(|v| {
            if paced(v) != Some(cursor_step) {
                return None;
            }
            let var = t.read(v, &env);
            if var.base != cursor.base {
                return None;
            }
            let d = cursor.off.add(&var.off, -1)?;
            let lb = ctx
                .lower_bound(&d, false)
                .max(ctx.lower_bound(&d, true))?;
            i64::try_from(lb).ok()
        })
        .collect()
}

#[cfg(test)]
mod tests {
    use super::*;

    fn base() -> std::path::PathBuf {
        std::path::Path::new(env!("CARGO_MANIFEST_DIR")).join("../input")
    }

    fn load(name: &str) -> FuncDef {
        let base = base();
        let mut f = crate::parser::yaml::parse_yaml(&base.join(format!("{name}/{name}.yaml")));
        let parsed = crate::parser::c_source::parse_c_source(&base.join(format!("{name}/{name}.c")));
        crate::parser::c_source::wire_parsed_source(&mut f, &parsed);
        f
    }

    fn period(op: BinOp, k: i64) -> Expr {
        Expr::BinOp(
            Box::new(Expr::Var("optInTimePeriod".into())),
            op,
            Box::new(Expr::IntLiteral(k)),
        )
    }

    #[test]
    fn facts_narrow_the_enumerated_parameters() {
        let trima = load("trima");
        let p = || Expr::Var("optInTimePeriod".into());
        let lb = |ctx: &Ctx, e: Expr, scale: i64| ctx.lower_bound(&Lin::atom(e).scale(scale).unwrap(), true);
        let odd = Expr::BinOp(
            Box::new(Expr::BinOp(Box::new(p()), BinOp::Mod, Box::new(Expr::IntLiteral(2)))),
            BinOp::Eq,
            Box::new(Expr::IntLiteral(1)),
        );
        let half = Expr::BinOp(Box::new(p()), BinOp::Shr, Box::new(Expr::IntLiteral(1)));
        assert_eq!(lb(&Ctx::new(&trima, &[], BTreeMap::new()), half.clone(), 1), Some(0));
        let even = Ctx::new(&trima, &[Expr::Not(Box::new(odd))], BTreeMap::new());
        assert_eq!(lb(&even, half, 1), Some(1));

        let small = Expr::BinOp(
            Box::new(period(BinOp::Eq, 2)),
            BinOp::Or,
            Box::new(period(BinOp::Eq, 3)),
        );
        let not_one = Expr::Not(Box::new(period(BinOp::Eq, 1)));
        let a = Ctx::new(&trima, &[not_one.clone(), small.clone()], BTreeMap::new());
        assert_eq!((lb(&a, p(), 1), lb(&a, p(), -1)), (Some(2), Some(-3)));
        let b = Ctx::new(&trima, &[not_one, Expr::Not(Box::new(small))], BTreeMap::new());
        assert_eq!(lb(&b, p(), 1), Some(4));
    }

    fn single(func: &FuncDef) -> Vec<Option<i64>> {
        let m = crate::streaming::analyze(func).expect("streams");
        let input = LagProofInput {
            prologue: &[],
            region: m.body,
            epilogue: &[],
            cursor: &m.cursor,
            facts: m.identity.iter().map(|i| Expr::Not(Box::new(i.condition.clone()))).collect(),
            lookup: &crate::streaming::FuncsLookup(&[]),
        };
        let vars: Vec<&str> = m.rings().iter().map(|r| r.var.as_str()).collect();
        ring_lag_lower_bounds(func, &input, &vars)
    }

    #[test]
    fn warm_up_loops_carry_the_lag() {
        // `while (i-- > 0)` advancing only the cursor, P >= 2 past the identity.
        assert_eq!(single(&load("kama")), vec![Some(2)]);
        // Three straight-line steps, then a `do .. while (--i != 0)` moving both.
        assert_eq!(single(&load("ht_dcperiod")), vec![Some(3)]);
        // (P - 1) / 2 is 0 at P = 2, the first period past the identity.
        assert_eq!(single(&load("zlema")), vec![Some(0)]);
    }

    #[test]
    fn hma_general_mode_counts_its_bounded_warm_up() {
        let hma = load("hma");
        let callees = [load("wma")];
        let lookup = crate::streaming::FuncsLookup(&callees);
        let d = crate::streaming::analyze_dual_mode(&hma).expect("dual mode");
        let facts = vec![
            Expr::Not(Box::new(d.mode_b.identity.as_ref().expect("identity").condition.clone())),
            Expr::Not(Box::new(d.predicate.clone())),
        ];
        let input = LagProofInput {
            prologue: d.prologue,
            region: d.mode_b.body,
            epilogue: d.epilogue,
            cursor: &d.mode_b.cursor,
            facts,
            lookup: &lookup,
        };
        let vars: Vec<&str> = d.mode_b.rings().iter().map(|r| r.var.as_str()).collect();
        let got: BTreeMap<&str, Option<i64>> =
            vars.iter().copied().zip(ring_lag_lower_bounds(&hma, &input, &vars)).collect();
        // Full: P - 1 at P = 4, both indices moving through the warm-up.
        assert_eq!(got["trailingIdxFull"], Some(3));
        // Half: P/2 - 1 at P = 4, after a `for` that advances only it.
        assert_eq!(got["trailingIdxHalf"], Some(1));
    }

    /// A ROC-shaped body with its own trailing ring; `lag` is spliced into the
    /// batch body as statements computing `inIdx` and `trailingIdx`.
    fn synthetic(lag: &str) -> FuncDef {
        let src = format!(
            "int roc_lookback(int optInTimePeriod)\n{{\n   return optInTimePeriod;\n}}\n\n\
             TA_RetCode roc(int startIdx, int endIdx, const double inReal[], int optInTimePeriod,\n\
             int *outBegIdx, int *outNBElement, double outReal[])\n{{\n\
             int inIdx, outIdx, trailingIdx, lb;\n\
             if( startIdx > endIdx ) return TA_SUCCESS;\n\
             outIdx = 0;\n{lag}\n\
             while( inIdx <= endIdx ) outReal[outIdx++] = inReal[inIdx++] - inReal[trailingIdx++];\n\
             *outNBElement = outIdx;\n*outBegIdx = startIdx;\nreturn TA_SUCCESS;\n}}\n"
        );
        let mut f = crate::parser::yaml::parse_yaml(&base().join("roc/roc.yaml"));
        let parsed = crate::parser::c_source::parse_c_source_str(&src);
        crate::parser::c_source::wire_parsed_source(&mut f, &parsed);
        f
    }

    fn proven(f: &FuncDef, lookup: &dyn CalleeLookup) -> bool {
        let m = crate::streaming::analyze_with(f, lookup).expect("streams");
        let [ring] = m.rings() else { panic!("one ring") };
        ring.lag_ge1
    }

    #[test]
    fn a_callee_lookback_outside_its_range_proves_nothing() {
        // MOM's lookback is its period over [1, ..]: at a period of 1 the
        // argument is 0, the generated lookback answers -1 and the lag is 0.
        let callees = [load("mom")];
        let lookup = crate::streaming::FuncsLookup(&callees);
        let lag = |arg: &str| {
            format!("lb = mom_lookback( {arg} ) + 1;\ninIdx = startIdx;\ntrailingIdx = startIdx - lb;")
        };
        assert!(proven(&synthetic(&lag("optInTimePeriod")), &lookup));
        assert!(!proven(&synthetic(&lag("optInTimePeriod - 1")), &lookup));
    }

    #[test]
    fn a_real_valued_call_is_not_an_integer_atom() {
        // Read as integer atoms the two offsets differ by exactly 1; in C the
        // second rounds `P / 2.0 * 2` back to P, so an odd period lags by 0.
        let lag = |call: &str| {
            synthetic(&format!(
                "inIdx = startIdx + 2 * (int)({call}(optInTimePeriod, 0) / 2) + 1;\n\
                 trailingIdx = startIdx + (int)({call}(optInTimePeriod, 0) / 2 * 2);"
            ))
        };
        let none = crate::streaming::FuncsLookup(&[]);
        assert!(proven(&lag("max"), &none));
        assert!(!proven(&lag("fmax"), &none));
    }
}
