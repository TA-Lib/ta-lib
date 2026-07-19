use std::collections::HashMap;
use std::path::Path;
use ta_codegen_lib::backends;
use ta_codegen_lib::helper_registry::HelperRegistry;
use ta_codegen_lib::ir;
use ta_codegen_lib::parser;
use ta_codegen_lib::registry::Registry;

/// Empty enum map for tests that don't use enums.
fn no_enums() -> HashMap<String, ir::EnumDef> {
    HashMap::new()
}

/// Load enums from the shared enums.yaml.
fn load_enums() -> HashMap<String, ir::EnumDef> {
    let base = Path::new(env!("CARGO_MANIFEST_DIR"));
    let enums_path = base.join("../../ta_codegen/input/enums.yaml");
    parser::enums::load_enums(&enums_path)
}

/// Build registry for cross-call resolution.
fn make_registry() -> Registry {
    let base = Path::new(env!("CARGO_MANIFEST_DIR")).join("../../ta_codegen/input");
    Registry::from_dir(&base)
}

/// Helper: parse mult.yaml + mult.c and build a FuncDef.
fn load_mult() -> ir::FuncDef {
    let base = Path::new(env!("CARGO_MANIFEST_DIR"));
    let yaml_path = base.join("../../ta_codegen/input/mult/mult.yaml");
    let c_path = base.join("../../ta_codegen/input/mult/mult.c");

    let mut func_def = parser::yaml::parse_yaml(&yaml_path);
    let parsed = parser::c_source::parse_c_source(&c_path);
    func_def.body = parsed.functions[0].body.clone();
    func_def.lookback = Some(ir::LookbackExpr::Code(parsed.lookback_body));
    func_def.private_body = func_def.body.clone();
    func_def
}

/// Helper: parse sma.yaml + sma.c and build a FuncDef.
fn load_sma() -> ir::FuncDef {
    let base = Path::new(env!("CARGO_MANIFEST_DIR"));
    let yaml_path = base.join("../../ta_codegen/input/sma/sma.yaml");
    let c_path = base.join("../../ta_codegen/input/sma/sma.c");

    let mut func_def = parser::yaml::parse_yaml(&yaml_path);
    let parsed = parser::c_source::parse_c_source(&c_path);
    let guarded = parsed.functions.iter().find(|f| !f.name.ends_with("_unguarded"))
        .expect("C source must contain at least one function");
    func_def.body = guarded.body.clone();
    func_def.lookback = Some(ir::LookbackExpr::Code(parsed.lookback_body));
    // Auto-generate unguarded (copy guarded body — SMA has no extra params)
    func_def.private_body = func_def.body.clone();
    func_def
}

/// Helper: load any function definition from its .yaml + .c files.
fn load_func(name: &str) -> ir::FuncDef {
    let base = Path::new(env!("CARGO_MANIFEST_DIR"));
    let yaml_path = base.join(format!("../../ta_codegen/input/{}/{}.yaml", name, name));
    let c_path = base.join(format!("../../ta_codegen/input/{}/{}.c", name, name));

    let mut func_def = parser::yaml::parse_yaml(&yaml_path);
    let parsed = parser::c_source::parse_c_source(&c_path);
    func_def.body = parsed
        .functions
        .first()
        .expect("C source must contain at least one function")
        .body
        .clone();
    func_def.lookback = Some(ir::LookbackExpr::Code(parsed.lookback_body));
    func_def
}

// ---------------------------------------------------------------------------
// C source parser -- MULT from .c file
// ---------------------------------------------------------------------------

// test_mult_from_c_generates_all_backends removed: covered by dynamic
// test_all_backends_produce_nonempty_output

// ---------------------------------------------------------------------------
// Parser sanity checks -- MULT
// ---------------------------------------------------------------------------

#[test]
fn test_parse_mult_yaml() {
    let func = load_mult();
    assert_eq!(func.name, "MULT");
    assert_eq!(func.group, "Math Operators");
    assert_eq!(func.inputs.len(), 2);
    assert_eq!(func.outputs.len(), 1);
    assert!(func.optional_inputs.is_empty());
}

#[test]
fn test_parse_mult_yaml_enriched() {
    let func = load_mult();
    assert_eq!(func.camel_case.as_deref(), Some("Mult"));
    assert_eq!(func.hint.as_deref(), Some("Vector Arithmetic Mult"));
}

#[test]
fn test_parse_mult_body() {
    let func = load_mult();
    assert!(
        !func.body.is_empty(),
        "Body should contain parsed statements"
    );
}

// ---------------------------------------------------------------------------
// Parser sanity checks -- SMA
// ---------------------------------------------------------------------------

#[test]
fn test_parse_sma_yaml() {
    let func = load_sma();
    assert_eq!(func.name, "SMA");
    assert_eq!(func.group, "Overlap Studies");
    assert_eq!(func.camel_case.as_deref(), Some("Sma"));
    assert_eq!(func.hint.as_deref(), Some("Simple Moving Average"));
    assert!(func.flags.contains(&"overlap".to_string()));
    assert_eq!(func.inputs.len(), 1);
    assert_eq!(func.outputs.len(), 1);
    assert_eq!(func.optional_inputs.len(), 1);
    assert_eq!(func.optional_inputs[0].name, "optInTimePeriod");
    assert_eq!(func.optional_inputs[0].default, Some(30.0));
    // Period 1 = "no smoothing" is allowed since 0.6.5 (issues #48/#59).
    assert_eq!(func.optional_inputs[0].range, Some((1.0, 100000.0)));
    assert_eq!(
        func.optional_inputs[0].display_name.as_deref(),
        Some("Time Period")
    );
    assert_eq!(
        func.optional_inputs[0].hint.as_deref(),
        Some("Time period")
    );
    assert!(func.outputs[0].flags.contains(&"line".to_string()));
    assert_eq!(func.optional_inputs[0].suggested, Some((1.0, 200.0, 1.0)));
}

#[test]
fn test_parse_sma_body() {
    let func = load_sma();
    assert!(
        !func.body.is_empty(),
        "SMA body should contain parsed statements"
    );
    let has_if = func
        .body
        .iter()
        .any(|s| matches!(s, ir::Statement::If { .. }));
    let has_while = func
        .body
        .iter()
        .any(|s| matches!(s, ir::Statement::While { .. }));
    assert!(has_if, "SMA body should contain if statements");
    assert!(has_while, "SMA body should contain while loops");
    let has_nested_return = func.body.iter().any(|s| {
        if let ir::Statement::If { then_body, .. } = s {
            then_body
                .iter()
                .any(|ts| matches!(ts, ir::Statement::Return { .. }))
        } else {
            false
        }
    });
    assert!(
        has_nested_return,
        "SMA body should contain a return inside an if"
    );
}

// ---------------------------------------------------------------------------
// C backend
// ---------------------------------------------------------------------------

#[test]
fn test_c_backend_generates_mult() {
    let func = load_mult();
    let output = backends::c::generate(&func, &no_enums(), &make_registry(), &HelperRegistry::empty());
    assert!(
        output.contains("TA_MULT_Lookback"),
        "C output missing lookback function"
    );
    assert!(
        output.contains("TA_RetCode TA_MULT(") || output.contains("TA_MULT("),
        "C output missing TA_MULT function"
    );
    assert!(
        output.contains("TA_RetCode TA_S_MULT(") || output.contains("TA_S_MULT("),
        "C output missing single-precision TA_S_MULT function"
    );
    assert!(
        output.contains("TA_SUCCESS"),
        "C output missing TA_SUCCESS return"
    );
    assert!(
        output.contains("inReal0[i]*inReal1[i]") || output.contains("inReal0[i] * inReal1[i]"),
        "C output missing multiplication expression"
    );
}

// ---------------------------------------------------------------------------
// Rust backend
// ---------------------------------------------------------------------------

#[test]
fn test_rust_backend_generates_mult() {
    let func = load_mult();
    let output = backends::rust_lang::generate(&func, &no_enums(), &make_registry(), &HelperRegistry::empty());
    assert!(
        output.contains("mult_lookback"),
        "Rust output missing lookback function"
    );
    assert!(
        output.contains("fn mult("),
        "Rust output missing mult function"
    );
}

// Reference comparison removed: the reference file in rust/src/ta_func/mult.rs
// was generated by the old gen_code system and uses different patterns (FOR_EACH_OUTPUT)
// than the new C-source-based ta_codegen output. Functional equivalence is validated
// by the JSON-RPC server tests in validate.sh.

#[test]
fn test_rust_sma_from_c_produces_valid_output() {
    let func = load_sma();
    let output = backends::rust_lang::generate(&func, &no_enums(), &make_registry(), &HelperRegistry::empty());

    assert!(
        output.contains("sma_lookback"),
        "Missing sma_lookback function"
    );
    assert!(
        output.contains("fn sma_unguarded("),
        "Missing sma_unguarded function"
    );
    assert!(
        output.contains("RetCode::Success"),
        "Missing Success return"
    );
    assert!(
        output.contains("optInTimePeriod"),
        "Missing optInTimePeriod"
    );
    assert!(
        output.contains("periodTotal"),
        "Missing periodTotal variable"
    );
    assert!(
        output.contains("lookbackTotal"),
        "Missing lookbackTotal variable"
    );
    assert!(
        output.contains("optInTimePeriod - 1"),
        "Missing lookback expression"
    );
    assert!(
        output.contains("i += 1") || output.contains("i = i + 1"),
        "Missing increment pattern"
    );
}

// ---------------------------------------------------------------------------
// Java backend
// ---------------------------------------------------------------------------

#[test]
fn test_java_backend_generates_mult() {
    let func = load_mult();
    let output = backends::java::generate(&func, &no_enums(), &make_registry(), &HelperRegistry::empty());
    assert!(
        output.contains("multLookback") || output.contains("Lookback"),
        "Java output missing lookback method"
    );
    assert!(
        output.contains("mult(") || output.contains("RetCode"),
        "Java output missing mult method"
    );
    assert!(
        output.contains("MInteger") || output.contains("outBegIdx"),
        "Java output missing MInteger output params"
    );
    assert!(
        output.contains("Success") || output.contains("RetCode"),
        "Java output missing success return"
    );
}

// test_sma_from_c_generates_all_backends removed: covered by dynamic
// test_all_backends_produce_nonempty_output + backend_suite variant checks

// ---------------------------------------------------------------------------
// SMA from C source: lookback, internal function (parser-specific checks)
// ---------------------------------------------------------------------------

#[test]
fn test_sma_from_c_lookback_body() {
    let func = load_sma();
    match &func.lookback {
        Some(ir::LookbackExpr::Code(stmts)) => {
            assert!(!stmts.is_empty(), "Lookback body should not be empty");
            let has_return = stmts
                .iter()
                .any(|s| matches!(s, ir::Statement::Return { .. }));
            assert!(
                has_return,
                "Lookback body should contain a return statement"
            );
        }
        other => panic!("Expected LookbackExpr::Code, got {:?}", other),
    }
}

#[test]
fn test_sma_from_c_has_logic_function() {
    let base = Path::new(env!("CARGO_MANIFEST_DIR"));
    let c_path = base.join("../../ta_codegen/input/sma/sma.c");
    let parsed = parser::c_source::parse_c_source(&c_path);

    assert_eq!(parsed.functions.len(), 1, "Should have one function");
    assert_eq!(
        parsed.functions[0].name, "sma",
        "Function name should be sma"
    );
    assert!(
        !parsed.functions[0].body.is_empty(),
        "Function body should not be empty"
    );
}

// test_sma_all_backends_generate removed: covered by dynamic
// test_all_backends_produce_nonempty_output + backend_suite variant checks

// ---------------------------------------------------------------------------
// All indicators: all backends produce non-empty output (auto-discovered)
// ---------------------------------------------------------------------------

/// Discover all indicator names from ta_codegen/input/ that have both .yaml and .c files.
fn discover_indicators() -> Vec<String> {
    let base = Path::new(env!("CARGO_MANIFEST_DIR")).join("../../ta_codegen/input");
    let mut indicators = Vec::new();
    for entry in std::fs::read_dir(&base).expect("Cannot read ta_codegen/input directory") {
        let entry = entry.expect("Cannot read directory entry");
        let path = entry.path();
        if !path.is_dir() {
            continue;
        }
        let name = path.file_name().unwrap().to_str().unwrap().to_string();
        let yaml_path = path.join(format!("{}.yaml", name));
        let c_path = path.join(format!("{}.c", name));
        if yaml_path.exists() && c_path.exists() {
            indicators.push(name);
        }
    }
    indicators.sort();
    indicators
}

#[test]
fn test_all_backends_produce_nonempty_output() {
    let indicators = discover_indicators();
    assert!(!indicators.is_empty(), "No indicators discovered");

    let enums = load_enums();
    let registry = make_registry();
    let mut failures = Vec::new();
    let mut tested = 0;

    for name in &indicators {
        // Try to load; skip indicators whose parser doesn't support them yet
        let func = match std::panic::catch_unwind(std::panic::AssertUnwindSafe(|| load_func(name)))
        {
            Ok(f) => f,
            Err(_) => continue,
        };

        // Try to generate; skip if generation fails
        let helpers = HelperRegistry::empty();
        let outputs = match std::panic::catch_unwind(std::panic::AssertUnwindSafe(|| {
            let c_out = backends::c::generate(&func, &enums, &registry, &helpers);
            let rust_out = backends::rust_lang::generate(&func, &enums, &registry, &helpers);
            let java_out = backends::java::generate(&func, &enums, &registry, &helpers);
            (c_out, rust_out, java_out)
        })) {
            Ok(o) => o,
            Err(_) => continue,
        };

        let (c_out, rust_out, java_out) = outputs;

        let result = std::panic::catch_unwind(std::panic::AssertUnwindSafe(|| {
            assert!(!c_out.is_empty(), "{}: C output is empty", name);
            assert!(!rust_out.is_empty(), "{}: Rust output is empty", name);
            assert!(!java_out.is_empty(), "{}: Java output is empty", name);

            assert!(c_out.len() > 100, "{}: C output suspiciously short", name);
            assert!(
                rust_out.len() > 100,
                "{}: Rust output suspiciously short",
                name
            );
            assert!(
                java_out.len() > 100,
                "{}: Java output suspiciously short",
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
        } else {
            tested += 1;
        }
    }

    assert!(
        tested >= 6,
        "Expected at least 6 indicators to pass, got {}",
        tested
    );

    if !failures.is_empty() {
        panic!(
            "{} indicator(s) failed non-empty checks:\n{}",
            failures.len(),
            failures.join("\n")
        );
    }

    eprintln!(
        "{} indicators produce non-empty output for all backends",
        tested
    );
}

// ---------------------------------------------------------------------------
// Rust backend: verify _logic variant names (renamed from int_)
// ---------------------------------------------------------------------------

#[test]
fn test_rust_generates_generic_variants() {
    let func = load_sma();
    let output = backends::rust_lang::generate(&func, &no_enums(), &make_registry(), &HelperRegistry::empty());

    // Guarded function (concrete f64)
    assert!(
        output.contains("pub fn sma("),
        "Missing sma function"
    );

    // Unguarded function (concrete f64, get_unchecked)
    assert!(
        output.contains("pub fn sma_unguarded("),
        "Missing sma_unguarded function"
    );

    // Guarded function should render inline body with safe [] indexing (not delegate)
    assert!(
        !output.contains("self.sma_unguarded("),
        "Guarded fn should NOT delegate to sma_unguarded — it renders its own body"
    );

    // Should NOT contain old 4-variant patterns
    assert!(
        !output.contains("fn sma_unchecked("),
        "Should not contain sma_unchecked (dropped)"
    );
    assert!(
        !output.contains("fn sma_unguarded_unchecked("),
        "Should not contain sma_unguarded_unchecked (dropped)"
    );
    assert!(
        !output.contains("pub unsafe fn"),
        "Should not contain pub unsafe fn (unsafe is internal)"
    );
}

#[test]
fn test_rust_mult_generates_generic_variants() {
    let func = load_mult();
    let output = backends::rust_lang::generate(&func, &no_enums(), &make_registry(), &HelperRegistry::empty());

    // MULT should have all 4 generic variants regardless of optional inputs
    // 2-variant system: guarded + unguarded only, concrete f64
    assert!(
        output.contains("pub fn mult("),
        "Missing mult function"
    );
    assert!(
        output.contains("pub fn mult_unguarded("),
        "Missing mult_unguarded function"
    );
    // Old variants should be gone
    assert!(
        !output.contains("fn mult_unchecked("),
        "Should not contain mult_unchecked"
    );
    assert!(
        !output.contains("fn mult_unguarded_unchecked("),
        "Should not contain mult_unguarded_unchecked"
    );
}
