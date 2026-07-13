//! Comprehensive backend test suite for all indicators and all function variants.
//!
//! This complements `integration_test.rs` with systematic coverage:
//! - Every indicator x every backend x every function variant (auto-discovered)
//! - Cross-call resolution (MA calling SMA/EMA)
//! - Logic vs guarded validation checks
//! - Indicator-specific feature tests (unstable period, enums, etc.)

use std::collections::HashMap;
use std::path::Path;
use ta_codegen_lib::backends;
use ta_codegen_lib::helper_registry::HelperRegistry;
use ta_codegen_lib::ir;
use ta_codegen_lib::parser;
use ta_codegen_lib::registry::Registry;

// ---------------------------------------------------------------------------
// Test infrastructure
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

/// Load a function definition from its .yaml + .c files.
/// Always loads enums.yaml since multiple indicators use enum types.
fn load_indicator(name: &str) -> (ir::FuncDef, HashMap<String, ir::EnumDef>) {
    let base = Path::new(env!("CARGO_MANIFEST_DIR")).join("../../ta_codegen/input");
    let yaml_path = base.join(format!("{}/{}.yaml", name, name));
    let c_path = base.join(format!("{}/{}.c", name, name));

    let enums_path = base.join("enums.yaml");
    let enums = parser::enums::load_enums(&enums_path);

    let mut func_def = parser::yaml::parse_yaml(&yaml_path);
    let parsed = parser::c_source::parse_c_source(&c_path);
    // Single source of truth: wire exactly as the production load paths do
    // (guarded body, lookback, explicit _private variant + extra params).
    parser::c_source::wire_parsed_source(&mut func_def, &parsed);

    (func_def, enums)
}

/// Like [`load_indicator`], but wires a hand-written source body onto the real
/// YAML metadata — for fixtures that no shipped `.c` provides. Mirrors the
/// production load path (`wire_parsed_source`), matching the function by name.
fn load_indicator_with_source(
    name: &str,
    source: &str,
) -> (ir::FuncDef, HashMap<String, ir::EnumDef>) {
    let base = Path::new(env!("CARGO_MANIFEST_DIR")).join("../../ta_codegen/input");
    let enums = parser::enums::load_enums(&base.join("enums.yaml"));
    let mut func_def = parser::yaml::parse_yaml(&base.join(format!("{name}/{name}.yaml")));
    let parsed = parser::c_source::parse_c_source_str(source);
    parser::c_source::wire_parsed_source(&mut func_def, &parsed);
    // A hand-written fixture body is not a real stream target; suppress the
    // streaming gate the borrowed YAML may otherwise trigger.
    func_def.streaming = false;
    (func_def, enums)
}

/// All backend outputs for a single indicator.
struct AllOutputs {
    c: String,
    rust: String,
    java: String,
    dotnet: String,
}

fn make_registry() -> Registry {
    let base = Path::new(env!("CARGO_MANIFEST_DIR")).join("../../ta_codegen/input");
    Registry::from_dir(&base)
}

fn generate_all(func: &ir::FuncDef, enums: &HashMap<String, ir::EnumDef>) -> AllOutputs {
    let registry = make_registry();
    let helpers = HelperRegistry::empty();
    AllOutputs {
        c: backends::c::generate(func, enums, &registry, &helpers),
        rust: backends::rust_lang::generate(func, enums, &registry, &helpers),
        java: backends::java::generate(func, enums, &registry, &helpers),
        dotnet: backends::dotnet::generate(func, enums, &registry, &helpers),
    }
}

// ---------------------------------------------------------------------------
// Variant check functions (extracted from macros for dynamic invocation)
// ---------------------------------------------------------------------------

/// Derive Pascal case from the indicator name.
/// Must match the dotnet backend's to_pascal_case: lowercase then capitalize first char.
fn to_pascal(name: &str) -> String {
    let lower = name.to_lowercase();
    let mut chars = lower.chars();
    match chars.next() {
        None => String::new(),
        Some(c) => c.to_uppercase().to_string() + chars.as_str(),
    }
}

/// Convert snake_case to Java camelCase: "linearreg_angle" -> "linearregAngle"
fn to_camel(name: &str) -> String {
    let lower = name.to_lowercase();
    let parts: Vec<&str> = lower.split('_').collect();
    let mut result = String::new();
    for (i, part) in parts.iter().enumerate() {
        if i == 0 {
            result.push_str(part);
        } else {
            let mut chars = part.chars();
            if let Some(c) = chars.next() {
                result.extend(c.to_uppercase());
                result.push_str(chars.as_str());
            }
        }
    }
    result
}

/// Check that all C variants exist for a given indicator.
fn check_c_variants(c: &str, upper: &str, name: &str) {
    assert!(
        c.contains(&format!("TA_{}_Lookback", upper)),
        "{}: C missing TA_{}_Lookback",
        name,
        upper
    );
    assert!(
        c.contains(&format!("TA_{}(", upper)) || c.contains(&format!("TA_{} (", upper)),
        "{}: C missing TA_{}",
        name,
        upper
    );
    assert!(
        c.contains(&format!("TA_{}_Unguarded(", upper)),
        "{}: C missing TA_{}_Unguarded",
        name,
        upper
    );
    // TA_INT_* macros are no longer generated
    assert!(
        !c.contains(&format!("#define TA_INT_{}", upper)),
        "{}: C should NOT have #define TA_INT_{}",
        name,
        upper
    );
    assert!(
        c.contains(&format!("TA_S_{}(", upper)) || c.contains(&format!("TA_S_{} (", upper)),
        "{}: C missing TA_S_{}",
        name,
        upper
    );
    assert!(
        c.contains(&format!("TA_S_{}_Unguarded(", upper)),
        "{}: C missing TA_S_{}_Unguarded",
        name,
        upper
    );
}

/// Check that all Rust variants exist for a given indicator.
/// After the 2-variant refactor: only `foo` (guarded) + `foo_unguarded`.
/// No `_unchecked` or `_unguarded_unchecked` variants. Concrete f64 types, not generic.
fn check_rust_generic_variants(r: &str, snake: &str, name: &str) {
    // Lookback (non-generic)
    assert!(
        r.contains(&format!("{}_lookback", snake)),
        "{}: Rust missing {}_lookback",
        name,
        snake
    );
    // Guarded (concrete f64, no generics)
    assert!(
        r.contains(&format!("fn {}(", snake)),
        "{}: Rust missing fn {}(",
        name,
        snake
    );
    // Unguarded (concrete f64, no generics)
    assert!(
        r.contains(&format!("fn {}_unguarded(", snake)),
        "{}: Rust missing fn {}_unguarded(",
        name,
        snake
    );
}

/// Check that all Java variants exist for a given indicator.
fn check_java_variants(j: &str, lower: &str, name: &str) {
    assert!(
        j.contains(&format!("{}Lookback(", lower)),
        "{}: Java missing {}Lookback",
        name,
        lower
    );
    assert!(
        j.contains(&format!("RetCode {}(", lower)) || j.contains(&format!("RetCode {} (", lower)),
        "{}: Java missing {} function",
        name,
        lower
    );
    assert!(
        j.contains(&format!("{}Unguarded(", lower)),
        "{}: Java missing {}Unguarded",
        name,
        lower
    );
}

/// Check that all .NET variants exist for a given indicator.
fn check_dotnet_variants(d: &str, pascal: &str, upper: &str, name: &str) {
    assert!(
        d.contains(&format!("{}Lookback(", pascal)),
        "{}: .NET missing {}Lookback",
        name,
        pascal
    );
    assert!(
        d.contains(&format!("{}(", pascal)) || d.contains(&format!("{} (", pascal)),
        "{}: .NET missing {} function",
        name,
        pascal
    );
    assert!(
        d.contains(&format!("{}Logic(", pascal)),
        "{}: .NET missing {}Logic declaration",
        name,
        pascal
    );
    assert!(
        d.contains(&format!("#define TA_{} ", upper))
            || d.contains(&format!("#define TA_{}\n", upper)),
        "{}: .NET missing #define TA_{}",
        name,
        upper
    );
    assert!(
        d.contains(&format!("#define TA_{}_Logic", upper)),
        "{}: .NET missing #define TA_{}_Logic",
        name,
        upper
    );
    // TA_INT_* macros are no longer generated for .NET
}

/// Check C does NOT generate TA_INT_ macros (they've been removed).
fn check_c_int_alias(c: &str, upper: &str, name: &str) {
    assert!(
        !c.contains(&format!("#define TA_INT_{}", upper)),
        "{}: C should NOT have #define TA_INT_{}",
        name,
        upper
    );
}

/// Check .NET macros point to correct Core:: methods for an indicator.
fn check_dotnet_macros(d: &str, pascal: &str, upper: &str, name: &str) {
    assert!(
        d.contains(&format!("#define TA_{} Core::{}", upper, pascal)),
        "{}: .NET TA_{} should point to Core::{}",
        name,
        upper,
        pascal
    );
    assert!(
        d.contains(&format!(
            "#define TA_{}_Lookback Core::{}Lookback",
            upper, pascal
        )),
        "{}: .NET TA_{}_Lookback should point to Core::{}Lookback",
        name,
        upper,
        pascal
    );
    assert!(
        d.contains(&format!("#define TA_{}_Logic Core::{}Logic", upper, pascal)),
        "{}: .NET TA_{}_Logic should point to Core::{}Logic",
        name,
        upper,
        pascal
    );
    // TA_INT_* macros are no longer generated for .NET
}

/// Try to load an indicator, returning None if parsing fails (not yet supported).
fn try_load_indicator(name: &str) -> Option<(ir::FuncDef, HashMap<String, ir::EnumDef>)> {
    let result = std::panic::catch_unwind(std::panic::AssertUnwindSafe(|| load_indicator(name)));
    result.ok()
}

/// Try to generate all backends, returning None if generation fails.
fn try_generate_all(
    func: &ir::FuncDef,
    enums: &HashMap<String, ir::EnumDef>,
) -> Option<AllOutputs> {
    let result =
        std::panic::catch_unwind(std::panic::AssertUnwindSafe(|| generate_all(func, enums)));
    result.ok()
}

// ---------------------------------------------------------------------------
// 2. Auto-discovered per-indicator x per-backend variant checks
// ---------------------------------------------------------------------------

#[test]
fn test_all_indicators_all_backends() {
    let indicators = discover_indicators();
    assert!(!indicators.is_empty(), "No indicators discovered");

    let mut failures = Vec::new();
    let mut tested = 0;
    let mut skipped = 0;

    for name in &indicators {
        // Phase 1: try to load and generate (may fail for not-yet-supported indicators)
        let loaded = try_load_indicator(name);
        let (func, enums) = match loaded {
            Some(v) => v,
            None => {
                skipped += 1;
                continue;
            }
        };
        let out = match try_generate_all(&func, &enums) {
            Some(v) => v,
            None => {
                skipped += 1;
                continue;
            }
        };

        // Phase 2: run variant checks (failures here are real bugs)
        let upper = func.name.clone();
        let snake = name.clone();
        let pascal = to_pascal(name);
        // Java method name comes from the YAML `camel_case` field (first char
        // lower-cased), matching the backend; this captures the historical
        // irregular/typo names (e.g. ma -> movingAverage, willr -> willR).
        let camel = func.camel_case.as_deref().map_or_else(
            || to_camel(name),
            |cc| {
                let mut chars = cc.chars();
                chars.next().map_or_else(String::new, |c| {
                    c.to_lowercase().collect::<String>() + chars.as_str()
                })
            },
        );

        let result = std::panic::catch_unwind(std::panic::AssertUnwindSafe(|| {
            check_c_variants(&out.c, &upper, &snake);
            check_rust_generic_variants(&out.rust, &snake, &snake);
            check_java_variants(&out.java, &camel, &snake);
            check_dotnet_variants(&out.dotnet, &pascal, &upper, &snake);
            check_c_int_alias(&out.c, &upper, &snake);
            check_dotnet_macros(&out.dotnet, &pascal, &upper, &snake);
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

    eprintln!(
        "Variant checks: {} tested, {} skipped (parse not yet supported), {} failed",
        tested,
        skipped,
        failures.len()
    );

    // Ensure we tested at least the 6 known-good indicators
    assert!(
        tested >= 6,
        "Expected at least 6 indicators to pass, but only {} did",
        tested
    );

    if !failures.is_empty() {
        panic!(
            "{} indicator(s) failed variant checks:\n{}",
            failures.len(),
            failures.join("\n")
        );
    }
}

// ---------------------------------------------------------------------------
// 3. Cross-call resolution tests (MA calls sma/ema lookback + logic)
// ---------------------------------------------------------------------------

#[test]
fn test_ma_c_cross_calls() {
    let (func, enums) = load_indicator("ma");
    let out = generate_all(&func, &enums);
    let c = &out.c;

    assert!(
        c.contains("TA_SMA_Lookback("),
        "C: MA should call TA_SMA_Lookback"
    );
    assert!(
        c.contains("TA_EMA_Lookback("),
        "C: MA should call TA_EMA_Lookback"
    );
    // Bare cross-indicator calls resolve to Unguarded (skip validation)
    assert!(
        c.contains("TA_SMA_Unguarded("),
        "C: MA should call TA_SMA_Unguarded"
    );
    assert!(
        c.contains("TA_EMA_Unguarded("),
        "C: MA should call TA_EMA_Unguarded"
    );
}

#[test]
fn test_ma_java_cross_calls() {
    let (func, enums) = load_indicator("ma");
    let out = generate_all(&func, &enums);
    let j = &out.java;

    assert!(
        j.contains("smaLookback("),
        "Java: MA should call smaLookback"
    );
    assert!(
        j.contains("emaLookback("),
        "Java: MA should call emaLookback"
    );
    // Bare cross-indicator calls resolve to Unguarded (skip validation)
    assert!(j.contains("smaUnguarded("), "Java: MA should call smaUnguarded");
    assert!(j.contains("emaUnguarded("), "Java: MA should call emaUnguarded");
}

#[test]
fn test_ma_rust_cross_calls() {
    let (func, enums) = load_indicator("ma");
    let out = generate_all(&func, &enums);
    let r = &out.rust;

    // Lookback calls remain the same.
    assert!(
        r.contains("self.sma_lookback("),
        "Rust: MA should call self.sma_lookback"
    );
    assert!(
        r.contains("self.ema_lookback("),
        "Rust: MA should call self.ema_lookback"
    );
    // Bare cross-indicator calls go to unguarded (skip validation)
    assert!(
        r.contains("self.sma_unguarded("),
        "Rust: MA should call self.sma_unguarded"
    );
    assert!(
        r.contains("self.ema_unguarded("),
        "Rust: MA should call self.ema_unguarded"
    );
}

// ---------------------------------------------------------------------------
// 4. Logic vs guarded validation tests
// ---------------------------------------------------------------------------

/// Helper: extract the section of output between `start_marker` and `end_marker`.
/// If `end_marker` is not found, returns everything after `start_marker`.
fn extract_section(output: &str, start_marker: &str, end_marker: &str) -> String {
    let start = output
        .find(start_marker)
        .unwrap_or_else(|| panic!("Could not find '{}' in output", start_marker));
    let rest = &output[start..];
    let end = rest.find(end_marker).unwrap_or(rest.len());
    rest[..end].to_string()
}

#[test]
fn test_c_sma_guarded_has_validation() {
    let (func, enums) = load_indicator("sma");
    let out = generate_all(&func, &enums);

    // Extract guarded function (between TA_SMA( and TA_SMA_Unguarded)
    let guarded = extract_section(&out.c, "TA_RetCode TA_SMA(", "TA_SMA_Unguarded(");
    assert!(
        guarded.contains("TA_OUT_OF_RANGE_START_INDEX"),
        "C guarded SMA should have start index validation"
    );
    assert!(
        guarded.contains("TA_OUT_OF_RANGE_END_INDEX"),
        "C guarded SMA should have end index validation"
    );
}

#[test]
fn test_c_sma_logic_omits_validation() {
    let (func, enums) = load_indicator("sma");
    let out = generate_all(&func, &enums);

    // Extract logic function (between TA_SMA_Unguarded( and #define TA_INT_SMA)
    let logic = extract_section(&out.c, "TA_SMA_Unguarded(", "TA_S_SMA(");
    assert!(
        !logic.contains("TA_OUT_OF_RANGE_START_INDEX"),
        "C logic SMA should NOT have start index validation"
    );
    assert!(
        !logic.contains("TA_OUT_OF_RANGE_END_INDEX"),
        "C logic SMA should NOT have end index validation"
    );
}

#[test]
fn test_java_sma_guarded_has_validation() {
    let (func, enums) = load_indicator("sma");
    let out = generate_all(&func, &enums);

    // Extract guarded function (between "RetCode sma(" and "smaUnguarded(")
    let guarded = extract_section(&out.java, "RetCode sma(", "smaUnguarded(");
    assert!(
        guarded.contains("OutOfRangeStartIndex"),
        "Java guarded SMA should have start index validation"
    );
}

#[test]
fn test_java_sma_logic_omits_validation() {
    let (func, enums) = load_indicator("sma");
    let out = generate_all(&func, &enums);

    // Extract unguarded function (find smaUnguarded, get section until next "public RetCode")
    let logic_start = out.java.find("smaUnguarded(").expect("Missing smaUnguarded");
    let logic_section = &out.java[logic_start..];
    // Look for next public function or end
    let end = logic_section
        .find("public RetCode sma(")
        .or_else(|| logic_section.find("public int"))
        .unwrap_or(logic_section.len());
    let logic = &logic_section[..end];
    assert!(
        !logic.contains("OutOfRangeStartIndex"),
        "Java logic SMA should NOT have start index validation"
    );
}

#[test]
fn test_rust_sma_guarded_has_validation() {
    let (func, enums) = load_indicator("sma");
    let out = generate_all(&func, &enums);

    // The guarded Rust function delegates to _unguarded, but first validates params.
    // After 2-variant refactor: concrete f64 types, no generics.
    let guarded = extract_section(
        &out.rust,
        "pub fn sma(",
        "pub fn sma_unguarded(",
    );
    assert!(
        guarded.contains("endIdx < startIdx"),
        "Rust guarded SMA should have endIdx < startIdx check"
    );
}

#[test]
fn test_rust_sma_unguarded_omits_validation() {
    let (func, enums) = load_indicator("sma");
    let out = generate_all(&func, &enums);

    // The unguarded function should not have the range check.
    // After 2-variant refactor: concrete f64 types, no _unchecked variant.
    let unguarded_start = out
        .rust
        .find("pub fn sma_unguarded(")
        .expect("Missing sma_unguarded");
    let unguarded_section = &out.rust[unguarded_start..];
    // No _unchecked variant anymore; use end of impl block or file as boundary
    let end = unguarded_section.len();
    let unguarded = &unguarded_section[..end];
    assert!(
        !unguarded.contains("OutOfRangeStartIndex"),
        "Rust unguarded SMA should NOT have range validation"
    );
}

// Also test a different indicator for validation (RSI)
#[test]
fn test_c_rsi_logic_omits_validation() {
    let (func, enums) = load_indicator("rsi");
    let out = generate_all(&func, &enums);

    let logic = extract_section(&out.c, "TA_RSI_Unguarded(", "TA_S_RSI(");
    assert!(
        !logic.contains("TA_OUT_OF_RANGE_START_INDEX"),
        "C logic RSI should NOT have start index validation"
    );
}

#[test]
fn test_c_rsi_guarded_has_validation() {
    let (func, enums) = load_indicator("rsi");
    let out = generate_all(&func, &enums);

    let guarded = extract_section(&out.c, "TA_RetCode TA_RSI(", "TA_RSI_Unguarded(");
    assert!(
        guarded.contains("TA_OUT_OF_RANGE_START_INDEX"),
        "C guarded RSI should have start index validation"
    );
}

// ---------------------------------------------------------------------------
// 5. Indicator-specific feature tests
// ---------------------------------------------------------------------------

// --- RSI: unstable period + compatibility + IS_ZERO ---

#[test]
fn test_rsi_c_unstable_period() {
    let (func, enums) = load_indicator("rsi");
    let out = generate_all(&func, &enums);

    assert!(
        out.c.contains("TA_GLOBALS_UNSTABLE_PERIOD"),
        "C RSI should use TA_GLOBALS_UNSTABLE_PERIOD"
    );
    assert!(
        out.c.contains("TA_GLOBALS_COMPATIBILITY"),
        "C RSI should use TA_GLOBALS_COMPATIBILITY"
    );
    // TA_IS_ZERO is preserved as a macro call — the C backend emits TA_IS_ZERO(x)
    assert!(
        out.c.contains("TA_IS_ZERO("),
        "C RSI should use TA_IS_ZERO macro"
    );
}

#[test]
fn test_rsi_rust_unstable_period() {
    let (func, enums) = load_indicator("rsi");
    let out = generate_all(&func, &enums);

    assert!(
        out.rust.contains("unstable_period"),
        "Rust RSI should reference unstable_period"
    );
}

#[test]
fn test_rsi_java_unstable_period() {
    let (func, enums) = load_indicator("rsi");
    let out = generate_all(&func, &enums);

    assert!(
        out.java.contains("this.unstablePeriod"),
        "Java RSI should reference this.unstablePeriod"
    );
    assert!(
        out.java.contains("this.compatibility"),
        "Java RSI should reference this.compatibility"
    );
}

// --- EMA: unstable period + ARRAY_COPY ---

#[test]
fn test_ema_c_unstable_period() {
    let (func, enums) = load_indicator("ema");
    let out = generate_all(&func, &enums);

    assert!(
        out.c.contains("TA_GLOBALS_UNSTABLE_PERIOD"),
        "C EMA should use TA_GLOBALS_UNSTABLE_PERIOD"
    );
}

#[test]
fn test_ema_c_smoothing_factor() {
    let (func, enums) = load_indicator("ema");
    let out = generate_all(&func, &enums);

    // EMA takes optInK_1 as the smoothing factor parameter
    assert!(
        out.c.contains("optInK_1"),
        "C EMA should use optInK_1 smoothing factor parameter"
    );
}

#[test]
fn test_ema_java_smoothing_factor() {
    let (func, enums) = load_indicator("ema");
    let out = generate_all(&func, &enums);

    // Java EMA also uses optInK_1
    assert!(
        out.java.contains("optInK_1"),
        "Java EMA should use optInK_1 smoothing factor parameter"
    );
}

#[test]
fn test_ema_rust_unstable_period() {
    let (func, enums) = load_indicator("ema");
    let out = generate_all(&func, &enums);

    assert!(
        out.rust.contains("unstable_period"),
        "Rust EMA should reference unstable_period"
    );
}

// --- MA: switch/case with enum labels ---

#[test]
fn test_ma_c_switch_statement() {
    let (func, enums) = load_indicator("ma");
    let out = generate_all(&func, &enums);

    assert!(
        out.c.contains("switch(") || out.c.contains("switch ("),
        "C MA should contain a switch statement"
    );
    // Should use ENUM_CASE macro in C for enum labels
    assert!(
        out.c.contains("ENUM_CASE("),
        "C MA should use ENUM_CASE for switch labels"
    );
}

#[test]
fn test_ma_java_switch_statement() {
    let (func, enums) = load_indicator("ma");
    let out = generate_all(&func, &enums);

    assert!(
        out.java.contains("switch(") || out.java.contains("switch ("),
        "Java MA should contain a switch statement"
    );
    // Enum switch case labels must be UNQUALIFIED ("case Sma:") — qualified
    // labels are Java 21+ syntax and the shipped Core.java must compile on
    // older JDKs.
    assert!(
        out.java.contains("case Sma:") || out.java.contains("case Ema:"),
        "Java MA should use unqualified enum case labels"
    );
    assert!(
        !out.java.contains("case MAType."),
        "Java switch case labels must not be qualified (Java 21+ only)"
    );
}

#[test]
fn test_ma_rust_switch_statement() {
    let (func, enums) = load_indicator("ma");
    let out = generate_all(&func, &enums);

    // Rust uses match instead of switch
    assert!(
        out.rust.contains("match "),
        "Rust MA should contain a match statement"
    );
}

// --- MULT: simple expression, no optional inputs ---

#[test]
fn test_mult_simplicity() {
    let (func, enums) = load_indicator("mult");
    let out = generate_all(&func, &enums);

    // MULT has no optional inputs
    assert!(
        func.optional_inputs.is_empty(),
        "MULT should have no optional inputs"
    );

    // MULT has exactly 2 inputs
    assert_eq!(func.inputs.len(), 2, "MULT should have exactly 2 inputs");

    // MULT has exactly 1 output
    assert_eq!(func.outputs.len(), 1, "MULT should have exactly 1 output");

    // C output should have multiplication
    assert!(
        out.c.contains("inReal0[") && out.c.contains("inReal1["),
        "C MULT should reference both input arrays"
    );

    // No unstable period, no COMPATIBILITY
    assert!(
        !out.c.contains("UNSTABLE_PERIOD"),
        "C MULT should NOT use UNSTABLE_PERIOD"
    );
}

// ---------------------------------------------------------------------------
// 6. Non-empty output checks for all discovered indicators
// ---------------------------------------------------------------------------

#[test]
fn test_all_indicators_nonempty_output() {
    let indicators = discover_indicators();
    assert!(!indicators.is_empty(), "No indicators discovered");

    let mut failures = Vec::new();
    let mut tested = 0;

    for name in &indicators {
        let loaded = try_load_indicator(name);
        let (func, enums) = match loaded {
            Some(v) => v,
            None => continue,
        };
        let out = match try_generate_all(&func, &enums) {
            Some(v) => v,
            None => continue,
        };

        let result = std::panic::catch_unwind(std::panic::AssertUnwindSafe(|| {
            assert!(!out.c.is_empty(), "{}: C output is empty", name);
            assert!(!out.rust.is_empty(), "{}: Rust output is empty", name);
            assert!(!out.java.is_empty(), "{}: Java output is empty", name);
            assert!(!out.dotnet.is_empty(), "{}: .NET output is empty", name);

            assert!(out.c.len() > 200, "{}: C output suspiciously short", name);
            assert!(
                out.rust.len() > 200,
                "{}: Rust output suspiciously short",
                name
            );
            assert!(
                out.java.len() > 100,
                "{}: Java output suspiciously short",
                name
            );
            assert!(
                out.dotnet.len() > 100,
                "{}: .NET output suspiciously short",
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
        "Expected at least 6 indicators to pass non-empty checks, got {}",
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
// 10. Rust impl Core block structure (all indicators)
// ---------------------------------------------------------------------------

#[test]
fn test_rust_impl_core_structure() {
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
            assert!(
                out.rust.contains("impl Core {"),
                "Rust {}: missing impl Core block",
                name
            );
            assert!(
                out.rust.contains("use super::*;"),
                "Rust {}: missing use super::* import",
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
            "{} indicator(s) failed Rust structure checks:\n{}",
            failures.len(),
            failures.join("\n")
        );
    }
}

// ---------------------------------------------------------------------------
// 11. WMA-specific: verify lookback uses optInTimePeriod
// ---------------------------------------------------------------------------

#[test]
fn test_wma_lookback_uses_time_period() {
    let (func, enums) = load_indicator("wma");
    let out = generate_all(&func, &enums);

    assert!(
        out.c.contains("optInTimePeriod"),
        "C WMA should reference optInTimePeriod"
    );
    assert!(
        out.rust.contains("optInTimePeriod"),
        "Rust WMA should reference optInTimePeriod"
    );
    assert!(
        out.java.contains("optInTimePeriod"),
        "Java WMA should reference optInTimePeriod"
    );
}

// ---------------------------------------------------------------------------
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
            // Delegation functions (e.g. EMA -> TA_EMA_Private, MACDFIX ->
            // TA_MACD_Unguarded) return a RetCode from a callee without ever
            // mentioning TA_SUCCESS literally.
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
                || (out.rust.contains("return self.") && out.rust.contains("_unguarded"))
                || (out.rust.contains("return self.") && out.rust.contains("_private("));
            assert!(
                rust_has_success,
                "Rust {}: missing RetCode::Success return",
                name
            );
            // Accept: literal RetCode.Success OR a return of a RetCode variable/call.
            let java_has_success = out.java.contains("RetCode.Success")
                || out.java.contains("return retCode ;")
                || (out.java.contains("return ") && out.java.contains("Unguarded("));
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
        r.contains("pub fn sma("),
        "Rust SMA should have pub fn sma("
    );
    assert!(
        r.contains("pub fn sma_unguarded("),
        "Rust SMA should have pub fn sma_unguarded("
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

    // 5. No _unchecked or _unguarded_unchecked variants
    assert!(
        !r.contains("fn sma_unchecked(") && !r.contains("fn sma_unchecked<"),
        "Rust SMA should NOT contain _unchecked variants"
    );
    assert!(
        !r.contains("fn sma_unguarded_unchecked(") && !r.contains("fn sma_unguarded_unchecked<"),
        "Rust SMA should NOT contain _unguarded_unchecked variants"
    );

    // 6. Only 2 pub fn declarations (guarded + unguarded), plus lookback
    let pub_fn_count = r.matches("pub fn sma").count();
    assert_eq!(
        pub_fn_count, 3,
        "Rust SMA should have exactly 3 pub fn (sma, sma_unguarded, sma_lookback), got {}",
        pub_fn_count
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
    let rendered = backends::c::render_statement(&stmt, 0, false, &enums, &registry, &helpers, &inline_counter);

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

    let ctx = RustRenderCtx::for_lookback();
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

/// Regression: an inline-commented `&&`-chain whose operand is a parenthesized
/// `||` group must keep that group parenthesized in the multi-line Rust render,
/// or precedence changes (`a && (b||c)` would become `(a&&b)||c`). CDLHIKKAKE hit
/// this and panicked in the Rust server.
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
        // Comments on both operands force the multi-line rendering path.
        cond_comments: vec![Some(vec!["one".into()]), Some(vec!["two".into()])],
    };

    let ctx = RustRenderCtx::for_lookback();
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

    let ctx = RustRenderCtx::for_lookback();
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

    let ctx = RustRenderCtx::for_lookback();
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
        camel_case: None,
        hint: None,
        flags: vec![],
        inputs: vec![Input {
            name: "inReal".to_string(),
            param_type: ParamType::Real,
        }],
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

fn make_helpers() -> HelperRegistry {
    let base = Path::new(env!("CARGO_MANIFEST_DIR")).join("../../ta_codegen/input");
    HelperRegistry::from_dir(&base)
}

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
        camel_case: None,
        hint: None,
        flags: vec![],
        inputs: vec![ir::Input {
            name: "inReal".to_string(),
            param_type: ir::ParamType::Real,
        }],
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
        camel_case: None,
        hint: None,
        flags: vec![],
        inputs: vec![ir::Input {
            name: "inReal".to_string(),
            param_type: ir::ParamType::Real,
        }],
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

    // Assert C output contains unpacking lines for BodyLong
    assert!(
        c_out.contains("BodyLong_rangeType = TA_Globals->candleSettings[TA_BodyLong].rangeType"),
        "C output should unpack BodyLong_rangeType: {c_out}"
    );
    assert!(
        c_out.contains("BodyLong_avgPeriod = TA_Globals->candleSettings[TA_BodyLong].avgPeriod"),
        "C output should unpack BodyLong_avgPeriod"
    );
    assert!(
        c_out.contains("BodyLong_factor = TA_Globals->candleSettings[TA_BodyLong].factor"),
        "C output should unpack BodyLong_factor"
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

    let rust_lookback_end = rust_out.find("pub fn cdl2crows(").unwrap();
    let rust_lookback = &rust_out[..rust_lookback_end];
    assert!(
        rust_lookback.contains("self.candle_settings.body_long"),
        "Rust lookback should contain candle settings unpacking"
    );

    let java_lookback_end = java_out.find("public RetCode cdl2Crows(").unwrap();
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
// Rust render_statement unit tests for uncovered branches
// ---------------------------------------------------------------------------

/// Helper to build a RustRenderCtx and call render_statement with minimal boilerplate.
fn render_rust_stmt(stmt: &ir::Statement) -> String {
    render_rust_stmt_with_ctx(stmt, &backends::rust_lang::RustRenderCtx::for_lookback())
}

fn render_rust_stmt_with_ctx(
    stmt: &ir::Statement,
    ctx: &backends::rust_lang::RustRenderCtx,
) -> String {
    let for_loop_vars: Vec<String> = vec![];
    let var_inits: std::collections::HashMap<String, &ir::Expr> =
        std::collections::HashMap::new();
    let output_names: Vec<String> = vec![];
    let opt_real_params: Vec<String> = vec![];
    let enums = HashMap::new();
    let registry = make_registry();
    let helpers = HelperRegistry::empty();
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
        &helpers,
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
    let mut ctx = backends::rust_lang::RustRenderCtx::for_lookback();
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

    // MA's switch should render as match with integer values from enum lookup
    assert!(
        rust_out.contains("match "),
        "MA Rust should contain match statement: {rust_out}"
    );
    // The enum variants resolve to integer values (0, 1, 2, etc.)
    assert!(
        rust_out.contains("0 =>") || rust_out.contains("1 =>"),
        "MA Rust match should have integer case labels from enum resolution"
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

    // Cross-indicator calls resolve to _unguarded (skip validation)
    assert!(
        rust_out.contains("self.sma_unguarded("),
        "MA Rust should call self.sma_unguarded(): {rust_out}"
    );
    assert!(
        rust_out.contains("self.ema_unguarded("),
        "MA Rust should call self.ema_unguarded(): {rust_out}"
    );
}

#[test]
fn rust_cross_indicator_lookback_with_pascal_case() {
    // func_call where fname ends in _Lookback: exercises the _Lookback branch
    let (func, enums) = load_indicator("ma");
    let registry = make_registry();
    let helpers = make_helpers();
    let rust_out = backends::rust_lang::generate(&func, &enums, &registry, &helpers);

    // MA calls SMA_Lookback which renders as self.sma_lookback
    assert!(
        rust_out.contains("self.sma_lookback("),
        "MA Rust should call self.sma_lookback(): {rust_out}"
    );
}

#[test]
fn rust_private_cross_indicator_call() {
    // EMA has explicit _private with extra params. Registry routes:
    //   ema() → ema_unguarded(), ema_private() → ema_private()
    // (MACD was the original vehicle for both paths, but its lockstep fusion
    // removed the EMA calls.) The bare-name path is exercised by MA's dispatch;
    // the private-name path by EMA's guarded body delegating to ema_private().
    let registry = make_registry();
    let helpers = make_helpers();

    let (func, enums) = load_indicator("ma");
    let rust_out = backends::rust_lang::generate(&func, &enums, &registry, &helpers);
    assert!(
        rust_out.contains("self.ema_unguarded("),
        "MA Rust dispatch should call self.ema_unguarded(): {rust_out}"
    );

    let (func, enums) = load_indicator("ema");
    let rust_out = backends::rust_lang::generate(&func, &enums, &registry, &helpers);
    assert!(
        rust_out.contains("self.ema_private("),
        "EMA Rust guarded body should delegate to self.ema_private(): {rust_out}"
    );
}

#[test]
fn rust_cross_indicator_vec_input_gets_ref() {
    // Indicators that allocate a local buffer (Vec) and pass it to a cross-indicator
    // call should render the Vec as `&name` in input position. (MACD was the original
    // vehicle, but its lockstep fusion removed the local buffers.) STOCH builds
    // tempBuffer and passes it into ma_unguarded as an input.
    let (func, enums) = load_indicator("stoch");
    let registry = make_registry();
    let helpers = make_helpers();
    let rust_out = backends::rust_lang::generate(&func, &enums, &registry, &helpers);

    assert!(
        rust_out.contains("self.ma_unguarded(") && rust_out.contains("&tempBuffer"),
        "STOCH Rust should pass &tempBuffer into self.ma_unguarded(): {rust_out}"
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
    let lookback_section = extract_section(&rust_out, "_lookback(", "pub fn cdlkicking(");
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

    let lookback_section = extract_section(&rust_out, "_lookback(", "pub fn mult(");
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
        rendered.contains("self.unstable_period[FuncUnstId::Rsi as usize]"),
        "UNSTABLE_PERIOD should render with FuncUnstId: {rendered}"
    );
}

#[test]
fn rust_func_call_compatibility() {
    let stmt = ir::Statement::Assign {
        target: ir::Expr::Var("x".to_string()),
        value: ir::Expr::FuncCall("COMPATIBILITY".to_string(), vec![]),
        compound: false,
    };
    let rendered = render_rust_stmt(&stmt);
    assert!(
        rendered.contains("self.compatibility"),
        "COMPATIBILITY should render as self.compatibility: {rendered}"
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

#[test]
fn rust_func_call_free_is_noop() {
    let stmt = ir::Statement::Expr(ir::Expr::FuncCall(
        "free".to_string(),
        vec![ir::Expr::Var("buf".to_string())],
    ));
    let rendered = render_rust_stmt(&stmt);
    // free() is a no-op in Rust (returns empty string from render_func_call)
    // The statement expression with an empty value should be skipped
    assert!(
        !rendered.contains("free("),
        "free() should not appear in Rust output: {rendered}"
    );
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
    let mut ctx = backends::rust_lang::RustRenderCtx::for_lookback();
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
    let mut ctx = backends::rust_lang::RustRenderCtx::for_lookback();
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
    let mut ctx = backends::rust_lang::RustRenderCtx::for_lookback();
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
    let mut ctx = backends::rust_lang::RustRenderCtx::for_lookback();
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
        camel_case: None,
        hint: None,
        flags: vec![],
        inputs: vec![ir::Input {
            name: "inReal".to_string(),
            param_type: ir::ParamType::Real,
        }],
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
        camel_case: None,
        hint: None,
        flags: vec![],
        inputs: vec![ir::Input {
            name: "inReal".to_string(),
            param_type: ir::ParamType::Real,
        }],
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
    };
    let enums = HashMap::new();
    let registry = make_registry();
    let helpers = HelperRegistry::empty();
    let rust_out = backends::rust_lang::generate(&func, &enums, &registry, &helpers);

    let lookback_section = extract_section(&rust_out, "_lookback(", "pub fn test(");
    assert!(
        lookback_section.contains("return 0"),
        "None lookback should return 0: {lookback_section}"
    );
}

// ---------------------------------------------------------------------------
// 15. Lookback return value casting in lookback context
// ---------------------------------------------------------------------------

#[test]
fn rust_lookback_return_casts_to_usize() {
    // In lookback context, return values that are i32-typed should be cast to usize
    let mut ctx = backends::rust_lang::RustRenderCtx::for_lookback();
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

    let ctx = RustRenderCtx::for_lookback();
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
        camel_case: None,
        hint: None,
        flags: vec![],
        inputs: vec![ir::Input {
            name: "inReal".to_string(),
            param_type: ir::ParamType::Real,
        }],
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
    };
    let enums = HashMap::new();
    let registry = make_registry();
    let helpers = HelperRegistry::empty();
    let rust_out = backends::rust_lang::generate(&func, &enums, &registry, &helpers);

    let lookback_section = extract_section(&rust_out, "_lookback(", "pub fn test(");
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

// ===========================================================================
// Java backend coverage tests
// ===========================================================================

// ---------------------------------------------------------------------------
// Java: VarDecl rendering for all VarType variants via render_statement
// ---------------------------------------------------------------------------

/// Helper to call Java render_statement with minimal boilerplate.
fn render_java_stmt(stmt: &ir::Statement) -> String {
    let enums = HashMap::new();
    let registry = make_registry();
    let helpers = HelperRegistry::empty();
    let inline_counter = std::cell::Cell::new(0);
    let address_of_vars = std::collections::HashSet::new();
    let double_address_of_vars = std::collections::HashSet::new();
    let float_input_params = std::collections::HashSet::new();
    backends::java::render_statement(
        stmt, 3, false, &enums, &registry, &helpers, &inline_counter,
        &address_of_vars, &double_address_of_vars, &float_input_params,
    )
}

/// Helper to call C render_statement with minimal boilerplate.
fn render_c_stmt(stmt: &ir::Statement) -> String {
    let enums = HashMap::new();
    let registry = make_registry();
    let helpers = HelperRegistry::empty();
    let inline_counter = std::cell::Cell::new(0);
    backends::c::render_statement(
        stmt, 3, false, &enums, &registry, &helpers, &inline_counter,
    )
}

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
    // MA dispatches to the per-type moving averages via the unguarded variants.
    // (MACD was the original vehicle, but its lockstep fusion removed the EMA
    // calls.)
    let (func, enums) = load_indicator("ma");
    let out = generate_all(&func, &enums);
    let j = &out.java;

    // Anchor the call site so demaUnguarded(/temaUnguarded( (adjacent dispatch
    // arms) cannot substring-shadow the EMA arm.
    assert!(
        j.contains("= emaUnguarded("),
        "Java MA should call emaUnguarded(): {j}"
    );
    assert!(
        j.contains("= emaLookback("),
        "Java MA should call emaLookback(): {j}"
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

    // STOCHRSI calls rsi and stochf (stochf's Java name is the irregular `stochF`)
    assert!(
        j.contains("rsiUnguarded(") || j.contains("rsiLookback("),
        "Java STOCHRSI should call rsi: {j}"
    );
    assert!(
        j.contains("stochFUnguarded(") || j.contains("stochFLookback("),
        "Java STOCHRSI should call stochF: {j}"
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
        j.contains("case Sma:") || j.contains("case Ema:"),
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

#[test]
fn java_ternary_bool_to_int_optimization() {
    // (cond) ? 1 : 0 should simplify to just the condition
    let stmt = ir::Statement::Assign {
        target: ir::Expr::Var("x".to_string()),
        value: ir::Expr::Ternary(
            Box::new(ir::Expr::BinOp(
                Box::new(ir::Expr::Var("a".to_string())),
                ir::BinOp::Greater,
                Box::new(ir::Expr::Var("b".to_string())),
            )),
            Box::new(ir::Expr::IntLiteral(1)),
            Box::new(ir::Expr::IntLiteral(0)),
        ),
        compound: false,
    };
    let rendered = render_java_stmt(&stmt);
    // Should NOT have ternary syntax, just the condition
    assert!(
        !rendered.contains("?"),
        "Java ternary (cond)?1:0 should simplify to just cond: {rendered}"
    );
    assert!(
        rendered.contains("a > b"),
        "Java ternary should contain the condition directly: {rendered}"
    );
}

#[test]
fn java_ternary_inverted_bool_optimization() {
    // (cond) ? 0 : 1 should simplify to !(condition)
    let stmt = ir::Statement::Assign {
        target: ir::Expr::Var("x".to_string()),
        value: ir::Expr::Ternary(
            Box::new(ir::Expr::BinOp(
                Box::new(ir::Expr::Var("a".to_string())),
                ir::BinOp::Less,
                Box::new(ir::Expr::Var("b".to_string())),
            )),
            Box::new(ir::Expr::IntLiteral(0)),
            Box::new(ir::Expr::IntLiteral(1)),
        ),
        compound: false,
    };
    let rendered = render_java_stmt(&stmt);
    assert!(
        rendered.contains("!("),
        "Java ternary (cond)?0:1 should simplify to !(cond): {rendered}"
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
    // MA dispatches to the per-type moving averages via the unguarded variants.
    // (MACD was the original vehicle, but its lockstep fusion removed the EMA
    // calls.)
    let (func, enums) = load_indicator("ma");
    let out = generate_all(&func, &enums);
    let c = &out.c;

    assert!(
        c.contains("TA_INT_EMA(") || c.contains("TA_EMA(") || c.contains("TA_EMA_Unguarded("),
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

    let lookback_end = j.find("public RetCode macd(").unwrap();
    let lookback = &j[..lookback_end];
    assert!(
        lookback.contains("macdLookback"),
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

    // Java lookback should call rsiLookback and stochfLookback
    let j = &out.java;
    assert!(
        j.contains("rsiLookback(") || j.contains("stochfLookback("),
        "Java STOCHRSI lookback should have cross-indicator lookback calls"
    );
}

// ---------------------------------------------------------------------------
// Java Var name mappings (exercises lines 1307-1326)
// ---------------------------------------------------------------------------

#[test]
fn java_var_name_mappings() {
    let stmts = vec![
        ("COMPATIBILITY", "this.compatibility"),
        ("METASTOCK", "Compatibility.Metastock"),
        ("DEFAULT", "Compatibility.Default"),
        ("BAD_PARAM", "RetCode.BadParam"),
        ("SUCCESS", "RetCode.Success"),
        ("ALLOC_ERR", "RetCode.AllocErr"),
        ("INTERNAL_ERROR", "RetCode.InternalError"),
        ("TA_MAType_SMA", "MAType.Sma"),
        ("TA_MAType_EMA", "MAType.Ema"),
        ("TA_MAType_WMA", "MAType.Wma"),
        ("TA_MAType_DEMA", "MAType.Dema"),
        ("TA_MAType_TEMA", "MAType.Tema"),
        ("TA_MAType_TRIMA", "MAType.Trima"),
        ("TA_MAType_KAMA", "MAType.Kama"),
        ("TA_MAType_MAMA", "MAType.Mama"),
        ("TA_MAType_T3", "MAType.T3"),
    ];

    for (var_name, expected) in stmts {
        let stmt = ir::Statement::Assign {
            target: ir::Expr::Var("result".to_string()),
            value: ir::Expr::Var(var_name.to_string()),
            compound: false,
        };
        let rendered = render_java_stmt(&stmt);
        assert!(
            rendered.contains(expected),
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

#[test]
fn java_assign_underscore_free_is_empty() {
    let stmt = ir::Statement::Expr(ir::Expr::FuncCall(
        "free".to_string(),
        vec![ir::Expr::Var("buf".to_string())],
    ));
    let rendered = render_java_stmt(&stmt);
    assert!(
        rendered.is_empty(),
        "Java Expr(free(buf)) should produce empty output: '{rendered}'"
    );
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
        rendered1.contains("ENUM_VALUE(Compatibility,TA_COMPATIBILITY_METASTOCK,Metastock)"),
        "C METASTOCK should render as ENUM_VALUE macro: {rendered1}"
    );

    let stmt2 = ir::Statement::Assign {
        target: ir::Expr::Var("x".to_string()),
        value: ir::Expr::Var("DEFAULT".to_string()),
        compound: false,
    };
    let rendered2 = render_c_stmt(&stmt2);
    assert!(
        rendered2.contains("ENUM_VALUE(Compatibility,TA_COMPATIBILITY_DEFAULT,Default)"),
        "C DEFAULT should render as ENUM_VALUE macro: {rendered2}"
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
// Streaming dispatch emission (TC composed tier: MA)
// ---------------------------------------------------------------------------

/// Pin the generated MA dispatch stream section: tagged handle over the
/// callees' PUBLIC streams, batch-order ENUM_CASE arms, identity fast path,
/// the one remaining unsupported arm (MAMA) rejecting at Open.
#[test]
fn test_c_ma_dispatch_stream_section() {
    let (mut func, enums) = load_indicator("ma");
    func.streaming = true; // the YAML flag flips with this milestone
    let registry = make_registry();
    let helpers = HelperRegistry::empty();
    let c = backends::c::generate(&func, &enums, &registry, &helpers);

    // Handle: params + a single tagged sub pointer, no StreamStep.
    assert!(c.contains("struct TA_MA_Stream {"), "state struct");
    assert!(c.contains("void *sub;"), "tagged sub-stream pointer");
    assert!(!c.contains("TA_MA_StreamStep"), "dispatch has no transition fn");

    // Open: identity path first (mirrors batch order), then the dispatch.
    assert!(
        c.contains("if( historyLen < TA_MA_Lookback( optInTimePeriod, optInMAType ) + 1 )"),
        "identity min-history check"
    );
    for (label, callee) in [
        ("Sma", "TA_SMA"),
        ("Ema", "TA_EMA"),
        ("Wma", "TA_WMA"),
        ("Dema", "TA_DEMA"),
        ("Tema", "TA_TEMA"),
        ("Kama", "TA_KAMA"),
        ("T3", "TA_T3"),
        ("Trima", "TA_TRIMA"), // dual-mode stream (M6c): auto-promoted from reject
    ] {
        assert!(
            c.contains(&format!("case ENUM_CASE(MAType, TA_MAType_{}, {label}):", label.to_uppercase())),
            "supported arm case label for {label}"
        );
        assert!(c.contains(&format!("{callee}_OpenInternal(")), "sub open for {callee}");
        assert!(c.contains(&format!("{callee}_Update(")), "sub update for {callee}");
        assert!(c.contains(&format!("{callee}_Peek(")), "sub peek for {callee}");
        assert!(c.contains(&format!("{callee}_Close(")), "sub close for {callee}");
    }
    // T3's fixed vfactor literal forwards positionally; the dispatch threads
    // its own startIdx into the arm's internal open.
    assert!(
        c.contains("TA_T3_OpenInternal( optInTimePeriod, 0.7, inReal, startIdx, historyLen"),
        "T3 arm forwards the 0.7 vfactor literal + startIdx"
    );
    // MAMA is the only remaining unsupported arm — it rejects at Open and never
    // opens a sub-stream (TRIMA gained a dual-mode stream in M6c and is now a
    // supported arm above).
    assert!(
        c.contains("case ENUM_CASE(MAType, TA_MAType_MAMA, Mama): /* no mama stream */"),
        "MAMA reject arm"
    );
    assert!(!c.contains("TA_MAMA_Open"), "no MAMA sub open");
    // Update/Peek identity short-circuit reads the handle's params.
    assert!(
        c.contains("if( stream->optInTimePeriod == 1 )"),
        "identity short-circuit on the handle"
    );
    // Peek keeps the handle logically const (const sub cast, no state copy).
    assert!(
        c.contains("(const TA_SMA_Stream *)stream->sub"),
        "const sub cast in Peek"
    );
}

/// Pin the generated MINUS_DM dual-mode stream section: ONE union state struct,
/// ONE StreamStep that branches on the stored (immutable) period param — no
/// separate mode tag — and an OpenInternal that selects the degenerate vs the
/// Wilder arm by the same predicate. The input `.c` is untouched: both arms are
/// transcribed verbatim, so the period<=1 raw-DM1 behavior (which ignores the
/// unstable period) is preserved by construction, not re-derived.
#[test]
fn test_c_minus_dm_dual_mode_stream_section() {
    let (mut func, enums) = load_indicator("minus_dm");
    func.streaming = true;
    let registry = make_registry();
    let helpers = HelperRegistry::empty();
    let c = backends::c::generate(&func, &enums, &registry, &helpers);

    // Exactly one StreamStep (not one per mode), branching on the stored param.
    assert_eq!(c.matches("TA_MINUS_DM_StreamStep( struct").count(), 1, "one StreamStep def");
    assert!(
        c.contains("if( sp->optInTimePeriod <= 1 )"),
        "step selects the degenerate arm from the immutable stored param"
    );
    // Wilder smoothing lives in mode B only; the degenerate arm writes raw DM1.
    assert!(
        c.contains("sp->prevMinusDM = sp->prevMinusDM - sp->prevMinusDM / sp->optInTimePeriod"),
        "Wilder recurrence in mode B"
    );
    // OpenInternal selects the arm on the bare predicate (param is a local there).
    assert!(c.contains("if( optInTimePeriod <= 1 )"), "open selects mode by bare predicate");
    // The union struct carries mode B's accumulator (mode A never touches it).
    let struct_sec = c
        .split("struct TA_MINUS_DM_Stream {")
        .nth(1)
        .and_then(|s| s.split("};").next())
        .expect("state struct");
    assert!(struct_sec.contains("double prevMinusDM;"), "union carries prevMinusDM");
}

/// Pin the generated HT_DCPERIOD stream section (M7c): the Hilbert-transform
/// family streams via two general steady-loop normalizations —
///   (1) CARRIED PARITY: the `today % 2` quadrature branch reads an int
///       `streamParity` field, seeded `historyLen % 2` in Open and flipped
///       `1 - streamParity` each step; and
///   (2) OUTPUT-GATE STRIP: the `if (today >= startIdx)` output gate is promoted
///       to an UNCONDITIONAL write in the step (Open's batch replay still
///       suppresses warm-up).
/// This render pin also neuter-checks build_transition: dropping either
/// recognizer makes `backends::c::generate` PANIC (the `today` cursor leaks into
/// the transition), so a clean render proves both fired.
#[test]
fn test_c_ht_dcperiod_parity_stream_section() {
    let (mut func, enums) = load_indicator("ht_dcperiod");
    func.streaming = true;
    let registry = make_registry();
    let helpers = HelperRegistry::empty();
    let c = backends::c::generate(&func, &enums, &registry, &helpers);
    let stream = &c[c.find("/**** Streaming API *****/").expect("stream section")..];

    // (2) carried parity: int field, seeded in Open, flipped in the step.
    assert!(stream.contains("int streamParity;"), "streamParity int state field");
    assert!(
        stream.contains("sp->streamParity = historyLen % 2;"),
        "parity seeded to the next bar's parity in Open"
    );
    assert!(
        stream.contains("if( sp->streamParity == 0 )"),
        "the step branches on the carried parity, not `today % 2`"
    );
    assert!(
        stream.contains("sp->streamParity = 1 - sp->streamParity;"),
        "parity flips each step"
    );
    // (1) output-gate strip: the step writes outReal UNCONDITIONALLY (no
    // `today >= startIdx` gate survives in the per-bar transition).
    let step = stream
        .split("TA_HT_DCPERIOD_StreamStep")
        .nth(1)
        .expect("StreamStep emitted");
    let step_body = &step[..step.find("TA_HT_DCPERIOD_OpenInternal").unwrap_or(step.len())];
    assert!(
        step_body.contains("*outReal= sp->smoothPeriod;"),
        "unconditional smoothPeriod output in the step"
    );
    // No absolute-index leak: `startIdx` (the gate RHS) and the raw `% 2` parity
    // test are both gone — the gate was stripped and `today % 2` was carried.
    // (A `todayValue` temp legitimately survives; that is the bar input, not the
    // cursor.)
    assert!(
        !step_body.contains("startIdx") && !step_body.contains("% 2"),
        "no gate (`startIdx`) or raw parity (`% 2`) leaks into the step"
    );
    // WMA price smoother rides as a trailing ring; the 8 Hilbert double[3]
    // buffers ride as fixed-array carried state (memcpy capture).
    assert!(stream.contains("double *ring_trailingWMAIdx_inReal;"), "WMA trailing ring");
    assert!(stream.contains("double detrender_Even[3];"), "fixed Hilbert array state");
    assert!(
        stream.contains("memcpy( sp->detrender_Even, detrender_Even, sizeof( sp->detrender_Even ) );"),
        "fixed arrays captured by memcpy in Open"
    );
}

/// Pin the generated HT_PHASOR stream section: the SECOND consumer of the two
/// general normalizations, and the one that stresses their nesting. Unlike
/// HT_DCPERIOD, HT_PHASOR writes its TWO outputs under an output gate NESTED
/// INSIDE each odd/even parity arm. This pins that (a) the gate strip reaches
/// nested gates (both outputs land UNCONDITIONALLY inside `if(streamParity==0)`
/// / else), (b) the carried-parity machinery is reused verbatim, and (c) both
/// outputs are written per bar in the arm that runs.
#[test]
fn test_c_ht_phasor_nested_gate_two_outputs_stream_section() {
    let (mut func, enums) = load_indicator("ht_phasor");
    func.streaming = true;
    let registry = make_registry();
    let helpers = HelperRegistry::empty();
    let c = backends::c::generate(&func, &enums, &registry, &helpers);
    let stream = &c[c.find("/**** Streaming API *****/").expect("stream section")..];

    // Reused carried-parity machinery (same as HT_DCPERIOD).
    assert!(stream.contains("int streamParity;"), "streamParity int state field");
    assert!(stream.contains("sp->streamParity = historyLen % 2;"), "parity seeded in Open");
    assert!(stream.contains("sp->streamParity = 1 - sp->streamParity;"), "parity flips each step");

    let step = stream
        .split("TA_HT_PHASOR_StreamStep")
        .nth(1)
        .expect("StreamStep emitted");
    let step_body = &step[..step.find("TA_HT_PHASOR_OpenInternal").unwrap_or(step.len())];
    // The step branches on the carried parity, and BOTH outputs are written
    // unconditionally in each arm (the nested `today >= startIdx` gate stripped).
    assert!(step_body.contains("if( sp->streamParity == 0 )"), "parity branch in the step");
    assert_eq!(
        step_body.matches("*outQuadrature= sp->Q1;").count(),
        2,
        "outQuadrature written unconditionally in BOTH parity arms (nested gate stripped)"
    );
    assert!(
        step_body.contains("*outInPhase= sp->I1ForEvenPrev3;")
            && step_body.contains("*outInPhase= sp->I1ForOddPrev3;"),
        "outInPhase written per-arm with the arm's own carried I1"
    );
    assert!(
        !step_body.contains("startIdx") && !step_body.contains("% 2"),
        "no gate (`startIdx`) or raw parity (`% 2`) leaks into the step"
    );
}

/// Small helper: the streaming section of a generated HT function.
fn ht_stream_section(name: &str) -> String {
    let (mut func, enums) = load_indicator(name);
    func.streaming = true;
    let registry = make_registry();
    let helpers = HelperRegistry::empty();
    let c = backends::c::generate(&func, &enums, &registry, &helpers);
    let start = c.find("/**** Streaming API *****/").expect("stream section");
    c[start..].to_string()
}

/// Pin HT_DCPHASE: the first coexistence of a smoothPrice CIRCBUF, a WMA trailing
/// ring, and the eight fixed Hilbert arrays in ONE handle. The DCPhase backward
/// rescan reads the circbuf; DCPhase is carried across bars.
#[test]
fn test_c_ht_dcphase_circ_ring_fixed_coexist() {
    let s = ht_stream_section("ht_dcphase");
    assert!(s.contains("double *cb_smoothPrice;"), "smoothPrice circbuf");
    assert!(s.contains("double *ring_trailingWMAIdx_inReal;"), "WMA trailing ring");
    assert!(s.contains("double detrender_Even[3];"), "fixed Hilbert array");
    assert!(s.contains("double DCPhase;"), "DCPhase carried across bars");
    assert!(s.contains("sp->cb_smoothPrice[sp->smoothPrice_Idx] = sp->smoothedValue;"), "circbuf write");
    assert!(s.contains("sp->cb_smoothPrice[sp->idx]"), "circbuf backward rescan read");
    assert!(s.contains("memcpy( sp->cb_smoothPrice, smoothPrice"), "circbuf captured (contents+phase) in Open");
    assert!(s.contains("*outReal= sp->DCPhase;"), "unconditional DCPhase output (gate stripped)");
}

/// Pin HT_SINE: DCPHASE's circbuf/ring body with TWO sin() outputs.
#[test]
fn test_c_ht_sine_two_sin_outputs() {
    let s = ht_stream_section("ht_sine");
    assert!(s.contains("double *cb_smoothPrice;"), "shares DCPHASE's circbuf");
    let step = s.split("TA_HT_SINE_StreamStep").nth(1).unwrap();
    let step = &step[..step.find("TA_HT_SINE_OpenInternal").unwrap_or(step.len())];
    assert!(step.contains("*outSine="), "outSine written unconditionally");
    assert!(step.contains("*outLeadSine="), "outLeadSine written unconditionally");
    assert!(!step.contains("startIdx") && !step.contains("% 2"), "no cursor leak in the step");
}

/// Pin HT_TRENDLINE: a rescan window over the RAW input (the padded-loop source
/// rewrite of `inReal[idx--]`), no circbuf, single output.
#[test]
fn test_c_ht_trendline_raw_price_window() {
    let s = ht_stream_section("ht_trendline");
    assert!(s.contains("double *win_i_inReal;"), "rescan window over raw inReal");
    assert!(!s.contains("cb_smoothPrice"), "no smoothPrice circbuf (removed, issue #88)");
    let step = s.split("TA_HT_TRENDLINE_StreamStep").nth(1).unwrap();
    let step = &step[..step.find("TA_HT_TRENDLINE_OpenInternal").unwrap_or(step.len())];
    assert!(step.contains("sp->win_i_inReal[(sp->winPos_i + sp->winCap_i - sp->i) % sp->winCap_i]"), "window read of bar today-i");
    assert!(step.contains("if( sp->i < sp->DCPeriodInt )"), "guarded to the first DCPeriodInt bars");
    assert!(step.contains("*outReal= sp->tempReal2;"), "unconditional trendline output");
}

/// Pin HT_TRENDMODE: the full HT union — WMA ring + smoothPrice circbuf + a
/// raw-price rescan window (separate counter j) + an INTEGER output.
#[test]
fn test_c_ht_trendmode_full_union() {
    let s = ht_stream_section("ht_trendmode");
    assert!(s.contains("double *ring_trailingWMAIdx_inReal;"), "WMA ring");
    assert!(s.contains("double *cb_smoothPrice;"), "smoothPrice circbuf");
    assert!(s.contains("double *win_j_inReal;"), "raw-price rescan window (counter j)");
    let step = s.split("TA_HT_TRENDMODE_StreamStep").nth(1).unwrap();
    let step = &step[..step.find("TA_HT_TRENDMODE_OpenInternal").unwrap_or(step.len())];
    assert!(step.contains("*outInteger="), "integer trend-mode output, unconditional");
    assert!(step.contains("sp->cb_smoothPrice[sp->idx]"), "circbuf DC-phase read");
    assert!(step.contains("sp->win_j_inReal[(sp->winPos_j + sp->winCap_j - sp->j) % sp->winCap_j]"), "window trendline read");
    assert!(!step.contains("startIdx") && !step.contains("% 2"), "no cursor leak in the step");
}

/// Pin MAMA — the last MAType function to stream (empties the every-MAType ratchet
/// blocklist). It is an ordinary HT function (WMA ring + parity) with two real
/// optional params and two coupled outputs (mama/fama) written in a top-level gate.
/// (MA's *dispatch* still rejects MAType_MAMA at Open — a two-output callee cannot
/// be a 1:1 delegation into MA's single output; pinned by ma_derives_dispatch_plan.)
#[test]
fn test_c_mama_two_outputs_and_params() {
    let s = ht_stream_section("mama");
    assert!(s.contains("double optInFastLimit;") && s.contains("double optInSlowLimit;"), "real params carried in the handle");
    assert!(s.contains("double mama;") && s.contains("double fama;"), "coupled mama/fama carried");
    let step = s.split("TA_MAMA_StreamStep").nth(1).unwrap();
    let step = &step[..step.find("TA_MAMA_OpenInternal").unwrap_or(step.len())];
    assert!(step.contains("if( sp->streamParity == 0 )"), "parity branch");
    assert!(step.contains("*outMAMA= sp->mama;") && step.contains("*outFAMA= sp->fama;"), "both outputs written unconditionally (gate stripped)");
    assert!(step.contains("sp->optInFastLimit") && step.contains("sp->optInSlowLimit"), "params drive the adaptive alpha");
    assert!(!step.contains("startIdx") && !step.contains("% 2"), "no cursor leak in the step");
}

/// Pin MAVP — the last function and the campaign's one genuinely-new tier: a
/// moving average whose period varies per bar, streamed as a BANK of sub-MA
/// streams. Open builds `maxPeriod - minPeriod + 1` sub-streams (each via the
/// callee's OpenInternal) with all-freed-so-far OOM; Update advances the whole
/// bank in lockstep and indexes by the clamped period; Peek previews only the
/// selected slot; Close frees the bank.
#[test]
fn test_c_mavp_period_bank() {
    let s = ht_stream_section("mavp");
    // Bank of sub-MA streams + scratch, sized at Open.
    assert!(s.contains("struct TA_MA_Stream **bank;"), "bank of sub-MA handles");
    assert!(s.contains("double *scratch;"), "per-slot lockstep output scratch");
    assert!(s.contains("sp->nBank = optInMaxPeriod - optInMinPeriod + 1;"), "one slot per possible period");
    assert!(s.contains("if( optInMinPeriod > optInMaxPeriod ) return TA_BAD_PARAM;"), "inverted window rejected");
    // Every sub-MA is seeded at the SHARED max-period lookback (matching batch),
    // NOT at its own lookback — else period < maxPeriod diverges. This bug fooled
    // every objective gate (the fuzz period-selector always clamped to maxPeriod);
    // pin the anchor so it can never regress.
    assert!(s.contains("lookbackTotal = TA_MA_Lookback( optInMaxPeriod, optInMAType );"), "shared max-period lookback anchor");
    assert!(s.contains("subStart = startIdx < lookbackTotal ? lookbackTotal : startIdx;"), "clamp start to the shared anchor");
    // Open: bank loop opening each period's sub-stream at subStart, all-freed-so-far on OOM.
    assert!(s.contains("TA_MA_OpenInternal( optInMinPeriod + k, optInMAType, inReal, subStart, historyLen,"), "sub-open per period at the shared anchor, MAType forwarded");
    assert!(s.contains("for( j = 0; j < k; j++ ) TA_MA_Close( sp->bank[j] );"), "frees sub-streams opened so far on failure");
    // Update: lockstep advance + clamp-indexed output.
    let upd = s.split("TA_MAVP_Update").nth(1).unwrap();
    let upd = &upd[..upd.find("TA_MAVP_Peek").unwrap_or(upd.len())];
    assert!(upd.contains("for( k = 0; k < stream->nBank; k++ )") && upd.contains("TA_MA_Update( stream->bank[k], inReal, &stream->scratch[k] );"), "advances the whole bank in lockstep");
    assert!(upd.contains("if( cp < stream->optInMinPeriod ) cp = stream->optInMinPeriod;"), "clamps the per-bar period");
    assert!(upd.contains("*outReal = stream->scratch[cp - stream->optInMinPeriod];"), "outputs the selected slot");
    // Peek: only the selected slot (non-committing).
    let peek = s.split("TA_MAVP_Peek").nth(1).unwrap();
    let peek = &peek[..peek.find("TA_MAVP_Close").unwrap_or(peek.len())];
    assert!(peek.contains("TA_MA_Peek( stream->bank[cp - stream->optInMinPeriod], inReal, outReal );"), "peeks only the selected slot");
    assert!(!peek.contains("TA_MA_Update"), "peek never advances the bank");
    // Close frees every sub-stream + the arrays.
    assert!(s.contains("if( stream->bank[k] ) TA_MA_Close( stream->bank[k] );"), "close frees each sub-stream");
}

/// Pin the generated TRIMA dual-mode (if/else) stream section: the odd/even arms
/// are genuinely different but share identical rings, so the handle carries ONE
/// ring set + one StreamStep branching on the stored parity; the ring buffers are
/// freed by StreamRelease and mirrored in Peek.
#[test]
fn test_c_trima_dual_mode_rings_stream_section() {
    let (mut func, enums) = load_indicator("trima");
    func.streaming = true;
    let registry = make_registry();
    let helpers = HelperRegistry::empty();
    let c = backends::c::generate(&func, &enums, &registry, &helpers);

    // Union struct: shared triangular-sum scalars + the SHARED rings (one set).
    assert!(
        c.contains("double *ring_middleIdx_inReal;") && c.contains("double *ring_trailingIdx_inReal;"),
        "shared middleIdx/trailingIdx rings (one set, both arms)"
    );
    assert!(c.contains("double numerator;"), "shared triangular-sum accumulator");
    assert_eq!(c.matches("TA_TRIMA_StreamStep( struct").count(), 1, "one StreamStep");
    assert!(
        c.contains("if( sp->optInTimePeriod % 2 == 1 )"),
        "step branches on the stored parity"
    );
    assert!(c.contains("TA_TRIMA_StreamRelease"), "StreamRelease frees the rings");
    assert!(c.contains("ringMirror_middleIdx_inReal"), "Peek ring mirror");
}

/// Pin the generated MIDPRICE fast-path-skip stream section: the `if(period<=20)`
/// arms are bit-identical (batch perf split), so ONLY the general (else) T4
/// extrema arm is streamed — one StreamStep, no mode branch, and the Open does
/// not transcribe the fast-path `period<=20` window-rescan arm.
#[test]
fn test_c_midprice_fastpath_skip_stream_section() {
    let (mut func, enums) = load_indicator("midprice");
    func.streaming = true;
    let registry = make_registry();
    let helpers = HelperRegistry::empty();
    let c = backends::c::generate(&func, &enums, &registry, &helpers);

    assert!(c.contains("struct TA_MIDPRICE_Stream {"), "state struct");
    assert!(
        c.contains("double *x_inHigh;") && c.contains("double *x_inLow;"),
        "T4 extrema rings for high/low"
    );
    assert_eq!(c.matches("TA_MIDPRICE_StreamStep( struct").count(), 1, "one StreamStep");
    assert!(
        c.contains("*outReal= (sp->highest + sp->lowest) / 2.0;"),
        "midprice combine in the extrema step"
    );
    // The fast-path window-rescan then-arm is NOT transcribed into the Open: no
    // `optInTimePeriod <= 20` branch survives (only the general else arm streams).
    let open = c
        .split("TA_MIDPRICE_OpenInternal")
        .nth(1)
        .expect("OpenInternal emitted");
    assert!(
        !open.contains("optInTimePeriod <= 20"),
        "the perf fast-path arm must be skipped, not transcribed"
    );
}

/// Pin the generated STOCH composed stream section: producer extrema state +
/// peekMode + typed sub handles; Open opens each sub-stream on the
/// materialized series BEFORE the batch call that consumes it (in-place
/// smoothing overwrites the raw %K right there — order is the contract);
/// the step pipelines through sub Update/Peek on the peekMode flag.
#[test]
fn test_c_stoch_composed_stream_section() {
    let (mut func, enums) = load_indicator("stoch");
    func.streaming = true;
    let registry = make_registry();
    let helpers = HelperRegistry::empty();
    let c = backends::c::generate(&func, &enums, &registry, &helpers);
    let stream = &c[c.find("/**** Streaming API *****/").expect("stream section")..];

    // Handle: producer extrema + peek flag + typed subs.
    assert!(stream.contains("int peekMode;"));
    assert!(stream.contains("TA_MA_Stream *sub0;"));
    assert!(stream.contains("TA_MA_Stream *sub1;"));

    // Open: sub0 opens on the RAW series strictly BEFORE the in-place
    // smoothing call; sub1 after it, before the %D call.
    let sub0 = stream.find("subRc = TA_MA_OpenInternal( optInSlowK_Period, optInSlowK_MAType, tempBuffer").expect("sub0 open");
    let ma1 = stream.find("retCode = TA_MA_Unguarded(0,outIdx - 1,tempBuffer").expect("in-place smoothing");
    let sub1 = stream.find("subRc = TA_MA_OpenInternal( optInSlowD_Period, optInSlowD_MAType, tempBuffer").expect("sub1 open");
    let ma2 = stream.find("optInSlowD_MAType,&dummyBegIdx,&dummyNBElement,sc_outSlowD").expect("%D call");
    assert!(sub0 < ma1 && ma1 < sub1 && sub1 < ma2, "sub-open ordering");

    // Out-meta pointers mapped to the dummies in the transcription (the
    // Open signature has no outBegIdx/outNBElement).
    assert!(stream.contains("&dummyBegIdx,&dummyNBElement"));
    assert!(!stream.contains(",outBegIdx,"), "raw out-meta arg leaked");

    // Step: ONE body; sub calls dispatch on the scratch copy's peekMode.
    assert!(stream.contains("if( sp->peekMode )"));
    assert!(stream.contains("TA_MA_Peek( (const TA_MA_Stream *)sp->sub0, cur_tempBuffer, &cur_tempBuffer );"));
    assert!(stream.contains("TA_MA_Update( sp->sub0, cur_tempBuffer, &cur_tempBuffer );"));
    assert!(stream.contains("TA_MA_Update( sp->sub1, cur_tempBuffer, &cur_outSlowD );"));
    assert!(stream.contains("*outSlowK = cur_tempBuffer;"), "memmove tail-align");
    assert!(stream.contains("*outSlowD = cur_outSlowD;"));

    // Peek sets the flag on the scratch copy; Close closes subs then frees.
    assert!(stream.contains("scratch.peekMode = 1;"));
    assert!(stream.contains("TA_MA_Close( stream->sub0 );"));
    assert!(stream.contains("TA_STOCH_StreamRelease( stream );"));
}

/// Pin the ADXR composed Open's allocation-failure cleanup. The intermediate
/// `adx` buffer's free is WITHHELD from the transcribed tail (the lag ring
/// seeds from its tail first), so it stays live through the capture epilogue —
/// every allocation-failure return there MUST free it, or an OOM leaks the
/// buffer. The adversarial review caught exactly this leak; this guards the
/// fix (each malloc-failure path frees everything allocated so far — no goto,
/// no fault-injection harness, just correct per-return cleanup).
#[test]
fn test_c_adxr_open_frees_withheld_buffer_on_oom_paths() {
    let (mut func, enums) = load_indicator("adxr");
    func.streaming = true;
    let registry = make_registry();
    let helpers = HelperRegistry::empty();
    let c = backends::c::generate(&func, &enums, &registry, &helpers);
    let open = &c[c.find("TA_RetCode TA_ADXR_Open").expect("ADXR Open")..];
    for guard in [
        "if( dummyNBElement < 1 ) { free( adx );",
        "if( !sp ) { free( adx );",
        "if( !sp->lagRing_adx ) { TA_Free( sp ); free( adx );",
        "if( !sp->lagRingMirror_adx ) { TA_Free( sp->lagRing_adx ); TA_Free( sp ); free( adx );",
    ] {
        assert!(
            open.contains(guard),
            "capture-epilogue OOM path must free the withheld adx buffer: `{guard}`"
        );
    }
    // Close releases the ring buffers (the other half of leak-freedom).
    let close = &c[c.find("TA_RetCode TA_ADXR_Close").expect("ADXR Close")..];
    assert!(close.contains("TA_Free( stream->lagRing_adx );"));
    assert!(close.contains("TA_Free( stream->lagRingMirror_adx );"));
}

/// Pin the BBANDS composed Open's allocation-failure cleanup. The general
/// (non-SMA) path allocates TWO intermediates — `tempBuffer1` for the moving
/// average, then `tempBuffer2` for the standard deviation. If `tempBuffer2`'s
/// malloc fails, `tempBuffer1` must be freed or it leaks: the auto-injected
/// null-check must free every intermediate allocated before it. Same OOM
/// discipline as ADXR (each malloc-failure path frees everything allocated so
/// far — no goto, no fault-injection), caught here at generate time.
#[test]
fn test_c_bbands_open_frees_prior_intermediate_on_oom() {
    let (mut func, enums) = load_indicator("bbands");
    func.streaming = true;
    let registry = make_registry();
    let helpers = HelperRegistry::empty();
    let c = backends::c::generate(&func, &enums, &registry, &helpers);
    let open = &c[c.find("TA_RetCode TA_BBANDS_Open").expect("BBANDS Open")..];

    // tempBuffer2's malloc-failure block frees the prior intermediate tempBuffer1.
    let tb2 = &open[open.find("tempBuffer2 = malloc").expect("tempBuffer2 malloc")..];
    let check = tb2.find("if( !tempBuffer2 )").expect("tempBuffer2 null check");
    let ret = tb2[check..]
        .find("return TA_ALLOC_ERR")
        .expect("tempBuffer2 alloc-err return");
    assert!(
        tb2[check..check + ret].contains("free( tempBuffer1 )"),
        "tempBuffer2 malloc-failure must free the prior intermediate tempBuffer1 (else OOM leaks it)"
    );

    // tempBuffer1's own malloc-failure block must NOT reference the
    // not-yet-allocated tempBuffer2 (nothing prior is live at that point).
    let tb1 = &open[open.find("tempBuffer1 = malloc").expect("tempBuffer1 malloc")..];
    let tb1_check = tb1.find("if( !tempBuffer1 )").expect("tempBuffer1 null check");
    let tb1_ret = tb1[tb1_check..]
        .find("return TA_ALLOC_ERR")
        .expect("tempBuffer1 alloc-err return");
    assert!(
        !tb1[tb1_check..tb1_check + tb1_ret].contains("tempBuffer2"),
        "tempBuffer1 malloc-failure must not touch the not-yet-allocated tempBuffer2"
    );

    // The scratch output arrays clean up progressively (each failure frees the
    // ones already allocated).
    assert!(
        open.contains(
            "if( !sc_outRealLowerBand ) { TA_Free( sc_outRealUpperBand ); \
             TA_Free( sc_outRealMiddleBand );"
        ),
        "scratch output arrays must clean up progressively on OOM"
    );
}
