//! C respelled for LLVM: the same values, in the shape gcc gives the C
//! library (#438).
//!
//! Rust-only, run after the backend's `ir_cleanup` sequence, and
//! length-preserving like it: the stream emitters address a body by index.
//! No rewrite may change a computed value, only the shape LLVM sees.

use std::collections::HashSet;

use super::builtins::MathFn;
use super::fma::{self, FmaCtx};
use super::ir_cleanup::recurse;
use crate::helper_registry::HelperRegistry;
use crate::ir::{BinOp, Expr, Statement};
use crate::streaming::{expr_effect, exprs_equal, walk_expr};

// Both pseudo-calls return a double. Keep the `ta_` prefix: it is what makes
// the FMA fusion predicate type them as one, as it types what they replace.

/// Rendered as `f64::from_bits(p ^ q ^ x)` over the three operands' bits.
pub(crate) const SELECT_OTHER: &str = "ta_select_other";

/// `(rangeType, o_a, h_a, l_a, c_a, o_b, h_b, l_b, c_b)`, rendered as one
/// `match` on the range type whose arms are each range's difference.
pub(crate) const CANDLE_RANGE_DIFF: &str = "ta_candlerange_diff";

/// Rewrite every statement list in `body`, nested ones included.
///
/// `recurrent` is [`super::select_chain::recurrent_select_targets`] of the body
/// being rendered; an extreme on such a target stays a branch.
#[must_use]
pub(crate) fn rewrite(
    body: &[Statement],
    fma: &FmaCtx,
    helpers: &HelperRegistry,
    recurrent: &HashSet<String>,
) -> Vec<Statement> {
    let lists = rewrite_lists(body, fma, helpers, recurrent);
    crate::streaming::rewrite_stmts(&lists, &fuse_candle_range_diff, &|s| Some(s))
}

fn rewrite_lists(
    body: &[Statement],
    fma: &FmaCtx,
    helpers: &HelperRegistry,
    recurrent: &HashSet<String>,
) -> Vec<Statement> {
    let pass = |b: &[Statement]| rewrite_lists(b, fma, helpers, recurrent);
    // The inlined helpers only compute from their arguments.
    let pure = |e: &Expr| !expr_effect(e, &|n| MathFn::from_name(n).is_some() || helpers.get(n).is_some());
    let mut out: Vec<Statement> = body
        .iter()
        .map(|s| {
            let s = recurse(s, &pass);
            extreme(&s, fma, recurrent).or_else(|| merged_store(&s, &pure)).unwrap_or(s)
        })
        .collect();
    for i in 1..out.len() {
        if let Some(s) = other_of_pair(&out[i - 1], &out[i], fma, &pure) {
            out[i] = s;
        }
    }
    out
}

/// `ta_candlerange(rt, a..) - ta_candlerange(rt, b..)`, a running candle
/// total's update, as one dispatch. Two dispatches join into a pair LLVM's SLP
/// vectorizer packs into one vector register, which runs slower.
fn fuse_candle_range_diff(e: Expr) -> Expr {
    if let Expr::BinOp(l, BinOp::Sub, r) = &e {
        if let (Expr::FuncCall(fl, a), Expr::FuncCall(fr, b)) = (l.as_ref(), r.as_ref()) {
            if fl == "ta_candlerange" && fr == "ta_candlerange" && a.len() == 5 && b.len() == 5 && exprs_equal(&a[0], &b[0]) {
                return Expr::FuncCall(CANDLE_RANGE_DIFF.into(), a.iter().chain(&b[1..]).cloned().collect());
            }
        }
    }
    e
}

/// `if( v < t ) t = v;` is C's `t = min(v, t)`, and `if( v > t ) t = v;` its
/// `max`. As an `if`, LLVM makes it a MINSD only when the join is its own; as the
/// last statement of an enclosing arm it stays a branch.
fn extreme(s: &Statement, fma: &FmaCtx, recurrent: &HashSet<String>) -> Option<Statement> {
    let Statement::If { condition, then_body, else_body, cond_comments } = s else { return None };
    if !else_body.is_empty() || !cond_comments.is_empty() {
        return None;
    }
    let [Statement::Assign { target, value, compound: false }] = then_body.as_slice() else {
        return None;
    };
    let Expr::BinOp(l, op, r) = condition else { return None };
    let less = match op {
        BinOp::Less => true,
        BinOp::Greater => false,
        _ => return None,
    };
    if ![target, value].into_iter().all(is_place) || !compares_reals(target, value, fma) {
        return None;
    }
    if let Expr::Var(n) | Expr::ArrayAccess(n, _) = target {
        if recurrent.contains(fma::stream_base(n)) {
            return None;
        }
    }
    // `l op r` with the value on the left keeps the value when it compares
    // `op`; with the target on the left the verdict flips.
    let value_left = if exprs_equal(l, value) && exprs_equal(r, target) {
        true
    } else if exprs_equal(l, target) && exprs_equal(r, value) {
        false
    } else {
        return None;
    };
    let name = if less == value_left { "min" } else { "max" };
    Some(Statement::Assign {
        target: target.clone(),
        value: Expr::FuncCall(name.into(), vec![value.clone(), target.clone()]),
        compound: false,
    })
}

/// `if( c ) a[k] = x; else a[k] = y;` as `a[k] = c ? x : y;`. Each arm's store
/// carries its own bounds check, and two panics at two source lines keep LLVM
/// from merging the arms, so the branch on `c` stays.
///
/// Only operands LLVM computes for free: an arm that divides would then run on
/// every pass. And only a condition without `&&`, `||` or `?:`: as a value, a
/// short-circuit chain keeps its branches and costs more instructions, and a
/// helper under a `?:` arm would be hoisted out from under its guard.
fn merged_store(s: &Statement, pure: &dyn Fn(&Expr) -> bool) -> Option<Statement> {
    let Statement::If { condition, then_body, else_body, cond_comments } = s else { return None };
    if !cond_comments.is_empty() {
        return None;
    }
    let (
        [Statement::Assign { target: t1 @ Expr::ArrayAccess(..), value: a, compound: false }],
        [Statement::Assign { target: t2, value: b, compound: false }],
    ) = (then_body.as_slice(), else_body.as_slice())
    else {
        return None;
    };
    // `==`, not `exprs_equal`: the target is evaluated once either way, so an
    // index that increments is the same place in both arms.
    if t1 != t2 || !is_cheap(a) || !is_cheap(b) || !pure(condition) || short_circuits(condition) {
        return None;
    }
    Some(Statement::Assign {
        target: t1.clone(),
        value: Expr::Ternary(Box::new(condition.clone()), Box::new(a.clone()), Box::new(b.clone())),
        compound: false,
    })
}

/// `x = c ? p : q; y = c ? q : p;` as `x = c ? p : q; y = p ^ q ^ x` in bits.
/// x86 lowers only a compare's first select to a mask; a second select on the
/// same compare becomes a branch, whatever its spelling.
fn other_of_pair(
    first: &Statement,
    second: &Statement,
    fma: &FmaCtx,
    pure: &dyn Fn(&Expr) -> bool,
) -> Option<Statement> {
    let Statement::Assign { target: x @ Expr::Var(xn), value: Expr::Ternary(c1, p1, q1), compound: false } = first
    else {
        return None;
    };
    let Statement::Assign { target: y @ Expr::Var(yn), value: Expr::Ternary(c2, q2, p2), compound: false } = second
    else {
        return None;
    };
    if xn == yn || !exprs_equal(c1, c2) || !exprs_equal(p1, p2) || !exprs_equal(q1, q2) {
        return None;
    }
    // The second statement now reads both arms unconditionally, and after `x`
    // is written, so neither the arms nor the condition may read `x`.
    let plain = |e: &Expr| matches!(e, Expr::Var(_) | Expr::Literal(_));
    if !plain(p1) || !plain(q1) || !pure(c1) {
        return None;
    }
    if [c1.as_ref(), p1.as_ref(), q1.as_ref()].into_iter().any(|e| mentions(e, xn)) {
        return None;
    }
    let real = |e: &Expr| {
        matches!(e, Expr::Literal(_))
            || (fma::expr_is_float_typed(e, Some(fma)) && !fma::is_definitely_integer(e, fma))
    };
    if !real(p1) || !real(q1) {
        return None;
    }
    Some(Statement::Assign {
        target: y.clone(),
        value: Expr::FuncCall(SELECT_OTHER.into(), vec![p1.as_ref().clone(), q1.as_ref().clone(), x.clone()]),
        compound: false,
    })
}

/// A variable, or an element read at an index with no side effect.
fn is_place(e: &Expr) -> bool {
    match e {
        Expr::Var(_) => true,
        Expr::ArrayAccess(_, i) => !crate::streaming::expr_has_effect(i),
        _ => false,
    }
}

fn short_circuits(e: &Expr) -> bool {
    let mut found = false;
    walk_expr(e, &mut |x| found |= matches!(x, Expr::BinOp(_, BinOp::And | BinOp::Or, _) | Expr::Ternary(..)));
    found
}

fn is_cheap(e: &Expr) -> bool {
    match e {
        Expr::Literal(_) | Expr::IntLiteral(_) | Expr::Var(_) => true,
        Expr::Cast(_, inner) => is_cheap(inner),
        _ => false,
    }
}

/// Whether `l` against `r` is a floating-point compare. Neither side may be a
/// known integer, and at least one must be known real.
fn compares_reals(l: &Expr, r: &Expr, fma: &FmaCtx) -> bool {
    let int = |e: &Expr| matches!(e, Expr::IntLiteral(_)) || fma::is_definitely_integer(e, fma);
    !int(l) && !int(r) && (fma::expr_is_float_typed(l, Some(fma)) || fma::expr_is_float_typed(r, Some(fma)))
}

fn mentions(e: &Expr, name: &str) -> bool {
    let mut found = false;
    walk_expr(e, &mut |x| {
        found |= matches!(x, Expr::Var(n) | Expr::ArrayAccess(n, _) if n == name);
    });
    found
}

#[cfg(test)]
mod tests {
    use super::*;

    /// `Statement` has no `PartialEq`; its `Debug` form is structural.
    macro_rules! same {
        ($a:expr, $b:expr) => {
            assert_eq!(format!("{:?}", $a), format!("{:?}", $b))
        };
    }

    fn var(n: &str) -> Expr {
        Expr::Var(n.into())
    }
    fn bin(l: Expr, op: BinOp, r: Expr) -> Expr {
        Expr::BinOp(Box::new(l), op, Box::new(r))
    }
    fn assign(t: Expr, v: Expr) -> Statement {
        Statement::Assign { target: t, value: v, compound: false }
    }
    fn if_(c: Expr, then_body: Vec<Statement>, else_body: Vec<Statement>) -> Statement {
        Statement::If { condition: c, then_body, else_body, cond_comments: Vec::new() }
    }
    fn out_at(index: Expr) -> Expr {
        Expr::ArrayAccess("outInteger".into(), Box::new(index))
    }
    fn post_inc(n: &str) -> Expr {
        Expr::PostIncrement(Box::new(var(n)))
    }

    /// `v`, `t`, `w` and `c` are doubles; `k` and `n` integers.
    fn run_with(body: &[Statement], recurrent: &[&str]) -> Vec<Statement> {
        let set = |ns: &[&str]| ns.iter().map(|n| (*n).to_string()).collect::<HashSet<String>>();
        let (real, int, none) = (set(&["v", "t", "w", "c"]), set(&["k", "n"]), HashSet::new());
        let fma = FmaCtx {
            real_vars: &real,
            index_vars: &int,
            real_array_vars: &none,
            int_output_names: &none,
            sentinel_vars: &none,
        };
        let out = rewrite(body, &fma, &HelperRegistry::empty(), &set(recurrent));
        assert_eq!(out.len(), body.len(), "the pass must preserve length");
        out
    }
    fn run(body: &[Statement]) -> Vec<Statement> {
        run_with(body, &[])
    }
    fn call(f: &str, args: Vec<Expr>) -> Expr {
        Expr::FuncCall(f.into(), args)
    }

    #[test]
    fn a_strict_extreme_becomes_the_c_macro_in_all_four_spellings() {
        let cases = [
            (bin(var("v"), BinOp::Less, var("t")), "min"),
            (bin(var("t"), BinOp::Greater, var("v")), "min"),
            (bin(var("v"), BinOp::Greater, var("t")), "max"),
            (bin(var("t"), BinOp::Less, var("v")), "max"),
        ];
        for (cond, f) in cases {
            let got = run(&[if_(cond, vec![assign(var("t"), var("v"))], vec![])]);
            same!(got, vec![assign(var("t"), call(f, vec![var("v"), var("t")]))]);
        }
    }

    #[test]
    fn a_nested_extreme_is_rewritten_in_place() {
        let inner = if_(bin(var("v"), BinOp::Less, var("t")), vec![assign(var("t"), var("v"))], vec![]);
        let body = [Statement::While { condition: var("n"), body: vec![inner] }];
        let Statement::While { body, .. } = &run(&body)[0] else { panic!("shape") };
        same!(body, &vec![assign(var("t"), call("min", vec![var("v"), var("t")]))]);
    }

    #[test]
    fn near_miss_extremes_stay_branches() {
        let keep = [
            // Non-strict: `v <= t ? v : t` is not one MINSD on a tie or NaN.
            if_(bin(var("v"), BinOp::LessEq, var("t")), vec![assign(var("t"), var("v"))], vec![]),
            if_(bin(var("v"), BinOp::GreaterEq, var("t")), vec![assign(var("t"), var("v"))], vec![]),
            // An else arm.
            if_(bin(var("v"), BinOp::Less, var("t")), vec![assign(var("t"), var("v"))], vec![assign(var("w"), var("v"))]),
            // The stored value is not the compared one.
            if_(bin(var("v"), BinOp::Less, var("t")), vec![assign(var("t"), var("w"))], vec![]),
            // Integers already lower to a CMOV.
            if_(bin(var("k"), BinOp::Less, var("n")), vec![assign(var("n"), var("k"))], vec![]),
            // An index that calls: the rewrite would read the element once, not twice.
            if_(
                bin(Expr::ArrayAccess("in".into(), Box::new(call("f", vec![var("k")]))), BinOp::Less, var("t")),
                vec![assign(var("t"), Expr::ArrayAccess("in".into(), Box::new(call("f", vec![var("k")]))))],
                vec![],
            ),
        ];
        for s in keep {
            same!(run(std::slice::from_ref(&s)), vec![s]);
        }
    }

    #[test]
    fn an_extreme_on_an_arithmetic_recurrence_stays_a_branch() {
        let s = if_(bin(var("v"), BinOp::Less, var("t")), vec![assign(var("t"), var("v"))], vec![]);
        same!(run_with(std::slice::from_ref(&s), &["t"]), vec![s.clone()]);
        let streamed = if_(bin(var("v"), BinOp::Less, var("sp.t")), vec![assign(var("sp.t"), var("v"))], vec![]);
        same!(run_with(std::slice::from_ref(&streamed), &["t"]), vec![streamed]);
    }

    #[test]
    fn two_stores_to_one_element_become_one_store_of_a_ternary() {
        let c = bin(var("v"), BinOp::LessEq, var("t"));
        let s = if_(
            c.clone(),
            vec![assign(out_at(post_inc("k")), Expr::IntLiteral(100))],
            vec![assign(out_at(post_inc("k")), Expr::IntLiteral(0))],
        );
        let want = assign(
            out_at(post_inc("k")),
            Expr::Ternary(Box::new(c), Box::new(Expr::IntLiteral(100)), Box::new(Expr::IntLiteral(0))),
        );
        same!(run(&[s]), vec![want]);
    }

    #[test]
    fn a_condition_calling_a_math_builtin_still_merges() {
        let c = bin(call("fabs", vec![var("v")]), BinOp::LessEq, var("t"));
        let s = if_(
            c,
            vec![assign(out_at(var("k")), Expr::IntLiteral(100))],
            vec![assign(out_at(var("k")), Expr::IntLiteral(0))],
        );
        assert!(matches!(&run(&[s])[0], Statement::Assign { value: Expr::Ternary(..), .. }));
    }

    #[test]
    fn near_miss_store_pairs_keep_their_arms() {
        let arms = |c: Expr, a: Expr, b: Expr, ta: Expr, tb: Expr| {
            if_(c, vec![assign(ta, a)], vec![assign(tb, b)])
        };
        let c = bin(var("v"), BinOp::Less, var("t"));
        let (hundred, zero) = (Expr::IntLiteral(100), Expr::IntLiteral(0));
        let keep = [
            // Two elements.
            arms(c.clone(), hundred.clone(), zero.clone(), out_at(var("k")), out_at(var("n"))),
            // A scalar target gains nothing.
            arms(c.clone(), hundred.clone(), zero.clone(), var("w"), var("w")),
            // A divide would then run on every pass.
            arms(c.clone(), bin(var("v"), BinOp::Div, var("t")), zero.clone(), out_at(var("k")), out_at(var("k"))),
            // A condition with an effect.
            arms(bin(Expr::PostIncrement(Box::new(var("v"))), BinOp::Less, var("t")), hundred.clone(), zero.clone(), out_at(var("k")), out_at(var("k"))),
            // A short-circuit chain, or a guarded operand.
            arms(bin(c.clone(), BinOp::And, bin(var("w"), BinOp::Less, var("t"))), hundred.clone(), zero.clone(), out_at(var("k")), out_at(var("k"))),
            arms(bin(ternary(&c, var("w"), zero.clone()), BinOp::Less, var("t")), hundred.clone(), zero.clone(), out_at(var("k")), out_at(var("k"))),
            // An unknown call may have one.
            arms(bin(call("f", vec![]), BinOp::Less, var("t")), hundred, zero, out_at(var("k")), out_at(var("k"))),
        ];
        for s in keep {
            same!(run(std::slice::from_ref(&s)), vec![s]);
        }
    }

    fn ternary(c: &Expr, a: Expr, b: Expr) -> Expr {
        Expr::Ternary(Box::new(c.clone()), Box::new(a), Box::new(b))
    }

    #[test]
    fn a_swapped_pair_answers_its_second_select_from_the_first() {
        let c = bin(var("v"), BinOp::Less, Expr::Literal(0.0));
        let body = [
            assign(var("t"), ternary(&c, Expr::Literal(0.0), var("w"))),
            assign(var("c"), ternary(&c, var("w"), Expr::Literal(0.0))),
        ];
        let got = run(&body);
        same!(got[0], body[0]);
        same!(got[1], assign(var("c"), call(SELECT_OTHER, vec![Expr::Literal(0.0), var("w"), var("t")])));
    }

    #[test]
    fn near_miss_pairs_keep_their_second_select() {
        let c = bin(var("v"), BinOp::Less, Expr::Literal(0.0));
        let zero = Expr::Literal(0.0);
        let pair = |first: Expr, second: Expr| [assign(var("t"), first), assign(var("c"), second)];
        let keep = [
            // Not swapped.
            pair(ternary(&c, zero.clone(), var("w")), ternary(&c, zero.clone(), var("w"))),
            // Another condition.
            pair(ternary(&c, zero.clone(), var("w")), ternary(&bin(var("v"), BinOp::Greater, zero.clone()), var("w"), zero.clone())),
            // An arm reads the first target, which the second sees written.
            pair(ternary(&c, zero.clone(), var("t")), ternary(&c, var("t"), zero.clone())),
            // An arm that indexes would be read unconditionally.
            pair(
                ternary(&c, zero.clone(), Expr::ArrayAccess("in".into(), Box::new(var("k")))),
                ternary(&c, Expr::ArrayAccess("in".into(), Box::new(var("k"))), zero.clone()),
            ),
            // The condition reads the first target.
            pair(
                ternary(&bin(var("t"), BinOp::Less, zero.clone()), zero.clone(), var("w")),
                ternary(&bin(var("t"), BinOp::Less, zero.clone()), var("w"), zero.clone()),
            ),
            // A condition that may have an effect.
            pair(ternary(&call("f", vec![]), zero.clone(), var("w")), ternary(&call("f", vec![]), var("w"), zero.clone())),
            // Integers, or an integer arm beside a real literal.
            pair(ternary(&c, var("k"), var("n")), ternary(&c, var("n"), var("k"))),
            pair(ternary(&c, zero.clone(), var("k")), ternary(&c, var("k"), zero.clone())),
        ];
        for body in keep {
            same!(run(&body), body.to_vec());
        }
    }

    #[test]
    fn a_candle_total_update_takes_one_range_dispatch() {
        let range = |rt: &str, i: Expr| {
            let leg = |n: &str| Expr::ArrayAccess(n.into(), Box::new(i.clone()));
            call("ta_candlerange", vec![var(rt), leg("inOpen"), leg("inHigh"), leg("inLow"), leg("inClose")])
        };
        let update = |a: Expr, b: Expr| {
            vec![Statement::Assign { target: var("t"), value: bin(a, BinOp::Sub, b), compound: true }]
        };
        let got = run(&update(range("rt", var("k")), range("rt", var("n"))));
        let Statement::Assign { value: Expr::FuncCall(f, args), compound: true, .. } = &got[0] else { panic!("{got:?}") };
        assert_eq!((f.as_str(), args.len()), (CANDLE_RANGE_DIFF, 9));
        // The minuend's legs, then the subtrahend's: swapped, every total flips sign.
        let at = |i: usize| match &args[i] {
            Expr::ArrayAccess(_, idx) => format!("{idx:?}"),
            other => panic!("{other:?}"),
        };
        assert!((1..5).all(|i| at(i) == format!("{:?}", var("k"))) && (5..9).all(|i| at(i) == format!("{:?}", var("n"))));
        // Two range types are two dispatches.
        let two = update(range("rt", var("k")), range("rt2", var("n")));
        same!(run(&two), two);
    }
}
