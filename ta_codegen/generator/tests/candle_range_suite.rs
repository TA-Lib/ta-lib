//! Render pins for the candle range arms in the generated backends.
//!
//! `ta_candlerange` has more source texts than anyone expects. C is the
//! reference and spells the Shadows arm `TA_UPPERSHADOW + TA_LOWERSHADOW`
//! (`ta_utility.h`); the algebraically equal `(high - low) - |close - open|`
//! rounds differently on any bar whose low sits below half its high, which is
//! where Sterbenz' lemma stops making the subtractions exact (#217).
//!
//! Two of those texts live in `backends/rust_lang.rs` as hardcoded `format!`
//! strings, used whenever the helper call sits INSIDE a larger expression and
//! so cannot be hoisted to a `_candlerange_N` temporary rendered from
//! `input/helpers/candlestick.c`. Every shipped candlestick hits that path —
//! the call is inside an `if` condition — while SYNTH7/SYNTH8 do not, because
//! they assign the helper to a local. That is why fixing the helper body and
//! the Java/C# duplicates left Rust divergent with every gate green.
//!
//! These pins read the arms directly, so they fail on the rendering rather than
//! waiting for a value gate to notice a 1-ULP shift in a 3-valued output —
//! which, per #217, it essentially never does. Both renderings are covered: the
//! inline arms and the hoisted statement arms from the helper body.

use std::collections::HashMap;
use std::path::PathBuf;
use ta_codegen_lib::helper_registry::HelperRegistry;
use ta_codegen_lib::registry::Registry;
use ta_codegen_lib::{backends, ir, parser};

fn input_dir() -> PathBuf {
    PathBuf::from(env!("CARGO_MANIFEST_DIR")).join("../input")
}

fn load_indicator(name: &str) -> (ir::FuncDef, HashMap<String, ir::EnumDef>) {
    let dir = input_dir().join(name);
    let mut func = parser::yaml::parse_yaml(&dir.join(format!("{name}.yaml")));
    let parsed = parser::c_source::parse_c_source(&dir.join(format!("{name}.c")));
    parser::c_source::wire_parsed_source(&mut func, &parsed);
    let enums = parser::enums::load_enums(&input_dir().join("enums.yaml"));
    (func, enums)
}

/// Whitespace-collapsed so the pins survive whatever line breaks
/// `formatter.rs` chooses.
fn collapse(s: &str) -> String {
    s.split_whitespace().collect::<Vec<_>>().join(" ")
}

#[derive(Clone, Copy)]
enum Lang {
    Rust,
    Java,
    CSharp,
}

/// Generated source for one indicator, in one backend.
///
/// `HelperRegistry::from_dir` appends `helpers` itself and fails open to an
/// EMPTY registry on a bad path, so it takes `input/`, not `input/helpers`.
/// With an empty registry nothing hoists and the helper-body arms — one of the
/// five #217 source texts — would go uncovered here.
fn generated(name: &str, lang: Lang) -> String {
    let (func, enums) = load_indicator(name);
    let registry = Registry::from_dir(&input_dir());
    let helpers = HelperRegistry::from_dir(&input_dir());
    let full = match lang {
        Lang::Rust => backends::rust_lang::generate(&func, &enums, &registry, &helpers),
        Lang::Java => backends::java::generate(&func, &enums, &registry, &helpers),
        Lang::CSharp => backends::csharp::generate(&func, &enums, &registry, &helpers),
    };
    collapse(&full)
}

fn rust_source(name: &str) -> String {
    generated(name, Lang::Rust)
}

/// Every `match <SET>_rangeType { .. }` arm block in `src`, brace-balanced.
fn range_type_matches(src: &str) -> Vec<String> {
    let bytes = src.as_bytes();
    let mut out = Vec::new();
    let mut from = 0usize;
    while let Some(hit) = src[from..].find("_rangeType {") {
        let open = from + hit + "_rangeType ".len();
        let (mut depth, mut k) = (0usize, open);
        loop {
            match bytes[k] {
                b'{' => depth += 1,
                b'}' => {
                    depth -= 1;
                    if depth == 0 {
                        break;
                    }
                }
                _ => {}
            }
            k += 1;
        }
        out.push(src[open..=k].to_string());
        from = open + 1;
    }
    out
}

/// Split one range-type block into its Shadows arm and its fall-through arm.
/// Each block has exactly one `2 =>` and one `_ =>`, and the Shadows arm runs
/// up to the fall-through.
fn shadows_and_default(block: &str) -> (String, String) {
    let a2 = block
        .find("2 =>")
        .unwrap_or_else(|| panic!("Shadows needs its own arm, not folded into `_`: {block}"));
    let d = block
        .rfind("_ =>")
        .unwrap_or_else(|| panic!("no fall-through arm: {block}"));
    assert!(a2 < d, "arms out of order: {block}");
    (block[a2..d].to_string(), block[d..].to_string())
}

/// `upper + lower` is the only spelling with the two `close >= open` selects and
/// no `abs`; the #217 form is the exact opposite on both counts. Index-agnostic,
/// so it holds at `i`, `i - 3`, `sp.lag2_inClose`, ... alike, and it reads the
/// same for the inline and the hoisted statement rendering.
fn assert_shadows_arm_is_upper_plus_lower(arm: &str, ctx: &str) {
    assert!(
        arm.contains(">="),
        "{ctx}: Shadows arm has no max/min select, so it cannot be upper + lower \
         -- this is the #217 spelling: {arm}"
    );
    assert!(
        !arm.contains("abs"),
        "{ctx}: Shadows arm takes an absolute value; upper + lower does not \
         -- this is the #217 spelling: {arm}"
    );
}

fn assert_default_arm_is_zero(arm: &str, ctx: &str) {
    // TA_CANDLERANGE's innermost ternary falls through to 0, and
    // ta_global.c documents that an out-of-domain rangeType measures every
    // range as zero. Folding Shadows into `_` answers it with a shadow width.
    assert!(
        arm.contains("0.0") && !arm.contains(">=") && !arm.contains("abs"),
        "{ctx}: fall-through arm must be plain 0.0: {arm}"
    );
}

#[test]
fn rust_candlerange_spells_shadows_as_upper_plus_lower() {
    let s = rust_source("cdldoji");
    let blocks = range_type_matches(&s);
    assert!(
        !blocks.is_empty(),
        "cdldoji must render range-type matches -- if this fires, the hoisting \
         rules changed and these pins moved"
    );
    for block in &blocks {
        let (shadows, default) = shadows_and_default(block);
        assert_shadows_arm_is_upper_plus_lower(&shadows, "cdldoji");
        assert_default_arm_is_zero(&default, "cdldoji");
    }
}

#[test]
fn rust_covers_both_the_inline_and_the_hoisted_rendering() {
    // The bug lived only in the inline arms, because the hoisted ones render
    // from the helper body. A suite that saw just one of the two would have
    // missed it, so assert both shapes are actually present here.
    let s = rust_source("cdldoji");
    assert!(
        s.contains("_candlerange_"),
        "expected hoisted statement arms from input/helpers/candlestick.c"
    );
    assert!(
        s.contains("else { match BodyDoji_rangeType {"),
        "expected the inline arms -- the ta_candleaverage avgPeriod == 0 fallback"
    );
}

#[test]
fn java_and_csharp_also_spell_shadows_as_upper_plus_lower() {
    // The same arm has now been fixed twice in two different places: #217
    // caught java.rs and csharp.rs, and their hardcoded `format!` strings are
    // no more connected to the helper body than Rust's were. Pin all three so
    // the next duplicate cannot regress alone.
    for (lang, label) in [(Lang::Java, "java"), (Lang::CSharp, "csharp")] {
        let s = generated("cdldoji", lang);
        assert!(
            s.contains("(inHigh[i] - (((inClose[i]) >= (inOpen[i])) ? (inClose[i]) : (inOpen[i])))"),
            "{label}: upper shadow must be high - max(close, open) — see #217"
        );
        assert!(
            s.contains("((((inClose[i]) >= (inOpen[i])) ? (inOpen[i]) : (inClose[i])) - inLow[i])"),
            "{label}: lower shadow must be min(close, open) - low — see #217"
        );
    }
}

#[test]
fn no_candlestick_carries_the_divergent_shadows_spelling() {
    // Corpus sweep. The inline arms are reached wherever the helper call sits in
    // a condition, which is every candlestick, at whatever bar index that
    // pattern happens to read -- `i - 3` and `sp.lag2_inClose` included.
    let mut checked = 0usize;
    for entry in std::fs::read_dir(input_dir()).expect("input dir") {
        let dir = entry.expect("dir entry").path();
        let Some(name) = dir.file_name().map(|n| n.to_string_lossy().to_string()) else {
            continue;
        };
        if !name.starts_with("cdl") || !dir.join(format!("{name}.yaml")).exists() {
            continue;
        }
        let s = rust_source(&name);
        for block in range_type_matches(&s) {
            let (shadows, default) = shadows_and_default(&block);
            assert_shadows_arm_is_upper_plus_lower(&shadows, &name);
            assert_default_arm_is_zero(&default, &name);
        }
        checked += 1;
    }
    assert!(checked >= 55, "expected the candlestick corpus, saw {checked}");
}
