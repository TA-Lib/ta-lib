//! Divisions by a loop-accumulated variable must be guarded on that same variable.
//!
//! Three defects in one week shared one shape (#381): the divisor was a variable
//! accumulated across steady-loop iterations by add/subtract, and the guard that
//! dominated the division tested *something else*.
//!
//!   - VORTEX divided by a running true-range sum gated on a flat-bar counter. FP
//!     absorption drives the sum to exactly 0.0 while the counter is still short.
//!   - VORTEX again, after the first fix: a reseed zeroed the two numerator sums on a
//!     predicate that only proves the denominator's terms are zero, producing a
//!     negative -VI — unreachable for a ratio of sums of fabs().
//!   - ER used its asymmetric KAMA-parity clamp (`sumROC1 <= periodROC`) as the zero
//!     guard. It cannot fire when `periodROC < 0`, so a zero denominator divided.
//!
//! No existing gate sees this class. All four backends are generated from one input
//! body, so `--xlang-hash` agrees bitwise on the same wrong answer; `--codegen` has no
//! frozen baseline for a post-cutover function; and the reference corpus is benign in
//! exactly the ways that matter (no bar with TR == 0, no 14-bar halt, no consecutive
//! close ratio outside Sterbenz range).
//!
//! Data cannot settle it either. `TA_IS_ZERO_SCALED` is scale-invariant, so no series
//! magnitude distinguishes it from a fixed band — `test_kdj.c`'s small-magnitude leg is
//! documented as unable to discriminate for that reason. The question is structural, so
//! the check is structural: it reads the parsed body, costs milliseconds, and runs on
//! the PR gate that already exists.
//!
//! What it does NOT claim: a division this sweep passes is not proven safe. A guard on
//! the right variable can still be the wrong guard (ER's clamp names `sumROC1` and was
//! still unable to fire on a negative `periodROC`). This finds divisors nothing tests,
//! which is the cheap half.

use std::collections::HashSet;
use std::path::Path;

use ta_codegen_lib::{
    ir::{BinOp, Expr, FuncDef, Statement, VarType},
    parser,
};

fn load() -> Vec<FuncDef> {
    let base = Path::new(env!("CARGO_MANIFEST_DIR")).join("../input");
    let mut funcs = Vec::new();
    for e in std::fs::read_dir(&base).expect("read input dir").filter_map(Result::ok) {
        let dir = e.path();
        if !dir.is_dir() {
            continue;
        }
        let n = e.file_name().to_string_lossy().to_string();
        let (yaml, csrc) = (dir.join(format!("{n}.yaml")), dir.join(format!("{n}.c")));
        if !yaml.exists() || !csrc.exists() {
            continue;
        }
        let mut f = parser::yaml::parse_yaml(&yaml);
        let parsed = parser::c_source::parse_c_source(&csrc);
        parser::c_source::wire_parsed_source(&mut f, &parsed);
        funcs.push(f);
    }
    assert!(funcs.len() >= 200, "expected the whole input tree, got {}", funcs.len());
    funcs
}

/// Divisions the sweep reports that have been READ, each with what was concluded.
///
/// Deliberately not named "known-safe": one entry below is an open question rather
/// than a clearance. Annotating beats suppressing either way, because the entry states
/// a reason that a later edit can contradict -- a bare allowlist states nothing and
/// silently keeps holding once its reason expires.
///
/// Keyed `(FUNC, divisor variable, why, required_flag)`. A non-empty `required_flag`
/// means the reason rests on that YAML flag still being declared, which
/// `annotation_reasons_still_hold` asserts rather than trusts — an entry whose stated
/// reason has expired is exactly the silent allowlist this table exists to avoid.
const ANNOTATED: &[(&str, &str, &str, &str)] = &[
    // The Hilbert-transform trio share one body shape. `period` is clamped to [6, 50]
    // (`ht_dcphase.c:359-361` and its twins) before `smoothPeriod` is formed as
    // `0.33*period + 0.67*smoothPeriod` -- a convex combination with positive
    // coefficients, so it is bounded below by 0.33*6 once `period` is clamped and can
    // never reach zero. Reasoned from the clamp rather than proven by the sweep: it is
    // exactly the "provably non-zero from the lookback clamp" case, recorded so that
    // removing the clamp has something to contradict.
    ("HT_DCPHASE", "smoothPeriod", "period clamped to [6,50] before the combination", ""),
    ("HT_SINE", "smoothPeriod", "period clamped to [6,50] before the combination", ""),
    ("HT_TRENDMODE", "smoothPeriod", "period clamped to [6,50] before the combination", ""),

    // OPEN, not cleared. KAMA divides by the same running `sumROC1` that ER does, under
    // the same asymmetric clamp, and WITHOUT the exact denominator test ER gained in
    // #350 -- `er.c:56` records the difference explicitly ("the one thing kama.c has no
    // equivalent of"), and the #378 review asked for a fix that closes ER "without
    // touching KAMA parity". So the gap is deliberate and documented, and whether the
    // two should converge is a decision about KAMA's output, not a defect to patch
    // under a sweep. Listed so the sweep is green and the question stays visible.
    ("KAMA", "sumROC1", "OPEN: same shape as the ER defect fixed in #350; see #381", ""),

    // Unguarded BY DECISION, not by oversight. `vwma.c:124` divides by the rolling
    // volume sum with no test, and `vwma.yaml` declares `nan_inf_output` to say so --
    // the standing answer for this shape (set as the precedent for #56 RVOL; not 1.0,
    // not 0.0, not carry-forward, which would force TA_FUNC_FLG_START_DEP onto an
    // otherwise windowed function).
    //
    // What clears it is statelessness, not the flag alone: #112's concern is a NaN
    // that POISONS AN ACCUMULATOR (#39's KVO B2 is the live case). VWMA carries
    // nothing across bars, so a non-finite value is contained to the bar that produced
    // it. KAMA below is the contrast -- it carries `sumROC1`, so the same reasoning
    // does not reach it.
    ("VWMA", "tempV", "unguarded by decision: stateless per bar", "nan_inf_output"),
];

/// Every variable name mentioned anywhere in `e`.
fn vars_of(e: &Expr, out: &mut HashSet<String>) {
    match e {
        Expr::Var(v) => {
            out.insert(v.clone());
        }
        Expr::ArrayAccess(v, idx) => {
            out.insert(v.clone());
            vars_of(idx, out);
        }
        Expr::BinOp(l, _, r) => {
            vars_of(l, out);
            vars_of(r, out);
        }
        Expr::Cast(_, i) | Expr::Not(i) | Expr::BitwiseNot(i) | Expr::AddressOf(i) => {
            vars_of(i, out)
        }
        Expr::PostIncrement(i)
        | Expr::PostDecrement(i)
        | Expr::PreIncrement(i)
        | Expr::PreDecrement(i) => vars_of(i, out),
        Expr::FuncCall(_, args) => args.iter().for_each(|a| vars_of(a, out)),
        Expr::Ternary(c, t, f) => {
            vars_of(c, out);
            vars_of(t, out);
            vars_of(f, out);
        }
        Expr::PointerDeref(v) => {
            out.insert(v.clone());
        }
        Expr::Literal(_) | Expr::IntLiteral(_) => {}
    }
}

fn names(e: &Expr) -> HashSet<String> {
    let mut s = HashSet::new();
    vars_of(e, &mut s);
    s
}

/// Variables assigned from themselves by `+`/`-` inside a loop body — the shape that
/// makes exact 0.0 reachable through absorption rather than through a flat window.
///
/// `x = x + t` and `x += t` both qualify; `x = a / b` does not, and neither does a
/// variable only ever assigned a fresh value, because that cannot accumulate residue.
/// `in_loop` is threaded rather than inferred from the call site: the loops are not
/// all at the top of a body (ER's are nested under the priming block), and matching
/// only top-level loop statements found none of ER's accumulators — a silent empty
/// result that reads exactly like a clean sweep.
fn accumulated_in_loop(body: &[Statement], in_loop: bool, out: &mut HashSet<String>) {
    for st in body {
        match st {
            // `x += fabs(t)` parses to `Assign{ value: BinOp(x, Add, ...) }`, so the
            // additive test belongs on the value's operator, and the self-reference is
            // what distinguishes accumulation from a fresh assignment that merely
            // happens to add two other things.
            Statement::Assign { target: Expr::Var(t), value, .. }
                if in_loop
                    && names(value).contains(t)
                    && matches!(value, Expr::BinOp(_, BinOp::Add | BinOp::Sub, _)) =>
            {
                out.insert(t.clone());
            }
            Statement::While { body, .. }
            | Statement::DoWhile { body, .. }
            | Statement::For { body, .. } => accumulated_in_loop(body, true, out),
            Statement::ForC { body, init, update, .. } => {
                accumulated_in_loop(body, true, out);
                accumulated_in_loop(std::slice::from_ref(init), true, out);
                accumulated_in_loop(std::slice::from_ref(update), true, out);
            }
            Statement::Block { body } => accumulated_in_loop(body, in_loop, out),
            Statement::If { then_body, else_body, .. } => {
                accumulated_in_loop(then_body, in_loop, out);
                accumulated_in_loop(else_body, in_loop, out);
            }
            Statement::Switch { cases, default, .. } => {
                for (_, b) in cases {
                    accumulated_in_loop(b, in_loop, out);
                }
                accumulated_in_loop(default, in_loop, out);
            }
            _ => {}
        }
    }
}

/// Does `cond` test `var` against ZERO?
///
/// `TA_IS_ZERO(v)`, `TA_IS_ZERO_SCALED(v, s)`, `v > 0.0`, `v != 0.0`, `v <= 0.0` and
/// their negations count. A comparison of `var` against another *variable* does not.
///
/// That distinction is the whole check, and it is stricter than "some guard on that
/// same variable" for a measured reason: ER's pre-#350 guard was `sumROC1 <=
/// periodROC`, which names the divisor and still cannot fire when `periodROC` is
/// negative. Accepting any mention of the variable passes that defect — verified by
/// `the_sweep_detects_a_reintroduced_er_defect`, which fails under the looser rule.
/// Only a comparison against zero can establish that the divisor is non-zero.
fn tests_var_against_zero(cond: &Expr, var: &str) -> bool {
    fn is_zero_literal(e: &Expr) -> bool {
        matches!(e, Expr::Literal(z) if *z == 0.0) || matches!(e, Expr::IntLiteral(0))
    }
    // A positive lower bound establishes non-zero just as well as a zero test, and the
    // corpus uses it: MAMA divides by `tempReal` under `if( tempReal > 1.0 )`. Only a
    // LITERAL bound counts -- `sumROC1 <= periodROC` compares against a variable that
    // can be negative, which is the ER defect.
    fn is_positive_literal(e: &Expr) -> bool {
        matches!(e, Expr::Literal(z) if *z > 0.0) || matches!(e, Expr::IntLiteral(n) if *n > 0)
    }
    match cond {
        Expr::FuncCall(name, args) if name.contains("IS_ZERO") => {
            args.first().map(|a| names(a).contains(var)).unwrap_or(false)
        }
        Expr::BinOp(l, op, r) => match op {
            BinOp::And | BinOp::Or => {
                tests_var_against_zero(l, var) || tests_var_against_zero(r, var)
            }
            BinOp::Greater | BinOp::GreaterEq => {
                (names(l).contains(var) && (is_zero_literal(r) || is_positive_literal(r)))
                    || (names(r).contains(var) && is_zero_literal(l))
            }
            BinOp::Less | BinOp::LessEq | BinOp::Eq | BinOp::NotEq => {
                (names(l).contains(var) && is_zero_literal(r))
                    || (names(r).contains(var) && (is_zero_literal(l) || is_positive_literal(l)))
            }
            _ => false,
        },
        Expr::Not(inner) => tests_var_against_zero(inner, var),
        _ => false,
    }
}

/// Extend `accum` through plain copies: `t = v` where `v` is already accumulated.
///
/// The divisor is routinely a copy taken once per bar rather than the accumulator
/// itself — VORTEX reads `curTR = sTR;` and then divides by `curTR`, while `sTR` is
/// what `+=`/`-=` build up. Without this the sweep is silent on VORTEX's own defect,
/// which is how it was found: `the_sweep_detects_a_reintroduced_vortex_defect` failed
/// with an empty finding list.
///
/// Iterated to a fixpoint because a copy of a copy is still the same quantity.
fn propagate_copies(f: &FuncDef, accum: &mut HashSet<String>) {
    fn walk(body: &[Statement], accum: &HashSet<String>, add: &mut Vec<String>) {
        for st in body {
            match st {
                Statement::Assign { target: Expr::Var(t), value: Expr::Var(v), .. }
                    if accum.contains(v) && !accum.contains(t) =>
                {
                    add.push(t.clone());
                }
                Statement::While { body, .. }
                | Statement::DoWhile { body, .. }
                | Statement::For { body, .. }
                | Statement::ForC { body, .. }
                | Statement::Block { body } => walk(body, accum, add),
                Statement::If { then_body, else_body, .. } => {
                    walk(then_body, accum, add);
                    walk(else_body, accum, add);
                }
                _ => {}
            }
        }
    }
    loop {
        let mut add = Vec::new();
        walk(&f.body, accum, &mut add);
        walk(&f.private_body, accum, &mut add);
        if add.is_empty() {
            return;
        }
        accum.extend(add);
    }
}

/// `t` where the body contains `t = fabs(v)`, mapped to `v`.
///
/// A guard is routinely written on a magnitude copy rather than on the divisor:
/// HT_DCPHASE reads `tempReal = fabs(imagPart); if( tempReal > 0.0 ) ... realPart /
/// imagPart`. That is a correct guard, and without this map the sweep reports it as a
/// finding -- a false positive on the exact idiom it should be teaching.
fn magnitude_aliases(f: &FuncDef) -> Vec<(String, String)> {
    fn walk(body: &[Statement], out: &mut Vec<(String, String)>) {
        for st in body {
            match st {
                Statement::Assign { target: Expr::Var(t), value: Expr::FuncCall(name, args), .. }
                    if name == "fabs" || name == "std_fabs" =>
                {
                    if let Some(Expr::Var(v)) = args.first() {
                        out.push((t.clone(), v.clone()));
                    }
                }
                Statement::While { body, .. }
                | Statement::DoWhile { body, .. }
                | Statement::For { body, .. }
                | Statement::ForC { body, .. }
                | Statement::Block { body } => walk(body, out),
                Statement::If { then_body, else_body, .. } => {
                    walk(then_body, out);
                    walk(else_body, out);
                }
                _ => {}
            }
        }
    }
    let mut out = Vec::new();
    walk(&f.body, &mut out);
    walk(&f.private_body, &mut out);
    out
}

/// Names declared `double` in either body. The accumulator sweep is restricted to
/// these: an integer loop counter is add-accumulated too (`i = i + 1`), and reporting
/// `DIV: divides by i` is noise that buries the real rows.
fn real_valued(f: &FuncDef) -> HashSet<String> {
    fn walk(body: &[Statement], out: &mut HashSet<String>) {
        for st in body {
            match st {
                Statement::VarDecl { var_type: VarType::Real, name, .. } => {
                    out.insert(name.clone());
                }
                Statement::While { body, .. }
                | Statement::DoWhile { body, .. }
                | Statement::For { body, .. }
                | Statement::ForC { body, .. }
                | Statement::Block { body } => walk(body, out),
                Statement::If { then_body, else_body, .. } => {
                    walk(then_body, out);
                    walk(else_body, out);
                }
                _ => {}
            }
        }
    }
    let mut out = HashSet::new();
    walk(&f.body, &mut out);
    walk(&f.private_body, &mut out);
    out
}

#[derive(Debug)]
struct Finding {
    func: String,
    divisor: String,
}

/// Walk an expression for divisions whose denominator is an accumulated variable that
/// no guard in `guards` tests.
fn scan_expr(
    e: &Expr,
    accum: &HashSet<String>,
    guards: &[Expr],
    aliases: &[(String, String)],
    func: &str,
    out: &mut Vec<Finding>,
) {
    if let Expr::BinOp(num, BinOp::Div, den) = e {
        for v in names(den) {
            let guarded = guards.iter().any(|g| {
                tests_var_against_zero(g, &v)
                    || aliases
                        .iter()
                        .any(|(alias, of)| *of == v && tests_var_against_zero(g, alias))
            });
            if accum.contains(&v) && !guarded {
                out.push(Finding { func: func.to_string(), divisor: v });
            }
        }
        scan_expr(num, accum, guards, aliases, func, out);
        scan_expr(den, accum, guards, aliases, func, out);
        return;
    }
    match e {
        Expr::BinOp(l, _, r) => {
            scan_expr(l, accum, guards, aliases, func, out);
            scan_expr(r, accum, guards, aliases, func, out);
        }
        Expr::Cast(_, i) | Expr::Not(i) | Expr::BitwiseNot(i) | Expr::AddressOf(i) => {
            scan_expr(i, accum, guards, aliases, func, out)
        }
        Expr::FuncCall(_, args) => {
            args.iter().for_each(|a| scan_expr(a, accum, guards, aliases, func, out))
        }
        // A ternary's own condition guards both arms.
        Expr::Ternary(c, t, f) => {
            scan_expr(c, accum, guards, aliases, func, out);
            let mut inner = guards.to_vec();
            inner.push((**c).clone());
            scan_expr(t, accum, &inner, aliases, func, out);
            scan_expr(f, accum, &inner, aliases, func, out);
        }
        _ => {}
    }
}

/// `in_loop` gates reporting, not walking. A division outside every loop cannot be
/// dividing by a cross-iteration accumulation even when the name matches: C bodies
/// reuse `tempReal` freely, and HT_TRENDLINE's `rad2Deg = 45.0/tempReal` sits in the
/// prologue where `tempReal` is `atan(1)`, unrelated to the `tempReal` summed in the
/// steady loop 270 lines later.
fn scan_stmts(
    body: &[Statement],
    accum: &HashSet<String>,
    guards: &[Expr],
    aliases: &[(String, String)],
    in_loop: bool,
    func: &str,
    out: &mut Vec<Finding>,
) {
    for st in body {
        match st {
            Statement::Assign { value, .. } if in_loop => {
                scan_expr(value, accum, guards, aliases, func, out)
            }
            Statement::VarDecl { init: Some(v), .. } if in_loop => {
                scan_expr(v, accum, guards, aliases, func, out)
            }
            Statement::Expr(e) | Statement::Return { value: Some(e) } if in_loop => {
                scan_expr(e, accum, guards, aliases, func, out)
            }
            Statement::If { condition, then_body, else_body, .. } => {
                if in_loop {
                    scan_expr(condition, accum, guards, aliases, func, out);
                }
                let mut inner = guards.to_vec();
                inner.push(condition.clone());
                scan_stmts(then_body, accum, &inner, aliases, in_loop, func, out);
                // The else arm is guarded by the negation, which `tests_var` treats
                // the same way: it names the variable either way.
                scan_stmts(else_body, accum, &inner, aliases, in_loop, func, out);
            }
            Statement::While { condition, body } | Statement::DoWhile { condition, body } => {
                scan_expr(condition, accum, guards, aliases, func, out);
                scan_stmts(body, accum, guards, aliases, true, func, out);
            }
            Statement::For { body, .. } => {
                scan_stmts(body, accum, guards, aliases, true, func, out)
            }
            Statement::Block { body } => {
                scan_stmts(body, accum, guards, aliases, in_loop, func, out)
            }
            Statement::ForC { condition, body, .. } => {
                scan_expr(condition, accum, guards, aliases, func, out);
                scan_stmts(body, accum, guards, aliases, true, func, out);
            }
            Statement::Switch { cases, default, .. } => {
                for (_, b) in cases {
                    scan_stmts(b, accum, guards, aliases, in_loop, func, out);
                }
                scan_stmts(default, accum, guards, aliases, in_loop, func, out);
            }
            _ => {}
        }
    }
}

fn findings_for(f: &FuncDef) -> Vec<Finding> {
    let mut accum = HashSet::new();
    accumulated_in_loop(&f.body, false, &mut accum);
    accumulated_in_loop(&f.private_body, false, &mut accum);
    propagate_copies(f, &mut accum);
    let reals = real_valued(f);
    accum.retain(|v| reals.contains(v));
    if accum.is_empty() {
        return Vec::new();
    }
    let aliases = magnitude_aliases(f);
    let mut out = Vec::new();
    // Both bodies: `private_body` is where the arithmetic lives for every function
    // that declares a `_private` variant, and scanning only `body` skipped ER's
    // divisions entirely.
    scan_stmts(&f.body, &accum, &[], &aliases, false, &f.name, &mut out);
    scan_stmts(&f.private_body, &accum, &[], &aliases, false, &f.name, &mut out);
    out.sort_by(|a, b| a.divisor.cmp(&b.divisor));
    out.dedup_by(|a, b| a.func == b.func && a.divisor == b.divisor);
    out
}

#[test]
fn loop_accumulated_divisors_are_guarded_on_themselves() {
    let funcs = load();
    let mut flagged: Vec<String> = Vec::new();
    for f in &funcs {
        for fd in findings_for(f) {
            if ANNOTATED.iter().any(|(fn_, v, _, _)| *fn_ == fd.func && *v == fd.divisor) {
                continue;
            }
            flagged.push(format!("{}: divides by `{}`", fd.func, fd.divisor));
        }
    }
    flagged.sort();
    flagged.dedup();
    assert!(
        flagged.is_empty(),
        "divisor(s) accumulated across loop iterations with no guard testing that same \
         variable — each is either a missing zero guard or a known-safe case that \
         belongs in ANNOTATED with its reason:\n  {}",
        flagged.join("\n  ")
    );
}

/// The sweep must be able to see the defect it was written for.
///
/// A structural check that reports nothing is indistinguishable from one whose matcher
/// silently stopped matching — the failure mode #381 calls out and the reason this test
/// exists next to the sweep rather than in its place. ER's shipped guard is reduced to
/// the asymmetric clamp alone, which is exactly the defect that was fixed in #350, and
/// the sweep must flag it.
#[test]
fn the_sweep_detects_a_reintroduced_er_defect() {
    let funcs = load();
    let er = funcs.iter().find(|f| f.name == "ER").expect("ER is in the input tree");

    // Sanity: as shipped, ER's divisor is guarded and the sweep is silent about it.
    assert!(
        findings_for(er).is_empty(),
        "ER ships with an exact `sumROC1 <= 0.0` denominator test; the sweep should be \
         silent on it"
    );

    // Now strip that test back to the KAMA-parity clamp alone — the pre-#350 defect,
    // where the guard names the divisor only in a comparison that cannot fire for a
    // negative `periodROC`.
    let mut broken = er.clone();
    strip_exact_zero_test(&mut broken.body);
    strip_exact_zero_test(&mut broken.private_body);
    let found = findings_for(&broken);
    assert!(
        !found.is_empty(),
        "the sweep did not flag ER once its exact denominator test was removed — it \
         cannot see the defect class it exists for"
    );
}

/// Remove `sumROC1 <= 0.0 ||` from every guard, leaving the asymmetric clamp.
fn strip_exact_zero_test(body: &mut [Statement]) {
    fn is_exact_zero_test(e: &Expr) -> bool {
        matches!(e, Expr::BinOp(l, BinOp::LessEq, r)
            if matches!(**l, Expr::Var(ref v) if v == "sumROC1")
                && matches!(**r, Expr::Literal(z) if z == 0.0))
    }
    fn fix(e: &Expr) -> Expr {
        if let Expr::BinOp(l, BinOp::Or, r) = e {
            if is_exact_zero_test(l) {
                return (**r).clone();
            }
            if is_exact_zero_test(r) {
                return (**l).clone();
            }
        }
        e.clone()
    }
    for st in body.iter_mut() {
        match st {
            Statement::If { condition, then_body, else_body, .. } => {
                *condition = fix(condition);
                strip_exact_zero_test(then_body);
                strip_exact_zero_test(else_body);
            }
            Statement::While { body, .. }
            | Statement::DoWhile { body, .. }
            | Statement::For { body, .. }
            | Statement::ForC { body, .. }
            | Statement::Block { body } => strip_exact_zero_test(body),
            _ => {}
        }
    }
}

/// The same proof against the other defect on the card, and the cleanest of the three.
///
/// VORTEX's shipped guard is `if( curTR > 0.0 )` on the division itself. The defect it
/// replaced gated the divide on the flat-bar counter instead — `nullRun >=
/// optInTimePeriod` — which is true of a *different* variable and so proves nothing
/// about the running true-range sum that FP absorption can drive to exactly 0.0.
///
/// Asked for on #381 as a condition of landing: a gate that cannot fail is worse than
/// no gate. This one reintroduces the historical shape rather than deleting the guard,
/// so it fails if the sweep merely notices "no guard at all".
#[test]
fn the_sweep_detects_a_reintroduced_vortex_defect() {
    let funcs = load();
    let vortex = funcs.iter().find(|f| f.name == "VORTEX").expect("VORTEX is in the tree");

    assert!(
        findings_for(vortex).is_empty(),
        "VORTEX ships with `curTR > 0.0` on the division; the sweep should be silent"
    );

    // Swap the exact test on the divisor for the counter test the defect used.
    let mut broken = vortex.clone();
    counter_gate_the_divide(&mut broken.body);
    counter_gate_the_divide(&mut broken.private_body);
    let found = findings_for(&broken);
    assert!(
        found.iter().any(|f| f.divisor == "curTR"),
        "the sweep did not flag VORTEX once its divide was gated on the flat-bar \
         counter instead of on the divisor — got {found:?}"
    );
}

/// Rewrite `curTR > 0.0` into `nullRun < optInTimePeriod`: a guard that is about the
/// flat-bar counter, not about the quantity being divided by.
fn counter_gate_the_divide(body: &mut [Statement]) {
    fn is_curtr_test(e: &Expr) -> bool {
        matches!(e, Expr::BinOp(l, BinOp::Greater, r)
            if matches!(**l, Expr::Var(ref v) if v == "curTR")
                && matches!(**r, Expr::Literal(z) if z == 0.0))
    }
    for st in body.iter_mut() {
        match st {
            Statement::If { condition, then_body, else_body, .. } => {
                if is_curtr_test(condition) {
                    *condition = Expr::BinOp(
                        Box::new(Expr::Var("nullRun".to_string())),
                        BinOp::Less,
                        Box::new(Expr::Var("optInTimePeriod".to_string())),
                    );
                }
                counter_gate_the_divide(then_body);
                counter_gate_the_divide(else_body);
            }
            Statement::While { body, .. }
            | Statement::DoWhile { body, .. }
            | Statement::For { body, .. }
            | Statement::ForC { body, .. }
            | Statement::Block { body } => counter_gate_the_divide(body),
            _ => {}
        }
    }
}

/// An annotation that rests on a YAML flag must be re-read when the flag goes.
///
/// The table's own argument is that a stated reason can be contradicted where a bare
/// allowlist cannot. That only holds if something actually checks — VWMA's clearance
/// rests on `nan_inf_output` declaring that non-finite output is intended, so removing
/// the flag should surface the entry rather than leave it silently holding.
#[test]
fn annotation_reasons_still_hold() {
    let funcs = load();
    for (name, divisor, why, required_flag) in ANNOTATED {
        if required_flag.is_empty() {
            continue;
        }
        let f = funcs
            .iter()
            .find(|f| f.name == *name)
            .unwrap_or_else(|| panic!("{name} is annotated but not in the input tree"));
        assert!(
            f.flags.iter().any(|fl| fl == required_flag),
            "{name}'s divisor `{divisor}` is annotated \"{why}\", which rests on the \
             `{required_flag}` flag -- and {name} no longer declares it. The reason has \
             expired: re-read the division rather than re-adding the flag to silence this."
        );
    }
}
