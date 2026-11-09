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
use crate::streaming::{CalleeLookback, CalleeLookup, SubCallStep};

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
    /// Real parameters and `double` locals.
    reals: BTreeSet<String>,
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
    fn mentions_real(&self, l: &Lin) -> bool {
        let mut found = false;
        for (e, _) in l.terms.values() {
            crate::streaming::walk_expr(e, &mut |x| {
                if matches!(x, Expr::Var(n) if self.reals.contains(n) && self.params.contains(n)) {
                    found = true;
                }
            });
        }
        found
    }

    fn new(func: &'a FuncDef, facts: &[Expr], callees: BTreeMap<String, CalleeLookback>) -> Self {
        let params: BTreeSet<String> = func.optional_inputs.iter().map(|p| p.name.clone()).collect();
        let mut types: BTreeMap<String, Option<VarType>> = BTreeMap::new();
        collect_decl_types(func.stream_source(), &mut types);
        for n in ["startIdx", "endIdx"] {
            types.insert(n.to_string(), Some(VarType::Integer));
        }
        let reals = func
            .optional_inputs
            .iter()
            .filter(|p| p.param_type == ParamType::Real)
            .map(|p| p.name.clone())
            .chain(
                types
                    .iter()
                    .filter(|(_, t)| matches!(t, Some(VarType::Real)))
                    .map(|(n, _)| n.clone()),
            )
            .collect();
        let mut ctx = Ctx {
            func,
            params: params.clone(),
            boxes: BTreeMap::new(),
            facts: BTreeMap::new(),
            types,
            reals,
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

    /// Whether `<f>_lookback(args)` is at least 0 by the declarations alone:
    /// each argument is a parameter whose declared range (or enum) lies inside
    /// the callee parameter's, or an integer literal inside it. A lookback
    /// answers -1 only for an argument outside its range.
    fn lookback_axiom(&self, name: &str, args: &[Expr]) -> bool {
        let Some(callee) = name.strip_suffix("_lookback") else { return false };
        let params = if callee.eq_ignore_ascii_case(&self.func.name) {
            &self.func.optional_inputs
        } else {
            match self.callees.get(&callee.to_ascii_lowercase()) {
                Some(def) => &def.params,
                None => return false,
            }
        };
        params.len() == args.len()
            && params.iter().zip(args).all(|(p, a)| match a {
                Expr::Var(v) => self.func.optional_inputs.iter().any(|o| {
                    o.name == *v
                        && o.param_type == p.param_type
                        && match (&p.param_type, o.range, p.range) {
                            (ParamType::Enum(_), _, _) => true,
                            (_, Some((lo, hi)), Some((clo, chi))) => clo <= lo && hi <= chi,
                            _ => false,
                        }
                }),
                #[allow(clippy::cast_possible_truncation)]
                Expr::IntLiteral(k) => {
                    p.param_type == ParamType::Integer
                        && p.range.is_some_and(|(lo, hi)| (lo.ceil() as i64..=hi.floor() as i64).contains(k))
                }
                _ => false,
            })
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
            Expr::FuncCall(name, args) => self
                .inline_lookback(name, args, depth + 1)
                .and_then(|body| self.interval(&body, depth + 1))
                .or_else(|| self.lookback_axiom(name, args).then_some((0, INF))),
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

/// A lookback body as one expression: straight-line local assignments,
/// `(void)` casts and a `return`.
fn lookback_body_expr(stmts: &[Statement], mut env: BTreeMap<String, Expr>) -> Option<Expr> {
    for s in stmts {
        match s {
            Statement::Comment(_) | Statement::VarDecl { init: None, .. } => {}
            Statement::Block { body } if body.is_empty() => {}
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

/// Where an unknown came from. The trailing-ring proof reads only identity;
/// the composed walk refuses a delay by the tags left in it.
#[derive(Clone, Copy, Debug, PartialEq, Eq, PartialOrd, Ord)]
pub(crate) enum Tag {
    /// `endIdx`.
    V,
    /// One version of `startIdx`.
    S,
    /// Sub-call `k`'s begin index, when no rule resolved it.
    B(usize),
    /// Free of `endIdx` and of data.
    I,
    /// May depend on data or on `endIdx`.
    Top,
}

/// An unknown, minted once: equal ids are equal numbers.
#[derive(Clone, Copy, Debug, PartialEq, Eq, PartialOrd, Ord)]
struct Atom {
    id: u32,
    tag: Tag,
}

/// `sum(coef * unknown) + off`, `off` over parameter atoms. Arithmetic
/// cancels an unknown whatever its tag; only an unknown left in a result
/// constrains it.
#[derive(Clone, Debug, Default)]
pub(crate) struct Val {
    terms: BTreeMap<Atom, i64>,
    off: Lin,
    /// Read from a begin receiver, through copies and casts only.
    beg: bool,
    /// Built from a subtraction of a begin index that no enclosing guard
    /// orders: an empty sub-output resets its begin to 0. Never cleared.
    usub: bool,
}

impl Val {
    fn param(off: Lin) -> Self {
        Val {
            off,
            ..Val::default()
        }
    }

    pub(crate) fn konst(k: i64) -> Self {
        Self::param(Lin::konst(k))
    }

    fn unknown(a: Atom) -> Self {
        Val {
            terms: std::iter::once((a, 1)).collect(),
            ..Val::default()
        }
    }

    /// Free of unknowns: a function of the parameters alone.
    pub(crate) fn is_param(&self) -> bool {
        self.terms.is_empty()
    }

    pub(crate) fn as_const(&self) -> Option<i64> {
        if self.is_param() {
            self.off.as_const()
        } else {
            None
        }
    }

    /// Equal as numbers; the flags are not compared.
    pub(crate) fn same(&self, other: &Val) -> bool {
        self.terms == other.terms && self.off == other.off
    }

    /// `self + sign * other`, `None` on overflow. No longer a begin index.
    pub(crate) fn plus(&self, other: &Val, sign: i64) -> Option<Val> {
        let mut terms = self.terms.clone();
        for (a, c) in &other.terms {
            let slot = terms.entry(*a).or_insert(0);
            *slot = slot.checked_add(c.checked_mul(sign)?)?;
        }
        terms.retain(|_, c| *c != 0);
        Some(Val {
            terms,
            off: self.off.add(&other.off, sign)?,
            beg: false,
            usub: self.usub || other.usub,
        })
    }

    fn scaled(&self, s: i64) -> Option<Val> {
        let mut terms = BTreeMap::new();
        if s != 0 {
            for (a, c) in &self.terms {
                terms.insert(*a, c.checked_mul(s)?);
            }
        }
        Some(Val {
            terms,
            off: self.off.scale(s)?,
            beg: false,
            usub: self.usub,
        })
    }

    fn tagged(&self, f: impl Fn(Tag) -> bool) -> bool {
        self.terms.keys().any(|a| f(a.tag))
    }

    /// Free of `endIdx` and of data.
    fn clean(&self) -> bool {
        !self.tagged(|t| matches!(t, Tag::V | Tag::Top))
    }
}

impl Val {
    /// The coefficient of `endIdx`.
    pub(crate) fn v_coef(&self) -> i64 {
        self.terms
            .iter()
            .filter(|(a, _)| a.tag == Tag::V)
            .map(|(_, c)| *c)
            .sum()
    }

    pub(crate) fn has_top(&self) -> bool {
        self.tagged(|t| t == Tag::Top)
    }

    pub(crate) fn has_start(&self) -> bool {
        self.tagged(|t| t == Tag::S)
    }

    /// Exactly one version of `startIdx`: a copy of it.
    fn is_start(&self) -> bool {
        let mut terms = self.terms.iter();
        matches!((terms.next(), terms.next()), (Some((a, 1)), None) if a.tag == Tag::S)
            && self.off.as_const() == Some(0)
    }

    #[cfg(test)]
    pub(crate) fn is_begin(&self) -> bool {
        self.beg
    }

    pub(crate) fn subtracts_begin(&self) -> bool {
        self.usub
    }
}

type Env = BTreeMap<String, Val>;

/// An unmodelled construct on the path to the steady loop.
pub(crate) struct Unsupported;

/// Per-iteration net change of each scalar a loop body writes. `None` marks a
/// write that is not an unconditional constant step.
type Deltas = BTreeMap<String, Option<i64>>;

/// Floors a lower bound substitutes through before it gives up.
const FLOOR_DEPTH: u32 = 4;

/// What a join may keep of an `if`'s condition.
struct Cond {
    /// The condition over parameter atoms, when it is parameter-pure.
    pure: Option<Expr>,
    /// Every value it compares is free of `endIdx` and of data.
    clean: bool,
}

struct Tracker<'a> {
    ctx: Ctx<'a>,
    next: u32,
    /// Proven lower bounds of minted unknowns, by id.
    floors: BTreeMap<u32, Vec<Val>>,
    /// Each startIdx clamp's result, by id, with the value it clamped.
    clamped: BTreeMap<u32, Val>,
    /// The composed walk's rules: status codes and receiver cells.
    composed: bool,
    /// Per enclosing then-arm, the `(hi, lo)` pairs its condition orders.
    guards: Vec<Vec<(Val, Val)>>,
}

/// The env slot of `*p`.
fn cell(p: &str) -> String {
    format!("*{p}")
}

/// `e` with each env-held name replaced by its parameter-pure value; any other
/// name (a parameter, a constant) is kept. `None` when a held value is not
/// parameter-pure or `e` reads memory.
fn subst_env(e: &Expr, env: &Env) -> Option<Expr> {
    let sub = |x: &Expr| subst_env(x, env).map(Box::new);
    Some(match e {
        Expr::Var(v) => match env.get(v) {
            Some(x) if x.is_param() => x.off.to_expr(),
            Some(_) => return None,
            None => e.clone(),
        },
        Expr::IntLiteral(_) | Expr::Literal(_) => e.clone(),
        Expr::BinOp(l, op, r) => Expr::BinOp(sub(l)?, op.clone(), sub(r)?),
        Expr::Neg(x) => Expr::Neg(sub(x)?),
        Expr::Not(x) => Expr::Not(sub(x)?),
        Expr::Cast(t, x) => Expr::Cast(t.clone(), sub(x)?),
        Expr::Ternary(c, a, b) => Expr::Ternary(sub(c)?, sub(a)?, sub(b)?),
        Expr::FuncCall(n, args) => Expr::FuncCall(
            n.clone(),
            args.iter().map(|a| subst_env(a, env)).collect::<Option<_>>()?,
        ),
        _ => return None,
    })
}

impl<'a> Tracker<'a> {
    fn new(ctx: Ctx<'a>, composed: bool) -> Self {
        Tracker {
            ctx,
            next: 0,
            floors: BTreeMap::new(),
            clamped: BTreeMap::new(),
            composed,
            guards: Vec::new(),
        }
    }

    fn mint(&mut self, tag: Tag) -> (u32, Val) {
        self.next += 1;
        (self.next, Val::unknown(Atom { id: self.next, tag }))
    }

    fn fresh(&mut self) -> Val {
        self.mint(Tag::Top).1
    }

    /// A fresh unknown keeping the flag of the values it was computed from.
    fn opaque(&mut self, from: &[&Val]) -> Val {
        let mut v = self.fresh();
        v.usub = from.iter().any(|x| x.usub);
        v
    }

    fn at_least(&self, v: &Val, k: i64) -> bool {
        self.at_least_within(v, k, FLOOR_DEPTH)
    }

    /// Parameter-pure: over the box. One unknown with coefficient 1: through
    /// each of its floors.
    fn at_least_within(&self, v: &Val, k: i64, depth: u32) -> bool {
        if v.is_param() {
            return self.ctx.at_least(&v.off, k);
        }
        let mut terms = v.terms.iter();
        let (Some((a, &1)), None) = (terms.next(), terms.next()) else {
            return false;
        };
        if depth == 0 {
            return false;
        }
        let rest = Val::param(v.off.clone());
        self.floors.get(&a.id).is_some_and(|fl| {
            fl.iter()
                .any(|f| f.plus(&rest, 1).is_some_and(|s| self.at_least_within(&s, k, depth - 1)))
        })
    }

    fn read(&mut self, v: &str, env: &Env) -> Val {
        if let Some(x) = env.get(v) {
            return x.clone();
        }
        if self.ctx.params.contains(v) {
            return Val::param(Lin::atom(Expr::Var(v.to_string())));
        }
        if self.composed && matches!(v, "TA_SUCCESS" | "SUCCESS") {
            return Val::konst(0);
        }
        self.fresh()
    }

    /// A parameter-pure value converted to `size_t` keeps the conversion as an
    /// atom, so a negative operand fails evaluation instead of wrapping. A
    /// relative value only ever meets `+` and `-` before the capture's `int`,
    /// which the wrap commutes with.
    fn unsigned(v: Val) -> Val {
        if !v.is_param() {
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
        Val {
            off: Lin::atom(Expr::Cast(VarType::Index, Box::new(v.off.to_expr()))),
            ..v
        }
    }

    /// A value built from a real parameter, converted to `int`: the truncated
    /// number is a different atom from the real itself.
    fn truncated(&self, v: Val) -> Val {
        if !v.is_param() || !self.ctx.mentions_real(&v.off) {
            return v;
        }
        let already = v.off.k == 0
            && v.off.terms.len() == 1
            && v.off
                .terms
                .values()
                .all(|(e, c)| *c == 1 && matches!(e, Expr::Cast(VarType::Integer, _)));
        if already {
            return v;
        }
        Val {
            off: Lin::atom(Expr::Cast(VarType::Integer, Box::new(v.off.to_expr()))),
            ..v
        }
    }

    /// The value a store into `name` leaves, after C's implicit conversion to
    /// its declared type. Only `int`, `size_t`, an enum parameter and, in the
    /// composed walk, a status code are tracked.
    fn store(&mut self, name: &str, v: Val, env: &mut Env) {
        let enum_param = || {
            self.ctx
                .func
                .optional_inputs
                .iter()
                .any(|p| p.name == name && matches!(p.param_type, ParamType::Enum(_)))
        };
        let v = match self.ctx.types.get(name) {
            Some(Some(VarType::Integer)) => self.truncated(v),
            Some(Some(VarType::Index)) => Self::unsigned(v),
            Some(Some(VarType::RetCodeType)) if self.composed => v,
            None if enum_param() => v,
            _ => self.opaque(&[&v]),
        };
        env.insert(name.to_string(), v);
    }

    fn pure_expr(&mut self, e: &Expr, env: &Env) -> Option<Expr> {
        let v = self.aval(e, env);
        v.is_param().then(|| v.off.to_expr())
    }

    /// Computed in `double`: no integer identity holds of it.
    fn real_valued(&self, e: &Expr) -> bool {
        match e {
            Expr::Literal(_) | Expr::Cast(VarType::Real, _) => true,
            Expr::Var(v) => self.ctx.reals.contains(v),
            Expr::BinOp(l, BinOp::Add | BinOp::Sub | BinOp::Mul | BinOp::Div | BinOp::Mod, r) => {
                self.real_valued(l) || self.real_valued(r)
            }
            Expr::Neg(x) => self.real_valued(x),
            Expr::Ternary(_, a, b) => self.real_valued(a) || self.real_valued(b),
            Expr::FuncCall(name, args) if matches!(name.as_str(), "max" | "min") => {
                args.iter().any(|a| self.real_valued(a))
            }
            _ => false,
        }
    }

    /// The value of `e` in `env`, reads taken before any of its side effects.
    #[allow(clippy::too_many_lines)]
    fn aval(&mut self, e: &Expr, env: &Env) -> Val {
        if !matches!(e, Expr::Var(_)) && self.real_valued(e) {
            let mut v = self.fresh();
            let mut seen = Vec::new();
            crate::streaming::walk_expr(e, &mut |x| {
                if let Expr::Var(n) = x {
                    seen.push(n.clone());
                }
            });
            v.usub = seen.iter().any(|n| env.get(n).is_some_and(|w| w.usub));
            return v;
        }
        match e {
            Expr::IntLiteral(k) => Val::konst(*k),
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
                    let a = self.aval(arg, env);
                    if !a.is_param() {
                        return self.opaque(&[&a]);
                    }
                    let mut v = Val::param(Lin::atom(Expr::Cast(
                        VarType::Integer,
                        Box::new(Expr::FuncCall(
                            "sqrt".into(),
                            vec![Expr::Cast(VarType::Real, Box::new(a.off.to_expr()))],
                        )),
                    )));
                    v.usub = a.usub;
                    return v;
                }
                // `+`, `-` and constant `*` commute with the wrap to the
                // capture's `int`, and an in-range value survives the cast.
                let v = self.aval(x, env);
                self.truncated(v)
            }
            Expr::Cast(VarType::Index, x) => {
                let v = self.aval(x, env);
                Self::unsigned(v)
            }
            Expr::BinOp(lhs, op @ (BinOp::Add | BinOp::Sub), rhs) => {
                let va = self.aval(lhs, env);
                let vb = self.aval(rhs, env);
                let sign = if *op == BinOp::Add { 1 } else { -1 };
                let unguarded = sign == -1 && vb.beg && !self.guarded(&va, &vb);
                let mut out = va.plus(&vb, sign).unwrap_or_else(|| self.opaque(&[&va, &vb]));
                out.usub |= unguarded;
                out
            }
            Expr::BinOp(lhs, BinOp::Mul, rhs) => {
                let va = self.aval(lhs, env);
                let vb = self.aval(rhs, env);
                let out = match (va.as_const(), vb.as_const()) {
                    (Some(c), _) => vb.scaled(c),
                    (_, Some(c)) => va.scaled(c),
                    _ if va.is_param() && vb.is_param() => {
                        let mut v = Val::param(Lin::atom(Expr::BinOp(
                            Box::new(va.off.to_expr()),
                            BinOp::Mul,
                            Box::new(vb.off.to_expr()),
                        )));
                        v.usub = va.usub || vb.usub;
                        Some(v)
                    }
                    _ => None,
                };
                out.unwrap_or_else(|| self.opaque(&[&va, &vb]))
            }
            Expr::Neg(x) => {
                let a = self.aval(x, env);
                a.scaled(-1).unwrap_or_else(|| self.opaque(&[&a]))
            }
            Expr::FuncCall(name, args) if is_unstable_call(name) => {
                match args.iter().map(|a| subst_env(a, env)).collect::<Option<Vec<_>>>() {
                    Some(a) => Val::param(Lin::atom(Expr::FuncCall(name.clone(), a))),
                    None => self.fresh(),
                }
            }
            Expr::FuncCall(name, args) => {
                // A real literal argument is its own constant.
                let vals: Vec<Val> = args
                    .iter()
                    .map(|a| match a {
                        Expr::Literal(_) => Val::param(Lin::atom(a.clone())),
                        _ => self.aval(a, env),
                    })
                    .collect();
                let refs: Vec<&Val> = vals.iter().collect();
                // Only integer-valued calls may become exact atoms.
                let int_valued = name.ends_with("_lookback")
                    || (matches!(name.as_str(), "max" | "min") && vals.len() == 2);
                if !int_valued || !vals.iter().all(Val::is_param) {
                    return self.opaque(&refs);
                }
                let pure: Vec<Expr> = vals.iter().map(|v| v.off.to_expr()).collect();
                let mut out = match self.ctx.inline_lookback(name, &pure, 1) {
                    Some(body) => Some(self.aval(&body, &Env::new())).filter(Val::is_param),
                    None => None,
                }
                .unwrap_or_else(|| Val::param(Lin::atom(Expr::FuncCall(name.clone(), pure))));
                out.usub = refs.iter().any(|v| v.usub);
                out
            }
            Expr::BinOp(lhs, op, rhs) => {
                let va = self.aval(lhs, env);
                let vb = self.aval(rhs, env);
                if !(va.is_param() && vb.is_param()) {
                    return self.opaque(&[&va, &vb]);
                }
                let mut v = Val::param(Lin::atom(Expr::BinOp(
                    Box::new(va.off.to_expr()),
                    op.clone(),
                    Box::new(vb.off.to_expr()),
                )));
                v.usub = va.usub || vb.usub;
                v
            }
            Expr::Not(x) => {
                let a = self.aval(x, env);
                if !a.is_param() {
                    return self.opaque(&[&a]);
                }
                let mut v = Val::param(Lin::atom(Expr::Not(Box::new(a.off.to_expr()))));
                v.usub = a.usub;
                v
            }
            Expr::Ternary(sel, yes, no) => {
                let vc = self.aval(sel, env);
                let va = self.aval(yes, env);
                let vb = self.aval(no, env);
                if !(vc.is_param() && va.is_param() && vb.is_param()) {
                    return self.opaque(&[&vc, &va, &vb]);
                }
                let mut v = Val::param(Lin::atom(Expr::Ternary(
                    Box::new(vc.off.to_expr()),
                    Box::new(va.off.to_expr()),
                    Box::new(vb.off.to_expr()),
                )));
                v.usub = vc.usub || va.usub || vb.usub;
                v
            }
            Expr::PointerDeref(p) if self.composed => match env.get(&cell(p)) {
                Some(v) => v.clone(),
                None => self.fresh(),
            },
            _ => self.fresh(),
        }
    }

    fn shift(&mut self, v: &Val, by: &Lin) -> Val {
        v.plus(&Val::param(by.clone()), 1)
            .unwrap_or_else(|| self.opaque(&[v]))
    }

    /// The `(hi, lo)` pairs a then-arm of `cond` may assume `hi >= lo` of.
    fn guard_pairs(&mut self, cond: &Expr, env: &Env) -> Vec<(Val, Val)> {
        let mut cs = Vec::new();
        conjuncts(cond, &mut cs);
        let mut out = Vec::new();
        for c in cs {
            let Expr::BinOp(l, op, r) = &c else { continue };
            let (hi, lo) = match op {
                BinOp::Greater | BinOp::GreaterEq => (l, r),
                BinOp::Less | BinOp::LessEq => (r, l),
                _ => continue,
            };
            let pair = (self.aval(hi, env), self.aval(lo, env));
            out.push(pair);
        }
        out
    }

    fn guarded(&self, hi: &Val, lo: &Val) -> bool {
        self.guards
            .iter()
            .flatten()
            .any(|(h, l)| h.same(hi) && l.same(lo))
    }

    /// Every value `cond` compares is free of `endIdx` and of data.
    fn cond_clean(&mut self, cond: &Expr, env: &Env) -> bool {
        match cond {
            Expr::BinOp(
                l,
                BinOp::And
                | BinOp::Or
                | BinOp::Less
                | BinOp::LessEq
                | BinOp::Greater
                | BinOp::GreaterEq
                | BinOp::Eq
                | BinOp::NotEq,
                r,
            ) => self.cond_clean(l, env) && self.cond_clean(r, env),
            Expr::Not(x) => self.cond_clean(x, env),
            _ => self.aval(cond, env).clean(),
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
        if self.composed {
            // A pointer handed to a call may be written through.
            let mut cells = Vec::new();
            crate::streaming::walk_expr(e, &mut |x| {
                if let Expr::FuncCall(_, args) = x {
                    for a in args {
                        if let Expr::Var(p) = a {
                            cells.push(cell(p));
                        }
                    }
                }
            });
            for c in cells {
                if env.contains_key(&c) {
                    let f = self.fresh();
                    env.insert(c, f);
                }
            }
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

    fn exec_guarded(
        &mut self,
        guard: Vec<(Val, Val)>,
        stmts: &[Statement],
        env: &Env,
    ) -> Result<Option<Env>, Unsupported> {
        self.guards.push(guard);
        let out = self.exec_scoped(stmts, env);
        self.guards.pop();
        out
    }

    /// A name's value at the end of one arm: the parameter itself when the
    /// arm never wrote it.
    fn arm_value(&mut self, n: &str, env: &Env) -> Val {
        match env.get(n) {
            Some(v) => v.clone(),
            None if self.ctx.params.contains(n) => Val::param(Lin::atom(Expr::Var(n.to_string()))),
            None => self.fresh(),
        }
    }

    fn join(&mut self, cond: &Cond, then_env: Option<Env>, else_env: Option<Env>) -> Option<Env> {
        let (then_env, else_env) = match (then_env, else_env) {
            (None, live) | (live, None) => return live,
            (Some(t), Some(e)) => (t, e),
        };
        let names: BTreeSet<String> = then_env.keys().chain(else_env.keys()).cloned().collect();
        let mut out = Env::new();
        for n in names {
            let x = self.arm_value(&n, &then_env);
            let y = self.arm_value(&n, &else_env);
            let mut v = if x.same(&y) {
                x.clone()
            } else if let (Some(c), true) = (&cond.pure, x.is_param() && y.is_param()) {
                Val::param(Lin::atom(Expr::Ternary(
                    Box::new(c.clone()),
                    Box::new(x.off.to_expr()),
                    Box::new(y.off.to_expr()),
                )))
            } else if cond.clean && x.clean() && y.clean() {
                self.mint(Tag::I).1
            } else {
                self.fresh()
            };
            v.beg = x.beg || y.beg;
            v.usub = x.usub || y.usub;
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
                match target {
                    Expr::Var(name) => {
                        let mut touched = Vec::new();
                        let mut clobbered = BTreeSet::new();
                        effects(value, false, &mut touched, &mut clobbered);
                        let v = if touched.iter().any(|(t, _)| t == name) || clobbered.contains(name) {
                            self.opaque(&[&v])
                        } else {
                            v
                        };
                        self.store(name, v, &mut env);
                    }
                    Expr::PointerDeref(p) if self.composed => {
                        env.insert(cell(p), v);
                    }
                    _ => self.apply_effects(target, &mut env),
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
                let pure = self.pure_expr(condition, &env);
                let clean = pure.is_some() || self.cond_clean(condition, &env);
                let verdict = pure.as_ref().and_then(|c| self.ctx.decide(c));
                let guard = if self.composed {
                    self.guard_pairs(condition, &env)
                } else {
                    Vec::new()
                };
                self.apply_effects(condition, &mut env);
                let cond = Cond { pure, clean };
                return match verdict {
                    Some(true) => self.exec_guarded(guard, then_body, &env),
                    Some(false) => self.exec_scoped(else_body, &env),
                    None => {
                        let t = self.exec_guarded(guard, then_body, &env)?;
                        let e = self.exec_scoped(else_body, &env)?;
                        Ok(self.join(&cond, t, e))
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
        if !bound.is_param() {
            return None;
        }
        let signed = self.ctx.is_int(x) && expr_is_signed(r, &self.ctx);
        let x0 = self.read(x, env);
        let x0_nonneg = self.at_least(&x0, 0);
        if !(signed || x0_nonneg && self.ctx.at_least(&bound.off, 0)) {
            return None;
        }
        let tag = if !x0.clean() {
            Tag::Top
        } else if x == "startIdx" || x0.is_start() {
            Tag::S
        } else {
            Tag::I
        };
        let (id, mut fresh) = self.mint(tag);
        if tag == Tag::S {
            self.clamped.insert(id, x0.clone());
        }
        fresh.usub = x0.usub || bound.usub;
        let mut floors = vec![Val::param(bound.off)];
        if x0_nonneg {
            floors.push(Val::konst(0));
        }
        self.floors.insert(id, floors);
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

        // Unknown trip count t: every value stepping by d moves by the one
        // unknown d * t.
        let mut moved: BTreeMap<i64, Val> = BTreeMap::new();
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
                    None => self.opaque(&[&cur]),
                },
                (Some(d), None) => {
                    let by = moved.entry(d).or_insert_with(|| self.fresh()).clone();
                    cur.plus(&by, 1).unwrap_or_else(|| self.opaque(&[&cur]))
                }
                (None, _) => self.opaque(&[&cur]),
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
                        return i0.is_param().then_some(i0.off).and_then(|lin| max0(self, lin));
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
        let signed = self.ctx.is_int(i) && expr_is_signed(bound, &self.ctx);
        if !(signed || self.at_least(&i0, 0) && self.at_least(&b, 0)) {
            return None;
        }
        let (hi, lo) = if step == 1 { (&b, &i0) } else { (&i0, &b) };
        let span = hi.plus(lo, -1)?.plus(&Val::konst(extra), 1)?;
        if !span.is_param() {
            return None;
        }
        self.ctx.at_least(&span.off, 0).then_some(span.off)
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
            Statement::Assign {
                target: Expr::PointerDeref(p),
                ..
            } => {
                out.insert(cell(p));
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
/// `subs` adds the lookback of every function the body calls.
fn resolve_callees(
    func: &FuncDef,
    lookup: &dyn CalleeLookup,
    subs: bool,
) -> BTreeMap<String, CalleeLookback> {
    fn called(stmts: &[Statement], subs: bool, out: &mut Vec<String>) {
        for s in stmts {
            crate::streaming::walk_stmt_exprs(s, &mut |e| {
                crate::streaming::walk_expr(e, &mut |x| {
                    if let Expr::FuncCall(name, _) = x {
                        match name.strip_suffix("_lookback") {
                            Some(c) => out.push(c.to_ascii_lowercase()),
                            None if subs => out.push(name.to_ascii_lowercase()),
                            None => {}
                        }
                    }
                });
            });
        }
    }
    let own = func.name.to_ascii_lowercase();
    let mut todo = Vec::new();
    called(func.stream_source(), subs, &mut todo);
    if let Some(LookbackExpr::Code(body)) = &func.lookback {
        called(body, false, &mut todo);
    }
    let mut out = BTreeMap::new();
    let mut seen = BTreeSet::new();
    while let Some(name) = todo.pop() {
        if name == own || !seen.insert(name.clone()) {
            continue;
        }
        if let Some(def) = lookup.lookback(&name) {
            called(&def.body, false, &mut todo);
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
    let callees = resolve_callees(func, input.lookup, false);
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
    let mut t = Tracker::new(Ctx::new(func, &input.facts, callees), false);
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
    if w.iter().any(|n| t.ctx.params.contains(n)) {
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

    let mut env = Env::new();
    let (start_id, start) = t.mint(Tag::S);
    t.floors.insert(start_id, vec![Val::konst(0)]);
    env.insert("startIdx".into(), start);
    env.insert("endIdx".into(), t.mint(Tag::V).1);
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
            let d = cursor.plus(&var, -1)?;
            if !d.is_param() {
                return None;
            }
            let lb = t
                .ctx
                .lower_bound(&d.off, false)
                .max(t.ctx.lower_bound(&d.off, true))?;
            i64::try_from(lb).ok()
        })
        .collect()
}

// ---------------------------------------------------------------------------
// Composed walk
// ---------------------------------------------------------------------------

/// What a modelled sub-call leaves: its lookback, begin index and element
/// count, exact under an Open whose sub-open succeeded.
pub(crate) struct SubFacts {
    #[cfg_attr(not(test), allow(dead_code))]
    pub(crate) lookback: Val,
    #[cfg_attr(not(test), allow(dead_code))]
    pub(crate) begin: Val,
    pub(crate) count: Val,
}

/// A sub-call statement goes through the sub-call entry only, never also the
/// general one: both would apply its receiver writes.
pub(crate) struct ComposedWalk<'a> {
    t: Tracker<'a>,
    /// `None` once every path returned.
    env: Option<Env>,
    end: Val,
    /// Each unresolved sub-call begin, by atom id, with its start argument.
    starts: BTreeMap<u32, Val>,
}

impl<'a> ComposedWalk<'a> {
    pub(crate) fn new(func: &'a FuncDef, lookup: &dyn CalleeLookup) -> Self {
        let mut t = Tracker::new(Ctx::new(func, &[], resolve_callees(func, lookup, true)), true);
        let (start_id, start) = t.mint(Tag::S);
        t.floors.insert(start_id, vec![Val::konst(0)]);
        let end = t.mint(Tag::V).1;
        let env: Env = [("startIdx".to_string(), start), ("endIdx".to_string(), end.clone())]
            .into_iter()
            .collect();
        ComposedWalk {
            t,
            env: Some(env),
            end,
            starts: BTreeMap::new(),
        }
    }

    /// Runs `stmts` under the shared rules; a returning path ends the walk.
    pub(crate) fn exec(&mut self, stmts: &[Statement]) -> Result<(), Unsupported> {
        let Some(env) = self.env.clone() else { return Ok(()) };
        self.env = self.t.exec_seq(stmts, env)?;
        Ok(())
    }

    pub(crate) fn reachable(&self) -> bool {
        self.env.is_some()
    }

    pub(crate) fn value(&mut self, e: &Expr) -> Val {
        match &self.env {
            Some(env) => self.t.aval(e, env),
            None => self.t.fresh(),
        }
    }

    pub(crate) fn end_idx(&self) -> Val {
        self.end.clone()
    }

    /// An unknown that may depend on data or on `endIdx`.
    pub(crate) fn top(&mut self) -> Val {
        self.t.fresh()
    }

    /// `off` when `idx` is `cursor + off` with `off` free of the cursor.
    pub(crate) fn cursor_offset(&mut self, cursor: &str, idx: &Expr) -> Option<Val> {
        let env = self.env.as_mut()?;
        let saved = env.get(cursor).cloned();
        let (id, x) = self.t.mint(Tag::I);
        env.insert(cursor.to_string(), x);
        let mut v = self.t.aval(idx, env);
        match saved {
            Some(s) => env.insert(cursor.to_string(), s),
            None => env.remove(cursor),
        };
        let atom = Atom { id, tag: Tag::I };
        if v.terms.remove(&atom) != Some(1) {
            return None;
        }
        Some(v)
    }

    pub(crate) fn assign(&mut self, name: &str, v: Val) {
        if let Some(env) = &mut self.env {
            env.insert(name.to_string(), v);
        }
    }

    /// Every scalar `s` may write becomes unknown.
    pub(crate) fn forget_writes(&mut self, s: &Statement) {
        if let Some(env) = &mut self.env {
            for n in written_names(std::slice::from_ref(s)) {
                let f = self.t.fresh();
                env.insert(n, f);
            }
        }
    }

    pub(crate) fn at_least(&self, v: &Val, k: i64) -> bool {
        self.t.at_least(v, k)
    }

    /// Enters the then-arm of `cond`, whose ordering tests a begin-index
    /// subtraction inside may rely on.
    pub(crate) fn push_guard(&mut self, cond: &Expr) {
        let pairs = match &self.env {
            Some(env) => self.t.guard_pairs(cond, env),
            None => Vec::new(),
        };
        self.t.guards.push(pairs);
    }

    pub(crate) fn pop_guard(&mut self) {
        self.t.guards.pop();
    }

    /// Sub-call `k` at `stmt`: its receivers take the begin and count, its
    /// status variable 0, because a handle exists only when its sub-open
    /// succeeded over the same arguments.
    pub(crate) fn sub_call(&mut self, k: usize, stmt: &Statement, sub: &SubCallStep) -> SubFacts {
        let Some(mut env) = self.env.take() else {
            let mut top = || self.t.fresh();
            return SubFacts {
                lookback: top(),
                begin: top(),
                count: top(),
            };
        };
        let start = self.t.aval(&sub.s_arg, &env);
        let last = self.t.aval(&sub.e_arg, &env);
        let call = Expr::FuncCall(format!("{}_lookback", sub.callee), sub.opt_args.clone());
        let lookback = self.t.aval(&call, &env);
        let begin = self.begin_of(k, &start, &lookback);
        let count = last
            .plus(&begin, -1)
            .and_then(|span| span.plus(&Val::konst(1), 1))
            .unwrap_or_else(|| self.t.opaque(&[&last, &begin]));
        if let Statement::Assign { value, .. } = stmt {
            self.t.apply_effects(value, &mut env);
        }
        let recv = |arg: &Expr| match arg {
            Expr::AddressOf(inner) => match inner.as_ref() {
                Expr::Var(local) => Some(local.clone()),
                _ => None,
            },
            Expr::Var(ptr) if matches!(ptr.as_str(), "outBegIdx" | "outNBElement") => {
                Some(cell(ptr))
            }
            _ => None,
        };
        let names = crate::streaming::batch_call_out_args(stmt, sub)
            .map(|(meta, _)| (recv(meta[0]), recv(meta[1])));
        match names {
            Some((Some(beg_recv), Some(nb_recv))) if beg_recv != nb_recv => {
                let mut held = begin.clone();
                held.beg = true;
                env.insert(beg_recv, held);
                env.insert(nb_recv, count.clone());
            }
            Some((beg_recv, nb_recv)) => {
                for name in [beg_recv, nb_recv].into_iter().flatten() {
                    let unknown = self.t.fresh();
                    env.insert(name, unknown);
                }
            }
            None => {}
        }
        if let Statement::Assign {
            target: Expr::Var(status),
            ..
        } = stmt
        {
            env.insert(status.clone(), Val::konst(0));
        }
        self.env = Some(env);
        SubFacts {
            lookback,
            begin,
            count,
        }
    }

    /// The first rule that applies: the lookback when it is at least the
    /// start, the start when it is at least the lookback, else a fresh begin
    /// no smaller than either.
    fn begin_of(&mut self, k: usize, s: &Val, lb: &Val) -> Val {
        let usub = s.usub || lb.usub;
        let pick = |v: &Val| Val {
            beg: false,
            usub,
            ..v.clone()
        };
        if lb.plus(s, -1).is_some_and(|d| self.lookback_at_least(&d, lb)) || s.same(lb) {
            return pick(lb);
        }
        if s.plus(lb, -1).is_some_and(|d| self.t.at_least(&d, 0)) {
            return pick(s);
        }
        let (id, mut b) = self.t.mint(Tag::B(k));
        b.usub = usub;
        self.t.floors.insert(id, vec![pick(s), pick(lb)]);
        self.starts.insert(id, s.clone());
        b
    }

    /// `d >= 0`, granting the lookback atom `lb` of the call being resolved
    /// `lb >= 0`: a lookback answering -1 fails the sub-open, so Open returns
    /// before any handle exists.
    fn lookback_at_least(&self, d: &Val, lb: &Val) -> bool {
        if self.t.at_least(d, 0) {
            return true;
        }
        let single = lb.is_param() && lb.off.k == 0 && lb.off.terms.len() == 1;
        let Some((key, (call, 1))) = lb.off.terms.iter().next().filter(|_| single) else {
            return false;
        };
        if !matches!(call, Expr::FuncCall(n, _) if n.ends_with("_lookback")) {
            return false;
        }
        if d.off.terms.get(key).is_none_or(|(_, c)| *c <= 0) {
            return false;
        }
        let mut rest = d.clone();
        rest.off.terms.remove(key);
        self.t.at_least(&rest, 0)
    }

    /// `v` as if no startIdx clamp fired and every unresolved begin were its
    /// start argument: each such unknown replaced by the value it was computed
    /// from, until none is left. Each replaced unknown is at least its
    /// replacement, so the flag (every replaced coefficient was at most 0)
    /// makes `v` at most the result. `None` on overflow.
    ///
    /// Newest first, so each unknown is replaced once, at its net coefficient:
    /// one that a newer replacement cancels never counts against the flag.
    pub(crate) fn presumed(&self, v: &Val) -> Option<(Val, bool)> {
        let from = |id: u32| self.starts.get(&id).or_else(|| self.t.clamped.get(&id));
        let mut cur = v.clone();
        let mut nonpositive = true;
        while let Some((a, c)) =
            cur.terms.iter().rev().find(|(a, _)| from(a.id).is_some()).map(|(a, c)| (*a, *c))
        {
            nonpositive &= c <= 0;
            cur.terms.remove(&a);
            cur = cur.plus(&from(a.id)?.scaled(c)?, 1)?;
        }
        Some((cur, nonpositive))
    }
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
        // Full: P at P = 4, both indices moving through the warm-up.
        assert_eq!(got["trailingIdxFull"], Some(4));
        // Half: P/2 at P = 4.
        assert_eq!(got["trailingIdxHalf"], Some(2));
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

    // --- composed walk ------------------------------------------------------

    /// `body` as APO's batch body: two periods and an MA type over `inReal`.
    fn apo_body(body: &str) -> FuncDef {
        let src = format!(
            "int apo_lookback(int optInFastPeriod, int optInSlowPeriod, TA_MAType optInMAType)\n\
             {{\n   return 0;\n}}\n\n\
             TA_RetCode apo(int startIdx, int endIdx, const double inReal[],\n\
             int optInFastPeriod, int optInSlowPeriod, TA_MAType optInMAType,\n\
             int *outBegIdx, int *outNBElement, double outReal[])\n{{\n{body}\n}}\n"
        );
        let mut f = crate::parser::yaml::parse_yaml(&base().join("apo/apo.yaml"));
        let parsed = crate::parser::c_source::parse_c_source_str(&src);
        crate::parser::c_source::wire_parsed_source(&mut f, &parsed);
        f
    }

    fn var(n: &str) -> Expr {
        Expr::Var(n.into())
    }

    /// A call of a function `lookup` knows, as the composed analysis records it.
    fn sub_step(st: &Statement, lookup: &dyn CalleeLookup) -> Option<SubCallStep> {
        let Statement::Assign {
            value: Expr::FuncCall(name, args),
            ..
        } = st
        else {
            return None;
        };
        let sig = lookup.callee(name)?;
        let opt = 2 + sig.n_inputs;
        Some(SubCallStep {
            tail_idx: 0,
            callee: name.to_ascii_lowercase(),
            opt_args: args[opt..opt + sig.n_opts].to_vec(),
            s_arg: args[0].clone(),
            e_arg: args[1].clone(),
            srcs: Vec::new(),
            dsts: args[opt + sig.n_opts + 2..].iter().map(|a| format!("{a:?}")).collect(),
        })
    }

    /// Walks `body` statement by statement, each call `lookup` knows being
    /// the next sub-call.
    fn walk<'a>(
        func: &'a FuncDef,
        lookup: &dyn CalleeLookup,
        body: &[Statement],
    ) -> (ComposedWalk<'a>, Vec<SubFacts>) {
        let mut w = ComposedWalk::new(func, lookup);
        let mut subs = Vec::new();
        for st in body {
            if let Some(sub) = sub_step(st, lookup) {
                let facts = w.sub_call(subs.len(), st, &sub);
                subs.push(facts);
            } else {
                assert!(w.exec(std::slice::from_ref(st)).is_ok(), "unmodelled {st:?}");
            }
        }
        (w, subs)
    }

    fn tags(v: &Val) -> Vec<(Tag, i64)> {
        v.terms.iter().map(|(a, c)| (a.tag, *c)).collect()
    }

    #[test]
    fn end_minus_start_cancels_across_clamp_versions() {
        let f = apo_body(
            "int n0, n1, lb;\n\
             lb = optInSlowPeriod;\n\
             n0 = endIdx - startIdx;\n\
             if( startIdx < lb ) startIdx = lb;\n\
             n1 = endIdx - startIdx;",
        );
        let (mut w, _) = walk(&f, &crate::streaming::FuncsLookup(&[]), &f.body);
        let (n0, n1) = (w.value(&var("n0")), w.value(&var("n1")));
        assert_eq!((n0.v_coef(), n1.v_coef()), (1, 1));
        // endIdx cancels; the two startIdx versions stay apart.
        assert_eq!(tags(&n0.plus(&n1, -1).unwrap()), vec![(Tag::S, -1), (Tag::S, 1)]);
        let s1 = w.value(&var("startIdx"));
        assert!(n1.plus(&s1, 1).unwrap().same(&w.end_idx()));
        let over = s1.plus(&w.value(&var("lb")), -1).unwrap();
        assert!(w.at_least(&over, 0));
    }

    #[test]
    fn gator_clamp_floors_the_medprice_start() {
        let gator = load("gator");
        let callees = [load("medprice"), load("rma")];
        let lookup = crate::streaming::FuncsLookup(&callees);
        let first = gator.body.iter().position(|s| sub_step(s, &lookup).is_some()).unwrap();
        let (mut w, subs) = walk(&gator, &lookup, &gator.body[..=first]);
        assert!(w.reachable());
        // No straight-line body: an atom, at least 0 by its own declarations.
        let lt = w.value(&var("lookbackTotal"));
        assert!(lt.is_param() && w.at_least(&lt, 0));
        let s1 = w.value(&var("startIdx"));
        assert!(s1.has_start() && w.at_least(&s1.plus(&lt, -1).unwrap(), 0));
        assert!(subs[0].begin.same(&w.value(&var("mpStartIdx"))));
        let nb = w.value(&var("nbElement"));
        let span = w.value(&var("localEnd")).plus(&nb, -1).unwrap();
        assert!(span.same(&lt.plus(&Val::konst(1), -1).unwrap()));
        let back = w.value(&var("backJaw"));
        assert!(back.is_param());
        assert!(matches!(back.off.terms.values().collect::<Vec<_>>()[..], [(Expr::Ternary(..), 1)]));
        assert!(w.value(&var("tempNbElement")).same(&subs[0].count));
        assert!(w.value(&var("tempBegIdx")).is_begin());
    }

    #[test]
    fn a_parameter_join_is_one_atom_however_the_condition_is_spelled() {
        let f = apo_body(
            "int a, b, fast, slow, tmp;\n\
             a = 0;\n\
             if( optInFastPeriod > optInSlowPeriod ) a = optInFastPeriod - optInSlowPeriod;\n\
             fast = optInFastPeriod;\n\
             slow = optInSlowPeriod;\n\
             b = 0;\n\
             if( fast > slow ) b = fast - slow;\n\
             if( optInSlowPeriod < optInFastPeriod )\n\
             {\n\
                tmp = optInSlowPeriod;\n\
                optInSlowPeriod = optInFastPeriod;\n\
                optInFastPeriod = tmp;\n\
             }",
        );
        let (mut w, _) = walk(&f, &crate::streaming::FuncsLookup(&[]), &f.body);
        let a = w.value(&var("a"));
        assert!(a.is_param() && a.off.terms.len() == 1 && a.same(&w.value(&var("b"))));
        for p in ["optInFastPeriod", "optInSlowPeriod"] {
            assert!(w.value(&var(p)).is_param(), "{p}");
        }
    }

    #[test]
    fn a_join_keeps_what_its_condition_is_free_of() {
        let f = apo_body(
            "int x, y, z;\n\
             if( inReal[startIdx] > 0.0 ) x = 1; else x = 2;\n\
             if( startIdx > optInFastPeriod ) y = 1; else y = 2;\n\
             if( optInSlowPeriod > optInFastPeriod ) z = 1; else z = 2;",
        );
        let (mut w, _) = walk(&f, &crate::streaming::FuncsLookup(&[]), &f.body);
        assert_eq!(tags(&w.value(&var("x"))), vec![(Tag::Top, 1)]);
        assert_eq!(tags(&w.value(&var("y"))), vec![(Tag::I, 1)]);
        assert!(w.value(&var("z")).is_param());
    }

    #[test]
    fn sub_call_begins_resolve_by_literal_zero_same_atom_and_floor() {
        let f = apo_body(
            "int lb, b0, n0, b1, n1, b2, n2, b3, n3;\n\
             TA_RetCode retCode;\n\
             double *t;\n\
             lb = ma_lookback( optInFastPeriod - 1, optInMAType );\n\
             retCode = ma( 0, endIdx, inReal, optInFastPeriod - 1, optInMAType, &b0, &n0, t );\n\
             retCode = ma( lb, endIdx, inReal, optInFastPeriod - 1, optInMAType, &b1, &n1, t );\n\
             if( startIdx < lb ) startIdx = lb;\n\
             retCode = ma( startIdx, endIdx, inReal, optInFastPeriod - 1, optInMAType, &b2, &n2, t );\n\
             retCode = ma( optInSlowPeriod, endIdx, inReal, optInFastPeriod, optInMAType, &b3, &n3, t );",
        );
        let callees = [load("ma")];
        let (mut w, subs) = walk(&f, &crate::streaming::FuncsLookup(&callees), &f.body);
        let lb = w.value(&var("lb"));
        // Only the axiom of the call being resolved orders the literal 0.
        assert!(lb.is_param() && !w.at_least(&lb, 0));
        assert!(subs[0].begin.same(&lb));
        assert!(subs[1].begin.same(&lb));
        assert!(subs[2].begin.same(&w.value(&var("startIdx"))));
        let b3 = &subs[3].begin;
        assert_eq!(tags(b3), vec![(Tag::B(3), 1)]);
        let slow = w.value(&var("optInSlowPeriod"));
        assert!(w.at_least(&b3.plus(&slow, -1).unwrap(), 0));
        assert!(w.at_least(&b3.plus(&subs[3].lookback, -1).unwrap(), 0));
        let (p, nonpositive) = w.presumed(b3).unwrap();
        assert!(p.same(&slow) && !nonpositive);
        let (p, nonpositive) = w.presumed(&b3.scaled(-1).unwrap()).unwrap();
        assert!(p.same(&slow.scaled(-1).unwrap()) && nonpositive);
        assert!(w.value(&var("n0")).same(&subs[0].count));
        assert!(w.value(&var("b0")).is_begin());
    }

    #[test]
    fn a_presumption_lets_no_startidx_clamp_fire_and_cancels_before_it_replaces() {
        let f = apo_body(
            "int first, b0, n0;\n\
             TA_RetCode retCode;\n\
             double *t;\n\
             first = startIdx;\n\
             if( first < optInSlowPeriod ) first = optInSlowPeriod;\n\
             if( startIdx < optInSlowPeriod ) startIdx = optInSlowPeriod;\n\
             retCode = ma( startIdx, endIdx, inReal, optInFastPeriod, optInMAType, &b0, &n0, t );",
        );
        let callees = [load("ma")];
        let (mut w, subs) = walk(&f, &crate::streaming::FuncsLookup(&callees), &f.body);
        let b = &subs[0].begin;
        assert_eq!(tags(b), vec![(Tag::B(0), 1)]);
        let s1 = w.value(&var("startIdx"));
        let first = w.value(&var("first"));
        assert_eq!(tags(&first), vec![(Tag::S, 1)]);
        assert!(!first.same(&s1));
        let (p, _) = w.presumed(&b.plus(&first, -1).unwrap()).unwrap();
        assert!(p.same(&Val::konst(0)));
        let (p, nonpositive) = w.presumed(&s1).unwrap();
        assert!(p.has_start() && !p.same(&s1) && !nonpositive);
        let ahead = s1.plus(b, -1).unwrap().plus(&Val::konst(1), -1).unwrap();
        let (p, nonpositive) = w.presumed(&ahead).unwrap();
        assert!(p.same(&Val::konst(-1)) && nonpositive, "the begin cancels the clamp first");
    }

    #[test]
    fn a_receiver_cell_holds_the_count_after_a_modelled_call() {
        let f = apo_body(
            "TA_RetCode retCode;\n\
             int before, after, saved;\n\
             before = (int)*outNBElement;\n\
             retCode = sma( startIdx, endIdx, inReal, optInFastPeriod, outBegIdx, outNBElement, outReal );\n\
             after = (int)*outNBElement;\n\
             if( retCode == TA_SUCCESS ) saved = 1; else saved = 0;",
        );
        let callees = [load("sma")];
        let (mut w, subs) = walk(&f, &crate::streaming::FuncsLookup(&callees), &f.body);
        assert!(w.value(&var("before")).has_top());
        let after = w.value(&var("after"));
        assert!(after.same(&subs[0].count) && after.v_coef() == 1);
        assert!(w.value(&Expr::PointerDeref("outBegIdx".into())).is_begin());
        assert_eq!(w.value(&var("saved")).as_const(), Some(1));
    }

    #[test]
    fn a_call_atom_over_a_derived_argument_has_no_interval() {
        let apo = load("apo");
        let callees = [load("ma")];
        let ctx = Ctx::new(&apo, &[], resolve_callees(&apo, &crate::streaming::FuncsLookup(&callees), true));
        let call = |a: Expr| Expr::FuncCall("ma_lookback".into(), vec![a, var("optInMAType")]);
        assert_eq!(ctx.interval(&call(var("optInFastPeriod")), 0), Some((0, INF)));
        assert_eq!(ctx.interval(&call(Expr::IntLiteral(3)), 0), Some((0, INF)));
        let derived = Expr::BinOp(Box::new(var("optInFastPeriod")), BinOp::Sub, Box::new(Expr::IntLiteral(1)));
        assert_eq!(ctx.interval(&call(derived), 0), None);
        assert_eq!(ctx.interval(&call(Expr::IntLiteral(0)), 0), None);
    }

    #[test]
    fn an_unguarded_begin_subtraction_is_flagged_and_bbands_guarded_one_is_not() {
        let f = apo_body(
            "TA_RetCode retCode;\n\
             int fastBeg, fastNb, x;\n\
             retCode = sma( startIdx, endIdx, inReal, optInFastPeriod, &fastBeg, &fastNb, outReal );\n\
             retCode = sma( startIdx, endIdx, inReal, optInSlowPeriod, outBegIdx, outNBElement, outReal );\n\
             x = (int)*outBegIdx - fastBeg;",
        );
        let callees = [load("sma")];
        let (mut w, _) = walk(&f, &crate::streaming::FuncsLookup(&callees), &f.body);
        assert!(w.value(&var("x")).subtracts_begin());

        let bbands = load("bbands");
        let callees = [load("ma"), load("stddev")];
        let lookup = crate::streaming::FuncsLookup(&callees);
        let at = bbands
            .body
            .iter()
            .rposition(|s| {
                matches!(s, Statement::If { .. })
                    && written_names(std::slice::from_ref(s)).contains("shiftIdx")
            })
            .unwrap();
        let (mut w, subs) = walk(&bbands, &lookup, &bbands.body[..=at]);
        assert_eq!(subs.len(), 2);
        let shift = w.value(&var("shiftIdx"));
        assert!(!shift.subtracts_begin());
        assert_eq!(tags(&shift), vec![(Tag::I, 1)]);
    }

    #[test]
    fn a_receiver_that_is_not_an_out_pointer_leaves_both_unknown() {
        let f = apo_body(
            "TA_RetCode retCode;\n\
             int b0;\n\
             int *p;\n\
             retCode = sma( startIdx, endIdx, inReal, optInFastPeriod, &b0, p, outReal );",
        );
        let callees = [load("sma")];
        let (mut w, _) = walk(&f, &crate::streaming::FuncsLookup(&callees), &f.body);
        let b0 = w.value(&var("b0"));
        assert!(!b0.is_begin() && b0.has_top());
    }

    #[test]
    fn a_truncated_real_parameter_is_not_the_real() {
        let bbands = load("bbands");
        let mut w = ComposedWalk::new(&bbands, &crate::streaming::FuncsLookup(&[]));
        let real = w.value(&var("optInNbDevUp"));
        let cast = w.value(&Expr::Cast(VarType::Integer, Box::new(var("optInNbDevUp"))));
        assert!(real.is_param() && cast.is_param() && !real.same(&cast));
        let store = Statement::Assign {
            target: var("shiftIdx"),
            value: var("optInNbDevUp"),
            compound: false,
        };
        assert!(w.exec(std::slice::from_ref(&store)).is_ok());
        assert!(!w.value(&var("shiftIdx")).same(&real));
        assert!(w.value(&var("shiftIdx")).same(&cast));
        let int_param = w.value(&Expr::Cast(VarType::Integer, Box::new(var("optInTimePeriod"))));
        assert!(int_param.same(&w.value(&var("optInTimePeriod"))));
    }

    #[test]
    fn a_real_valued_expression_keeps_the_unguarded_subtraction_flag() {
        let f = apo_body(
            "TA_RetCode retCode;\n\
             int fastBeg, fastNb, x, z;\n\
             retCode = sma( startIdx, endIdx, inReal, optInFastPeriod, &fastBeg, &fastNb, outReal );\n\
             retCode = sma( startIdx, endIdx, inReal, optInSlowPeriod, outBegIdx, outNBElement, outReal );\n\
             x = (int)*outBegIdx - fastBeg;\n\
             z = (int)(x * 1.5);",
        );
        let callees = [load("sma")];
        let (mut w, _) = walk(&f, &crate::streaming::FuncsLookup(&callees), &f.body);
        assert!(w.value(&var("z")).subtracts_begin());
    }

    #[test]
    fn an_unmodelled_statement_leaves_the_walk_where_it_was() {
        let f = apo_body("int a;\na = 3;");
        let mut w = ComposedWalk::new(&f, &crate::streaming::FuncsLookup(&[]));
        assert!(w.exec(&f.body).is_ok());
        assert!(w.exec(&[Statement::Break]).is_err());
        assert!(w.reachable());
        assert_eq!(w.value(&var("a")).as_const(), Some(3));
    }
}
