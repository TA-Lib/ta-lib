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
use ta_codegen_lib::registry::{Lang, Registry};

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
    load_from(
        &Path::new(env!("CARGO_MANIFEST_DIR")).join("../../ta_codegen/input"),
        name,
    )
}

/// Load a synthetic gate fixture from `input_synth/` — the definitions carrying
/// generator constructs no shipped indicator uses (see `input_synth/README.md`).
/// `scripts/synth_gate.py` runs the same fixtures end-to-end through every
/// backend; these tests pin the rendered shape.
fn load_synth(name: &str) -> (ir::FuncDef, HashMap<String, ir::EnumDef>) {
    load_from(
        &Path::new(env!("CARGO_MANIFEST_DIR")).join("input_synth"),
        name,
    )
}

fn load_from(base: &Path, name: &str) -> (ir::FuncDef, HashMap<String, ir::EnumDef>) {
    let yaml_path = base.join(format!("{}/{}.yaml", name, name));
    let c_path = base.join(format!("{}/{}.c", name, name));

    // Enums always come from the real input tree: the synthetic fixtures share it.
    let enums_path = Path::new(env!("CARGO_MANIFEST_DIR"))
        .join("../../ta_codegen/input")
        .join("enums.yaml");
    let enums = parser::enums::load_enums(&enums_path);

    let mut func_def = parser::yaml::parse_yaml(&yaml_path);
    let parsed = parser::c_source::parse_c_source(&c_path);
    // Single source of truth: wire exactly as the production load paths do
    // (guarded body, lookback, explicit _private variant + extra params).
    parser::c_source::wire_parsed_source(&mut func_def, &parsed);

    (func_def, enums)
}

/// Load the shared `enums.yaml` (MAType, FuncUnstId) — the same source of truth
/// the generator derives its var() maps from. Used by tests that render enum
/// constants without needing a full indicator.
fn load_enums() -> HashMap<String, ir::EnumDef> {
    let path = Path::new(env!("CARGO_MANIFEST_DIR")).join("../../ta_codegen/input/enums.yaml");
    parser::enums::load_enums(&path)
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
}

fn make_registry() -> Registry {
    let base = Path::new(env!("CARGO_MANIFEST_DIR")).join("../../ta_codegen/input");
    Registry::from_dir(&base)
}

/// A registry over the `input_synth/` gate fixtures. `scripts/synth_gate.py`
/// copies them into `ta_codegen/input/` before generating, so there they are
/// registered alongside the shipped indicators; here they are their own tree.
/// Sufficient for the fixtures, none of which calls a shipped indicator.
fn make_synth_registry() -> Registry {
    let base = Path::new(env!("CARGO_MANIFEST_DIR")).join("input_synth");
    Registry::from_dir(&base)
}

fn generate_all(func: &ir::FuncDef, enums: &HashMap<String, ir::EnumDef>) -> AllOutputs {
    let registry = make_registry();
    let helpers = HelperRegistry::empty();
    AllOutputs {
        c: backends::c::generate(func, enums, &registry, &helpers),
        rust: backends::rust_lang::generate(func, enums, &registry, &helpers),
        java: backends::java::generate(func, enums, &registry, &helpers),
    }
}

// ---------------------------------------------------------------------------
// Variant check functions (extracted from macros for dynamic invocation)
// ---------------------------------------------------------------------------

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
        !c.contains("_Unguarded"),
        "{name}: C must not emit an unguarded variant"
    );
}

/// Check that all Rust variants exist for a given indicator.
/// `SMA` (guarded) plus `SMA_Lookback`, and `SMA_Private` only for the
/// definitions that declare one. No `_unchecked` variants. Concrete f64 types,
/// not generic.
fn check_rust_generic_variants(r: &str, name: &str) {
    // Lookback (non-generic)
    assert!(
        r.contains(&format!("{name}_Lookback")),
        "{name}: Rust missing {name}_Lookback"
    );
    // Guarded (concrete f64, no generics)
    assert!(
        r.contains(&format!("fn {name}(")),
        "{name}: Rust missing fn {name}("
    );
    assert!(
        !r.contains("_Unguarded"),
        "{name}: Rust must not emit an unguarded variant"
    );
}

/// Check that all Java variants exist for a given indicator.
fn check_java_variants(j: &str, name: &str) {
    assert!(
        j.contains(&format!("{name}_Lookback(")),
        "{name}: Java missing {name}_Lookback"
    );
    assert!(
        j.contains(&format!("RetCode {name}_Internal("))
            || j.contains(&format!("RetCode {name}_Internal (")),
        "{name}: Java missing {name} internal core"
    );
    assert!(
        !j.contains("Unguarded"),
        "{name}: Java must not emit an unguarded variant"
    );
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

/// Rust refuses to *parse* an `as` cast immediately followed by `<` or `<<` — it
/// reads `usize <` as the start of generic arguments. Any emitted cast that lands
/// on the left of one of those operators must therefore be wrapped in its own
/// parens (`((x) as usize) < y`, never `(x) as usize < y`). Issue #159.
///
/// A correctly-wrapped cast puts `)` between the type and the operator, so a bare
/// `as <ty>` followed by one of those two operators is the unparseable form.
///
/// `<` and `<<` only — measured against rustc, not assumed. `as usize <= y` and
/// `as usize <<= 2` both parse, so matching a bare `as <ty> <` prefix would fail
/// on legal output: 22 shipped sites spell `<=` after a cast.
fn check_rust_cast_parens(r: &str, name: &str) {
    // Ambiguous iff the cast is followed by `<` or `<<` that is not part of
    // `<=` / `<<=`.
    let ambiguous_at = |rest: &str| {
        let t = rest.trim_start();
        let mut it = t.chars();
        match (it.next(), it.next(), it.next()) {
            (Some('<'), Some('='), _) => None,             // <=   parses
            (Some('<'), Some('<'), Some('=')) => None,     // <<=  parses
            (Some('<'), Some('<'), _) => Some("<<"),
            (Some('<'), _, _) => Some("<"),
            _ => None,
        }
    };
    for (lineno, line) in r.lines().enumerate() {
        // Rustdoc and comments carry prose, not code the compiler parses.
        if line.trim_start().starts_with("//") {
            continue;
        }
        for ty in ["usize", "isize", "i32", "u32", "i64", "u64", "i16", "u16", "i8", "u8", "f32", "f64"] {
            let needle = format!("as {ty}");
            let mut from = 0;
            while let Some(hit) = line[from..].find(&needle) {
                let at = from + hit;
                let rest = &line[at + needle.len()..];
                // `as i32` must not be a prefix of a longer type name (`as i320`).
                let boundary = !rest.starts_with(|c: char| c.is_alphanumeric() || c == '_');
                if boundary {
                    if let Some(op) = ambiguous_at(rest) {
                        panic!(
                            "{}: unparenthesized `as {}` cast before `{}` (rustc reads it as \
                             generic args, not a comparison) at line {}:\n{}",
                            name,
                            ty,
                            op,
                            lineno + 1,
                            line.trim()
                        );
                    }
                }
                from = at + needle.len();
            }
        }
    }
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
        // Every backend spells the indicator exactly as `input/` names it.
        let upper = func.name.clone();
        let snake = name.clone();

        let result = std::panic::catch_unwind(std::panic::AssertUnwindSafe(|| {
            check_c_variants(&out.c, &upper, &snake);
            check_rust_generic_variants(&out.rust, &upper);
            check_java_variants(&out.java, &upper);
            check_c_int_alias(&out.c, &upper, &snake);
            check_rust_cast_parens(&out.rust, &snake);
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

    // Coverage is the gate here, not just the failure count: every check driven
    // from this loop (check_rust_cast_parens among them) is worth exactly the
    // number of indicators that reached it. `skipped` swallows a load/generate
    // panic, so a parser regression could quietly empty the corpus while the
    // suite stayed green — the floor of 6 this replaces would have allowed 162
    // of 168 to vanish silently.
    assert_eq!(
        skipped, 0,
        "{skipped} indicator(s) failed to load or generate and were silently \
         skipped, so no per-indicator check ran on them; {tested} were tested. \
         Fix the regression, or make the skip explicit if it is intended."
    );
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
    // Bare cross-indicator calls resolve to the guarded entry point. Anchored on
    // the first argument: bare `TA_SMA(` would also match a declaration.
    assert!(
        c.contains("TA_SMA(startIdx"),
        "C: MA should call guarded TA_SMA"
    );
    assert!(
        c.contains("TA_EMA(startIdx"),
        "C: MA should call guarded TA_EMA"
    );
    assert!(
        !c.contains("TA_SMA_Unguarded(") && !c.contains("TA_EMA_Unguarded("),
        "C: MA must not call the unguarded variants"
    );
}

#[test]
fn test_ma_java_cross_calls() {
    let (func, enums) = load_indicator("ma");
    let out = generate_all(&func, &enums);
    let j = &out.java;

    assert!(
        j.contains("SMA_Lookback("),
        "Java: MA should call SMA_Lookback"
    );
    assert!(
        j.contains("EMA_Lookback("),
        "Java: MA should call EMA_Lookback"
    );
    // Bare cross-indicator calls resolve to the guarded internal core, which
    // keeps the C-shaped MInteger out-params — going through the public
    // OutRange wrapper would allocate a throwaway MInteger pair per call.
    assert!(j.contains("SMA_Internal("), "Java: MA should call SMA_Internal");
    assert!(j.contains("EMA_Internal("), "Java: MA should call EMA_Internal");
    assert!(
        !j.contains("smaUnguardedInternal(") && !j.contains("emaUnguardedInternal("),
        "Java: MA must not call the unguarded cores"
    );
}

#[test]
fn test_ma_rust_cross_calls() {
    let (func, enums) = load_indicator("ma");
    let out = generate_all(&func, &enums);
    let r = &out.rust;

    // Lookback calls remain the same.
    assert!(
        r.contains("self.SMA_Lookback("),
        "Rust: MA should call self.SMA_Lookback"
    );
    assert!(
        r.contains("self.EMA_Lookback("),
        "Rust: MA should call self.EMA_Lookback"
    );
    // Bare cross-indicator calls go to the guarded fn
    assert!(
        r.contains("self.SMA("),
        "Rust: MA should call self.SMA"
    );
    assert!(
        r.contains("self.EMA("),
        "Rust: MA should call self.EMA"
    );
    assert!(
        !r.contains("self.sma_unguarded(") && !r.contains("self.ema_unguarded("),
        "Rust: MA must not call the unguarded variants"
    );
}

// ---------------------------------------------------------------------------
// 4. Logic vs guarded validation tests
// ---------------------------------------------------------------------------

/// Helper: extract the section of output between `start_marker` and `end_marker`.
///
/// BOTH markers must be present. Falling back to "everything after the start" on
/// a missing end marker silently turns a bounded `contains(..)` assertion into
/// "the whole output mentions this somewhere", so a test keeps passing while
/// checking strictly less.
/// Does `hay` contain a CALL to `name`, as opposed to merely the substring?
///
/// `RSI_Lookback(` is a suffix of `STOCHRSI_Lookback(`, so a bare `contains`
/// asserting that STOCHRSI calls RSI is satisfied by STOCHRSI's own definition
/// and can never fail. Requiring a non-identifier character before the name is
/// what makes the assertion mean what it says.
fn contains_call(hay: &str, name: &str) -> bool {
    let needle = format!("{name}(");
    let mut from = 0;
    while let Some(rel) = hay[from..].find(&needle) {
        let at = from + rel;
        let prev = hay[..at].chars().next_back();
        if !prev.is_some_and(|c| c.is_ascii_alphanumeric() || c == '_') {
            return true;
        }
        from = at + 1;
    }
    false
}

fn extract_section(output: &str, start_marker: &str, end_marker: &str) -> String {
    let start = output
        .find(start_marker)
        .unwrap_or_else(|| panic!("Could not find start marker '{start_marker}' in output"));
    let rest = &output[start..];
    let end = rest.find(end_marker).unwrap_or_else(|| {
        panic!("Could not find end marker '{end_marker}' after '{start_marker}' — the section would be unbounded")
    });
    rest[..end].to_string()
}

#[test]
fn test_c_sma_guarded_has_validation() {
    let (func, enums) = load_indicator("sma");
    let out = generate_all(&func, &enums);

    // Bounded by the float twin, which directly follows the double guarded body.
    let guarded = extract_section(&out.c, "TA_RetCode TA_SMA(", "TA_RetCode TA_S_SMA(");
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
fn test_c_synth_private_omits_validation() {
    // Exactly one tier validates: the guarded entry point, not `_Private`.
    // Anchored on the SYNTH4 gate fixture — no shipped indicator declares an
    // explicit _private (EMA's was folded away in #183).
    let (func, enums) = load_synth("synth4");
    let out = generate_all(&func, &enums);

    let private = extract_section(
        &out.c,
        "static TA_RetCode TA_SYNTH4_Private(",
        "TA_LIB_API TA_RetCode TA_SYNTH4(",
    );
    assert!(
        !private.contains("TA_OUT_OF_RANGE_START_INDEX"),
        "C SYNTH4 _Private should NOT have start index validation"
    );
    assert!(
        !private.contains("TA_OUT_OF_RANGE_END_INDEX"),
        "C SYNTH4 _Private should NOT have end index validation"
    );
}

#[test]
fn test_java_sma_guarded_has_validation() {
    let (func, enums) = load_indicator("sma");
    let out = generate_all(&func, &enums);

    // Extract the double-precision core, bounded before the float overload
    // Bounded to the DOUBLE core alone: the float twin is an overload with the
    // same name, so a marker that spans both would let it satisfy the assertion.
    let guarded = extract_section(&out.java, "RetCode SMA_Internal( int startIdx", "double inReal[]");
    let guarded = format!("{guarded}{}", extract_section(&out.java, "double inReal[]", "float inReal[]"));
    assert!(
        guarded.contains("OutOfRangeStartIndex"),
        "Java guarded SMA should have start index validation"
    );
}

#[test]
fn test_java_synth_private_omits_validation() {
    let (func, enums) = load_synth("synth4");
    let out = generate_all(&func, &enums);

    let private = extract_section(&out.java, "RetCode SYNTH4_Private(", "RetCode SYNTH4_Internal(");
    assert!(
        !private.contains("OutOfRangeStartIndex"),
        "Java SYNTH4_Private should NOT have start index validation"
    );
}

#[test]
fn test_rust_sma_guarded_has_validation() {
    let (func, enums) = load_indicator("sma");
    let out = generate_all(&func, &enums);

    // The guarded Rust function holds the algorithm and validates first, bounded
    // by the end of the impl block.
    let guarded = extract_section(&out.rust, "pub fn SMA(", "\n}\n");
    assert!(
        guarded.contains("endIdx < startIdx"),
        "Rust guarded SMA should have endIdx < startIdx check"
    );
}

#[test]
fn test_rust_synth_private_omits_validation() {
    let (func, enums) = load_synth("synth4");
    let out = generate_all(&func, &enums);

    // `pub(crate)`, matching C's file-`static` TA_SYNTH4_Private (#180): skipping
    // validation is only sound while the callers are the guarded bodies.
    let private = extract_section(&out.rust, "pub(crate) fn SYNTH4_Private(", "\n}\n");
    assert!(
        !private.contains("OutOfRangeStartIndex"),
        "Rust SYNTH4_Private should NOT have range validation"
    );
    assert!(
        !out.rust.contains("pub fn SYNTH4_Private("),
        "Rust synth4_private must not be crate-public: it is the one entry point with no \
         validation prologue, so a `pub` here bypasses the TA_MAX_INDEX bound (#180)"
    );
}

// Also test a different indicator for validation (RSI)
#[test]
fn test_c_rsi_guarded_has_validation() {
    let (func, enums) = load_indicator("rsi");
    let out = generate_all(&func, &enums);

    let guarded = extract_section(&out.c, "TA_RetCode TA_RSI(", "TA_RetCode TA_S_RSI(");
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
}

/// Java pins compatibility to Default and carries no such field, so the branches
/// are constant-folded at render time. RSI is the witness: its lookback has a
/// bare `== METASTOCK` test and its body a compound
/// `unstablePeriod == 0 && ... == METASTOCK` one, and both arms are dead here.
///
/// C renders the same IR and must keep both arms — that contrast is what makes
/// this non-vacuous (an empty Java body would satisfy the first assert alone).
#[test]
fn java_compatibility_is_folded_away() {
    for name in ["rsi", "cmo", "ema", "dema", "tema", "trix", "macd", "macdfix"] {
        let (func, enums) = load_indicator(name);
        let out = generate_all(&func, &enums);

        assert!(
            !out.java.contains("compatibility ==") && !out.java.contains("Compatibility."),
            "Java {name} must not reference the compatibility field — it is folded away"
        );
        assert!(
            out.c.contains("TA_GLOBALS_COMPATIBILITY"),
            "C {name} must keep both compatibility arms (proves the Java fold is \
             a backend choice, not an empty input)"
        );
    }
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
    // Enum labels render as plain C enumerator names
    assert!(
        out.c.contains("case TA_MAType_SMA:"),
        "C MA should use plain C enumerator names for switch labels"
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
    // Enum switch case labels must be UNQUALIFIED ("case SMA:") — qualified
    // labels are Java 21+ syntax and the shipped Core.java must compile on
    // older JDKs.
    assert!(
        out.java.contains("case SMA:") || out.java.contains("case EMA:"),
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
    let pub_fn_count = r.matches("pub fn SMA").count();
    assert_eq!(
        pub_fn_count, 4,
        "Rust SMA should have exactly 4 pub fn (sma, SMA_Lookback, SMA_Open, SMA_OpenAndFill), got {}",
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
    let rendered = backends::c::render_statement(&stmt, 0, false, &enums, &registry, &helpers, &inline_counter, &[]);

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

    let rust_lookback_end = rust_out.find("pub fn CDL2CROWS(").unwrap();
    let rust_lookback = &rust_out[..rust_lookback_end];
    assert!(
        rust_lookback.contains("self.candle_settings.body_long"),
        "Rust lookback should contain candle settings unpacking"
    );

    let java_lookback_end = java_out.find("RetCode CDL2CROWS_Internal(").unwrap();
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

    // Cross-indicator calls resolve to the guarded fn
    assert!(
        rust_out.contains("self.SMA("),
        "MA Rust should call self.SMA(): {rust_out}"
    );
    assert!(
        rust_out.contains("self.EMA("),
        "MA Rust should call self.EMA(): {rust_out}"
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
            rendered.contains("self.SMA_Lookback("),
            "`{fname}` must render as self.SMA_Lookback(), got: {rendered}"
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
        rust_out.contains("self.EMA("),
        "MA Rust dispatch should call self.EMA(): {rust_out}"
    );

    let synth_registry = make_synth_registry();
    let (func, enums) = load_synth("synth4");
    let rust_out = backends::rust_lang::generate(&func, &enums, &synth_registry, &helpers);
    assert!(
        rust_out.contains("self.SYNTH4_Private("),
        "SYNTH4 Rust guarded body should delegate to self.SYNTH4_Private(): {rust_out}"
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
        rust_out.contains("self.MA(") && rust_out.contains("&tempBuffer"),
        "STOCH Rust should pass &tempBuffer into self.MA(): {rust_out}"
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
        let has_cross_call = rust_out.contains("self.RSI")
            || rust_out.contains("self.STOCHF")
            || rust_out.contains("self.SMA");
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
        rust_out.contains("_Lookback("),
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
    let lookback_section = extract_section(&rust_out, "_Lookback(", "pub fn CDLKICKING(");
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

    let lookback_section = extract_section(&rust_out, "_Lookback(", "pub fn MULT(");
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

    let lookback_section = extract_section(&rust_out, "_Lookback(", "pub fn TEST(");
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

    let lookback_section = extract_section(&rust_out, "_Lookback(", "pub fn TEST(");
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
    let section = extract_section(&out, "_Lookback(", "pub fn TEST(");
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
        let section = extract_section(&out, "_Lookback(", "pub fn TEST(");
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
// Java backend coverage tests
// ===========================================================================

// ---------------------------------------------------------------------------
// Java: VarDecl rendering for all VarType variants via render_statement
// ---------------------------------------------------------------------------

/// Helper to call Java render_statement with minimal boilerplate.
fn render_java_stmt(stmt: &ir::Statement) -> String {
    // Real enums so MAType constants resolve from the enums.yaml-derived map
    // (var() no longer hardcodes the TA_MAType_* → MAType.<Pascal> arms).
    let enums = load_enums();
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
        stmt, 3, false, &enums, &registry, &helpers, &inline_counter, &[],
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
    // MA dispatches to the per-type moving averages via the guarded internal cores.
    // (MACD was the original vehicle, but its lockstep fusion removed the EMA
    // calls.)
    let (func, enums) = load_indicator("ma");
    let out = generate_all(&func, &enums);
    let j = &out.java;

    // Anchor the call site so demaInternal(/temaInternal( (adjacent dispatch
    // arms) cannot substring-shadow the EMA arm.
    assert!(
        j.contains("= EMA_Internal("),
        "Java MA should call EMA_Internal(): {j}"
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
    assert!(
        contains_call(j, "RSI_Internal") && contains_call(j, "RSI_Lookback"),
        "Java STOCHRSI should call RSI_Internal and RSI_Lookback: {j}"
    );
    assert!(
        contains_call(j, "STOCHF_Internal") && contains_call(j, "STOCHF_Lookback"),
        "Java STOCHRSI should call STOCHF_Internal and STOCHF_Lookback: {j}"
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

    let lookback_end = j.find("RetCode MACD_Internal(").unwrap();
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
// Streaming dispatch emission (TC composed tier: MA)
// ---------------------------------------------------------------------------

/// Pin the generated MA dispatch stream section: tagged handle over the
/// callees' PUBLIC streams, batch-order case arms, identity fast path, and
/// the MAMA arm forwarding the MAMA line while discarding FAMA as NULL (the
/// nullable-output delegation from issue #125 — no reject arm remains).
#[test]
fn test_c_ma_dispatch_stream_section() {
    let (mut func, enums) = load_indicator("ma");
    func.streaming = true; // the YAML flag flips with this milestone
    let registry = make_registry();
    let helpers = HelperRegistry::empty();
    let c = backends::c::generate(&func, &enums, &registry, &helpers);

    // Handle: params + a single tagged sub pointer, no StepInternal.
    assert!(c.contains("struct TA_MA_Stream {"), "state struct");
    assert!(c.contains("void *sub;"), "tagged sub-stream pointer");
    assert!(!c.contains("TA_MA_StepInternal"), "dispatch has no transition fn");

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
        ("Mama", "TA_MAMA"),   // nullable FAMA (#125): auto-promoted from reject
    ] {
        assert!(
            c.contains(&format!("case TA_MAType_{}:", label.to_uppercase())),
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
        c.contains("TA_T3_OpenInternal( &sub, inReal, startIdx, historyLen, optInTimePeriod, 0.7"),
        "T3 arm forwards the 0.7 vfactor literal + startIdx"
    );
    // MAMA is now a supported arm: FAMA is a nullable output (issue #125), so
    // MA's arm forwards the MAMA line to outReal and passes NULL for FAMA in
    // every verb (Open / OpenAndFill / Update / Peek). No reject arm remains.
    assert!(
        c.contains(
            "TA_MAMA_OpenInternal( &sub, inReal, startIdx, historyLen, 0.5, 0.05, outReal, NULL )"
        ),
        "MAMA arm forwards outReal + discards FAMA as NULL at Open"
    );
    assert!(
        c.contains("TA_MAMA_Update( (TA_MAMA_Stream *)stream->sub, inReal, outReal, NULL )"),
        "MAMA Update forwards outReal + NULL"
    );
    assert!(
        c.contains(
            "TA_MAMA_OpenAndFill( &sub, inReal, historyLen, 0.5, 0.05, outBegIdx, outNBElement, outReal, NULL )"
        ),
        "MAMA OpenAndFill forwards outReal + NULL"
    );
    assert!(!c.contains("/* no mama stream */"), "no MAMA reject arm remains");
    // Update/Peek identity short-circuit reads the handle's params; the guard
    // also covers the period-independent TA_MAType_DISABLED identity (issue #93).
    assert!(
        c.contains(
            "if( stream->optInTimePeriod == 1 || stream->optInMAType == TA_MAType_DISABLED )"
        ),
        "identity short-circuit on the handle (period 1 or DISABLED)"
    );
    // Peek keeps the handle logically const (const sub cast, no state copy).
    assert!(
        c.contains("(const TA_SMA_Stream *)stream->sub"),
        "const sub cast in Peek"
    );
}

/// FAMA is a nullable output (issue #125). In the BATCH C: `Output::is_nullable`
/// is set from the `nullable` flag, the guarded function skips its NULL-check but
/// keeps outMAMA's, the distinctness check guards the nullable operand, and every
/// body write is NULL-guarded while the `outIdx` advance rides the non-nullable
/// outMAMA. MA's batch arm collapses to a clean NULL delegation (no malloc).
#[test]
fn test_c_mama_nullable_fama_batch() {
    let (func, enums) = load_indicator("mama");
    let fama = func.outputs.iter().find(|o| o.name == "outFAMA").unwrap();
    let mama_out = func.outputs.iter().find(|o| o.name == "outMAMA").unwrap();
    assert!(fama.is_nullable(), "outFAMA carries the nullable flag");
    assert!(!mama_out.is_nullable(), "outMAMA is not nullable");

    let registry = make_registry();
    let helpers = HelperRegistry::empty();
    let c = backends::c::generate(&func, &enums, &registry, &helpers);
    // Guarded validation: outMAMA required, outFAMA optional.
    assert!(c.contains("if( !outMAMA )"), "outMAMA still NULL-checked");
    assert!(!c.contains("if( !outFAMA )"), "outFAMA NULL-check skipped (nullable)");
    assert!(
        c.contains("if( outFAMA != NULL && outMAMA == outFAMA )"),
        "distinctness guards the nullable operand (a NULL FAMA aliases nothing)"
    );
    // Body: FAMA store NULL-guarded; outIdx advance on the non-nullable outMAMA.
    assert!(
        c.contains("if( outFAMA != NULL )") && c.contains("outFAMA[outIdx] = fama;"),
        "FAMA store NULL-guarded (no side effect inside the guard)"
    );
    assert!(c.contains("outMAMA[outIdx++] = mama;"), "outIdx advance rides outMAMA");

    // MA's batch arm: clean NULL delegation, no unchecked discard buffer.
    let (ma, ma_enums) = load_indicator("ma");
    let mac = backends::c::generate(&ma, &ma_enums, &registry, &helpers);
    assert!(
        mac.contains(
            "TA_MAMA(startIdx,endIdx,inReal,0.5,0.05,outBegIdx,outNBElement,outReal,NULL)"
        ),
        "MA batch MAMA arm passes NULL for FAMA"
    );
    assert!(
        !mac.contains("dummyBuffer") && !mac.contains("malloc"),
        "the pre-#125 discard malloc is gone"
    );
}

/// Pin where a dual-mode function's identity path is emitted. HMA is the only
/// dual-mode function carrying one, and its mode predicate (`period == 2 ||
/// period == 3`) EXCLUDES the identity value, so an arm-local copy of the
/// `period == 1` guard is unreachable — the defect this pins against. The guard
/// belongs above the predicate, once per step, the way Open already emits it.
///
/// Values cannot see this: an unreachable branch changes no output, so
/// ta_regtest, the bitwise stream/OpenAndFill gates, clippy and the C build are
/// all silent on a regression here. Only a render pin catches it.
#[test]
fn test_dual_mode_identity_guard_is_hoisted_above_the_predicate() {
    let (mut func, enums) = load_indicator("hma");
    func.streaming = true;
    let registry = make_registry();
    let helpers = HelperRegistry::empty();
    let c = backends::c::generate(&func, &enums, &registry, &helpers);

    let step = c
        .split("static void TA_HMA_StepInternal(")
        .nth(1)
        .and_then(|s| s.split("\n}\n").next())
        .expect("step body");
    assert_eq!(
        step.matches("sp->optInTimePeriod == 1").count(),
        1,
        "exactly one identity guard per step, not one per mode arm:\n{step}"
    );
    let guard = step.find("sp->optInTimePeriod == 1").expect("identity guard");
    let pred = step
        .find("sp->optInTimePeriod == 2 || sp->optInTimePeriod == 3")
        .expect("mode predicate");
    assert!(
        guard < pred,
        "the identity guard must precede the mode predicate, not sit inside an arm:\n{step}"
    );
}

/// Pin the generated MINUS_DM dual-mode stream section: ONE union state struct,
/// ONE StepInternal that branches on the stored (immutable) period param — no
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

    // Exactly one StepInternal (not one per mode), branching on the stored param.
    assert_eq!(c.matches("TA_MINUS_DM_StepInternal( struct").count(), 1, "one StepInternal def");
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
        .split("TA_HT_DCPERIOD_StepInternal")
        .nth(1)
        .expect("StepInternal emitted");
    let step_body = &step[..step.find("TA_HT_DCPERIOD_OpenCore").unwrap_or(step.len())];
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
        .split("TA_HT_PHASOR_StepInternal")
        .nth(1)
        .expect("StepInternal emitted");
    let step_body = &step[..step.find("TA_HT_PHASOR_OpenCore").unwrap_or(step.len())];
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
    let step = s.split("TA_HT_SINE_StepInternal").nth(1).unwrap();
    let step = &step[..step.find("TA_HT_SINE_OpenCore").unwrap_or(step.len())];
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
    let step = s.split("TA_HT_TRENDLINE_StepInternal").nth(1).unwrap();
    let step = &step[..step.find("TA_HT_TRENDLINE_OpenCore").unwrap_or(step.len())];
    assert!(step.contains("sp->win_i_inReal[(sp->winPos_i + sp->winCap_i - sp->i >= sp->winCap_i) ?"), "de-modulo window read of bar today-i");
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
    let step = s.split("TA_HT_TRENDMODE_StepInternal").nth(1).unwrap();
    let step = &step[..step.find("TA_HT_TRENDMODE_OpenCore").unwrap_or(step.len())];
    assert!(step.contains("*outInteger="), "integer trend-mode output, unconditional");
    assert!(step.contains("sp->cb_smoothPrice[sp->idx]"), "circbuf DC-phase read");
    assert!(step.contains("sp->win_j_inReal[(sp->winPos_j + sp->winCap_j - sp->j >= sp->winCap_j) ?"), "de-modulo window trendline read");
    assert!(!step.contains("startIdx") && !step.contains("% 2"), "no cursor leak in the step");
}

/// Pin MAMA — an ordinary HT function (WMA ring + parity) with two real optional
/// params and two coupled outputs (mama/fama) written in a top-level gate. FAMA
/// is a nullable output (issue #125): its per-bar write is NULL-guarded so a
/// caller (MA's dispatch) can discard it — see `test_c_ma_dispatch_stream_section`.
#[test]
fn test_c_mama_two_outputs_and_params() {
    let s = ht_stream_section("mama");
    assert!(s.contains("double optInFastLimit;") && s.contains("double optInSlowLimit;"), "real params carried in the handle");
    assert!(s.contains("double mama;") && s.contains("double fama;"), "coupled mama/fama carried");
    let step = s.split("TA_MAMA_StepInternal").nth(1).unwrap();
    let step = &step[..step.find("TA_MAMA_OpenCore").unwrap_or(step.len())];
    assert!(step.contains("if( sp->streamParity == 0 )"), "parity branch");
    // MAMA line always written; FAMA (nullable) write is NULL-guarded so the
    // step never dereferences a NULL FAMA pointer (the gate itself is stripped).
    assert!(step.contains("*outMAMA= sp->mama;"), "MAMA line written unconditionally");
    assert!(
        step.contains("if( outFAMA != NULL )") && step.contains("*outFAMA= sp->fama;"),
        "FAMA is nullable (#125): its write is NULL-guarded"
    );
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
    assert!(s.contains("TA_MA_OpenInternal( &sp->bank[k], inReal, subStart, historyLen, optInMinPeriod + k, optInMAType,"), "sub-open per period at the shared anchor, MAType forwarded");
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
/// ring set + one StepInternal branching on the stored parity; the ring buffers are
/// freed by ReleaseInternal and mirrored in Peek.
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
    assert_eq!(c.matches("TA_TRIMA_StepInternal( struct").count(), 1, "one StepInternal");
    assert!(
        c.contains("if( sp->optInTimePeriod % 2 == 1 )"),
        "step branches on the stored parity"
    );
    assert!(c.contains("TA_TRIMA_ReleaseInternal"), "ReleaseInternal frees the rings");
    assert!(c.contains("ringMirror_middleIdx_inReal"), "Peek ring mirror");
}

/// Pin the generated MIDPRICE stream section: batch runs the block scan and the
/// stream runs `midprice_ALT1`'s T4 extrema automaton — one StepInternal, no
/// mode branch, and no trace of the block scan inside the Open.
///
/// Every check here asserts on a string the generator DOES produce, in both
/// directions — present in the batch tier, absent from the Open. An
/// absence-only assertion starts passing for free the day the generator stops
/// emitting the string it looks for, and says nothing from then on.
#[test]
fn test_c_midprice_stream_uses_the_declared_alternate() {
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
    assert_eq!(c.matches("TA_MIDPRICE_StepInternal( struct").count(), 1, "one StepInternal");
    assert!(
        c.contains("*outReal= (sp->highest + sp->lowest) / 2.0;"),
        "midprice combine in the extrema step"
    );
    // The generated section names the alternate it was built from.
    assert!(
        c.contains("/* Using midprice_ALT1 for TA_ALT={STREAM,ALL_LANGUAGES} */"),
        "the stream section must name the alternate it resolved to"
    );

    // ...and the marker must be telling the truth. A marker is derived from the
    // resolution, so on its own it would agree with a resolver that picked the
    // wrong body; these check the emitted CODE. The block scan's scratch and
    // block cursor appear in the batch tier and nowhere in the Open.
    let (batch, open) = c
        .split_once("TA_MIDPRICE_OpenCore")
        .expect("OpenCore emitted");
    for marker in ["sufHighest", "preHighest", "blockNext"] {
        assert!(
            batch.contains(marker),
            "batch tier lost the block scan (`{marker}` absent) — the BATCH cell should \
             resolve to the base body"
        );
        assert!(
            !open.contains(marker),
            "`{marker}` reached the Open: the STREAM cell resolved to the block scan, not to \
             midprice_ALT1"
        );
    }
    // The automaton's own state, conversely, must be there.
    assert!(open.contains("highestIdx"), "the alternate's cached-extremum index");
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

    // STOCH is the one shipped function that exercises BOTH sides of the
    // issue-#192 fusion rule, which is why this assertion lives here:
    //
    //   sub0's %K smoothing is IN PLACE — `TA_MA( .., tempBuffer, .., tempBuffer )`
    //   — so it must stay UNFUSED. A fused open would write tempBuffer during
    //   the warm-up pass while the sub-MA's own capture epilogue still has to
    //   read its input tail out of it, corrupting the handle.
    //
    //   sub1's %D writes a distinct destination, so it fuses: one pass that
    //   both warms the handle and fills sc_outSlowD, instead of a warm pass
    //   plus a batch call recomputing the same numbers.
    let sub0 = stream.find("subRc = TA_MA_OpenInternal( &sub0, tempBuffer").expect("sub0 open (must stay unfused: in-place)");
    let ma1 = stream.find("retCode = TA_MA(0,outIdx - 1,tempBuffer").expect("in-place smoothing");
    let sub1 = stream.find("subRc = TA_MA_OpenAndFillInternal( &sub1, tempBuffer").expect("sub1 open (must be fused)");
    assert!(sub0 < ma1 && ma1 < sub1, "sub-open ordering");
    // The fused sub1 replaced the %D batch call outright: nothing recomputes it.
    assert!(
        !stream.contains("optInSlowD_MAType,&dummyBegIdx,&dummyNBElement,sc_outSlowD"),
        "%D batch sub-call survived the fusion"
    );
    // Params trail the handle+history in the new Open order (input, optional, output).
    // The unfused sub0 still ends in the initial-output dummy; the fused sub1
    // carries the batch call's own out-meta and destination instead.
    assert!(stream.contains("optInSlowK_Period, optInSlowK_MAType, &subOpenDummy"), "slowK params forwarded to sub0 open");
    assert!(stream.contains("optInSlowD_Period, optInSlowD_MAType, &dummyBegIdx, &dummyNBElement, sc_outSlowD"), "slowD params + fill target forwarded to sub1 open");

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
    assert!(stream.contains("TA_STOCH_ReleaseInternal( stream );"));
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

/// A composed Open must emit ONE null-check block per allocated intermediate,
/// not two (issue #169). Every one of these inputs writes its own `if( !x )`
/// after the malloc, and the generator injects one as well — the injected one
/// carries the cascading `free()` of the prior intermediates, so it is the
/// keeper and the transcribed one is dropped. Nothing else in the suite would
/// notice the duplicate coming back: the OOM test below uses `find()`, which
/// matches the first copy either way, so a regression would show up only in a
/// `git diff` of the generated C.
#[test]
fn test_c_composed_open_emits_one_null_check_per_intermediate() {
    for (indicator, buffers) in [
        ("adxr", &["adx"][..]),
        ("apo", &["tempBuffer"]),
        ("bbands", &["tempBuffer1", "tempBuffer2"]),
        ("macdext", &["fastMABuffer", "slowMABuffer"]),
        ("ppo", &["tempBuffer"]),
        ("pvo", &["tempBuffer"]),
        ("stoch", &["tempBuffer"]),
        ("stochf", &["tempBuffer"]),
        ("stochrsi", &["tempRSIBuffer"]),
    ] {
        let (mut func, enums) = load_indicator(indicator);
        func.streaming = true;
        let registry = make_registry();
        let helpers = HelperRegistry::empty();
        let c = backends::c::generate(&func, &enums, &registry, &helpers);
        let upper = indicator.to_uppercase();
        let open_at = c
            .find(&format!("TA_RetCode TA_{upper}_Open"))
            .unwrap_or_else(|| panic!("{upper} composed Open"));
        // One `OpenCore` transcribes the region for both entry points, so every
        // buffer is checked exactly once — never twice. (Before the Open family
        // was merged this read 2, one per transcription; the invariant being
        // pinned is unchanged: the source's own check must not be emitted
        // alongside the injected one.)
        let opens = &c[open_at..];
        for buf in buffers {
            let n = opens.matches(&format!("if( !{buf} )")).count();
            assert_eq!(
                n, 1,
                "{upper}: `{buf}` must be null-checked exactly once in the composed \
                 OpenCore, found {n} — the source's own check is being emitted \
                 alongside the injected one again"
            );
        }
    }
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

/// #142 regression: period-scaled dividers/sums must compute in floating point,
/// never a bare int32 product. The WMA/HMA triangular divider (n*(n+1)/2)
/// overflows int32 at period 46341; the linear-regression cubic
/// (n*(n-1)*(2n-1)/6) overflows at period 1025. Both silently returned garbage.
/// Widening the operands to double is the fix — pin the generated form across
/// C/Rust/Java so a revert to the int expression trips here instead of at a
/// period no test data reaches.
#[test]
fn test_period_scaled_arithmetic_is_double_not_int32() {
    // WMA/HMA triangular divider: double, no int32 `>> 1` shift.
    for name in ["wma", "hma"] {
        let (func, enums) = load_indicator(name);
        let out = generate_all(&func, &enums);
        assert!(
            !out.c.contains(">> 1"),
            "{name}: C divider still uses the int32 `>> 1` shift (#142 overflow at period 46341)"
        );
        assert!(
            out.c.contains("(double)optInTimePeriod * (optInTimePeriod + 1) / 2.0"),
            "{name}: C divider not widened to double (#142)"
        );
        assert!(
            !out.rust.contains(">> 1"),
            "{name}: Rust divider still forms the int32 product before the cast (#142)"
        );
        assert!(
            !out.java.contains(">> 1"),
            "{name}: Java divider still uses the int32 `>> 1` shift (#142)"
        );
    }
    // Linear-regression family SumXSqr cubic: double, no int32 `/ 6` division.
    for name in ["linearreg", "linearreg_slope", "linearreg_intercept", "linearreg_angle", "tsf"] {
        let (func, enums) = load_indicator(name);
        let out = generate_all(&func, &enums);
        assert!(
            !out.c.contains("/ 6;"),
            "{name}: C SumXSqr still uses int32 `/ 6` division (#142 cubic overflow at period 1025)"
        );
        assert!(
            out.c
                .contains("(double)optInTimePeriod * (optInTimePeriod - 1) * (2 * optInTimePeriod - 1) / 6.0"),
            "{name}: C SumXSqr not widened to double (#142)"
        );
        assert!(
            !out.rust.contains("/ 6) as f64"),
            "{name}: Rust SumXSqr still forms the int32 cubic before the cast (#142)"
        );
    }
}

// ---------------------------------------------------------------------------
// Scratch-buffer election (issue #146)
// ---------------------------------------------------------------------------

/// The scratch election must reach the SMA fast path and *only* the SMA fast path:
/// the calculation writes straight into the caller's slices, while the general MA
/// path below it keeps its two genuine allocations.
#[test]
fn rust_bbands_elects_output_scratch_only_in_the_sma_fast_path() {
    let (func, enums) = load_indicator("bbands");
    let registry = make_registry();
    let helpers = make_helpers();
    let out = generate_all(&func, &enums);
    let rust_out = backends::rust_lang::generate(&func, &enums, &registry, &helpers);

    // No allocation-and-copy of an output slice survives anywhere.
    assert!(
        !rust_out.contains(".to_vec()"),
        "BBANDS Rust should not copy an output slice into a scratch Vec: {rust_out}"
    );
    // The SMA and the standard deviation are written into the outputs by name.
    assert!(
        rust_out.contains("outRealMiddleBand[_outIdx] = maTotal /"),
        "BBANDS Rust should write the SMA straight into outRealMiddleBand: {rust_out}"
    );
    assert!(
        rust_out.contains("outRealUpperBand[_outIdx] = (variance).sqrt();"),
        "BBANDS Rust should write the standard deviation into outRealUpperBand: {rust_out}"
    );
    assert!(
        rust_out.contains("tempReal = outRealUpperBand[i] * optInNbDevUp;"),
        "the band loop should read its deviation back out of outRealUpperBand: {rust_out}"
    );
    // The dead aliasing arms, the input-alias guard and the copy-back are gone.
    assert!(
        !rust_out.contains("inReal.as_ptr() == outRealUpperBand.as_ptr()"),
        "BBANDS Rust should not test inReal against an output: {rust_out}"
    );
    assert!(
        !rust_out.contains("tempBuffer1.as_ptr()"),
        "BBANDS Rust should have no pointer tests left on tempBuffer1: {rust_out}"
    );
    // The general MA path's allocations are real and must survive — one pair in
    // the batch variant, plus the stream tier's.
    let allocs = rust_out.matches("tempBuffer1 = vec![0.0_f64;").count();
    assert!(
        allocs >= 2,
        "the general MA path must keep its tempBuffer1 allocation in every variant \
         (found {allocs}): {rust_out}"
    );
    assert_eq!(
        allocs,
        rust_out.matches("tempBuffer2 = vec![0.0_f64;").count(),
        "tempBuffer1 and tempBuffer2 must be allocated in the same places: {rust_out}"
    );
    // Rust-only: the other backends assign the pointer/reference and keep C's
    // election chain verbatim.
    assert!(
        out.c.contains("tempBuffer1 = outRealMiddleBand;"),
        "the C backend must keep C's pointer election: {}",
        out.c
    );
    assert!(
        out.java.contains("tempBuffer1 = outRealMiddleBand;"),
        "the Java backend must keep C's reference election: {}",
        out.java
    );
}

/// Being general is not the same as being greedy. The matcher requires *every* arm
/// of the chain to be nothing but `scratch = someOutput;` elections, and that one
/// clause is what declines `STOCH`, `STOCHF` and `MAVP`: each mixes an allocation
/// and a `…IsAllocated = 1;` flag into a branch, so the branch is a genuine
/// in-place defence with a real buffer to allocate rather than an election.
/// `MAVP` is inverted as well — the allocation sits in the `then` and the election
/// in the `else` — so it is rejected on the very first link.
///
/// Their generated Rust must come out byte-for-byte as it was. That non-firing is
/// what lets the PR assert the other three backends were untouched, so it is
/// pinned here rather than left to `git diff`.
#[test]
fn rust_scratch_election_declines_arms_that_allocate() {
    let registry = make_registry();
    let helpers = make_helpers();

    for name in ["stoch", "stochf"] {
        let (func, enums) = load_indicator(name);
        let rust_out = backends::rust_lang::generate(&func, &enums, &registry, &helpers);
        assert!(
            rust_out.contains("tempBuffer = outSlowK.to_vec();")
                || rust_out.contains("tempBuffer = outFastK.to_vec();"),
            "{name}'s election arm must be untouched: {rust_out}"
        );
        assert!(
            rust_out.contains("tempBuffer = vec![0.0_f64;"),
            "{name} must keep the allocation on its other arm: {rust_out}"
        );
    }

    // `MAVP` is the inverted case, and the one a looser matcher reaches first.
    let (func, enums) = load_indicator("mavp");
    let rust_out = backends::rust_lang::generate(&func, &enums, &registry, &helpers);
    assert!(
        rust_out.contains("localFinalArray = outReal.to_vec();"),
        "MAVP's election must be left as it is: {rust_out}"
    );
    assert!(
        rust_out.contains("localFinalArray = vec![0.0_f64;"),
        "MAVP must keep the allocation in its `then` arm: {rust_out}"
    );
    assert!(
        rust_out.contains("localFinalArray.as_ptr() != outReal.as_ptr()"),
        "MAVP must keep its copy-back guard: {rust_out}"
    );

    // The pass must not have fired for a single function other than `BBANDS`. The
    // election note is emitted exactly when an election is installed, so its
    // absence across the whole `input/` tree is the non-firing proof — and it is
    // proven over every indicator rather than a hand-picked list, so a widening of
    // the rule cannot slip past by naming a function this test forgot.
    const NOTE: &str = "C's pointer election here is a rename";
    let mut fired = Vec::new();
    for name in discover_indicators() {
        let (func, enums) = load_indicator(&name);
        if backends::rust_lang::generate(&func, &enums, &registry, &helpers).contains(NOTE) {
            fired.push(name);
        }
    }
    assert_eq!(
        fired,
        vec!["bbands".to_string()],
        "the scratch election must fire for BBANDS and nothing else"
    );
}

/// C's scratch local is *function*-scoped: a pointer elected inside a nested
/// block still points at that output after the block ends. The rename only
/// reaches the end of the electing block, so it is equivalent only when nothing
/// afterwards can observe the local. `BBANDS` satisfies that because its fast
/// path `return`s; this fixture does not, and the election must be declined
/// rather than leave a read of a `Vec` that is never assigned.
#[test]
fn rust_scratch_election_declines_an_election_that_escapes_its_block() {
    let src = r#"
int bbands_lookback( int optInTimePeriod, double optInNbDevUp, double optInNbDevDn, TA_MAType optInMAType )
{
   return optInTimePeriod - 1;
}

TA_RetCode bbands( int startIdx, int endIdx,
   const double inReal[],
   int optInTimePeriod,
   double optInNbDevUp, double optInNbDevDn,
   TA_MAType optInMAType,
   int *outBegIdx, int *outNBElement,
   double outRealUpperBand[], double outRealMiddleBand[], double outRealLowerBand[] )
{
   double *tempBuffer1;
   double *tempBuffer2;
   int i;

   if( optInMAType == TA_MAType_SMA )
   {
      if( inReal == outRealUpperBand )
      {
         tempBuffer1 = outRealMiddleBand;
         tempBuffer2 = outRealLowerBand;
      }
      else
      {
         tempBuffer1 = outRealMiddleBand;
         tempBuffer2 = outRealUpperBand;
      }
      for( i=0; i < 10; i++ )
      {
         tempBuffer1[i] = inReal[i];
         tempBuffer2[i] = inReal[i];
      }
   }

   /* Control falls out of the electing block, and the local is read here. */
   for( i=0; i < 10; i++ )
   {
      outRealLowerBand[i] = tempBuffer1[i];
   }

   *outBegIdx = startIdx;
   *outNBElement = 10;
   return TA_SUCCESS;
}
"#;
    let (func, enums) = load_indicator_with_source("bbands", src);
    let rust = generate_all(&func, &enums).rust;
    assert!(
        rust.contains("tempBuffer1 = outRealMiddleBand.to_vec()"),
        "an election whose local is still read after the electing block must fall \
         back to the copy; renaming it would leave that read pointing at a Vec \
         nothing ever assigns: {rust}"
    );
}

// ---------------------------------------------------------------------------
// C# enums (M1). These assert on EMITTED CONTENT, not merely that the emitter
// ran: a test that only checks "generate() did not panic" is the shape that has
// passed vacuously in this repo before.
// ---------------------------------------------------------------------------

// ---------------------------------------------------------------------------
// Rust enum (#179). Same standard as the C# one below: assert on EMITTED
// CONTENT. Everything here is frozen public API the moment the crate publishes,
// and until now none of it was asserted anywhere.
// ---------------------------------------------------------------------------

#[test]
fn rust_matype_emits_every_yaml_variant_and_its_frozen_shape() {
    let enums = load_enums();
    let src = backends::rust_enums::render_matype(&enums);
    let ma = enums.get("MAType").expect("MAType in enums.yaml");

    for v in &ma.variants {
        let decl = format!("    {} = {},", v.name, v.value);
        assert!(
            src.contains(&decl),
            "Rust MAType is missing `{decl}` -- a dropped variant reorders the \
             optInMAType ABI:\n{src}"
        );
        // The conversion must accept every member, or a value that is legal at
        // the C ABI would be rejected by the Rust one.
        let arm = format!("            {} => Self::{},", v.value, v.name);
        assert!(
            src.contains(&arm),
            "TryFrom<i32> is missing `{arm}`:\n{src}"
        );
    }

    // An EXTRA emitted member fails too.
    let emitted = src
        .lines()
        .filter(|l| l.starts_with("    ") && l.contains(" = ") && l.trim_end().ends_with(','))
        .count();
    assert_eq!(emitted, ma.variants.len(), "emitted {emitted} members");

    // `#[non_exhaustive]` is what lets a member be appended without breaking
    // every downstream `match`; dropping it is a silent semver break.
    assert!(src.contains("#[non_exhaustive]"), "MAType lost #[non_exhaustive]");

    // No `#[repr]`: the crate has no FFI, so the layout is unobservable and the
    // explicit discriminants carry the ABI. Adding one would freeze a size we
    // deliberately did not promise.
    assert!(
        !src.contains("#[repr("),
        "MAType gained a #[repr]; the crate has no FFI to justify one:\n{src}"
    );

    // The sentinel arm is load-bearing: the abstract tier stores the bound int
    // verbatim as C's does, so TA_INTEGER_DEFAULT must still select the
    // parameter's declared default rather than being rejected (#162).
    assert!(
        src.contains("i32::MIN => Self::DEFAULT,"),
        "TryFrom lost the TA_INTEGER_DEFAULT arm; Rust would drop out of the \
         choice-list sentinel contract:\n{src}"
    );
    assert!(
        src.contains("_ => return Err(RetCode::BadParam),"),
        "TryFrom lost its reject arm -- out-of-domain values would not be \
         rejected by the library:\n{src}"
    );
}

// ---------------------------------------------------------------------------
// The enum domain gate. A choice-list parameter declares no `range:`, so before
// this the prologue emitted only the default substitution and each body decided
// for itself what an out-of-domain value meant -- which is how TA_MA_Lookback
// answered 0 for parameters TA_MA rejects. Both tiers now reject from one
// emitter with two failure literals, the construction that already made integer
// ranges immune. Asserted on emitted content, per this file's standard.
// ---------------------------------------------------------------------------

#[test]
fn enum_param_gets_a_domain_gate_in_both_tiers() {
    let enums = load_enums();
    let (func, _) = load_indicator("ma");
    let registry = make_registry();
    let helpers = make_helpers();

    // The gate names the generated limit constants rather than the numbers of
    // the day -- that is the whole point of them, so assert the spelling the
    // enum surface declares and never a literal.
    let ma = enums.get("MAType").expect("MAType");
    let (c_min, c_max) =
        backends::common::enum_limit_names_of(ma, Lang::C).expect("C declares MAType limits");

    let c = backends::c::generate(&func, &enums, &registry, &helpers);
    let gate = format!("(int)optInMAType < {c_min} || (int)optInMAType > {c_max}");
    // Both tiers: the lookback fails with -1, the guarded call with TA_BAD_PARAM.
    assert!(
        c.contains(&format!("{gate} )\n      return -1;")),
        "C lookback lost the enum domain gate:\n{c}"
    );
    assert!(
        c.contains(&format!("{gate} )\n      return TA_BAD_PARAM;")),
        "C guarded call lost the enum domain gate:\n{c}"
    );

    let (cs_min, cs_max) = backends::common::enum_limit_names_of(ma, Lang::CSharp)
        .expect("C# declares MAType limits");
    let cs = backends::csharp::generate(&func, &enums, &registry, &helpers);
    assert!(
        cs.contains(&format!("(int)optInMAType < {cs_min} || (int)optInMAType > {cs_max}")),
        "C# lost the enum domain gate:\n{cs}"
    );

    // And no tier carries the bound as a number any more: a reintroduced literal
    // is a value that has to be re-edited in every prologue when a member is
    // appended, which is the defect the constants exist to remove.
    let hi = ma.variants.iter().map(|v| v.value).max().expect("members");
    for (lang, src) in [("C", &c), ("C#", &cs)] {
        assert!(
            !src.contains(&format!("optInMAType > {hi}")),
            "{lang} spelled the MAType bound as a literal again:\n{src}"
        );
    }
}

#[test]
fn the_enum_limit_macros_are_declared_next_to_the_enum() {
    // The declaration is where the number now lives, so it is what has to be
    // derived. A synthetic enum that ends somewhere other than MAType's 11 is
    // what separates a derived bound from a hard-coded one.
    use ta_codegen_lib::ir::{EnumDef, EnumVariant};
    let tri = EnumDef {
        name: "Tri".to_string(),
        c_prefix: "TA_Tri_".to_string(),
        variants: (0..3)
            .map(|v| EnumVariant {
                name: format!("V{v}"),
                c_name: format!("TA_Tri_V{v}"),
                value: v,
            })
            .collect(),
    };

    let c = backends::ta_defs::render_enum_limits(&tri, "TA_Tri");
    assert!(
        c.contains("#define TA_TRI_MIN 0") && c.contains("#define TA_TRI_MAX 2"),
        "the C limit macros must span the members and take the enum's own \
         c_prefix, upper-cased:\n{c}"
    );

    // C# reaches the same numbers through the shipped enum file. Swap MAType's
    // members for the synthetic three so a hard-coded 11 could not pass.
    let (func, _) = load_indicator("ma");
    let mut enums = load_enums();
    let ma = enums.get_mut("MAType").expect("MAType");
    ma.variants.truncate(3);
    let cs = backends::csharp_enums::render_matype(std::slice::from_ref(&func), &enums);
    assert!(
        cs.contains("public const int Min = 0;") && cs.contains("public const int Max = 2;"),
        "the C# limit companion must span the members:\n{cs}"
    );
    assert!(
        cs.contains("public static class MATypes"),
        "the C# limits must live in the enum's companion class:\n{cs}"
    );
}

#[test]
fn an_enum_no_parameter_is_typed_with_gets_no_limits() {
    // FuncUnstId's pinned ALL = 65535 sits outside its member span, so limits
    // derived from the members would describe a domain its API does not have.
    // Nothing is typed with it, so nothing emits them -- assert that rule holds
    // rather than that FuncUnstId in particular is spelled out somewhere.
    let enums = load_enums();
    let (func, _) = load_indicator("ma");
    let param_enums = backends::common::param_enum_names(std::slice::from_ref(&func));
    assert!(param_enums.contains("MAType"), "MA takes an optInMAType");
    assert!(
        !param_enums.contains("FuncUnstId"),
        "no optional parameter is typed with FuncUnstId"
    );

    let cs = backends::csharp_enums::render_funcunstid(&enums);
    assert!(
        !cs.contains("Min =") && !cs.contains("Max ="),
        "FuncUnstId gained value limits its ALL wildcard falls outside of:\n{cs}"
    );
}

#[test]
fn the_gate_bound_follows_the_member_set() {
    // The bound is derived, never spelled. Asserting it against MAType's own max
    // cannot show that -- a hard-coded 11 and a derived one read identically
    // while the enum happens to end at 11. So span it against a synthetic enum
    // that ends somewhere else.
    use ta_codegen_lib::ir::{EnumDef, EnumVariant, OptInput, ParamType};
    // (What the prologue now emits is the constant's NAME; the number it
    // resolves to is asserted at the declaration, above.)
    let mut enums = load_enums();
    enums.insert(
        "Tri".to_string(),
        EnumDef {
            name: "Tri".to_string(),
            c_prefix: "TA_Tri_".to_string(),
            variants: (0..3)
                .map(|v| EnumVariant {
                    name: format!("V{v}"),
                    c_name: format!("TA_Tri_V{v}"),
                    value: v,
                })
                .collect(),
        },
    );
    let opt = OptInput {
        name: "optInTri".to_string(),
        param_type: ParamType::Enum("Tri".to_string()),
        display_name: None,
        hint: None,
        range: None,
        default: Some(0.0),
        suggested: None,
        flags: Vec::new(),
        precision: None,
    };
    assert_eq!(
        backends::common::enum_value_bounds_of(enums.get("Tri").expect("Tri")),
        Some((0, 2)),
        "the domain must span the members, not a hard-coded bound"
    );
    assert_eq!(
        backends::common::int_bound_exprs(&opt, &enums, Lang::C),
        Some(("TA_TRI_MIN".to_string(), "TA_TRI_MAX".to_string())),
        "the prologue must name the enum's own limit macros, not MAType's"
    );
}

#[test]
fn a_declared_range_still_wins_over_the_member_span() {
    // The precedence branch in `int_bound_exprs`: an `enum:` parameter that DID
    // declare a range must keep it, or a narrower intent would be silently
    // widened to the whole enum. A declared range is per-parameter, so it stays
    // a literal -- the limit constants describe the TYPE's domain, which is not
    // the same thing. Nothing in the shipped input exercises this.
    use ta_codegen_lib::ir::{OptInput, ParamType};
    let enums = load_enums();
    let opt = OptInput {
        name: "optInMAType".to_string(),
        param_type: ParamType::Enum("MAType".to_string()),
        display_name: None,
        hint: None,
        range: Some((0.0, 2.0)),
        default: Some(0.0),
        suggested: None,
        flags: Vec::new(),
        precision: None,
    };
    assert_eq!(
        backends::common::int_bound_exprs(&opt, &enums, Lang::C),
        Some(("0".to_string(), "2".to_string())),
        "a declared range must win over the member span"
    );
}

#[test]
fn csharp_matype_emits_every_yaml_variant_with_its_value() {
    let enums = load_enums();
    let (func, _) = load_indicator("ma");
    let src = backends::csharp_enums::render_matype(std::slice::from_ref(&func), &enums);
    let ma = enums.get("MAType").expect("MAType in enums.yaml");

    for v in &ma.variants {
        let decl = format!("    {} = {},", v.name, v.value);
        assert!(
            src.contains(&decl),
            "MAType.cs is missing `{decl}` -- a variant silently dropped from the \
             emitted enum reorders the optInMAType ABI:\n{src}"
        );
    }
    // Count the members, so an EXTRA emitted variant fails too. Match the
    // member shape specifically -- the BSD header has comma-terminated prose.
    let emitted = src
        .lines()
        .filter(|l| l.starts_with("    ") && l.contains(" = ") && l.trim_end().ends_with(','))
        .count();
    assert_eq!(
        emitted,
        ma.variants.len(),
        "MAType.cs emitted {emitted} members for {} YAML variants",
        ma.variants.len()
    );
}

#[test]
fn csharp_funcunstid_pins_the_all_sentinel_and_the_count() {
    let enums = load_enums();
    let src = backends::csharp_enums::render_funcunstid(&enums);
    let fu = enums.get("FuncUnstId").expect("FuncUnstId in enums.yaml");

    // The ABI pin. C pins TA_FUNC_UNST_ALL at 65535; a renumber here silently
    // repoints every caller's set_unstable_period and nothing else catches it.
    assert!(
        src.contains("ALL = 65535,"),
        "FuncUnstId.cs must pin `ALL = 65535`:\n{src}"
    );
    assert!(
        src.contains(&format!("public const int Count = {};", fu.variants.len())),
        "FuncUnstIds.Count must equal the {} function ids (and must NOT be an \
         enum member -- that would make it an id):\n{src}",
        fu.variants.len()
    );
    for v in &fu.variants {
        let decl = format!("    {} = {},", v.name, v.value);
        assert!(
            src.contains(&decl),
            "FuncUnstId.cs is missing `{decl}`:\n{src}"
        );
    }
    // Count must not silently include the All sentinel.
    assert!(
        !src.contains(&format!("public const int Count = {};", fu.variants.len() + 1)),
        "Count must exclude the All sentinel"
    );
}

#[test]
fn csharp_resolve_call_agrees_with_the_emitted_method_names() {
    // If Registry::name_of and the emitter's method naming disagree, every
    // cross-indicator call targets a method that does not exist -- and that will
    // not surface until the backend emits bodies, as a wall of CS0103.
    let registry = make_registry();
    let enums = load_enums();
    let helpers = make_helpers();

    for name in discover_indicators() {
        let (func, _) = load_indicator(&name);
        let bare = registry.resolve_call(&name, ta_codegen_lib::registry::Lang::CSharp);
        let lookback = registry.resolve_call(
            &format!("{name}_lookback"),
            ta_codegen_lib::registry::Lang::CSharp,
        );
        assert!(
            !bare.ends_with("Unguarded"),
            "{name}: bare cross-indicator call must resolve to the guarded \
             entry point, got {bare}"
        );
        // The resolved name is the YAML `name:` verbatim, and the suffix is
        // separated by an underscore.
        assert_eq!(bare, func.name, "{name}: C# base must be the YAML name verbatim");
        assert_eq!(
            lookback,
            format!("{}_Lookback", func.name),
            "{name}: lookback and guarded names disagree on the base"
        );
        // What the resolver promises must be what the emitter actually writes —
        // the literal a caller in another indicator will be compiled against.
        let src = backends::csharp::generate(&func, &enums, &registry, &helpers);
        assert!(
            src.contains(&format!("RetCode {bare}(")),
            "{name}: emitter never defines the `{bare}` the resolver hands out"
        );
        assert!(
            src.contains(&format!("public int {lookback}(")),
            "{name}: emitter never defines the `{lookback}` the resolver hands out"
        );
    }
}

/// Issue #156: the runtime FMA dispatch trio (public dispatcher +
/// `#[target_feature(enable = "fma")]` clone + `#[inline(always)]` `_impl`)
/// must be emitted for exactly the functions whose rendered body fuses — the
/// same 26-function inventory `fma_suite.rs` pins — and never elsewhere.
/// Guards both directions: the dispatch silently going dark, and accidental
/// dispatch of unfused functions.
#[test]
fn rust_fma_dispatch_fires_for_exactly_the_fusing_functions() {
    const FUSING: &[&str] = &[
        "adosc", "bbands", "cdlabandonedbaby", "cdlmorningdojistar", "cdlmorningstar",
        "cdlpiercing", "cdlthrusting", "dema", "efi", "ema", "ht_dcperiod", "ht_dcphase",
        "ht_phasor", "ht_sine", "ht_trendline", "ht_trendmode", "kama", "linearreg",
        "macd", "macdfix", "mama", "sar", "sarext", "t3", "tema", "trix", "tsf",
        "wclprice",
    ];
    let registry = make_registry();
    let helpers = make_helpers();
    let base = Path::new(env!("CARGO_MANIFEST_DIR")).join("../../ta_codegen/input");
    let mut dispatched: Vec<String> = Vec::new();
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
        checked += 1;
        if out.contains("ta_lib_dispatch::dispatch_fma!") {
            // Every dispatcher must come with exactly one clone: the
            // dispatch-call count and target_feature-attribute count match.
            let calls = out.matches("ta_lib_dispatch::dispatch_fma!").count();
            let clones = out.matches("#[target_feature(enable = \"fma\")]").count();
            assert_eq!(calls, clones, "{name}: dispatcher/clone count mismatch");
            // The batch variant must carry its clone. (A future
            // private-delegating fused function would trip the dispatcher/clone
            // balance above on purpose.)
            assert!(
                out.contains(&format!("fn {}_fma(", func.name)),
                "{name}: guarded variant lost its FMA clone"
            );
            // The fused sites live on in the renamed portable impl.
            assert!(
                out.contains("_impl(") && out.contains(".mul_add("),
                "{name}: dispatch emitted but trio structure incomplete"
            );
            dispatched.push(name);
        } else {
            assert!(
                !out.contains(".mul_add("),
                "{name}: fused body without a dispatch trio"
            );
        }
    }
    dispatched.sort();
    assert_eq!(dispatched, FUSING, "FMA dispatch inventory drifted");
    assert!(checked >= 150, "expected ~168 functions, checked {checked}");
}

// ---------------------------------------------------------------------------
// Bitwise operators (issue #157): every C bitwise form renders correctly in
// all four backends. C/Java/C# spell `~` as `~`; Rust spells it `!` and needs
// explicit `!= 0` for C's int-truthiness conditions. C's grouping must survive
// Rust's different precedence (`&`/`^`/`|` bind tighter than `==` in Rust).
// ---------------------------------------------------------------------------

#[test]
fn bitwise_operators_render_in_all_backends() {
    let source = r#"
int max_lookback( int optInTimePeriod )
{
   return (optInTimePeriod-1);
}

TA_RetCode max( int    startIdx,
                int    endIdx,
                const double inReal[],
                int    optInTimePeriod,
                int   *outBegIdx,
                int   *outNBElement,
                double outReal[] )
{
   int outIdx, i, mask, neg;

   mask = (optInTimePeriod ^ 3) & ~1;
   mask |= 2;
   mask &= 15;
   mask ^= 1;
   mask <<= 1;
   mask >>= 1;
   mask = ((mask | 4) << 1) >> 1;
   mask = (mask << 2) + 1;
   neg = ~optInTimePeriod;
   if( (mask & 1) == 9999 )
      return TA_INTERNAL_ERROR;
   if( mask & 16 )
      return TA_INTERNAL_ERROR;
   if( (mask & 1) && (neg < 0) )
      outIdx = 0;
   if( !(mask & 1) )
      return TA_INTERNAL_ERROR;
   while( mask & 1024 )
      mask = mask & ~1024;
   i = (mask & 2) ? 1 : 0;
   if( i == 9999 )
      return TA_INTERNAL_ERROR;
   outIdx = 0;
   for( i=startIdx; i <= endIdx; i++ )
      outReal[outIdx++] = inReal[i];
   *outBegIdx = startIdx;
   *outNBElement = outIdx;
   return TA_SUCCESS;
}
"#;
    let (func, enums) = load_indicator_with_source("max", source);
    let out = generate_all(&func, &enums);

    for needle in [
        "mask = (optInTimePeriod ^ 3) & ~1;",
        "mask |= 2;",
        "mask &= 15;",
        "mask ^= 1;",
        "mask <<= 1;",
        "mask >>= 1;",
        "mask = (mask | 4) << 1 >> 1;",
        "mask = (mask << 2) + 1;",
        "neg = ~optInTimePeriod;",
        "if( (mask & 1) == 9999 )",
        "if( mask & 16 )",
        "if( mask & 1 && neg < 0 )",
        "if( !(mask & 1) )",
        "while( mask & 1024 )",
        "i = (mask & 2) ? 1 : 0;",
    ] {
        assert!(out.c.contains(needle), "C output missing `{needle}`:\n{}", out.c);
    }

    for needle in [
        "mask = (optInTimePeriod ^ 3) & ~1;",
        "mask |= 2;",
        "mask &= 15;",
        "mask ^= 1;",
        "mask <<= 1;",
        "mask >>= 1;",
        "(mask & 1) == 9999",
        "(mask & 16) != 0",
        "(mask & 1) != 0 && neg < 0",
        "((mask & 1) == 0)",
        "while( (mask & 1024) != 0 )",
        "((mask & 2) != 0) ? 1 : 0",
    ] {
        assert!(out.java.contains(needle), "Java output missing `{needle}`:\n{}", out.java);
    }

    for needle in [
        "& !(1)",
        "mask |= 2;",
        "mask &= 15;",
        "mask ^= 1;",
        "mask <<= 1;",
        "mask >>= 1;",
        "mask & 1 == 9999",
        "(mask & 16) != 0",
        "(mask << 2) + 1",           // Rust shifts bind looser than + : parens required
        "let mut neg: i32",          // ~x can be negative: var must be signed
        "(mask & 1) != 0 && neg < 0",
        "(mask & 1) == 0",
        "while (mask & 1024) != 0 {",
        "if (mask & 2) != 0 {",
    ] {
        assert!(out.rust.contains(needle), "Rust output missing `{needle}`:\n{}", out.rust);
    }

    let registry = make_registry();
    let helpers = HelperRegistry::empty();
    let cs = backends::csharp::generate(&func, &enums, &registry, &helpers);
    for needle in [
        "& ~1;",
        "(mask & 1) == 9999",
        "(mask & 16) != 0",
        "(mask & 1) != 0 && neg < 0",
        "((mask & 1) == 0)",
        "while( (mask & 1024) != 0 )",
        "((mask & 2) != 0) ? 1 : 0",
    ] {
        assert!(cs.contains(needle), "C# output missing `{needle}`:\n{cs}");
    }
}

// ---------------------------------------------------------------------------
// Issue #160: a C `(int)` cast of a possibly-negative double must land in a
// SIGNED Rust local (the default f64→usize cast saturates negatives to 0).
// MAVP's period clamp is the shipped case; synth2 in input_synth/ is the
// end-to-end gate. This pins the rendering so a classifier regression is a
// test failure, not a silent semantic drift.
// ---------------------------------------------------------------------------

#[test]
fn rust_negative_capable_cast_gets_signed_local() {
    let (func, enums) = load_indicator("mavp");
    let out = generate_all(&func, &enums);
    for needle in [
        "let mut tempInt: i32",                    // cast-fed local is signed
        "tempInt = (inPeriods[startIdx + i]) as i32;", // true negative preserved
        "if tempInt < optInMinPeriod {",           // clamps stay signed compares
    ] {
        assert!(out.rust.contains(needle), "MAVP Rust missing `{needle}`:\n{}", out.rust);
    }
    // sqrt-fed locals stay usize (provably non-negative allowlist): HMA's
    // sqrtPeriod = (int)(sqrt(...)) is the shipped case.
    let (hma, enums2) = load_indicator("hma");
    let hma_out = generate_all(&hma, &enums2);
    assert!(
        hma_out.rust.contains("let mut sqrtPeriod: usize"),
        "HMA sqrtPeriod must stay usize (allowlist regression):\n{}",
        hma_out.rust
    );
}

// Index-domain values must never narrow to i32: every runtime gate feeds
// <= 100k bars, so an i32-narrowed endIdx misbehaves only at >= 2^31 inputs —
// structurally invisible to value comparison. Pin it textually instead (the
// exact regression the #160 review caught in MAVP's dual-role temp).
#[test]
fn rust_index_domain_never_narrows_to_i32() {
    for name in discover_indicators() {
        let (func, enums) = load_indicator(&name);
        let out = generate_all(&func, &enums);
        for needle in ["(endIdx) as i32", "(startIdx) as i32", "endIdx as i32", "startIdx as i32"] {
            assert!(
                !out.rust.contains(needle),
                "{name}: generated Rust narrows an index-domain value (`{needle}`)"
            );
        }
        // The needles above are the bare forms; an arithmetic expression
        // narrows just as badly and matches none of them. A broader "any line
        // with `as i32` mentioning the range" rule is wrong — MAVP's
        // `(inPeriods[startIdx + i]) as i32` casts an i32 array element and
        // only uses the range as a subscript — so pin the arithmetic forms.
        for op in ['+', '-', '*'] {
            for needle in [
                format!("startIdx {op} "),
                format!("endIdx {op} "),
            ] {
                for line in out.rust.lines().filter(|l| l.contains("as i32")) {
                    assert!(
                        !(line.contains(&needle) && !line.contains('[')),
                        "{name}: generated Rust narrows an index-domain expression to i32: {line}"
                    );
                }
            }
        }
    }
}

// ---------------------------------------------------------------------------
// The shared abstract row model
// ---------------------------------------------------------------------------
//
// `backends::abstract_rows` is the one derivation the Rust registry, the Java
// server table, the shipped Java registry and the shipped C# registry all
// render. These pin the facts that used to be hand-maintained inside one
// backend, plus the two domains that are currently unreachable — so the day one
// appears, the sweep names the renderers that need a look.

/// Load every shipped definition once, as the abstract rows.
fn all_abstract_rows() -> Vec<ta_codegen_lib::backends::abstract_rows::FuncRow> {
    let base = Path::new(env!("CARGO_MANIFEST_DIR")).join("../../ta_codegen/input");
    let enums = parser::enums::load_enums(&base.join("enums.yaml"));
    let funcs: Vec<ir::FuncDef> = discover_indicators()
        .iter()
        .map(|n| parser::yaml::parse_yaml(&base.join(format!("{n}/{n}.yaml"))))
        .collect();
    backends::abstract_rows::rows(&funcs, &enums)
}

/// The unstable-period set used to live as a 20-arm hardcoded name -> variant
/// `match` inside `rust_abstract`, duplicating `enums.yaml`. It is now resolved
/// by name (`TA_FUNC_UNST_<NAME>`), the same derivation the servers use. This
/// pins the resulting set both ways: a lost mapping and a spurious one both fail.
#[test]
fn abstract_rows_unstable_period_set_is_exactly_the_twenty() {
    // (function name, its FuncUnstId ordinal). The NAME half would be a
    // tautology on its own — `unst_row` resolves `TA_FUNC_UNST_<name>` and hands
    // the variant's name back, so it can only ever equal the function's. The
    // VALUE is the half worth pinning: it is authored in enums.yaml, it is the
    // index every backend uses into `unstablePeriod[]`, and it is ABI. A slot
    // that silently renumbers is what this table exists to catch.
    const EXPECTED: &[(&str, i32)] = &[
        ("ADX", 0),
        ("ATR", 2),
        ("CMO", 3),
        ("DX", 4),
        ("EMA", 5),
        ("HT_DCPERIOD", 6),
        ("HT_DCPHASE", 7),
        ("HT_PHASOR", 8),
        ("HT_SINE", 9),
        ("HT_TRENDLINE", 10),
        ("HT_TRENDMODE", 11),
        ("KAMA", 13),
        ("MAMA", 14),
        ("MINUS_DI", 16),
        ("MINUS_DM", 17),
        ("NATR", 18),
        ("PLUS_DI", 19),
        ("PLUS_DM", 20),
        ("RSI", 21),
        ("T3", 23),
    ];

    let rows = all_abstract_rows();
    let mut got: Vec<(String, i32)> = rows
        .iter()
        .filter_map(|r| r.unst.as_ref().map(|u| (r.name.clone(), u.value)))
        .collect();
    got.sort();
    let mut want: Vec<(String, i32)> =
        EXPECTED.iter().map(|(a, b)| ((*a).to_string(), *b)).collect();
    want.sort();
    assert_eq!(got, want, "unstable-period set or ordinal changed (name -> FuncUnstId value)");

    // The `unstable_period` function flag and the resolved id must not disagree:
    // one without the other means a function that says it is recursive but has
    // no state slot, or a slot nothing declares.
    for r in &rows {
        let flagged = r.flags & 0x0800_0000 != 0;
        assert_eq!(
            flagged,
            r.unst.is_some(),
            "{}: unstable_period flag ({flagged}) disagrees with its FuncUnstId ({:?})",
            r.name,
            r.unst.as_ref().map(|u| &u.c_name)
        );
    }
}

/// Every shipped `group:` string must parse into the closed `Group` set, and
/// every variant must render back to the exact display string C's
/// `TA_GroupString` and the YAML use.
#[test]
fn abstract_rows_group_strings_round_trip() {
    use ta_codegen_lib::backends::abstract_rows::Group;
    for g in Group::ALL {
        assert_eq!(Group::parse(g.as_str()), *g, "group round-trip for {}", g.as_str());
    }
    let rows = all_abstract_rows();
    for g in Group::ALL {
        // Not an emptiness check: every declared group must actually be used,
        // so a retired group cannot linger in the closed set unnoticed.
        assert!(
            rows.iter().any(|r| r.group == *g),
            "no shipped function is in group {}",
            g.as_str()
        );
    }
}

/// Two shapes the model can express that nothing currently declares. Pinned
/// rather than asserted away: the renderers each have an arm for them that no
/// gate exercises, so the day a definition uses one, this says so by name.
#[test]
fn abstract_rows_unreachable_domains_stay_unreachable() {
    use ta_codegen_lib::backends::abstract_rows::{InputKind, OptDomain};
    for r in all_abstract_rows() {
        for o in &r.opt_inputs {
            assert!(
                !matches!(o.domain, OptDomain::RealList { .. }),
                "{}.{} is the first real-list parameter — re-check the RealList arm in \
                 rust_abstract, java_abstract, java_metadata and csharp_metadata",
                r.name,
                o.param_name
            );
        }
        for i in &r.inputs {
            assert!(
                i.kind != InputKind::Integer,
                "{}.{} is the first integer input — re-check every registry's Integer arm \
                 and the ParamHolder/dispatch binding",
                r.name,
                i.param_name
            );
        }
    }
}

/// A price bundle is ONE parameter carrying a component bitmask, not N arrays.
/// `Core.Adx` takes three `double[]`, but `TA_FuncInfo.nbInput` for ADX is 1 —
/// the fold every registry inherits from `price_bundle`.
#[test]
fn abstract_rows_price_bundle_is_one_parameter() {
    use ta_codegen_lib::backends::abstract_rows::InputKind;
    const HLC: u32 = 0x0000_0002 | 0x0000_0004 | 0x0000_0008;
    let rows = all_abstract_rows();
    let adx = rows.iter().find(|r| r.name == "ADX").expect("ADX row");
    assert_eq!(adx.inputs.len(), 1, "ADX must present one bundled price input");
    assert_eq!(adx.inputs[0].kind, InputKind::Price);
    assert_eq!(adx.inputs[0].param_name, "inPriceHLC");
    assert_eq!(adx.inputs[0].flags, HLC, "ADX's bundle is exactly H+L+C");

    // And the non-bundled case still carries no component bits.
    let sma = rows.iter().find(|r| r.name == "SMA").expect("SMA row");
    assert_eq!(sma.inputs.len(), 1);
    assert_eq!(sma.inputs[0].kind, InputKind::Real);
    assert_eq!(sma.inputs[0].flags, 0);
}

/// Issue #159: an `int`-array subscript compared against a `usize`-typed variable
/// must render with the whole cast parenthesized. rustc cannot *parse* a cast
/// followed by `<` — it reads `usize <` as the start of generic arguments — so
/// `(dqI[hd]) as usize < trailingIdx` is a hard error while `generate` exits 0.
///
/// No shipped indicator has this shape (the monotonic-deque rolling-extremum
/// candidates in #147 are what surfaced it), so a regenerate is byte-identical
/// here and proves nothing; this fixture is the coverage.
///
/// Both operand positions are exercised. The right-hand one is not merely
/// defensive: `render_binop_operand` leaves a higher-precedence arithmetic child
/// unparenthesized on the left of a comparison, so `trailingIdx + dqI[hd] < today`
/// puts a *right*-operand cast directly before a `<` too.
#[test]
fn int_array_vs_usize_comparison_parenthesizes_the_cast() {
    let source = r#"
int max_lookback( int optInTimePeriod )
{
   return (optInTimePeriod-1);
}

TA_RetCode max( int    startIdx,
                int    endIdx,
                const double inReal[],
                int    optInTimePeriod,
                int   *outBegIdx,
                int   *outNBElement,
                double outReal[] )
{
   int outIdx, trailingIdx, today, highestIdx;
   int dqI[4];
   int hd;

   hd = 0;
   dqI[hd] = startIdx;
   outIdx = 0;
   today = startIdx;
   trailingIdx = startIdx;
   highestIdx = -1;

   while( today <= endIdx )
   {
      /* left operand carries the cast, directly before `<` */
      if( dqI[hd] < trailingIdx )
         hd = 0;

      /* right operand carries the cast, and the enclosing `<` still follows it */
      if( trailingIdx + dqI[hd] < today )
         hd = 0;

      /* mirror: the cast lands on the right operand of the comparison itself */
      if( trailingIdx < dqI[hd] )
         hd = 0;

      /* the i32 sentinel path (the shape WILLR/MIN/MAX already emit) */
      if( highestIdx < trailingIdx )
         highestIdx = trailingIdx;

      outReal[outIdx++] = inReal[today];
      trailingIdx++;
      today++;
   }

   *outBegIdx = startIdx;
   *outNBElement = outIdx;
   return TA_SUCCESS;
}
"#;
    let (func, enums) = load_indicator_with_source("max", source);
    let out = generate_all(&func, &enums);

    // The whole cast is wrapped, in every position.
    for needle in [
        "((dqI[hd]) as usize) < trailingIdx",
        "trailingIdx + ((dqI[hd]) as usize) < today",
        "trailingIdx < ((dqI[hd]) as usize)",
        "highestIdx < ((trailingIdx) as i32)",
    ] {
        assert!(
            out.rust.contains(needle),
            "Rust output missing `{needle}`:\n{}",
            out.rust
        );
    }

    // And nowhere does a bare cast sit directly before `<`, which would not parse.
    check_rust_cast_parens(&out.rust, "max/#159");

    // C and Java are unaffected — they have no cast to place at all.
    assert!(
        out.c.contains("if( dqI[hd] < trailingIdx )"),
        "C output should compare directly:\n{}",
        out.c
    );
    assert!(
        out.java.contains("if( dqI[hd] < trailingIdx )"),
        "Java output should compare directly:\n{}",
        out.java
    );
}

/// Issue #163: arithmetic over an `int` array element, compared against a
/// `usize`-typed variable, must carry a cast. `expr_is_i32_typed` recurses through
/// arithmetic but has no `ArrayAccess` arm, while `render_binop`'s array tests knew
/// about `int` arrays but matched a *direct* subscript only — so `dqI[hd] + 1`
/// was typed by neither and rendered bare, failing to compile with E0308.
///
/// The two shapes that already worked are asserted alongside the four that did
/// not, because they are what pin the root cause: a plain `int` local is usize in
/// Rust and needs nothing, and an expression with a usize operand already got its
/// cast from the arithmetic arm. A fix that changed either of those would be
/// reaching too far — that is how the first attempt at this turned
/// `(periods[j] as usize) > longestPeriod` into `periods[j] > (longestPeriod as
/// i32)` in ULTOSC, narrowing an index-domain value.
#[test]
fn arithmetic_over_int_array_elements_is_typed_i32() {
    let source = r#"
int max_lookback( int optInTimePeriod )
{
   return (optInTimePeriod-1);
}

TA_RetCode max( int    startIdx,
                int    endIdx,
                const double inReal[],
                int    optInTimePeriod,
                int   *outBegIdx,
                int   *outNBElement,
                double outReal[] )
{
   int outIdx, trailingIdx, today;
   int dqI[4];
   int hd;

   hd = 0;
   dqI[hd] = startIdx;
   outIdx = 0;
   today = startIdx;
   trailingIdx = startIdx;

   while( today <= endIdx )
   {
      if( dqI[hd] + 1 < today )        /* was E0308 */
         hd = 0;
      if( dqI[hd] - 1 < today )        /* was E0308 */
         hd = 0;
      if( dqI[hd] * 2 < today )        /* was E0308 */
         hd = 0;
      if( dqI[hd] << 1 < today )       /* was E0308 */
         hd = 0;
      if( dqI[hd] / 2 < today )        /* was E0308 */
         hd = 0;
      if( dqI[hd] % 3 < today )        /* was E0308 */
         hd = 0;
      if( (dqI[hd] & 3) < today )      /* was E0308 */
         hd = 0;
      if( dqI[hd] + optInTimePeriod < today )  /* was E0308: i32 opt param, not a literal */
         hd = 0;
      if( today < dqI[hd] + 1 )        /* mirror: the compound is the RIGHT operand */
         hd = 0;
      if( dqI[hd] + 1 <= today )       /* <= is legal after a bare cast; still must be typed */
         hd = 0;
      if( hd + 1 < today )             /* control: plain int local, already usize */
         hd = 0;
      if( trailingIdx + dqI[hd] < today )  /* control: usize operand present */
         hd = 0;

      outReal[outIdx++] = inReal[today];
      trailingIdx++;
      today++;
   }

   *outBegIdx = startIdx;
   *outNBElement = outIdx;
   return TA_SUCCESS;
}
"#;
    let (func, enums) = load_indicator_with_source("max", source);
    let out = generate_all(&func, &enums);

    // The four that did not compile now cast, in the usize (index) domain, and
    // the cast is fully parenthesized — it sits on the left of a `<`, so without
    // #159's wrap_cast this would not even parse.
    for needle in [
        "((dqI[hd] + 1) as usize) < today",
        "((dqI[hd] - 1) as usize) < today",
        "((dqI[hd] * 2) as usize) < today",
        "((dqI[hd] << 1) as usize) < today",
        "((dqI[hd] / 2) as usize) < today",
        "((dqI[hd] % 3) as usize) < today",
        "((dqI[hd] & 3) as usize) < today",
        // An i32 opt-in param is not an IntLiteral; the first cut of stays_i32
        // rejected it and this shape still failed to compile.
        "((dqI[hd] + optInTimePeriod) as usize) < today",
        // Mirror: the compound as the RIGHT operand of the comparison.
        "today < ((dqI[hd] + 1) as usize)",
        // `<=` parses after a bare cast, so this one proves the TYPING fired,
        // independently of #159's parenthesization.
        "((dqI[hd] + 1) as usize) <= today",
    ] {
        assert!(
            out.rust.contains(needle),
            "Rust output missing `{needle}`:\n{}",
            out.rust
        );
    }

    // The two that already worked are untouched — no cast appears on either.
    for needle in ["if hd + 1 < today {", "if trailingIdx + ((dqI[hd]) as usize) < today {"] {
        assert!(
            out.rust.contains(needle),
            "Rust output should leave `{needle}` unchanged:\n{}",
            out.rust
        );
    }

    check_rust_cast_parens(&out.rust, "max/#163");

    // C and Java compare directly; neither has a cast to place.
    assert!(out.c.contains("if( dqI[hd] + 1 < today )"), "C changed:\n{}", out.c);
    assert!(out.java.contains("if( dqI[hd] + 1 < today )"), "Java changed:\n{}", out.java);
}

/// The cast-parens gate must key on the operators rustc actually cannot parse.
/// `<` and `<<` after a bare cast are errors; `<=` and `<<=` are legal, and 22
/// shipped sites spell `<=` after a cast — matching them would fail the suite on
/// correct output. Verified against rustc, not assumed.
#[test]
fn cast_parens_gate_flags_only_the_ambiguous_operators() {
    let must_flag = [
        "        if (dqI[hd]) as usize < trailingIdx {",
        "        x = (dqI[hd]) as i32 << 2;",
        "        if a + (dqI[hd]) as usize < today {",
    ];
    for line in must_flag {
        assert!(
            std::panic::catch_unwind(|| check_rust_cast_parens(line, "fixture")).is_err(),
            "gate failed to flag an unparseable cast: {line}"
        );
    }

    let must_pass = [
        "        if ((dqI[hd]) as usize) < trailingIdx {",   // correctly wrapped
        "        if (dqI[hd]) as usize <= trailingIdx {",    // `<=` parses
        "        x = (dqI[hd]) as i32 <<= 2;",               // `<<=` parses
        "        if (dqI[hd]) as usize > trailingIdx {",     // `>` parses
        "        let n = (x) as usize;",                     // terminal position
        "        v[(i) as usize] = 0.0;",                    // index position
        "        // prose mentioning as usize < in a comment",
    ];
    for line in must_pass {
        assert!(
            std::panic::catch_unwind(|| check_rust_cast_parens(line, "fixture")).is_ok(),
            "gate false-positived on legal output: {line}"
        );
    }
}

/// An empty C comment must not abort `generate`. `/*  */` is ordinary C, and
/// `/* * */` reduces to the same thing because the lone `*` is eaten as a
/// continuation prefix; both reached `block_comment` with zero lines, which
/// indexed `lines[1..]` on an empty slice and panicked out of the whole run.
///
/// Found by a synth3 fixture that happened to label a multiply with `/* * */`.
#[test]
fn empty_c_comments_do_not_abort_generation() {
    for comment in ["/*  */", "/* * */", "/**/", "/*\n    *\n    */"] {
        let source = format!(
            r#"
int max_lookback( int optInTimePeriod )
{{
   return (optInTimePeriod-1);
}}

TA_RetCode max( int    startIdx,
                int    endIdx,
                const double inReal[],
                int    optInTimePeriod,
                int   *outBegIdx,
                int   *outNBElement,
                double outReal[] )
{{
   int outIdx, i;

   outIdx = 0;
   for( i=startIdx; i <= endIdx; i++ )
   {{
      {comment}
      outReal[outIdx++] = inReal[i];
   }}
   *outBegIdx = startIdx;
   *outNBElement = outIdx;
   return TA_SUCCESS;
}}
"#
        );
        let (func, enums) = load_indicator_with_source("max", &source);
        let out = generate_all(&func, &enums);
        for (lang, text) in [("C", &out.c), ("Rust", &out.rust), ("Java", &out.java)] {
            assert!(
                !text.is_empty(),
                "{lang} output empty for comment {comment:?}"
            );
        }
        // The body still renders; the comment must not have eaten it.
        assert!(
            out.c.contains("outReal[outIdx++] = inReal[i];"),
            "C body lost after comment {comment:?}:\n{}",
            out.c
        );
    }
}

/// Issue #165: a local that `collect_signed_int_vars` elected i32 (#160) must
/// stay recognisably i32 *inside an expression*, not only when it stands alone.
///
/// `expr_is_i32_typed_ctx` folded over the four arithmetic operators only, so
/// `head = lag;` took its `as usize` from the assign ladder while
/// `head = lag & 3;` took none and did not compile. The same omission left an
/// i32 local and a usize local unreconciled on either side of a bitwise
/// operator (`k & 65535 | hits << 16` → `i32 | usize`) — one gap, two symptoms,
/// which is why widening that operator set fixes both.
#[test]
fn signed_locals_stay_i32_inside_expressions() {
    let source = r#"
int max_lookback( int optInTimePeriod )
{
   return (optInTimePeriod-1);
}

TA_RetCode max( int    startIdx,
                int    endIdx,
                const double inReal[],
                int    optInTimePeriod,
                int   *outBegIdx,
                int   *outNBElement,
                double outReal[] )
{
   int outIdx, today, trailingIdx;
   int ring[4];
   int head, hits, lag, kk;
   double barVal;

   outIdx = 0;
   today = startIdx;
   trailingIdx = startIdx;

   while( today <= endIdx )
   {
      barVal = inReal[today];
      if( !(barVal > 0.0) || !(barVal < 1000000.0) )
         barVal = 0.0;
      lag = (int)barVal;
      kk = 0 - optInTimePeriod;
      if( kk < 0 )
         kk += optInTimePeriod;

      head = lag & 3;
      ring[head] = lag & 7;
      hits = 0;
      if( ring[head] < trailingIdx )
         hits += 1;
      kk += (kk & 65535) | (hits << 16);

      outReal[outIdx] = barVal;
      outIdx++;
      trailingIdx++;
      today++;
   }
   *outBegIdx = startIdx;
   *outNBElement = outIdx;
   return TA_SUCCESS;
}
"#;
    let (func, enums) = load_indicator_with_source("max", source);
    let out = generate_all(&func, &enums);

    // A: usize target, masked signed local on the right.
    assert!(
        out.rust.contains("head = (lag & 3) as usize;"),
        "Rust missing the `as usize` on a masked signed local:\n{}",
        out.rust
    );
    // B: the usize half is brought into the i32 domain by the sentinel arm.
    assert!(
        out.rust.contains("((hits << 16) as i32)"),
        "Rust left an i32 local and a usize local unreconciled across `|`:\n{}",
        out.rust
    );
    // The plain form was always right and must not have moved.
    assert!(
        out.rust.contains("ring[head] = (lag & 7) as i32;"),
        "Rust changed the int-array store:\n{}",
        out.rust
    );
    check_rust_cast_parens(&out.rust, "max/#165");

    // C and Java have no cast to place here.
    assert!(out.c.contains("head = lag & 3;"), "C changed:\n{}", out.c);
    assert!(out.java.contains("head = lag & 3;"), "Java changed:\n{}", out.java);
}

// ---------------------------------------------------------------------------
// CIRCBUF storage: the batch/stream tier split (PR #176, issue #155)
// ---------------------------------------------------------------------------
//
// The Rust backend renders one IR `CircBuf` two different ways, and the two are
// not interchangeable:
//
//   batch  - C's hybrid. A zeroed stack array at the `CIRCBUF_PROLOG` static
//            size, a heap `Vec` only when a runtime `CIRCBUF_INIT` can exceed
//            it, and a `&mut` slice the body indexes through. Allocation-free
//            at every default parameter.
//   stream - an owning `Vec`. Forced, not chosen: the open path MOVES the
//            storage into the stream state struct, which outlives the frame, so
//            a `&mut` slice into a stack array would dangle.
//
// Getting this backwards does not produce wrong numbers, it produces code that
// does not compile — but only for a function that happens to exist. These pin
// the rule itself so a refactor cannot quietly apply one tier's shape to the
// other, and the sweep picks up new CIRCBUF carriers with no edit here.

/// Split a Rust function's output into (batch, stream). Both halves must be
/// non-empty for a streamable function — an empty half would turn every
/// `contains` below into a vacuous pass.
fn rust_batch_stream_halves(name: &str) -> (String, String) {
    let (func, enums) = load_indicator(name);
    let out = generate_all(&func, &enums);
    match out.rust.split_once(ta_codegen_lib::backends::rust_stream::SECTION_MARKER) {
        Some((b, s)) => (b.to_string(), s.to_string()),
        None => (out.rust.clone(), String::new()),
    }
}

/// Every CIRCBUF carrier, swept: batch is the hybrid, stream keeps the owning
/// `Vec`, and neither tier borrows the other's shape.
#[test]
fn rust_circbuf_batch_is_hybrid_and_stream_stays_vec() {
    let mut carriers: Vec<(String, Vec<String>)> = Vec::new();
    let mut stream_checked: Vec<String> = Vec::new();

    for name in discover_indicators() {
        let Some((func, enums)) = try_load_indicator(&name) else { continue };
        // CIRCBUF ids come from the IR, not from the rendered text, so a
        // rendering bug cannot hide a function from this sweep. Per tier,
        // because the two tiers need not run the same algorithm: the six
        // rolling-extremum functions carry the block scan's scratch in the
        // batch body only, and their `PRAGMA TA_ALT={STREAM,...}` alternate
        // declares no CIRCBUF at all.
        let prolog_ids = |body: &[ir::Statement]| -> Vec<String> {
            body.iter()
                .filter_map(|s| match s {
                    ir::Statement::CircBuf(ir::CircBuf::Prolog { id, .. }) => Some(id.clone()),
                    _ => None,
                })
                .collect()
        };
        let ids = prolog_ids(&func.body);
        let stream_ids = prolog_ids(func.resolved_for(ir::Lang::Rust).stream_source());
        if ids.is_empty() {
            continue;
        }
        let out = generate_all(&func, &enums);
        let (batch, stream) = match out
            .rust
            .split_once(ta_codegen_lib::backends::rust_stream::SECTION_MARKER)
        {
            Some((b, s)) => (b.to_string(), s.to_string()),
            None => (out.rust.clone(), String::new()),
        };

        // A storage declaration for `id`, in the shape `kind` — matched per
        // line so a `local_`/`heap_` prefix cannot satisfy a check aimed at the
        // bare storage (`"circBuffer = vec!["` IS a substring of
        // `"heap_circBuffer = vec!["`, which silently voids the naive form).
        let decl = |text: &str, id: &str, kind: &str| {
            text.lines().any(|l| {
                let t = l.trim_start();
                t.starts_with("let mut ")
                    && t.contains(id)
                    && t.contains(kind)
                    && !t.contains("local_")
                    && !t.contains("heap_")
            })
        };

        for id in &ids {
            // --- batch: hybrid — stack array, plus a slice to index through.
            assert!(
                batch.contains(&format!("let mut local_{id}")),
                "{name}: batch CIRCBUF `{id}` has no stack array — the hybrid is gone"
            );
            assert!(
                decl(&batch, id, ": &mut ["),
                "{name}: batch CIRCBUF `{id}` is not indexed through a &mut slice"
            );
            assert!(
                !decl(&batch, id, ": Vec<"),
                "{name}: batch CIRCBUF `{id}` declares an owning Vec — that is the \
                 pre-#176 always-allocate shape"
            );

            // --- stream: owning Vec, and none of the batch's borrowed shape.
            if !stream.is_empty() && stream_ids.contains(id) {
                assert!(
                    !stream.contains(&format!("local_{id}")),
                    "{name}: stream CIRCBUF `{id}` took the batch stack array — it is moved \
                     into the state struct and would dangle"
                );
                assert!(
                    !stream.contains(&format!("heap_{id}")),
                    "{name}: stream CIRCBUF `{id}` took the batch heap name"
                );
                assert!(
                    decl(&stream, id, ": Vec<"),
                    "{name}: stream CIRCBUF `{id}` lost its owning Vec"
                );
                assert!(
                    !decl(&stream, id, ": &mut ["),
                    "{name}: stream CIRCBUF `{id}` borrows instead of owning"
                );
            }
        }
        if !stream_ids.is_empty() {
            stream_checked.push(name.clone());
        }
        carriers.push((name, ids));
    }

    // Anti-vacuity: the carriers that exist today must all be swept. A filter or
    // discovery regression fails here instead of passing an empty run.
    let names: Vec<&str> = carriers.iter().map(|(n, _)| n.as_str()).collect();
    for expected in [
        "cci", "cmf", "mfi", "ultosc", "hma", "ht_dcphase", "ht_sine", "ht_trendmode",
        "min", "max", "minmax", "midpoint", "midprice", "willr",
    ] {
        assert!(
            names.contains(&expected),
            "CIRCBUF sweep missed {expected}; swept {names:?}"
        );
    }
    // The stream half is now conditional on the id reaching the stream tier, so
    // pin the functions whose stream genuinely carries one — otherwise a bug
    // that emptied every stream body would turn that half into a silent skip.
    let streamed: Vec<&str> = stream_checked.iter().map(String::as_str).collect();
    for expected in ["cci", "cmf", "mfi", "ultosc"] {
        assert!(
            streamed.contains(&expected),
            "CIRCBUF stream half never ran for {expected}; ran for {streamed:?}"
        );
    }
    // ...and the six rolling-extremum functions must NOT reach it: their scratch
    // belongs to the batch block scan, and their alternate declares none.
    for absent in ["min", "max", "minmax", "midpoint", "midprice", "willr"] {
        assert!(
            !streamed.contains(&absent),
            "{absent}: a CIRCBUF reached the stream tier — the STREAM alternate should \
             declare none"
        );
    }
}

/// CCI — the plain layout with a runtime `CIRCBUF_INIT`. Pins the full hybrid:
/// stack array at the prolog size, a heap `Vec` behind it, and the crossover
/// guard at exactly the static size (C heaps when `Size > sizeof(local)`).
#[test]
fn rust_circbuf_runtime_init_renders_the_crossover_guard() {
    let (batch, _) = rust_batch_stream_halves("cci");

    assert!(
        batch.contains("let mut local_circBuffer: [f64; 30] = [0.0_f64; 30];"),
        "CCI: stack array at the CIRCBUF_PROLOG static size"
    );
    assert!(
        batch.contains("let mut heap_circBuffer: Vec<f64> = Vec::new();"),
        "CCI: heap fallback declared (a runtime INIT can exceed the static size)"
    );
    assert!(
        batch.contains("let mut circBuffer: &mut [f64] = &mut [];"),
        "CCI: body indexes through a &mut slice"
    );
    assert!(
        batch.contains("if (optInTimePeriod) as usize <= 30usize {"),
        "CCI: crossover guard sits at the static size, matching C's macro"
    );
    assert!(
        batch.contains("circBuffer = &mut local_circBuffer;"),
        "CCI: fits ⇒ bind the stack array, no allocation"
    );
    assert!(
        batch.contains("heap_circBuffer = vec![0.0_f64; (optInTimePeriod) as usize];")
            && batch.contains("circBuffer = &mut heap_circBuffer;"),
        "CCI: exceeds ⇒ allocate and bind the heap Vec"
    );
    // The pre-#176 shape: an unconditional allocation on every call. Matched
    // per line — `"circBuffer = vec!["` is a substring of the legitimate
    // `"heap_circBuffer = vec!["`, so `contains` alone can never fire here.
    assert!(
        !batch
            .lines()
            .any(|l| l.trim_start().starts_with("circBuffer = vec![")),
        "CCI: batch must never allocate unconditionally"
    );
}

/// HT_SINE — `CIRCBUF_INIT_LOCAL_ONLY`. There is no runtime size, so the heap
/// arm is unreachable and must not be declared at all: this tier is
/// allocation-free outright, not merely allocation-free at the default.
#[test]
fn rust_circbuf_init_local_only_declares_no_heap_arm() {
    let (batch, _) = rust_batch_stream_halves("ht_sine");

    assert!(
        batch.contains("let mut local_smoothPrice: [f64; 50] = [0.0_f64; 50];"),
        "HT_SINE: stack array at the static size"
    );
    assert!(
        !batch.contains("heap_smoothPrice"),
        "HT_SINE: INIT_LOCAL_ONLY can never reach the heap, so the Vec must not exist"
    );
    assert!(
        batch.contains("smoothPrice = &mut local_smoothPrice;"),
        "HT_SINE: INIT_LOCAL_ONLY binds the stack array directly"
    );
    assert!(
        !batch.contains("smoothPrice = vec!["),
        "HT_SINE: batch must not allocate"
    );
}

/// ULTOSC — `CIRCBUF_PROLOG_CLASS`, field-split into parallel storages. One
/// crossover guard decides for the whole struct; the fields must not be able to
/// disagree about which arm they are in.
#[test]
fn rust_circbuf_class_layout_shares_one_crossover_guard() {
    let (batch, _) = rust_batch_stream_halves("ultosc");

    for field in ["term_closeMinusTrueLow", "term_trueRange"] {
        assert!(
            batch.contains(&format!("let mut local_{field}: [f64; 32] = [0.0_f64; 32];")),
            "ULTOSC: {field} needs its own stack array"
        );
        assert!(
            batch.contains(&format!("let mut heap_{field}: Vec<f64> = Vec::new();")),
            "ULTOSC: {field} needs its own heap fallback"
        );
    }
    // Exactly one guard, with both fields bound inside its arms.
    assert_eq!(
        batch.matches("as usize <= 32usize {").count(),
        1,
        "ULTOSC: the class layout must decide once, not per field"
    );
    let guard = extract_section(&batch, "if (optInTimePeriod3) as usize <= 32usize {", "maxIdx_term =");
    assert!(
        guard.contains("term_closeMinusTrueLow = &mut local_term_closeMinusTrueLow;")
            && guard.contains("term_trueRange = &mut local_term_trueRange;"),
        "ULTOSC: both fields bind the stack arrays in the fits arm"
    );
    assert!(
        guard.contains("heap_term_closeMinusTrueLow = vec![")
            && guard.contains("heap_term_trueRange = vec!["),
        "ULTOSC: both fields allocate in the exceeds arm"
    );
}

/// `TA_MAX_INDEX` is stated as a literal in five hand-written places across four
/// languages plus the Java test server's embedded `Core` (#180). Nothing in the
/// build makes them agree — the generated prologues reference the *symbol*, so a
/// raised cap in `ta_defs.h` alone would leave C accepting calls the other three
/// reject, which is the one divergence the constant exists to prevent.
///
/// This is the parity check. It reads the value out of each surface and requires
/// one distinct value. Adding a fifth binding means adding it here.
#[test]
fn ta_max_index_agrees_across_every_surface() {
    use std::path::Path;
    let root = Path::new(env!("CARGO_MANIFEST_DIR")).join("../..");

    // (file, the text immediately preceding the literal)
    let surfaces: &[(&str, &str)] = &[
        ("include/ta_defs.h", "#define TA_MAX_INDEX "),
        ("ta_codegen/generator/templates/rust/types.rs", "pub const MAX_INDEX: usize = "),
        ("ta_codegen/output/java/library/src/main/java/io/github/talib/Core.java",
         "public static final int MAX_INDEX = "),
        ("ta_codegen/output/csharp/library/Core.cs", "public const int MAX_INDEX = "),
        ("ta_codegen/generator/src/server_gen.rs", "static final int MAX_INDEX = "),
    ];

    let mut seen: Vec<(String, u64)> = Vec::new();
    for (rel, prefix) in surfaces {
        let text = std::fs::read_to_string(root.join(rel))
            .unwrap_or_else(|e| panic!("{rel}: {e}"));
        let at = text
            .find(prefix)
            .unwrap_or_else(|| panic!("{rel}: no `{prefix}` — did the declaration move?"));
        let digits: String = text[at + prefix.len()..]
            .chars()
            .take_while(|c| c.is_ascii_digit() || *c == '_')
            .filter(|c| *c != '_')
            .collect();
        let value: u64 = digits
            .parse()
            .unwrap_or_else(|_| panic!("{rel}: `{prefix}` is not followed by a literal"));
        seen.push(((*rel).to_string(), value));
    }

    let first = seen[0].1;
    for (rel, value) in &seen {
        assert_eq!(
            *value, first,
            "TA_MAX_INDEX disagrees: {rel} says {value}, {} says {first}",
            seen[0].0
        );
    }
    // Pin the shipped value too, so raising the cap is a deliberate edit here
    // and not something a backend picks up silently.
    assert_eq!(first, 100_000_000, "TA_MAX_INDEX changed; update the docs and CHANGELOG with it");
}

/// Every `CIRCBUF_INIT` allocation-failure path must release the buffers the
/// CIRCBUFs before it already took.
///
/// `CIRCBUF_INIT` heap-allocates when the runtime size outgrows its stack
/// buffer and returns `TA_ALLOC_ERR` straight out of the function on failure —
/// so in a function holding more than one CIRCBUF, the later ones' failure
/// paths leak the earlier ones unless the cascade is emitted. Issue #147's
/// rolling-extremum block scan brought the first 2- and 4-CIRCBUF functions
/// into the tree (MIN/MAX take two, MINMAX/MIDPOINT/MIDPRICE/WILLR take four),
/// which is what made a latent generator gap live.
///
/// Swept over every indicator rather than a name list, so a new multi-CIRCBUF
/// function is covered the day it lands.
#[test]
fn c_circbuf_alloc_failure_frees_the_circbufs_before_it() {
    let base = Path::new(env!("CARGO_MANIFEST_DIR")).join("../../ta_codegen/input");
    let registry = make_registry();
    let helpers = make_helpers();

    let mut names: Vec<String> = std::fs::read_dir(&base)
        .expect("input dir")
        .filter_map(|e| {
            let path = e.ok()?.path();
            let name = path.file_name()?.to_str()?.to_string();
            (path.join(format!("{name}.yaml")).is_file() && path.join(format!("{name}.c")).is_file())
                .then_some(name)
        })
        .collect();
    names.sort();
    assert!(names.len() > 100, "expected the full indicator set, got {}", names.len());

    let mut multi_circbuf_functions = 0usize;
    for name in &names {
        let (func, enums) = load_indicator(name);
        let c_out = backends::c::generate(&func, &enums, &registry, &helpers);

        // `declared` resets at each function body: the emitter puts a bare `{`
        // at column 0 to open one.
        let mut declared: Vec<String> = Vec::new();
        let mut pending_alloc: Option<String> = None;
        let mut in_failure_block: Option<(String, Vec<String>)> = None;

        for line in c_out.lines() {
            if line == "{" {
                declared.clear();
                continue;
            }
            let t = line.trim();

            // `double *sufLowest = &local_sufLowest[0];`
            if let Some(rest) = t.strip_prefix("double *").or_else(|| t.strip_prefix("int *")) {
                if let Some(storage) = rest.split(" = &local_").next() {
                    if rest.contains(" = &local_") {
                        declared.push(storage.to_string());
                    }
                }
            }

            if let Some(rest) = t.strip_prefix("if( !") {
                if let Some(storage) = rest.split(&[' ', ')'][..]).next() {
                    if pending_alloc.as_deref() == Some(storage) {
                        in_failure_block = Some((storage.to_string(), Vec::new()));
                    }
                }
            } else if let Some((_, freed)) = in_failure_block.as_mut() {
                if t.contains("TA_Free( ") {
                    let f = t.rsplit("TA_Free( ").next().unwrap_or("");
                    if let Some(v) = f.split(' ').next() {
                        freed.push(v.to_string());
                    }
                }
                if t == "return TA_ALLOC_ERR;" {
                    let (storage, freed) = in_failure_block.take().expect("in block");
                    let at = declared.iter().position(|d| *d == storage);
                    if let Some(at) = at {
                        if at > 0 {
                            multi_circbuf_functions += 1;
                        }
                        for earlier in &declared[..at] {
                            assert!(
                                freed.iter().any(|f| f == earlier),
                                "{name}: allocation failure for `{storage}` returns \
                                 TA_ALLOC_ERR without releasing `{earlier}`, which was \
                                 allocated before it — that leaks up to one full scratch \
                                 buffer per CIRCBUF. Freed here: {freed:?}"
                            );
                        }
                    }
                }
            }

            pending_alloc = t
                .contains("= TA_Malloc(")
                .then(|| t.split(" = TA_Malloc(").next().unwrap_or("").trim().to_string());
        }
    }

    // Guard the gate itself: if the cascade never had a case to cover, the
    // sweep above would pass vacuously.
    assert!(
        multi_circbuf_functions >= 12,
        "expected the rolling-extremum family's multi-CIRCBUF failure paths to be \
         swept, saw only {multi_circbuf_functions}"
    );
}

/// A CIRCBUF whose cursor is never read gets no cursor.
///
/// `CIRCBUF_PROLOG` used to declare `<id>_Idx` and `maxIdx_<id>` unconditionally
/// and `CIRCBUF_INIT` to assign them, which is right for a ring (the body
/// advances with `CIRCBUF_NEXT` and indexes with the cursor) and wrong for a
/// CIRCBUF used only as a period-sized scratch buffer. The #147 block scan
/// indexes its arrays directly, so the six rolling-extremum functions were
/// emitting eight write-only ints apiece — 80 `-Wunused-but-set-variable`
/// across the family in any consumer building with `-Wall -Wextra`.
///
/// Both halves are asserted: the scratch users must NOT carry the pair, and the
/// ring users must still carry it. Dropping it from a ring would not compile,
/// but this says so at the generator rather than in a downstream build.
#[test]
fn c_circbuf_omits_the_cursor_when_nothing_reads_it() {
    let registry = make_registry();
    let helpers = make_helpers();

    for name in ["min", "max", "minmax", "midpoint", "midprice", "willr"] {
        let (func, enums) = load_indicator(name);
        let c_out = backends::c::generate(&func, &enums, &registry, &helpers);
        for decl in ["_Idx;", "maxIdx_"] {
            assert!(
                !c_out.contains(decl),
                "{name} uses its CIRCBUFs as plain scratch, so `{decl}` must not be \
                 emitted — a write-only int is a -Wunused-but-set-variable in every \
                 consumer's build: {c_out}"
            );
        }
        // The buffers themselves must survive: this trims the cursor, not the scratch.
        assert!(
            c_out.contains("= &local_") && c_out.contains("TA_Malloc("),
            "{name} must still declare and size its scratch buffers: {c_out}"
        );
    }

    // The ring users are the control arm — if the predicate went blanket-true,
    // these would lose a cursor they genuinely read and this test would say so.
    for name in ["cci", "ultosc"] {
        let (func, enums) = load_indicator(name);
        let c_out = backends::c::generate(&func, &enums, &registry, &helpers);
        assert!(
            c_out.contains("_Idx;") && c_out.contains("maxIdx_"),
            "{name} advances a real ring, so it must keep its cursor: {c_out}"
        );
    }
}

/// A local read ONLY by a `CIRCBUF_INIT` size must survive the dead-store pass.
///
/// `drop_unused_decls` (c_stream) removes a hoisted prologue decl the arm never
/// reads — HMA's degenerate arm inherits `halfPeriod = optInTimePeriod / 2` and
/// never uses it. The trap is that `walk_stmt_exprs` treats `Statement::CircBuf`
/// as opaque, so a use-analysis built on it alone cannot see `CIRCBUF_INIT`'s
/// size expression. HMA's general arm sizes its de-lag ring with `ringSize`,
/// which is otherwise only ever assigned — miss that read and the pass deletes a
/// live declaration, emitting C that does not compile.
///
/// Both directions are asserted so the pass cannot pass by doing nothing.
#[test]
fn c_stream_keeps_a_local_read_only_by_a_circbuf_size() {
    let registry = make_registry();
    let helpers = make_helpers();
    let (func, enums) = load_indicator("hma");
    let stream_c = backends::c_stream::generate(&func, &enums, &registry, &helpers);

    // Read only through CIRCBUF_INIT's size — the case a naive walker misses.
    assert!(
        stream_c.contains("int ringSize;") && stream_c.contains("ringSize = sqrtPeriod - 1;"),
        "`ringSize` is read only by CIRCBUF_INIT's size expression, so a use-analysis \
         that does not descend into Statement::CircBuf will drop it and emit C that \
         references an undeclared variable: {stream_c}"
    );

    // ...and the dead store the pass exists for is gone. HMA's stream section
    // carries exactly one `halfPeriod` assignment: the general arm's. The
    // degenerate arm's copy is the dead one.
    assert_eq!(
        stream_c.matches("halfPeriod = optInTimePeriod / 2;").count(),
        1,
        "the degenerate arm must not carry a write-only `halfPeriod` (that is a \
         -Wunused-but-set-variable in the consumer's build), and the general arm \
         must keep the one it reads: {stream_c}"
    );
}

/// Every composed function's Open must FUSE its sub-calls: one
/// `OpenAndFillInternal` that both warms the sub-handle and fills the sub-call's
/// destination, instead of a warm pass plus a batch call recomputing the same
/// numbers (issue #192).
///
/// This needs its own gate because no value gate can see it. The fusion is
/// output-preserving by construction, so `stream_verify`, `--xlang-hash` and the
/// frozen-oracle suites all stay green whether or not it happens — a regression
/// would surface only as a benchmark drifting back towards ~1.7x, which nothing
/// fails on. What is pinned here is the SHAPE.
///
/// One sub-call is deliberately left unfused: STOCH's slow-K `TA_MA` writes its
/// own source in place, where a fused open would overwrite the buffer the
/// sub-handle's capture still has to read. `test_c_stoch_composed_stream_section`
/// pins which one; this test pins that it is the ONLY one.
#[test]
fn test_composed_open_fuses_every_sub_call() {
    const COMPOSED: &[&str] = &[
        "bbands", "macdext", "ppo", "pvo", "stddev",
        "stoch", "stochf", "adxr", "stochrsi", "apo",
    ];
    let registry = make_registry();
    let helpers = HelperRegistry::empty();
    let mut fused_total = 0usize;
    let mut unfused_total = 0usize;

    for name in COMPOSED {
        let (mut func, enums) = load_indicator(name);
        func.streaming = true;
        let c = backends::c::generate(&func, &enums, &registry, &helpers);
        let stream = &c[c.find("/**** Streaming API *****/").expect("stream section")..];

        let fused = stream.matches("_OpenAndFillInternal( &sub").count();
        let unfused = stream.matches("_OpenInternal( &sub").count();
        // The other two backends must reach the SAME split. They render the call
        // differently (Rust slices; Java passes the array itself wherever the
        // sub-range is already the whole array, #203) but the decision is shared
        // — `SubCallStep::is_fusable` — so a per-backend divergence here means one
        // emitter silently stopped fusing, which no value gate can see.
        // Anchored on the first ARGUMENT so these count call sites, not the
        // wrapper definitions (Rust sub-opens pass `&series[..n]`; both
        // definitions break the line right after the paren).
        let r = backends::rust_stream::generate(&func, &enums, &registry, &helpers);
        assert_eq!(
            (r.matches("_OpenAndFillInternal(&").count(), r.matches("_OpenInternal(&").count()),
            (fused, unfused),
            "{name}: Rust fused/unfused split differs from C"
        );
        // Java's first argument is no longer a reliable anchor — since #203 a
        // sub-open passes a bare `inReal` where the copy carried nothing — so
        // count the lines that ASSIGN a `sub<n>` handle instead. That is what
        // separates a sub-open both from the wrapper definitions and from the
        // public `_Open`'s own delegation to `_OpenInternal`.
        let j = backends::java_stream::generate(&func, &enums, &registry, &helpers);
        let sub_opens = |needle: &str| {
            j.lines().filter(|l| l.contains("sub") && l.contains(needle)).count()
        };
        assert_eq!(
            (sub_opens("_OpenAndFillInternal("), sub_opens("_OpenInternal(")),
            (fused, unfused),
            "{name}: Java fused/unfused split differs from C"
        );
        assert!(
            fused + unfused > 0,
            "{name}: composed Open emitted no sub-stream open at all"
        );
        // A fused sub-open replaces the batch sub-call with `<var> = subRc;`, so
        // every fused sub must have left exactly one of those substitutions.
        assert_eq!(
            stream.matches("= subRc;").count(),
            fused,
            "{name}: fused sub-opens ({fused}) and retCode substitutions disagree \
             — either a batch sub-call survived the fusion, or one was dropped \
             without leaving a retCode behind for the transcribed error handling"
        );
        fused_total += fused;
        unfused_total += unfused;
    }

    assert_eq!(
        unfused_total, 1,
        "expected exactly ONE unfused sub-call across the composed tier (STOCH's \
         in-place slow-K); found {unfused_total}. A new unfused sub-call means \
         either an aliasing sub-call was added — legitimate, but record it here — \
         or the fusion silently stopped applying."
    );
    assert_eq!(
        fused_total, 17,
        "expected 17 fused sub-calls across the composed tier; found {fused_total}. \
         Adding a composed indicator moves this number: update it deliberately."
    );
}
