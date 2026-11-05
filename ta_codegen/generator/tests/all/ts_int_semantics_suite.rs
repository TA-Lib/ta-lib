//! The TypeScript integer-semantics gate (docs/typescript-backend.md, numerical semantics).
//! JS has no `int`, so every place C's integer arithmetic differs from float
//! arithmetic must carry `| 0`, and a missed or extra one changes values with
//! no error. The verdict is the emitter's own type pass, so a wrong verdict is
//! caught by the one independent classifier the generator has: `fma.rs`'s.

use crate::common::{discover_indicators, load_indicator, make_helpers, make_registry};
use ta_codegen_lib::backends::ctype::IntFix;
use ta_codegen_lib::backends::ts::{self, IntAxis, IntSite};

fn all_sites() -> Vec<IntSite> {
    let (registry, helpers) = (make_registry(), make_helpers());
    let mut sites = Vec::new();
    for name in discover_indicators() {
        let (func, enums) = load_indicator(&name);
        sites.extend(ts::int_semantics_sites(&func, &enums, registry, helpers));
    }
    let (_, enums) = load_indicator("sma");
    sites.extend(ts::helper_int_semantics_sites(&enums, registry, helpers));
    sites
}

fn wrapped_whole(s: &str) -> bool {
    s.starts_with("((") && s.ends_with(") | 0)")
}

fn report(bad: &[&IntSite]) -> String {
    bad.iter().take(10).map(|s| format!("  {} {:?}: {}\n", s.func, s.axis, s.rendered)).collect()
}

#[test]
fn every_integer_division_modulo_and_product_truncates() {
    let sites = all_sites();
    for (fix, name) in [(IntFix::TruncDiv, "division"), (IntFix::TruncMod, "modulo"), (IntFix::WrapMul, "product")] {
        let found: Vec<&IntSite> = sites.iter().filter(|s| s.axis == IntAxis::Fix(fix)).collect();
        assert!(!found.is_empty(), "VACUOUS: no integer {name} in the corpus");
        let bad: Vec<&IntSite> = found.iter().copied().filter(|s| !wrapped_whole(&s.rendered)).collect();
        assert!(bad.is_empty(), "integer {name} without `| 0`:\n{}", report(&bad));
    }
}

#[test]
fn every_real_to_int_cast_and_integer_negation_truncates() {
    let sites = all_sites();
    for (fix, name) in [(IntFix::TruncCast, "(int) of a real"), (IntFix::WrapNeg, "integer negation")] {
        let found: Vec<&IntSite> = sites.iter().filter(|s| s.axis == IntAxis::Fix(fix)).collect();
        assert!(!found.is_empty(), "VACUOUS: no {name} in the corpus");
        let bad: Vec<&IntSite> = found.iter().copied().filter(|s| !s.rendered.ends_with(" | 0)")).collect();
        assert!(bad.is_empty(), "{name} without `| 0`:\n{}", report(&bad));
    }
}

#[test]
fn no_real_division_truncates() {
    let sites = all_sites();
    let found: Vec<&IntSite> = sites.iter().filter(|s| s.axis == IntAxis::RealDiv).collect();
    assert!(!found.is_empty(), "VACUOUS: no real division in the corpus");
    let bad: Vec<&IntSite> = found.iter().copied().filter(|s| s.rendered.ends_with("| 0)")).collect();
    assert!(bad.is_empty(), "real division carrying `| 0`:\n{}", report(&bad));
}

/// The type pass and `fma.rs`'s name-based classifier come to their verdicts
/// independently; a division one calls integer and the other real is a wrong
/// verdict in one of them, and only the type pass decides what TS emits.
#[test]
fn the_type_pass_agrees_with_the_independent_classifier_on_every_division() {
    let sites = all_sites();
    let int_divs: Vec<&IntSite> = sites.iter().filter(|s| s.axis == IntAxis::Fix(IntFix::TruncDiv)).collect();
    let real_divs: Vec<&IntSite> = sites.iter().filter(|s| s.axis == IntAxis::RealDiv).collect();
    assert!(!int_divs.is_empty() && !real_divs.is_empty(), "VACUOUS: no division of one kind");
    let bad: Vec<&IntSite> = int_divs.iter().copied().filter(|s| s.real_by_name).collect();
    assert!(bad.is_empty(), "typed int, classified real:\n{}", report(&bad));
    let bad: Vec<&IntSite> = real_divs.iter().copied().filter(|s| !s.real_by_name).collect();
    assert!(bad.is_empty(), "typed real, classified int:\n{}", report(&bad));
}

/// A real reaching an `int` local, return or parameter truncates in C with no
/// cast written. No shipped body does it, so the sweep's coverage is what is
/// asserted: every such store was visited and would carry the form.
#[test]
fn every_implicit_real_to_int_conversion_truncates() {
    let sites = all_sites();
    for (axis, name) in [(IntAxis::IntLocal, "int local"), (IntAxis::IntReturn, "int return"), (IntAxis::IntArg, "int argument")] {
        let found: Vec<&IntSite> = sites.iter().filter(|s| s.axis == axis).collect();
        assert!(!found.is_empty(), "VACUOUS: the sweep visited no {name}");
        let bad: Vec<&IntSite> =
            found.iter().copied().filter(|s| s.truncates && !s.rendered.ends_with(" | 0)")).collect();
        assert!(bad.is_empty(), "implicit conversion at an {name} without `| 0`:\n{}", report(&bad));
    }
}

/// The stream frames (open, step, peek, tape) are walked with the contexts that
/// render them, and they reach the axes the batch bodies do.
#[test]
fn the_stream_frames_are_swept() {
    let sites = all_sites();
    let stream: Vec<&IntSite> = sites.iter().filter(|s| s.stream).collect();
    for (axis, name) in [
        (IntAxis::Fix(IntFix::TruncDiv), "integer division"),
        (IntAxis::Fix(IntFix::WrapMul), "integer product"),
        (IntAxis::RealDiv, "real division"),
        (IntAxis::IntLocal, "int local"),
    ] {
        assert!(stream.iter().any(|s| s.axis == axis), "VACUOUS: no {name} in any stream frame");
    }
}
