//! Every guard shape in every position the eager pass rewrites, executed: on a
//! bar where the guard is false, the rewritten loop body must never evaluate
//! the operand the guard protects.
//!
//! The evaluator gives each pseudo-call its most eager backend reading: a
//! select runs both arms, as Rust's `select_unpredictable` and C's integer
//! mask do, and only `GUARD` and `&&` / `||` / `?:` skip an operand.

use std::collections::HashMap;

use super::eager_select::{note_of, rewrite, Declared, EAGER_ALL, EAGER_AND, EAGER_OR, GUARD, SELECT, SELECT_INT};
use crate::helper_registry::HelperRegistry;
use crate::ir::{BinOp, Expr, Output, ParamType, Statement, VarType};

const TEMPLATE: &str = "
TA_RetCode f( int startIdx, int endIdx, const double inClose[], const double inHigh[], double x, double y, int n, int sumN, int big, int *p, const double tmp[],
              int *outBegIdx, int *outNBElement, int outInteger[], double outReal[] )
{
   int i, j, k, m;
   double d, e;
   i = startIdx;
   j = 0;
   k = 0;
   while( i <= endIdx )
   {
      m = i - 4;
      d = i;
      e = inClose[i] + inHigh[i];
      BODY
      i++;
   }
   return TA_SUCCESS;
}
";

struct Shape {
    name: &'static str,
    guard: &'static str,
    /// The guarded operand as a test, and as a stored value.
    test: &'static str,
    value: &'static str,
    /// Whether evaluating the operand on the guard-false bar fails at all.
    hazard: bool,
    /// Whether the guard alone may run eagerly.
    eager_guard: bool,
}

const SHAPES: &[Shape] = &[
    Shape { name: "index offset", guard: "i > 3", test: "inClose[i-4] > x", value: "inClose[i-4]", hazard: true, eager_guard: false },
    Shape { name: "integer divide", guard: "n != 0", test: "i / n > 1", value: "i / n", hazard: true, eager_guard: false },
    Shape { name: "real divide", guard: "fabs(y) > 0.000001", test: "x / y > 1.0", value: "x / y", hazard: false, eager_guard: true },
    Shape { name: "pointer deref", guard: "p != 0", test: "*p > 1", value: "*p", hazard: true, eager_guard: false },
    Shape { name: "array subscript", guard: "tmp != 0", test: "tmp[i] > x", value: "tmp[i]", hazard: true, eager_guard: false },
    Shape { name: "side effect", guard: "x > 1.0", test: "j++ > 0", value: "j++", hazard: true, eager_guard: true },
    Shape { name: "aliased index", guard: "i > 3", test: "inClose[m] > x", value: "inClose[m]", hazard: true, eager_guard: false },
    Shape { name: "element guard", guard: "inHigh[i] < 8.0", test: "inClose[(int)inHigh[i]] > x", value: "inClose[(int)inHigh[i]]", hazard: true, eager_guard: true },
    Shape { name: "signed overflow", guard: "big < 1000", test: "i * big > 5", value: "i * big", hazard: true, eager_guard: false },
    Shape { name: "int divide sum-named param", guard: "sumN != 0", test: "i / sumN > 1", value: "i / sumN", hazard: true, eager_guard: false },
    Shape { name: "index offset, real-proxy guard", guard: "d > 3.0", test: "inClose[i-4] > x", value: "inClose[i-4]", hazard: true, eager_guard: true },
    Shape { name: "index offset, helper guard", guard: "ta_candlegapup(i, 3.5)", test: "inClose[i-4] > x", value: "inClose[i-4]", hazard: true, eager_guard: true },
    Shape { name: "aliased index, real-proxy guard", guard: "d > 3.0", test: "inClose[m] > x", value: "inClose[m]", hazard: true, eager_guard: true },
    Shape { name: "read ahead, real-proxy guard", guard: "d < 1.0", test: "inClose[i+6] > x", value: "inClose[i+6]", hazard: true, eager_guard: true },
    Shape { name: "integer divide, admitted guard", guard: "x > 1.0", test: "i / n > 1", value: "i / n", hazard: true, eager_guard: true },
    Shape { name: "int divide sum-named, admitted guard", guard: "x > 1.0", test: "i / sumN > 1", value: "i / sumN", hazard: true, eager_guard: true },
    Shape { name: "signed overflow, admitted guard", guard: "x > 1.0", test: "i * big > 5", value: "i * big", hazard: true, eager_guard: true },
    Shape { name: "pointer deref, admitted guard", guard: "x > 1.0", test: "*p > 1", value: "*p", hazard: true, eager_guard: true },
    Shape { name: "array subscript, admitted guard", guard: "x > 1.0", test: "tmp[i] > x", value: "tmp[i]", hazard: true, eager_guard: true },
    Shape { name: "non-unit flag", guard: "x > 1.0", test: "2 * (x < 0.6 ? 2000000000 : 0) > 5", value: "2 * (x < 0.6 ? 2000000000 : 0)", hazard: true, eager_guard: true },
    Shape { name: "non-pure call", guard: "x > 1.0", test: "external_effect(x) > 0.0", value: "external_effect(x)", hazard: true, eager_guard: true },
];

const SAFE_GUARD: &str = "x < 1.0";
const SAFE_TEST: &str = "x > 1.0";
const SAFE_VALUE: &str = "x";

/// `$G` is the guard, `$T` or `$V` what it guards. `x > 0.75` is false on the
/// probe bar and `x < 1.0` true, so each position reaches its guard.
const POSITIONS: &[(&str, &str)] = &[
    ("flat tree", "if( x > 0.75 && $G && $T ) outInteger[k++] = 100; else outInteger[k++] = 0;"),
    (
        "outer pre-check",
        "if( $G && $T ) { if( x > 0.75 ) outInteger[k++] = 100; else outInteger[k++] = 0; } else outInteger[k++] = 0;",
    ),
    (
        "second chain link",
        "if( x > 0.75 ) { if( x > 2.0 ) outInteger[k++] = 100; else outInteger[k++] = 0; } \
         else if( $G && $T ) { if( x < 1.0 ) outInteger[k++] = -100; else outInteger[k++] = 0; } else outInteger[k++] = 0;",
    ),
    (
        "third chain link",
        "if( x > 0.75 ) { if( x > 2.0 ) outInteger[k++] = 100; else outInteger[k++] = 0; } \
         else if( x > 2.0 ) { if( x > 3.0 ) outInteger[k++] = 100; else outInteger[k++] = 0; } \
         else if( $G && $T ) { if( x < 1.0 ) outInteger[k++] = -100; else outInteger[k++] = 0; } else outInteger[k++] = 0;",
    ),
    (
        "inner tree",
        "if( x < 1.0 ) { if( $G && $T ) outInteger[k++] = 100; else outInteger[k++] = 0; } else outInteger[k++] = 0;",
    ),
    ("or disjunct", "if( x > 0.75 || ( $G && $T ) ) outInteger[k++] = 100; else outInteger[k++] = 0;"),
    ("leaf value", "if( $G ) outReal[k++] = $V; else outReal[k++] = 0.0;"),
];

#[derive(Clone, Copy, Debug)]
enum Val {
    I(i64),
    R(f64),
}

impl Val {
    fn truth(self) -> bool {
        match self {
            Val::I(v) => v != 0,
            Val::R(v) => v != 0.0,
        }
    }
    fn real(self) -> f64 {
        match self {
            #[allow(clippy::cast_precision_loss)]
            Val::I(v) => v as f64,
            Val::R(v) => v,
        }
    }
}

/// The probe bar: every guard false, every guarded operand failing.
struct Bar {
    ints: HashMap<&'static str, i64>,
    reals: HashMap<&'static str, f64>,
    arrays: HashMap<&'static str, Vec<f64>>,
}

fn probe_bar() -> Bar {
    Bar {
        ints: HashMap::from([("i", 2), ("j", 0), ("k", 0), ("n", 0), ("p", 0), ("tmp", 0), ("m", -2), ("sumN", 0), ("big", 2_000_000_000)]),
        reals: HashMap::from([("x", 0.5), ("y", 0.0), ("d", 2.0)]),
        arrays: HashMap::from([("inClose", vec![0.0; 8]), ("inHigh", vec![100.0; 8])]),
    }
}

type Eval = Result<Val, String>;

fn bool_val(b: bool) -> Val {
    Val::I(i64::from(b))
}

fn eval(e: &Expr, bar: &Bar) -> Eval {
    let index = |i: &Expr| -> Result<usize, String> {
        match eval(i, bar)? {
            Val::I(v) => usize::try_from(v).map_err(|_| format!("negative index {v}")),
            Val::R(v) => Err(format!("real index {v}")),
        }
    };
    Ok(match e {
        Expr::Literal(v) => Val::R(*v),
        Expr::IntLiteral(v) => Val::I(*v),
        Expr::Var(n) => match (bar.ints.get(n.as_str()), bar.reals.get(n.as_str())) {
            (Some(v), _) => Val::I(*v),
            (_, Some(v)) => Val::R(*v),
            _ => panic!("the probe bar has no `{n}`"),
        },
        Expr::ArrayAccess(a, i) => {
            let Some(arr) = bar.arrays.get(a.as_str()) else { return Err(format!("{a}[..] read through a null array")) };
            let k = index(i)?;
            Val::R(*arr.get(k).ok_or_else(|| format!("{a}[{k}] out of bounds"))?)
        }
        Expr::PointerDeref(n) => {
            let Some(arr) = bar.arrays.get(n.as_str()) else { return Err(format!("*{n} through a null pointer")) };
            Val::R(arr[0])
        }
        Expr::BinOp(l, BinOp::And, r) => bool_val(eval(l, bar)?.truth() && eval(r, bar)?.truth()),
        Expr::BinOp(l, BinOp::Or, r) => bool_val(eval(l, bar)?.truth() || eval(r, bar)?.truth()),
        Expr::BinOp(l, op, r) => binop(eval(l, bar)?, op, eval(r, bar)?)?,
        Expr::Not(x) => bool_val(!eval(x, bar)?.truth()),
        Expr::Neg(x) => match eval(x, bar)? {
            Val::I(v) => Val::I(i64::from(i32::try_from(v).ok().and_then(i32::checked_neg).ok_or("signed overflow")?)),
            Val::R(v) => Val::R(-v),
        },
        Expr::Cast(to, operand) => {
            let value = eval(operand, bar)?.real();
            match to {
                VarType::Integer | VarType::Index => {
                    let whole = value.trunc();
                    if !(f64::from(i32::MIN)..=f64::from(i32::MAX)).contains(&whole) {
                        return Err(format!("(int){whole} out of range"));
                    }
                    #[allow(clippy::cast_possible_truncation)]
                    Val::I(whole as i64)
                }
                _ => Val::R(value),
            }
        }
        Expr::Ternary(c, t, f) => {
            if eval(c, bar)?.truth() {
                eval(t, bar)?
            } else {
                eval(f, bar)?
            }
        }
        Expr::PostIncrement(x) | Expr::PreIncrement(x) | Expr::PostDecrement(x) | Expr::PreDecrement(x) => {
            return Err(format!("side effect on {x:?}"))
        }
        Expr::FuncCall(n, args) => call(n, args, bar)?,
        other => panic!("the evaluator does not model {other:?}"),
    })
}

fn call(n: &str, args: &[Expr], bar: &Bar) -> Eval {
    if note_of(n).is_some() {
        return eval(&args[0], bar);
    }
    if n == GUARD {
        return if eval(&args[0], bar)?.truth() { eval(&args[1], bar) } else { eval(&args[2], bar) };
    }
    let vals = args.iter().map(|a| eval(a, bar)).collect::<Result<Vec<_>, _>>()?;
    Ok(match n {
        _ if n == EAGER_AND => bool_val(vals.iter().all(|v| v.truth())),
        _ if n == EAGER_OR => bool_val(vals.iter().any(|v| v.truth())),
        _ if n == EAGER_ALL => bool_val(vals.iter().all(|v| v.truth())),
        _ if n == SELECT || n == SELECT_INT => {
            if vals[0].truth() {
                vals[1]
            } else {
                vals[2]
            }
        }
        "fabs" => Val::R(vals[0].real().abs()),
        "ta_candlegapup" => bool_val(vals[0].real() > vals[1].real()),
        _ => return Err(format!("call to {n}")),
    })
}

fn binop(l: Val, op: &BinOp, r: Val) -> Eval {
    if let (Val::I(a), Val::I(b)) = (l, r) {
        // C `int` is 32 bits, and Rust debug panics where it overflows.
        let int = |v: Option<i32>| v.map(|v| Val::I(i64::from(v))).ok_or_else(|| "signed overflow".to_string());
        let (a32, b32) = (i32::try_from(a).map_err(|_| "signed overflow")?, i32::try_from(b).map_err(|_| "signed overflow")?);
        return Ok(match op {
            BinOp::Add => int(a32.checked_add(b32))?,
            BinOp::Sub => int(a32.checked_sub(b32))?,
            BinOp::Mul => int(a32.checked_mul(b32))?,
            BinOp::Div | BinOp::Mod if b == 0 => return Err("integer division by zero".into()),
            BinOp::Div => int(a32.checked_div(b32))?,
            BinOp::Mod => int(a32.checked_rem(b32))?,
            BinOp::BitwiseAnd => Val::I(a & b),
            BinOp::BitwiseOr => Val::I(a | b),
            BinOp::BitwiseXor => Val::I(a ^ b),
            _ => compare(Val::I(a), op, Val::I(b)),
        });
    }
    let (a, b) = (l.real(), r.real());
    Ok(match op {
        BinOp::Add => Val::R(a + b),
        BinOp::Sub => Val::R(a - b),
        BinOp::Mul => Val::R(a * b),
        BinOp::Div => Val::R(a / b),
        _ => compare(l, op, r),
    })
}

#[allow(clippy::float_cmp)]
fn compare(l: Val, op: &BinOp, r: Val) -> Val {
    let (a, b) = (l.real(), r.real());
    bool_val(match op {
        BinOp::Less => a < b,
        BinOp::LessEq => a <= b,
        BinOp::Greater => a > b,
        BinOp::GreaterEq => a >= b,
        BinOp::Eq => a == b,
        BinOp::NotEq => a != b,
        other => panic!("the evaluator does not model {other:?}"),
    })
}

/// One pass of the loop body; the loop's own `i++` is not run.
fn run(body: &[Statement], bar: &Bar) -> Result<(), String> {
    for s in body {
        match s {
            Statement::If { condition, then_body, else_body, .. } => {
                run(if eval(condition, bar)?.truth() { then_body } else { else_body }, bar)?;
            }
            Statement::Assign { target: Expr::ArrayAccess(_, _), value, .. } => {
                eval(value, bar)?;
            }
            Statement::Block { body } => run(body, bar)?,
            Statement::Comment(_) | Statement::Expr(Expr::PostIncrement(_)) | Statement::Assign { target: Expr::Var(_), .. } => {}
            other => panic!("the evaluator does not model {other:?}"),
        }
    }
    Ok(())
}

/// The loop body of `body` spliced into the template, before and after the pass.
fn loop_bodies(body: &str) -> (Vec<Statement>, Vec<Statement>) {
    loop_bodies_in(TEMPLATE, body)
}

fn loop_bodies_in(template: &str, body: &str) -> (Vec<Statement>, Vec<Statement>) {
    let parsed = crate::parser::c_source::parse_c_source_str(&template.replace("BODY", body));
    let f = parsed.functions.into_iter().find(|f| f.name == "f").expect("the template parses");
    let outputs = [
        Output { name: "outInteger".into(), param_type: ParamType::Integer, flags: Vec::new(), levels: Vec::new() },
        Output { name: "outReal".into(), param_type: ParamType::Real, flags: Vec::new(), levels: Vec::new() },
    ];
    let declared = Declared {
        arrays: ["inClose", "inHigh"].map(String::from).to_vec(),
        reals: ["x", "y"].map(String::from).to_vec(),
        lookback: "f_lookback".into(),
        fixed: 2,
        ..Declared::default()
    };
    let after = rewrite(&f.body, &declared, &outputs, &HelperRegistry::empty());
    let inner = |b: &[Statement]| {
        b.iter()
            .find_map(|s| match s {
                Statement::While { body, .. } | Statement::DoWhile { body, .. } => Some(body.clone()),
                _ => None,
            })
            .expect("the template's loop")
    };
    (inner(&f.body), inner(&after))
}

fn cell(position: &str, guard: &str, test: &str, value: &str) -> String {
    position.replace("$G", guard).replace("$T", test).replace("$V", value)
}

fn folded(position_name: &str, body: &[Statement]) -> bool {
    let text = format!("{body:?}");
    if position_name.contains("pre-check") || position_name.contains("chain") {
        text.contains(EAGER_ALL)
    } else {
        text.contains(SELECT) || text.contains(GUARD)
    }
}

#[test]
fn no_rewrite_reaches_a_guarded_operand_on_a_bar_its_guard_rejects() {
    let bar = probe_bar();
    let (mut failed, mut vacuous) = (Vec::new(), Vec::new());
    for shape in SHAPES {
        let (bare, _) = loop_bodies(&format!("if( {} ) outInteger[k++] = 1; else outInteger[k++] = 0;", shape.test));
        assert_eq!(run(&bare, &bar).is_err(), shape.hazard, "{}: the probe bar does not decide the operand's hazard", shape.name);
        for &(position, text) in POSITIONS {
            let (before, after) = loop_bodies(&cell(text, shape.guard, shape.test, shape.value));
            assert_eq!(run(&before, &bar), Ok(()), "{} / {position}: the source itself reaches the operand", shape.name);
            if let Err(why) = run(&after, &bar) {
                failed.push(format!("{} / {position}: {why}", shape.name));
            }
            let (_, active) = loop_bodies(&cell(text, SAFE_GUARD, SAFE_TEST, SAFE_VALUE));
            if !folded(position, &active) {
                vacuous.push(format!("{position}: a safe tree does not fold here"));
            }
            let (_, guard_alone) = loop_bodies(&cell(text, shape.guard, SAFE_TEST, SAFE_VALUE));
            if folded(position, &guard_alone) != shape.eager_guard {
                vacuous.push(format!("{} / {position}: the guard alone should fold: {}", shape.name, shape.eager_guard));
            }
        }
    }
    assert!(vacuous.is_empty(), "{} control(s) wrong:\n{}", vacuous.len(), vacuous.join("\n"));
    assert!(failed.is_empty(), "{} cell(s) reach a guarded operand:\n{}", failed.len(), failed.join("\n"));
}

/// The canonical prologue, a lookback of `max(...) + 2`, and nothing read on
/// every pass: the floor and the loop test alone bound the reads.
const CANONICAL: &str = "
TA_RetCode f( int startIdx, int endIdx, const double inClose[], const double inHigh[], double x, double y,
              int *outBegIdx, int *outNBElement, int outInteger[], double outReal[] )
{
   int i, k, lookbackTotal;
   double d;
   lookbackTotal = f_lookback();
   if( startIdx < lookbackTotal ) {
      startIdx = lookbackTotal;
   }
   if( startIdx > endIdx ) {
      return TA_SUCCESS;
   }
   i = startIdx;
   k = 0;
   do
   {
      d = i;
      BODY
      i++;
   } while( i <= endIdx );
   return TA_SUCCESS;
}
";

#[test]
fn after_the_canonical_prologue_reads_fold_from_the_lookback_to_the_bar() {
    // The first bar the prologue admits, on the shortest series that has it.
    let bar = Bar {
        ints: HashMap::from([("i", 2), ("k", 0)]),
        reals: HashMap::from([("x", 0.5), ("y", 0.0), ("d", 2.0)]),
        arrays: HashMap::from([("inClose", vec![0.0; 3]), ("inHigh", vec![0.0; 3])]),
    };
    let mut failed = Vec::new();
    for (name, guard, test, value) in [
        ("below the lookback", "d > 3.0", "inClose[i-3] > x", "inClose[i-3]"),
        ("past the loop test", "d < 1.0", "inClose[i+1] > x", "inClose[i+1]"),
    ] {
        for &(position, text) in POSITIONS {
            let (before, after) = loop_bodies_in(CANONICAL, &cell(text, guard, test, value));
            assert_eq!(run(&before, &bar), Ok(()), "{name} / {position}: the source itself reaches the operand");
            if let Err(why) = run(&after, &bar) {
                failed.push(format!("{name} / {position}: {why}"));
            }
        }
    }
    for &(position, text) in POSITIONS {
        for edge in ["inClose[i-2]", "inClose[i]"] {
            let (_, after) = loop_bodies_in(CANONICAL, &cell(text, SAFE_GUARD, &format!("{edge} > x"), edge));
            assert!(folded(position, &after), "{position}: a read at {edge} is inside the proven window");
            assert_eq!(run(&after, &bar), Ok(()));
        }
    }
    assert!(failed.is_empty(), "{} cell(s) reach a guarded operand:\n{}", failed.len(), failed.join("\n"));
}
