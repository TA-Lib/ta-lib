//! A loop's if-tree whose every leaf stores one output element, as one store
//! of a select: `if( c ) out[k++] = a; else out[k++] = b;` becomes
//! `out[k++] = select(eager(c), a, b)`, nested for a deeper tree. `eager`
//! respells every `&&` / `||` in a condition as a non-short-circuit `&` / `|`.
//! A candlestick decision is unpredictable, so its branches mispredict, and
//! the select runs every operand on every bar instead.
//!
//! C and Rust batch only, run before anything derived from the body. Java and
//! C# never see the pseudo-calls; their renderers refuse one.
//!
//! Every operand of the tree runs on every pass, the leaf values too in
//! Rust, so every operand must be built only from what an allowlist admits
//! by declared type (see `eager_safe`), and none may read a stepped store
//! index. Anything else keeps its branch. Length-preserving at every level.
//!
//! Inside a rewritten condition each atom takes its cheapest exact boolean
//! form: a two-constant flag compared with a constant or another flag becomes
//! its predicate (`!` kept explicit, never flipped into the opposite
//! comparison, for NaN), and a comparison with `min` / `max` becomes the `&` /
//! `|` of plain comparisons. The latter differs from the original only on a NaN
//! operand, which the library leaves undefined mid-series.
//!
//! An integer store renders in C as a mask select, which gcc keeps branch-free
//! where it turns a `?:` back into a branch chain.
//!
//! A tree whose every condition is one atom is a predictable domain check,
//! worth a select only in a loop that carries no state, where dropping the
//! branch lets the loop vectorize.
//!
//! A decision nested directly in another `if` whose every way out but the
//! inner matches falls to the outer `else`, `if( pre ) { if( rest ) X; else Z; }
//! else Z;`, keeps the outer test one branch: the input marks it as the cheap,
//! selective pre-check, and only the inner tree is rewritten. An `else if`
//! chain of such tests ending in one common `else` keeps each test a branch.
//! A test's conjuncts are counted, not chained, so the compiler cannot split
//! them back into a chain of branches; counting runs them all, so they must
//! pass the whole-tree rules above, else the test keeps its short circuit.
//! Any other nest, and a flat condition, is rewritten whole.
//!
//! A condition comment rides on its operand as a note pseudo-call, and an arm
//! comment on its arm's value: a trailing one at the end of the value's line,
//! a leading one on a line of its own before it. The renderers emit both
//! inline and [`lay_out`] places them.

use crate::backends::builtins::{MathFn, SpecialBuiltin};
use crate::helper_registry::{try_inline_expr, HelperRegistry};
use crate::ir::{BinOp, CircBuf, Expr, FuncDef, LookbackExpr, Output, ParamType, Statement, VarType};
use crate::streaming::{rewrite_expr, walk_expr};

pub(crate) const EAGER_AND: &str = "ta_eager_and";
pub(crate) const EAGER_OR: &str = "ta_eager_or";
/// `(condition, then, else)`.
pub(crate) const SELECT: &str = "ta_select_unpredictable";
/// [`SELECT`] whose arms are integers, so C may pick by mask.
pub(crate) const SELECT_INT: &str = "ta_select_unpredictable_int";
/// `(condition, then, else)`: `then` only where the eager condition holds,
/// `else` elsewhere, by a plain conditional rather than a select.
pub(crate) const GUARD: &str = "ta_eager_guard";
/// Whether every 0/1 operand holds, as a count of the ones that do.
pub(crate) const EAGER_ALL: &str = "ta_eager_all";

const PURE_HELPERS: &[&str] = &[
    "ta_realbody",
    "ta_candlecolor",
    "ta_uppershadow",
    "ta_lowershadow",
    "ta_highlowrange",
    "ta_realbodygapup",
    "ta_realbodygapdown",
    "ta_candlegapup",
    "ta_candlegapdown",
    "ta_candlerange",
];

/// Pure, but it divides.
const DIVIDING_HELPER: &str = "ta_candleaverage";

/// The bar a candle helper's price arguments name, when they are `inOpen`,
/// `inHigh`, `inLow` and `inClose` at one index: C reads them through a macro
/// that takes that index alone.
fn candle_bar(ohlc: &[Expr]) -> Option<&Expr> {
    let mut at: Option<&Expr> = None;
    for (a, want) in ohlc.iter().zip(["inOpen", "inHigh", "inLow", "inClose"]) {
        let Expr::ArrayAccess(n, i) = a else { return None };
        if n != want || at.is_some_and(|x| !crate::streaming::exprs_equal(x, i)) {
            return None;
        }
        at = Some(i);
    }
    (ohlc.len() == 4).then_some(at?)
}

/// `NOTE<comment>(operand)`: the comment trails the operand.
const NOTE: &str = "\u{1}note:";
/// `LEAD<comment>(operand)`: the comment has a line of its own before it.
const LEAD: &str = "\u{1}lead:";
/// Opens a continuation line, indented by [`lay_out`].
const BREAK: &str = "\n\u{1}";
const OPEN: char = '\u{2}';
const CLOSE: char = '\u{3}';
const LEAD_OPEN: char = '\u{4}';

pub(crate) fn is_pseudo_call(name: &str) -> bool {
    matches!(name, EAGER_AND | EAGER_OR | SELECT | SELECT_INT | GUARD | EAGER_ALL) || name.starts_with(NOTE) || name.starts_with(LEAD)
}

pub(crate) fn is_select(name: &str) -> bool {
    name == SELECT || name == SELECT_INT
}

/// `e` without the notes wrapping it, and those notes outermost first.
pub(crate) fn peel(mut e: &Expr) -> (&Expr, Vec<(&str, bool)>) {
    let mut notes = Vec::new();
    while let Expr::FuncCall(n, a) = e {
        let (Some(note), [inner]) = (note_of(n), a.as_slice()) else { break };
        notes.push(note);
        e = inner;
    }
    (e, notes)
}

/// A note call's comment, and whether it leads its operand.
pub(crate) fn note_of(name: &str) -> Option<(&str, bool)> {
    name.strip_prefix(NOTE).map(|c| (c, false)).or_else(|| name.strip_prefix(LEAD).map(|c| (c, true)))
}

/// A rendered operand with its comment.
pub(crate) fn noted(operand: &str, (comment, leads): (&str, bool)) -> String {
    if leads {
        format!("{BREAK}{LEAD_OPEN}{comment}{CLOSE}{BREAK}{operand}")
    } else {
        format!("{operand}{OPEN}{comment}{CLOSE}")
    }
}

/// `left`, `sep`, `right`, breaking the line after `sep` when `left` ends on a
/// comment, so that a line never carries two.
pub(crate) fn glue(left: &str, sep: &str, right: &str) -> String {
    let last = left.rsplit(BREAK).next().unwrap_or(left);
    if last.contains(OPEN) {
        format!("{left}{}{BREAK}{right}", sep.trim_end())
    } else {
        format!("{left}{sep}{right}")
    }
}

pub(crate) fn join(parts: &[String], sep: &str) -> String {
    let mut out = String::new();
    for (i, p) in parts.iter().enumerate() {
        out = if i == 0 { p.clone() } else { glue(&out, sep, p) };
    }
    out
}

/// Each note's comment at the end of its line, aligned within its statement;
/// each continuation line one level in from the line that starts it.
#[must_use]
pub(crate) fn lay_out(text: &str, block_style: bool) -> String {
    if !text.contains(OPEN) && !text.contains('\u{1}') {
        return text.to_string();
    }
    let comment = |n: &str| if block_style { format!("/* {n} */") } else { format!("// {n}") };
    let mut out = String::new();
    let mut group: Vec<(String, Vec<String>)> = Vec::new();
    let flush = |group: &mut Vec<(String, Vec<String>)>, out: &mut String| {
        let align = group.iter().filter(|(c, _)| !c.contains(LEAD_OPEN)).map(|(c, _)| c.len()).filter(|&w| w <= 72).max().unwrap_or(0);
        for (code, notes) in group.drain(..) {
            if let Some((pad, lead)) = code.split_once(LEAD_OPEN) {
                out.push_str(&format!("{pad}{}\n", comment(lead.trim_end_matches(CLOSE))));
                continue;
            }
            out.push_str(&code);
            for n in &notes {
                let gap = " ".repeat(if code.len() < align { align - code.len() + 1 } else { 1 });
                out.push_str(&format!("{gap}{}", comment(n)));
            }
            out.push('\n');
        }
    };
    let mut indent = String::new();
    for line in text.split('\n') {
        let line = if let Some(rest) = line.strip_prefix('\u{1}') {
            if let Some((code, _)) = group.last_mut() {
                code.truncate(code.trim_end().len());
            }
            format!("{indent}    {rest}")
        } else {
            flush(&mut group, &mut out);
            indent = line.chars().take_while(|c| *c == ' ').collect();
            line.to_string()
        };
        let (mut code, mut notes, mut rest) = (String::new(), Vec::new(), line.as_str());
        while let Some(o) = rest.find(OPEN) {
            code.push_str(&rest[..o]);
            let after = &rest[o + OPEN.len_utf8()..];
            let c = after.find(CLOSE).unwrap_or(after.len());
            notes.push(after[..c].to_string());
            rest = after.get(c + CLOSE.len_utf8()..).unwrap_or("");
        }
        code.push_str(rest);
        if !notes.is_empty() {
            code.truncate(code.trim_end().len());
        }
        group.push((code, notes));
    }
    flush(&mut group, &mut out);
    out.pop();
    out
}

/// Whether `e` is already 0 or 1 in C (a comparison, a negation or a logical
/// operator), so it needs no `!= 0` to join a bitwise `&` / `|`.
pub(crate) fn is_boolean(e: &Expr) -> bool {
    match e {
        Expr::BinOp(_, op, _) => matches!(
            op,
            BinOp::Eq | BinOp::NotEq | BinOp::Less | BinOp::LessEq | BinOp::Greater | BinOp::GreaterEq | BinOp::And | BinOp::Or
        ),
        Expr::Not(_) => true,
        Expr::FuncCall(n, _) if n == EAGER_AND || n == EAGER_OR || n == EAGER_ALL => true,
        Expr::FuncCall(n, a) if note_of(n).is_some() => a.first().is_some_and(is_boolean),
        _ => false,
    }
}

/// What a function declares: its input arrays, those of them real, its real
/// scalar parameters, its parameters in order, its lookback function and the
/// literal part of what that returns. Locals are read off the body.
#[derive(Default)]
pub(crate) struct Declared {
    pub(crate) inputs: Vec<String>,
    pub(crate) arrays: Vec<String>,
    pub(crate) reals: Vec<String>,
    pub(crate) params: Vec<String>,
    pub(crate) lookback: String,
    pub(crate) fixed: i64,
}

impl Declared {
    pub(crate) fn of(func: &FuncDef) -> Self {
        let is_empty = |s: &&Statement| matches!(s, Statement::Comment(_)) || matches!(s, Statement::Block { body } if body.is_empty());
        // `N`, or `max(...) + N` over an argument never negative: the pattern's fixed bars.
        let fixed = match &func.lookback {
            Some(LookbackExpr::Literal(n)) => i64::from(*n),
            Some(LookbackExpr::Code(body)) => match body.iter().filter(|s| !is_empty(s)).collect::<Vec<_>>()[..] {
                [Statement::Return { value: Some(Expr::IntLiteral(n)) }] => *n,
                [Statement::Return { value: Some(Expr::BinOp(l, BinOp::Add, r)) }] => match (l.as_ref(), r.as_ref()) {
                    (Expr::FuncCall(m, args), Expr::IntLiteral(n)) if matches!(MathFn::from_name(m), Some(MathFn::Max)) && args.iter().any(non_negative) => *n,
                    _ => 0,
                },
                _ => 0,
            },
            _ => 0,
        };
        Self {
            inputs: func.inputs.iter().map(|i| i.name.clone()).collect(),
            arrays: func.inputs.iter().filter(|i| i.param_type == ParamType::Real).map(|i| i.name.clone()).collect(),
            reals: func.optional_inputs.iter().filter(|o| o.param_type == ParamType::Real).map(|o| o.name.clone()).collect(),
            params: func.optional_inputs.iter().map(|o| o.name.clone()).collect(),
            lookback: format!("{}_lookback", func.name.to_lowercase()),
            fixed: fixed.max(0),
        }
    }
}

fn non_negative(e: &Expr) -> bool {
    match e {
        Expr::IntLiteral(n) => *n >= 0,
        Expr::Var(v) => crate::candle_settings::is_avg_period(v),
        Expr::FuncCall(m, args) if matches!(MathFn::from_name(m), Some(MathFn::Max)) => args.iter().any(non_negative),
        _ => false,
    }
}

/// What the function's top level proves about `startIdx` on entering a
/// loop there.
#[derive(Clone, Copy, Default)]
struct Entering {
    /// Nothing but `if( startIdx < E ) startIdx = E;` writes `startIdx`, so it
    /// is never below 0: the public tier rejects a negative one.
    raised_only: bool,
    /// The top-level index after which the canonical prologue has raised it
    /// to at least the lookback's fixed part.
    canonical: Option<(usize, i64)>,
    /// The top-level index after which `startIdx <= endIdx` holds.
    ordered_from: Option<usize>,
    /// `endIdx` is never written.
    end_fixed: bool,
}

fn writes_of(body: &[Statement]) -> (Vec<String>, Vec<String>) {
    let (mut written, mut taken) = (Vec::new(), Vec::new());
    each_statement(body, &mut |s| {
        // A declaration starts another variable of that name.
        if let Statement::Assign { target: Expr::Var(v), .. } | Statement::VarDecl { name: v, .. } | Statement::For { var: v, .. } = s {
            written.push(v.clone());
        }
        for e in exprs_of(s) {
            walk_expr(e, &mut |x| match x {
                Expr::PostIncrement(v) | Expr::PreIncrement(v) | Expr::PostDecrement(v) | Expr::PreDecrement(v) => {
                    if let Expr::Var(v) = v.as_ref() {
                        written.push(v.clone());
                    }
                }
                Expr::AddressOf(v) => walk_expr(v, &mut |y| {
                    if let Expr::Var(n) | Expr::ArrayAccess(n, _) = y {
                        taken.push(n.clone());
                    }
                }),
                _ => {}
            });
        }
    });
    (written, taken)
}

fn entering(body: &[Statement], declared: &Declared) -> Entering {
    let (written, taken) = writes_of(body);
    let count = |n: &str| written.iter().filter(|w| *w == n).count();
    let mut decls = Vec::new();
    each_statement(body, &mut |s| {
        if let Statement::VarDecl { name, .. } = s {
            decls.push(name.clone());
        }
    });
    let free = |n: &str| count(n) == 0 && !taken.iter().any(|t| t == n);
    let var = |e: &Expr, n: &str| matches!(e, Expr::Var(v) if v == n);
    let clamp = |s: &Statement| match s {
        Statement::If { condition: Expr::BinOp(l, BinOp::Less, r), then_body, else_body, .. } if var(l, "startIdx") && else_body.is_empty() => {
            matches!(then_body.as_slice(), [Statement::Assign { target, value, compound: false }] if var(target, "startIdx") && value == r.as_ref())
                .then(|| r.as_ref().clone())
        }
        _ => None,
    };
    let exits = |s: &Statement| {
        matches!(s, Statement::If { condition: Expr::BinOp(l, BinOp::Greater, r), then_body, .. }
            if var(l, "startIdx") && var(r, "endIdx") && then_body.iter().any(|t| matches!(t, Statement::Return { .. })))
    };
    let top: Vec<(usize, &Statement)> = body.iter().enumerate().filter(|(_, s)| !matches!(s, Statement::Comment(_))).collect();
    let clamps = top.iter().filter(|(_, s)| clamp(s).is_some()).count();
    if taken.iter().any(|t| t == "startIdx") || count("startIdx") != clamps {
        return Entering::default();
    }
    let last_clamp = top.iter().rposition(|(_, s)| clamp(s).is_some());
    let ordered_from = top.iter().enumerate().find(|(k, (_, s))| exits(s) && last_clamp.is_none_or(|c| *k > c)).map(|(_, (at, _))| *at);
    let canonical = top.windows(3).find(|w| {
        let [(_, Statement::Assign { target: Expr::Var(lb), value: Expr::FuncCall(f, args), compound: false }), (_, c), (_, e)] = w else { return false };
        let verbatim = args.len() == declared.params.len() && args.iter().zip(&declared.params).all(|(a, p)| var(a, p));
        *f == declared.lookback
            && verbatim
            && clamps == 1
            && clamp(c).is_some_and(|v| var(&v, lb))
            && exits(e)
            // A declaration of it runs before the assignment, or names another variable.
            && count(lb) == 1 + decls.iter().filter(|d| *d == lb).count()
            && !taken.iter().any(|t| t == lb)
            && declared.params.iter().all(|p| free(p))
    });
    Entering { raised_only: true, canonical: canonical.map(|w| (w[2].0, declared.fixed)), ordered_from, end_fixed: free("endIdx") }
}

#[must_use]
pub(crate) fn rewrite(body: &[Statement], declared: &Declared, outputs: &[Output], helpers: &HelperRegistry) -> Vec<Statement> {
    let sets = super::fma::build_fma_var_sets(body, outputs, &super::fma::INDEX_PARAM_SEEDS);
    let fs = sets.view();
    let real = |e: &Expr| super::fma::expr_is_float_typed(e, Some(&fs)) && !super::fma::is_definitely_integer(e, &fs);
    let mut decls: Vec<(String, VarType)> = Vec::new();
    each_statement(body, &mut |s| {
        if let Statement::VarDecl { var_type, name, .. } = s {
            decls.push((name.clone(), var_type.clone()));
        }
    });
    // A local read before its first write is indeterminate in C, even where the
    // select then discards it: one runs eagerly only once written before the
    // first loop, or first in every loop pass that names it.
    let holds_loop = |s: &Statement| {
        let mut hit = false;
        each_statement(std::slice::from_ref(s), &mut |x| hit |= matches!(x, Statement::While { .. } | Statement::DoWhile { .. } | Statement::For { .. } | Statement::ForC { .. }));
        hit
    };
    let mut defined: Vec<String> = Vec::new();
    each_statement(body, &mut |s| {
        if let Statement::VarDecl { name, init: Some(_), .. } = s {
            defined.push(name.clone());
        }
    });
    let prologue = &body[..body.iter().position(holds_loop).unwrap_or(body.len())];
    for s in prologue {
        if let Statement::Assign { target: Expr::Var(v), compound: false, .. } = s {
            defined.push(v.clone());
        }
    }
    decls.extend(declared.reals.iter().map(|n| (n.clone(), VarType::Real)));
    // Every rule here goes by name, so a name declared twice, a parameter or
    // candle setting included, is refused: what holds for one of its
    // variables may not hold for the other.
    let parameter = |n: &str| {
        ["startIdx", "endIdx", "outBegIdx", "outNBElement"].contains(&n)
            || crate::candle_settings::is_setting_property(n)
            || declared.inputs.iter().chain(&declared.params).any(|p| p == n)
            || outputs.iter().any(|o| o.name == n)
    };
    let one_decl = |n: &str| decls.iter().filter(|(d, _)| d == n).count() + usize::from(parameter(n) && !declared.reals.iter().any(|r| r == n)) == 1;
    let is_defined = |n: &str| one_decl(n) && (defined.iter().any(|d| d == n) || declared.reals.iter().any(|r| r == n) || written_first(body, n));
    let arrays: Vec<String> = declared.arrays.iter().filter(|a| !decls.iter().any(|(d, _)| d == *a)).cloned().collect();
    let reals: Vec<String> = decls.iter().filter(|(n, t)| *t == VarType::Real && is_defined(n)).map(|(n, _)| n.clone()).collect();
    let undefined: Vec<String> = decls.iter().filter(|(n, _)| !is_defined(n)).map(|(n, _)| n.clone()).collect();
    let length = |a: &str| match decls.iter().find(|(d, _)| d == a) {
        Some((_, VarType::RealArray(len))) if one_decl(a) => len.trim().parse::<i64>().ok(),
        _ => None,
    };
    let filled: Vec<(String, i64)> = prologue
        .iter()
        .filter_map(|s| match s {
            Statement::Assign { target: Expr::ArrayAccess(a, k), compound: false, .. } => match k.as_ref() {
                Expr::IntLiteral(k) if length(a).is_some_and(|len| (0..len).contains(k)) => Some((a.clone(), *k)),
                _ => None,
            },
            _ => None,
        })
        .collect();
    let rules = Rules {
        outputs: outputs.iter().map(|o| o.name.as_str()).collect(),
        int_outputs: outputs.iter().filter(|o| o.param_type == ParamType::Integer).map(|o| o.name.as_str()).collect(),
        helpers,
        real: &real,
        arrays: &arrays,
        reals: &reals,
        undefined: &undefined,
        filled: &filled,
        taken: &writes_of(body).1,
        entering: entering(body, declared),
    };
    walk(body, None, true, &rules)
}

fn written_first(body: &[Statement], v: &str) -> bool {
    let names = |s: &Statement| {
        let mut hit = false;
        each_statement(std::slice::from_ref(s), &mut |x| hit |= exprs_of(x).iter().any(|e| mentions(e, v)));
        hit
    };
    let mut ok = true;
    each_statement(body, &mut |s| {
        let (test, pass, update) = match s {
            Statement::While { condition, body } | Statement::DoWhile { condition, body } => (condition, body, None),
            Statement::For { count, body, .. } => (count, body, None),
            Statement::ForC { condition, body, update, .. } => (condition, body, Some(update.as_ref())),
            _ => return,
        };
        let first = pass.iter().chain(update).find(|s| names(s));
        if mentions(test, v) || first.is_some() {
            ok &= !mentions(test, v) && matches!(first, Some(Statement::Assign { target: Expr::Var(t), value, compound: false }) if t == v && !mentions(value, v));
        }
    });
    ok
}

fn each_statement(body: &[Statement], f: &mut dyn FnMut(&Statement)) {
    for s in body {
        f(s);
        match s {
            Statement::While { body, .. } | Statement::DoWhile { body, .. } | Statement::For { body, .. } | Statement::Block { body } => {
                each_statement(body, f);
            }
            Statement::ForC { init, update, body, .. } => {
                each_statement(std::slice::from_ref(init.as_ref()), f);
                each_statement(std::slice::from_ref(update.as_ref()), f);
                each_statement(body, f);
            }
            Statement::If { then_body, else_body, .. } => {
                each_statement(then_body, f);
                each_statement(else_body, f);
            }
            Statement::Switch { cases, default, .. } => {
                for (_, case) in cases {
                    each_statement(case, f);
                }
                each_statement(default, f);
            }
            _ => {}
        }
    }
}

/// A statement's own expressions, not those of the bodies it holds.
fn exprs_of(s: &Statement) -> Vec<&Expr> {
    match s {
        Statement::Assign { target, value, .. } => vec![target, value],
        Statement::VarDecl { init: Some(e), .. } | Statement::Return { value: Some(e) } | Statement::Expr(e) => vec![e],
        Statement::If { condition, .. } | Statement::While { condition, .. } | Statement::DoWhile { condition, .. } | Statement::ForC { condition, .. } => {
            vec![condition]
        }
        Statement::For { count, .. } => vec![count],
        Statement::Switch { expr, .. } => vec![expr],
        _ => Vec::new(),
    }
}

struct Rules<'a> {
    outputs: Vec<&'a str>,
    int_outputs: Vec<&'a str>,
    helpers: &'a HelperRegistry,
    /// Inferred a double; picks how an integer store is spelled, never whether
    /// a term is safe to run.
    real: &'a dyn Fn(&Expr) -> bool,
    /// Declared real.
    arrays: &'a [String],
    reals: &'a [String],
    /// Locals of any type that a pass may read before anything writes them.
    undefined: &'a [String],
    /// The elements of real local arrays stored before the first loop.
    filled: &'a [(String, i64)],
    /// Address-taken anywhere in the function.
    taken: &'a [String],
    entering: Entering,
}

/// Per loop step, the offsets `o` of the input reads `in[step + o]` that the
/// pass runs on every iteration, as `(step, lowest, highest)`.
type Windows = [(String, i64, i64)];

/// The innermost loop: whether it carries no state, and its read windows.
/// An eager read is admitted only inside a window: the source reads that far
/// on every pass, so the bounds hold on every pass.
struct Pass {
    stateless: bool,
    windows: Vec<(String, i64, i64)>,
}

/// How a loop is entered: the statements before it at the function's top
/// level (`None` when nested), its test, and whether that runs first.
struct Entry<'a> {
    before: Option<&'a [Statement]>,
    test: Option<&'a Expr>,
    tested_first: bool,
}

fn pass_of(pass: &[Statement], counter: Option<&String>, entry: &Entry, rules: &Rules) -> Pass {
    let mut leaves = false;
    each_statement(pass, &mut |s| {
        leaves |= matches!(s, Statement::Break | Statement::Continue | Statement::Return { .. } | Statement::CircBuf(CircBuf::Init { .. }))
            || exprs_of(s).iter().any(|e| may_exit(e, rules.helpers));
    });
    let tested: Vec<Statement> = entry.test.map(|t| Statement::Expr(t.clone())).into_iter().collect();
    let (mut writes, _) = writes_of(&[pass, tested.as_slice()].concat());
    writes.extend(rules.taken.iter().cloned());
    let written = |v: &String| writes.iter().filter(|w| *w == v).count();
    let increment = |s: &Statement| match s {
        Statement::Expr(Expr::PostIncrement(v) | Expr::PreIncrement(v)) => match v.as_ref() {
            Expr::Var(n) => Some(n.clone()),
            _ => None,
        },
        Statement::Comment(_) => None,
        other => step_of(other).map(str::to_string),
    };
    // Only the trailing increments: every read in the pass sees one value.
    let tail = pass.iter().rev().take_while(|s| matches!(s, Statement::Comment(_)) || increment(s).is_some()).count();
    let mut steps: Vec<String> = pass[pass.len() - tail..].iter().filter_map(increment).filter(|v| written(v) == 1).collect();
    steps.extend(counter.filter(|c| written(c) == 0).cloned());
    // Per step, the lowest offset proven at or above index 0 and the highest
    // proven below the inputs' length.
    let mut proof: Vec<(String, Option<i64>, Option<i64>)> = steps.iter().map(|v| (v.clone(), None, None)).collect();
    let mut prove = |v: &str, lo: Option<i64>, hi: Option<i64>| {
        if let Some((_, l, h)) = proof.iter_mut().find(|(w, _, _)| w == v) {
            *l = match (*l, lo) {
                (Some(a), Some(b)) => Some(a.min(b)),
                (a, b) => a.or(b),
            };
            *h = match (*h, hi) {
                (Some(a), Some(b)) => Some(a.max(b)),
                (a, b) => a.or(b),
            };
        }
    };
    if !leaves {
        for s in pass {
            sure_reads(s, rules.helpers, &mut |a, i| {
                if let Some((v, o)) = offset(i).filter(|_| rules.arrays.iter().any(|x| x == a)) {
                    prove(v, Some(o), Some(o));
                }
            });
        }
        let at = &rules.entering;
        for v in &steps {
            // A counted loop starts its counter at the count, whatever came before.
            let init = entry.before.filter(|_| counter != Some(v)).and_then(|b| enters_as_start(v, b));
            let from_start = init.is_some();
            if let (Some(init), true) = (init, at.raised_only) {
                let floor = at.canonical.filter(|(end, _)| init > *end).map_or(0, |(_, fixed)| fixed);
                prove(v, Some(-floor), None);
            }
            let tested = matches!(entry.test, Some(Expr::BinOp(t, BinOp::LessEq, end))
                if matches!(t.as_ref(), Expr::Var(t) if t == v) && matches!(end.as_ref(), Expr::Var(e) if e == "endIdx"));
            let ordered = from_start && at.ordered_from.is_some_and(|o| entry.before.is_some_and(|b| o < b.len()));
            if tested && at.end_fixed && (entry.tested_first || ordered) {
                prove(v, None, Some(0));
            }
        }
    }
    let windows = proof.into_iter().filter_map(|(v, lo, hi)| Some((v, lo?, hi?))).filter(|(_, lo, hi)| lo <= hi).collect();
    Pass { stateless: carries_nothing(pass), windows }
}

/// Where `v` takes `startIdx` before the loop: its last top-level assignment
/// or declaration there, with nothing after it writing `v`.
fn enters_as_start(v: &str, before: &[Statement]) -> Option<usize> {
    let start = |e: &Expr| matches!(e, Expr::Var(n) if n == "startIdx");
    let entry = |s: &Statement| match s {
        Statement::Assign { target: Expr::Var(t), value, compound } if t == v => Some(!compound && start(value)),
        Statement::VarDecl { name, init, .. } if name == v => Some(init.as_ref().is_some_and(start)),
        _ => None,
    };
    let at = before.iter().rposition(|s| entry(s).is_some())?;
    let (later, taken) = writes_of(&before[at + 1..]);
    (entry(&before[at])? && !later.iter().chain(&taken).any(|w| w == v)).then_some(at)
}

/// `v`, `v + c` or `v - c` as `(v, offset)`.
fn offset(i: &Expr) -> Option<(&String, i64)> {
    match i {
        Expr::Var(v) => Some((v, 0)),
        Expr::BinOp(v, op @ (BinOp::Add | BinOp::Sub), c) => match (v.as_ref(), c.as_ref()) {
            (Expr::Var(v), Expr::IntLiteral(c)) => Some((v, if *op == BinOp::Add { *c } else { -*c })),
            _ => None,
        },
        _ => None,
    }
}

/// Whether `e` calls something whose rendering may return early: a backend
/// renders a cross-call's failure, or a buffer allocation's, as a return the
/// IR does not hold.
fn may_exit(e: &Expr, helpers: &HelperRegistry) -> bool {
    let mut hit = false;
    walk_expr(e, &mut |x| {
        if let Expr::FuncCall(n, _) = x {
            let known = PURE_HELPERS.contains(&n.as_str()) || n == DIVIDING_HELPER || helpers.get(n).is_some();
            hit |= !known && MathFn::from_name(n).is_none() && SpecialBuiltin::from_name(n).is_none() && !is_pseudo_call(n);
        }
    });
    hit
}

/// The array reads a top-level statement of the pass runs whatever the data.
/// The backends paste a one-expression helper's body in place of its call, so
/// an argument counts only where that body reads it unconditionally.
fn sure_reads(s: &Statement, helpers: &HelperRegistry, see: &mut dyn FnMut(&str, &Expr)) {
    fn expr(e: &Expr, helpers: &HelperRegistry, see: &mut dyn FnMut(&str, &Expr)) {
        match e {
            Expr::ArrayAccess(a, i) => {
                see(a, i);
                expr(i, helpers, see);
            }
            Expr::BinOp(l, BinOp::And | BinOp::Or, _) | Expr::Ternary(l, _, _) => expr(l, helpers, see),
            Expr::BinOp(l, _, r) => {
                expr(l, helpers, see);
                expr(r, helpers, see);
            }
            // Every range type the settings accept reads the bar, but only
            // where all four prices name one.
            Expr::FuncCall(n, args) if n == "ta_candlerange" => {
                if let Some(i) = args.get(1..).and_then(candle_bar) {
                    see("inClose", i);
                    expr(i, helpers, see);
                }
            }
            Expr::FuncCall(n, args) if MathFn::from_name(n).is_some() => args.iter().for_each(|a| expr(a, helpers, see)),
            Expr::FuncCall(n, args) => {
                if let Some(inlined) = helpers.get(n).and_then(|h| try_inline_expr(h, args)) {
                    expr(&inlined, helpers, see);
                }
            }
            Expr::Cast(_, x)
            | Expr::Not(x)
            | Expr::Neg(x)
            | Expr::BitwiseNot(x)
            | Expr::PostIncrement(x)
            | Expr::PostDecrement(x)
            | Expr::PreIncrement(x)
            | Expr::PreDecrement(x) => expr(x, helpers, see),
            _ => {}
        }
    }
    match s {
        Statement::Assign { target, value, .. } => {
            if let Expr::ArrayAccess(_, i) = target {
                expr(i, helpers, see);
            }
            expr(value, helpers, see);
        }
        Statement::If { condition: e, .. } | Statement::VarDecl { init: Some(e), .. } | Statement::Expr(e) => expr(e, helpers, see),
        Statement::Block { body } => body.iter().for_each(|b| sure_reads(b, helpers, see)),
        _ => {}
    }
}

/// `pass` is `None` outside a loop.
/// `top` while `body` is the function's own top level.
fn walk(body: &[Statement], pass: Option<&Pass>, top: bool, rules: &Rules) -> Vec<Statement> {
    body.iter()
        .enumerate()
        .map(|(at, s)| {
            let before = top.then(|| &body[..at]);
            if let Some(pass) = pass {
                if let Some(folded) = fold(s, pass, rules) {
                    return folded;
                }
            }
            let inner = match s {
                Statement::While { body, condition } => Some(pass_of(body, None, &Entry { before, test: Some(condition), tested_first: true }, rules)),
                Statement::DoWhile { body, condition } => Some(pass_of(body, None, &Entry { before, test: Some(condition), tested_first: false }, rules)),
                Statement::For { var, body, count } => {
                    let before = before.map(|b| [b, &[Statement::Expr(count.clone())]].concat());
                    Some(pass_of(body, Some(var), &Entry { before: before.as_deref(), test: None, tested_first: true }, rules))
                }
                Statement::ForC { init, body, update, condition } => {
                    let pass = [body.as_slice(), std::slice::from_ref(update.as_ref())].concat();
                    let before = before.map(|b| [b, std::slice::from_ref(init.as_ref())].concat());
                    Some(pass_of(&pass, None, &Entry { before: before.as_deref(), test: Some(condition), tested_first: true }, rules))
                }
                _ => None,
            };
            super::ir_cleanup::recurse(s, &|b| walk(b, inner.as_ref().or(pass), false, rules))
        })
        .collect()
}

/// Whether a loop pass carries nothing to the next but its index steps: every
/// other variable it assigns is written on every pass before it is read, no
/// array it stores to is read, and it holds no inner loop or effect.
fn carries_nothing(pass: &[Statement]) -> bool {
    let (mut steps, mut others) = (Vec::new(), Vec::new());
    if !scan(pass, &mut steps, &mut others) {
        return false;
    }
    let state = |n: &str| others.iter().any(|o| o == n);
    let (mut written, mut read) = (Vec::new(), Vec::new());
    ordered(pass, &mut Vec::new(), &state, &mut written, &mut read) && !written.iter().any(|w| read.contains(w))
}

fn incremented(e: &Expr, out: &mut Vec<String>) {
    walk_expr(e, &mut |x| {
        if let Expr::PostIncrement(i) | Expr::PreIncrement(i) | Expr::PostDecrement(i) | Expr::PreDecrement(i) = x {
            if let Expr::Var(n) = i.as_ref() {
                out.push(n.clone());
            }
        }
    });
}

/// The pass's step variables and the other variables it assigns; `false` on a
/// statement no stateless pass holds.
fn scan(pass: &[Statement], steps: &mut Vec<String>, others: &mut Vec<String>) -> bool {
    for s in pass {
        match s {
            Statement::Assign { target, value, .. } => {
                incremented(target, steps);
                incremented(value, steps);
                match target {
                    Expr::Var(t) if step_of(s).is_some() || as_step_down(s) => steps.push(t.clone()),
                    Expr::Var(t) => others.push(t.clone()),
                    Expr::ArrayAccess(..) => {}
                    _ => return false,
                }
            }
            Statement::If { condition, then_body, else_body, .. } => {
                incremented(condition, steps);
                if !scan(then_body, steps, others) || !scan(else_body, steps, others) {
                    return false;
                }
            }
            Statement::Expr(e @ (Expr::PostIncrement(_) | Expr::PreIncrement(_) | Expr::PostDecrement(_) | Expr::PreDecrement(_))) => {
                incremented(e, steps);
            }
            Statement::VarDecl { name, init, .. } => {
                if let Some(e) = init {
                    incremented(e, steps);
                }
                others.push(name.clone());
            }
            Statement::Comment(_) => {}
            _ => return false,
        }
    }
    true
}

fn reads(e: &Expr, carried: &dyn Fn(&str) -> bool) -> bool {
    let mut hit = false;
    walk_expr(e, &mut |x| hit |= matches!(x, Expr::Var(n) if carried(n)));
    hit
}

/// Whether each read of a `state` variable follows a write to it earlier in
/// the same pass.
fn ordered(pass: &[Statement], defined: &mut Vec<String>, state: &dyn Fn(&str) -> bool, written: &mut Vec<String>, read: &mut Vec<String>) -> bool {
    for s in pass {
        let carried = |n: &str| state(n) && !defined.iter().any(|d| d == n);
        match s {
            Statement::Assign { target, value, compound } => {
                walk_expr(value, &mut |x| {
                    if let Expr::ArrayAccess(a, _) = x {
                        read.push(a.clone());
                    }
                });
                let index_reads = match target {
                    Expr::ArrayAccess(a, i) => {
                        written.push(a.clone());
                        reads(i, &carried)
                    }
                    Expr::Var(t) => *compound && carried(t),
                    _ => false,
                };
                if index_reads || reads(value, &carried) {
                    return false;
                }
                if let Expr::Var(t) = target {
                    defined.push(t.clone());
                }
            }
            Statement::If { condition, then_body, else_body, .. } => {
                walk_expr(condition, &mut |x| {
                    if let Expr::ArrayAccess(a, _) = x {
                        read.push(a.clone());
                    }
                });
                if reads(condition, &carried) {
                    return false;
                }
                for arm in [then_body, else_body] {
                    if !ordered(arm, &mut defined.clone(), state, written, read) {
                        return false;
                    }
                }
            }
            Statement::VarDecl { name, init: Some(init), .. } => {
                if reads(init, &carried) {
                    return false;
                }
                defined.push(name.clone());
            }
            _ => {}
        }
    }
    true
}

/// `k -= 1` or `k = k - 1`.
fn as_step_down(stmt: &Statement) -> bool {
    let Statement::Assign { target: Expr::Var(k), value: Expr::BinOp(lhs, BinOp::Sub, rhs), .. } = stmt else { return false };
    matches!(lhs.as_ref(), Expr::Var(n) if n == k) && matches!(rhs.as_ref(), Expr::IntLiteral(1))
}

struct Choice {
    /// `a[k++]` for a stepping store, whatever its spelling.
    target: Expr,
    value: Expr,
    /// Every condition and leaf value, unrewritten.
    operands: Vec<Expr>,
    leaves: Vec<Expr>,
    /// Some condition joins atoms with `&&` / `||`.
    joined: bool,
}

fn fold(s: &Statement, pass: &Pass, rules: &Rules) -> Option<Statement> {
    if pre_check_end(s).is_none() {
        return fold_tree(s, pass.stateless, &pass.windows, rules);
    }
    fold_pre_check(s, pass, rules)
}

fn fold_pre_check(s: &Statement, pass: &Pass, rules: &Rules) -> Option<Statement> {
    let Statement::If { condition, then_body, else_body, cond_comments } = s else { return None };
    let [k] = code(then_body)[..] else { return None };
    let ends_here = same(final_else(&then_body[k])?, else_body);
    let mut then_body = then_body.clone();
    // The decision is the pre-check's conjunction with the inner tree's, so no
    // one-atom domain check.
    then_body[k] = fold_tree(&then_body[k], true, &pass.windows, rules)?;
    let mut tests = Vec::new();
    conjuncts(condition, &mut tests);
    let eager_ok = tests.iter().all(|t| eager_safe(t, &pass.windows, rules));
    let mut counted = Vec::new();
    if eager_ok {
        let mut terms = Vec::new();
        conjuncts(&annotated(condition, cond_comments)?, &mut terms);
        counted = terms.iter().map(|t| eager(&simplify(t, rules.helpers))).collect();
        if let [one] = counted.as_slice() {
            counted = atoms_of(one);
        }
    }
    let (condition, cond_comments) = if counted.len() < 2 {
        (condition.clone(), cond_comments.clone())
    } else {
        (Expr::FuncCall(EAGER_ALL.into(), counted), Vec::new())
    };
    let else_body = if ends_here {
        walk(else_body, Some(pass), false, rules)
    } else {
        let [j] = code(else_body)[..] else { return None };
        let mut next = else_body.clone();
        next[j] = fold_pre_check(&else_body[j], pass, rules)?;
        next
    };
    Some(Statement::If { condition, then_body, else_body, cond_comments })
}

fn code(arm: &[Statement]) -> Vec<usize> {
    (0..arm.len()).filter(|&k| !matches!(arm[k], Statement::Comment(_))).collect()
}

fn same(a: &[Statement], b: &[Statement]) -> bool {
    let text = |arm: &[Statement]| code(arm).iter().map(|&k| format!("{:?}", arm[k])).collect::<Vec<_>>();
    text(a) == text(b)
}

/// The `else` that ends `s`'s `else if` chain.
fn final_else(s: &Statement) -> Option<&[Statement]> {
    let mut chain = s;
    loop {
        let Statement::If { else_body: last, .. } = chain else { return None };
        match code(last)[..] {
            [j] if matches!(last[j], Statement::If { .. }) => chain = &last[j],
            _ => return Some(last),
        }
    }
}

/// A pre-check of two links or more.
pub(crate) fn is_pre_check_chain(s: &Statement) -> bool {
    let Statement::If { else_body, .. } = s else { return false };
    pre_check_end(s).is_some_and(|end| !same(end, else_body))
}

/// The common final `else` when `s` is a pre-check: an `if` whose then arm is
/// only an `if` chain ending in that `else`, and whose own `else` is that
/// `else` or another pre-check ending in it: each test then only splits a
/// conjunction.
pub(crate) fn pre_check_end(s: &Statement) -> Option<&[Statement]> {
    let Statement::If { then_body, else_body, .. } = s else { return None };
    let [k] = code(then_body)[..] else { return None };
    let end = final_else(&then_body[k])?;
    if same(end, else_body) {
        return Some(end);
    }
    let [j] = code(else_body)[..] else { return None };
    pre_check_end(&else_body[j]).filter(|next| same(next, end)).map(|_| end)
}

/// A lone eager conjunct's own `&` operands, its comment on the first, so a
/// one-test pre-check that expands into several is still counted.
fn atoms_of(e: &Expr) -> Vec<Expr> {
    let (inner, notes) = peel(e);
    let Expr::FuncCall(n, args) = inner else { return vec![e.clone()] };
    if n != EAGER_AND {
        return vec![e.clone()];
    }
    let mut out = args.clone();
    for (comment, leads) in notes.into_iter().rev() {
        let kind = if leads { LEAD } else { NOTE };
        out[0] = Expr::FuncCall(format!("{kind}{comment}"), vec![out[0].clone()]);
    }
    out
}

/// The top-level `&&` operands of `e`, in source order.
fn conjuncts(e: &Expr, out: &mut Vec<Expr>) {
    match e {
        Expr::BinOp(l, BinOp::And, r) => {
            conjuncts(l, out);
            conjuncts(r, out);
        }
        other => out.push(other.clone()),
    }
}

fn fold_tree(s: &Statement, stateless: bool, windows: &Windows, rules: &Rules) -> Option<Statement> {
    if !matches!(s, Statement::If { .. }) {
        return None;
    }
    let c = choice(std::slice::from_ref(s), rules)?;
    if !c.joined && !stateless {
        return None;
    }
    let Expr::ArrayAccess(array, idx) = &c.target else { return None };
    // C leaves the step unsequenced against any other read of it.
    if let Expr::PostIncrement(k) = idx.as_ref() {
        if let Expr::Var(k) = k.as_ref() {
            if c.operands.iter().any(|e| mentions(e, k)) {
                return None;
            }
        }
    }
    if !c.operands.iter().all(|e| eager_safe(e, windows, rules)) {
        return None;
    }
    let mut value = c.value;
    if rules.int_outputs.contains(&array.as_str()) && !c.leaves.iter().any(|v| (rules.real)(v)) {
        let plain = |e: &Expr| plain_value(e, rules.helpers);
        value = rewrite_expr(&value, &|x| match x {
            Expr::FuncCall(n, a) if n == SELECT && a[1..].iter().all(|v| is_branch(v) || plain(v)) => Expr::FuncCall(SELECT_INT.into(), a),
            Expr::FuncCall(n, a) if n == SELECT => Expr::FuncCall(GUARD.into(), a),
            other => other,
        });
    }
    Some(Statement::Assign { target: c.target, value, compound: false })
}

fn is_branch(e: &Expr) -> bool {
    matches!(peel(e).0, Expr::FuncCall(n, _) if is_select(n) || n == GUARD)
}

/// An integer arm as cheap to pick by mask as a constant: a constant, a name,
/// a choice between two constants, or a constant times a colour sign, each
/// read at plain indices. Anything else, a data-dependent load above all, is
/// picked only on the bars that match.
fn plain_value(e: &Expr, helpers: &HelperRegistry) -> bool {
    let direct = |e: &Expr| {
        let mut direct = true;
        walk_expr(e, &mut |x| {
            if let Expr::ArrayAccess(_, i) = x {
                walk_expr(i, &mut |y| direct &= matches!(y, Expr::Var(_) | Expr::IntLiteral(_) | Expr::BinOp(_, BinOp::Add | BinOp::Sub, _)));
            }
        });
        direct
    };
    let plain_sign = |e: &Expr| flag(e, helpers).is_some_and(|(p, _, _)| direct(&p));
    let constant = |e: &Expr| int_value(e).is_some() || matches!(e, Expr::Literal(_));
    match peel(e).0 {
        Expr::IntLiteral(_) | Expr::Literal(_) | Expr::Var(_) => true,
        Expr::Neg(x) => plain_value(x, helpers),
        Expr::BinOp(x, BinOp::Mul, y) => (constant(x) && plain_sign(y)) || (plain_sign(x) && constant(y)),
        Expr::Ternary(c, t, f) if constant(t) && constant(f) => direct(c),
        other => plain_sign(other),
    }
}

fn choice(arm: &[Statement], rules: &Rules) -> Option<Choice> {
    let (mut before, mut after, mut code) = (Vec::new(), Vec::new(), Vec::new());
    for s in arm {
        match s {
            Statement::Comment(lines) if code.is_empty() => before.extend(lines.iter().cloned()),
            Statement::Comment(lines) => after.extend(lines.iter().cloned()),
            other => code.push(other),
        }
    }
    let mut c = match code.as_slice() {
        [Statement::Assign { target: Expr::ArrayAccess(a, i), value, compound: false }] => {
            let stepped = match i.as_ref() {
                Expr::PostIncrement(k) => match k.as_ref() {
                    Expr::Var(k) => Some(k.as_str()),
                    _ => return None,
                },
                i if pure(i, rules) => None,
                _ => return None,
            };
            if stepped.is_some_and(|k| mentions(value, k)) {
                return None;
            }
            leaf(a, i.as_ref().clone(), value, rules)?
        }
        [Statement::Assign { target: Expr::ArrayAccess(a, i), value, compound: false }, step] => {
            let k = step_of(step)?;
            if !matches!(i.as_ref(), Expr::Var(v) if v == k) || mentions(value, k) {
                return None;
            }
            leaf(a, Expr::PostIncrement(Box::new(Expr::Var(k.to_string()))), value, rules)?
        }
        [Statement::If { condition, then_body, else_body, cond_comments }] => {
            let yes = choice(then_body, rules)?;
            let no = choice(else_body, rules)?;
            if yes.target != no.target || !pure(condition, rules) {
                return None;
            }
            let condition = annotated(condition, cond_comments)?;
            let mut operands = vec![condition.clone()];
            operands.extend(yes.operands);
            operands.extend(no.operands);
            let mut leaves = yes.leaves;
            leaves.extend(no.leaves);
            let joined = yes.joined || no.joined || matches!(condition, Expr::BinOp(_, BinOp::And | BinOp::Or, _));
            Choice {
                target: yes.target,
                value: Expr::FuncCall(SELECT.into(), vec![eager(&simplify(&condition, rules.helpers)), yes.value, no.value]),
                operands,
                leaves,
                joined,
            }
        }
        _ => return None,
    };
    for (lines, kind) in [(after, NOTE), (before, LEAD)] {
        let text = lines.iter().map(|l| l.trim()).filter(|l| !l.is_empty()).collect::<Vec<_>>().join(" ");
        if !text.is_empty() {
            c.value = Expr::FuncCall(format!("{kind}{text}"), vec![c.value]);
        }
    }
    Some(c)
}

fn leaf(array: &str, idx: Expr, value: &Expr, rules: &Rules) -> Option<Choice> {
    if !rules.outputs.contains(&array) || !pure(value, rules) {
        return None;
    }
    Some(Choice {
        target: Expr::ArrayAccess(array.to_string(), Box::new(idx)),
        value: value.clone(),
        operands: vec![value.clone()],
        leaves: vec![value.clone()],
        joined: false,
    })
}

/// `k += 1` or `k = k + 1`.
fn step_of(stmt: &Statement) -> Option<&str> {
    let Statement::Assign { target: Expr::Var(k), value: Expr::BinOp(lhs, BinOp::Add, rhs), .. } = stmt else { return None };
    (matches!(lhs.as_ref(), Expr::Var(n) if n == k) && matches!(rhs.as_ref(), Expr::IntLiteral(1))).then_some(k.as_str())
}

fn pure(e: &Expr, rules: &Rules) -> bool {
    let mut ok = true;
    walk_expr(e, &mut |x| {
        ok &= match x {
            // An integer division may trap where its guard kept it from running.
            Expr::BinOp(_, BinOp::Mod, _)
            | Expr::PostIncrement(_)
            | Expr::PostDecrement(_)
            | Expr::PreIncrement(_)
            | Expr::PreDecrement(_)
            | Expr::AddressOf(_) => false,
            Expr::BinOp(l, BinOp::Div, r) => (rules.real)(l) || (rules.real)(r),
            Expr::FuncCall(n, _) if n == DIVIDING_HELPER => true,
            Expr::FuncCall(n, _) => {
                PURE_HELPERS.contains(&n.as_str())
                    || matches!(MathFn::from_name(n), Some(MathFn::Abs | MathFn::Max | MathFn::Min))
            }
            _ => true,
        };
    });
    ok
}

/// Whether running `e` on every pass can neither trap nor act. An allowlist:
/// literals; names declared once, real, and written before a pass reads them;
/// input-array reads at a loop step plus or minus a literal inside the step's
/// proven window (or a choice between two such); real local arrays at a
/// literal stored before the first loop; the pure candle helpers, which take
/// any name but an undefined local as an argument; `fabs`, `max` and `min`;
/// `+ - * /` with an operand declared real, so the arithmetic is floating;
/// comparisons, `!`, `&&` and `||`; a choice between two literals; and a
/// literal times a 1/-1 or 1/0 flag. Everything else is refused, integer
/// arithmetic above all: a guard on it may be what keeps it in range.
fn eager_safe(e: &Expr, windows: &Windows, rules: &Rules) -> bool {
    let safe = |x: &Expr| eager_safe(x, windows, rules);
    let real = |x: &Expr| declared_real(x, rules);
    match e {
        Expr::Literal(_) | Expr::IntLiteral(_) => true,
        Expr::Var(n) => rules.reals.contains(n),
        Expr::ArrayAccess(a, i) => (rules.arrays.contains(a) && in_window(i, windows, rules)) || filled(a, i, rules),
        Expr::BinOp(l, op, r) => match op {
            BinOp::And | BinOp::Or | BinOp::Less | BinOp::LessEq | BinOp::Greater | BinOp::GreaterEq | BinOp::Eq | BinOp::NotEq => {
                safe(l) && safe(r)
            }
            BinOp::Add | BinOp::Sub | BinOp::Div => safe(l) && safe(r) && (real(l) || real(r)),
            BinOp::Mul => safe(l) && safe(r) && (real(l) || real(r) || scaled_flag(l, r, rules) || scaled_flag(r, l, rules)),
            _ => false,
        },
        Expr::Not(x) | Expr::Neg(x) => safe(x),
        Expr::Ternary(c, t, f) => safe(c) && literal(t) && literal(f),
        Expr::FuncCall(n, args) => {
            if note_of(n).is_some() {
                args.iter().all(safe)
            } else if PURE_HELPERS.contains(&n.as_str()) || n == DIVIDING_HELPER {
                args.iter().all(|a| safe(a) || matches!(a, Expr::Var(n) if !rules.undefined.contains(n)))
            } else {
                matches!(MathFn::from_name(n), Some(MathFn::Abs | MathFn::Max | MathFn::Min)) && args.iter().all(safe)
            }
        }
        _ => false,
    }
}

/// A double by declaration.
fn declared_real(e: &Expr, rules: &Rules) -> bool {
    let real = |x: &Expr| declared_real(x, rules);
    match e {
        Expr::Literal(_) => true,
        Expr::Var(n) => rules.reals.contains(n),
        Expr::ArrayAccess(a, _) => rules.arrays.contains(a) || rules.filled.iter().any(|(n, _)| n == a),
        Expr::BinOp(l, BinOp::Add | BinOp::Sub | BinOp::Mul | BinOp::Div, r) => real(l) || real(r),
        Expr::Neg(x) => real(x),
        Expr::FuncCall(n, args) if note_of(n).is_some() => args.iter().all(real),
        Expr::FuncCall(n, args) => match MathFn::from_name(n) {
            Some(MathFn::Abs) => true,
            Some(MathFn::Max | MathFn::Min) => args.iter().any(real),
            _ => n == DIVIDING_HELPER || rules.helpers.get(n).is_some_and(|h| h.return_type == VarType::Real),
        },
        _ => false,
    }
}

fn literal(e: &Expr) -> bool {
    int_value(e).is_some() || matches!(e, Expr::Literal(_))
}

/// `k * f`, `k` an integer literal and `f` a flag between 1 and -1, or 1 and
/// 0: no product overflows that `k` itself does not.
fn scaled_flag(k: &Expr, f: &Expr, rules: &Rules) -> bool {
    int_value(k).is_some() && flag(f, rules.helpers).is_some_and(|(_, t, f)| matches!((t, f), (1, -1 | 0) | (-1 | 0, 1)))
}

/// A real local array read at a literal stored before the first loop.
fn filled(a: &str, i: &Expr, rules: &Rules) -> bool {
    matches!(i, Expr::IntLiteral(k) if rules.filled.iter().any(|(n, f)| n == a && f == k))
}

/// A step plus or minus a literal inside the step's window, or a choice
/// between two of them on a safe condition.
fn in_window(i: &Expr, windows: &Windows, rules: &Rules) -> bool {
    if let Expr::Ternary(c, t, f) = i {
        return eager_safe(c, windows, rules) && in_window(t, windows, rules) && in_window(f, windows, rules);
    }
    offset(i).is_some_and(|(v, o)| windows.iter().any(|(w, lo, hi)| w == v && (*lo..=*hi).contains(&o)))
}

fn mentions(e: &Expr, name: &str) -> bool {
    let mut hit = false;
    walk_expr(e, &mut |x| hit |= matches!(x, Expr::Var(n) | Expr::ArrayAccess(n, _) if n == name));
    hit
}

/// `condition` with each commented `&&` / `||` leaf in a note; `None` when
/// the comments do not pair with the leaves.
fn annotated(condition: &Expr, comments: &[Option<Vec<String>>]) -> Option<Expr> {
    fn go(e: &Expr, comments: &mut std::slice::Iter<Option<Vec<String>>>) -> Option<Expr> {
        if let Expr::BinOp(l, op @ (BinOp::And | BinOp::Or), r) = e {
            return Some(Expr::BinOp(Box::new(go(l, comments)?), op.clone(), Box::new(go(r, comments)?)));
        }
        let text = comments.next()?.iter().flatten().filter(|l| !l.is_empty()).cloned().collect::<Vec<_>>().join(" ");
        Some(if text.is_empty() { e.clone() } else { Expr::FuncCall(format!("{NOTE}{text}"), vec![e.clone()]) })
    }
    if comments.is_empty() {
        return Some(condition.clone());
    }
    let mut it = comments.iter();
    let out = go(condition, &mut it)?;
    it.next().is_none().then_some(out)
}

/// `cond` with each `&&` / `||` atom in its cheapest exact boolean form.
fn simplify(cond: &Expr, helpers: &HelperRegistry) -> Expr {
    match cond {
        Expr::BinOp(l, op @ (BinOp::And | BinOp::Or), r) => {
            Expr::BinOp(Box::new(simplify(l, helpers)), op.clone(), Box::new(simplify(r, helpers)))
        }
        Expr::FuncCall(n, a) if note_of(n).is_some() && a.len() == 1 => Expr::FuncCall(n.clone(), vec![simplify(&a[0], helpers)]),
        Expr::Not(inner) => Expr::Not(Box::new(simplify(inner, helpers))),
        atom => {
            let t = truth(atom, helpers);
            match t {
                Expr::Not(_) => t,
                t => signed(&t, helpers).unwrap_or_else(|| expand(&t)),
            }
        }
    }
}

fn int_value(e: &Expr) -> Option<i64> {
    match e {
        Expr::IntLiteral(v) => Some(*v),
        Expr::Neg(i) => int_value(i).map(|v| -v),
        _ => None,
    }
}

/// `e` as `P ? k1 : k2` with two distinct integer constants and a 0/1 `P`,
/// seen through a negation or a pure helper's one-expression body.
fn flag(e: &Expr, helpers: &HelperRegistry) -> Option<(Expr, i64, i64)> {
    match e {
        Expr::Ternary(p, t, f) => {
            let (t, f) = (int_value(t)?, int_value(f)?);
            (t != f && is_boolean(p)).then(|| ((**p).clone(), t, f))
        }
        Expr::Neg(i) => flag(i, helpers).map(|(p, t, f)| (p, -t, -f)),
        Expr::FuncCall(n, args) if PURE_HELPERS.contains(&n.as_str()) => match try_inline_expr(helpers.get(n)?, args)? {
            body @ Expr::Ternary(..) => flag(&body, helpers),
            _ => None,
        },
        _ => None,
    }
}

fn not(p: Expr) -> Expr {
    Expr::Not(Box::new(p))
}

/// `a * S op b * S`, `S` a ±1 flag on `P`, as `(P & (a op b)) | (!P & (b op a))`
/// for `S = 1` on `P`: exact, since a product by ±1 is, and a NaN fails both.
fn signed(atom: &Expr, helpers: &HelperRegistry) -> Option<Expr> {
    let Expr::BinOp(lhs, op @ (BinOp::Less | BinOp::LessEq | BinOp::Greater | BinOp::GreaterEq), rhs) = atom else { return None };
    let scaled = |e: &Expr| -> Option<(Expr, (Expr, i64, i64))> {
        let Expr::BinOp(x, BinOp::Mul, y) = e else { return None };
        let unit = |f: &(Expr, i64, i64)| matches!((f.1, f.2), (1, -1) | (-1, 1));
        if let Some(f) = flag(y, helpers).filter(unit) {
            return Some(((**x).clone(), f));
        }
        flag(x, helpers).filter(unit).map(|f| ((**y).clone(), f))
    };
    let (left, (pred, on, _)) = scaled(lhs)?;
    let (right, (other, other_on, _)) = scaled(rhs)?;
    if format!("{pred:?}") != format!("{other:?}") || on != other_on {
        return None;
    }
    let (up, down) = if on == 1 { (pred.clone(), not(pred)) } else { (not(pred.clone()), pred) };
    let and = |x: Expr, y: Expr| Expr::BinOp(Box::new(x), BinOp::And, Box::new(y));
    let forward = expand(&Expr::BinOp(Box::new(left.clone()), op.clone(), Box::new(right.clone())));
    let mirrored = expand(&Expr::BinOp(Box::new(right), op.clone(), Box::new(left)));
    Some(Expr::BinOp(Box::new(and(up, forward)), BinOp::Or, Box::new(and(down, mirrored))))
}

/// A truth-valued `atom` as its flag's predicate where that is exact.
fn truth(atom: &Expr, helpers: &HelperRegistry) -> Expr {
    if let Some((p, t, f)) = flag(atom, helpers) {
        if f == 0 {
            return p;
        }
        if t == 0 {
            return not(p);
        }
        return atom.clone();
    }
    let Expr::BinOp(lhs, op @ (BinOp::Eq | BinOp::NotEq), rhs) = atom else { return atom.clone() };
    let is_eq = *op == BinOp::Eq;
    let one = |pred: Expr, same: bool| if same == is_eq { pred } else { not(pred) };
    let pair = |left: Expr, right: Expr, same: bool| {
        let op = if same == is_eq { BinOp::Eq } else { BinOp::NotEq };
        Expr::BinOp(Box::new(left), op, Box::new(right))
    };
    match (flag(lhs, helpers), flag(rhs, helpers), int_value(lhs), int_value(rhs)) {
        (Some((pred, on, off)), None, _, Some(k)) | (None, Some((pred, on, off)), Some(k), _) if k == on || k == off => {
            one(pred, k == on)
        }
        (Some((left, on, off)), Some((right, r_on, r_off)), ..) if (on, off) == (r_on, r_off) || (on, off) == (r_off, r_on) => {
            pair(left, right, (on, off) == (r_on, r_off))
        }
        _ => atom.clone(),
    }
}

/// The two arguments of a `min` / `max`, and whether it is `max`.
fn min_max(e: &Expr) -> Option<(bool, &Expr, &Expr)> {
    let Expr::FuncCall(n, args) = e else { return None };
    let [a, b] = args.as_slice() else { return None };
    match MathFn::from_name(n)? {
        MathFn::Max => Some((true, a, b)),
        MathFn::Min => Some((false, a, b)),
        _ => None,
    }
}

fn spread(e: &Expr) -> usize {
    min_max(e).map_or(1, |(_, a, b)| spread(a) + spread(b))
}

/// The most atoms one comparison may expand into.
const MAX_SPREAD: usize = 8;

/// An ordering comparison with `min` / `max` as the `&` / `|` of plain ones.
fn expand(e: &Expr) -> Expr {
    let Expr::BinOp(l, op @ (BinOp::Less | BinOp::LessEq | BinOp::Greater | BinOp::GreaterEq), r) = e else { return e.clone() };
    let n = spread(l) * spread(r);
    if n == 1 || n > MAX_SPREAD {
        return e.clone();
    }
    split(l, op, r)
}

fn split(l: &Expr, op: &BinOp, r: &Expr) -> Expr {
    let rising = matches!(op, BinOp::Greater | BinOp::GreaterEq);
    let join = |is_max: bool, rises: bool, x: Expr, y: Expr| {
        let j = if is_max == rises { BinOp::Or } else { BinOp::And };
        Expr::BinOp(Box::new(x), j, Box::new(y))
    };
    if let Some((is_max, a, b)) = min_max(l) {
        return join(is_max, rising, split(a, op, r), split(b, op, r));
    }
    if let Some((is_max, a, b)) = min_max(r) {
        return join(is_max, !rising, split(l, op, a), split(l, op, b));
    }
    Expr::BinOp(Box::new(l.clone()), op.clone(), Box::new(r.clone()))
}

/// Every `&&` / `||` as its non-short-circuit call, a chain flattened into one.
fn eager(cond: &Expr) -> Expr {
    rewrite_expr(cond, &|node| match node {
        Expr::BinOp(lhs, op @ (BinOp::And | BinOp::Or), rhs) => {
            let name = if op == BinOp::And { EAGER_AND } else { EAGER_OR };
            let mut args = Vec::new();
            for side in [*lhs, *rhs] {
                match side {
                    Expr::FuncCall(n, a) if n == name => args.extend(a),
                    other => args.push(other),
                }
            }
            Expr::FuncCall(name.into(), args)
        }
        other => other,
    })
}

#[cfg(test)]
mod tests {
    use super::*;
    use crate::ir::ParamType;

    macro_rules! same {
        ($a:expr, $b:expr) => {
            assert_eq!(format!("{:?}", $a), format!("{:?}", $b))
        };
    }

    fn var(n: &str) -> Expr {
        Expr::Var(n.into())
    }
    fn int(v: i64) -> Expr {
        Expr::IntLiteral(v)
    }
    fn bin(l: Expr, op: BinOp, r: Expr) -> Expr {
        Expr::BinOp(Box::new(l), op, Box::new(r))
    }
    fn at(a: &str, i: Expr) -> Expr {
        Expr::ArrayAccess(a.into(), Box::new(i))
    }
    fn i_minus(k: i64) -> Expr {
        bin(var("i"), BinOp::Sub, int(k))
    }
    fn gt(a: Expr, b: Expr) -> Expr {
        bin(a, BinOp::Greater, b)
    }
    fn call(n: &str, args: Vec<Expr>) -> Expr {
        Expr::FuncCall(n.into(), args)
    }
    fn stepped() -> Expr {
        at("out", Expr::PostIncrement(Box::new(var("k"))))
    }
    fn store(t: Expr, v: Expr) -> Statement {
        Statement::Assign { target: t, value: v, compound: false }
    }
    fn iff(c: Expr, t: Vec<Statement>, e: Vec<Statement>) -> Statement {
        Statement::If { condition: c, then_body: t, else_body: e, cond_comments: Vec::new() }
    }
    /// The input arrays and real scalars the tests declare.
    fn declared() -> &'static Declared {
        static D: std::sync::OnceLock<Declared> = std::sync::OnceLock::new();
        D.get_or_init(|| Declared {
            inputs: names(&["inClose", "inOpen", "inHigh", "inLow"]),
            arrays: names(&["inClose", "inOpen", "inHigh", "inLow"]),
            reals: names(&["x", "y", "a", "b", "c", "d", "o", "h", "l", "f", "s"]),
            params: names(&["x", "y", "a", "b", "c", "d", "o", "h", "l", "f", "s", "p"]),
            ..Declared::default()
        })
    }
    /// Every pass reads `inClose` from `i - 4` to `i`, so reads in that window
    /// may run eagerly.
    fn reads() -> Statement {
        store(var("w"), bin(at("inClose", i_minus(4)), BinOp::Add, at("inClose", var("i"))))
    }
    fn step() -> Statement {
        Statement::Expr(Expr::PostIncrement(Box::new(var("i"))))
    }
    fn in_loop(s: Statement) -> Vec<Statement> {
        vec![Statement::While { condition: gt(var("n"), int(0)), body: vec![s, reads(), step()] }]
    }
    /// `x` and `y` are doubles.
    fn real(e: &Expr) -> bool {
        matches!(e, Expr::Literal(_)) || matches!(e, Expr::Var(v) if v == "x" || v == "y")
    }
    fn names(n: &[&str]) -> Vec<String> {
        n.iter().map(|s| (*s).to_string()).collect()
    }
    fn helpers() -> &'static HelperRegistry {
        static H: std::sync::OnceLock<HelperRegistry> = std::sync::OnceLock::new();
        H.get_or_init(|| HelperRegistry::from_dir(&std::path::Path::new(env!("CARGO_MANIFEST_DIR")).join("../input")))
    }
    fn rules() -> Rules<'static> {
        Rules { outputs: vec!["out"], int_outputs: Vec::new(), helpers: helpers(), real: &real, arrays: &declared().arrays, reals: &declared().reals, undefined: &[], filled: &[], taken: &[], entering: Entering::default() }
    }
    fn run(body: &[Statement]) -> Vec<Statement> {
        walk(body, None, true, &rules())
    }
    fn folded_by(s: Statement, rules: &Rules) -> Statement {
        let out = walk(&in_loop(s), None, true, rules);
        let [Statement::While { body, .. }] = out.as_slice() else { panic!("loop gone") };
        body[0].clone()
    }
    /// The single statement of the loop body after the pass.
    fn folded(s: Statement) -> Statement {
        folded_by(s, &rules())
    }
    fn is_fold(s: &Statement) -> bool {
        matches!(s, Statement::Assign { value: Expr::FuncCall(n, _), .. } if is_select(n))
    }
    fn a_gt_b() -> Expr {
        gt(at("inClose", i_minus(1)), at("inOpen", i_minus(2)))
    }
    fn c_lt_d() -> Expr {
        bin(at("inClose", var("i")), BinOp::Less, at("inClose", i_minus(1)))
    }
    fn simple_if(cond: Expr, yes: Expr) -> Statement {
        iff(cond, vec![store(stepped(), yes)], vec![store(stepped(), int(0))])
    }

    #[test]
    fn post_increment_store_folds_with_eager_condition() {
        let cond = bin(bin(a_gt_b(), BinOp::And, c_lt_d()), BinOp::Or, gt(var("x"), int(0)));
        let got = folded(simple_if(cond, int(100)));
        let want = store(
            stepped(),
            call(
                SELECT,
                vec![call(EAGER_OR, vec![call(EAGER_AND, vec![a_gt_b(), c_lt_d()]), gt(var("x"), int(0))]), int(100), int(0)],
            ),
        );
        same!(got, want);
    }

    #[test]
    fn store_then_step_folds_to_one_stepping_store() {
        let leaf = |v: Expr| {
            vec![
                store(at("out", var("k")), v),
                store(var("k"), bin(var("k"), BinOp::Add, int(1))),
            ]
        };
        let got = folded(iff(a_gt_b(), leaf(int(1)), leaf(int(2))));
        same!(got, store(stepped(), call(SELECT, vec![a_gt_b(), int(1), int(2)])));
    }

    #[test]
    fn plain_index_store_folds() {
        let t = at("out", var("i"));
        let s = iff(a_gt_b(), vec![store(t.clone(), int(3))], vec![store(t.clone(), int(0))]);
        same!(folded(s), store(t, call(SELECT, vec![a_gt_b(), int(3), int(0)])));
    }

    #[test]
    fn a_directly_nested_decision_keeps_its_outer_test_a_branch() {
        let x = call("ta_candlecolor", vec![at("inClose", i_minus(1)), at("inOpen", i_minus(1))]);
        let inner = iff(c_lt_d(), vec![store(stepped(), x.clone())], vec![simple_if(gt(var("y"), int(0)), int(80))]);
        let s = iff(a_gt_b(), vec![Statement::Comment(vec!["pre".into()]), inner], vec![store(stepped(), int(0))]);
        let tree = call(SELECT, vec![c_lt_d(), x, call(SELECT, vec![gt(var("y"), int(0)), int(80), int(0)])]);
        let want = Statement::If {
            condition: a_gt_b(),
            then_body: vec![Statement::Comment(vec!["pre".into()]), store(stepped(), tree)],
            else_body: vec![store(stepped(), int(0))],
            cond_comments: Vec::new(),
        };
        same!(folded(s), want);
    }

    #[test]
    fn each_link_of_an_else_if_chain_of_pre_checks_keeps_its_test_a_branch() {
        let zero = || vec![store(stepped(), int(0))];
        let link = |lead: Expr, inner: Expr, v: i64, rest: Vec<Statement>| iff(lead, vec![simple_if(inner, int(v))], rest);
        let y = || gt(var("y"), int(0));
        let s = link(a_gt_b(), y(), 100, vec![link(c_lt_d(), y(), -100, zero())]);
        let kept = |lead: Expr, v: i64, rest: Vec<Statement>| Statement::If {
            condition: lead,
            then_body: vec![store(stepped(), call(SELECT, vec![y(), int(v), int(0)]))],
            else_body: rest,
            cond_comments: Vec::new(),
        };
        same!(folded(s), kept(a_gt_b(), 100, vec![kept(c_lt_d(), -100, zero())]));
        let other_end = link(a_gt_b(), y(), 100, vec![link(c_lt_d(), y(), -100, vec![store(stepped(), int(7))])]);
        assert!(is_fold(&folded(other_end)), "a link ending elsewhere is a choice, folded whole");
    }

    #[test]
    fn a_step_written_again_or_ahead_of_a_read_indexes_nothing() {
        let looped = |pass: Vec<Statement>| {
            let out = run(&[Statement::While { condition: gt(var("n"), int(0)), body: pass }]);
            let [Statement::While { body, .. }] = out.as_slice() else { panic!() };
            body[0].clone()
        };
        let tree = || simple_if(bin(a_gt_b(), BinOp::And, c_lt_d()), int(1));
        assert!(is_fold(&looped(vec![tree(), reads(), step()])));
        let rewind = iff(gt(var("x"), int(0)), vec![store(var("i"), int(0))], Vec::new());
        assert!(!is_fold(&looped(vec![tree(), reads(), rewind, step()])));
        assert!(!is_fold(&looped(vec![tree(), step(), reads()])));
        let counted = |body: Vec<Statement>| {
            let out = run(&[Statement::For { var: "i".into(), count: var("n"), body }]);
            let [Statement::For { body, .. }] = out.as_slice() else { panic!() };
            body[0].clone()
        };
        assert!(is_fold(&counted(vec![tree(), reads()])));
        assert!(!is_fold(&counted(vec![tree(), reads(), store(var("i"), int(0))])));
    }

    #[test]
    fn the_eager_allowlist_admits_only_declared_reals_and_step_reads() {
        let (arrays, reals) = (names(&["inClose", "inOpen", "inHigh"]), names(&["x", "y"]));
        let filled = [("tot".to_string(), 1)];
        let mut r = rules();
        (r.arrays, r.reals, r.filled) = (&arrays, &reals, &filled);
        let windows = [("i".to_string(), -4, 0)];
        let ok = |e: Expr| eager_safe(&e, &windows, &r);
        let nonzero = |n: &str| bin(var(n), BinOp::NotEq, int(0));
        assert!(ok(gt(at("inClose", i_minus(1)), var("x"))));
        assert!(ok(gt(bin(at("inHigh", var("i")), BinOp::Div, int(2)), at("tot", int(1)))));
        assert!(ok(gt(call("fabs", vec![bin(at("inClose", var("i")), BinOp::Sub, var("y"))]), int(0))));
        assert!(ok(gt(bin(color(1), BinOp::Mul, int(100)), int(0))));
        let big = Expr::Ternary(Box::new(gt(int(1), var("x"))), Box::new(int(2_000_000_000)), Box::new(int(0)));
        for refused in [
            gt(at("inClose", i_minus(5)), var("x")),
            gt(at("inClose", bin(var("i"), BinOp::Add, int(1))), var("x")),
            gt(bin(int(2), BinOp::Mul, big), int(5)),
            gt(var("i"), int(3)),
            nonzero("p"),
            gt(Expr::PointerDeref("p".into()), var("x")),
            nonzero("tmp"),
            gt(at("inClose", var("m")), var("x")),
            gt(at("inClose", Expr::Cast(VarType::Integer, Box::new(at("inHigh", var("i"))))), var("x")),
            gt(at("tot", int(0)), var("x")),
            gt(bin(var("i"), BinOp::Div, var("sumN")), int(1)),
            gt(bin(var("i"), BinOp::Mul, var("n")), int(5)),
            gt(call("external_effect", vec![var("x")]), var("y")),
            gt(Expr::PostIncrement(Box::new(var("j"))), int(0)),
        ] {
            assert!(!ok(refused.clone()), "{refused:?}");
        }
    }

    #[test]
    fn below_the_outer_test_a_nested_tree_folds_whole() {
        let second = iff(c_lt_d(), vec![simple_if(gt(var("y"), int(0)), int(100))], vec![store(stepped(), int(0))]);
        let s = iff(a_gt_b(), vec![second], vec![store(stepped(), int(0))]);
        let Statement::If { then_body, .. } = folded(s) else { panic!("the outer test stays a branch") };
        let tree = call(SELECT, vec![c_lt_d(), call(SELECT, vec![gt(var("y"), int(0)), int(100), int(0)]), int(0)]);
        same!(then_body, vec![store(stepped(), tree)]);
        let flat = simple_if(bin(a_gt_b(), BinOp::And, c_lt_d()), int(100));
        assert!(is_fold(&folded(flat)));
    }

    #[test]
    fn comments_ride_their_operand_or_their_arm() {
        let mut s = simple_if(bin(a_gt_b(), BinOp::And, c_lt_d()), int(1));
        if let Statement::If { cond_comments, then_body, else_body, .. } = &mut s {
            *cond_comments = vec![None, Some(vec!["lower".into()])];
            then_body.insert(0, Statement::Comment(vec!["bull".into()]));
            else_body.push(Statement::Comment(vec!["none".into(), "at all".into()]));
        }
        let note = call(&format!("{NOTE}lower"), vec![c_lt_d()]);
        let yes = call(&format!("{LEAD}bull"), vec![int(1)]);
        let no = call(&format!("{NOTE}none at all"), vec![int(0)]);
        same!(folded(s), store(stepped(), call(SELECT, vec![call(EAGER_AND, vec![a_gt_b(), note]), yes, no])));
    }

    #[test]
    fn comments_that_do_not_pair_with_the_leaves_refuse_the_fold() {
        let mut s = simple_if(a_gt_b(), int(1));
        if let Statement::If { cond_comments, .. } = &mut s {
            *cond_comments = vec![Some(vec!["a".into()]), Some(vec!["b".into()])];
        }
        refused(s);
    }

    #[test]
    fn lay_out_gives_a_leading_comment_its_own_line() {
        let text = format!("  x = {};\n", join(&["s(c".into(), noted("1", ("bull", true)), "0)".into()], ", "));
        assert_eq!(lay_out(&text, false), "  x = s(c,\n      // bull\n      1, 0);\n");
    }

    #[test]
    fn lay_out_ends_each_line_on_its_one_comment() {
        let first = join(&[noted("(a)", ("one", false)), "(b)".into()], " & ");
        let text = format!("  x = {};\n  y;\n", glue(&first, " | ", &noted("(c)", ("two", false))));
        assert_eq!(lay_out(&text, true), "  x = (a) &      /* one */\n      (b) | (c); /* two */\n  y;\n");
        assert_eq!(lay_out("plain\n", false), "plain\n");
    }

    #[test]
    fn outside_a_loop_nothing_changes_and_lengths_hold() {
        let body = vec![simple_if(a_gt_b(), int(1)), Statement::Break, in_loop(simple_if(a_gt_b(), int(1))).remove(0)];
        let out = run(&body);
        assert_eq!(out.len(), body.len());
        assert!(matches!(out[0], Statement::If { .. }));
        let Statement::While { body: inner, .. } = &out[2] else { panic!() };
        assert_eq!(inner.len(), 3);
        assert!(is_fold(&inner[0]));
    }

    fn refused(s: Statement) {
        assert!(matches!(folded(s), Statement::If { .. }), "should be refused");
    }

    #[test]
    fn refuses_side_effects() {
        let inc = |e: Expr| Expr::PostIncrement(Box::new(e));
        refused(simple_if(gt(inc(var("j")), int(0)), int(1)));
        refused(simple_if(a_gt_b(), Expr::PreDecrement(Box::new(var("j")))));
        refused(simple_if(gt(Expr::AddressOf(Box::new(var("j"))), int(0)), int(1)));
    }

    #[test]
    fn refuses_a_call_off_the_pure_list() {
        refused(simple_if(gt(call("sma", vec![var("x")]), int(0)), int(1)));
        refused(simple_if(a_gt_b(), call("ta_unlikely", vec![var("x")])));
        assert!(is_fold(&folded(simple_if(gt(call("fabs", vec![var("x")]), int(0)), int(1)))));
    }

    #[test]
    fn refuses_a_value_reading_the_store_index_or_target() {
        refused(simple_if(a_gt_b(), var("k")));
        refused(simple_if(gt(at("out", i_minus(1)), int(0)), int(1)));
    }

    #[test]
    fn refuses_a_condition_reading_the_stepped_index() {
        let tmp_at_k = gt(at("tmp", var("k")), var("x"));
        let real_leaf = |v: f64| store(stepped(), Expr::Literal(v));
        refused(iff(tmp_at_k.clone(), vec![real_leaf(1.0)], vec![real_leaf(2.0)]));
        let inner = iff(tmp_at_k, vec![real_leaf(1.0)], vec![real_leaf(2.0)]);
        refused(iff(a_gt_b(), vec![inner], vec![real_leaf(0.0)]));
        let split = |v: Expr| vec![store(at("out", var("k")), v), store(var("k"), bin(var("k"), BinOp::Add, int(1)))];
        refused(iff(gt(at("tmp", var("k")), var("x")), split(int(1)), split(int(2))));
    }

    #[test]
    fn refuses_an_index_difference_outside_a_subscript() {
        refused(simple_if(gt(var("j"), int(2)), bin(var("j"), BinOp::Sub, int(3))));
        assert!(is_fold(&folded(simple_if(a_gt_b(), bin(var("x"), BinOp::Sub, var("y"))))));
    }

    #[test]
    fn refuses_a_tested_subscript_variable() {
        let guard = bin(gt(var("i"), int(0)), BinOp::And, gt(at("inClose", i_minus(1)), int(0)));
        refused(simple_if(guard, int(1)));
    }

    #[test]
    fn refuses_mismatched_targets_a_missing_else_and_a_non_output() {
        let other = at("out2", Expr::PostIncrement(Box::new(var("k"))));
        refused(iff(a_gt_b(), vec![store(stepped(), int(1))], vec![store(other, int(0))]));
        refused(iff(a_gt_b(), vec![store(stepped(), int(1))], vec![]));
        let not_output = at("tmp", var("i"));
        refused(iff(a_gt_b(), vec![store(not_output.clone(), int(1))], vec![store(not_output, int(0))]));
    }

    #[test]
    fn a_float_division_folds_and_an_integer_one_does_not() {
        assert!(is_fold(&folded(simple_if(gt(bin(var("x"), BinOp::Div, var("y")), int(1)), int(1)))));
        let avg = call("ta_candleaverage", vec![var("rt"), var("p"), var("f"), var("s"), var("o"), var("h"), var("l"), var("c")]);
        assert!(is_fold(&folded(simple_if(gt(call("ta_realbody", vec![var("c"), var("o")]), avg), int(1)))));
        refused(simple_if(a_gt_b(), bin(var("x"), BinOp::Mod, int(3))));
        refused(simple_if(gt(bin(var("n"), BinOp::Div, var("j")), int(1)), int(1)));
        refused(simple_if(a_gt_b(), bin(var("n"), BinOp::Div, int(2))));
    }

    #[test]
    fn rewrite_reads_outputs_by_name() {
        let outputs = [Output { name: "out".into(), param_type: ParamType::Integer, flags: Vec::new(), levels: Vec::new() }];
        let body = in_loop(simple_if(a_gt_b(), int(1)));
        let Statement::While { body: inner, .. } = &rewrite(&body, declared(), &outputs, helpers())[0] else { panic!() };
        assert!(is_fold(&inner[0]));
    }

    fn color(k: i64) -> Expr {
        call("ta_candlecolor", vec![at("inClose", i_minus(k)), at("inOpen", i_minus(k))])
    }
    fn white(k: i64) -> Expr {
        bin(at("inClose", i_minus(k)), BinOp::GreaterEq, at("inOpen", i_minus(k)))
    }
    fn neg(e: Expr) -> Expr {
        Expr::Neg(Box::new(e))
    }
    fn tern(p: Expr, t: i64, f: i64) -> Expr {
        Expr::Ternary(Box::new(p), Box::new(int(t)), Box::new(int(f)))
    }
    fn simple(e: &Expr) -> Expr {
        simplify(e, helpers())
    }

    #[test]
    fn a_flag_against_a_constant_is_its_predicate() {
        same!(simple(&bin(color(1), BinOp::Eq, int(1))), white(1));
        same!(simple(&bin(color(1), BinOp::Eq, int(-1))), not(white(1)));
        same!(simple(&bin(color(1), BinOp::Eq, neg(int(1)))), not(white(1)));
        same!(simple(&bin(int(-1), BinOp::NotEq, color(1))), white(1));
        same!(simple(&bin(color(1), BinOp::NotEq, int(1))), not(white(1)));
        same!(simple(&bin(tern(white(2), 1, 0), BinOp::NotEq, int(0))), white(2));
        same!(simple(&tern(white(2), 1, 0)), white(2));
        same!(simple(&tern(white(2), 0, 1)), not(white(2)));
        let never = bin(color(1), BinOp::Eq, int(2));
        same!(simple(&never.clone()), never);
    }

    #[test]
    fn two_flags_compare_as_their_predicates() {
        let eq = |l: Expr, op: BinOp, r: Expr| bin(l, op, r);
        same!(simple(&eq(color(2), BinOp::Eq, color(1))), eq(white(2), BinOp::Eq, white(1)));
        same!(simple(&eq(color(2), BinOp::NotEq, color(1))), eq(white(2), BinOp::NotEq, white(1)));
        same!(simple(&eq(color(1), BinOp::Eq, neg(color(0)))), eq(white(1), BinOp::NotEq, white(0)));
        same!(simple(&eq(neg(color(0)), BinOp::Eq, color(1))), eq(white(0), BinOp::NotEq, white(1)));
        same!(simple(&eq(color(1), BinOp::NotEq, neg(tern(white(0), 1, -1)))), eq(white(1), BinOp::Eq, white(0)));
        let mixed = eq(color(1), BinOp::Eq, tern(white(0), 1, 0));
        same!(simple(&mixed.clone()), mixed);
    }

    #[test]
    fn a_flag_on_a_non_boolean_predicate_and_a_value_arm_stay() {
        let t = bin(tern(var("j"), 1, -1), BinOp::Eq, int(1));
        same!(simple(&t.clone()), t);
        let value = bin(color(2), BinOp::Mul, int(100));
        let got = folded(simple_if(bin(color(1), BinOp::Eq, int(1)), value.clone()));
        same!(got, store(stepped(), call(SELECT, vec![white(1), value, int(0)])));
    }

    #[test]
    fn a_negated_atom_keeps_its_negation() {
        let lt = |b: Expr| bin(var("x"), BinOp::Less, b);
        same!(simple(&not(lt(call("max", vec![var("a"), var("b")])))), not(or(lt(var("a")), lt(var("b")))));
        same!(simple(&not(bin(color(1), BinOp::Eq, int(1)))), not(white(1)));
        same!(simple(&not(color(1))), not(color(1)));
    }

    fn mm(n: &str, a: &str, b: &str) -> Expr {
        call(n, vec![var(a), var(b)])
    }
    fn or(l: Expr, r: Expr) -> Expr {
        bin(l, BinOp::Or, r)
    }
    fn and(l: Expr, r: Expr) -> Expr {
        bin(l, BinOp::And, r)
    }

    #[test]
    fn a_comparison_with_min_or_max_splits_into_plain_ones() {
        let c = |l: Expr, op: BinOp, r: Expr| bin(l, op, r);
        for (op, rising) in [(BinOp::Greater, true), (BinOp::GreaterEq, true), (BinOp::Less, false), (BinOp::LessEq, false)] {
            let one = |a: &str| c(var(a), op.clone(), var("y"));
            let (any, all) = (or(one("a"), one("b")), and(one("a"), one("b")));
            same!(simple(&c(mm("max", "a", "b"), op.clone(), var("y"))), if rising { any.clone() } else { all.clone() });
            same!(simple(&c(mm("fmin", "a", "b"), op.clone(), var("y"))), if rising { all } else { any });
            let mirror = |b: &str| c(var("y"), op.clone(), var(b));
            let (any, all) = (or(mirror("a"), mirror("b")), and(mirror("a"), mirror("b")));
            same!(simple(&c(var("y"), op.clone(), mm("fmax", "a", "b"))), if rising { all.clone() } else { any.clone() });
            same!(simple(&c(var("y"), op.clone(), mm("min", "a", "b"))), if rising { any } else { all });
        }
    }

    #[test]
    fn min_against_max_splits_both_sides_within_the_cap() {
        let gt = |a: &str, b: &str| bin(var(a), BinOp::Greater, var(b));
        let want = and(and(gt("a", "c"), gt("a", "d")), and(gt("b", "c"), gt("b", "d")));
        same!(simple(&bin(mm("min", "a", "b"), BinOp::Greater, mm("max", "c", "d"))), want);
        let flagged = tern(bin(mm("fmin", "a", "b"), BinOp::Greater, mm("fmax", "c", "d")), 1, 0);
        same!(simple(&bin(flagged, BinOp::NotEq, int(0))), want);
        let wide = |p: &str| call("max", vec![mm("max", &format!("{p}1"), &format!("{p}2")), var(&format!("{p}3"))]);
        let nine = bin(wide("a"), BinOp::Less, wide("b"));
        same!(simple(&nine.clone()), nine);
        let eq = bin(mm("max", "a", "b"), BinOp::Eq, var("y"));
        same!(simple(&eq.clone()), eq);
    }

    #[test]
    fn an_integer_output_selects_by_mask_and_a_real_one_does_not() {
        let mut r = rules();
        r.int_outputs = vec!["out"];
        let int_leaf = simple_if(a_gt_b(), int(100));
        same!(folded_by(int_leaf.clone(), &r), store(stepped(), call(SELECT_INT, vec![a_gt_b(), int(100), int(0)])));
        let real_leaf = simple_if(a_gt_b(), var("x"));
        assert!(matches!(folded_by(real_leaf, &r), Statement::Assign { value: Expr::FuncCall(n, _), .. } if n == SELECT));
        assert!(matches!(folded_by(int_leaf, &rules()), Statement::Assign { value: Expr::FuncCall(n, _), .. } if n == SELECT));
    }

    fn assign(t: &str, v: Expr) -> Statement {
        store(var(t), v)
    }
    fn after(mut pass: Vec<Statement>, s: Statement) -> Vec<Statement> {
        pass.push(s);
        pass
    }

    #[test]
    fn a_loop_that_writes_each_temporary_before_reading_it_carries_nothing() {
        let tmp = assign("t", at("inReal", Expr::PostIncrement(Box::new(var("j")))));
        let pass = vec![tmp, simple_if(gt(var("t"), Expr::Literal(0.0)), int(1)), assign("i", bin(var("i"), BinOp::Add, int(1)))];
        assert!(carries_nothing(&pass));
        let step_down = vec![store(var("n"), bin(var("n"), BinOp::Sub, int(1))), simple_if(a_gt_b(), int(1))];
        assert!(carries_nothing(&step_down));
    }

    #[test]
    fn an_accumulator_a_conditional_write_an_inner_loop_or_a_reread_store_is_state() {
        let check = simple_if(gt(var("t"), Expr::Literal(0.0)), int(1));
        let sum = Statement::Assign { target: var("t"), value: bin(var("t"), BinOp::Add, at("inReal", var("i"))), compound: true };
        assert!(!carries_nothing(&[sum, check.clone()]));
        assert!(!carries_nothing(&[check.clone(), assign("t", at("inReal", var("i")))]));
        let in_arm = iff(a_gt_b(), vec![assign("t", int(1))], vec![]);
        assert!(!carries_nothing(&[in_arm, check.clone()]));
        let inner = Statement::While { condition: gt(var("n"), int(0)), body: vec![] };
        assert!(!carries_nothing(&[inner, simple_if(a_gt_b(), int(1))]));
        let reread = store(at("out", var("i")), at("out", i_minus(1)));
        assert!(!carries_nothing(&after(vec![reread], simple_if(a_gt_b(), int(1)))));
    }

    #[test]
    fn a_one_atom_tree_folds_only_in_a_loop_that_carries_nothing() {
        let carried = Statement::Assign { target: var("t"), value: bin(var("t"), BinOp::Add, var("x")), compound: true };
        let in_state = |s: Statement| {
            let out = run(&[Statement::While { condition: gt(var("n"), int(0)), body: vec![carried.clone(), s, reads(), step()] }]);
            let [Statement::While { body, .. }] = out.as_slice() else { panic!() };
            body[1].clone()
        };
        assert!(matches!(in_state(simple_if(a_gt_b(), int(1))), Statement::If { .. }));
        let nested = iff(a_gt_b(), vec![store(stepped(), int(1))], vec![simple_if(c_lt_d(), int(2))]);
        assert!(matches!(in_state(nested), Statement::If { .. }));
        assert!(is_fold(&in_state(simple_if(and(a_gt_b(), c_lt_d()), int(1)))));
        assert!(is_fold(&folded(simple_if(a_gt_b(), int(1)))));
    }

    #[test]
    fn a_comparison_of_two_sign_scaled_values_splits_on_the_sign() {
        let mul = |l: Expr, r: Expr| bin(l, BinOp::Mul, r);
        let split = |p: Expr, q: Expr, op: BinOp| or(and(p, bin(var("x"), op.clone(), var("y"))), and(q, bin(var("y"), op, var("x"))));
        let got = simple(&bin(mul(var("x"), color(4)), BinOp::Less, mul(var("y"), color(4))));
        same!(got, split(white(4), not(white(4)), BinOp::Less));
        let got = simple(&bin(mul(color(4), var("x")), BinOp::GreaterEq, mul(var("y"), tern(white(4), 1, -1))));
        same!(got, split(white(4), not(white(4)), BinOp::GreaterEq));
        let got = simple(&bin(mul(var("x"), neg(color(4))), BinOp::Greater, mul(var("y"), neg(color(4)))));
        same!(got, split(not(white(4)), white(4), BinOp::Greater));
    }

    #[test]
    fn a_sign_scaled_comparison_needs_one_unit_sign_on_both_sides() {
        let mul = |l: Expr, r: Expr| bin(l, BinOp::Mul, r);
        for kept in [
            bin(mul(var("x"), color(4)), BinOp::Less, mul(var("y"), color(3))),
            bin(mul(var("x"), color(4)), BinOp::Less, var("y")),
            bin(mul(var("x"), tern(white(4), 2, -2)), BinOp::Less, mul(var("y"), tern(white(4), 2, -2))),
            bin(mul(var("x"), color(4)), BinOp::Eq, mul(var("y"), color(4))),
        ] {
            same!(simple(&kept), kept);
        }
    }

    fn int_rules() -> Rules<'static> {
        let mut r = rules();
        r.int_outputs = vec!["out"];
        r
    }
    fn picked_by(s: Statement) -> String {
        match folded_by(s, &int_rules()) {
            Statement::Assign { value: Expr::FuncCall(n, _), .. } => n,
            other => panic!("not folded: {other:?}"),
        }
    }

    #[test]
    fn an_integer_arm_cheap_as_a_constant_is_picked_by_mask() {
        let mul = |l: Expr, r: Expr| bin(l, BinOp::Mul, r);
        let gap = Expr::Ternary(Box::new(call("ta_realbodygapup", vec![var("o1"), var("c1"), var("o2"), var("c2")])), Box::new(int(100)), Box::new(int(-100)));
        for v in [int(100), neg(int(80)), mul(color(1), int(100)), mul(int(100), neg(color(2))), tern(a_gt_b(), 100, -100), gap] {
            assert_eq!(picked_by(simple_if(and(a_gt_b(), c_lt_d()), v)), SELECT_INT);
        }
        let int_name = folded_by(simple_if(and(a_gt_b(), c_lt_d()), var("lookbackTotal")), &int_rules());
        assert!(matches!(int_name, Statement::If { .. }), "an integer name is not declared real");
    }

    #[test]
    fn a_costly_integer_arm_is_picked_only_on_a_match() {
        let pick = Expr::Ternary(Box::new(c_lt_d()), Box::new(var("i")), Box::new(i_minus(1)));
        let loaded = call("ta_candlecolor", vec![at("inClose", pick.clone()), at("inOpen", pick)]);
        let cond = and(a_gt_b(), c_lt_d());
        assert_eq!(picked_by(simple_if(cond.clone(), bin(loaded, BinOp::Mul, int(100)))), GUARD);
        assert_eq!(picked_by(simple_if(cond.clone(), at("inClose", var("i")))), GUARD);
        let nested = iff(a_gt_b(), vec![store(stepped(), at("inClose", var("i")))], vec![simple_if(c_lt_d(), int(80))]);
        let Statement::Assign { value: Expr::FuncCall(n, a), .. } = folded_by(nested, &int_rules()) else { panic!() };
        assert_eq!(n, GUARD);
        assert!(matches!(&a[2], Expr::FuncCall(m, _) if m == SELECT_INT));
        assert!(is_fold(&folded(simple_if(cond, at("inClose", var("i"))))));
    }

    #[test]
    fn a_nest_choosing_between_values_folds_whole() {
        let inner = iff(c_lt_d(), vec![store(stepped(), int(100))], vec![store(stepped(), int(80))]);
        let s = iff(a_gt_b(), vec![inner], vec![store(stepped(), int(0))]);
        let want = store(stepped(), call(SELECT, vec![a_gt_b(), call(SELECT, vec![c_lt_d(), int(100), int(80)]), int(0)]));
        same!(folded(s), want);
    }

    #[test]
    fn a_pre_check_of_several_conjuncts_is_counted() {
        let pre = bin(bin(a_gt_b(), BinOp::And, c_lt_d()), BinOp::And, gt(var("x"), int(0)));
        let s = iff(pre, vec![simple_if(gt(var("y"), int(0)), int(100))], vec![store(stepped(), int(0))]);
        let Statement::If { condition, then_body, .. } = folded(s) else { panic!("the pre-check stays a branch") };
        same!(condition, call(EAGER_ALL, vec![a_gt_b(), c_lt_d(), gt(var("x"), int(0))]));
        assert!(is_fold(&then_body[0]));
    }

    #[test]
    fn a_one_test_pre_check_that_expands_is_counted_over_its_atoms() {
        let gap = bin(mm("min", "a", "b"), BinOp::Greater, mm("max", "c", "d"));
        let s = iff(gap, vec![simple_if(gt(var("y"), int(0)), int(100))], vec![store(stepped(), int(0))]);
        let Statement::If { condition, .. } = folded(s) else { panic!("the pre-check stays a branch") };
        let gt2 = |a: &str, b: &str| bin(var(a), BinOp::Greater, var(b));
        same!(condition, call(EAGER_ALL, vec![gt2("a", "c"), gt2("a", "d"), gt2("b", "c"), gt2("b", "d")]));
        let doji = iff(a_gt_b(), vec![simple_if(c_lt_d(), int(100))], vec![store(stepped(), int(0))]);
        let Statement::If { condition, .. } = folded(doji) else { panic!() };
        same!(condition, a_gt_b());
    }

    /// Whether `rewrite` folds some tree of `body`.
    fn folds_in(body: &[Statement], declared: &Declared) -> bool {
        let outputs = [Output { name: "out".into(), param_type: ParamType::Integer, flags: Vec::new(), levels: Vec::new() }];
        let mut hit = false;
        each_statement(&rewrite(body, declared, &outputs, helpers()), &mut |s| hit |= is_fold(s));
        hit
    }
    fn local(t: VarType, n: &str, init: Option<Expr>) -> Statement {
        Statement::VarDecl { var_type: t, name: n.into(), init }
    }
    fn x_half_and(e: Expr) -> Statement {
        simple_if(and(gt(var("x"), Expr::Literal(0.5)), e), int(1))
    }

    #[test]
    fn a_step_declared_in_its_for_init_enters_where_the_declaration_says() {
        let d = Declared { arrays: declared().arrays.clone(), reals: names(&["x"]), lookback: "f_lookback".into(), fixed: 3, ..Declared::default() };
        let looped = |init: Statement| {
            let body = vec![
                assign("lookbackTotal", call("f_lookback", Vec::new())),
                iff(bin(var("startIdx"), BinOp::Less, var("lookbackTotal")), vec![assign("startIdx", var("lookbackTotal"))], Vec::new()),
                iff(gt(var("startIdx"), var("endIdx")), vec![Statement::Return { value: None }], Vec::new()),
                assign("i", var("startIdx")),
                Statement::ForC {
                    init: Box::new(init),
                    condition: bin(var("i"), BinOp::LessEq, var("endIdx")),
                    update: Box::new(step()),
                    body: vec![x_half_and(gt(at("inClose", i_minus(3)), at("inOpen", var("i"))))],
                },
            ];
            folds_in(&body, &d)
        };
        let below = bin(var("startIdx"), BinOp::Sub, int(3));
        assert!(!looped(local(VarType::Integer, "i", Some(below.clone()))), "a declared step entering below the floor folds");
        assert!(!looped(assign("i", below)));
        assert!(looped(local(VarType::Integer, "i", Some(var("startIdx")))));
        assert!(looped(assign("i", var("startIdx"))));
    }

    #[test]
    fn a_helper_argument_must_be_written_before_it_runs() {
        let tree = || x_half_and(gt(call("ta_realbody", vec![var("z"), at("inClose", var("i"))]), Expr::Literal(1.0)));
        let write = || assign("z", at("inClose", var("i")));
        let looped = |pass: Vec<Statement>| vec![local(VarType::Real, "z", None), Statement::While { condition: gt(var("n"), int(0)), body: pass }];
        assert!(!folds_in(&looped(vec![tree(), write(), reads(), step()]), declared()), "an unwritten local runs eagerly in a helper");
        assert!(folds_in(&looped(vec![write(), tree(), reads(), step()]), declared()));
    }

    #[test]
    fn an_inner_declaration_writes_only_its_own_name() {
        let looped = |pre: Vec<Statement>| {
            let mut body = pre;
            body.extend(in_loop(x_half_and(gt(var("z"), at("inClose", var("i"))))));
            body
        };
        let inner = Statement::Block { body: vec![local(VarType::Real, "z", Some(Expr::Literal(1.0)))] };
        assert!(!folds_in(&looped(vec![local(VarType::Real, "z", None), inner]), declared()), "a shadowing declaration writes the outer name");
        assert!(folds_in(&looped(vec![local(VarType::Real, "z", Some(Expr::Literal(1.0)))]), declared()));
    }

    #[test]
    fn a_local_array_element_must_be_written_before_the_loop() {
        let looped = |pre: Vec<Statement>| {
            let mut body = vec![local(VarType::RealArray("2".into()), "buf", None)];
            body.extend(pre);
            body.extend(in_loop(x_half_and(gt(at("buf", int(0)), at("inClose", var("i"))))));
            body
        };
        let set = |k: i64| store(at("buf", int(k)), Expr::Literal(0.0));
        assert!(!folds_in(&looped(Vec::new()), declared()), "an unwritten array element runs eagerly");
        assert!(!folds_in(&looped(vec![set(1)]), declared()));
        assert!(folds_in(&looped(vec![set(0)]), declared()));
    }

    /// The canonical prologue, raising `startIdx` to 3, then `rest`.
    fn after_prologue(rest: Vec<Statement>) -> (Vec<Statement>, Declared) {
        let mut body = vec![
            assign("lookbackTotal", call("f_lookback", Vec::new())),
            iff(bin(var("startIdx"), BinOp::Less, var("lookbackTotal")), vec![assign("startIdx", var("lookbackTotal"))], Vec::new()),
            iff(gt(var("startIdx"), var("endIdx")), vec![Statement::Return { value: None }], Vec::new()),
            assign("i", var("startIdx")),
        ];
        body.extend(rest);
        (body, Declared { arrays: declared().arrays.clone(), reals: names(&["x"]), lookback: "f_lookback".into(), fixed: 3, ..Declared::default() })
    }
    /// `pre`, a loop whose pass is `head`, `tree` and its window reads, then `after`.
    fn around(mut pre: Vec<Statement>, head: Vec<Statement>, tree: Statement, after: Vec<Statement>) -> Vec<Statement> {
        let mut pass = head;
        pass.extend([tree, reads(), step()]);
        pre.push(Statement::While { condition: gt(var("n"), int(0)), body: pass });
        pre.extend(after);
        pre
    }

    #[test]
    fn a_step_redeclared_in_the_pass_or_a_for_init_list_indexes_nothing() {
        let tree = || x_half_and(gt(at("inClose", i_minus(3)), at("inOpen", var("i"))));
        let below = || local(VarType::Integer, "i", Some(bin(var("startIdx"), BinOp::Sub, int(3))));
        let to_end = || bin(var("i"), BinOp::LessEq, var("endIdx"));
        let looped = |pass: Vec<Statement>| {
            let (body, d) = after_prologue(vec![Statement::While { condition: to_end(), body: pass }]);
            folds_in(&body, &d)
        };
        assert!(looped(vec![tree(), step()]));
        assert!(!looped(vec![below(), tree(), step()]));
        assert!(!looped(vec![Statement::Block { body: vec![below(), tree()] }, step()]));
        let init = Statement::Block { body: vec![below(), assign("k", int(0))] };
        let listed = Statement::ForC { init: Box::new(init), condition: to_end(), update: Box::new(step()), body: vec![tree()] };
        let (body, d) = after_prologue(vec![listed]);
        assert!(!folds_in(&body, &d));
    }

    #[test]
    fn a_name_declared_twice_is_refused_even_where_each_is_written() {
        let tree = || x_half_and(gt(var("z"), at("inClose", var("i"))));
        let z = |init: Option<Expr>| local(VarType::Real, "z", init);
        let write = || assign("z", at("inOpen", var("i")));
        assert!(folds_in(&around(vec![z(None)], vec![write()], tree(), vec![]), declared()));
        let shadowed = Statement::Block { body: vec![z(None), tree()] };
        assert!(!folds_in(&around(vec![z(None)], vec![write()], shadowed, vec![]), declared()), "the inner z is never written");
        let both = vec![Statement::Block { body: vec![z(Some(Expr::Literal(1.0)))] }];
        assert!(!folds_in(&around(vec![z(Some(Expr::Literal(0.0)))], vec![], tree(), both), declared()));
        let buf = || local(VarType::RealArray("2".into()), "buf", None);
        let read_buf = || x_half_and(gt(at("buf", int(0)), at("inClose", var("i"))));
        let put = || store(at("buf", int(0)), Expr::Literal(0.0));
        assert!(!folds_in(&around(vec![buf(), put()], vec![], read_buf(), vec![Statement::Block { body: vec![buf()] }]), declared()));
    }

    #[test]
    fn a_local_shadowing_any_parameter_is_refused() {
        let shadow = |n: &str, t: VarType, init: Option<Expr>| vec![Statement::Block { body: vec![local(t, n, init)] }];
        // `p` and the setting are integers: the division may trap.
        for n in ["p", "BodyDoji_avgPeriod"] {
            let divides = x_half_and(gt(bin(int(100), BinOp::Div, var(n)), at("inClose", var("i"))));
            assert!(!folds_in(&around(vec![], vec![], divides, shadow(n, VarType::Real, Some(Expr::Literal(1.0)))), declared()), "{n}");
        }
        let output = x_half_and(gt(at("out", int(0)), at("inClose", var("i"))));
        let put = store(at("out", int(0)), Expr::Literal(0.0));
        assert!(!folds_in(&around(vec![put], vec![], output, shadow("out", VarType::RealArray("2".into()), None)), declared()));
    }

    #[test]
    fn a_counted_loop_takes_no_floor_from_before_it() {
        let r = Rules { entering: Entering { raised_only: true, canonical: Some((0, 3)), end_fixed: true, ..Entering::default() }, ..rules() };
        let folds = |reads: Expr| {
            let tree = x_half_and(gt(at("inClose", i_minus(3)), at("inOpen", var("i"))));
            let counted = Statement::For { var: "i".into(), count: var("n"), body: vec![tree, assign("w", reads)] };
            let out = walk(&[Statement::Comment(vec!["prologue".into()]), assign("i", var("startIdx")), counted], None, true, &r);
            let Statement::For { body, .. } = &out[2] else { panic!("loop gone") };
            is_fold(&body[0])
        };
        assert!(folds(bin(at("inClose", i_minus(3)), BinOp::Add, at("inOpen", var("i")))));
        assert!(!folds(at("inOpen", var("i"))));
        // `j` is no counter: the loop leaves it as `j = startIdx` set it.
        let j_folds = |count: Expr| {
            let tree = x_half_and(gt(at("inClose", bin(var("j"), BinOp::Sub, int(3))), at("inOpen", var("j"))));
            let j_step = Statement::Expr(Expr::PostIncrement(Box::new(var("j"))));
            let counted = Statement::For { var: "m".into(), count, body: vec![tree, assign("w", at("inOpen", var("j"))), j_step] };
            let out = walk(&[Statement::Comment(vec!["prologue".into()]), assign("j", var("startIdx")), counted], None, true, &r);
            let Statement::For { body, .. } = &out[2] else { panic!("loop gone") };
            is_fold(&body[0])
        };
        assert!(j_folds(var("n")));
        assert!(!j_folds(Expr::PostDecrement(Box::new(var("j")))));
        let reset_by = |counter: &str| {
            let tree = x_half_and(gt(at("inClose", i_minus(3)), at("inOpen", var("i"))));
            let lp = Statement::While { condition: bin(var("i"), BinOp::LessEq, var("endIdx")), body: vec![tree, step()] };
            let counted = Statement::For { var: counter.into(), count: int(5), body: Vec::new() };
            let out = walk(&[Statement::Comment(vec!["prologue".into()]), assign("i", var("startIdx")), counted, lp], None, true, &r);
            let Statement::While { body, .. } = &out[3] else { panic!("loop gone") };
            is_fold(&body[0])
        };
        assert!(reset_by("m"));
        assert!(!reset_by("i"), "the counted loop leaves i at 0");
    }

    /// Whether a tree reading `inClose[i-3]` folds in a pass that runs `pre`
    /// first and surely reads `inClose[i]`.
    fn reads_back_three_after(pre: Statement) -> bool {
        let tree = x_half_and(gt(at("inClose", i_minus(3)), at("inOpen", var("i"))));
        let out = run(&[Statement::While { condition: gt(var("n"), int(0)), body: vec![pre, tree, assign("w", at("inClose", var("i"))), step()] }]);
        let [Statement::While { body, .. }] = out.as_slice() else { panic!("loop gone") };
        is_fold(&body[1])
    }

    #[test]
    fn a_call_proves_only_the_reads_its_rendering_runs_on_every_pass() {
        assert!(reads_back_three_after(assign("j", at("inOpen", i_minus(3)))));
        let cast = Expr::Cast(VarType::Integer, Box::new(at("inOpen", i_minus(3))));
        assert!(!reads_back_three_after(assign("j", call("ta_auto_stabilization_ema", vec![cast, var("i")]))), "read on one arm of the helper's body");
        let range = |open: Expr, rest: Expr| {
            let named = |a: &str| at(a, rest.clone());
            call("ta_candlerange", vec![var("BodyLong_rangeType"), open, named("inHigh"), named("inLow"), named("inClose")])
        };
        assert!(reads_back_three_after(assign("e", range(at("inOpen", i_minus(3)), i_minus(3)))));
        assert!(!reads_back_three_after(assign("e", range(at("inOpen", i_minus(3)), var("i")))), "the high-low range never reads the open");
    }

    #[test]
    fn a_call_that_may_return_proves_no_window() {
        let looped = |extra: Vec<Statement>| {
            let mut pass = vec![x_half_and(gt(at("inClose", i_minus(3)), at("inOpen", var("i"))))];
            pass.extend(extra);
            pass.extend([reads(), step()]);
            let out = run(&[Statement::While { condition: gt(var("n"), int(0)), body: pass }]);
            let [Statement::While { body, .. }] = out.as_slice() else { panic!("loop gone") };
            is_fold(&body[0])
        };
        assert!(looped(vec![]));
        assert!(!looped(vec![assign("t", call("sma", vec![var("i")]))]), "a cross-call's failure returns");
        let layout = || crate::ir::CircBufLayout::Plain(VarType::Real);
        assert!(!looped(vec![Statement::CircBuf(CircBuf::Init { id: "b".into(), layout: layout(), size: var("n") })]), "an allocation's failure returns");
        assert!(looped(vec![Statement::CircBuf(CircBuf::Next { id: "b".into() })]));
    }
}
