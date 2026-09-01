//! Java backend: unit tests of `render_statement` and related rendering
//! branches. Split out of the former `backend_suite.rs`.

#[path = "common/mod.rs"]
mod common;

use common::{contains_call, generate_all, load_indicator, render_java_stmt};
use ta_codegen_lib::ir;

// Java backend coverage tests
// ===========================================================================

// ---------------------------------------------------------------------------
// Java: VarDecl rendering for all VarType variants via render_statement
// ---------------------------------------------------------------------------

#[test]
fn java_vardecl_retcode_type() {
    let stmt = ir::Statement::VarDecl {
        var_type: ir::VarType::RetCodeType,
        name: "retCode".to_string(),
        init: None,
    };
    let rendered = render_java_stmt(&stmt);
    assert!(
        rendered.contains("RetCode retCode"),
        "Java VarDecl RetCodeType should render as 'RetCode retCode': {rendered}"
    );
}

#[test]
fn java_vardecl_real_pointer() {
    let stmt = ir::Statement::VarDecl {
        var_type: ir::VarType::RealPointer,
        name: "buf".to_string(),
        init: None,
    };
    let rendered = render_java_stmt(&stmt);
    assert!(
        rendered.contains("double[] buf"),
        "Java VarDecl RealPointer should render as 'double[] buf': {rendered}"
    );
}

#[test]
fn java_vardecl_int_pointer() {
    let stmt = ir::Statement::VarDecl {
        var_type: ir::VarType::IntPointer,
        name: "indices".to_string(),
        init: None,
    };
    let rendered = render_java_stmt(&stmt);
    assert!(
        rendered.contains("int[] indices"),
        "Java VarDecl IntPointer should render as 'int[] indices': {rendered}"
    );
}

#[test]
fn java_vardecl_real_array() {
    let stmt = ir::Statement::VarDecl {
        var_type: ir::VarType::RealArray("30".to_string()),
        name: "arr".to_string(),
        init: None,
    };
    let rendered = render_java_stmt(&stmt);
    assert!(
        rendered.contains("double[] arr = new double[30]"),
        "Java VarDecl RealArray should render as 'double[] arr = new double[30]': {rendered}"
    );
}

#[test]
fn java_vardecl_int_array() {
    let stmt = ir::Statement::VarDecl {
        var_type: ir::VarType::IntArray("5".to_string()),
        name: "flags".to_string(),
        init: None,
    };
    let rendered = render_java_stmt(&stmt);
    assert!(
        rendered.contains("int[] flags = new int[5]"),
        "Java VarDecl IntArray should render as 'int[] flags = new int[5]': {rendered}"
    );
}

#[test]
fn java_vardecl_with_init_expr() {
    let stmt = ir::Statement::VarDecl {
        var_type: ir::VarType::Real,
        name: "total".to_string(),
        init: Some(ir::Expr::Literal(2.71)),
    };
    let rendered = render_java_stmt(&stmt);
    assert!(
        rendered.contains("double total = 2.71"),
        "Java VarDecl with init should render the init expression: {rendered}"
    );
}

// ---------------------------------------------------------------------------
// Java: Return None renders 'return ;'
// ---------------------------------------------------------------------------

#[test]
fn java_return_none() {
    let stmt = ir::Statement::Return { value: None };
    let rendered = render_java_stmt(&stmt);
    assert!(
        rendered.contains("return ;"),
        "Java Return None should render as 'return ;': {rendered}"
    );
}

// ---------------------------------------------------------------------------
// Java: For countdown loop rendering
// ---------------------------------------------------------------------------

#[test]
fn java_for_countdown_loop() {
    let stmt = ir::Statement::For {
        var: "i".to_string(),
        count: ir::Expr::Var("optInTimePeriod".to_string()),
        body: vec![ir::Statement::Assign {
            target: ir::Expr::Var("tempReal".to_string()),
            value: ir::Expr::Literal(1.0),
            compound: false,
        }],
    };
    let rendered = render_java_stmt(&stmt);
    assert!(
        rendered.contains("for( i = optInTimePeriod; i > 0; i-- )"),
        "Java For countdown should render as 'for( i = count; i > 0; i-- )': {rendered}"
    );
    assert!(
        rendered.contains("tempReal = 1.0"),
        "Java For countdown body should be rendered: {rendered}"
    );
}

// ---------------------------------------------------------------------------
// Java: Block statement with VarDecls exercises lines 1085-1120
// ---------------------------------------------------------------------------

#[test]
fn java_block_statement_with_vardecls() {
    let stmt = ir::Statement::Block {
        body: vec![
            ir::Statement::VarDecl {
                var_type: ir::VarType::RetCodeType,
                name: "rc".to_string(),
                init: None,
            },
            ir::Statement::VarDecl {
                var_type: ir::VarType::RealPointer,
                name: "buf".to_string(),
                init: None,
            },
            ir::Statement::VarDecl {
                var_type: ir::VarType::IntPointer,
                name: "idx".to_string(),
                init: None,
            },
            ir::Statement::VarDecl {
                var_type: ir::VarType::RealArray("10".to_string()),
                name: "darr".to_string(),
                init: None,
            },
            ir::Statement::VarDecl {
                var_type: ir::VarType::IntArray("3".to_string()),
                name: "iarr".to_string(),
                init: None,
            },
            ir::Statement::VarDecl {
                var_type: ir::VarType::Real,
                name: "x".to_string(),
                init: Some(ir::Expr::Literal(42.0)),
            },
            ir::Statement::Assign {
                target: ir::Expr::Var("x".to_string()),
                value: ir::Expr::Literal(99.0),
                compound: false,
            },
        ],
    };
    let rendered = render_java_stmt(&stmt);
    // Block VarDecl declarations should appear
    assert!(
        rendered.contains("RetCode rc"),
        "Block should declare RetCode: {rendered}"
    );
    assert!(
        rendered.contains("double[] buf"),
        "Block should declare double[]: {rendered}"
    );
    assert!(
        rendered.contains("int[] idx"),
        "Block should declare int[]: {rendered}"
    );
    assert!(
        rendered.contains("double[] darr = new double[10]"),
        "Block should declare RealArray: {rendered}"
    );
    assert!(
        rendered.contains("int[] iarr = new int[3]"),
        "Block should declare IntArray: {rendered}"
    );
    assert!(
        rendered.contains("double x = 42.0"),
        "Block should declare VarDecl with init: {rendered}"
    );
    // Non-VarDecl statements should also render
    assert!(
        rendered.contains("x = 99.0"),
        "Block should render non-VarDecl statements: {rendered}"
    );
}

// ---------------------------------------------------------------------------
// Java: ForC rendering exercises lines 1035-1083
// ---------------------------------------------------------------------------

#[test]
fn java_forc_single_init_renders_correctly() {
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
    let rendered = render_java_stmt(&stmt);
    assert!(
        rendered.contains("for("),
        "Java ForC should render as for(): {rendered}"
    );
    assert!(
        rendered.contains("i < n"),
        "Java ForC should render condition: {rendered}"
    );
}

// ---------------------------------------------------------------------------
// Java: STOCH exercises malloc/free/memcpy; MA exercises cross-indicator calls
// ---------------------------------------------------------------------------

#[test]
fn java_stoch_malloc_renders_as_new_array() {
    // STOCH mallocs a temp %K buffer, memcpy's it into the caller buffer, and
    // frees it. (MACD was the original vehicle, but its lockstep fusion removed
    // the temp buffers.)
    let (func, enums) = load_indicator("stoch");
    let out = generate_all(&func, &enums);
    let j = &out.java;

    // malloc should become new double[] or new int[] in Java
    assert!(
        j.contains("new double["),
        "Java STOCH should render malloc as new double[]: {j}"
    );
    // free should be removed (no-op in Java)
    assert!(
        !j.contains("free("),
        "Java STOCH should not contain free() calls"
    );
    // memcpy should become System.arraycopy
    assert!(
        j.contains("System.arraycopy("),
        "Java STOCH should render memcpy as System.arraycopy(): {j}"
    );
}

#[test]
fn java_ma_cross_indicator_calls() {
    // MA dispatches to the per-type moving averages via the guarded internal cores.
    // (MACD was the original vehicle, but its lockstep fusion removed the EMA
    // calls.)
    let (func, enums) = load_indicator("ma");
    let out = generate_all(&func, &enums);
    let j = &out.java;

    // Anchor the call site so the adjacent dispatch arms cannot substring-shadow
    // the EMA one. The callee is the PUBLIC entry point since #236 step 3, and
    // the range it returns is bound to the caller's out-params.
    assert!(
        j.contains("= EMA("),
        "Java MA should call the public EMA(): {j}"
    );
    assert!(
        j.contains("= EMA_Lookback("),
        "Java MA should call EMA_Lookback(): {j}"
    );
}

// ---------------------------------------------------------------------------
// Java: STOCHRSI exercises cross-indicator calls with MAType enum
// ---------------------------------------------------------------------------

#[test]
fn java_stochrsi_cross_indicator_calls() {
    let (func, enums) = load_indicator("stochrsi");
    let out = generate_all(&func, &enums);
    let j = &out.java;

    // STOCHRSI composes RSI and STOCHF, and must call BOTH. `contains_call`
    // rather than `contains`: `RSI_Lookback(` is a suffix of STOCHRSI's own
    // `STOCHRSI_Lookback(`, so a plain substring test cannot fail here.
    // Anchored on the ASSIGNMENT, not the bare name: `RSI(` occurs inside
    // STOCHRSI's own javadoc and inside `STOCHRSI(`, so `contains_call(j, "RSI")`
    // is satisfied by text that is not a call at all and cannot fail.
    assert!(
        j.contains("= RSI(") && contains_call(j, "RSI_Lookback"),
        "Java STOCHRSI should call the public RSI and RSI_Lookback: {j}"
    );
    assert!(
        j.contains("= STOCHF(") && contains_call(j, "STOCHF_Lookback"),
        "Java STOCHRSI should call the public STOCHF and STOCHF_Lookback: {j}"
    );
}

// ---------------------------------------------------------------------------
// Java: T3 exercises For countdown loop (real indicator)
// ---------------------------------------------------------------------------

#[test]
fn java_t3_for_countdown_loops() {
    let (func, enums) = load_indicator("t3");
    let out = generate_all(&func, &enums);
    let j = &out.java;

    // T3 uses multiple for(i=period-1; i>0; i--) loops (rendered as i -= 1)
    assert!(
        j.contains("i > 0; i -= 1"),
        "Java T3 should contain countdown for loops: {j}"
    );
}

// ---------------------------------------------------------------------------
// Java: MA switch statement exercises MAType variable rendering
// ---------------------------------------------------------------------------

#[test]
fn java_ma_switch_variable_rendering() {
    let (func, enums) = load_indicator("ma");
    let out = generate_all(&func, &enums);
    let j = &out.java;

    // MA's switch should use the optInMAType variable
    assert!(
        j.contains("switch(") || j.contains("switch ("),
        "Java MA should contain switch: {j}"
    );
    // Should render enum cases with UNQUALIFIED labels (pre-Java-21 compatible)
    assert!(
        j.contains("case SMA:") || j.contains("case EMA:"),
        "Java MA should use unqualified enum case labels in switch: {j}"
    );
    assert!(
        !j.contains("case MAType."),
        "Java switch case labels must not be qualified (Java 21+ only): {j}"
    );
}

// ---------------------------------------------------------------------------
// Java: Assign to _ target (statement expression) exercises lines 736-761
// ---------------------------------------------------------------------------

#[test]
fn java_assign_to_underscore_skips_bare_var() {
    // Expr(someVar) should produce empty output (no side effects)
    let stmt = ir::Statement::Expr(ir::Expr::Var("someVar".to_string()));
    let rendered = render_java_stmt(&stmt);
    assert!(
        rendered.is_empty(),
        "Statement expression with bare Var should produce empty output: '{rendered}'"
    );
}

#[test]
fn java_assign_to_underscore_renders_func_call() {
    // Expr(someFunc(x)) should render as someFunc(x);
    let stmt = ir::Statement::Expr(ir::Expr::FuncCall(
        "someFunc".to_string(),
        vec![ir::Expr::Var("x".to_string())],
    ));
    let rendered = render_java_stmt(&stmt);
    // Should render the function call as a statement
    assert!(
        rendered.contains("someFunc("),
        "Statement expression with FuncCall should render the call: {rendered}"
    );
}

// ---------------------------------------------------------------------------
// Java: outBegIdx/outNBElement scalar assignment exercises lines 764-773
// ---------------------------------------------------------------------------

#[test]
fn java_output_scalar_assignment() {
    let stmt = ir::Statement::Assign {
        target: ir::Expr::Var("outBegIdx".to_string()),
        value: ir::Expr::IntLiteral(0),
        compound: false,
    };
    let rendered = render_java_stmt(&stmt);
    assert!(
        rendered.contains("outBegIdx.value = 0"),
        "Java outBegIdx assignment should use .value: {rendered}"
    );

    let stmt2 = ir::Statement::Assign {
        target: ir::Expr::Var("outNBElement".to_string()),
        value: ir::Expr::IntLiteral(0),
        compound: false,
    };
    let rendered2 = render_java_stmt(&stmt2);
    assert!(
        rendered2.contains("outNBElement.value = 0"),
        "Java outNBElement assignment should use .value: {rendered2}"
    );
}

// ---------------------------------------------------------------------------
// Java: Ternary expression rendering exercises lines 1450-1468
// ---------------------------------------------------------------------------

/// `(cond) ? 1 : 0` collapses to the bare condition — in BOOLEAN position.
///
/// The vehicle is an `if`, not an assignment. It used to be an assignment, which
/// pinned the collapse in the one position where it is wrong: C has no booleans,
/// so an assignment's destination is always an `int`, and `x = a > b;` does not
/// compile in Java (#262). The collapse rule itself is unchanged and still what
/// this asserts.
#[test]
fn java_ternary_bool_to_int_optimization() {
    let flag = ir::Expr::Ternary(
        Box::new(ir::Expr::BinOp(
            Box::new(ir::Expr::Var("a".to_string())),
            ir::BinOp::Greater,
            Box::new(ir::Expr::Var("b".to_string())),
        )),
        Box::new(ir::Expr::IntLiteral(1)),
        Box::new(ir::Expr::IntLiteral(0)),
    );
    let rendered = render_java_stmt(&ir::Statement::If {
        condition: flag.clone(),
        then_body: vec![],
        else_body: vec![],
        cond_comments: vec![],
    });
    assert!(
        !rendered.contains('?'),
        "Java ternary (cond)?1:0 in an `if` should simplify to just cond: {rendered}"
    );
    assert!(
        rendered.contains("a > b"),
        "Java ternary should contain the condition directly: {rendered}"
    );

    // The other half of the same rule: stored, it keeps the int form.
    let stored = render_java_stmt(&ir::Statement::Assign {
        target: ir::Expr::Var("x".to_string()),
        value: flag,
        compound: false,
    });
    assert!(
        stored.contains("? 1 : 0"),
        "an assignment's destination is an int, so the ternary must survive: {stored}"
    );
}

/// `(cond) ? 0 : 1` collapses to `!(cond)` — again, in boolean position only.
#[test]
fn java_ternary_inverted_bool_optimization() {
    let flag = ir::Expr::Ternary(
        Box::new(ir::Expr::BinOp(
            Box::new(ir::Expr::Var("a".to_string())),
            ir::BinOp::Less,
            Box::new(ir::Expr::Var("b".to_string())),
        )),
        Box::new(ir::Expr::IntLiteral(0)),
        Box::new(ir::Expr::IntLiteral(1)),
    );
    let rendered = render_java_stmt(&ir::Statement::If {
        condition: flag.clone(),
        then_body: vec![],
        else_body: vec![],
        cond_comments: vec![],
    });
    assert!(
        rendered.contains("!("),
        "Java ternary (cond)?0:1 in an `if` should simplify to !(cond): {rendered}"
    );

    let stored = render_java_stmt(&ir::Statement::Assign {
        target: ir::Expr::Var("x".to_string()),
        value: flag,
        compound: false,
    });
    assert!(
        stored.contains("? 0 : 1"),
        "an assignment's destination is an int, so the ternary must survive: {stored}"
    );
}

#[test]
fn java_ternary_general_case() {
    // General ternary: (cond) ? a : b
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
    let rendered = render_java_stmt(&stmt);
    assert!(
        rendered.contains("?") && rendered.contains(":"),
        "Java general ternary should render as (cond) ? (then) : (else): {rendered}"
    );
}

// ---------------------------------------------------------------------------
// Java: Cast expression rendering exercises lines 1385-1398
// ---------------------------------------------------------------------------

#[test]
fn java_cast_expression_types() {
    // Cast to Integer
    let stmt = ir::Statement::Assign {
        target: ir::Expr::Var("x".to_string()),
        value: ir::Expr::Cast(
            ir::VarType::Integer,
            Box::new(ir::Expr::Literal(2.71)),
        ),
        compound: false,
    };
    let rendered = render_java_stmt(&stmt);
    assert!(
        rendered.contains("(int)2.71"),
        "Java Cast to Integer should render as (int)...: {rendered}"
    );

    // Cast to RetCodeType
    let stmt2 = ir::Statement::Assign {
        target: ir::Expr::Var("rc".to_string()),
        value: ir::Expr::Cast(
            ir::VarType::RetCodeType,
            Box::new(ir::Expr::IntLiteral(0)),
        ),
        compound: false,
    };
    let rendered2 = render_java_stmt(&stmt2);
    assert!(
        rendered2.contains("(RetCode)0"),
        "Java Cast to RetCodeType should render as (RetCode)...: {rendered2}"
    );
}

// ---------------------------------------------------------------------------
// Java: PointerDeref and AddressOf expression rendering
// ---------------------------------------------------------------------------

#[test]
fn java_pointer_deref_renders_as_value() {
    let stmt = ir::Statement::Assign {
        target: ir::Expr::Var("x".to_string()),
        value: ir::Expr::PointerDeref("outBegIdx".to_string()),
        compound: false,
    };
    let rendered = render_java_stmt(&stmt);
    assert!(
        rendered.contains("outBegIdx.value"),
        "Java PointerDeref should render as .value: {rendered}"
    );
}

#[test]
fn java_address_of_renders_inner() {
    let stmt = ir::Statement::Assign {
        target: ir::Expr::Var("x".to_string()),
        value: ir::Expr::AddressOf(Box::new(ir::Expr::Var("myVar".to_string()))),
        compound: false,
    };
    let rendered = render_java_stmt(&stmt);
    assert!(
        rendered.contains("myVar"),
        "Java AddressOf should render the inner expression: {rendered}"
    );
    // Should NOT have & prefix (Java has no address-of)
    assert!(
        !rendered.contains("&myVar"),
        "Java should not render & prefix: {rendered}"
    );
}

// ---------------------------------------------------------------------------
// Java: PostIncrement/PostDecrement/PreIncrement/PreDecrement
// ---------------------------------------------------------------------------

#[test]
fn java_increment_decrement_expressions() {
    let post_inc = ir::Statement::Assign {
        target: ir::Expr::Var("x".to_string()),
        value: ir::Expr::PostIncrement(Box::new(ir::Expr::Var("i".to_string()))),
        compound: false,
    };
    let rendered = render_java_stmt(&post_inc);
    assert!(
        rendered.contains("i++"),
        "Java PostIncrement should render as i++: {rendered}"
    );

    let pre_dec = ir::Statement::Assign {
        target: ir::Expr::Var("x".to_string()),
        value: ir::Expr::PreDecrement(Box::new(ir::Expr::Var("j".to_string()))),
        compound: false,
    };
    let rendered2 = render_java_stmt(&pre_dec);
    assert!(
        rendered2.contains("--j"),
        "Java PreDecrement should render as --j: {rendered2}"
    );
}

// ---------------------------------------------------------------------------
// Java: Not expression rendering
// ---------------------------------------------------------------------------

#[test]
fn java_not_expression() {
    let stmt = ir::Statement::Assign {
        target: ir::Expr::Var("x".to_string()),
        value: ir::Expr::Not(Box::new(ir::Expr::Var("flag".to_string()))),
        compound: false,
    };
    let rendered = render_java_stmt(&stmt);
    assert!(
        rendered.contains("!flag"),
        "Java Not expression should render as !expr: {rendered}"
    );
}

// ===========================================================================
