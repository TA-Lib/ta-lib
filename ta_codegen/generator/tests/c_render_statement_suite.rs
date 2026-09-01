//! C backend: unit tests of `render_statement` and related rendering
//! branches (plus a handful of Java rendering tests that exercise the same
//! fixtures side by side). Split out of the former `backend_suite.rs`.

#[path = "common/mod.rs"]
mod common;

use common::{
    contains_call, generate_all, load_enums, load_indicator, make_registry, render_c_stmt,
    render_java_stmt,
};
use std::collections::HashMap;
use ta_codegen_lib::backends;
use ta_codegen_lib::helper_registry::HelperRegistry;
use ta_codegen_lib::ir;

// C backend coverage tests
// ===========================================================================

// ---------------------------------------------------------------------------
// C: VarDecl rendering for all VarType variants via render_statement
// ---------------------------------------------------------------------------

#[test]
fn c_vardecl_retcode_type() {
    let stmt = ir::Statement::VarDecl {
        var_type: ir::VarType::RetCodeType,
        name: "retCode".to_string(),
        init: None,
    };
    let rendered = render_c_stmt(&stmt);
    assert!(
        rendered.contains("TA_RetCode retCode"),
        "C VarDecl RetCodeType should render as 'TA_RetCode retCode': {rendered}"
    );
}

#[test]
fn c_vardecl_real_pointer() {
    let stmt = ir::Statement::VarDecl {
        var_type: ir::VarType::RealPointer,
        name: "buf".to_string(),
        init: None,
    };
    let rendered = render_c_stmt(&stmt);
    assert!(
        rendered.contains("double *buf"),
        "C VarDecl RealPointer should render as 'double *buf': {rendered}"
    );
}

#[test]
fn c_vardecl_int_pointer() {
    let stmt = ir::Statement::VarDecl {
        var_type: ir::VarType::IntPointer,
        name: "indices".to_string(),
        init: None,
    };
    let rendered = render_c_stmt(&stmt);
    assert!(
        rendered.contains("int *indices"),
        "C VarDecl IntPointer should render as 'int *indices': {rendered}"
    );
}

#[test]
fn c_vardecl_real_array() {
    let stmt = ir::Statement::VarDecl {
        var_type: ir::VarType::RealArray("30".to_string()),
        name: "arr".to_string(),
        init: None,
    };
    let rendered = render_c_stmt(&stmt);
    assert!(
        rendered.contains("double arr[30]"),
        "C VarDecl RealArray should render as 'double arr[30]': {rendered}"
    );
}

#[test]
fn c_vardecl_int_array() {
    let stmt = ir::Statement::VarDecl {
        var_type: ir::VarType::IntArray("5".to_string()),
        name: "flags".to_string(),
        init: None,
    };
    let rendered = render_c_stmt(&stmt);
    assert!(
        rendered.contains("int flags[5]"),
        "C VarDecl IntArray should render as 'int flags[5]': {rendered}"
    );
}

#[test]
fn c_vardecl_with_init_expr() {
    let stmt = ir::Statement::VarDecl {
        var_type: ir::VarType::Real,
        name: "total".to_string(),
        init: Some(ir::Expr::Literal(2.71)),
    };
    let rendered = render_c_stmt(&stmt);
    assert!(
        rendered.contains("double total = 2.71"),
        "C VarDecl with init should render the init expression: {rendered}"
    );
}

// ---------------------------------------------------------------------------
// C: Return None renders 'return;'
// ---------------------------------------------------------------------------

#[test]
fn c_return_none() {
    let stmt = ir::Statement::Return { value: None };
    let rendered = render_c_stmt(&stmt);
    assert!(
        rendered.contains("return;"),
        "C Return None should render as 'return;': {rendered}"
    );
}

// ---------------------------------------------------------------------------
// C: For countdown loop rendering
// ---------------------------------------------------------------------------

#[test]
fn c_for_countdown_loop() {
    let stmt = ir::Statement::For {
        var: "i".to_string(),
        count: ir::Expr::Var("optInTimePeriod".to_string()),
        body: vec![ir::Statement::Assign {
            target: ir::Expr::Var("tempReal".to_string()),
            value: ir::Expr::Literal(1.0),
            compound: false,
        }],
    };
    let rendered = render_c_stmt(&stmt);
    assert!(
        rendered.contains("for( i = optInTimePeriod; i > 0; i-- )"),
        "C For countdown should render correctly: {rendered}"
    );
    assert!(
        rendered.contains("tempReal = 1.0"),
        "C For countdown body should be rendered: {rendered}"
    );
}

// ---------------------------------------------------------------------------
// C: ForC rendering
// ---------------------------------------------------------------------------

#[test]
fn c_forc_single_init_renders_correctly() {
    let stmt = ir::Statement::ForC {
        init: Box::new(ir::Statement::Assign {
            target: ir::Expr::Var("i".to_string()),
            value: ir::Expr::IntLiteral(0),
            compound: false,
        }),
        condition: ir::Expr::BinOp(
            Box::new(ir::Expr::Var("i".to_string())),
            ir::BinOp::Less,
            Box::new(ir::Expr::Var("n".to_string())),
        ),
        update: Box::new(ir::Statement::Assign {
            target: ir::Expr::Var("i".to_string()),
            value: ir::Expr::PostIncrement(Box::new(ir::Expr::Var("i".to_string()))),
            compound: false,
        }),
        body: vec![ir::Statement::Assign {
            target: ir::Expr::Var("sum".to_string()),
            value: ir::Expr::Literal(1.0),
            compound: false,
        }],
    };
    let rendered = render_c_stmt(&stmt);
    assert!(
        rendered.contains("for("),
        "C ForC should render as for(): {rendered}"
    );
    assert!(
        rendered.contains("i < n"),
        "C ForC should render condition: {rendered}"
    );
}

// ---------------------------------------------------------------------------
// C: Block statement rendering
// ---------------------------------------------------------------------------

#[test]
fn c_block_statement_renders_inner_stmts() {
    let stmt = ir::Statement::Block {
        body: vec![
            ir::Statement::Assign {
                target: ir::Expr::Var("x".to_string()),
                value: ir::Expr::Literal(1.0),
                compound: false,
            },
            ir::Statement::Assign {
                target: ir::Expr::Var("y".to_string()),
                value: ir::Expr::Literal(2.0),
                compound: false,
            },
        ],
    };
    let rendered = render_c_stmt(&stmt);
    assert!(
        rendered.contains("x = 1.0"),
        "C Block should render inner statements: {rendered}"
    );
    assert!(
        rendered.contains("y = 2.0"),
        "C Block should render all inner statements: {rendered}"
    );
}

// ---------------------------------------------------------------------------
// C: T3 exercises For countdown loop (real indicator)
// ---------------------------------------------------------------------------

#[test]
fn c_t3_for_countdown_loops() {
    let (func, enums) = load_indicator("t3");
    let out = generate_all(&func, &enums);
    let c = &out.c;

    // T3 uses multiple for(i=period-1; i>0; i--) loops (rendered as i -= 1)
    assert!(
        c.contains("i > 0; i -= 1"),
        "C T3 should contain countdown for loops"
    );
}

// ---------------------------------------------------------------------------
// C: STOCH exercises malloc/free/memcpy; MA exercises cross-indicator calls;
//    MACD lockstep-fusion stays fused
// ---------------------------------------------------------------------------

#[test]
fn c_stoch_has_malloc_and_free() {
    // STOCH mallocs a temp %K buffer, memmove's it into the caller buffer, and
    // frees it. (memmove, not memcpy: the temp aliases outSlowK when the caller
    // reuses the buffer — see #94. MACD was the original vehicle, but its
    // lockstep fusion removed the temp buffers.)
    let (func, enums) = load_indicator("stoch");
    let out = generate_all(&func, &enums);
    let c = &out.c;

    assert!(
        c.contains("malloc("),
        "C STOCH should contain malloc calls"
    );
    assert!(
        c.contains("free("),
        "C STOCH should contain free calls"
    );
    assert!(
        c.contains("memmove("),
        "C STOCH should contain memmove calls"
    );
}

#[test]
fn c_ma_cross_indicator_calls() {
    // MA dispatches to the per-type moving averages via the guarded internal cores.
    // (MACD was the original vehicle, but its lockstep fusion removed the EMA
    // calls.)
    let (func, enums) = load_indicator("ma");
    let out = generate_all(&func, &enums);
    let c = &out.c;

    assert!(
        c.contains("TA_INT_EMA(") || c.contains("TA_EMA("),
        "C MA should call EMA: {c}"
    );
    assert!(
        c.contains("TA_EMA_Lookback("),
        "C MA should call TA_EMA_Lookback"
    );
}

#[test]
fn c_macd_lockstep_stays_fused() {
    // Pin the MACD lockstep optimization (97b1a258/07199aa4): both EMAs, the
    // signal EMA and the histogram are fused into one pass — no temp buffers,
    // no cross-indicator EMA compute calls. If this fails, the optimization
    // regressed back to the buffered form.
    let (func, enums) = load_indicator("macd");
    let out = generate_all(&func, &enums);
    let c = &out.c;

    assert!(
        !c.contains("malloc("),
        "C MACD lockstep form should not allocate temp buffers"
    );
    assert!(
        !c.contains("TA_INT_EMA(") && !c.contains("TA_EMA_Unguarded("),
        "C MACD lockstep form should not delegate to EMA compute calls"
    );
}

// ---------------------------------------------------------------------------
// C: Expression rendering edge cases
// ---------------------------------------------------------------------------

#[test]
fn c_var_name_mappings() {
    // Test that special variable names are mapped correctly
    let stmts = vec![
        ("COMPATIBILITY", "TA_GLOBALS_COMPATIBILITY"),
        ("SUCCESS", "TA_SUCCESS"),
        ("BAD_PARAM", "TA_BAD_PARAM"),
        ("ALLOC_ERR", "TA_ALLOC_ERR"),
        ("INTERNAL_ERROR", "TA_INTERNAL_ERROR"),
    ];

    for (var_name, expected) in stmts {
        let stmt = ir::Statement::Assign {
            target: ir::Expr::Var("x".to_string()),
            value: ir::Expr::Var(var_name.to_string()),
            compound: false,
        };
        let rendered = render_c_stmt(&stmt);
        assert!(
            rendered.contains(expected),
            "C Var '{var_name}' should map to '{expected}': {rendered}"
        );
    }
}

// ---------------------------------------------------------------------------
// C: Cast expression rendering
// ---------------------------------------------------------------------------

#[test]
fn c_cast_expression_types() {
    let stmt = ir::Statement::Assign {
        target: ir::Expr::Var("x".to_string()),
        value: ir::Expr::Cast(
            ir::VarType::Integer,
            Box::new(ir::Expr::Literal(2.71)),
        ),
        compound: false,
    };
    let rendered = render_c_stmt(&stmt);
    assert!(
        rendered.contains("(int)2.71"),
        "C Cast to Integer should render as (int)...: {rendered}"
    );

    let stmt2 = ir::Statement::Assign {
        target: ir::Expr::Var("rc".to_string()),
        value: ir::Expr::Cast(
            ir::VarType::RetCodeType,
            Box::new(ir::Expr::IntLiteral(0)),
        ),
        compound: false,
    };
    let rendered2 = render_c_stmt(&stmt2);
    assert!(
        rendered2.contains("(TA_RetCode)0"),
        "C Cast to RetCodeType should render as (TA_RetCode)...: {rendered2}"
    );

    let stmt3 = ir::Statement::Assign {
        target: ir::Expr::Var("p".to_string()),
        value: ir::Expr::Cast(
            ir::VarType::RealPointer,
            Box::new(ir::Expr::Var("buf".to_string())),
        ),
        compound: false,
    };
    let rendered3 = render_c_stmt(&stmt3);
    assert!(
        rendered3.contains("(double *)buf"),
        "C Cast to RealPointer should render as (double *)...: {rendered3}"
    );

    let stmt4 = ir::Statement::Assign {
        target: ir::Expr::Var("p".to_string()),
        value: ir::Expr::Cast(
            ir::VarType::IntPointer,
            Box::new(ir::Expr::Var("arr".to_string())),
        ),
        compound: false,
    };
    let rendered4 = render_c_stmt(&stmt4);
    assert!(
        rendered4.contains("(int *)arr"),
        "C Cast to IntPointer should render as (int *)...: {rendered4}"
    );
}

// ---------------------------------------------------------------------------
// C: PointerDeref and AddressOf expression rendering
// ---------------------------------------------------------------------------

#[test]
fn c_pointer_deref_renders_star() {
    let stmt = ir::Statement::Assign {
        target: ir::Expr::Var("x".to_string()),
        value: ir::Expr::PointerDeref("outBegIdx".to_string()),
        compound: false,
    };
    let rendered = render_c_stmt(&stmt);
    assert!(
        rendered.contains("*outBegIdx"),
        "C PointerDeref should render as *name: {rendered}"
    );
}

#[test]
fn c_address_of_renders_ampersand() {
    let stmt = ir::Statement::Assign {
        target: ir::Expr::Var("x".to_string()),
        value: ir::Expr::AddressOf(Box::new(ir::Expr::Var("myVar".to_string()))),
        compound: false,
    };
    let rendered = render_c_stmt(&stmt);
    assert!(
        rendered.contains("&myVar"),
        "C AddressOf should render as &name: {rendered}"
    );
}

// ---------------------------------------------------------------------------
// C: Ternary expression rendering
// ---------------------------------------------------------------------------

#[test]
fn c_ternary_expression() {
    let stmt = ir::Statement::Assign {
        target: ir::Expr::Var("x".to_string()),
        value: ir::Expr::Ternary(
            Box::new(ir::Expr::BinOp(
                Box::new(ir::Expr::Var("a".to_string())),
                ir::BinOp::Greater,
                Box::new(ir::Expr::Var("b".to_string())),
            )),
            Box::new(ir::Expr::Var("a".to_string())),
            Box::new(ir::Expr::Var("b".to_string())),
        ),
        compound: false,
    };
    let rendered = render_c_stmt(&stmt);
    assert!(
        rendered.contains("?") && rendered.contains(":"),
        "C ternary should render as (cond) ? (then) : (else): {rendered}"
    );
}

// ---------------------------------------------------------------------------
// C: Increment/Decrement expressions
// ---------------------------------------------------------------------------

#[test]
fn c_increment_decrement_expressions() {
    let post_inc = ir::Statement::Assign {
        target: ir::Expr::Var("x".to_string()),
        value: ir::Expr::PostIncrement(Box::new(ir::Expr::Var("i".to_string()))),
        compound: false,
    };
    let rendered = render_c_stmt(&post_inc);
    assert!(
        rendered.contains("i++"),
        "C PostIncrement should render as i++: {rendered}"
    );

    let post_dec = ir::Statement::Assign {
        target: ir::Expr::Var("x".to_string()),
        value: ir::Expr::PostDecrement(Box::new(ir::Expr::Var("j".to_string()))),
        compound: false,
    };
    let rendered2 = render_c_stmt(&post_dec);
    assert!(
        rendered2.contains("j--"),
        "C PostDecrement should render as j--: {rendered2}"
    );

    let pre_inc = ir::Statement::Assign {
        target: ir::Expr::Var("x".to_string()),
        value: ir::Expr::PreIncrement(Box::new(ir::Expr::Var("k".to_string()))),
        compound: false,
    };
    let rendered3 = render_c_stmt(&pre_inc);
    assert!(
        rendered3.contains("++k"),
        "C PreIncrement should render as ++k: {rendered3}"
    );

    let pre_dec = ir::Statement::Assign {
        target: ir::Expr::Var("x".to_string()),
        value: ir::Expr::PreDecrement(Box::new(ir::Expr::Var("m".to_string()))),
        compound: false,
    };
    let rendered4 = render_c_stmt(&pre_dec);
    assert!(
        rendered4.contains("--m"),
        "C PreDecrement should render as --m: {rendered4}"
    );
}

// ---------------------------------------------------------------------------
// C: Not expression rendering
// ---------------------------------------------------------------------------

#[test]
fn c_not_expression() {
    let stmt = ir::Statement::Assign {
        target: ir::Expr::Var("x".to_string()),
        value: ir::Expr::Not(Box::new(ir::Expr::Var("flag".to_string()))),
        compound: false,
    };
    let rendered = render_c_stmt(&stmt);
    assert!(
        rendered.contains("!flag"),
        "C Not expression should render as !expr: {rendered}"
    );
}

// ---------------------------------------------------------------------------
// C: BinOp rendering for all operators
// ---------------------------------------------------------------------------

#[test]
fn c_binop_all_operators() {
    let ops = vec![
        (ir::BinOp::Add, "+"),
        (ir::BinOp::Sub, "-"),
        (ir::BinOp::Mul, "*"),
        (ir::BinOp::Div, "/"),
        (ir::BinOp::Mod, "%"),
        (ir::BinOp::LessEq, "<="),
        (ir::BinOp::Less, "<"),
        (ir::BinOp::Greater, ">"),
        (ir::BinOp::GreaterEq, ">="),
        (ir::BinOp::Eq, "=="),
        (ir::BinOp::NotEq, "!="),
        (ir::BinOp::And, "&&"),
        (ir::BinOp::Or, "||"),
        (ir::BinOp::BitwiseOr, "|"),
        (ir::BinOp::Shr, ">>"),
        (ir::BinOp::Shl, "<<"),
    ];

    for (op, expected) in ops {
        let stmt = ir::Statement::Assign {
            target: ir::Expr::Var("result".to_string()),
            value: ir::Expr::BinOp(
                Box::new(ir::Expr::Var("a".to_string())),
                op,
                Box::new(ir::Expr::Var("b".to_string())),
            ),
            compound: false,
        };
        let rendered = render_c_stmt(&stmt);
        assert!(
            rendered.contains(expected),
            "C BinOp should contain '{expected}': {rendered}"
        );
    }
}

// ---------------------------------------------------------------------------
// C: MACD lookback exercises lookback code rendering (lines 1140-1210)
// ---------------------------------------------------------------------------

#[test]
fn c_macd_lookback_code_rendering() {
    let (func, enums) = load_indicator("macd");
    let out = generate_all(&func, &enums);
    let c = &out.c;

    // MACD lookback should have the swap logic
    let lookback_end = c.find("TA_LIB_API TA_RetCode TA_MACD(").unwrap();
    let lookback = &c[..lookback_end];
    assert!(
        lookback.contains("TA_MACD_Lookback"),
        "C MACD should have lookback function"
    );
    // The lookback body should contain variable declarations and logic
    assert!(
        lookback.contains("tempInteger") || lookback.contains("int "),
        "C MACD lookback should have variable declarations"
    );
}

// ---------------------------------------------------------------------------
// Java: MACD lookback code rendering
// ---------------------------------------------------------------------------

#[test]
fn java_macd_lookback_code_rendering() {
    let (func, enums) = load_indicator("macd");
    let out = generate_all(&func, &enums);
    let j = &out.java;

    let lookback_end = j.find("RetCode MACD_Impl(").unwrap();
    let lookback = &j[..lookback_end];
    assert!(
        lookback.contains("MACD_Lookback"),
        "Java MACD should have lookback function"
    );
}

// ---------------------------------------------------------------------------
// Java/C: STOCHRSI lookback exercises cross-indicator lookback calls
// ---------------------------------------------------------------------------

#[test]
fn stochrsi_lookback_cross_calls() {
    let (func, enums) = load_indicator("stochrsi");
    let out = generate_all(&func, &enums);

    // C lookback should call rsi_lookback and stochf_lookback
    let c = &out.c;
    assert!(
        c.contains("TA_RSI_Lookback(") || c.contains("TA_STOCHF_Lookback("),
        "C STOCHRSI lookback should have cross-indicator lookback calls"
    );

    // Java lookback sums both callees' lookbacks. `contains_call` for the same
    // reason as above — `RSI_Lookback(` is a suffix of `STOCHRSI_Lookback(`.
    let j = &out.java;
    assert!(
        contains_call(j, "RSI_Lookback") && contains_call(j, "STOCHF_Lookback"),
        "Java STOCHRSI lookback should have cross-indicator lookback calls"
    );
}

// ---------------------------------------------------------------------------
// Java Var name mappings (exercises lines 1307-1326)
// ---------------------------------------------------------------------------

#[test]
fn java_var_name_mappings() {
    // Fixed (non-enum) constant renderings. COMPATIBILITY/METASTOCK/DEFAULT are
    // deliberately absent: Java pins the mode to Default and the branches are
    // constant-folded away before rendering, so those names never reach `var`
    // (reaching it panics — see `java_compatibility_is_folded_away`).
    let mut cases: Vec<(String, String)> = [
        ("BAD_PARAM", "RetCode.BadParam"),
        ("SUCCESS", "RetCode.Success"),
        ("ALLOC_ERR", "RetCode.AllocErr"),
        ("INTERNAL_ERROR", "RetCode.InternalError"),
    ]
    .iter()
    .map(|(a, b)| ((*a).to_string(), (*b).to_string()))
    .collect();

    // MAType constants are derived from enums.yaml — iterate the enum rather than
    // a literal table so the test can never go stale when a TA_MAType_X row lands.
    let enums = load_enums();
    let matype = &enums["MAType"];
    assert!(!matype.variants.is_empty(), "MAType enum should be non-empty");
    for v in &matype.variants {
        cases.push((v.c_name.clone(), format!("MAType.{}", v.name)));
    }

    for (var_name, expected) in cases {
        let stmt = ir::Statement::Assign {
            target: ir::Expr::Var("result".to_string()),
            value: ir::Expr::Var(var_name.clone()),
            compound: false,
        };
        let rendered = render_java_stmt(&stmt);
        assert!(
            rendered.contains(&expected),
            "Java Var '{var_name}' should map to '{expected}': {rendered}"
        );
    }
}

// ---------------------------------------------------------------------------
// C: STOCHRSI exercises full generate with malloc/free/cross-calls
// ---------------------------------------------------------------------------

#[test]
fn c_stochrsi_full_generate() {
    let (func, enums) = load_indicator("stochrsi");
    let out = generate_all(&func, &enums);

    // C should have malloc and free
    assert!(
        out.c.contains("malloc("),
        "C STOCHRSI should contain malloc"
    );
    assert!(
        out.c.contains("free("),
        "C STOCHRSI should contain free"
    );

    // Java should have new array and no free
    assert!(
        out.java.contains("new double["),
        "Java STOCHRSI should use new double[]"
    );
    assert!(
        !out.java.contains("free("),
        "Java STOCHRSI should not contain free"
    );
}

// ---------------------------------------------------------------------------
// Java: Assign to _ with free() should be empty (exercises lines 756-758)
// ---------------------------------------------------------------------------

/// See `rust_free_never_reaches_the_renderer` — same mechanism, same reason.
#[test]
fn java_free_never_reaches_the_renderer() {
    let (func, enums) = load_indicator("stoch");
    let out = generate_all(&func, &enums);
    assert!(!out.java.contains("free("), "a free() survived into the Java output");
}

// ---------------------------------------------------------------------------
// Java: BinOp with single_precision float input params (lines 1347-1357)
// ---------------------------------------------------------------------------

#[test]
fn java_single_precision_eq_comparison_optimization() {
    let enums = HashMap::new();
    let registry = make_registry();
    let helpers = HelperRegistry::empty();
    let inline_counter = std::cell::Cell::new(0);
    let address_of_vars = std::collections::HashSet::new();
    let double_address_of_vars = std::collections::HashSet::new();
    let mut float_input_params = std::collections::HashSet::new();
    float_input_params.insert("inReal".to_string());

    // When comparing a float input param with a non-float param using ==,
    // it should render as "false" since they can never alias
    let stmt = ir::Statement::If {
        condition: ir::Expr::BinOp(
            Box::new(ir::Expr::Var("inReal".to_string())),
            ir::BinOp::Eq,
            Box::new(ir::Expr::Var("outReal".to_string())),
        ),
        then_body: vec![ir::Statement::Assign {
            target: ir::Expr::Var("x".to_string()),
            value: ir::Expr::IntLiteral(1),
            compound: false,
        }],
        else_body: vec![],
        cond_comments: vec![],
    };

    let rendered = backends::java::render_statement(
        &stmt, 0, true, &enums, &registry, &helpers, &inline_counter,
        &address_of_vars, &double_address_of_vars, &float_input_params,
    );
    assert!(
        rendered.contains("false"),
        "Java single precision == comparison of float vs non-float should be 'false': {rendered}"
    );
}

// ---------------------------------------------------------------------------
// Java: PointerDeref with double_address_of_vars (lines 1412-1416)
// ---------------------------------------------------------------------------

#[test]
fn java_pointer_deref_double_address_of() {
    let enums = HashMap::new();
    let registry = make_registry();
    let helpers = HelperRegistry::empty();
    let inline_counter = std::cell::Cell::new(0);
    let address_of_vars = std::collections::HashSet::new();
    let mut double_address_of_vars = std::collections::HashSet::new();
    double_address_of_vars.insert("myBuf".to_string());
    let float_input_params = std::collections::HashSet::new();

    let stmt = ir::Statement::Assign {
        target: ir::Expr::Var("x".to_string()),
        value: ir::Expr::PointerDeref("myBuf".to_string()),
        compound: false,
    };
    let rendered = backends::java::render_statement(
        &stmt, 0, false, &enums, &registry, &helpers, &inline_counter,
        &address_of_vars, &double_address_of_vars, &float_input_params,
    );
    assert!(
        rendered.contains("myBuf[0]"),
        "Java PointerDeref of double_address_of var should render as name[0]: {rendered}"
    );
}

// ---------------------------------------------------------------------------
// Java: Var in address_of_vars renders with .value (lines 1327-1328)
// ---------------------------------------------------------------------------

#[test]
fn java_var_address_of_renders_dot_value() {
    let enums = HashMap::new();
    let registry = make_registry();
    let helpers = HelperRegistry::empty();
    let inline_counter = std::cell::Cell::new(0);
    let mut address_of_vars = std::collections::HashSet::new();
    address_of_vars.insert("outBegIdx1".to_string());
    let double_address_of_vars = std::collections::HashSet::new();
    let float_input_params = std::collections::HashSet::new();

    let stmt = ir::Statement::Assign {
        target: ir::Expr::Var("x".to_string()),
        value: ir::Expr::Var("outBegIdx1".to_string()),
        compound: false,
    };
    let rendered = backends::java::render_statement(
        &stmt, 0, false, &enums, &registry, &helpers, &inline_counter,
        &address_of_vars, &double_address_of_vars, &float_input_params,
    );
    assert!(
        rendered.contains("outBegIdx1.value"),
        "Java Var in address_of_vars should render as name.value: {rendered}"
    );
}

// ---------------------------------------------------------------------------
// Java: Var in double_address_of_vars renders with [0] (lines 1329-1330)
// ---------------------------------------------------------------------------

#[test]
fn java_var_double_address_of_renders_bracket_zero() {
    let enums = HashMap::new();
    let registry = make_registry();
    let helpers = HelperRegistry::empty();
    let inline_counter = std::cell::Cell::new(0);
    let address_of_vars = std::collections::HashSet::new();
    let mut double_address_of_vars = std::collections::HashSet::new();
    double_address_of_vars.insert("tempBuf".to_string());
    let float_input_params = std::collections::HashSet::new();

    let stmt = ir::Statement::Assign {
        target: ir::Expr::Var("x".to_string()),
        value: ir::Expr::Var("tempBuf".to_string()),
        compound: false,
    };
    let rendered = backends::java::render_statement(
        &stmt, 0, false, &enums, &registry, &helpers, &inline_counter,
        &address_of_vars, &double_address_of_vars, &float_input_params,
    );
    assert!(
        rendered.contains("tempBuf[0]"),
        "Java Var in double_address_of_vars should render as name[0]: {rendered}"
    );
}

// ---------------------------------------------------------------------------
// C: Enum/Compatibility variable rendering
// ---------------------------------------------------------------------------

#[test]
fn c_metastock_and_default_var_rendering() {
    let stmt1 = ir::Statement::Assign {
        target: ir::Expr::Var("x".to_string()),
        value: ir::Expr::Var("METASTOCK".to_string()),
        compound: false,
    };
    let rendered1 = render_c_stmt(&stmt1);
    assert!(
        rendered1.contains("TA_COMPATIBILITY_METASTOCK"),
        "C METASTOCK should render as the plain enumerator: {rendered1}"
    );

    let stmt2 = ir::Statement::Assign {
        target: ir::Expr::Var("x".to_string()),
        value: ir::Expr::Var("DEFAULT".to_string()),
        compound: false,
    };
    let rendered2 = render_c_stmt(&stmt2);
    assert!(
        rendered2.contains("TA_COMPATIBILITY_DEFAULT"),
        "C DEFAULT should render as the plain enumerator: {rendered2}"
    );
}

// ---------------------------------------------------------------------------
// C/Java: DoWhile rendering
// ---------------------------------------------------------------------------

#[test]
fn c_dowhile_renders_do_while() {
    let stmt = ir::Statement::DoWhile {
        condition: ir::Expr::BinOp(
            Box::new(ir::Expr::Var("x".to_string())),
            ir::BinOp::Greater,
            Box::new(ir::Expr::IntLiteral(0)),
        ),
        body: vec![ir::Statement::Assign {
            target: ir::Expr::Var("x".to_string()),
            value: ir::Expr::BinOp(
                Box::new(ir::Expr::Var("x".to_string())),
                ir::BinOp::Sub,
                Box::new(ir::Expr::IntLiteral(1)),
            ),
            compound: false,
        }],
    };
    let rendered = render_c_stmt(&stmt);
    assert!(
        rendered.contains("do") && rendered.contains("while"),
        "C DoWhile should render as do...while: {rendered}"
    );
}

#[test]
fn java_dowhile_renders_do_while() {
    let stmt = ir::Statement::DoWhile {
        condition: ir::Expr::BinOp(
            Box::new(ir::Expr::Var("x".to_string())),
            ir::BinOp::Greater,
            Box::new(ir::Expr::IntLiteral(0)),
        ),
        body: vec![ir::Statement::Assign {
            target: ir::Expr::Var("x".to_string()),
            value: ir::Expr::BinOp(
                Box::new(ir::Expr::Var("x".to_string())),
                ir::BinOp::Sub,
                Box::new(ir::Expr::IntLiteral(1)),
            ),
            compound: false,
        }],
    };
    let rendered = render_java_stmt(&stmt);
    assert!(
        rendered.contains("do") && rendered.contains("while"),
        "Java DoWhile should render as do...while: {rendered}"
    );
}

// ---------------------------------------------------------------------------
// C/Java: While rendering
// ---------------------------------------------------------------------------

#[test]
fn c_while_renders_correctly() {
    let stmt = ir::Statement::While {
        condition: ir::Expr::BinOp(
            Box::new(ir::Expr::Var("i".to_string())),
            ir::BinOp::Less,
            Box::new(ir::Expr::Var("n".to_string())),
        ),
        body: vec![ir::Statement::Assign {
            target: ir::Expr::Var("i".to_string()),
            value: ir::Expr::PostIncrement(Box::new(ir::Expr::Var("i".to_string()))),
            compound: false,
        }],
    };
    let rendered = render_c_stmt(&stmt);
    assert!(
        rendered.contains("while(") || rendered.contains("while ("),
        "C While should render as while(...): {rendered}"
    );
}

#[test]
fn java_while_renders_correctly() {
    let stmt = ir::Statement::While {
        condition: ir::Expr::BinOp(
            Box::new(ir::Expr::Var("i".to_string())),
            ir::BinOp::Less,
            Box::new(ir::Expr::Var("n".to_string())),
        ),
        body: vec![ir::Statement::Assign {
            target: ir::Expr::Var("i".to_string()),
            value: ir::Expr::PostIncrement(Box::new(ir::Expr::Var("i".to_string()))),
            compound: false,
        }],
    };
    let rendered = render_java_stmt(&stmt);
    assert!(
        rendered.contains("while(") || rendered.contains("while ("),
        "Java While should render as while(...): {rendered}"
    );
}

// ---------------------------------------------------------------------------
// C/Java: Break and Continue rendering
// ---------------------------------------------------------------------------

#[test]
fn c_break_and_continue() {
    let break_rendered = render_c_stmt(&ir::Statement::Break);
    assert!(
        break_rendered.contains("break;"),
        "C Break should render as 'break;': {break_rendered}"
    );

    let continue_rendered = render_c_stmt(&ir::Statement::Continue);
    assert!(
        continue_rendered.contains("continue;"),
        "C Continue should render as 'continue;': {continue_rendered}"
    );
}

#[test]
fn java_break_and_continue() {
    let break_rendered = render_java_stmt(&ir::Statement::Break);
    assert!(
        break_rendered.contains("break;"),
        "Java Break should render as 'break;': {break_rendered}"
    );

    let continue_rendered = render_java_stmt(&ir::Statement::Continue);
    assert!(
        continue_rendered.contains("continue;"),
        "Java Continue should render as 'continue;': {continue_rendered}"
    );
}

// ---------------------------------------------------------------------------
// C/Java: Switch rendering via render_statement
// ---------------------------------------------------------------------------

#[test]
fn c_switch_renders_with_cases() {
    let stmt = ir::Statement::Switch {
        expr: ir::Expr::Var("mode".to_string()),
        cases: vec![
            (
                "0".to_string(),
                vec![ir::Statement::Assign {
                    target: ir::Expr::Var("x".to_string()),
                    value: ir::Expr::IntLiteral(1),
                    compound: false,
                }],
            ),
            (
                "1".to_string(),
                vec![ir::Statement::Assign {
                    target: ir::Expr::Var("x".to_string()),
                    value: ir::Expr::IntLiteral(2),
                    compound: false,
                }],
            ),
        ],
        default: vec![ir::Statement::Assign {
            target: ir::Expr::Var("x".to_string()),
            value: ir::Expr::IntLiteral(0),
            compound: false,
        }],
    };
    let rendered = render_c_stmt(&stmt);
    // Switch with all cases assigning to same target renders as ternary chain
    assert!(
        rendered.contains("mode==0") && rendered.contains("mode==1"),
        "Simple switch should render as ternary chain: {rendered}"
    );
    assert!(
        rendered.contains("x ="),
        "Ternary should assign to target variable: {rendered}"
    );
    // Default case is the innermost fallback in the ternary chain
    assert!(
        rendered.contains("(0)") || rendered.contains("default:"),
        "Should have default value in ternary or default label: {rendered}"
    );
    // Ternary rendering doesn't need break statements
    assert!(
        rendered.contains("break;") || rendered.contains("?"),
        "Should have break (switch) or ternary operator: {rendered}"
    );
}

#[test]
fn java_switch_renders_with_cases() {
    let stmt = ir::Statement::Switch {
        expr: ir::Expr::Var("mode".to_string()),
        cases: vec![
            (
                "0".to_string(),
                vec![ir::Statement::Assign {
                    target: ir::Expr::Var("x".to_string()),
                    value: ir::Expr::IntLiteral(1),
                    compound: false,
                }],
            ),
        ],
        default: vec![ir::Statement::Assign {
            target: ir::Expr::Var("x".to_string()),
            value: ir::Expr::IntLiteral(0),
            compound: false,
        }],
    };
    let rendered = render_java_stmt(&stmt);
    assert!(
        rendered.contains("switch(") || rendered.contains("switch ("),
        "Java Switch should render as switch(): {rendered}"
    );
    assert!(
        rendered.contains("default:"),
        "Java Switch should have default label: {rendered}"
    );
}

// ---------------------------------------------------------------------------
// C/Java: If-else rendering
// ---------------------------------------------------------------------------

#[test]
fn c_if_else_rendering() {
    let stmt = ir::Statement::If {
        condition: ir::Expr::BinOp(
            Box::new(ir::Expr::Var("x".to_string())),
            ir::BinOp::Greater,
            Box::new(ir::Expr::IntLiteral(0)),
        ),
        then_body: vec![ir::Statement::Assign {
            target: ir::Expr::Var("y".to_string()),
            value: ir::Expr::IntLiteral(1),
            compound: false,
        }],
        else_body: vec![ir::Statement::Assign {
            target: ir::Expr::Var("y".to_string()),
            value: ir::Expr::IntLiteral(0),
            compound: false,
        }],
        cond_comments: vec![],
    };
    let rendered = render_c_stmt(&stmt);
    assert!(
        rendered.contains("if(") || rendered.contains("if ("),
        "C If should render as if(): {rendered}"
    );
    assert!(
        rendered.contains("else"),
        "C If with else_body should contain 'else': {rendered}"
    );
}

#[test]
fn java_if_else_rendering() {
    let stmt = ir::Statement::If {
        condition: ir::Expr::BinOp(
            Box::new(ir::Expr::Var("x".to_string())),
            ir::BinOp::Greater,
            Box::new(ir::Expr::IntLiteral(0)),
        ),
        then_body: vec![ir::Statement::Assign {
            target: ir::Expr::Var("y".to_string()),
            value: ir::Expr::IntLiteral(1),
            compound: false,
        }],
        else_body: vec![ir::Statement::Assign {
            target: ir::Expr::Var("y".to_string()),
            value: ir::Expr::IntLiteral(0),
            compound: false,
        }],
        cond_comments: vec![],
    };
    let rendered = render_java_stmt(&stmt);
    assert!(
        rendered.contains("if(") || rendered.contains("if ("),
        "Java If should render: {rendered}"
    );
    assert!(
        rendered.contains("else"),
        "Java If with else_body should contain 'else': {rendered}"
    );
}

// ---------------------------------------------------------------------------
// Java: If-else-if chain rendering
// ---------------------------------------------------------------------------

#[test]
fn java_if_else_if_chain() {
    let stmt = ir::Statement::If {
        condition: ir::Expr::BinOp(
            Box::new(ir::Expr::Var("x".to_string())),
            ir::BinOp::Greater,
            Box::new(ir::Expr::IntLiteral(0)),
        ),
        then_body: vec![ir::Statement::Assign {
            target: ir::Expr::Var("y".to_string()),
            value: ir::Expr::IntLiteral(1),
            compound: false,
        }],
        else_body: vec![ir::Statement::If {
            condition: ir::Expr::BinOp(
                Box::new(ir::Expr::Var("x".to_string())),
                ir::BinOp::Less,
                Box::new(ir::Expr::IntLiteral(0)),
            ),
            then_body: vec![ir::Statement::Assign {
                target: ir::Expr::Var("y".to_string()),
                value: ir::Expr::IntLiteral(-1),
                compound: false,
            }],
            else_body: vec![],
            cond_comments: vec![],
        }],
        cond_comments: vec![],
    };
    let rendered = render_java_stmt(&stmt);
    // Should chain as "} else if(" not "} else {\n  if("
    assert!(
        rendered.contains("} else if(") || rendered.contains("} else if ("),
        "Java if-else-if should chain without extra braces: {rendered}"
    );
}

// ---------------------------------------------------------------------------
// Java: Compound assignment rendering
// ---------------------------------------------------------------------------

#[test]
fn java_compound_assignment() {
    let stmt = ir::Statement::Assign {
        target: ir::Expr::Var("x".to_string()),
        value: ir::Expr::BinOp(
            Box::new(ir::Expr::Var("x".to_string())),
            ir::BinOp::Add,
            Box::new(ir::Expr::Literal(1.0)),
        ),
        compound: true,
    };
    let rendered = render_java_stmt(&stmt);
    assert!(
        rendered.contains("x += 1.0"),
        "Java compound assignment should render as x += 1.0: {rendered}"
    );
}

#[test]
fn c_compound_assignment() {
    let stmt = ir::Statement::Assign {
        target: ir::Expr::Var("x".to_string()),
        value: ir::Expr::BinOp(
            Box::new(ir::Expr::Var("x".to_string())),
            ir::BinOp::Sub,
            Box::new(ir::Expr::Literal(2.0)),
        ),
        compound: true,
    };
    let rendered = render_c_stmt(&stmt);
    assert!(
        rendered.contains("x -= 2.0"),
        "C compound assignment should render as x -= 2.0: {rendered}"
    );
}

// ---------------------------------------------------------------------------
// Java/C: Literal and IntLiteral rendering
// ---------------------------------------------------------------------------

#[test]
fn java_literal_rendering() {
    // Whole number literals should render as N.0
    let stmt = ir::Statement::Assign {
        target: ir::Expr::Var("x".to_string()),
        value: ir::Expr::Literal(42.0),
        compound: false,
    };
    let rendered = render_java_stmt(&stmt);
    assert!(
        rendered.contains("42.0"),
        "Java whole number literal should render as 42.0: {rendered}"
    );

    // Non-whole number should render as-is
    let stmt2 = ir::Statement::Assign {
        target: ir::Expr::Var("x".to_string()),
        value: ir::Expr::Literal(2.71),
        compound: false,
    };
    let rendered2 = render_java_stmt(&stmt2);
    assert!(
        rendered2.contains("2.71"),
        "Java non-whole literal should render as 2.71: {rendered2}"
    );
}

#[test]
fn java_int_literal_rendering() {
    let stmt = ir::Statement::Assign {
        target: ir::Expr::Var("x".to_string()),
        value: ir::Expr::IntLiteral(42),
        compound: false,
    };
    let rendered = render_java_stmt(&stmt);
    assert!(
        rendered.contains("42"),
        "Java IntLiteral should render as 42: {rendered}"
    );
    // Should NOT have a decimal point
    assert!(
        !rendered.contains("42.0"),
        "Java IntLiteral should NOT have decimal point: {rendered}"
    );
}

// ---------------------------------------------------------------------------
// Java: ArrayAccess rendering
// ---------------------------------------------------------------------------

#[test]
fn java_array_access() {
    let stmt = ir::Statement::Assign {
        target: ir::Expr::Var("x".to_string()),
        value: ir::Expr::ArrayAccess(
            "inReal".to_string(),
            Box::new(ir::Expr::Var("i".to_string())),
        ),
        compound: false,
    };
    let rendered = render_java_stmt(&stmt);
    assert!(
        rendered.contains("inReal[i]"),
        "Java ArrayAccess should render as arr[idx]: {rendered}"
    );
}

// ---------------------------------------------------------------------------
