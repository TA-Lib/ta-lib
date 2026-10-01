//! Display shift (issue #489): what the generator must refuse, and the one
//! property of the emitted query that no declared range can supply.
//!
//! The values themselves are `ta_regtest`'s to check, across all four servers
//! (`test_abstract.c`, and `synth_gate.py` for a shift that differs per output).
//! Nothing on the PR gate runs those, so the two things that fail SILENTLY are
//! pinned here: a flag and a definition that disagree, and a query that
//! validates less than its lookback does.

use std::path::Path;

use ta_codegen_lib::{backends, ir::DISPLAY_SHIFT_FLAG, parser};

use super::common::{generate_all, load_enums, load_indicator, load_synth, make_registry};

fn synth24_source() -> String {
    let path = Path::new(env!("CARGO_MANIFEST_DIR")).join("input_synth/synth24/synth24.c");
    std::fs::read_to_string(path).expect("synth24.c")
}

fn without_definition(src: &str) -> String {
    let start = src.find("int synth24_display_shift").expect("definition");
    let end = src.find("TA_RetCode synth24(").expect("body");
    format!("{}{}", &src[..start], &src[end..])
}

#[test]
fn the_function_flag_is_derived_from_the_output_flags() {
    let (func, _) = load_synth("synth24");
    let per_output: Vec<bool> = func
        .outputs
        .iter()
        .map(|o| o.flags.iter().any(|f| f == DISPLAY_SHIFT_FLAG))
        .collect();
    assert_eq!(per_output, [false, true, true], "synth24's outputs are (own bar, ahead, behind)");
    assert!(func.flags.iter().any(|f| f == DISPLAY_SHIFT_FLAG), "derived function flag");
    assert!(func.display_shift.is_some());

    let (plain, _) = load_indicator("sma");
    assert!(!plain.flags.iter().any(|f| f == DISPLAY_SHIFT_FLAG));
    assert!(plain.display_shift.is_none());
}

#[test]
#[should_panic(expected = "defines no")]
fn a_flag_without_a_definition_is_refused() {
    let (mut func, _) = load_synth("synth24");
    let parsed = parser::c_source::parse_c_source_str(&without_definition(&synth24_source()));
    parser::c_source::wire_parsed_source(&mut func, &parsed);
}

#[test]
#[should_panic(expected = "no output carries")]
fn a_definition_without_a_flag_is_refused() {
    let (mut func, _) = load_synth("synth24");
    for out in &mut func.outputs {
        out.flags.retain(|f| f != DISPLAY_SHIFT_FLAG);
    }
    let parsed = parser::c_source::parse_c_source_str(&synth24_source());
    parser::c_source::wire_parsed_source(&mut func, &parsed);
}

#[test]
#[should_panic(expected = "must take the optional inputs")]
fn the_definition_takes_the_optional_inputs_then_the_index() {
    let (mut func, _) = load_synth("synth24");
    let swapped = synth24_source().replace(
        "int synth24_display_shift(int optInTimePeriod, int outputIdx)",
        "int synth24_display_shift(int outputIdx, int optInTimePeriod)",
    );
    assert_ne!(swapped, synth24_source(), "the signature to swap was not found");
    let parsed = parser::c_source::parse_c_source_str(&swapped);
    parser::c_source::wire_parsed_source(&mut func, &parsed);
}

/// The body is narrowing-checked against its own signature while the emitted one
/// comes from the YAML, so a type that differs would let C narrow silently.
#[test]
#[should_panic(expected = "must take the optional inputs")]
fn the_definition_declares_the_yaml_types() {
    let (mut func, _) = load_synth("synth24");
    let retyped = synth24_source().replace(
        "int synth24_display_shift(int optInTimePeriod, int outputIdx)",
        "int synth24_display_shift(TA_MAType optInTimePeriod, int outputIdx)",
    );
    assert_ne!(retyped, synth24_source(), "the signature to retype was not found");
    let parsed = parser::c_source::parse_c_source_str(&retyped);
    parser::c_source::wire_parsed_source(&mut func, &parsed);
}

/// The query text of one backend, from its declaration to its closing brace.
fn query<'a>(lang: &str, text: &'a str, decl: &str) -> &'a str {
    let start = text.find(decl).unwrap_or_else(|| panic!("{lang}: no `{decl}`"));
    let body = &text[start..];
    let end = ["\n   }\n", "\n    }\n", "\n}\n"].iter().filter_map(|t| body.find(t)).min();
    &body[..end.unwrap_or_else(|| panic!("{lang}: unterminated query"))]
}

/// An output without the flag answers 0 whatever the body says: the generator
/// returns before the body for it. synth24's first output is the unflagged one.
#[test]
fn an_unflagged_output_is_answered_before_the_body() {
    let (func, enums) = load_synth("synth24");
    let out = generate_all(&func, &enums);
    let csharp = backends::csharp::generate(
        &func,
        &load_enums(),
        make_registry(),
        &ta_codegen_lib::helper_registry::HelperRegistry::empty(),
    );
    for (lang, text, decl) in [
        ("C", &out.c, "int TA_SYNTH24_DisplayShift("),
        ("Rust", &out.rust, "fn synth24_display_shift("),
        ("Java", &out.java, "int synth24DisplayShift("),
        ("C#", &csharp, "int Synth24DisplayShift("),
    ] {
        let q = query(lang, text, decl);
        let guard = q.find("outputIdx == 0").unwrap_or_else(|| panic!("{lang}: no guard:\n{q}"));
        let body = q.find("optInTimePeriod / outputIdx").unwrap_or_else(|| panic!("{lang}: no body:\n{q}"));
        assert!(guard < body, "{lang}: the guard must precede the body:\n{q}");
    }
}

/// FRAMA's lookback refuses an odd period, which its declared range allows. A
/// display shift that only range-checked would answer 0 for a call that cannot
/// run, so every backend's query has to go through its own lookback.
#[test]
fn the_query_asks_its_own_lookback_in_every_backend() {
    let (func, enums) = load_indicator("frama");
    let out = generate_all(&func, &enums);
    let csharp = backends::csharp::generate(
        &func,
        &load_enums(),
        make_registry(),
        &ta_codegen_lib::helper_registry::HelperRegistry::empty(),
    );

    for (lang, text, decl, call) in [
        ("C", &out.c, "int TA_FRAMA_DisplayShift(", "TA_FRAMA_Lookback("),
        ("Rust", &out.rust, "fn frama_display_shift(", "self.frama_lookback("),
        ("Java", &out.java, "int framaDisplayShift(", "framaLookback("),
        ("C#", &csharp, "int FramaDisplayShift(", "FramaLookback("),
    ] {
        let body = query(lang, text, decl);
        assert!(body.contains(call), "{lang}: the display shift does not call `{call}`:\n{body}");
    }
}
