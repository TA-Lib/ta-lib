//! Shared test infrastructure for the `*_suite.rs` integration test binaries
//! that used to live together in one `backend_suite.rs`. Each binary that
//! needs it pulls it in via `#[path = "common/mod.rs"] mod common;`.
//!
//! Not every consumer uses every helper here, so unused ones in a given
//! binary are expected — that is the point of sharing one module across
//! several independent `tests/*.rs` crates.
#![allow(dead_code)]

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
pub fn discover_indicators() -> Vec<String> {
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
pub fn load_indicator(name: &str) -> (ir::FuncDef, HashMap<String, ir::EnumDef>) {
    load_from(
        &Path::new(env!("CARGO_MANIFEST_DIR")).join("../../ta_codegen/input"),
        name,
    )
}

/// Load a synthetic gate fixture from `input_synth/` — the definitions carrying
/// generator constructs no shipped indicator uses (see `input_synth/README.md`).
/// `scripts/synth_gate.py` runs the same fixtures end-to-end through every
/// backend; these tests pin the rendered shape.
pub fn load_synth(name: &str) -> (ir::FuncDef, HashMap<String, ir::EnumDef>) {
    load_from(
        &Path::new(env!("CARGO_MANIFEST_DIR")).join("input_synth"),
        name,
    )
}

pub fn load_from(base: &Path, name: &str) -> (ir::FuncDef, HashMap<String, ir::EnumDef>) {
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
pub fn load_enums() -> HashMap<String, ir::EnumDef> {
    let path = Path::new(env!("CARGO_MANIFEST_DIR")).join("../../ta_codegen/input/enums.yaml");
    parser::enums::load_enums(&path)
}

/// Like [`load_indicator`], but wires a hand-written source body onto the real
/// YAML metadata — for fixtures that no shipped `.c` provides. Mirrors the
/// production load path (`wire_parsed_source`), matching the function by name.
pub fn load_indicator_with_source(
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
pub struct AllOutputs {
    pub c: String,
    pub rust: String,
    pub java: String,
}

pub fn make_registry() -> Registry {
    let base = Path::new(env!("CARGO_MANIFEST_DIR")).join("../../ta_codegen/input");
    Registry::from_dir(&base)
}

/// A registry over the `input_synth/` gate fixtures. `scripts/synth_gate.py`
/// copies them into `ta_codegen/input/` before generating, so there they are
/// registered alongside the shipped indicators; here they are their own tree.
/// Sufficient for the fixtures, none of which calls a shipped indicator.
pub fn make_synth_registry() -> Registry {
    let base = Path::new(env!("CARGO_MANIFEST_DIR")).join("input_synth");
    Registry::from_dir(&base)
}

/// Build a `HelperRegistry` over the shipped `.helper.c` files (issue #146+).
pub fn make_helpers() -> HelperRegistry {
    let base = Path::new(env!("CARGO_MANIFEST_DIR")).join("../../ta_codegen/input");
    HelperRegistry::from_dir(&base)
}

pub fn generate_all(func: &ir::FuncDef, enums: &HashMap<String, ir::EnumDef>) -> AllOutputs {
    let registry = make_registry();
    let helpers = HelperRegistry::empty();
    AllOutputs {
        c: backends::c::generate(func, enums, &registry, &helpers),
        rust: backends::rust_lang::generate(func, enums, &registry, &helpers),
        java: backends::java::generate(func, enums, &registry, &helpers),
    }
}

/// Load every shipped definition once, as the abstract rows.
pub fn all_abstract_rows() -> Vec<ta_codegen_lib::backends::abstract_rows::FuncRow> {
    let base = Path::new(env!("CARGO_MANIFEST_DIR")).join("../../ta_codegen/input");
    let enums = parser::enums::load_enums(&base.join("enums.yaml"));
    let funcs: Vec<ir::FuncDef> = discover_indicators()
        .iter()
        .map(|n| parser::yaml::parse_yaml(&base.join(format!("{n}/{n}.yaml"))))
        .collect();
    backends::abstract_rows::rows(&funcs, &enums)
}

// ---------------------------------------------------------------------------
// Variant check functions (extracted from macros for dynamic invocation)
// ---------------------------------------------------------------------------

/// Check that all C variants exist for a given indicator.
pub fn check_c_variants(c: &str, upper: &str, name: &str) {
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
pub fn check_rust_generic_variants(r: &str, name: &str) {
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
pub fn check_java_variants(j: &str, name: &str) {
    assert!(
        j.contains(&format!("{name}_Lookback(")),
        "{name}: Java missing {name}_Lookback"
    );
    // #236 step 5 deleted the C-shaped tier. The body is what is left below
    // the public wrapper, and it is what must exist.
    assert!(
        j.contains(&format!("RetCode {name}_Impl("))
            || j.contains(&format!("RetCode {name}_Impl (")),
        "{name}: Java missing {name} body"
    );
    assert!(
        !j.contains(&format!("{name}_Internal")),
        "{name}: the deleted C-shaped tier must not come back"
    );
    assert!(
        !j.contains("Unguarded"),
        "{name}: Java must not emit an unguarded variant"
    );
}

/// Check C does NOT generate TA_INT_ macros (they've been removed).
pub fn check_c_int_alias(c: &str, upper: &str, name: &str) {
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
pub fn check_rust_cast_parens(r: &str, name: &str) {
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
pub fn try_load_indicator(name: &str) -> Option<(ir::FuncDef, HashMap<String, ir::EnumDef>)> {
    let result = std::panic::catch_unwind(std::panic::AssertUnwindSafe(|| load_indicator(name)));
    result.ok()
}

/// Try to generate all backends, returning None if generation fails.
pub fn try_generate_all(
    func: &ir::FuncDef,
    enums: &HashMap<String, ir::EnumDef>,
) -> Option<AllOutputs> {
    let result =
        std::panic::catch_unwind(std::panic::AssertUnwindSafe(|| generate_all(func, enums)));
    result.ok()
}

/// Does `hay` contain a CALL to `name`, as opposed to merely the substring?
///
/// `RSI_Lookback(` is a suffix of `STOCHRSI_Lookback(`, so a bare `contains`
/// asserting that STOCHRSI calls RSI is satisfied by STOCHRSI's own definition
/// and can never fail. Requiring a non-identifier character before the name is
/// what makes the assertion mean what it says.
pub fn contains_call(hay: &str, name: &str) -> bool {
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

pub fn extract_section(output: &str, start_marker: &str, end_marker: &str) -> String {
    let start = output
        .find(start_marker)
        .unwrap_or_else(|| panic!("Could not find start marker '{start_marker}' in output"));
    let rest = &output[start..];
    let end = rest.find(end_marker).unwrap_or_else(|| {
        panic!("Could not find end marker '{end_marker}' after '{start_marker}' — the section would be unbounded")
    });
    rest[..end].to_string()
}

/// Helper to call Java render_statement with minimal boilerplate.
pub fn render_java_stmt(stmt: &ir::Statement) -> String {
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
pub fn render_c_stmt(stmt: &ir::Statement) -> String {
    let enums = HashMap::new();
    let registry = make_registry();
    let helpers = HelperRegistry::empty();
    let inline_counter = std::cell::Cell::new(0);
    backends::c::render_statement(
        stmt, 3, false, &enums, &registry, &helpers, &inline_counter, &[], false,
    )
}
