//! Renderer feature tests: memmove lowering, TA_SUCCESS/RetCode presence,
//! ForC rendering, math-function idioms, expression/block inlining, and
//! candle-settings unpacking. Split out of the former `backend_suite.rs`.

#[path = "common/mod.rs"]
mod common;

use common::{
    discover_indicators, generate_all, load_indicator, load_indicator_with_source, make_helpers,
    make_registry, try_generate_all, try_load_indicator,
};
use std::collections::HashMap;
use std::path::Path;
use ta_codegen_lib::backends;
use ta_codegen_lib::helper_registry::HelperRegistry;
use ta_codegen_lib::ir;
use ta_codegen_lib::parser;

// 11b. Rust memmove lowering: in-place (same-buffer) move must be overlap-safe
// ---------------------------------------------------------------------------

/// Red/green guard for the Rust `memmove` lowering (issue #99 follow-up).
///
/// A `memmove` into the *same* backing array (an in-place, possibly overlapping
/// move) must lower to `slice::copy_within`, not `copy_from_slice`: the latter
/// needs a simultaneous `&mut` and `&` borrow of one slice — which does not
/// compile — and is UB on overlap regardless. A move between *distinct* buffers
/// stays `copy_from_slice`.
///
/// No shipped indicator carries a same-buffer memmove any more (BBANDS was
/// restructured for streaming: its #99 realign now copies `tempBuffer1` into the
/// *distinct* middle-band output), so a synthetic fixture pins the lowering. It
/// carries both a same-buffer move (`tempBuffer` <- `&tempBuffer[shiftIdx]`) and
/// a distinct-buffer move (`outReal` <- `&inReal[startIdx]`). WMA additionally
/// covers a real distinct-buffer move.
#[test]
fn test_rust_memmove_same_buffer_uses_copy_within() {
    let src = r#"
int sma_lookback( int optInTimePeriod )
{
   return optInTimePeriod - 1;
}

TA_RetCode sma( int startIdx, int endIdx,
   const double inReal[],
   int optInTimePeriod,
   int *outBegIdx, int *outNBElement,
   double outReal[] )
{
   double *tempBuffer;
   int shiftIdx;
   tempBuffer = malloc((endIdx-startIdx+1) * sizeof(double));
   shiftIdx = optInTimePeriod;
   memmove( tempBuffer, &tempBuffer[shiftIdx], (endIdx-startIdx+1) * sizeof(double) );
   memmove( outReal, &inReal[startIdx], (endIdx-startIdx+1) * sizeof(double) );
   *outBegIdx = startIdx;
   *outNBElement = endIdx - startIdx + 1;
   free( tempBuffer );
   return TA_SUCCESS;
}
"#;
    let (func, enums) = load_indicator_with_source("sma", src);
    let rust = generate_all(&func, &enums).rust;

    assert!(
        rust.contains("tempBuffer.copy_within("),
        "in-place (same-buffer) memmove must lower to copy_within (overlap-safe)"
    );
    assert!(
        rust.contains("copy_from_slice("),
        "distinct-buffer memmove must stay copy_from_slice"
    );

    // The fix must stay surgical: a move between distinct buffers is still a
    // plain copy_from_slice, never copy_within.
    let (wma, wenums) = load_indicator("wma");
    let wrust = generate_all(&wma, &wenums).rust;
    assert!(
        wrust.contains("copy_from_slice("),
        "WMA: memmove between distinct buffers should stay copy_from_slice"
    );
    assert!(
        !wrust.contains(".copy_within("),
        "WMA: distinct-buffer memmove must not use copy_within"
    );
}

// ---------------------------------------------------------------------------
// 12. MA has 2 optional inputs (timePeriod + MAType enum)
// ---------------------------------------------------------------------------

#[test]
fn test_ma_has_two_optional_inputs() {
    let (func, _enums) = load_indicator("ma");
    assert_eq!(
        func.optional_inputs.len(),
        2,
        "MA should have 2 optional inputs"
    );

    // One should be an enum type
    let has_enum = func
        .optional_inputs
        .iter()
        .any(|opt| matches!(opt.param_type, ir::ParamType::Enum(_)));
    assert!(has_enum, "MA should have an enum optional input (MAType)");
}

// ---------------------------------------------------------------------------
// 13. Validate TA_SUCCESS / RetCode::Success presence in function bodies
// ---------------------------------------------------------------------------

#[test]
fn test_all_indicators_contain_success_returns() {
    let indicators = discover_indicators();
    let mut failures = Vec::new();

    for name in &indicators {
        let (func, enums) = match try_load_indicator(name) {
            Some(v) => v,
            None => continue,
        };
        let out = match try_generate_all(&func, &enums) {
            Some(v) => v,
            None => continue,
        };

        let result = std::panic::catch_unwind(std::panic::AssertUnwindSafe(|| {
            // Delegation functions (e.g. MACDFIX -> TA_MACD) return a RetCode
            // from a callee without ever mentioning TA_SUCCESS literally.
            // Accept: literal TA_SUCCESS OR a `return TA_<func>( ... )` delegation.
            let c_has_success = out.c.contains("TA_SUCCESS")
                || out.c.lines().any(|l| {
                    let t = l.trim_start();
                    t.starts_with("return TA_") && t.contains('(')
                });
            assert!(c_has_success, "C {}: missing TA_SUCCESS return", name);
            // Delegation functions (e.g. MACDFIX) return a RetCode from a
            // callee without ever mentioning RetCode.Success literally.
            // Accept: literal RetCode::Success OR a return of a RetCode from a cross-indicator call.
            let rust_has_success = out.rust.contains("RetCode::Success")
                || out.rust.contains("return self.");
            assert!(
                rust_has_success,
                "Rust {}: missing RetCode::Success return",
                name
            );
            // Accept: literal RetCode.Success OR a return of a RetCode variable/call.
            let java_has_success = out.java.contains("RetCode.Success")
                || out.java.contains("return retCode ;")
                || (out.java.contains("return ") && out.java.contains("Internal("));
            assert!(
                java_has_success,
                "Java {}: missing RetCode.Success return",
                name
            );
        }));
        if let Err(e) = result {
            let msg = if let Some(s) = e.downcast_ref::<String>() {
                s.clone()
            } else if let Some(s) = e.downcast_ref::<&str>() {
                s.to_string()
            } else {
                format!("Unknown panic for indicator {}", name)
            };
            failures.push(msg);
        }
    }

    if !failures.is_empty() {
        panic!(
            "{} indicator(s) failed success-return checks:\n{}",
            failures.len(),
            failures.join("\n")
        );
    }
}

// ---------------------------------------------------------------------------
// Rust generic output smoke test
// ---------------------------------------------------------------------------

#[test]
fn test_rust_generic_output_smoke() {
    let (func, enums) = load_indicator("sma");
    let out = generate_all(&func, &enums);
    let r = &out.rust;

    // After the 2-variant refactor, Rust uses concrete f64 types, not generics.

    // 1. Concrete f64 signatures present (no generics)
    assert!(
        r.contains("pub fn SMA("),
        "Rust SMA should have pub fn SMA("
    );
    assert!(
        !r.contains("_unguarded"),
        "Rust SMA must not emit an unguarded variant"
    );

    // 2. No _s suffix methods
    assert!(
        !r.contains("fn sma_s(") && !r.contains("fn sma_s<"),
        "Rust SMA should NOT contain _s suffixed methods"
    );

    // 3. Output params use concrete f64
    assert!(
        r.contains("&mut [f64]"),
        "Rust SMA output params should use concrete type &mut [f64]"
    );

    // 4. Input params use concrete f64
    assert!(
        r.contains("&[f64]"),
        "Rust SMA input params should use concrete type &[f64]"
    );

    // 5. No _unchecked variants
    assert!(
        !r.contains("fn sma_unchecked(") && !r.contains("fn sma_unchecked<"),
        "Rust SMA should NOT contain _unchecked variants"
    );
    assert!(
        !r.contains("fn sma_unguarded_unchecked(") && !r.contains("fn sma_unguarded_unchecked<"),
        "Rust SMA should NOT contain _unguarded_unchecked variants"
    );

    // 6. Exactly 4 pub fn: guarded + lookback + the stream tier's open +
    // open_and_fill (open_internal is pub(crate), update/peek live on the handle
    // type).
    let batch_pub_fn_count = r.matches("pub fn SMA").count();
    assert_eq!(
        batch_pub_fn_count, 2,
        "Rust SMA batch tier should have exactly 2 pub fn (sma, SMA_Lookback), got {}",
        batch_pub_fn_count
    );
    let stream_pub_fn_count = r.matches("pub fn sma_open").count();
    assert_eq!(
        stream_pub_fn_count, 2,
        "Rust SMA stream tier should have exactly 2 pub fn (sma_open, sma_open_and_fill), got {}",
        stream_pub_fn_count
    );
}

// ---------------------------------------------------------------------------
// ForC init/update Block rendering: comma-separated, not semicolons
// ---------------------------------------------------------------------------

#[test]
fn c_for_loop_multi_init_comma_separated() {
    use ta_codegen_lib::ir::*;

    // Build synthetic ForC: for(j=0, i=startIdx; i<=endIdx; i=i+1, j=j+1)
    let init = Box::new(Statement::Block {
        body: vec![
            Statement::Assign {
                target: Expr::Var("j".into()),
                value: Expr::Literal(0.0),
                compound: false,
            },
            Statement::Assign {
                target: Expr::Var("i".into()),
                value: Expr::Var("startIdx".into()),
                compound: false,
            },
        ],
    });
    let condition = Expr::BinOp(
        Box::new(Expr::Var("i".into())),
        BinOp::LessEq,
        Box::new(Expr::Var("endIdx".into())),
    );
    let update = Box::new(Statement::Block {
        body: vec![
            Statement::Assign {
                target: Expr::Var("i".into()),
                value: Expr::BinOp(
                    Box::new(Expr::Var("i".into())),
                    BinOp::Add,
                    Box::new(Expr::Literal(1.0)),
                ),
                compound: false,
            },
            Statement::Assign {
                target: Expr::Var("j".into()),
                value: Expr::BinOp(
                    Box::new(Expr::Var("j".into())),
                    BinOp::Add,
                    Box::new(Expr::Literal(1.0)),
                ),
                compound: false,
            },
        ],
    });
    let stmt = Statement::ForC {
        init,
        condition,
        update,
        body: vec![],
    };

    let enums = HashMap::new();
    let registry = make_registry();
    let helpers = HelperRegistry::empty();
    let inline_counter = std::cell::Cell::new(0);
    let rendered = backends::c::render_statement(&stmt, 0, false, &enums, &registry, &helpers, &inline_counter, &[], false);

    // Should produce: for( j = 0, i = startIdx; ... ; i = i + 1, j = j + 1 )
    // NOT: for( j = 0;\ni = startIdx; ... )
    assert!(
        !rendered.contains(";\n"),
        "ForC init/update should use commas, not semicolons: {rendered}"
    );
    assert!(
        rendered.contains(", "),
        "ForC init/update should be comma-separated: {rendered}"
    );
}

#[test]
fn java_for_loop_multi_init_comma_separated() {
    use ta_codegen_lib::ir::*;

    // Build synthetic ForC: for(j=0, i=startIdx; i<=endIdx; i=i+1, j=j+1)
    let init = Box::new(Statement::Block {
        body: vec![
            Statement::Assign {
                target: Expr::Var("j".into()),
                value: Expr::Literal(0.0),
                compound: false,
            },
            Statement::Assign {
                target: Expr::Var("i".into()),
                value: Expr::Var("startIdx".into()),
                compound: false,
            },
        ],
    });
    let condition = Expr::BinOp(
        Box::new(Expr::Var("i".into())),
        BinOp::LessEq,
        Box::new(Expr::Var("endIdx".into())),
    );
    let update = Box::new(Statement::Block {
        body: vec![
            Statement::Assign {
                target: Expr::Var("i".into()),
                value: Expr::BinOp(
                    Box::new(Expr::Var("i".into())),
                    BinOp::Add,
                    Box::new(Expr::Literal(1.0)),
                ),
                compound: false,
            },
            Statement::Assign {
                target: Expr::Var("j".into()),
                value: Expr::BinOp(
                    Box::new(Expr::Var("j".into())),
                    BinOp::Add,
                    Box::new(Expr::Literal(1.0)),
                ),
                compound: false,
            },
        ],
    });
    let stmt = Statement::ForC {
        init,
        condition,
        update,
        body: vec![],
    };

    let enums = HashMap::new();
    let registry = make_registry();
    let helpers = HelperRegistry::empty();
    let inline_counter = std::cell::Cell::new(0);
    let address_of_vars = std::collections::HashSet::new();
    let double_address_of_vars = std::collections::HashSet::new();
    let float_input_params = std::collections::HashSet::new();
    let rendered = backends::java::render_statement(&stmt, 0, false, &enums, &registry, &helpers, &inline_counter, &address_of_vars, &double_address_of_vars, &float_input_params);

    // Should produce: for( j = 0, i = startIdx; ... ; i = i + 1, j = j + 1 )
    // NOT: for( j = 0;\ni = startIdx; ... )
    assert!(
        !rendered.contains(";\n"),
        "Java ForC init/update should use commas, not semicolons: {rendered}"
    );
    assert!(
        rendered.contains(", "),
        "Java ForC init/update should be comma-separated: {rendered}"
    );
}

// ---------------------------------------------------------------------------
// Rust ForC range iteration optimization
// ---------------------------------------------------------------------------

#[test]
fn rust_forc_emits_range_iteration_when_possible() {
    use ta_codegen_lib::backends::rust_lang::{render_statement, RustRenderCtx};
    use ta_codegen_lib::ir::*;

    // Build synthetic ForC: for(i=startIdx; i<=endIdx; i++)
    // Single counter, <= condition, simple increment by 1
    let init = Box::new(Statement::Assign {
        target: Expr::Var("i".into()),
        value: Expr::Var("startIdx".into()),
        compound: false,
    });
    let condition = Expr::BinOp(
        Box::new(Expr::Var("i".into())),
        BinOp::LessEq,
        Box::new(Expr::Var("endIdx".into())),
    );
    let update = Box::new(Statement::Assign {
        target: Expr::Var("i".into()),
        value: Expr::BinOp(
            Box::new(Expr::Var("i".into())),
            BinOp::Add,
            Box::new(Expr::IntLiteral(1)),
        ),
        compound: false,
    });
    let stmt = Statement::ForC {
        init,
        condition,
        update,
        body: vec![],
    };

    let ctx = RustRenderCtx::empty();
    let for_loop_vars: Vec<String> = vec![];
    let var_inits: std::collections::HashMap<String, &Expr> = std::collections::HashMap::new();
    let output_names: Vec<String> = vec![];
    let opt_real_params: Vec<String> = vec![];
    let enums = HashMap::new();
    let registry = make_registry();
    let helpers = HelperRegistry::empty();
    let inline_counter = std::cell::Cell::new(0);

    let rendered = render_statement(
        &stmt,
        0,
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
        rendered.contains("..") && rendered.contains("+ 1"),
        "Simple ForC should emit exclusive range iteration: {rendered}"
    );
    assert!(
        !rendered.contains("while "),
        "Simple ForC should not fall through to while: {rendered}"
    );
}

/// Regression: an inline-commented condition whose operand is a parenthesized
/// `||` group must keep that group parenthesized in the multi-line Rust render,
/// or precedence changes (`a && (b||c)` would become `(a&&b)||c`). CDLHIKKAKE hit
/// this and panicked in the Rust server.
///
/// The comment slots are per boolean-spine *leaf*, so the group's own two
/// operands each carry one — the count is what admits the multi-line path at all.
#[test]
fn rust_inline_condition_parenthesizes_or_operand() {
    use ta_codegen_lib::backends::rust_lang::{render_statement, RustRenderCtx};
    use ta_codegen_lib::ir::*;

    let cmp = |v: &str| {
        Expr::BinOp(
            Box::new(Expr::Var(v.into())),
            BinOp::Greater,
            Box::new(Expr::IntLiteral(0)),
        )
    };
    let or_bc = Expr::BinOp(Box::new(cmp("b")), BinOp::Or, Box::new(cmp("c")));
    let condition = Expr::BinOp(Box::new(cmp("a")), BinOp::And, Box::new(or_bc));
    let stmt = Statement::If {
        condition,
        then_body: vec![],
        else_body: vec![],
        // One slot per spine leaf (`a`, `b`, `c`) forces the multi-line path.
        cond_comments: vec![
            Some(vec!["one".into()]),
            Some(vec!["two".into()]),
            Some(vec!["three".into()]),
        ],
    };

    let ctx = RustRenderCtx::empty();
    let enums = HashMap::new();
    let registry = make_registry();
    let helpers = HelperRegistry::empty();
    let inline_counter = std::cell::Cell::new(0);
    let rendered = render_statement(
        &stmt,
        0,
        &ctx,
        &[],
        &std::collections::HashMap::new(),
        &[],
        &[],
        &enums,
        &registry,
        &helpers,
        &inline_counter,
    );

    // Strip comments and whitespace, then confirm the `||` group is parenthesized.
    let code: String = rendered
        .lines()
        .map(|l| l.split("//").next().unwrap_or(""))
        .collect::<Vec<_>>()
        .join("");
    let flat: String = code.chars().filter(|c| !c.is_whitespace()).collect();
    assert!(
        flat.contains("(b>0||c>0)"),
        "the `||` operand must stay parenthesized in the multi-line render: {rendered}"
    );
    // Vacuity guard: the assertion above also holds of the flat one-line form.
    assert_eq!(
        rendered.lines().filter(|l| l.contains("//")).count(),
        3,
        "expected one commented line per leaf: {rendered}"
    );
}

/// Regression for the boolean-context wrapping that broke the shipped Core.java
/// twice: a condition that is a single-return candle helper whose body is a
/// `(comparison) ? 1 : 0` ternary. The Java renderer inlines the helper and
/// collapses the ternary to the bare comparison (already boolean), so
/// is_boolean_expr must agree and NOT wrap it with `!= 0` (`boolean != 0` is a
/// Java type error). ta_realbodygapup is one of the real helpers that hit this.
#[test]
fn java_condition_from_bool_ternary_helper_is_not_wrapped() {
    use ta_codegen_lib::ir::*;

    // if( ta_realbodygapup(inOpen[i-1], inClose[i-1], inOpen[i-2], inClose[i-2]) ) {}
    let arg = |a: &str, k: i64| {
        Expr::ArrayAccess(
            a.into(),
            Box::new(Expr::BinOp(
                Box::new(Expr::Var("i".into())),
                BinOp::Sub,
                Box::new(Expr::IntLiteral(k)),
            )),
        )
    };
    let cond = Expr::FuncCall(
        "ta_realbodygapup".into(),
        vec![
            arg("inOpen", 1),
            arg("inClose", 1),
            arg("inOpen", 2),
            arg("inClose", 2),
        ],
    );
    let stmt = Statement::If {
        condition: cond,
        then_body: vec![],
        else_body: vec![],
        cond_comments: vec![],
    };

    let enums = HashMap::new();
    let registry = make_registry();
    let helpers = make_helper_registry();
    let inline_counter = std::cell::Cell::new(0);
    let address_of_vars = std::collections::HashSet::new();
    let double_address_of_vars = std::collections::HashSet::new();
    let float_input_params = std::collections::HashSet::new();
    let rendered = backends::java::render_statement(
        &stmt, 0, false, &enums, &registry, &helpers, &inline_counter,
        &address_of_vars, &double_address_of_vars, &float_input_params,
    );

    assert!(
        rendered.contains("Math.min") && rendered.contains('>'),
        "helper should inline to the bare comparison: {rendered}"
    );
    assert!(
        !rendered.contains("!= 0"),
        "a collapsed bool ternary must NOT be wrapped with `!= 0` (that is \
         `boolean != 0`, a Java type error): {rendered}"
    );
}

/// Complement to the above: a `cond ? 1 : 0` whose condition is an int-typed
/// expression (a bare variable, not a comparison). The renderer still collapses
/// it to the bare variable, which is NOT boolean, so is_boolean_expr must return
/// false and the `!= 0` wrap MUST be applied (`if( flag )` is invalid Java).
/// This pins the `is_boolean_expr(cond)` guard on the collapse.
#[test]
fn java_condition_from_int_ternary_is_wrapped() {
    use ta_codegen_lib::ir::*;

    let cond = Expr::Ternary(
        Box::new(Expr::Var("flag".into())),
        Box::new(Expr::IntLiteral(1)),
        Box::new(Expr::IntLiteral(0)),
    );
    let stmt = Statement::If {
        condition: cond,
        then_body: vec![],
        else_body: vec![],
        cond_comments: vec![],
    };

    let enums = HashMap::new();
    let registry = make_registry();
    let helpers = HelperRegistry::empty();
    let inline_counter = std::cell::Cell::new(0);
    let address_of_vars = std::collections::HashSet::new();
    let double_address_of_vars = std::collections::HashSet::new();
    let float_input_params = std::collections::HashSet::new();
    let rendered = backends::java::render_statement(
        &stmt, 0, false, &enums, &registry, &helpers, &inline_counter,
        &address_of_vars, &double_address_of_vars, &float_input_params,
    );

    assert!(
        rendered.contains("flag") && rendered.contains("!= 0"),
        "a collapsed int ternary condition must be wrapped with `!= 0`: {rendered}"
    );
}

/// Rust complement: Rust does not collapse `? 1 : 0` — it keeps the integer
/// ternary and, in a boolean context, wraps `!= 0`. Pins that a candle helper
/// inlining to an int ternary, used as an `&&` operand, keeps its `!= 0` wrap
/// so the generated Rust type-checks.
#[test]
fn rust_condition_from_int_ternary_helper_is_wrapped() {
    use ta_codegen_lib::backends::rust_lang::{render_statement, RustRenderCtx};
    use ta_codegen_lib::ir::*;

    let arg = |a: &str, k: i64| {
        Expr::ArrayAccess(
            a.into(),
            Box::new(Expr::BinOp(
                Box::new(Expr::Var("i".into())),
                BinOp::Sub,
                Box::new(Expr::IntLiteral(k)),
            )),
        )
    };
    let helper_call = Expr::FuncCall(
        "ta_realbodygapup".into(),
        vec![
            arg("inOpen", 1),
            arg("inClose", 1),
            arg("inOpen", 2),
            arg("inClose", 2),
        ],
    );
    // Force the boolean-context path: helper && (a > 0)
    let cond = Expr::BinOp(
        Box::new(helper_call),
        BinOp::And,
        Box::new(Expr::BinOp(
            Box::new(Expr::Var("a".into())),
            BinOp::Greater,
            Box::new(Expr::IntLiteral(0)),
        )),
    );
    let stmt = Statement::If {
        condition: cond,
        then_body: vec![],
        else_body: vec![],
        cond_comments: vec![],
    };

    let ctx = RustRenderCtx::empty();
    let enums = HashMap::new();
    let registry = make_registry();
    let helpers = make_helper_registry();
    let inline_counter = std::cell::Cell::new(0);
    let rendered = render_statement(
        &stmt, 0, &ctx, &[], &std::collections::HashMap::new(), &[], &[],
        &enums, &registry, &helpers, &inline_counter,
    );

    assert!(
        rendered.contains("!= 0"),
        "the int-producing helper used in a boolean context must keep its \
         `!= 0` wrap in Rust: {rendered}"
    );
}

// ---------------------------------------------------------------------------
// 15. HT_TRENDMODE: verify Hilbert transform macros parse and generate
// ---------------------------------------------------------------------------

#[test]
fn ht_trendmode_parses_and_generates() {
    let (func, enums) = load_indicator("ht_trendmode");
    let _outputs = generate_all(&func, &enums);
    // If we get here without panic, parsing and generation succeeded
}

#[test]
fn rust_forc_multi_init_falls_through_to_while() {
    use ta_codegen_lib::backends::rust_lang::{render_statement, RustRenderCtx};
    use ta_codegen_lib::ir::*;

    // Build ForC with multi-init Block — should NOT get range optimization
    let init = Box::new(Statement::Block {
        body: vec![
            Statement::Assign {
                target: Expr::Var("j".into()),
                value: Expr::Literal(0.0),
                compound: false,
            },
            Statement::Assign {
                target: Expr::Var("i".into()),
                value: Expr::Var("startIdx".into()),
                compound: false,
            },
        ],
    });
    let condition = Expr::BinOp(
        Box::new(Expr::Var("i".into())),
        BinOp::LessEq,
        Box::new(Expr::Var("endIdx".into())),
    );
    let update = Box::new(Statement::Block {
        body: vec![
            Statement::Assign {
                target: Expr::Var("i".into()),
                value: Expr::BinOp(
                    Box::new(Expr::Var("i".into())),
                    BinOp::Add,
                    Box::new(Expr::Literal(1.0)),
                ),
                compound: false,
            },
            Statement::Assign {
                target: Expr::Var("j".into()),
                value: Expr::BinOp(
                    Box::new(Expr::Var("j".into())),
                    BinOp::Add,
                    Box::new(Expr::Literal(1.0)),
                ),
                compound: false,
            },
        ],
    });
    let stmt = Statement::ForC {
        init,
        condition,
        update,
        body: vec![],
    };

    let ctx = RustRenderCtx::empty();
    let for_loop_vars: Vec<String> = vec![];
    let var_inits: std::collections::HashMap<String, &Expr> = std::collections::HashMap::new();
    let output_names: Vec<String> = vec![];
    let opt_real_params: Vec<String> = vec![];
    let enums = HashMap::new();
    let registry = make_registry();
    let helpers = HelperRegistry::empty();
    let inline_counter = std::cell::Cell::new(0);

    let rendered = render_statement(
        &stmt,
        0,
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
        rendered.contains("while "),
        "Multi-init ForC should fall through to while: {rendered}"
    );
}

// ---------------------------------------------------------------------------
// 16. Math function idiomatic rendering per backend
// ---------------------------------------------------------------------------

#[test]
fn backends_render_max_min_fmax_fmin_abs() {
    use ta_codegen_lib::backends;
    use ta_codegen_lib::ir::{
        Expr, FuncDef, Input, LookbackExpr, Output, ParamType, Statement, VarType,
    };

    // Build a synthetic FuncDef whose body assigns each math function to a variable.
    // Variable a = max(x, y)
    // Variable b = min(x, y)
    // Variable c = fmax(x, y)
    // Variable d = fmin(x, y)
    // Variable e = ABS(x)
    let make_assign = |var: &str, func: &str, args: Vec<Expr>| Statement::Assign {
        target: Expr::Var(var.to_string()),
        value: Expr::FuncCall(func.to_string(), args),
        compound: false,
    };

    let x = Expr::Var("x".to_string());
    let y = Expr::Var("y".to_string());

    let body = vec![
        Statement::VarDecl {
            var_type: VarType::Real,
            name: "x".to_string(),
            init: Some(Expr::Literal(1.0)),
        },
        Statement::VarDecl {
            var_type: VarType::Real,
            name: "y".to_string(),
            init: Some(Expr::Literal(2.0)),
        },
        Statement::VarDecl {
            var_type: VarType::Real,
            name: "a".to_string(),
            init: None,
        },
        Statement::VarDecl {
            var_type: VarType::Real,
            name: "b".to_string(),
            init: None,
        },
        Statement::VarDecl {
            var_type: VarType::Real,
            name: "c".to_string(),
            init: None,
        },
        Statement::VarDecl {
            var_type: VarType::Real,
            name: "d".to_string(),
            init: None,
        },
        Statement::VarDecl {
            var_type: VarType::Real,
            name: "e".to_string(),
            init: None,
        },
        make_assign("a", "max", vec![x.clone(), y.clone()]),
        make_assign("b", "min", vec![x.clone(), y.clone()]),
        make_assign("c", "fmax", vec![x.clone(), y.clone()]),
        make_assign("d", "fmin", vec![x.clone(), y.clone()]),
        make_assign("e", "ABS", vec![x.clone()]),
    ];

    let func = FuncDef {
        name: "TESTFUNC".to_string(),
        group: "Test".to_string(),
        description: None,
        hint: None,
        flags: vec![],
        inputs: vec![Input::new("inReal", ParamType::Real)],
        optional_inputs: vec![],
        outputs: vec![Output {
            name: "outReal".to_string(),
            param_type: ParamType::Real,
            flags: vec![],
        }],
        lookback: Some(LookbackExpr::Literal(0)),
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

    let enums = std::collections::HashMap::new();
    let registry = make_registry();
    let helpers = HelperRegistry::empty();

    let c_out = backends::c::generate(&func, &enums, &registry, &helpers);
    let java_out = backends::java::generate(&func, &enums, &registry, &helpers);
    let rust_out = backends::rust_lang::generate(&func, &enums, &registry, &helpers);

    // C: max/min → the ta_utility.h branch macros max()/min() (NOT C99 fmin/fmax);
    // ABS(x) → fabs(x). See #102: fmin/fmax carry IEEE-754 NaN/signed-zero semantics
    // that block a branchless (vectorizable) lowering and force int→double
    // round-trips; the branch macros match the pre-cutover reference bit-for-bit.
    assert!(
        c_out.contains("= max(") && c_out.contains("= min("),
        "C: max/min should render as the ta_utility.h branch macros max()/min() (#102): {c_out}"
    );
    assert!(
        c_out.contains("fabs("),
        "C: ABS should render as fabs(): {c_out}"
    );
    // C must NOT emit the C99 fmax()/fmin() library calls (the #102 regression)
    assert!(
        !c_out.contains("fmax(") && !c_out.contains("fmin("),
        "C: must not emit the C99 fmax()/fmin() library calls (#102): {c_out}"
    );
    // C must NOT emit ABS() calls
    assert!(
        !c_out.contains("ABS("),
        "C: must not emit ABS() calls"
    );

    // Java: max/fmax → Math.max, min/fmin → Math.min, ABS → Math.abs
    assert!(
        java_out.contains("Math.max("),
        "Java: max/fmax should render as Math.max(): {java_out}"
    );
    assert!(
        java_out.contains("Math.min("),
        "Java: min/fmin should render as Math.min(): {java_out}"
    );
    assert!(
        java_out.contains("Math.abs("),
        "Java: ABS should render as Math.abs(): {java_out}"
    );

    // Rust: max/fmax → .max(), min/fmin → .min(), ABS → .ta_abs() (generic) or .abs()
    assert!(
        rust_out.contains(".max("),
        "Rust: max/fmax should render as .max(): {rust_out}"
    );
    assert!(
        rust_out.contains(".min("),
        "Rust: min/fmin should render as .min(): {rust_out}"
    );
    assert!(
        rust_out.contains(".abs()"),
        "Rust: ABS should render as .abs(): {rust_out}"
    );
    // Rust must NOT emit bare ABS() free-function calls
    assert!(
        !rust_out.contains("ABS("),
        "Rust: must not emit bare ABS() calls"
    );
}

#[test]
fn backends_render_math_functions_idiomatically() {
    let (func, enums) = load_indicator("ht_trendmode");
    let registry = make_registry();
    let helpers = HelperRegistry::empty();

    let c_out = backends::c::generate(&func, &enums, &registry, &helpers);
    let java_out = backends::java::generate(&func, &enums, &registry, &helpers);
    let rust_out = backends::rust_lang::generate(&func, &enums, &registry, &helpers);

    // C: plain atan() from <math.h>
    assert!(
        c_out.contains("atan("),
        "C backend should render atan() as plain C math call: {}",
        &c_out[c_out.find("atan").unwrap_or(0)..c_out.find("atan").unwrap_or(0) + 40]
    );
    // C: must NOT produce TA_atan
    assert!(
        !c_out.contains("TA_atan(") && !c_out.contains("TA_S_atan("),
        "C backend must not prefix math functions with TA_"
    );

    // Java: Math.atan()
    assert!(
        java_out.contains("Math.atan("),
        "Java backend should render Math.atan()"
    );
    // Java: fabs renders as Math.abs, not Math.fabs
    let java_fabs = java_out.contains("Math.abs(");
    let java_wrong_fabs = java_out.contains("Math.fabs(");
    if java_out.contains("fabs(") || java_out.contains("Math.abs(") || java_out.contains("Math.fabs(") {
        assert!(java_fabs, "Java backend should render fabs as Math.abs");
        assert!(!java_wrong_fabs, "Java backend must not render Math.fabs");
    }

    // Rust: method call syntax on concrete f64 — .atan()
    assert!(
        rust_out.contains(".atan()"),
        "Rust backend should render atan as .atan() method call"
    );
    // Rust must NOT produce bare atan() free-function calls (but .atan() is fine)
    let has_bare_atan = rust_out
        .match_indices("atan(")
        .any(|(i, _)| !rust_out[..i].ends_with('.'));
    assert!(
        !has_bare_atan,
        "Rust backend must not render math functions as free-function calls"
    );
}

#[test]
fn report_failing_parse_indicators() {
    let indicators = discover_indicators();
    let mut failing = Vec::new();
    for name in &indicators {
        let base = std::path::Path::new(env!("CARGO_MANIFEST_DIR")).join("../../ta_codegen/input");
        let c_path = base.join(format!("{}/{}.c", name, name));
        let result = std::panic::catch_unwind(std::panic::AssertUnwindSafe(|| {
            parser::c_source::parse_c_source(&c_path);
        }));
        if let Err(e) = result {
            let msg = if let Some(s) = e.downcast_ref::<String>() {
                s.clone()
            } else if let Some(s) = e.downcast_ref::<&str>() {
                s.to_string()
            } else {
                "unknown panic".to_string()
            };
            failing.push(format!("{}: {}", name, msg));
        }
    }
    for f in &failing {
        eprintln!("PARSE_FAIL: {}", f);
    }
    eprintln!("Total failing: {} / {}", failing.len(), indicators.len());
}

#[test]
fn helper_def_stores_params_and_body() {
    use ta_codegen_lib::ir::{BinOp, Expr, HelperDef, HelperParam, Statement, VarType};

    let helper = HelperDef {
        name: "ta_realbody".to_string(),
        return_type: VarType::Real,
        params: vec![
            HelperParam { name: "close".to_string(), var_type: VarType::Real },
            HelperParam { name: "open".to_string(), var_type: VarType::Real },
        ],
        body: vec![Statement::Return {
            value: Some(Expr::FuncCall(
                "fabs".to_string(),
                vec![Expr::BinOp(
                    Box::new(Expr::Var("close".to_string())),
                    BinOp::Sub,
                    Box::new(Expr::Var("open".to_string())),
                )],
            )),
        }],
    };
    assert_eq!(helper.name, "ta_realbody");
    assert_eq!(helper.params.len(), 2);
    assert_eq!(helper.params[0].name, "close");
}

#[test]
fn parse_helper_file_extracts_functions() {
    use ta_codegen_lib::parser::c_source::parse_helper_file_str;

    let source = r#"
double ta_realbody(double close, double open) {
    return fabs(close - open);
}

int ta_candlecolor(double close, double open) {
    return (close >= open) ? 1 : -1;
}
"#;

    let helpers = parse_helper_file_str(source);
    assert_eq!(helpers.len(), 2);
    assert_eq!(helpers[0].name, "ta_realbody");
    assert_eq!(helpers[0].params.len(), 2);
    assert_eq!(helpers[0].params[0].name, "close");
    assert_eq!(helpers[1].name, "ta_candlecolor");
    assert_eq!(helpers[1].params.len(), 2);
}

#[test]
fn parse_helper_with_switch() {
    use ta_codegen_lib::parser::c_source::parse_helper_file_str;
    use ta_codegen_lib::ir::Statement;

    let source = r#"
double ta_candlerange(int rangeType, double open, double high, double low, double close) {
    switch (rangeType) {
        case 0: return fabs(close - open);
        case 1: return high - low;
        case 2: return high - low - fabs(close - open);
        default: return 0.0;
    }
}
"#;

    let helpers = parse_helper_file_str(source);
    assert_eq!(helpers.len(), 1);
    assert_eq!(helpers[0].name, "ta_candlerange");
    assert_eq!(helpers[0].params.len(), 5);
    assert!(matches!(helpers[0].body[0], Statement::Switch { .. }));
}

#[test]
fn parse_helper_file_reads_from_disk() {
    use ta_codegen_lib::parser::c_source::parse_helper_file;
    let path = std::path::Path::new(env!("CARGO_MANIFEST_DIR"))
        .join("../../ta_codegen/input/helpers/candlestick.c");
    let helpers = parse_helper_file(&path);
    assert_eq!(helpers.len(), 11);
    assert!(helpers.iter().any(|h| h.name == "ta_realbody" && h.params.len() == 2));
    assert!(helpers.iter().any(|h| h.name == "ta_candleaverage" && h.params.len() == 8));
}

#[test]
fn helper_registry_loads_from_disk() {
    use ta_codegen_lib::helper_registry::HelperRegistry;

    let base = std::path::Path::new(env!("CARGO_MANIFEST_DIR")).join("../../ta_codegen/input");
    let registry = HelperRegistry::from_dir(&base);

    // Should find all helpers from candlestick.c, range.c, rounding.c
    assert!(registry.get("ta_realbody").is_some());
    assert!(registry.get("ta_candlerange").is_some());
    assert!(registry.get("ta_true_range").is_some());
    assert!(registry.get("ta_round_pos").is_some());
    assert!(registry.get("ta_sar_rounding").is_some());
    assert!(registry.get("ta_candleaverage").is_some());

    // Should NOT contain indicator functions
    assert!(registry.get("sma").is_none());
    assert!(registry.get("ema").is_none());
}

// ---------------------------------------------------------------------------
// Expression inlining tests
// ---------------------------------------------------------------------------

/// Load a HelperRegistry from the real helper files on disk.
fn make_helper_registry() -> HelperRegistry {
    let base = Path::new(env!("CARGO_MANIFEST_DIR")).join("../../ta_codegen/input");
    HelperRegistry::from_dir(&base)
}

#[test]
fn substitute_expr_replaces_vars() {
    use ta_codegen_lib::helper_registry::substitute_expr;
    use ta_codegen_lib::ir::{BinOp, Expr};
    use std::collections::HashMap;

    // Build: close - open
    let expr = Expr::BinOp(
        Box::new(Expr::Var("close".to_string())),
        BinOp::Sub,
        Box::new(Expr::Var("open".to_string())),
    );

    let mut subs = HashMap::new();
    subs.insert("close".to_string(), Expr::Var("inClose[i]".to_string()));
    subs.insert("open".to_string(), Expr::Var("inOpen[i]".to_string()));

    let result = substitute_expr(&expr, &subs);
    // Result should be: inClose[i] - inOpen[i]
    if let Expr::BinOp(l, BinOp::Sub, r) = &result {
        if let (Expr::Var(ln), Expr::Var(rn)) = (l.as_ref(), r.as_ref()) {
            assert_eq!(ln, "inClose[i]");
            assert_eq!(rn, "inOpen[i]");
        } else {
            panic!("Expected Var nodes after substitution, got: {:?}", result);
        }
    } else {
        panic!("Expected BinOp after substitution, got: {:?}", result);
    }
}

#[test]
fn try_inline_expr_works_for_single_return() {
    use ta_codegen_lib::helper_registry::try_inline_expr;
    use ta_codegen_lib::ir::{BinOp, Expr, HelperDef, HelperParam, Statement, VarType};

    // ta_realbody(close, open) => return fabs(close - open);
    let helper = HelperDef {
        name: "ta_realbody".to_string(),
        return_type: VarType::Real,
        params: vec![
            HelperParam { name: "close".to_string(), var_type: VarType::Real },
            HelperParam { name: "open".to_string(), var_type: VarType::Real },
        ],
        body: vec![Statement::Return {
            value: Some(Expr::FuncCall(
                "fabs".to_string(),
                vec![Expr::BinOp(
                    Box::new(Expr::Var("close".to_string())),
                    BinOp::Sub,
                    Box::new(Expr::Var("open".to_string())),
                )],
            )),
        }],
    };

    let args = vec![
        Expr::Var("inClose[i]".to_string()),
        Expr::Var("inOpen[i]".to_string()),
    ];

    let result = try_inline_expr(&helper, &args);
    assert!(result.is_some(), "Single-return helper should be inlineable");

    // The inlined result should be fabs(inClose[i] - inOpen[i])
    let inlined = result.unwrap();
    if let Expr::FuncCall(name, inner_args) = &inlined {
        assert_eq!(name, "fabs");
        assert_eq!(inner_args.len(), 1);
    } else {
        panic!("Expected FuncCall(fabs, ...) after inlining, got: {:?}", inlined);
    }
}

#[test]
fn try_inline_returns_none_for_multi_statement() {
    use ta_codegen_lib::helper_registry::try_inline_expr;
    use ta_codegen_lib::ir::{Expr, HelperDef, HelperParam, Statement, VarType};

    // A multi-statement helper: { int x = 0; return x; }
    let helper = HelperDef {
        name: "multi".to_string(),
        return_type: VarType::Integer,
        params: vec![HelperParam { name: "a".to_string(), var_type: VarType::Integer }],
        body: vec![
            Statement::VarDecl {
                var_type: VarType::Integer,
                name: "x".to_string(),
                init: Some(Expr::IntLiteral(0)),
            },
            Statement::Return {
                value: Some(Expr::Var("x".to_string())),
            },
        ],
    };

    let result = try_inline_expr(&helper, &[Expr::IntLiteral(42)]);
    assert!(result.is_none(), "Multi-statement helper should NOT be inlineable");
}

#[test]
fn c_backend_inlines_single_expr_helper() {
    let helpers = make_helper_registry();
    let registry = make_registry();

    // Load a candlestick indicator that calls ta_realbody
    let (func, enums) = load_indicator("cdlkicking");

    let output = backends::c::generate(&func, &enums, &registry, &helpers);

    // ta_realbody(close, open) => fabs(close - open)
    // After inlining, the output should contain fabs( (from inlined ta_realbody body)
    // and should NOT contain "ta_realbody(" as a direct call
    assert!(
        output.contains("fabs("),
        "C output should contain fabs( from inlined ta_realbody"
    );
    assert!(
        !output.contains("ta_realbody("),
        "C output should NOT contain ta_realbody( -- it should be inlined"
    );

    // ta_candlecolor is also single-expression: (close >= open) ? 1 : -1
    // After inlining it should not appear as a function call
    assert!(
        !output.contains("ta_candlecolor("),
        "C output should NOT contain ta_candlecolor( -- it should be inlined"
    );
}

#[test]
fn java_backend_inlines_single_expr_helper() {
    let helpers = make_helper_registry();
    let registry = make_registry();

    let (func, enums) = load_indicator("cdlkicking");

    let output = backends::java::generate(&func, &enums, &registry, &helpers);

    // Java uses Math.abs instead of fabs, but inlined ta_realbody should produce Math.abs(
    assert!(
        output.contains("Math.abs("),
        "Java output should contain Math.abs( from inlined ta_realbody"
    );
    assert!(
        !output.contains("ta_realbody("),
        "Java output should NOT contain ta_realbody( -- it should be inlined"
    );
    assert!(
        !output.contains("ta_candlecolor("),
        "Java output should NOT contain ta_candlecolor( -- it should be inlined"
    );
}

#[test]
fn rust_backend_inlines_single_expr_helper() {
    let helpers = make_helper_registry();
    let registry = make_registry();

    let (func, enums) = load_indicator("cdlkicking");

    let output = backends::rust_lang::generate(&func, &enums, &registry, &helpers);

    // Rust uses .abs() for fabs, so inlined ta_realbody should produce that
    // The Rust backend renders fabs as a function call
    assert!(
        !output.contains("ta_realbody("),
        "Rust output should NOT contain ta_realbody( -- it should be inlined"
    );
    assert!(
        !output.contains("ta_candlecolor("),
        "Rust output should NOT contain ta_candlecolor( -- it should be inlined"
    );
}

#[test]
fn inlining_with_empty_registry_leaves_helpers_as_calls() {
    let helpers = HelperRegistry::empty();
    let registry = make_registry();

    let (func, enums) = load_indicator("cdlkicking");

    let output = backends::c::generate(&func, &enums, &registry, &helpers);

    // With an empty helper registry, helper calls should remain as-is
    // (they'll be treated as regular function calls by the fallback path)
    assert!(
        output.contains("ta_realbody(") || output.contains("TA_ta_realbody("),
        "With empty helpers, ta_realbody should remain as a function call"
    );
}

// ---------------------------------------------------------------------------
// Block inlining tests (Task 10)
// ---------------------------------------------------------------------------

/// Build a minimal FuncDef whose body contains an assignment
/// calling a given helper function.
fn make_func_with_helper_call(
    call_name: &str,
    args: Vec<ir::Expr>,
) -> ir::FuncDef {
    let body = vec![
        ir::Statement::VarDecl {
            var_type: ir::VarType::Real,
            name: "result".to_string(),
            init: None,
        },
        ir::Statement::Assign {
            target: ir::Expr::Var("result".to_string()),
            value: ir::Expr::FuncCall(call_name.to_string(), args),
            compound: false,
        },
    ];
    ir::FuncDef {
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
        lookback: Some(ir::LookbackExpr::Literal(0)),
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
    }
}

#[test]
fn c_backend_inlines_multi_statement_helper_with_temp_var() {
    // ta_true_range has 3 VarDecls + 2 Ifs + Return => multi-statement
    let func = make_func_with_helper_call(
        "ta_true_range",
        vec![
            ir::Expr::Var("high".to_string()),
            ir::Expr::Var("low".to_string()),
            ir::Expr::Var("prev".to_string()),
        ],
    );
    let enums = HashMap::new();
    let registry = make_registry();
    let helpers = make_helpers();

    let output = backends::c::generate(&func, &enums, &registry, &helpers);

    // Should NOT contain ta_true_range as a function call
    assert!(
        !output.contains("ta_true_range("),
        "ta_true_range should be inlined, not called: {output}"
    );
    // Should contain a temp var declaration
    assert!(
        output.contains("_true_range_"),
        "Should have a temp var like _true_range_0: {output}"
    );
    // Should contain the inlined body pattern (the if-statements)
    assert!(
        output.contains("if("),
        "Inlined body should contain if-statements: {output}"
    );
}

#[test]
fn c_backend_inlines_candlerange_switch() {
    // ta_candlerange emits a C preprocessor macro instead of expanded code
    let func = make_func_with_helper_call(
        "ta_candlerange",
        vec![
            ir::Expr::Var("BodyLong_rangeType".to_string()),
            ir::Expr::ArrayAccess("inOpen".to_string(), Box::new(ir::Expr::Var("i".to_string()))),
            ir::Expr::ArrayAccess("inHigh".to_string(), Box::new(ir::Expr::Var("i".to_string()))),
            ir::Expr::ArrayAccess("inLow".to_string(), Box::new(ir::Expr::Var("i".to_string()))),
            ir::Expr::ArrayAccess("inClose".to_string(), Box::new(ir::Expr::Var("i".to_string()))),
        ],
    );
    let enums = HashMap::new();
    let registry = make_registry();
    let helpers = make_helpers();

    let output = backends::c::generate(&func, &enums, &registry, &helpers);

    assert!(
        output.contains("TA_CANDLERANGE(BodyLong,i)"),
        "ta_candlerange should emit C macro: {output}"
    );
    // No expanded temporaries — the macro handles everything
    assert!(
        !output.contains("_candlerange_"),
        "Should NOT have temp var — macro replaces it: {output}"
    );
}

#[test]
fn inlining_counter_avoids_name_collisions() {
    // Call ta_candlerange twice in a FuncDef body — both emit macros with different settings
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
        lookback: Some(ir::LookbackExpr::Literal(0)),
        body: vec![
            ir::Statement::VarDecl {
                var_type: ir::VarType::Real,
                name: "a".to_string(),
                init: None,
            },
            ir::Statement::VarDecl {
                var_type: ir::VarType::Real,
                name: "b".to_string(),
                init: None,
            },
            ir::Statement::Assign {
                target: ir::Expr::Var("a".to_string()),
                value: ir::Expr::FuncCall(
                    "ta_candlerange".to_string(),
                    vec![
                        ir::Expr::Var("BodyLong_rangeType".to_string()),
                        ir::Expr::ArrayAccess("inOpen".to_string(), Box::new(ir::Expr::Var("i".to_string()))),
                        ir::Expr::ArrayAccess("inHigh".to_string(), Box::new(ir::Expr::Var("i".to_string()))),
                        ir::Expr::ArrayAccess("inLow".to_string(), Box::new(ir::Expr::Var("i".to_string()))),
                        ir::Expr::ArrayAccess("inClose".to_string(), Box::new(ir::Expr::Var("i".to_string()))),
                    ],
                ),
                compound: false,
            },
            ir::Statement::Assign {
                target: ir::Expr::Var("b".to_string()),
                value: ir::Expr::FuncCall(
                    "ta_candlerange".to_string(),
                    vec![
                        ir::Expr::Var("BodyShort_rangeType".to_string()),
                        ir::Expr::ArrayAccess("inOpen".to_string(), Box::new(ir::Expr::BinOp(
                            Box::new(ir::Expr::Var("i".to_string())),
                            ir::BinOp::Sub,
                            Box::new(ir::Expr::IntLiteral(1)),
                        ))),
                        ir::Expr::ArrayAccess("inHigh".to_string(), Box::new(ir::Expr::BinOp(
                            Box::new(ir::Expr::Var("i".to_string())),
                            ir::BinOp::Sub,
                            Box::new(ir::Expr::IntLiteral(1)),
                        ))),
                        ir::Expr::ArrayAccess("inLow".to_string(), Box::new(ir::Expr::BinOp(
                            Box::new(ir::Expr::Var("i".to_string())),
                            ir::BinOp::Sub,
                            Box::new(ir::Expr::IntLiteral(1)),
                        ))),
                        ir::Expr::ArrayAccess("inClose".to_string(), Box::new(ir::Expr::BinOp(
                            Box::new(ir::Expr::Var("i".to_string())),
                            ir::BinOp::Sub,
                            Box::new(ir::Expr::IntLiteral(1)),
                        ))),
                    ],
                ),
                compound: false,
            },
        ],
        private_body: vec![],
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
    let helpers = make_helpers();

    let output = backends::c::generate(&func, &enums, &registry, &helpers);

    // Both calls should emit C macros with different settings
    assert!(
        output.contains("TA_CANDLERANGE(BodyLong,i)"),
        "First call should emit BodyLong macro: {output}"
    );
    assert!(
        output.contains("TA_CANDLERANGE(BodyShort,i - 1)"),
        "Second call should emit BodyShort macro with offset: {output}"
    );
    // No expanded temporaries
    assert!(
        !output.contains("_candlerange_"),
        "Should NOT have temp vars — macros replace them: {output}"
    );
}

#[test]
fn java_backend_inlines_multi_statement_helper() {
    let func = make_func_with_helper_call(
        "ta_true_range",
        vec![
            ir::Expr::Var("high".to_string()),
            ir::Expr::Var("low".to_string()),
            ir::Expr::Var("prev".to_string()),
        ],
    );
    let enums = HashMap::new();
    let registry = make_registry();
    let helpers = make_helpers();

    let output = backends::java::generate(&func, &enums, &registry, &helpers);

    assert!(
        !output.contains("ta_true_range("),
        "Java: ta_true_range should be inlined: {output}"
    );
    assert!(
        output.contains("_true_range_"),
        "Java: should have a temp var: {output}"
    );
}

#[test]
fn rust_backend_inlines_multi_statement_helper() {
    let func = make_func_with_helper_call(
        "ta_true_range",
        vec![
            ir::Expr::Var("high".to_string()),
            ir::Expr::Var("low".to_string()),
            ir::Expr::Var("prev".to_string()),
        ],
    );
    let enums = HashMap::new();
    let registry = make_registry();
    let helpers = make_helpers();

    let output = backends::rust_lang::generate(&func, &enums, &registry, &helpers);

    assert!(
        !output.contains("ta_true_range("),
        "Rust: ta_true_range should be inlined: {output}"
    );
    assert!(
        output.contains("_true_range_"),
        "Rust: should have a temp var: {output}"
    );
}

#[test]
fn nested_block_inlining_candleaverage_calls_candlerange() {
    // ta_candleaverage emits a C macro — the nested ta_candlerange call
    // is handled by the macro definition, not by the codegen.
    let func = make_func_with_helper_call(
        "ta_candleaverage",
        vec![
            ir::Expr::Var("BodyLong_rangeType".to_string()),
            ir::Expr::Var("BodyLong_avgPeriod".to_string()),
            ir::Expr::Var("BodyLong_factor".to_string()),
            ir::Expr::Var("periodTotal".to_string()),
            ir::Expr::ArrayAccess("inOpen".to_string(), Box::new(ir::Expr::Var("i".to_string()))),
            ir::Expr::ArrayAccess("inHigh".to_string(), Box::new(ir::Expr::Var("i".to_string()))),
            ir::Expr::ArrayAccess("inLow".to_string(), Box::new(ir::Expr::Var("i".to_string()))),
            ir::Expr::ArrayAccess("inClose".to_string(), Box::new(ir::Expr::Var("i".to_string()))),
        ],
    );
    let enums = HashMap::new();
    let registry = make_registry();
    let helpers = make_helpers();

    let output = backends::c::generate(&func, &enums, &registry, &helpers);

    // Should emit a single C macro — no expanded temporaries
    assert!(
        output.contains("TA_CANDLEAVERAGE(BodyLong,periodTotal,i)"),
        "ta_candleaverage should emit C macro: {output}"
    );
    assert!(
        !output.contains("_candleaverage_"),
        "Should NOT have _candleaverage_ temp var: {output}"
    );
    assert!(
        !output.contains("_candlerange_"),
        "Should NOT have _candlerange_ temp var: {output}"
    );
}

// ---------------------------------------------------------------------------
// Candle settings unpacking tests (Task 11)
// ---------------------------------------------------------------------------

#[test]
fn c_backend_emits_candle_settings_unpacking() {
    let (func, enums) = load_indicator("cdl2crows");
    let registry = make_registry();
    let helpers = make_helpers();
    let c_out = backends::c::generate(&func, &enums, &registry, &helpers);

    // Only the properties the rendered C actually reads. CDL2CROWS reaches
    // BodyLong through TA_CANDLERANGE / TA_CANDLEAVERAGE, which take the setting
    // as a token and read TA_Globals themselves, so `_avgPeriod` is the only
    // local it needs -- declaring the other two is the -Wunused-variable this
    // emitter used to produce on 62 candlestick files.
    assert!(
        c_out.contains("BodyLong_avgPeriod = TA_Globals->candleSettings[TA_BodyLong].avgPeriod"),
        "C output should unpack BodyLong_avgPeriod: {c_out}"
    );
    assert!(
        !c_out.contains("BodyLong_rangeType"),
        "C output should not declare BodyLong_rangeType, which nothing reads"
    );
    assert!(
        !c_out.contains("BodyLong_factor"),
        "C output should not declare BodyLong_factor, which nothing reads"
    );

    // Should NOT contain settings that aren't referenced
    assert!(
        !c_out.contains("ShadowLong_rangeType"),
        "C output should not unpack unreferenced ShadowLong"
    );
}

#[test]
fn rust_backend_emits_candle_settings_from_core() {
    let (func, enums) = load_indicator("cdl2crows");
    let registry = make_registry();
    let helpers = make_helpers();
    let rust_out = backends::rust_lang::generate(&func, &enums, &registry, &helpers);

    // Assert Rust output contains unpacking lines
    assert!(
        rust_out.contains("self.candle_settings.body_long.range_type"),
        "Rust output should unpack body_long.range_type: {rust_out}"
    );
    assert!(
        rust_out.contains("self.candle_settings.body_long.avg_period"),
        "Rust output should unpack body_long.avg_period"
    );
    assert!(
        rust_out.contains("self.candle_settings.body_long.factor"),
        "Rust output should unpack body_long.factor"
    );
    assert!(
        rust_out.contains("#[allow(non_snake_case)]"),
        "Rust output should have non_snake_case allow attribute"
    );
}

#[test]
fn java_backend_emits_candle_settings() {
    let (func, enums) = load_indicator("cdl2crows");
    let registry = make_registry();
    let helpers = make_helpers();
    let java_out = backends::java::generate(&func, &enums, &registry, &helpers);

    // Assert Java output contains unpacking lines (canonical array/ordinal form)
    assert!(
        java_out.contains("this.candleSettings[CandleSettingType.BodyLong.ordinal()].rangeType"),
        "Java output should unpack BodyLong.rangeType: {java_out}"
    );
    assert!(
        java_out.contains("this.candleSettings[CandleSettingType.BodyLong.ordinal()].avgPeriod"),
        "Java output should unpack BodyLong.avgPeriod"
    );
    assert!(
        java_out.contains("this.candleSettings[CandleSettingType.BodyLong.ordinal()].factor"),
        "Java output should unpack BodyLong.factor"
    );
}

#[test]
fn candle_settings_unpacking_in_lookback() {
    // cdl2crows lookback references BodyLong_avgPeriod
    let (func, enums) = load_indicator("cdl2crows");
    let registry = make_registry();
    let helpers = make_helpers();

    let c_out = backends::c::generate(&func, &enums, &registry, &helpers);
    let rust_out = backends::rust_lang::generate(&func, &enums, &registry, &helpers);
    let java_out = backends::java::generate(&func, &enums, &registry, &helpers);

    // The lookback body references BodyLong_avgPeriod, so unpacking should appear
    // in the lookback function output
    let c_lookback_end = c_out.find("TA_LIB_API TA_RetCode TA_CDL2CROWS(").unwrap();
    let c_lookback = &c_out[..c_lookback_end];
    assert!(
        c_lookback.contains("TA_Globals->candleSettings[TA_BodyLong]"),
        "C lookback should contain candle settings unpacking"
    );

    let rust_lookback_end = rust_out.find("pub fn CDL2CROWS(").unwrap();
    let rust_lookback = &rust_out[..rust_lookback_end];
    assert!(
        rust_lookback.contains("self.candle_settings.body_long"),
        "Rust lookback should contain candle settings unpacking"
    );

    let java_lookback_end = java_out.find("RetCode CDL2CROWS_Impl(").unwrap();
    let java_lookback = &java_out[..java_lookback_end];
    assert!(
        java_lookback.contains("this.candleSettings[CandleSettingType.BodyLong.ordinal()]"),
        "Java lookback should contain candle settings unpacking"
    );
}

#[test]
fn candle_settings_multiple_settings_in_kicking() {
    // cdlkicking uses both BodyLong and ShadowVeryShort
    let (func, enums) = load_indicator("cdlkicking");
    let registry = make_registry();
    let helpers = make_helpers();

    let c_out = backends::c::generate(&func, &enums, &registry, &helpers);
    assert!(
        c_out.contains("TA_Globals->candleSettings[TA_BodyLong]"),
        "C output should unpack BodyLong"
    );
    assert!(
        c_out.contains("TA_Globals->candleSettings[TA_ShadowVeryShort]"),
        "C output should unpack ShadowVeryShort"
    );

    let rust_out = backends::rust_lang::generate(&func, &enums, &registry, &helpers);
    assert!(
        rust_out.contains("self.candle_settings.body_long"),
        "Rust output should unpack body_long"
    );
    assert!(
        rust_out.contains("self.candle_settings.shadow_very_short"),
        "Rust output should unpack shadow_very_short"
    );

    let java_out = backends::java::generate(&func, &enums, &registry, &helpers);
    assert!(
        java_out.contains("this.candleSettings[CandleSettingType.BodyLong.ordinal()]"),
        "Java output should unpack BodyLong"
    );
    assert!(
        java_out.contains("this.candleSettings[CandleSettingType.ShadowVeryShort.ordinal()]"),
        "Java output should unpack ShadowVeryShort"
    );
}

#[test]
fn non_candlestick_indicator_has_no_candle_unpacking() {
    let (func, enums) = load_indicator("sma");
    let registry = make_registry();
    let helpers = make_helpers();

    let c_out = backends::c::generate(&func, &enums, &registry, &helpers);
    assert!(
        !c_out.contains("candleSettings"),
        "SMA should not have candle settings unpacking"
    );

    let rust_out = backends::rust_lang::generate(&func, &enums, &registry, &helpers);
    assert!(
        !rust_out.contains("candle_settings"),
        "SMA should not have candle settings unpacking in Rust"
    );

    let java_out = backends::java::generate(&func, &enums, &registry, &helpers);
    assert!(
        !java_out.contains("candleSettings"),
        "SMA should not have candle settings unpacking in Java"
    );
}


#[test]
fn java_backend_hoisted_helper_declares_local_vars() {
    // Regression test: hoisted block helpers must declare their local variables.
    // ta_true_range has `double range = th - tl; double tmp = fabs(...);` which
    // become `double range_0 = ...;` and `double tmp_0 = ...;` after inlining.
    let func = make_func_with_helper_call(
        "ta_true_range",
        vec![
            ir::Expr::Var("high".to_string()),
            ir::Expr::Var("low".to_string()),
            ir::Expr::Var("prev".to_string()),
        ],
    );
    let enums = HashMap::new();
    let registry = make_registry();
    let helpers = make_helpers();

    let output = backends::java::generate(&func, &enums, &registry, &helpers);
    assert!(
        output.contains("double range_0"),
        "Should declare 'double range_0' for hoisted local: {output}"
    );
    assert!(
        output.contains("double tmp_0"),
        "Should declare 'double tmp_0' for hoisted local: {output}"
    );
}

// ---------------------------------------------------------------------------
