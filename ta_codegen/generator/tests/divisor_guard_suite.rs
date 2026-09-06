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

/// An annotation whose subject is no longer flagged is stale, and must surface.
///
/// `ANNOTATED` is consumed by the sweep as a `continue`, so a row outlives the division
/// it excuses: fix the guard and the row sits there forever, still asserting an open
/// question that has been closed. That is the same silent-allowlist failure the table
/// exists to avoid, one step removed. #385 produced the first instance -- KAMA's row
/// went stale the moment its denominator test landed.
#[test]
fn no_annotation_is_stale() {
    let funcs = load();
    let mut stale = Vec::new();
    for (name, divisor, why, _) in ANNOTATED {
        let f = funcs
            .iter()
            .find(|f| f.name == *name)
            .unwrap_or_else(|| panic!("{name} is annotated but not in the input tree"));
        if !findings_for(f).iter().any(|fd| fd.divisor == *divisor) {
            stale.push(format!("{name}: `{divisor}` -- annotated \"{why}\""));
        }
    }
    assert!(
        stale.is_empty(),
        "annotation(s) whose division the sweep no longer reports. The guard was fixed \
         or the divisor renamed; delete the row rather than leaving it asserting a \
         question that is closed:\n  {}",
        stale.join("\n  ")
    );
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

/// Variables assigned, inside a loop, from an expression that MULTIPLIES OR DIVIDES
/// something — mapped to the variables that expression reads.
///
/// This is the second defect shape, and it is not the accumulator one. STOCH used to
/// guard `highest - lowest` while dividing by a copy of it scaled by 1/100 (#390).
/// Scaling can send a non-zero quantity to exactly 0.0 by underflow, so a guard on the
/// pre-scaled value does not establish what the division needs — the divisor is a
/// different number. `the_scaled_arm_fires_on_a_scaled_divisor_and_not_otherwise`
/// reconstructs that shape; no shipped function carries it now. Addition and
/// subtraction are excluded: they cannot turn a guarded-non-zero into a zero divisor
/// the way a scaling can.
fn scaled_derivations(f: &FuncDef) -> Vec<(String, HashSet<String>)> {
    fn scaling_reads(e: &Expr) -> Option<HashSet<String>> {
        match e {
            Expr::BinOp(l, BinOp::Mul | BinOp::Div, r) => {
                let mut s = names(l);
                s.extend(names(r));
                Some(s)
            }
            Expr::BinOp(l, _, r) => scaling_reads(l).or_else(|| scaling_reads(r)),
            Expr::Cast(_, i) => scaling_reads(i),
            _ => None,
        }
    }
    fn walk(body: &[Statement], in_loop: bool, out: &mut Vec<(String, HashSet<String>)>) {
        for st in body {
            match st {
                Statement::Assign { target: Expr::Var(t), value, .. } if in_loop => {
                    if let Some(reads) = scaling_reads(value) {
                        if !reads.contains(t) {
                            out.push((t.clone(), reads));
                        }
                    }
                }
                Statement::While { body, .. }
                | Statement::DoWhile { body, .. }
                | Statement::For { body, .. }
                | Statement::ForC { body, .. } => walk(body, true, out),
                Statement::Block { body } => walk(body, in_loop, out),
                Statement::If { then_body, else_body, .. } => {
                    walk(then_body, in_loop, out);
                    walk(else_body, in_loop, out);
                }
                _ => {}
            }
        }
    }
    let mut out = Vec::new();
    walk(&f.body, false, &mut out);
    walk(&f.private_body, false, &mut out);
    out
}

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
    kind: FindingKind,
}

#[derive(Debug, Clone, Copy, PartialEq)]
enum FindingKind {
    /// Divisor is accumulated across loop iterations and no guard tests it.
    Accumulated,
    /// Divisor is a SCALED derivation of a quantity a dominating guard does test —
    /// the guard proves the pre-scaled value non-zero, which the divisor is not.
    ScaledFromGuarded,
}

impl FindingKind {
    fn label(self) -> &'static str {
        match self {
            FindingKind::Accumulated => "accumulated, untested",
            FindingKind::ScaledFromGuarded => "scaled from a guarded value",
        }
    }
}

/// Walk an expression for divisions whose denominator is an accumulated variable that
/// no guard in `guards` tests.
fn scan_expr(
    e: &Expr,
    accum: &HashSet<String>,
    guards: &[Expr],
    aliases: &[(String, String)],
    derived: &[(String, HashSet<String>)],
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
                out.push(Finding {
                    func: func.to_string(),
                    divisor: v.clone(),
                    kind: FindingKind::Accumulated,
                });
            }
            // Second shape: the divisor is untested, but a guard DOES test something
            // the divisor was scaled from. That reads as guarded and is not -- scaling
            // can underflow a non-zero value to exactly 0.0.
            if !guarded {
                let scaled_from_a_guarded_value = derived
                    .iter()
                    .filter(|(d, _)| *d == v)
                    .any(|(_, reads)| {
                        reads.iter().any(|r| guards.iter().any(|g| tests_var_against_zero(g, r)))
                    });
                if scaled_from_a_guarded_value {
                    out.push(Finding {
                        func: func.to_string(),
                        divisor: v,
                        kind: FindingKind::ScaledFromGuarded,
                    });
                }
            }
        }
        scan_expr(num, accum, guards, aliases, derived, func, out);
        scan_expr(den, accum, guards, aliases, derived, func, out);
        return;
    }
    match e {
        Expr::BinOp(l, _, r) => {
            scan_expr(l, accum, guards, aliases, derived, func, out);
            scan_expr(r, accum, guards, aliases, derived, func, out);
        }
        Expr::Cast(_, i) | Expr::Not(i) | Expr::BitwiseNot(i) | Expr::AddressOf(i) => {
            scan_expr(i, accum, guards, aliases, derived, func, out)
        }
        Expr::FuncCall(_, args) => {
            args.iter().for_each(|a| scan_expr(a, accum, guards, aliases, derived, func, out))
        }
        // A ternary's own condition guards both arms.
        Expr::Ternary(c, t, f) => {
            scan_expr(c, accum, guards, aliases, derived, func, out);
            let mut inner = guards.to_vec();
            inner.push((**c).clone());
            scan_expr(t, accum, &inner, aliases, derived, func, out);
            scan_expr(f, accum, &inner, aliases, derived, func, out);
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
    derived: &[(String, HashSet<String>)],
    in_loop: bool,
    func: &str,
    out: &mut Vec<Finding>,
) {
    for st in body {
        match st {
            Statement::Assign { value, .. } if in_loop => {
                scan_expr(value, accum, guards, aliases, derived, func, out)
            }
            Statement::VarDecl { init: Some(v), .. } if in_loop => {
                scan_expr(v, accum, guards, aliases, derived, func, out)
            }
            Statement::Expr(e) | Statement::Return { value: Some(e) } if in_loop => {
                scan_expr(e, accum, guards, aliases, derived, func, out)
            }
            Statement::If { condition, then_body, else_body, .. } => {
                if in_loop {
                    scan_expr(condition, accum, guards, aliases, derived, func, out);
                }
                let mut inner = guards.to_vec();
                inner.push(condition.clone());
                scan_stmts(then_body, accum, &inner, aliases, derived, in_loop, func, out);
                // The else arm is guarded by the negation, which `tests_var` treats
                // the same way: it names the variable either way.
                scan_stmts(else_body, accum, &inner, aliases, derived, in_loop, func, out);
            }
            Statement::While { condition, body } | Statement::DoWhile { condition, body } => {
                scan_expr(condition, accum, guards, aliases, derived, func, out);
                scan_stmts(body, accum, guards, aliases, derived, true, func, out);
            }
            Statement::For { body, .. } => {
                scan_stmts(body, accum, guards, aliases, derived, true, func, out)
            }
            Statement::Block { body } => {
                scan_stmts(body, accum, guards, aliases, derived, in_loop, func, out)
            }
            Statement::ForC { condition, body, .. } => {
                scan_expr(condition, accum, guards, aliases, derived, func, out);
                scan_stmts(body, accum, guards, aliases, derived, true, func, out);
            }
            Statement::Switch { cases, default, .. } => {
                for (_, b) in cases {
                    scan_stmts(b, accum, guards, aliases, derived, in_loop, func, out);
                }
                scan_stmts(default, accum, guards, aliases, derived, in_loop, func, out);
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
    let aliases = magnitude_aliases(f);
    let derived = scaled_derivations(f);
    let mut out = Vec::new();
    // Both bodies: `private_body` is where the arithmetic lives for every function
    // that declares a `_private` variant, and scanning only `body` skipped ER's
    // divisions entirely.
    scan_stmts(&f.body, &accum, &[], &aliases, &derived, false, &f.name, &mut out);
    scan_stmts(&f.private_body, &accum, &[], &aliases, &derived, false, &f.name, &mut out);
    out.sort_by(|a, b| a.divisor.cmp(&b.divisor));
    out.sort_by(|a, b| (a.divisor.clone(), a.kind.label()).cmp(&(b.divisor.clone(), b.kind.label())));
    out.dedup_by(|a, b| a.func == b.func && a.divisor == b.divisor && a.kind == b.kind);
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
            flagged.push(format!("{}: divides by `{}` ({})", fd.func, fd.divisor, fd.kind.label()));
        }
    }
    flagged.sort();
    flagged.dedup();
    assert!(
        flagged.is_empty(),
        "divisor(s) no dominating guard establishes non-zero for — either accumulated \
         across loop iterations with nothing testing them, or SCALED from a value the \
         guard does test, which scaling can underflow to 0.0 independently. Each is a \
         missing guard or a case that belongs in ANNOTATED with its reason:\n  {}",
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

/// The scaled-derivation arm must fire on the defect and go quiet without it.
///
/// Same shape as the ER and VORTEX self-tests, but in both directions, because this
/// arm has no shipped instance to point at: #390 removed the last one by giving STOCH
/// the range itself as its divisor. A check that flagged every scaled divisor
/// unconditionally would satisfy the "fires" half and be worthless, so the two
/// "goes quiet" halves are what pin it: one to respecting a guard that dominates
/// the divisor, one to checking that the guard tests what the divisor was scaled
/// from.
#[test]
fn the_scaled_arm_fires_on_a_scaled_divisor_and_not_otherwise() {
    let funcs = load();
    let stoch = funcs.iter().find(|f| f.name == "STOCH").expect("STOCH is in the tree");

    // As shipped, %K divides by `highest - lowest` -- the very expression its guard
    // tests -- so there is nothing to scale and the sweep is silent.
    assert!(
        !findings_for(stoch).iter().any(|f| f.kind == FindingKind::ScaledFromGuarded),
        "STOCH ships dividing by the guarded range itself; the sweep should be silent"
    );

    // Reintroduce the pre-#390 shape: a loop-local scaled by 1/100, divided by while
    // the guard still tests the unscaled range.
    let mut broken = stoch.clone();
    divide_by_a_scaled_copy(&mut broken.body);
    divide_by_a_scaled_copy(&mut broken.private_body);
    assert!(
        findings_for(&broken)
            .iter()
            .any(|f| f.divisor == "diff" && f.kind == FindingKind::ScaledFromGuarded),
        "the sweep did not flag a divisor scaled from the guarded value -- it cannot \
         see the defect class this arm exists for"
    );

    // And with a test of that scaled divisor ADDED in front of the range guard, the
    // finding must go. This is the half that pins the arm to READING the guard: strip
    // its `!guarded` check and this assertion fails, because the range guard is still
    // there to be scaled from. The first check cannot serve -- shipped STOCH divides
    // by an expression rather than a variable, so this arm never examines it at all.
    let mut repaired = broken.clone();
    also_guard_on_diff(&mut repaired.body);
    also_guard_on_diff(&mut repaired.private_body);
    assert!(
        !findings_for(&repaired).iter().any(|f| f.divisor == "diff"),
        "the sweep still flags `diff` after the guard was moved onto it -- the check \
         is not reading the guard, it is flagging every scaled divisor"
    );

    // And a scaled divisor whose SOURCE nothing guards is not this defect: the
    // inference the arm reports is "a guard proves the pre-scaled value non-zero",
    // so with no such guard there is nothing to mis-infer from. Without this, an
    // arm that flagged every unguarded scaled divisor would pass the two above.
    let mut unrelated = stoch.clone();
    scale_from_an_unguarded_source(&mut unrelated.body);
    scale_from_an_unguarded_source(&mut unrelated.private_body);
    assert!(
        !findings_for(&unrelated).iter().any(|f| f.divisor == "diff"),
        "the sweep flagged a divisor scaled from a value no guard tests -- it is \
         reporting every scaled divisor, not the guard/divisor mismatch"
    );
}

/// Same injection as `divide_by_a_scaled_copy`, but `diff` is scaled from `tmp`,
/// which no guard in the body tests.
fn scale_from_an_unguarded_source(body: &mut [Statement]) {
    divide_by_a_scaled_copy(body);
    fn retarget(body: &mut [Statement]) {
        for st in body.iter_mut() {
            match st {
                Statement::Assign { target: Expr::Var(t), value, .. } if t == "diff" => {
                    *value = Expr::BinOp(
                        Box::new(Expr::Var("tmp".to_string())),
                        BinOp::Div,
                        Box::new(Expr::Literal(100.0)),
                    );
                }
                Statement::While { body, .. }
                | Statement::DoWhile { body, .. }
                | Statement::For { body, .. }
                | Statement::ForC { body, .. }
                | Statement::Block { body } => retarget(body),
                Statement::If { then_body, else_body, .. } => {
                    retarget(then_body);
                    retarget(else_body);
                }
                _ => {}
            }
        }
    }
    for st in body.iter_mut() {
        match st {
            Statement::While { body, .. }
            | Statement::DoWhile { body, .. }
            | Statement::For { body, .. }
            | Statement::ForC { body, .. }
            | Statement::Block { body } => retarget(body),
            Statement::If { then_body, else_body, .. } => {
                retarget(then_body);
                retarget(else_body);
            }
            _ => {}
        }
    }
}

/// ADD `diff != 0.0` in front of the range guard, keeping it.
///
/// Conjoining rather than replacing is what makes the caller discriminating. Replace
/// the range guard and both mechanisms stop applying at once -- the divisor becomes
/// tested AND its source stops being tested -- so the finding disappears either way
/// and the assertion cannot say which one did it. Kept, only the divisor test is new.
fn also_guard_on_diff(body: &mut [Statement]) {
    fn guards_the_range(e: &Expr) -> bool {
        match e {
            Expr::FuncCall(name, args) => {
                name.contains("IS_ZERO") && args.iter().any(|a| names(a).contains("highest"))
            }
            Expr::BinOp(l, _, r) => guards_the_range(l) || guards_the_range(r),
            Expr::Not(i) => guards_the_range(i),
            _ => false,
        }
    }
    for st in body.iter_mut() {
        match st {
            Statement::If { condition, then_body, else_body, .. } => {
                if guards_the_range(condition) {
                    *condition = Expr::BinOp(
                        Box::new(Expr::BinOp(
                            Box::new(Expr::Var("diff".to_string())),
                            BinOp::NotEq,
                            Box::new(Expr::Literal(0.0)),
                        )),
                        BinOp::And,
                        Box::new(condition.clone()),
                    );
                }
                also_guard_on_diff(then_body);
                also_guard_on_diff(else_body);
            }
            Statement::While { body, .. }
            | Statement::DoWhile { body, .. }
            | Statement::For { body, .. }
            | Statement::ForC { body, .. }
            | Statement::Block { body } => also_guard_on_diff(body),
            _ => {}
        }
    }
}

/// Rewrite `num / (highest - lowest)` into `num / diff`, preceded by
/// `diff = (highest - lowest) / 100.0` -- the divisor shape #390 removed.
fn divide_by_a_scaled_copy(body: &mut [Statement]) {
    fn is_range(e: &Expr) -> bool {
        matches!(e, Expr::BinOp(l, BinOp::Sub, r)
            if matches!(**l, Expr::Var(ref v) if v == "highest")
                && matches!(**r, Expr::Var(ref v) if v == "lowest"))
    }
    fn rewrite(e: &Expr, hit: &mut bool) -> Expr {
        match e {
            Expr::BinOp(l, BinOp::Div, r) if is_range(r) => {
                *hit = true;
                Expr::BinOp(
                    Box::new(rewrite(l, hit)),
                    BinOp::Div,
                    Box::new(Expr::Var("diff".to_string())),
                )
            }
            Expr::BinOp(l, op, r) => {
                Expr::BinOp(Box::new(rewrite(l, hit)), op.clone(), Box::new(rewrite(r, hit)))
            }
            _ => e.clone(),
        }
    }
    fn scale_assign() -> Statement {
        Statement::Assign {
            target: Expr::Var("diff".to_string()),
            value: Expr::BinOp(
                Box::new(Expr::BinOp(
                    Box::new(Expr::Var("highest".to_string())),
                    BinOp::Sub,
                    Box::new(Expr::Var("lowest".to_string())),
                )),
                BinOp::Div,
                Box::new(Expr::Literal(100.0)),
            ),
            compound: false,
        }
    }
    fn walk(body: &mut Vec<Statement>) {
        let mut inject = Vec::new();
        for (i, st) in body.iter_mut().enumerate() {
            match st {
                Statement::If { then_body, else_body, .. } => {
                    let mut hit = false;
                    for b in [&mut *then_body, &mut *else_body] {
                        for inner in b.iter_mut() {
                            if let Statement::Assign { value, .. } = inner {
                                *value = rewrite(value, &mut hit);
                            }
                        }
                    }
                    if hit {
                        inject.push(i);
                    }
                    walk(then_body);
                    walk(else_body);
                }
                Statement::While { body, .. }
                | Statement::DoWhile { body, .. }
                | Statement::For { body, .. }
                | Statement::ForC { body, .. }
                | Statement::Block { body } => walk(body),
                _ => {}
            }
        }
        for i in inject.into_iter().rev() {
            body.insert(i, scale_assign());
        }
    }
    // `body` is a slice here, so recurse into the owned Vecs the statements hold.
    for st in body.iter_mut() {
        match st {
            Statement::While { body, .. }
            | Statement::DoWhile { body, .. }
            | Statement::For { body, .. }
            | Statement::ForC { body, .. }
            | Statement::Block { body } => walk(body),
            Statement::If { then_body, else_body, .. } => {
                walk(then_body);
                walk(else_body);
            }
            _ => {}
        }
    }
}
