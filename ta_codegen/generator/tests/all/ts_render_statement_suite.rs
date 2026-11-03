//! TypeScript statement rendering, one construct at a time, and the text gate
//! over every generated module (H7, H8).

use crate::common::{discover_indicators, load_enums, load_indicator, make_helpers, make_registry};
use ta_codegen_lib::backends::ctype::CType;
use ta_codegen_lib::backends::ts;
use ta_codegen_lib::ir::{BinOp, Expr, Statement, VarType};

fn var(n: &str) -> Expr {
    Expr::Var(n.to_string())
}

fn bin(l: Expr, op: BinOp, r: Expr) -> Expr {
    Expr::BinOp(Box::new(l), op, Box::new(r))
}

fn assign(target: Expr, value: Expr, compound: bool) -> Statement {
    Statement::Assign { target, value, compound }
}

const SCOPE: &[(&str, CType)] = &[
    ("i", CType::Int32),
    ("k", CType::Int32),
    ("x", CType::Real),
    ("y", CType::Real),
    ("outInt", CType::IntPtr),
    ("outReal", CType::RealPtr),
    ("inReal", CType::RealPtr),
];

fn render(stmt: &Statement) -> String {
    ts::render_statement(stmt, 0, &load_enums(), make_registry(), make_helpers(), SCOPE)
}

#[test]
fn integer_division_and_modulo_truncate() {
    assert_eq!(render(&assign(var("i"), bin(var("i"), BinOp::Div, var("k")), false)), "i = ((i / k) | 0);\n");
    assert_eq!(render(&assign(var("i"), bin(var("i"), BinOp::Mod, Expr::IntLiteral(2)), false)), "i = ((i % 2) | 0);\n");
    assert_eq!(render(&assign(var("x"), bin(var("x"), BinOp::Div, var("k")), false)), "x = x / k;\n");
}

#[test]
fn an_integer_compound_keeps_the_truncation() {
    assert_eq!(render(&assign(var("i"), bin(var("i"), BinOp::Div, var("k")), true)), "i = ((i / k) | 0);\n");
    assert_eq!(render(&assign(var("i"), bin(var("i"), BinOp::Mul, Expr::IntLiteral(2)), true)), "i = ((i * 2) | 0);\n");
    assert_eq!(render(&assign(var("i"), bin(var("i"), BinOp::Add, var("x")), true)), "i = (i + x | 0);\n");
    assert_eq!(render(&assign(var("i"), bin(var("i"), BinOp::Add, var("k")), true)), "i += k;\n");
    assert_eq!(render(&assign(var("x"), bin(var("x"), BinOp::Div, var("k")), true)), "x /= k;\n");
}

/// `x += a*b` keeps C's unfused compound; `x = x + a*b` fuses (H9).
#[test]
fn only_the_plain_form_of_an_accumulation_fuses() {
    let body = vec![
        Statement::VarDecl { var_type: VarType::Real, name: "sum".to_string(), init: None },
        assign(var("sum"), bin(var("sum"), BinOp::Add, bin(var("x"), BinOp::Mul, var("y"))), true),
        assign(var("sum"), bin(var("sum"), BinOp::Add, bin(var("x"), BinOp::Mul, var("y"))), false),
    ];
    let (mut func, enums) = load_indicator("sma");
    func.body = body;
    func.body.insert(0, Statement::VarDecl { var_type: VarType::Real, name: "y".to_string(), init: None });
    func.body.insert(0, Statement::VarDecl { var_type: VarType::Real, name: "x".to_string(), init: None });
    let text = ts::generate(&func, &enums, make_registry(), make_helpers());
    assert!(text.contains("  sum += x * y;\n"), "{text}");
    assert!(text.contains("  sum = fma(x, y, sum);\n"), "{text}");
}

/// A loop with three fused sites gets `fma`'s body copied in, except where it
/// would run ahead of what the statement evaluates first or of a condition.
#[test]
fn a_loop_with_three_fused_sites_copies_the_fast_path_in() {
    let fused = |c: Expr| bin(bin(var("x"), BinOp::Mul, var("y")), BinOp::Add, c);
    let at = |n: &str, i: Expr| Expr::ArrayAccess(n.to_string(), Box::new(i));
    let decl = |t: VarType, n: &str| Statement::VarDecl { var_type: t, name: n.to_string(), init: None };
    let loop_body = vec![
        assign(var("x"), fused(var("x")), false),
        assign(var("y"), fused(var("y")), false),
        assign(at("outReal", Expr::PostIncrement(Box::new(var("i")))), fused(at("inReal", var("i"))), false),
        assign(var("x"), Expr::Ternary(Box::new(bin(var("x"), BinOp::Greater, var("y"))), Box::new(fused(var("x"))), Box::new(var("y"))), false),
    ];
    let (mut func, enums) = load_indicator("sma");
    func.body = vec![
        decl(VarType::Real, "x"),
        decl(VarType::Real, "y"),
        decl(VarType::Integer, "i"),
        Statement::While { condition: bin(var("i"), BinOp::Less, Expr::IntLiteral(9)), body: loop_body },
        assign(var("x"), fused(var("x")), false),
    ];
    let text = ts::generate(&func, &enums, make_registry(), make_helpers());
    assert_eq!(text.matches("fmaSlow(_a, _b, _c, _th, _tl, _pl)").count(), 2, "{text}");
    assert!(text.contains("\n      x = _w && "), "{text}");
    assert!(text.contains("\n    outReal[i++] = fma(x, y, inReal[i]);\n"), "{text}");
    assert!(text.contains("\n    x = (x > y) ? fma(x, y, x) : y;\n"), "{text}");
    assert!(text.contains("\n  x = fma(x, y, x);\n"), "{text}");
}

#[test]
fn integer_negation_and_product_clear_negative_zero() {
    assert_eq!(render(&assign(var("x"), Expr::Neg(Box::new(var("i"))), false)), "x = (-i | 0);\n");
    assert_eq!(render(&assign(var("x"), bin(var("i"), BinOp::Mul, var("k")), false)), "x = ((i * k) | 0);\n");
    assert_eq!(render(&assign(var("x"), Expr::Neg(Box::new(var("y"))), false)), "x = -y;\n");
}

#[test]
fn a_real_reaching_an_int_truncates_explicit_or_implicit() {
    let cast = Expr::Cast(VarType::Integer, Box::new(bin(var("x"), BinOp::Mul, var("y"))));
    assert_eq!(render(&assign(var("i"), cast, false)), "i = (x * y | 0);\n");
    assert_eq!(render(&assign(var("i"), var("x"), false)), "i = (x | 0);\n");
    // The element store truncates by itself.
    let elem = Expr::ArrayAccess("outInt".to_string(), Box::new(var("i")));
    assert_eq!(render(&assign(elem, var("x"), false)), "outInt[i] = x;\n");
    assert_eq!(render(&assign(var("x"), Expr::Cast(VarType::Real, Box::new(var("i"))), false)), "x = i;\n");
}

#[test]
fn a_bool_used_as_a_number_and_a_number_tested_for_truth() {
    let less = bin(var("x"), BinOp::Less, var("y"));
    assert_eq!(render(&assign(var("i"), less.clone(), false)), "i = (x < y ? 1 : 0);\n");
    let ternary = Expr::Ternary(Box::new(less), Box::new(Expr::IntLiteral(1)), Box::new(Expr::IntLiteral(0)));
    assert_eq!(render(&assign(var("i"), bin(var("i"), BinOp::Add, ternary), true)), "i += (x < y ? 1 : 0);\n");
    let stmt = Statement::If { condition: var("i"), then_body: vec![Statement::Break], else_body: vec![], cond_comments: vec![] };
    assert_eq!(render(&stmt), "if (i !== 0) {\n  break;\n}\n");
    let not = Statement::If { condition: Expr::Not(Box::new(var("i"))), then_body: vec![Statement::Break], else_body: vec![], cond_comments: vec![] };
    assert_eq!(render(&not), "if (!(i !== 0)) {\n  break;\n}\n");
}

#[test]
fn max_and_min_are_c_macros_in_operand_order() {
    let call = |n: &str| Expr::FuncCall(n.to_string(), vec![var("x"), var("y")]);
    assert_eq!(render(&assign(var("x"), call("max"), false)), "x = (x > y ? x : y);\n");
    assert_eq!(render(&assign(var("x"), call("fmin"), false)), "x = (x < y ? x : y);\n");
}

#[test]
fn shifts_and_masks_render_as_written() {
    assert_eq!(render(&assign(var("i"), bin(var("k"), BinOp::Shr, Expr::IntLiteral(1)), false)), "i = k >> 1;\n");
    assert_eq!(render(&assign(var("i"), bin(var("k"), BinOp::BitwiseAnd, Expr::IntLiteral(7)), false)), "i = k & 7;\n");
}

/// C pointer equality is the same buffer at the same offset (H15).
#[test]
fn pointer_equality_compares_buffer_and_offset() {
    let stmt = Statement::If {
        condition: bin(var("inReal"), BinOp::Eq, var("outReal")),
        then_body: vec![Statement::Break],
        else_body: vec![],
        cond_comments: vec![],
    };
    assert_eq!(
        render(&stmt),
        "if ((inReal.buffer === outReal.buffer && inReal.byteOffset === outReal.byteOffset)) {\n  break;\n}\n"
    );
}

#[test]
fn literals_are_shortest_round_trip() {
    for (v, text) in [(0.1, "0.1"), (1e-13, "1e-13"), (3.0, "3.0"), (2.0 / 3.0, "0.6666666666666666"), (1e300, "1e300")] {
        assert_eq!(render(&assign(var("x"), Expr::Literal(v), false)), format!("x = {text};\n"));
    }
}

#[test]
fn a_reserved_function_name_is_mangled() {
    let (func, enums) = load_indicator("var");
    let text = ts::generate(&func, &enums, make_registry(), make_helpers());
    assert!(text.contains("export function var_("), "VAR must render as `var_`");
    assert!(ts::public_exports(&func).contains(&"var_".to_string()));
}

/// `Math.max`/`Math.min` differ from C on NaN and signed zeros, `Math.round`
/// from every C rounding, and `>>>` is unsigned where C's `>>` on an `int` is
/// not. Generated fresh, so a committed tree cannot hide a regression.
#[test]
fn no_generated_module_spells_a_js_only_numeric_form() {
    let (registry, helpers) = (make_registry(), make_helpers());
    let mut modules = 0;
    for name in discover_indicators() {
        let (func, enums) = load_indicator(&name);
        let text = ts::generate(&func, &enums, registry, helpers);
        for bad in ["Math.max(", "Math.min(", "Math.round(", ">>>"] {
            assert!(!text.contains(bad), "{name}: emits `{bad}`");
        }
        modules += 1;
    }
    assert!(modules > 0, "VACUOUS: no module generated");
}
