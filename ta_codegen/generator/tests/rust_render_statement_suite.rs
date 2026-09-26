//! Rust backend: unit tests of `render_statement` and related rendering
//! branches. Split out of the former `backend_suite.rs`.

#[path = "common/mod.rs"]
mod common;

use common::{
    extract_section, generate_all, load_indicator, load_synth, make_registry,
    make_synth_registry, make_helpers,
};
use std::collections::HashMap;
use std::path::Path;
use ta_codegen_lib::backends;
use ta_codegen_lib::helper_registry::HelperRegistry;
use ta_codegen_lib::ir;
use ta_codegen_lib::parser;

// Rust render_statement unit tests for uncovered branches
// ---------------------------------------------------------------------------

/// Helper to build a RustRenderCtx and call render_statement with minimal boilerplate.
fn render_rust_stmt(stmt: &ir::Statement) -> String {
    render_rust_stmt_with_ctx(stmt, &backends::rust_lang::RustRenderCtx::empty())
}

fn render_rust_stmt_with_ctx(
    stmt: &ir::Statement,
    ctx: &backends::rust_lang::RustRenderCtx,
) -> String {
    render_rust_stmt_with_helpers(stmt, ctx, &HelperRegistry::empty())
}

fn render_rust_stmt_with_helpers(
    stmt: &ir::Statement,
    ctx: &backends::rust_lang::RustRenderCtx,
    helpers: &HelperRegistry,
) -> String {
    let for_loop_vars: Vec<String> = vec![];
    let var_inits: std::collections::HashMap<String, &ir::Expr> =
        std::collections::HashMap::new();
    let output_names: Vec<String> = vec![];
    let opt_real_params: Vec<String> = vec![];
    let enums = HashMap::new();
    let registry = make_registry();
    let inline_counter = std::cell::Cell::new(0);

    backends::rust_lang::render_statement(
        stmt,
        12, // indent > 8 so VarDecl at nested level is emitted
        ctx,
        &for_loop_vars,
        &var_inits,
        &output_names,
        &opt_real_params,
        &enums,
        &registry,
        helpers,
        &inline_counter,
    )
}

// ---------------------------------------------------------------------------
// 1. VarDecl types: IntPointer, RealPointer, RealArray, IntArray, RetCodeType
// ---------------------------------------------------------------------------

#[test]
fn rust_vardecl_int_pointer_renders_vec_i32() {
    let stmt = ir::Statement::VarDecl {
        var_type: ir::VarType::IntPointer,
        name: "buf".to_string(),
        init: None,
    };
    let rendered = render_rust_stmt(&stmt);
    assert!(
        rendered.contains("Vec<i32>"),
        "IntPointer VarDecl should render as Vec<i32>: {rendered}"
    );
    assert!(
        rendered.contains("Vec::new()"),
        "IntPointer VarDecl without init should default to Vec::new(): {rendered}"
    );
}

#[test]
fn rust_vardecl_real_pointer_renders_vec_f64() {
    let stmt = ir::Statement::VarDecl {
        var_type: ir::VarType::RealPointer,
        name: "buf".to_string(),
        init: None,
    };
    let rendered = render_rust_stmt(&stmt);
    assert!(
        rendered.contains("Vec<f64>"),
        "RealPointer VarDecl should render as Vec<f64>: {rendered}"
    );
    assert!(
        rendered.contains("Vec::new()"),
        "RealPointer VarDecl without init should default to Vec::new(): {rendered}"
    );
}

#[test]
fn rust_vardecl_real_array_renders_fixed_size() {
    let stmt = ir::Statement::VarDecl {
        var_type: ir::VarType::RealArray("30".to_string()),
        name: "arr".to_string(),
        init: None,
    };
    let rendered = render_rust_stmt(&stmt);
    assert!(
        rendered.contains("[f64; 30 as usize]"),
        "RealArray VarDecl should render as [f64; N as usize]: {rendered}"
    );
    assert!(
        rendered.contains("0.0_f64"),
        "RealArray VarDecl should initialize with 0.0_f64: {rendered}"
    );
}

#[test]
fn rust_vardecl_int_array_renders_fixed_size() {
    let stmt = ir::Statement::VarDecl {
        var_type: ir::VarType::IntArray("5".to_string()),
        name: "flags".to_string(),
        init: None,
    };
    let rendered = render_rust_stmt(&stmt);
    assert!(
        rendered.contains("[i32; 5 as usize]"),
        "IntArray VarDecl should render as [i32; N as usize]: {rendered}"
    );
    assert!(
        rendered.contains("0i32"),
        "IntArray VarDecl should initialize with 0i32: {rendered}"
    );
}

#[test]
fn rust_vardecl_retcode_type_renders_retcode() {
    let stmt = ir::Statement::VarDecl {
        var_type: ir::VarType::RetCodeType,
        name: "retCode".to_string(),
        init: None,
    };
    let rendered = render_rust_stmt(&stmt);
    assert!(
        rendered.contains("RetCode"),
        "RetCodeType VarDecl should render as RetCode: {rendered}"
    );
    assert!(
        rendered.contains("RetCode::Success"),
        "RetCodeType VarDecl without init should default to RetCode::Success: {rendered}"
    );
}

#[test]
fn rust_compound_assign_casts_i32_param_into_inferred_usize_var() {
    // `trailingPos1` is usize only via subscript inference (ctx.index_vars) —
    // its name matches no index heuristic — and the RHS is an i32 optIn param.
    // Regression for the `usize -= i32` mismatch in PR #154's ULTOSC ring wraps.
    let mut ctx = backends::rust_lang::RustRenderCtx::empty();
    ctx.is_lookback = false;
    ctx.index_vars.insert("trailingPos1".to_string());
    let stmt = ir::Statement::Assign {
        target: ir::Expr::Var("trailingPos1".to_string()),
        value: ir::Expr::BinOp(
            Box::new(ir::Expr::Var("trailingPos1".to_string())),
            ir::BinOp::Sub,
            Box::new(ir::Expr::Var("optInTimePeriod3".to_string())),
        ),
        compound: true,
    };
    let rendered = render_rust_stmt_with_ctx(&stmt, &ctx);
    assert!(
        rendered.contains("trailingPos1 -= (optInTimePeriod3) as usize"),
        "compound assign into a subscript-inferred usize var must cast the i32 RHS: {rendered}"
    );

    // Ctx construction removes sentinels from index_vars, so in production a
    // sentinel (i32-rendered) reaches this gate only through the name
    // heuristic. Pin that arm: a heuristic-matched sentinel must stay uncast.
    let mut sctx = backends::rust_lang::RustRenderCtx::empty();
    sctx.is_lookback = false;
    sctx.sentinel_vars.insert("highestIdx".to_string());
    let sstmt = ir::Statement::Assign {
        target: ir::Expr::Var("highestIdx".to_string()),
        value: ir::Expr::BinOp(
            Box::new(ir::Expr::Var("highestIdx".to_string())),
            ir::BinOp::Sub,
            Box::new(ir::Expr::Var("optInTimePeriod3".to_string())),
        ),
        compound: true,
    };
    let rendered = render_rust_stmt_with_ctx(&sstmt, &sctx);
    assert!(
        rendered.contains("highestIdx -= optInTimePeriod3")
            && !rendered.contains("as usize"),
        "compound assign into a heuristic-named sentinel (i32) var must stay uncast: {rendered}"
    );
}

/// Issue #158: the mirror of the test above. A target the generator has typed
/// as an integer must never take the f64 RHS cast just because its name is on
/// no index list — and an i32 target with a usize RHS needs the third branch
/// (`as i32`) that used to be missing entirely.
#[test]
fn rust_compound_assign_types_target_by_declaration_not_by_name() {
    // `k` is the strongest possible name to test with: `expr_is_float_typed`
    // hard-codes it as Real (EMA's k factor). The declaration must still win.
    // (a) declared Integer -> usize target, i32 optIn RHS: `as usize`, never `as f64`.
    let mut ctx = backends::rust_lang::RustRenderCtx::empty();
    ctx.is_lookback = false;
    ctx.index_vars.insert("k".to_string());
    let stmt = ir::Statement::Assign {
        target: ir::Expr::Var("k".to_string()),
        value: ir::Expr::BinOp(
            Box::new(ir::Expr::Var("k".to_string())),
            ir::BinOp::Add,
            Box::new(ir::Expr::Var("optInTimePeriod".to_string())),
        ),
        compound: true,
    };
    let rendered = render_rust_stmt_with_ctx(&stmt, &ctx);
    assert!(
        rendered.contains("k += (optInTimePeriod) as usize") && !rendered.contains("as f64"),
        "declared-Integer target must take the usize cast, not f64: {rendered}"
    );

    // (b) signed local (i32) + i32 optIn RHS: no cast at all. This is the shape
    // issue #158 was filed on, but it was already correct at HEAD (b8619ed6b
    // excluded sentinels from the f64 arm); what this pins is the I32 arm's
    // bare-render path, which the three-arm rewrite could easily have lost.
    let mut sctx = backends::rust_lang::RustRenderCtx::empty();
    sctx.is_lookback = false;
    sctx.sentinel_vars.insert("k".to_string());
    let rendered = render_rust_stmt_with_ctx(&stmt, &sctx);
    assert!(
        rendered.contains("k += optInTimePeriod")
            && !rendered.contains("as f64")
            && !rendered.contains("as usize"),
        "signed local + i32 param must render uncast: {rendered}"
    );

    // (c) signed local (i32) + usize RHS: `as i32`. Without this branch the
    // bare `k += today` failed E0277 the other way round.
    let mut mctx = backends::rust_lang::RustRenderCtx::empty();
    mctx.is_lookback = false;
    mctx.sentinel_vars.insert("k".to_string());
    mctx.index_vars.insert("today".to_string());
    let mixed = ir::Statement::Assign {
        target: ir::Expr::Var("k".to_string()),
        value: ir::Expr::BinOp(
            Box::new(ir::Expr::Var("k".to_string())),
            ir::BinOp::Add,
            Box::new(ir::Expr::Var("today".to_string())),
        ),
        compound: true,
    };
    let rendered = render_rust_stmt_with_ctx(&mixed, &mctx);
    assert!(
        rendered.contains("k += (today) as i32"),
        "i32 target with a usize RHS must take the i32 cast: {rendered}"
    );

    // (d) a Real local still gets the f64 cast — positively, via real_vars.
    let mut rctx = backends::rust_lang::RustRenderCtx::empty();
    rctx.is_lookback = false;
    rctx.real_vars.insert("k".to_string());
    let rendered = render_rust_stmt_with_ctx(&stmt, &rctx);
    assert!(
        rendered.contains("k += ((optInTimePeriod) as f64)"),
        "Real target must still cast the i32 RHS to f64: {rendered}"
    );

    // (e) The cast has to follow what the RHS *renders* as, not its C type.
    // `today + optInTimePeriod` renders `today + (optInTimePeriod) as usize`,
    // i.e. usize, even though `expr_is_i32_typed` sees an i32 operand. Both an
    // i32 and an f64 target must cast it.
    let mixed_rhs = |target: &str| ir::Statement::Assign {
        target: ir::Expr::Var(target.to_string()),
        value: ir::Expr::BinOp(
            Box::new(ir::Expr::Var(target.to_string())),
            ir::BinOp::Add,
            Box::new(ir::Expr::BinOp(
                Box::new(ir::Expr::Var("today".to_string())),
                ir::BinOp::Add,
                Box::new(ir::Expr::Var("optInTimePeriod".to_string())),
            )),
        ),
        compound: true,
    };
    let mut xctx = backends::rust_lang::RustRenderCtx::empty();
    xctx.is_lookback = false;
    xctx.index_vars.insert("today".to_string());
    xctx.sentinel_vars.insert("k".to_string());
    xctx.real_vars.insert("total".to_string());
    let rendered = render_rust_stmt_with_ctx(&mixed_rhs("k"), &xctx);
    assert!(
        rendered.contains("as i32"),
        "i32 target with a usize-RENDERING mixed RHS must cast: {rendered}"
    );
    let rendered = render_rust_stmt_with_ctx(&mixed_rhs("total"), &xctx);
    assert!(
        rendered.contains("as f64"),
        "Real target with a usize-RENDERING mixed RHS must cast: {rendered}"
    );

    // (g) The bar range never narrows to i32, even into a signed target: bare,
    // so it fails to compile rather than truncating above 2^31.
    let mut ictx = backends::rust_lang::RustRenderCtx::empty();
    ictx.is_lookback = false;
    ictx.sentinel_vars.insert("k".to_string());
    ictx.index_vars.insert("startIdx".to_string());
    ictx.index_vars.insert("endIdx".to_string());
    let range_rhs = ir::Statement::Assign {
        target: ir::Expr::Var("k".to_string()),
        value: ir::Expr::BinOp(
            Box::new(ir::Expr::Var("k".to_string())),
            ir::BinOp::Add,
            Box::new(ir::Expr::BinOp(
                Box::new(ir::Expr::Var("endIdx".to_string())),
                ir::BinOp::Sub,
                Box::new(ir::Expr::Var("startIdx".to_string())),
            )),
        ),
        compound: true,
    };
    let rendered = render_rust_stmt_with_ctx(&range_rhs, &ictx);
    assert!(
        !rendered.contains("as i32"),
        "the caller's bar range must never be narrowed to i32: {rendered}"
    );

    // (h) An unlisted Real optional parameter is Real because the YAML says so.
    // `is_i32_opt_in_param` is a NEGATIVE allowlist, so consulting it first
    // would call any Real param it has not been told about an integer.
    let mut pctx = backends::rust_lang::RustRenderCtx::empty();
    pctx.is_lookback = false;
    let param_stmt = ir::Statement::Assign {
        target: ir::Expr::Var("optInThreshold".to_string()),
        value: ir::Expr::BinOp(
            Box::new(ir::Expr::Var("optInThreshold".to_string())),
            ir::BinOp::Add,
            Box::new(ir::Expr::Var("optInTimePeriod".to_string())),
        ),
        compound: true,
    };
    let rendered = backends::rust_lang::render_statement(
        &param_stmt,
        12,
        &pctx,
        &[],
        &std::collections::HashMap::new(),
        &[],
        &["optInThreshold".to_string()],
        &HashMap::new(),
        &make_registry(),
        &HelperRegistry::empty(),
        &std::cell::Cell::new(0),
    );
    assert!(
        rendered.contains("optInThreshold += ((optInTimePeriod) as f64)"),
        "a YAML-declared Real optIn param must be Real: {rendered}"
    );

    // (f) A signed local reaching a Real target is an integer too — the plain
    // `expr_is_i32_typed` does not know about sentinels, so this arrived uncast.
    let sentinel_rhs = ir::Statement::Assign {
        target: ir::Expr::Var("total".to_string()),
        value: ir::Expr::BinOp(
            Box::new(ir::Expr::Var("total".to_string())),
            ir::BinOp::Add,
            Box::new(ir::Expr::Var("k".to_string())),
        ),
        compound: true,
    };
    let rendered = render_rust_stmt_with_ctx(&sentinel_rhs, &xctx);
    assert!(
        rendered.contains("total += ((k) as f64)"),
        "Real target must cast a signed-local RHS to f64: {rendered}"
    );
}

/// An implicit `double` -> `int` conversion in the input C is refused at parse
/// time. C narrows silently, but Java, C# and Rust all reject the statement, so
/// it used to generate four files of which three did not compile — with no
/// diagnostic. The four languages also disagree on negative and out-of-range
/// values (issue #160), so the generator must not pick a meaning.
#[test]
#[should_panic(expected = "is an integer, and it is assigned a floating-point expression")]
fn parser_rejects_implicit_double_to_int_narrowing() {
    parser::c_source::parse_c_source_str(
        "TA_RetCode test( int startIdx, int endIdx, const double inReal[],
                          int *outBegIdx, int *outNBElement, double outReal[] )
         {
            int r;
            double q;
            q = inReal[startIdx];
            r = q;
            *outBegIdx = 0; *outNBElement = r;
            return TA_SUCCESS;
         }",
    );
}

/// The same body with the cast written out is accepted — the check must not
/// fire on the explicit form every shipped function uses.
#[test]
fn parser_accepts_explicit_double_to_int_cast() {
    let parsed = parser::c_source::parse_c_source_str(
        "TA_RetCode test( int startIdx, int endIdx, const double inReal[],
                          int *outBegIdx, int *outNBElement, double outReal[] )
         {
            int r;
            double q;
            q = inReal[startIdx];
            r = (int)q;
            *outBegIdx = 0; *outNBElement = r;
            return TA_SUCCESS;
         }",
    );
    assert_eq!(parsed.functions.len(), 1, "explicit cast must parse cleanly");
}

/// Issue #158: a helper-inlined temporary has no `VarDecl` in the body it is
/// inlined into — the inliner renames the helper's own local `range` to
/// `range_0` — so it must be typed from the HELPER's declaration, not from its
/// name. Before this was handled, `range_0` reached the classifier with nothing
/// to go on.
#[test]
fn rust_compound_assign_types_helper_inlined_temp_from_the_helper() {
    let helper = |name: &str, local: &str, ty: ir::VarType| ir::HelperDef {
        name: name.to_string(),
        return_type: ir::VarType::Real,
        params: vec![],
        body: vec![ir::Statement::VarDecl {
            var_type: ty,
            name: local.to_string(),
            init: None,
        }],
    };
    let helpers = HelperRegistry::from_defs(vec![
        helper("ta_true_range", "range", ir::VarType::Real),
        helper("ta_some_counter", "slot", ir::VarType::Integer),
    ]);
    let compound = |target: &str| ir::Statement::Assign {
        target: ir::Expr::Var(target.to_string()),
        value: ir::Expr::BinOp(
            Box::new(ir::Expr::Var(target.to_string())),
            ir::BinOp::Add,
            Box::new(ir::Expr::Var("optInTimePeriod".to_string())),
        ),
        compound: true,
    };
    let mut ctx = backends::rust_lang::RustRenderCtx::empty();
    ctx.is_lookback = false;

    // `range` is a double in the helper -> the i32 param must be cast to f64.
    let rendered = render_rust_stmt_with_helpers(&compound("range_0"), &ctx, &helpers);
    assert!(
        rendered.contains("range_0 += ((optInTimePeriod) as f64)"),
        "helper-declared Real temp must take the f64 cast: {rendered}"
    );

    // `slot` is an int in the helper -> usize, so the cast is `as usize`.
    // Its NAME is on no index list, which is the whole point.
    let rendered = render_rust_stmt_with_helpers(&compound("slot_2"), &ctx, &helpers);
    assert!(
        rendered.contains("slot_2 += (optInTimePeriod) as usize"),
        "helper-declared integer temp must take the usize cast: {rendered}"
    );

    // Two helpers declaring the SAME name with different types must not resolve
    // by whichever the registry yields first — it is a `HashMap`, so that would
    // make generation depend on hash order.
    let conflicting = HelperRegistry::from_defs(vec![
        helper("ta_one", "amount", ir::VarType::Real),
        helper("ta_two", "amount", ir::VarType::Integer),
    ]);
    let rendered = render_rust_stmt_with_helpers(&compound("amount_0"), &ctx, &conflicting);
    assert!(
        !rendered.contains("as usize"),
        "a name two helpers type differently must not resolve from helper decls: {rendered}"
    );
}

/// The lookback leg of the implicit-narrowing check: a `LookbackExpr::Code`
/// body is parsed separately from the function bodies and needs its own guard.
#[test]
#[should_panic(expected = "is an integer, and it is assigned a floating-point expression")]
fn parser_rejects_implicit_narrowing_in_a_lookback_body() {
    parser::c_source::parse_c_source_str(
        "int test_lookback( int optInTimePeriod )
         {
            int lb;
            double scale;
            scale = optInTimePeriod * 0.5;
            lb = scale;
            return lb;
         }",
    );
}

/// A lookback's own parameters are not all integers — 14 shipped lookbacks take
/// a `double` (`optInPenetration`, `optInNbDev`, ...). They have to be typed
/// from the signature, or assigning one to an int local slips through.
#[test]
#[should_panic(expected = "is an integer, and it is assigned a floating-point expression")]
fn parser_rejects_implicit_narrowing_of_a_real_lookback_param() {
    parser::c_source::parse_c_source_str(
        "int test_lookback( int optInTimePeriod, double optInPenetration )
         {
            int lb;
            lb = optInPenetration;
            return lb + optInTimePeriod;
         }",
    );
}

/// `input/helpers/*.c` parse through a different entry point. A narrowing there
/// is inlined into every call site, so it reaches all four backends multiplied
/// by however many sites the helper serves.
#[test]
#[should_panic(expected = "is an integer, and it is assigned a floating-point expression")]
fn parser_rejects_implicit_narrowing_inside_a_helper() {
    parser::c_source::parse_helper_file_str(
        "double ta_scaled_range(double th, double tl) {
            double range = th - tl;
            int whole;
            whole = range;
            return range + whole;
         }",
    );
}

/// Two disjoint blocks may reuse a name with different types. The backends
/// render those as separate scopes and compile, so the check must not flatten
/// a function into one namespace and reject the second declaration.
#[test]
fn parser_accepts_same_name_different_type_in_disjoint_scopes() {
    let parsed = parser::c_source::parse_c_source_str(
        "TA_RetCode test( int startIdx, int endIdx, const double inReal[],
                          int *outBegIdx, int *outNBElement, double outReal[] )
         {
            if( startIdx > 0 ) { int    tmpz; tmpz = startIdx; (void)tmpz; }
            if( startIdx > 1 ) { double tmpz; tmpz = 1.5;      (void)tmpz; }
            *outBegIdx = 0; *outNBElement = 0;
            return TA_SUCCESS;
         }",
    );
    assert_eq!(parsed.functions.len(), 1, "disjoint scopes must parse cleanly");
}

#[test]
fn rust_vardecl_with_init_expr() {
    let stmt = ir::Statement::VarDecl {
        var_type: ir::VarType::Real,
        name: "total".to_string(),
        init: Some(ir::Expr::Literal(2.71)),
    };
    let rendered = render_rust_stmt(&stmt);
    assert!(
        rendered.contains("2.71"),
        "VarDecl with init should render the init expression: {rendered}"
    );
    assert!(
        rendered.contains("let mut total: f64"),
        "VarDecl should declare with type: {rendered}"
    );
}

#[test]
fn rust_vardecl_sentinel_var_renders_i32() {
    let mut ctx = backends::rust_lang::RustRenderCtx::empty();
    ctx.sentinel_vars.insert("highestIdx".to_string());
    let stmt = ir::Statement::VarDecl {
        var_type: ir::VarType::Integer,
        name: "highestIdx".to_string(),
        init: None,
    };
    let rendered = render_rust_stmt_with_ctx(&stmt, &ctx);
    assert!(
        rendered.contains("i32"),
        "Sentinel var VarDecl should render as i32: {rendered}"
    );
}

// ---------------------------------------------------------------------------
// 2. Switch/case rendering
// ---------------------------------------------------------------------------

#[test]
fn rust_switch_renders_match_with_cases() {
    let stmt = ir::Statement::Switch {
        expr: ir::Expr::Var("optInMAType".to_string()),
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
    let rendered = render_rust_stmt(&stmt);
    assert!(
        rendered.contains("match optInMAType"),
        "Switch should render as match: {rendered}"
    );
    assert!(
        rendered.contains("0 =>"),
        "Switch case 0 should render: {rendered}"
    );
    assert!(
        rendered.contains("1 =>"),
        "Switch case 1 should render: {rendered}"
    );
    assert!(
        rendered.contains("_ =>"),
        "Switch default should render as _ =>: {rendered}"
    );
}

#[test]
fn rust_switch_without_default() {
    let stmt = ir::Statement::Switch {
        expr: ir::Expr::Var("mode".to_string()),
        cases: vec![(
            "42".to_string(),
            vec![ir::Statement::Break],
        )],
        default: vec![],
    };
    let rendered = render_rust_stmt(&stmt);
    assert!(
        rendered.contains("match mode"),
        "Switch should render as match: {rendered}"
    );
    assert!(
        rendered.contains("42 =>"),
        "Switch case should render: {rendered}"
    );
    assert!(
        !rendered.contains("_ =>"),
        "Switch without default should not have _ => arm: {rendered}"
    );
}

#[test]
fn rust_switch_with_enum_label_lookup() {
    // Test switch rendering with real MA indicator (exercises render_switch_label with enum lookup)
    let (func, enums) = load_indicator("ma");
    let registry = make_registry();
    let helpers = make_helpers();
    let rust_out = backends::rust_lang::generate(&func, &enums, &registry, &helpers);

    // MA's switch renders as a match whose arms name the enum members. This
    // pins the member spelling rather than "some integer": the subject is the
    // typed parameter, so a bare ordinal would not even compile.
    assert!(
        rust_out.contains("match "),
        "MA Rust should contain match statement: {rust_out}"
    );
    assert!(
        rust_out.contains("MAType::SMA =>") && rust_out.contains("MAType::EMA =>"),
        "MA Rust match should have qualified member case labels: {rust_out}"
    );
    assert!(
        !rust_out.contains("            0 => {"),
        "MA Rust match must not fall back to bare ordinals: {rust_out}"
    );
}

// ---------------------------------------------------------------------------
// 3. DoWhile rendering
// ---------------------------------------------------------------------------

#[test]
fn rust_dowhile_renders_loop_with_break() {
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
    let rendered = render_rust_stmt(&stmt);
    assert!(
        rendered.contains("loop {"),
        "DoWhile should render as loop: {rendered}"
    );
    assert!(
        rendered.contains("if !(") && rendered.contains("{ break; }"),
        "DoWhile should have conditional break at end: {rendered}"
    );
    // Body should come before the break condition
    let body_pos = rendered.find("x =").expect("Should have body assignment");
    let break_pos = rendered.find("break").expect("Should have break");
    assert!(
        body_pos < break_pos,
        "DoWhile body should execute before break check"
    );
}

// ---------------------------------------------------------------------------
// 4. ForC rendering: countdown loop and generic fallback
// ---------------------------------------------------------------------------

#[test]
fn rust_forc_countdown_renders_loop_break_pattern() {
    // for(i = 10; i >= 0; i--) → loop { body; if i == 0 { break; } i -= 1; }
    let stmt = ir::Statement::ForC {
        init: Box::new(ir::Statement::Assign {
            target: ir::Expr::Var("i".to_string()),
            value: ir::Expr::IntLiteral(10),
            compound: false,
        }),
        condition: ir::Expr::BinOp(
            Box::new(ir::Expr::Var("i".to_string())),
            ir::BinOp::GreaterEq,
            Box::new(ir::Expr::IntLiteral(0)),
        ),
        update: Box::new(ir::Statement::Assign {
            target: ir::Expr::Var("i".to_string()),
            value: ir::Expr::BinOp(
                Box::new(ir::Expr::Var("i".to_string())),
                ir::BinOp::Sub,
                Box::new(ir::Expr::IntLiteral(1)),
            ),
            compound: false,
        }),
        body: vec![ir::Statement::Assign {
            target: ir::Expr::Var("sum".to_string()),
            value: ir::Expr::Literal(1.0),
            compound: false,
        }],
    };
    let rendered = render_rust_stmt(&stmt);
    assert!(
        rendered.contains("loop {"),
        "Countdown ForC should render as loop: {rendered}"
    );
    assert!(
        rendered.contains("break"),
        "Countdown ForC should contain break: {rendered}"
    );
    assert!(
        rendered.contains("i -= 1"),
        "Countdown ForC should have decrement: {rendered}"
    );
}

#[test]
fn rust_forc_pre_decrement_countdown() {
    // for(i = 5; i >= 0; --i) using PreDecrement
    let stmt = ir::Statement::ForC {
        init: Box::new(ir::Statement::Assign {
            target: ir::Expr::Var("i".to_string()),
            value: ir::Expr::IntLiteral(5),
            compound: false,
        }),
        condition: ir::Expr::BinOp(
            Box::new(ir::Expr::Var("i".to_string())),
            ir::BinOp::GreaterEq,
            Box::new(ir::Expr::IntLiteral(0)),
        ),
        update: Box::new(ir::Statement::Assign {
            target: ir::Expr::Var("i".to_string()),
            value: ir::Expr::PreDecrement(Box::new(ir::Expr::Var("i".to_string()))),
            compound: false,
        }),
        body: vec![],
    };
    let rendered = render_rust_stmt(&stmt);
    assert!(
        rendered.contains("loop {"),
        "Pre-decrement countdown ForC should render as loop: {rendered}"
    );
    assert!(
        rendered.contains("break"),
        "Pre-decrement countdown ForC should contain break: {rendered}"
    );
}

#[test]
fn rust_forc_generic_fallback_uses_while() {
    // for(i = 0; i < n; i = i * 2) — not simple increment, not simple decrement
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
            value: ir::Expr::BinOp(
                Box::new(ir::Expr::Var("i".to_string())),
                ir::BinOp::Mul,
                Box::new(ir::Expr::IntLiteral(2)),
            ),
            compound: false,
        }),
        body: vec![ir::Statement::Assign {
            target: ir::Expr::Var("sum".to_string()),
            value: ir::Expr::Literal(1.0),
            compound: false,
        }],
    };
    let rendered = render_rust_stmt(&stmt);
    assert!(
        rendered.contains("while "),
        "Generic ForC should fall through to while: {rendered}"
    );
    assert!(
        rendered.contains("// for("),
        "Generic ForC should include comment with original C form: {rendered}"
    );
}

#[test]
fn rust_forc_range_iteration_post_loop_fixup() {
    // for(i = startIdx; i <= endIdx; i++) should emit range + post-loop fixup
    let stmt = ir::Statement::ForC {
        init: Box::new(ir::Statement::Assign {
            target: ir::Expr::Var("i".to_string()),
            value: ir::Expr::Var("startIdx".to_string()),
            compound: false,
        }),
        condition: ir::Expr::BinOp(
            Box::new(ir::Expr::Var("i".to_string())),
            ir::BinOp::LessEq,
            Box::new(ir::Expr::Var("endIdx".to_string())),
        ),
        update: Box::new(ir::Statement::Assign {
            target: ir::Expr::Var("i".to_string()),
            value: ir::Expr::BinOp(
                Box::new(ir::Expr::Var("i".to_string())),
                ir::BinOp::Add,
                Box::new(ir::Expr::IntLiteral(1)),
            ),
            compound: false,
        }),
        body: vec![],
    };
    let rendered = render_rust_stmt(&stmt);
    assert!(
        rendered.contains("..") && rendered.contains("+ 1"),
        "Range ForC should use exclusive range: {rendered}"
    );
    // Post-loop fixup: i = (endIdx as usize) + 1
    assert!(
        rendered.contains("+ 1"),
        "Range ForC should have post-loop fixup: {rendered}"
    );
}

// ---------------------------------------------------------------------------
// 5. Block rendering
// ---------------------------------------------------------------------------

#[test]
fn rust_block_renders_inner_statements() {
    let stmt = ir::Statement::Block {
        body: vec![
            ir::Statement::Assign {
                target: ir::Expr::Var("x".to_string()),
                value: ir::Expr::IntLiteral(1),
                compound: false,
            },
            ir::Statement::Assign {
                target: ir::Expr::Var("y".to_string()),
                value: ir::Expr::IntLiteral(2),
                compound: false,
            },
        ],
    };
    let rendered = render_rust_stmt(&stmt);
    assert!(
        rendered.contains("x = 1"),
        "Block should render first statement: {rendered}"
    );
    assert!(
        rendered.contains("y = 2"),
        "Block should render second statement: {rendered}"
    );
}

// ---------------------------------------------------------------------------
// 6. Cross-indicator argument rendering
// ---------------------------------------------------------------------------

#[test]
fn rust_cross_indicator_call_via_generate() {
    // MA calls sma, ema etc. — exercises the registry-based cross-indicator path
    let (func, enums) = load_indicator("ma");
    let registry = make_registry();
    let helpers = make_helpers();
    let rust_out = backends::rust_lang::generate(&func, &enums, &registry, &helpers);

    // Cross-indicator calls resolve to the callee's PUBLIC entry point (#267)
    assert!(
        rust_out.contains("match self.sma("),
        "MA Rust should call the public self.sma(): {rust_out}"
    );
    assert!(
        rust_out.contains("match self.ema("),
        "MA Rust should call the public self.ema(): {rust_out}"
    );
    assert!(
        !rust_out.contains("self.sma_impl(") && !rust_out.contains("self.ema_impl("),
        "MA Rust must not call a callee's numerics tier: {rust_out}"
    );
    // `self.` makes this a call, not a definition, so the negative is real.
    // step 1 still emits — so the negative is real, not vacuous.
    assert!(
        !rust_out.contains("self.sma_unguarded(") && !rust_out.contains("self.ema_unguarded("),
        "MA Rust must not call the unguarded variants: {rust_out}"
    );
}

#[test]
fn rust_cross_indicator_lookback_with_pascal_case() {
    // Two authored spellings name the same lookback — the prefix-free
    // `sma_lookback`, and the legacy `TA_SMA_Lookback` whose `TA_` the parser
    // strips. Both must render as the SAME method call on `self`.
    //
    // Rendered through the statement renderer, not through an indicator: every
    // shipped input uses the lower-case spelling, so generating a real function
    // exercises one arm and silently leaves the other unpinned. That is how the
    // legacy arm was once lost — a build of the crate is the only thing that
    // catches it, and only if some input happens to use the spelling.
    for fname in ["sma_lookback", "SMA_Lookback"] {
        let stmt = ir::Statement::Assign {
            target: ir::Expr::Var("lookbackTotal".to_string()),
            value: ir::Expr::FuncCall(
                (*fname).to_string(),
                vec![ir::Expr::Var("optInTimePeriod".to_string())],
            ),
            compound: false,
        };
        let rendered = render_rust_stmt(&stmt);
        assert!(
            rendered.contains("self.sma_lookback("),
            "`{fname}` must render as self.sma_lookback(), got: {rendered}"
        );
    }
}

#[test]
fn rust_private_cross_indicator_call() {
    // Two distinct call-resolution paths. The bare-name path is exercised by
    // MA's dispatch to ema(); the private-name path (`<name>_private()` →
    // `<Name>_Private`) by the SYNTH4 gate fixture's guarded body, which is the
    // only definition left declaring an explicit _private with extra params
    // (EMA's was folded away in #183).
    let registry = make_registry();
    let helpers = make_helpers();

    let (func, enums) = load_indicator("ma");
    let rust_out = backends::rust_lang::generate(&func, &enums, &registry, &helpers);
    assert!(
        rust_out.contains("match self.ema("),
        "MA Rust dispatch should call the public self.ema(): {rust_out}"
    );

    let synth_registry = make_synth_registry();
    let (func, enums) = load_synth("synth4");
    let rust_out = backends::rust_lang::generate(&func, &enums, &synth_registry, &helpers);
    assert!(
        rust_out.contains("self.synth4_private("),
        "SYNTH4 Rust guarded body should delegate to self.synth4_private(): {rust_out}"
    );
}

#[test]
fn rust_public_entry_documents_exactly_its_parameters() {
    // The `# Arguments` list and the signature are emitted by two different
    // functions (rust_doc::guarded_docs and rust_lang::gen_public_entry), so they
    // can disagree silently: rustdoc has no lint for documenting a parameter that
    // does not exist, and a doctest exercises the call, not its prose. That is
    // exactly how the out-param bullets survived the move to a Result-returning
    // wrapper. Sweep every indicator and require the two to agree, in order.
    let registry = make_registry();
    let helpers = make_helpers();
    let base = Path::new(env!("CARGO_MANIFEST_DIR")).join("../../ta_codegen/input");
    let mut checked = 0usize;
    for entry in std::fs::read_dir(&base).expect("input dir") {
        let entry = entry.expect("dir entry");
        if !entry.file_type().map(|t| t.is_dir()).unwrap_or(false) {
            continue;
        }
        let name = entry.file_name().to_string_lossy().to_string();
        let dir = entry.path();
        if !dir.join(format!("{name}.c")).is_file() || !dir.join(format!("{name}.yaml")).is_file() {
            continue;
        }
        let (func, enums) = load_indicator(&name);
        let out = backends::rust_lang::generate(&func, &enums, &registry, &helpers);

        // The public wrapper is the only fn returning Result<OutRange, RetCode>.
        let sig_open = format!("    pub fn {}(\n", backends::common::snake_words(&func.name));
        let at = out
            .find(&sig_open)
            .unwrap_or_else(|| panic!("{name}: no public entry `{sig_open}`"));
        let rest = &out[at + sig_open.len()..];
        let end = rest
            .find("    ) -> Result<OutRange, RetCode> {")
            .unwrap_or_else(|| panic!("{name}: public entry does not return Result<OutRange, RetCode>"));
        let params: Vec<String> = rest[..end]
            .lines()
            .filter_map(|l| l.trim().strip_suffix(','))
            .filter(|l| *l != "&self")
            .filter_map(|l| l.split_once(": ").map(|(n, _)| n.trim().to_string()))
            .collect();

        // The `# Arguments` block immediately above that signature (the lookback
        // has one of its own further up, hence rfind).
        let head = &out[..at];
        let a = head
            .rfind("/// # Arguments")
            .unwrap_or_else(|| panic!("{name}: public entry has no # Arguments"));
        let r = head[a..]
            .find("/// # Returns")
            .unwrap_or_else(|| panic!("{name}: public entry has no # Returns"));
        let documented: Vec<String> = head[a..a + r]
            .lines()
            .filter_map(|l| l.trim().strip_prefix("/// * `"))
            .filter_map(|l| l.split('`').next().map(str::to_string))
            .collect();

        assert_eq!(
            documented, params,
            "{name}: rustdoc `# Arguments` disagrees with the public signature"
        );
        assert!(!params.is_empty(), "{name}: parsed no parameters -- the test would be vacuous");
        checked += 1;
    }
    assert!(checked >= 200, "expected the whole corpus, checked only {checked}");
}

/// `word` appearing in `hay` with no identifier character on either side.
fn mentions_word(hay: &str, word: &str) -> bool {
    hay.match_indices(word).any(|(i, _)| {
        let before_ok = !hay[..i]
            .chars()
            .next_back()
            .is_some_and(|c| c.is_alphanumeric() || c == '_');
        let after_ok = !hay[i + word.len()..]
            .chars()
            .next()
            .is_some_and(|c| c.is_alphanumeric() || c == '_');
        before_ok && after_ok
    })
}

#[test]
fn every_integer_output_carries_an_example_claim() {
    // The generated example checks a real output for finiteness. An integer output
    // has nothing analogous, so 65 of them -- 61 candlestick patterns, HT_TRENDMODE
    // and the three index functions -- asserted nothing about their values at all
    // (#179 E8, deferred from #136). The domain is per-function data and lives in
    // `rust_doc::integer_domain_claim`; nothing in the metadata carries it, since
    // all 69 integer outputs declare the same `line` flag. This is the gate that a
    // function arriving with an integer output states its domain instead of
    // silently rejoining that set.
    let registry = make_registry();
    let helpers = make_helpers();
    let base = Path::new(env!("CARGO_MANIFEST_DIR")).join("../../ta_codegen/input");
    let mut checked = 0usize;
    for entry in std::fs::read_dir(&base).expect("input dir") {
        let entry = entry.expect("dir entry");
        if !entry.file_type().map(|t| t.is_dir()).unwrap_or(false) {
            continue;
        }
        let name = entry.file_name().to_string_lossy().to_string();
        let dir = entry.path();
        if !dir.join(format!("{name}.c")).is_file() || !dir.join(format!("{name}.yaml")).is_file() {
            continue;
        }
        // Shipped functions only. `scripts/synth_gate.py` copies its fixtures
        // into input/, and this claim is a curated per-function table feeding
        // the rustdoc EXAMPLE -- documentation for the published crate, which a
        // fixture never reaches. Requiring one would put a `("SYNTH3", _)` arm
        // in the shipped doc emitter forever, to document a function no reader
        // can call. The `checked` count below is a shipped-corpus count for the
        // same reason.
        if name.starts_with("synth") {
            continue;
        }
        let (func, enums) = load_indicator(&name);
        let ints = func
            .outputs
            .iter()
            .filter(|o| o.param_type == ir::ParamType::Integer)
            .count();
        if ints == 0 {
            continue;
        }
        let out = backends::rust_lang::generate(&func, &enums, &registry, &helpers);
        // One claim per integer output, counted BY THAT OUTPUT'S OWN example
        // variable. Counting bare `[..out_range.count]` instead let a mixed
        // real+integer function satisfy the floor with the real output's
        // finiteness assert alone -- SUPERTREND passed this gate with no integer
        // claim at all, which is the whole condition it exists to catch.
        let claims: usize = func
            .outputs
            .iter()
            .zip(backends::rust_doc::output_var_names(&func))
            .filter(|(o, _)| o.param_type == ir::ParamType::Integer)
            .map(|(_, var)| {
                // Word boundary, not `contains`: `trend` is a proper suffix of
                // `supertrend`, so the real output's finiteness assert would
                // answer the integer output's needle.
                usize::from(mentions_word(&out, &format!("{var}[..out_range.count]")))
            })
            .sum();
        assert!(
            claims >= ints,
            "{name}: {ints} integer output(s) but {claims} example claim(s) naming them -- add the \
             output's domain to rust_doc::integer_domain_claim"
        );
        checked += 1;
    }
    assert_eq!(
        checked, 67,
        "expected the 67 integer-output functions, swept {checked}"
    );
}

#[test]
fn rust_cross_indicator_vec_input_gets_ref() {
    // Indicators that allocate a local buffer (Vec) and pass it to a cross-indicator
    // call should render the Vec as `&name` in input position. (MACD was the original
    // vehicle, but its lockstep fusion removed the local buffers.) STOCH builds
    // tempBuffer and passes it into ma as an input.
    let (func, enums) = load_indicator("stoch");
    let registry = make_registry();
    let helpers = make_helpers();
    let rust_out = backends::rust_lang::generate(&func, &enums, &registry, &helpers);

    assert!(
        rust_out.contains("self.ma(") && rust_out.contains("&tempBuffer"),
        "STOCH Rust should pass &tempBuffer into self.ma(): {rust_out}"
    );
}

#[test]
fn rust_is_ta_function_renders_self_call() {
    // All-uppercase function names that aren't builtins are treated as cross-indicator calls
    // via is_ta_function, rendered as self.{lowercase}(args).
    // STOCHRSI calls STOCHF which should be rendered as self.stochf(...)
    let (func, enums) = load_indicator("stochrsi");
    let registry = make_registry();
    let helpers = make_helpers();
    let result = std::panic::catch_unwind(std::panic::AssertUnwindSafe(|| {
        backends::rust_lang::generate(&func, &enums, &registry, &helpers)
    }));
    if let Ok(rust_out) = result {
        // Should contain self.rsi or self.stochf calls
        let has_cross_call = rust_out.contains("self.rsi")
            || rust_out.contains("self.stochf")
            || rust_out.contains("self.sma");
        assert!(
            has_cross_call,
            "STOCHRSI Rust should contain cross-indicator self.xxx calls: {rust_out}"
        );
    }
    // If it panics, the indicator might not be parseable yet — skip silently
}

// ---------------------------------------------------------------------------
// 7. Lookback code rendering with candle settings
// ---------------------------------------------------------------------------

#[test]
fn rust_lookback_code_rendering_cdlkicking() {
    // CDL indicators have complex lookback bodies with candle settings
    let (func, enums) = load_indicator("cdlkicking");
    let registry = make_registry();
    let helpers = make_helpers();
    let rust_out = backends::rust_lang::generate(&func, &enums, &registry, &helpers);

    // Lookback function should exist
    assert!(
        rust_out.contains("_lookback("),
        "CDL indicator should have lookback function: {rust_out}"
    );
    // Candle settings should be unpacked
    assert!(
        rust_out.contains("candle_settings"),
        "CDL lookback should unpack candle_settings: {rust_out}"
    );
}

#[test]
fn rust_lookback_code_with_vars() {
    // Test that lookback code renders VarDecls with proper types
    let (func, enums) = load_indicator("cdlkicking");
    let registry = make_registry();
    let helpers = make_helpers();
    let rust_out = backends::rust_lang::generate(&func, &enums, &registry, &helpers);

    // CDL indicators have local vars in their lookback body (e.g., lookbackTotal)
    // They should be declared as `let mut` or `let`
    let lookback_section = extract_section(&rust_out, "_lookback(", "pub(crate) fn cdlkicking_impl(");
    assert!(
        lookback_section.contains("let ") || lookback_section.contains("let mut "),
        "Lookback code should declare local variables: {lookback_section}"
    );
}

#[test]
fn rust_lookback_literal_renders_return() {
    // SMA has LookbackExpr::ParamMinus or simple literal
    let (func, enums) = load_indicator("mult");
    let registry = make_registry();
    let helpers = make_helpers();
    let rust_out = backends::rust_lang::generate(&func, &enums, &registry, &helpers);

    let lookback_section = extract_section(&rust_out, "_lookback(", "pub(crate) fn mult_impl(");
    assert!(
        lookback_section.contains("return"),
        "Lookback should have return statement: {lookback_section}"
    );
}

// ---------------------------------------------------------------------------
// 8. Expression rendering edge cases
// ---------------------------------------------------------------------------

#[test]
fn rust_ternary_renders_if_else() {
    let stmt = ir::Statement::Assign {
        target: ir::Expr::Var("result".to_string()),
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
    let rendered = render_rust_stmt(&stmt);
    assert!(
        rendered.contains("if ") && rendered.contains("else"),
        "Ternary should render as if/else: {rendered}"
    );
}

#[test]
fn rust_post_increment_renders_block() {
    let stmt = ir::Statement::Assign {
        target: ir::Expr::Var("result".to_string()),
        value: ir::Expr::PostIncrement(Box::new(ir::Expr::Var("i".to_string()))),
        compound: false,
    };
    let rendered = render_rust_stmt(&stmt);
    assert!(
        rendered.contains("let _v =") && rendered.contains("+= 1"),
        "PostIncrement should render as block with temp: {rendered}"
    );
}

#[test]
fn rust_post_decrement_renders_block() {
    let stmt = ir::Statement::Assign {
        target: ir::Expr::Var("result".to_string()),
        value: ir::Expr::PostDecrement(Box::new(ir::Expr::Var("i".to_string()))),
        compound: false,
    };
    let rendered = render_rust_stmt(&stmt);
    assert!(
        rendered.contains("let _v =") && rendered.contains("i.wrapping_sub(1)"),
        "PostDecrement should render as block with temp and a debug-safe wrapping decrement: {rendered}"
    );
}

#[test]
fn rust_pre_increment_renders_block() {
    let stmt = ir::Statement::Assign {
        target: ir::Expr::Var("result".to_string()),
        value: ir::Expr::PreIncrement(Box::new(ir::Expr::Var("i".to_string()))),
        compound: false,
    };
    let rendered = render_rust_stmt(&stmt);
    assert!(
        rendered.contains("+= 1"),
        "PreIncrement should render with increment: {rendered}"
    );
}

#[test]
fn rust_pre_decrement_renders_block() {
    let stmt = ir::Statement::Assign {
        target: ir::Expr::Var("result".to_string()),
        value: ir::Expr::PreDecrement(Box::new(ir::Expr::Var("i".to_string()))),
        compound: false,
    };
    let rendered = render_rust_stmt(&stmt);
    assert!(
        rendered.contains("i.wrapping_sub(1)"),
        "PreDecrement should render with a debug-safe wrapping decrement: {rendered}"
    );
}

#[test]
fn rust_not_expr_renders_negation() {
    let stmt = ir::Statement::Assign {
        target: ir::Expr::Var("result".to_string()),
        value: ir::Expr::Not(Box::new(ir::Expr::Var("flag".to_string()))),
        compound: false,
    };
    let rendered = render_rust_stmt(&stmt);
    assert!(
        rendered.contains("!(flag)"),
        "Not should render as !(): {rendered}"
    );
}

#[test]
fn rust_cast_renders_as_type() {
    let stmt = ir::Statement::Assign {
        target: ir::Expr::Var("x".to_string()),
        value: ir::Expr::Cast(
            ir::VarType::Real,
            Box::new(ir::Expr::Var("n".to_string())),
        ),
        compound: false,
    };
    let rendered = render_rust_stmt(&stmt);
    assert!(
        rendered.contains("as f64"),
        "Cast to Real should render as 'as f64': {rendered}"
    );
}

#[test]
fn rust_cast_to_integer_renders_as_usize() {
    let stmt = ir::Statement::Assign {
        target: ir::Expr::Var("x".to_string()),
        value: ir::Expr::Cast(
            ir::VarType::Integer,
            Box::new(ir::Expr::Var("val".to_string())),
        ),
        compound: false,
    };
    let rendered = render_rust_stmt(&stmt);
    assert!(
        rendered.contains("as usize"),
        "Cast to Integer should render as 'as usize': {rendered}"
    );
}

#[test]
fn rust_pointer_deref_renders_star() {
    let stmt = ir::Statement::Assign {
        target: ir::Expr::Var("x".to_string()),
        value: ir::Expr::PointerDeref("outBegIdx".to_string()),
        compound: false,
    };
    let rendered = render_rust_stmt(&stmt);
    assert!(
        rendered.contains("(*outBegIdx)"),
        "PointerDeref should render as (*name): {rendered}"
    );
}

#[test]
fn rust_address_of_renders_inner() {
    let stmt = ir::Statement::Assign {
        target: ir::Expr::Var("x".to_string()),
        value: ir::Expr::AddressOf(Box::new(ir::Expr::Var("val".to_string()))),
        compound: false,
    };
    let rendered = render_rust_stmt(&stmt);
    // AddressOf renders inner expression directly in Rust (not idiomatic)
    assert!(
        rendered.contains("val"),
        "AddressOf should render inner expression: {rendered}"
    );
}

// ---------------------------------------------------------------------------
// 9. render_func_call branches
// ---------------------------------------------------------------------------

#[test]
fn rust_func_call_unstable_period() {
    let stmt = ir::Statement::Assign {
        target: ir::Expr::Var("x".to_string()),
        value: ir::Expr::FuncCall(
            "UNSTABLE_PERIOD".to_string(),
            vec![ir::Expr::Var("FUNC_UNST_RSI".to_string())],
        ),
        compound: false,
    };
    let rendered = render_rust_stmt(&stmt);
    assert!(
        rendered.contains("self.unstable_period[FuncUnstId::RSI as usize]"),
        "UNSTABLE_PERIOD should render with FuncUnstId: {rendered}"
    );
}

#[test]
fn rust_func_call_is_zero() {
    let stmt = ir::Statement::Assign {
        target: ir::Expr::Var("x".to_string()),
        value: ir::Expr::FuncCall(
            "IS_ZERO".to_string(),
            vec![ir::Expr::Var("val".to_string())],
        ),
        compound: false,
    };
    let rendered = render_rust_stmt(&stmt);
    assert!(
        rendered.contains(".abs() < 1e-14"),
        "IS_ZERO should render as abs() < 1e-14: {rendered}"
    );
}

#[test]
fn rust_func_call_is_zero_or_neg() {
    let stmt = ir::Statement::Assign {
        target: ir::Expr::Var("x".to_string()),
        value: ir::Expr::FuncCall(
            "IS_ZERO_OR_NEG".to_string(),
            vec![ir::Expr::Var("val".to_string())],
        ),
        compound: false,
    };
    let rendered = render_rust_stmt(&stmt);
    assert!(
        rendered.contains("< 1e-14"),
        "IS_ZERO_OR_NEG should render with 1e-14 epsilon: {rendered}"
    );
    assert!(
        !rendered.contains(".abs()"),
        "IS_ZERO_OR_NEG should not use .abs(): {rendered}"
    );
}

#[test]
fn rust_func_call_per_to_k() {
    let stmt = ir::Statement::Assign {
        target: ir::Expr::Var("k".to_string()),
        value: ir::Expr::FuncCall(
            "PER_TO_K".to_string(),
            vec![ir::Expr::Var("optInTimePeriod".to_string())],
        ),
        compound: false,
    };
    let rendered = render_rust_stmt(&stmt);
    assert!(
        rendered.contains("2.0_f64 / ("),
        "PER_TO_K should render as 2.0_f64 / (...): {rendered}"
    );
}

#[test]
fn rust_func_call_sizeof() {
    let stmt = ir::Statement::Assign {
        target: ir::Expr::Var("x".to_string()),
        value: ir::Expr::FuncCall(
            "sizeof".to_string(),
            vec![ir::Expr::Var("double".to_string())],
        ),
        compound: false,
    };
    let rendered = render_rust_stmt(&stmt);
    assert!(
        rendered.contains("1"),
        "sizeof should render as 1: {rendered}"
    );
}

#[test]
fn rust_func_call_malloc_renders_vec() {
    let stmt = ir::Statement::Assign {
        target: ir::Expr::Var("buf".to_string()),
        value: ir::Expr::FuncCall(
            "malloc".to_string(),
            vec![ir::Expr::BinOp(
                Box::new(ir::Expr::Var("n".to_string())),
                ir::BinOp::Mul,
                Box::new(ir::Expr::FuncCall(
                    "sizeof".to_string(),
                    vec![ir::Expr::Var("int".to_string())],
                )),
            )],
        ),
        compound: false,
    };
    let rendered = render_rust_stmt(&stmt);
    assert!(
        rendered.contains("vec![0_i32;"),
        "malloc with sizeof(int) should render as vec![0_i32; ...]: {rendered}"
    );
}

#[test]
fn rust_func_call_malloc_f64_default() {
    let stmt = ir::Statement::Assign {
        target: ir::Expr::Var("buf".to_string()),
        value: ir::Expr::FuncCall(
            "malloc".to_string(),
            vec![ir::Expr::BinOp(
                Box::new(ir::Expr::Var("n".to_string())),
                ir::BinOp::Mul,
                Box::new(ir::Expr::FuncCall(
                    "sizeof".to_string(),
                    vec![ir::Expr::Var("double".to_string())],
                )),
            )],
        ),
        compound: false,
    };
    let rendered = render_rust_stmt(&stmt);
    assert!(
        rendered.contains("vec![0.0_f64;"),
        "malloc with sizeof(double) should render as vec![0.0_f64; ...]: {rendered}"
    );
}

/// `free()` no longer reaches the Rust renderer at all: `drop_deallocation`
/// removes it from the IR, and the renderer's arm is now the assertion that it
/// did. The behaviour this used to check lives in `ir_cleanup`'s own tests; what
/// is worth pinning here is the corpus-level property, which
/// `deallocation_is_dropped_only_where_the_backend_has_none` carries.
#[test]
fn rust_free_never_reaches_the_renderer() {
    let (func, enums) = load_indicator("stoch");
    let out = generate_all(&func, &enums);
    assert!(!out.rust.contains("free("), "a free() survived into the Rust output");
    assert!(out.c.contains("free("), "C must keep its deallocation");
}

#[test]
fn rust_func_call_memcpy_renders_copy_from_slice() {
    let stmt = ir::Statement::Expr(ir::Expr::FuncCall(
        "memcpy".to_string(),
        vec![
            ir::Expr::Var("dst".to_string()),
            ir::Expr::Var("src".to_string()),
            ir::Expr::Var("count".to_string()),
        ],
    ));
    let rendered = render_rust_stmt(&stmt);
    assert!(
        rendered.contains("copy_from_slice"),
        "memcpy should render as copy_from_slice: {rendered}"
    );
}

#[test]
fn rust_func_call_array_copy_renders_copy_from_slice() {
    let stmt = ir::Statement::Expr(ir::Expr::FuncCall(
        "ARRAY_COPY".to_string(),
        vec![
            ir::Expr::Var("dst".to_string()),
            ir::Expr::IntLiteral(0),
            ir::Expr::Var("src".to_string()),
            ir::Expr::IntLiteral(0),
            ir::Expr::Var("n".to_string()),
        ],
    ));
    let rendered = render_rust_stmt(&stmt);
    assert!(
        rendered.contains("copy_from_slice"),
        "ARRAY_COPY should render as copy_from_slice: {rendered}"
    );
}

#[test]
fn rust_func_call_ta_candlerange_renders_match() {
    let stmt = ir::Statement::Assign {
        target: ir::Expr::Var("cr".to_string()),
        value: ir::Expr::FuncCall(
            "ta_candlerange".to_string(),
            vec![
                ir::Expr::Var("rt".to_string()),
                ir::Expr::Var("open".to_string()),
                ir::Expr::Var("high".to_string()),
                ir::Expr::Var("low".to_string()),
                ir::Expr::Var("close".to_string()),
            ],
        ),
        compound: false,
    };
    let rendered = render_rust_stmt(&stmt);
    assert!(
        rendered.contains("match rt"),
        "ta_candlerange should render with match: {rendered}"
    );
}

#[test]
fn rust_func_call_ta_candleaverage_renders_inline() {
    let stmt = ir::Statement::Assign {
        target: ir::Expr::Var("avg".to_string()),
        value: ir::Expr::FuncCall(
            "ta_candleaverage".to_string(),
            vec![
                ir::Expr::Var("rt".to_string()),
                ir::Expr::Var("ap".to_string()),
                ir::Expr::Var("factor".to_string()),
                ir::Expr::Var("sum".to_string()),
                ir::Expr::Var("open".to_string()),
                ir::Expr::Var("high".to_string()),
                ir::Expr::Var("low".to_string()),
                ir::Expr::Var("close".to_string()),
            ],
        ),
        compound: false,
    };
    let rendered = render_rust_stmt(&stmt);
    assert!(
        rendered.contains("match") && !rendered.contains("let _cr"),
        "ta_candleaverage should render as single nested expression (no let bindings): {rendered}"
    );
}

// ---------------------------------------------------------------------------
// 10. Return statement rendering
// ---------------------------------------------------------------------------

#[test]
fn rust_return_success_renders_retcode() {
    let stmt = ir::Statement::Return {
        value: Some(ir::Expr::Var("SUCCESS".to_string())),
    };
    let rendered = render_rust_stmt(&stmt);
    assert!(
        rendered.contains("RetCode::Success"),
        "Return SUCCESS should render as RetCode::Success: {rendered}"
    );
}

#[test]
fn rust_return_bad_param_renders_retcode() {
    let stmt = ir::Statement::Return {
        value: Some(ir::Expr::Var("BadParam".to_string())),
    };
    let rendered = render_rust_stmt(&stmt);
    assert!(
        rendered.contains("RetCode::BadParam"),
        "Return BadParam should render as RetCode::BadParam: {rendered}"
    );
}

#[test]
fn rust_return_alloc_err_renders_retcode() {
    let stmt = ir::Statement::Return {
        value: Some(ir::Expr::Var("ALLOC_ERR".to_string())),
    };
    let rendered = render_rust_stmt(&stmt);
    assert!(
        rendered.contains("RetCode::AllocErr"),
        "Return ALLOC_ERR should render as RetCode::AllocErr: {rendered}"
    );
}

#[test]
fn rust_return_none_renders_bare_return() {
    let stmt = ir::Statement::Return { value: None };
    let rendered = render_rust_stmt(&stmt);
    assert!(
        rendered.contains("return;"),
        "Return without value should render as 'return;': {rendered}"
    );
}

#[test]
fn rust_break_renders() {
    let stmt = ir::Statement::Break;
    let rendered = render_rust_stmt(&stmt);
    assert!(
        rendered.contains("break;"),
        "Break should render as 'break;': {rendered}"
    );
}

#[test]
fn rust_continue_renders() {
    let stmt = ir::Statement::Continue;
    let rendered = render_rust_stmt(&stmt);
    assert!(
        rendered.contains("continue;"),
        "Continue should render as 'continue;': {rendered}"
    );
}

// ---------------------------------------------------------------------------
// 11. Compound assignment rendering
// ---------------------------------------------------------------------------

#[test]
fn rust_compound_add_assignment() {
    let mut ctx = backends::rust_lang::RustRenderCtx::empty();
    ctx.real_vars.insert("total".to_string());
    let stmt = ir::Statement::Assign {
        target: ir::Expr::Var("total".to_string()),
        value: ir::Expr::BinOp(
            Box::new(ir::Expr::Var("total".to_string())),
            ir::BinOp::Add,
            Box::new(ir::Expr::Literal(1.0)),
        ),
        compound: true,
    };
    let rendered = render_rust_stmt_with_ctx(&stmt, &ctx);
    assert!(
        rendered.contains("+="),
        "Compound add should render as +=: {rendered}"
    );
}

#[test]
fn rust_compound_sub_assignment() {
    let mut ctx = backends::rust_lang::RustRenderCtx::empty();
    ctx.real_vars.insert("total".to_string());
    let stmt = ir::Statement::Assign {
        target: ir::Expr::Var("total".to_string()),
        value: ir::Expr::BinOp(
            Box::new(ir::Expr::Var("total".to_string())),
            ir::BinOp::Sub,
            Box::new(ir::Expr::Literal(1.0)),
        ),
        compound: true,
    };
    let rendered = render_rust_stmt_with_ctx(&stmt, &ctx);
    assert!(
        rendered.contains("-="),
        "Compound sub should render as -=: {rendered}"
    );
}

#[test]
fn rust_compound_mul_assignment() {
    let mut ctx = backends::rust_lang::RustRenderCtx::empty();
    ctx.real_vars.insert("total".to_string());
    let stmt = ir::Statement::Assign {
        target: ir::Expr::Var("total".to_string()),
        value: ir::Expr::BinOp(
            Box::new(ir::Expr::Var("total".to_string())),
            ir::BinOp::Mul,
            Box::new(ir::Expr::Literal(2.0)),
        ),
        compound: true,
    };
    let rendered = render_rust_stmt_with_ctx(&stmt, &ctx);
    assert!(
        rendered.contains("*="),
        "Compound mul should render as *=: {rendered}"
    );
}

#[test]
fn rust_compound_div_assignment() {
    let mut ctx = backends::rust_lang::RustRenderCtx::empty();
    ctx.real_vars.insert("total".to_string());
    let stmt = ir::Statement::Assign {
        target: ir::Expr::Var("total".to_string()),
        value: ir::Expr::BinOp(
            Box::new(ir::Expr::Var("total".to_string())),
            ir::BinOp::Div,
            Box::new(ir::Expr::Literal(2.0)),
        ),
        compound: true,
    };
    let rendered = render_rust_stmt_with_ctx(&stmt, &ctx);
    assert!(
        rendered.contains("/="),
        "Compound div should render as /=: {rendered}"
    );
}

// ---------------------------------------------------------------------------
// 12. For (countdown) rendering
// ---------------------------------------------------------------------------

#[test]
fn rust_for_countdown_renders_rev() {
    let stmt = ir::Statement::For {
        var: "i".to_string(),
        count: ir::Expr::Var("n".to_string()),
        body: vec![ir::Statement::Assign {
            target: ir::Expr::Var("sum".to_string()),
            value: ir::Expr::Literal(1.0),
            compound: false,
        }],
    };
    let rendered = render_rust_stmt(&stmt);
    assert!(
        rendered.contains(".rev()"),
        "For countdown should use .rev(): {rendered}"
    );
    assert!(
        rendered.contains("1..="),
        "For countdown should use 1..=count: {rendered}"
    );
}

// ---------------------------------------------------------------------------
// 13. If/else rendering with alloc_err suppression
// ---------------------------------------------------------------------------

#[test]
fn rust_if_with_alloc_err_return_is_suppressed() {
    let stmt = ir::Statement::If {
        condition: ir::Expr::BinOp(
            Box::new(ir::Expr::Var("ptr".to_string())),
            ir::BinOp::Eq,
            Box::new(ir::Expr::IntLiteral(0)),
        ),
        then_body: vec![ir::Statement::Return {
            value: Some(ir::Expr::Var("ALLOC_ERR".to_string())),
        }],
        else_body: vec![],
        cond_comments: vec![],
    };
    let rendered = render_rust_stmt(&stmt);
    assert!(
        rendered.is_empty(),
        "If with ALLOC_ERR return should be suppressed (dead code in Rust): got '{rendered}'"
    );
}

#[test]
fn rust_if_else_chain_renders() {
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
    let rendered = render_rust_stmt(&stmt);
    assert!(
        rendered.contains("} else if"),
        "If/else if chain should render with 'else if': {rendered}"
    );
}

// ---------------------------------------------------------------------------
// 14. Lookback rendering with different LookbackExpr variants
// ---------------------------------------------------------------------------

#[test]
fn rust_lookback_param_minus() {
    // Test ParamMinus lookback variant
    let body = vec![ir::Statement::Return {
        value: Some(ir::Expr::Var("SUCCESS".to_string())),
    }];
    let func = ir::FuncDef {
        name: "TEST".to_string(),
        group: "Test".to_string(),
        description: None,
        hint: None,
        flags: vec![],
        inputs: vec![ir::Input::new("inReal", ir::ParamType::Real)],
        optional_inputs: vec![ir::OptInput {
            name: "optInTimePeriod".to_string(),
            param_type: ir::ParamType::Integer,
            range: Some((2.0, 100000.0)),
            default: Some(30.0),
            display_name: None,
            hint: None,
            flags: vec![],
            suggested: None,
            precision: None,
        }],
        outputs: vec![ir::Output {
            name: "outReal".to_string(),
            param_type: ir::ParamType::Real,
            flags: vec![],
        }],
        lookback: Some(ir::LookbackExpr::ParamMinus("optInTimePeriod".to_string(), 1)),
        body: body.clone(),
        private_body: body,
        private_extra_params: vec![],
        private_param_init: vec![],
        has_explicit_private: false,
        header_comments: vec![],
        doc: None,
        streaming: false,
        alternates: vec![],
        resolved_stream_body: None,
    };
    let enums = HashMap::new();
    let registry = make_registry();
    let helpers = HelperRegistry::empty();
    let rust_out = backends::rust_lang::generate(&func, &enums, &registry, &helpers);

    assert!(
        rust_out.contains("optInTimePeriod - 1"),
        "ParamMinus lookback should render as param - offset: {rust_out}"
    );
    assert!(
        rust_out.contains("as usize"),
        "ParamMinus lookback should cast to usize: {rust_out}"
    );
}

#[test]
fn rust_lookback_none() {
    // Test None lookback variant (returns 0)
    let body = vec![ir::Statement::Return {
        value: Some(ir::Expr::Var("SUCCESS".to_string())),
    }];
    let func = ir::FuncDef {
        name: "TEST".to_string(),
        group: "Test".to_string(),
        description: None,
        hint: None,
        flags: vec![],
        inputs: vec![ir::Input::new("inReal", ir::ParamType::Real)],
        optional_inputs: vec![],
        outputs: vec![ir::Output {
            name: "outReal".to_string(),
            param_type: ir::ParamType::Real,
            flags: vec![],
        }],
        lookback: None,
        body: body.clone(),
        private_body: body,
        private_extra_params: vec![],
        private_param_init: vec![],
        has_explicit_private: false,
        header_comments: vec![],
        doc: None,
        streaming: false,
        alternates: vec![],
        resolved_stream_body: None,
    };
    let enums = HashMap::new();
    let registry = make_registry();
    let helpers = HelperRegistry::empty();
    let rust_out = backends::rust_lang::generate(&func, &enums, &registry, &helpers);

    let lookback_section = extract_section(&rust_out, "_lookback(", "pub(crate) fn test_impl(");
    assert!(
        lookback_section.contains("return Ok(0)"),
        "None lookback should return Ok(0): {lookback_section}"
    );
}

// ---------------------------------------------------------------------------
// 15. Lookback return value casting in lookback context
// ---------------------------------------------------------------------------

#[test]
fn rust_lookback_return_casts_to_usize() {
    // In lookback context, return values that are i32-typed should be cast to usize
    let mut ctx = backends::rust_lang::RustRenderCtx::empty();
    ctx.is_lookback = true;

    let stmt = ir::Statement::Return {
        value: Some(ir::Expr::Var("optInTimePeriod".to_string())),
    };
    let rendered = render_rust_stmt_with_ctx(&stmt, &ctx);
    assert!(
        rendered.contains("as usize"),
        "Lookback return of i32 param should cast to usize: {rendered}"
    );
}

// ---------------------------------------------------------------------------
// 16. While loop with for-loop-var pattern
// ---------------------------------------------------------------------------

#[test]
fn rust_while_with_for_loop_var_renders_for_in() {
    use backends::rust_lang::RustRenderCtx;

    let ctx = RustRenderCtx::empty();
    let for_loop_vars: Vec<String> = vec!["i".to_string()];
    let init_expr = ir::Expr::Var("startIdx".to_string());
    let mut var_inits: std::collections::HashMap<String, &ir::Expr> =
        std::collections::HashMap::new();
    var_inits.insert("i".to_string(), &init_expr);
    let output_names: Vec<String> = vec![];
    let opt_real_params: Vec<String> = vec![];
    let enums = HashMap::new();
    let registry = make_registry();
    let helpers = HelperRegistry::empty();
    let inline_counter = std::cell::Cell::new(0);

    // while (i <= endIdx) { body; i = i + 1; }
    // The last statement is the increment — it gets stripped when rendering as for-in
    let stmt = ir::Statement::While {
        condition: ir::Expr::BinOp(
            Box::new(ir::Expr::Var("i".to_string())),
            ir::BinOp::LessEq,
            Box::new(ir::Expr::Var("endIdx".to_string())),
        ),
        body: vec![
            ir::Statement::Assign {
                target: ir::Expr::Var("sum".to_string()),
                value: ir::Expr::Literal(1.0),
                compound: false,
            },
            ir::Statement::Assign {
                target: ir::Expr::Var("i".to_string()),
                value: ir::Expr::BinOp(
                    Box::new(ir::Expr::Var("i".to_string())),
                    ir::BinOp::Add,
                    Box::new(ir::Expr::IntLiteral(1)),
                ),
                compound: false,
            },
        ],
    };

    let rendered = backends::rust_lang::render_statement(
        &stmt,
        12,
        &ctx,
        &for_loop_vars,
        &var_inits,
        &output_names,
        &opt_real_params,
        &enums,
        &registry,
        &helpers,
        &inline_counter,
    );
    assert!(
        rendered.contains("for i in"),
        "While with for-loop-var pattern should render as for-in: {rendered}"
    );
    assert!(
        rendered.contains("..") && rendered.contains("+ 1"),
        "While-to-for should use exclusive range syntax: {rendered}"
    );
}

// ---------------------------------------------------------------------------
// 17. memset renders as fill
// ---------------------------------------------------------------------------

#[test]
fn rust_func_call_memset_renders_fill() {
    let stmt = ir::Statement::Expr(ir::Expr::FuncCall(
        "memset".to_string(),
        vec![
            ir::Expr::Var("buf".to_string()),
            ir::Expr::IntLiteral(0),
            ir::Expr::Var("count".to_string()),
        ],
    ));
    let rendered = render_rust_stmt(&stmt);
    assert!(
        rendered.contains(".fill("),
        "memset should render as .fill(): {rendered}"
    );
}

// ---------------------------------------------------------------------------
// 18. Lookback code rendering with VarDecl types in lookback body
// ---------------------------------------------------------------------------

#[test]
fn rust_lookback_code_renders_var_types_correctly() {
    // Build a synthetic lookback code body with multiple VarDecl types
    let lookback_stmts = vec![
        ir::Statement::VarDecl {
            var_type: ir::VarType::Real,
            name: "sum".to_string(),
            init: None,
        },
        ir::Statement::VarDecl {
            var_type: ir::VarType::Integer,
            name: "count".to_string(),
            init: None,
        },
        ir::Statement::VarDecl {
            var_type: ir::VarType::RetCodeType,
            name: "retCode".to_string(),
            init: None,
        },
        ir::Statement::VarDecl {
            var_type: ir::VarType::RealPointer,
            name: "buf".to_string(),
            init: None,
        },
        ir::Statement::VarDecl {
            var_type: ir::VarType::IntPointer,
            name: "ibuf".to_string(),
            init: None,
        },
        ir::Statement::VarDecl {
            var_type: ir::VarType::RealArray("10".to_string()),
            name: "rarr".to_string(),
            init: None,
        },
        ir::Statement::VarDecl {
            var_type: ir::VarType::IntArray("5".to_string()),
            name: "iarr".to_string(),
            init: None,
        },
        ir::Statement::Assign {
            target: ir::Expr::Var("count".to_string()),
            value: ir::Expr::IntLiteral(42),
            compound: false,
        },
        ir::Statement::Return {
            value: Some(ir::Expr::Var("count".to_string())),
        },
    ];

    let body = vec![ir::Statement::Return {
        value: Some(ir::Expr::Var("SUCCESS".to_string())),
    }];
    let func = ir::FuncDef {
        name: "TEST".to_string(),
        group: "Test".to_string(),
        description: None,
        hint: None,
        flags: vec![],
        inputs: vec![ir::Input::new("inReal", ir::ParamType::Real)],
        optional_inputs: vec![],
        outputs: vec![ir::Output {
            name: "outReal".to_string(),
            param_type: ir::ParamType::Real,
            flags: vec![],
        }],
        lookback: Some(ir::LookbackExpr::Code(lookback_stmts)),
        body: body.clone(),
        private_body: body,
        private_extra_params: vec![],
        private_param_init: vec![],
        has_explicit_private: false,
        header_comments: vec![],
        doc: None,
        streaming: false,
        alternates: vec![],
        resolved_stream_body: None,
    };
    let enums = HashMap::new();
    let registry = make_registry();
    let helpers = HelperRegistry::empty();
    let rust_out = backends::rust_lang::generate(&func, &enums, &registry, &helpers);

    let lookback_section = extract_section(&rust_out, "_lookback(", "pub(crate) fn test_impl(");
    // sum has no assignments in the body, so count_assignments returns 0 => `let` not `let mut`
    assert!(
        lookback_section.contains("let sum: f64 = 0.0_f64"),
        "Lookback should declare f64 var: {lookback_section}"
    );
    assert!(
        lookback_section.contains("let mut count: usize = 0_usize"),
        "Lookback should declare usize var: {lookback_section}"
    );
    assert!(
        lookback_section.contains("RetCode"),
        "Lookback should declare RetCode var: {lookback_section}"
    );
    assert!(
        lookback_section.contains("Vec<f64>"),
        "Lookback should declare Vec<f64> var: {lookback_section}"
    );
    assert!(
        lookback_section.contains("Vec<i32>"),
        "Lookback should declare Vec<i32> var: {lookback_section}"
    );
    assert!(
        lookback_section.contains("[f64; 10 as usize]"),
        "Lookback should declare RealArray: {lookback_section}"
    );
    assert!(
        lookback_section.contains("[i32; 5 as usize]"),
        "Lookback should declare IntArray: {lookback_section}"
    );
}

/// A lookback body must never fuse a multiply-add, in any backend.
///
/// C, Java and C# all pass `fma: None` when rendering a lookback ("pure integer
/// index arithmetic"). Rust reaches fusion through `real_vars`, which was empty
/// in the lookback context until issue #158 populated it — so fusing would have
/// silently become Rust-only, and a lookback drives `outBegIdx` and the output
/// length, making that a shape divergence rather than a tolerance one.
#[test]
fn rust_lookback_body_never_fuses_multiply_add() {
    let decl = |name: &str| ir::Statement::VarDecl {
        var_type: ir::VarType::Real,
        name: name.to_string(),
        init: None,
    };
    let lookback_stmts = vec![
        decl("acc"),
        decl("scale"),
        decl("bias"),
        // The canonical fusable shape: acc = acc + scale * bias.
        ir::Statement::Assign {
            target: ir::Expr::Var("acc".to_string()),
            value: ir::Expr::BinOp(
                Box::new(ir::Expr::Var("acc".to_string())),
                ir::BinOp::Add,
                Box::new(ir::Expr::BinOp(
                    Box::new(ir::Expr::Var("scale".to_string())),
                    ir::BinOp::Mul,
                    Box::new(ir::Expr::Var("bias".to_string())),
                )),
            ),
            compound: false,
        },
        ir::Statement::Return { value: Some(ir::Expr::IntLiteral(0)) },
    ];
    let body = vec![ir::Statement::Return {
        value: Some(ir::Expr::Var("SUCCESS".to_string())),
    }];
    let func = ir::FuncDef {
        name: "TEST".to_string(),
        group: "Test".to_string(),
        description: None,
        hint: None,
        flags: vec![],
        inputs: vec![ir::Input::new("inReal", ir::ParamType::Real)],
        optional_inputs: vec![],
        outputs: vec![ir::Output {
            name: "outReal".to_string(),
            param_type: ir::ParamType::Real,
            flags: vec![],
        }],
        lookback: Some(ir::LookbackExpr::Code(lookback_stmts)),
        body: body.clone(),
        private_body: body,
        private_extra_params: vec![],
        private_param_init: vec![],
        has_explicit_private: false,
        header_comments: vec![],
        doc: None,
        streaming: false,
        alternates: vec![],
        resolved_stream_body: None,
    };
    let enums = HashMap::new();
    let out = backends::rust_lang::generate(&func, &enums, &make_registry(), &HelperRegistry::empty());
    let section = extract_section(&out, "_lookback(", "pub fn test(");
    let section = &section[..section.find("\n    }").expect("lookback body must close")];
    assert!(
        section.contains("acc = acc + scale * bias"),
        "the fusable shape must actually reach the lookback renderer: {section}"
    );
    assert!(
        !section.contains(".mul_add("),
        "a lookback body must not fuse — C/Java/C# do not: {section}"
    );
}

/// Issue #158: a lookback body's locals are typed by their declarations, so the
/// variable's *name* cannot change the generated code.
///
/// The lookback renderer used to build an empty `RustRenderCtx`, which left
/// every local to the naming heuristics. `expr_is_float_typed` hard-codes `k`
/// as Real (EMA's k factor), so `int k; k += optInTimePeriod;` was declared
/// `usize` and assigned `((optInTimePeriod) as f64)` — E0277 — while the same
/// body written with `j` compiled. Both must now render identically.
#[test]
fn rust_lookback_body_types_locals_by_declaration_not_name() {
    fn lookback_section_for(var: &str) -> String {
        let lookback_stmts = vec![
            ir::Statement::VarDecl {
                var_type: ir::VarType::Integer,
                name: var.to_string(),
                init: None,
            },
            ir::Statement::Assign {
                target: ir::Expr::Var(var.to_string()),
                value: ir::Expr::Var("optInTimePeriod".to_string()),
                compound: false,
            },
            ir::Statement::Assign {
                target: ir::Expr::Var(var.to_string()),
                value: ir::Expr::BinOp(
                    Box::new(ir::Expr::Var(var.to_string())),
                    ir::BinOp::Add,
                    Box::new(ir::Expr::Var("optInTimePeriod".to_string())),
                ),
                compound: true,
            },
            ir::Statement::Return {
                value: Some(ir::Expr::Var(var.to_string())),
            },
        ];
        let body = vec![ir::Statement::Return {
            value: Some(ir::Expr::Var("SUCCESS".to_string())),
        }];
        let func = ir::FuncDef {
            name: "TEST".to_string(),
            group: "Test".to_string(),
            description: None,
            hint: None,
            flags: vec![],
            inputs: vec![ir::Input::new("inReal", ir::ParamType::Real)],
            optional_inputs: vec![ir::OptInput {
                name: "optInTimePeriod".to_string(),
                param_type: ir::ParamType::Integer,
                range: Some((2.0, 100_000.0)),
                default: Some(30.0),
                display_name: None,
                hint: None,
                flags: vec![],
                suggested: None,
                precision: None,
            }],
            outputs: vec![ir::Output {
                name: "outReal".to_string(),
                param_type: ir::ParamType::Real,
                flags: vec![],
            }],
            lookback: Some(ir::LookbackExpr::Code(lookback_stmts)),
            body: body.clone(),
            private_body: body,
            private_extra_params: vec![],
            private_param_init: vec![],
            has_explicit_private: false,
            header_comments: vec![],
            doc: None,
            streaming: false,
            alternates: vec![],
            resolved_stream_body: None,
        };
        let enums = HashMap::new();
        let registry = make_registry();
        let helpers = HelperRegistry::empty();
        let out = backends::rust_lang::generate(&func, &enums, &registry, &helpers);
        let section = extract_section(&out, "_lookback(", "pub fn test(");
        // Stop at the lookback's own closing brace — the tail of that slice is
        // the guarded function's rustdoc, whose doctest mentions `as f64`.
        let end = section.find("\n    }").expect("lookback body must close");
        section[..end].to_string()
    }

    let with_k = lookback_section_for("k");
    assert!(
        with_k.contains("let mut k: usize = 0_usize"),
        "lookback int local must declare usize: {with_k}"
    );
    assert!(
        !with_k.contains("as f64"),
        "an integer lookback local must never take an f64 RHS cast: {with_k}"
    );
    assert!(
        with_k.contains("k += (optInTimePeriod) as usize"),
        "usize lookback local must cast the i32 param RHS to usize: {with_k}"
    );

    // The name is not allowed to matter — this is the whole point of the issue.
    // Substitute the identifier only where it stands alone (`lookback` contains
    // a k) and keep every other byte, punctuation included: splitting on
    // non-alphanumerics and re-joining would erase the operators, making
    // `k -= x;` and `j += x;` compare equal.
    fn blank_ident(src: &str, ident: &str) -> String {
        let mut out = String::with_capacity(src.len());
        let mut rest = src;
        while let Some(pos) = rest.find(ident) {
            let (before, at) = rest.split_at(pos);
            let tail = &at[ident.len()..];
            let boundary = |c: char| !c.is_alphanumeric() && c != '_';
            let standalone = before.chars().next_back().is_none_or(boundary)
                && tail.chars().next().is_none_or(boundary);
            out.push_str(before);
            out.push_str(if standalone { "@" } else { ident });
            rest = tail;
        }
        out.push_str(rest);
        out
    }
    let with_j = lookback_section_for("j");
    assert_eq!(
        blank_ident(&with_k, "k"),
        blank_ident(&with_j, "j"),
        "renaming a lookback local must not change the generated code"
    );
}

// ===========================================================================

// ---------------------------------------------------------------------------
// Slice lengths LLVM can bound an index by (#438). Nothing reads them but the
// optimizer, so losing one changes no value, only the checks per bar.
// ---------------------------------------------------------------------------

#[test]
fn rust_batch_body_reslices_its_inputs_past_the_empty_range_exit() {
    let (func, enums) = load_indicator("sma");
    let rust_out = backends::rust_lang::generate(&func, &enums, &make_registry(), &make_helpers());
    let body = rust_out.split("fn sma_impl(").nth(1).expect("sma_impl");
    let exit = body.find("if startIdx > endIdx {").expect("the empty-range exit");
    let reslice = body.find("let inReal = &inReal[..=endIdx];").expect("the input reslice");
    // Before the exit a sub-lookback call may hold a shorter input, and the
    // reslice would panic where the call must succeed.
    assert!(reslice > exit, "the reslice must follow the exit: {body}");
}

/// SMA's declaration rendered over `body`, streaming off; the batch body.
fn sma_impl_over(body: Vec<ir::Statement>) -> String {
    let (mut func, enums) = load_indicator("sma");
    func.body = body.clone();
    func.private_body = body;
    func.streaming = false;
    let rust_out = backends::rust_lang::generate(&func, &enums, &make_registry(), &make_helpers());
    rust_out.split("fn sma_impl(").nth(1).expect("sma_impl").to_string()
}

#[test]
fn rust_keeps_an_initializer_read_before_a_plain_store() {
    use ir::{Expr, Statement, VarType};
    let var = |n: &str| Expr::Var(n.to_string());
    let store = |t: Expr, v: Expr| Statement::Assign { target: t, value: v, compound: false };
    let decl = |n: &str, init: Option<Expr>| Statement::VarDecl { var_type: VarType::Real, name: n.into(), init };
    let out0 = || Expr::ArrayAccess("outReal".into(), Box::new(Expr::IntLiteral(0)));
    // Read by an earlier statement, then overwritten.
    let a = sma_impl_over(vec![
        decl("lim", Some(Expr::Literal(1000.0))),
        store(out0(), var("lim")),
        store(var("lim"), Expr::Literal(2.0)),
    ]);
    assert!(a.contains("lim = 1000"), "read first, so the initializer stays: {a}");
    // Read by another local's initializer, which runs ahead of the body.
    let b = sma_impl_over(vec![
        decl("lim", Some(Expr::Literal(1000.0))),
        decl("cap", Some(var("lim"))),
        store(var("lim"), Expr::Literal(2.0)),
        store(out0(), var("cap")),
    ]);
    assert!(b.contains("lim = 1000"), "read by cap's initializer: {b}");
    // Control: a plain store before any read still drops the dead initializer.
    let c = sma_impl_over(vec![
        decl("lim", Some(Expr::Literal(1000.0))),
        store(var("lim"), Expr::Literal(2.0)),
        store(out0(), var("lim")),
    ]);
    assert!(!c.contains("lim = 1000"), "overwritten before any read: {c}");
}

#[test]
fn rust_keeps_the_initializer_of_a_local_read_before_it_is_overwritten() {
    use ir::{BinOp, Expr, Statement, VarType};
    let (mut func, enums) = load_indicator("sma");
    let var = |n: &str| Expr::Var(n.to_string());
    // `double lim = 1000.0; v = inReal[startIdx]; if( v < lim ) lim = v;`: the
    // strict extreme becomes a top-level `lim = min(v, lim)`, which reads lim.
    let body = vec![
        Statement::VarDecl { var_type: VarType::Real, name: "lim".into(), init: Some(Expr::Literal(1000.0)) },
        Statement::VarDecl { var_type: VarType::Real, name: "v".into(), init: None },
        Statement::Assign {
            target: var("v"),
            value: Expr::ArrayAccess("inReal".into(), Box::new(var("startIdx"))),
            compound: false,
        },
        Statement::If {
            condition: Expr::BinOp(Box::new(var("v")), BinOp::Less, Box::new(var("lim"))),
            then_body: vec![Statement::Assign { target: var("lim"), value: var("v"), compound: false }],
            else_body: vec![],
            cond_comments: vec![],
        },
        Statement::Assign {
            target: Expr::ArrayAccess("outReal".into(), Box::new(Expr::IntLiteral(0))),
            value: var("lim"),
            compound: false,
        },
    ];
    func.body = body.clone();
    func.private_body = body;
    func.streaming = false;
    let rust_out = backends::rust_lang::generate(&func, &enums, &make_registry(), &make_helpers());
    let body = rust_out.split("fn sma_impl(").nth(1).expect("sma_impl");
    let init = body.find("lim = 1000").expect("lim's initializer was dropped");
    let min = body.find("lim = c_min(v, lim);").expect("the extreme");
    assert!(init < min, "the initializer must come first: {body}");
}

// ---------------------------------------------------------------------------
// Pure shift loops lower to `copy_within` (issue #437)
// ---------------------------------------------------------------------------

fn sv(n: &str) -> ir::Expr {
    ir::Expr::Var(n.to_string())
}

fn sb(l: ir::Expr, op: ir::BinOp, r: ir::Expr) -> ir::Expr {
    ir::Expr::BinOp(Box::new(l), op, Box::new(r))
}

fn sa(a: &str, i: ir::Expr) -> ir::Expr {
    ir::Expr::ArrayAccess(a.to_string(), Box::new(i))
}

/// `dst[v] = src[v <read> 1]; v <step>= 1;` as the parser desugars it.
fn shift_body(dst: &str, src: &str, v: &str, read: ir::BinOp, step: ir::BinOp) -> Vec<ir::Statement> {
    vec![
        ir::Statement::Assign {
            target: sa(dst, sv(v)),
            value: sa(src, sb(sv(v), read, ir::Expr::IntLiteral(1))),
            compound: false,
        },
        ir::Statement::Assign {
            target: sv(v),
            value: sb(sv(v), step, ir::Expr::IntLiteral(1)),
            compound: true,
        },
    ]
}

fn shift_while(cond: ir::Expr, body: Vec<ir::Statement>) -> String {
    render_rust_stmt(&ir::Statement::While { condition: cond, body })
}

#[test]
fn rust_forward_shift_while_is_one_copy_within() {
    let out = shift_while(
        sb(sv("j"), ir::BinOp::Less, sb(sv("pos"), ir::BinOp::Sub, ir::Expr::IntLiteral(1))),
        shift_body("sorted", "sorted", "j", ir::BinOp::Add, ir::BinOp::Add),
    );
    assert!(!out.contains("while"), "{out}");
    assert!(out.contains("if j < pos - 1 {"), "{out}");
    assert!(out.contains("sorted.copy_within(j + 1..=pos - 1, j);"), "{out}");
    assert!(out.contains("j = pos - 1;"), "the loop leaves j at its bound: {out}");
}

#[test]
fn rust_backward_shift_while_is_one_copy_within() {
    let out = shift_while(
        sb(sv("j"), ir::BinOp::Greater, sv("pos")),
        shift_body("sorted", "sorted", "j", ir::BinOp::Sub, ir::BinOp::Sub),
    );
    assert!(!out.contains("while"), "{out}");
    assert!(out.contains("sorted.copy_within(pos..j, pos + 1);"), "{out}");
    assert!(out.contains("j = pos;"), "{out}");
    let lit = shift_while(
        sb(sv("j"), ir::BinOp::Greater, ir::Expr::IntLiteral(0)),
        shift_body("sorted", "sorted", "j", ir::BinOp::Sub, ir::BinOp::Sub),
    );
    assert!(lit.contains("sorted.copy_within(0..j, 1);"), "{lit}");
    let compound = shift_while(
        sb(sv("j"), ir::BinOp::Greater, sb(sv("a"), ir::BinOp::Sub, sv("b"))),
        shift_body("sorted", "sorted", "j", ir::BinOp::Sub, ir::BinOp::Sub),
    );
    assert!(compound.contains("sorted.copy_within(a - b..j, (a - b) + 1);"), "{compound}");
}

#[test]
fn rust_shift_for_loop_keeps_its_init_then_copies() {
    let mut body = shift_body("sorted", "sorted", "j", ir::BinOp::Add, ir::BinOp::Add);
    let update = body.pop().unwrap();
    let out = render_rust_stmt(&ir::Statement::ForC {
        init: Box::new(ir::Statement::Assign { target: sv("j"), value: sv("slot"), compound: false }),
        condition: sb(sv("j"), ir::BinOp::Less, sv("e")),
        update: Box::new(update),
        body,
    });
    assert!(!out.contains("while"), "{out}");
    let init = out.find("j = slot;").expect(&out);
    let copy = out.find("sorted.copy_within(j + 1..=e, j);").expect(&out);
    assert!(init < copy, "{out}");
    assert!(out.contains("j = e;"), "{out}");
}

#[test]
fn rust_loops_that_are_not_pure_shifts_stay_loops() {
    let bwd = || shift_body("sorted", "sorted", "j", ir::BinOp::Sub, ir::BinOp::Sub);
    let cases = [
        // An insertion step exits on the data, not on a bound.
        (
            sb(
                sb(sv("j"), ir::BinOp::Greater, ir::Expr::IntLiteral(0)),
                ir::BinOp::And,
                sb(sa("sorted", sb(sv("j"), ir::BinOp::Sub, ir::Expr::IntLiteral(1))), ir::BinOp::Greater, sv("x")),
            ),
            bwd(),
        ),
        // A bound read from the buffer being moved.
        (sb(sv("j"), ir::BinOp::Greater, sa("sorted", ir::Expr::IntLiteral(0))), bwd()),
        // A bound that names the index.
        (sb(sv("j"), ir::BinOp::Greater, sb(sv("j"), ir::BinOp::Sub, sv("n"))), bwd()),
        // Two arrays: a copy, not a shift within one.
        (
            sb(sv("j"), ir::BinOp::Greater, sv("pos")),
            shift_body("sorted", "ring", "j", ir::BinOp::Sub, ir::BinOp::Sub),
        ),
        // Reads ahead while walking back.
        (
            sb(sv("j"), ir::BinOp::Greater, sv("pos")),
            shift_body("sorted", "sorted", "j", ir::BinOp::Add, ir::BinOp::Sub),
        ),
        // The store lands on a fixed slot, not on the index.
        (
            sb(sv("j"), ir::BinOp::Less, sv("e")),
            vec![
                ir::Statement::Assign {
                    target: sa("sorted", sv("e")),
                    value: sa("sorted", sb(sv("j"), ir::BinOp::Add, ir::Expr::IntLiteral(1))),
                    compound: false,
                },
                shift_body("sorted", "sorted", "j", ir::BinOp::Add, ir::BinOp::Add).remove(1),
            ],
        ),
        // A stride of two moves every other slot.
        (
            sb(sv("j"), ir::BinOp::Greater, sv("pos")),
            vec![
                shift_body("sorted", "sorted", "j", ir::BinOp::Sub, ir::BinOp::Sub).remove(0),
                ir::Statement::Assign {
                    target: sv("j"),
                    value: sb(sv("j"), ir::BinOp::Sub, ir::Expr::IntLiteral(2)),
                    compound: true,
                },
            ],
        ),
        // The step writes another variable, so `j` never reaches the bound.
        (
            sb(sv("j"), ir::BinOp::Greater, sv("pos")),
            vec![
                shift_body("sorted", "sorted", "j", ir::BinOp::Sub, ir::BinOp::Sub).remove(0),
                ir::Statement::Assign {
                    target: sv("i"),
                    value: sb(sv("j"), ir::BinOp::Sub, ir::Expr::IntLiteral(1)),
                    compound: false,
                },
            ],
        ),
        // An index the renderer casts (`k` is not a known index name) is not a usize range.
        (
            sb(sv("k"), ir::BinOp::Greater, sv("pos")),
            shift_body("sorted", "sorted", "k", ir::BinOp::Sub, ir::BinOp::Sub),
        ),
        // `<=` runs one slot further than `e`.
        (
            sb(sv("j"), ir::BinOp::LessEq, sv("pos")),
            shift_body("sorted", "sorted", "j", ir::BinOp::Add, ir::BinOp::Add),
        ),
    ];
    for (cond, body) in cases {
        let out = shift_while(cond, body);
        assert!(!out.contains("copy_within"), "{out}");
        assert!(out.contains("while"), "{out}");
    }
}

/// The block scans of the rolling-extremum family run check-free only as
/// counted loops over windows (#442). Every access inside such a loop must go
/// through a window, or the check is back; only an array stored in a branch
/// keeps its check.
#[test]
fn rust_block_scans_run_as_counted_windows() {
    let registry = make_registry();
    let helpers = make_helpers();
    for (name, scans) in [("min", 3), ("max", 3), ("minmax", 3), ("midpoint", 3), ("midprice", 3), ("willr", 2)] {
        let (func, enums) = load_indicator(name);
        let rust = backends::rust_lang::generate(&func, &enums, &registry, &helpers);
        let loops = windowed_loop_bodies(&rust);
        assert!(loops.len() >= scans, "{name}: {} counted window loop(s), want at least {scans}:\n{rust}", loops.len());
        for body in &loops {
            let branches = body.contains("} else {");
            for line in body.lines().map(str::trim).filter(|l| !l.starts_with("//")) {
                for (at, _) in line.match_indices('[') {
                    let array = line[..at].rsplit(|c: char| !(c.is_alphanumeric() || c == '_')).next().unwrap_or("");
                    let arm_store = branches && line.starts_with(&format!("{array}[")) && line.contains("] = ");
                    assert!(
                        arm_store || array.strip_prefix("_w").is_some_and(|j| !j.is_empty() && j.bytes().all(|b| b.is_ascii_digit())),
                        "{name}: `{array}[` is indexed directly inside a counted window loop: `{line}`"
                    );
                }
            }
        }
    }
}

/// A candlestick's main loop that reads through a trailing index runs over
/// windows (#442): the reads of its pattern test sit in `&&` chains and helper
/// arms, so the windows are cut with `get`, while what the pattern's arms read
/// and store keeps its check. One that reads only at or behind the counter
/// stays as written.
#[test]
fn rust_candle_loops_read_their_inputs_through_windows() {
    let registry = make_registry();
    let helpers = make_helpers();
    let base = Path::new(env!("CARGO_MANIFEST_DIR")).join("../../ta_codegen/input");
    let mut seen = 0usize;
    for entry in std::fs::read_dir(&base).expect("input dir") {
        let name = entry.expect("dir entry").file_name().to_string_lossy().to_string();
        if !name.starts_with("cdl") {
            continue;
        }
        let (func, enums) = load_indicator(&name);
        let rust = backends::rust_lang::generate(&func, &enums, &registry, &helpers);
        let batch = &rust[..rust.find("_step_impl(").unwrap_or(rust.len())];
        if !batch.contains("TrailingIdx") {
            continue;
        }
        let loops = windowed_loop_bodies(batch);
        assert!(
            loops.iter().any(|body| body.contains("outIdx += 1;")),
            "{name}: its main loop is not a counted window loop:\n{batch}"
        );
        // A data-dependent index (`in[c ? i : i - 1]`) is no window's to take.
        let linear = |index: &str| index.split([' ', '+', '-']).filter(|t| !t.is_empty()).all(|t| t.bytes().all(|b| b.is_ascii_alphanumeric() || b == b'_'));
        for body in &loops {
            // The pattern test: the body up to the brace that opens its first arm.
            let test = &body[..body.find("{\n").map_or(body.len(), |at| at + 1)];
            for input in ["inOpen[", "inHigh[", "inLow[", "inClose["] {
                for (at, _) in test.match_indices(input) {
                    let rest = &test[at + input.len()..];
                    let index = &rest[..rest.find(']').unwrap_or(rest.len())];
                    assert!(!linear(index), "{name}: `{input}{index}]` indexed directly inside a counted window loop:\n{body}");
                }
            }
        }
        seen += 1;
    }
    assert!(seen >= 57, "saw {seen} candlestick functions reading a trailing index");
}

/// The bodies of the `for _wk in ...` loops in `rust`.
fn windowed_loop_bodies(rust: &str) -> Vec<String> {
    let mut out = Vec::new();
    let mut lines = rust.lines();
    while let Some(line) = lines.next() {
        if !line.trim_start().starts_with("for _wk in ") {
            continue;
        }
        let mut depth = 1i32;
        let mut body = String::new();
        for inner in lines.by_ref() {
            depth += inner.matches('{').count() as i32 - inner.matches('}').count() as i32;
            if depth <= 0 {
                break;
            }
            body.push_str(inner);
            body.push('\n');
        }
        out.push(body);
    }
    out
}

/// A loop whose condition decrements its counter: each pass decrements first,
/// the failing test decrements once more on both paths, and `--c != 0`, which
/// passes on a wrapped `c`, keeps the loop as written for a failed guard.
#[test]
fn rust_windowed_countdown_keeps_every_decrement_of_its_condition() {
    use ir::{BinOp, Expr, Statement};
    let var = |n: &str| Expr::Var(n.into());
    let bin = |l: Expr, op: BinOp, r: Expr| Expr::BinOp(Box::new(l), op, Box::new(r));
    let mut ctx = backends::rust_lang::RustRenderCtx::empty();
    ctx.is_lookback = false;
    ctx.window_loops = true;
    for n in ["c", "k"] {
        ctx.index_vars.insert(n.into());
    }
    let body = vec![
        Statement::Assign {
            target: Expr::ArrayAccess("outReal".into(), Box::new(var("k"))),
            value: Expr::Literal(0.0),
            compound: false,
        },
        Statement::Assign { target: var("k"), value: bin(var("k"), BinOp::Add, Expr::IntLiteral(1)), compound: true },
    ];
    let post = Expr::PostDecrement(Box::new(var("c")));
    let pre = Expr::PreDecrement(Box::new(var("c")));
    for (cond, guard, trip, as_written) in [
        (bin(post, BinOp::Greater, Expr::IntLiteral(0)), "if c > 0 {", "let _wn: usize = c;", false),
        (bin(pre, BinOp::NotEq, Expr::IntLiteral(0)), "if c > 1 {", "let _wn: usize = c - 1;", true),
    ] {
        let out = render_rust_stmt_with_ctx(&Statement::While { condition: cond, body: body.clone() }, &ctx);
        let lines: Vec<&str> = out.lines().map(str::trim).collect();
        let at = |l: &str| lines.iter().position(|x| *x == l).unwrap_or_else(|| panic!("no `{l}` in:\n{out}"));
        let (g, t, pass, dec, last, other) =
            (at(guard), at(trip), at("for _wk in 0.._wn {"), at("c -= 1;"), at("c = c.wrapping_sub(1);"), at("} else {"));
        assert!(g < t && t < pass && pass < dec && dec < last && last < other, "{out}");
        let unguarded = lines[other + 1];
        if as_written {
            assert!(unguarded.starts_with("while "), "{out}");
        } else {
            assert_eq!(unguarded, "c = c.wrapping_sub(1);", "{out}");
        }
    }
}
