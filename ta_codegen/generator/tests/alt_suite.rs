//! `PRAGMA TA_ALT` — alternate implementations (issue #190).
//!
//! An alternate is a whole second body for the same function, declared in the
//! input `.c` as `<name>_ALT<n>` under a one-line `PRAGMA` decoration, and
//! selected per `(API tier, language)`. It is **generator input only** — no
//! backend gains a symbol for it — so nothing but the *content* of the emitted
//! code can show which body won. That is what most of this file checks.
//!
//! The two synthetic fixtures are the point of the emission tests. SYNTH5
//! claims STREAM (the orientation the six rolling-extremum functions ship) and
//! SYNTH6 claims BATCH (the orientation nothing ships), so a resolver that
//! always returned the base — or always returned the alternate — fails one of
//! the two rather than passing both.

use std::collections::HashMap;
use std::path::Path;

use ta_codegen_lib::backends;
use ta_codegen_lib::helper_registry::HelperRegistry;
use ta_codegen_lib::ir::{self, ApiClaim, LangClaim, Lang, Tier};
use ta_codegen_lib::parser;
use ta_codegen_lib::registry::Registry;

fn input_dir() -> std::path::PathBuf {
    Path::new(env!("CARGO_MANIFEST_DIR")).join("../input")
}

fn synth_dir() -> std::path::PathBuf {
    Path::new(env!("CARGO_MANIFEST_DIR")).join("input_synth")
}

fn load_from(base: &Path, name: &str) -> ir::FuncDef {
    let mut func = parser::yaml::parse_yaml(&base.join(format!("{name}/{name}.yaml")));
    let parsed = parser::c_source::parse_c_source(&base.join(format!("{name}/{name}.c")));
    parser::c_source::wire_parsed_source(&mut func, &parsed);
    func
}

/// Wire a FuncDef from an inline `.c` body, borrowing SMA's metadata. Used for
/// the malformed-input cases, which must fail during parse/wire.
fn wire_source(source: impl AsRef<str>) -> ir::FuncDef {
    let base = input_dir();
    let mut func = parser::yaml::parse_yaml(&base.join("sma/sma.yaml"));
    let parsed = parser::c_source::parse_c_source_str(source.as_ref());
    parser::c_source::wire_parsed_source(&mut func, &parsed);
    func
}

fn enums() -> HashMap<String, ir::EnumDef> {
    parser::enums::load_enums(&input_dir().join("enums.yaml"))
}

/// Generated (C, Rust, Java) for a synth fixture.
fn synth_outputs(name: &str) -> (String, String, String) {
    let func = load_from(&synth_dir(), name);
    let e = enums();
    let registry = Registry::from_dir(&synth_dir());
    let helpers = HelperRegistry::empty();
    (
        backends::c::generate(&func, &e, &registry, &helpers),
        backends::rust_lang::generate(&func, &e, &registry, &helpers),
        backends::java::generate(&func, &e, &registry, &helpers),
    )
}

/// Split a generated file at its streaming-section marker.
fn halves<'a>(text: &'a str, marker: &str) -> (&'a str, &'a str) {
    text.split_once(marker)
        .unwrap_or_else(|| panic!("no `{marker}` in the generated output"))
}

// ---------------------------------------------------------------------------
// Grammar and resolution
// ---------------------------------------------------------------------------

#[test]
fn claim_is_parsed_off_the_pragma() {
    let f = load_from(&synth_dir(), "synth5");
    assert_eq!(f.alternates.len(), 1);
    let a = &f.alternates[0];
    assert_eq!(a.name, "synth5_ALT1");
    assert_eq!(a.index, 1);
    assert_eq!(a.api, ApiClaim::Stream);
    assert_eq!(a.lang, LangClaim::AllLanguages);
}

/// The whole rule: later declarations override earlier ones for every cell they
/// claim, with the base as the implicit position-0 entry claiming
/// `{ALL_API,ALL_LANGUAGES}`. Position decides — there is no specificity table.
#[test]
fn later_declarations_win_the_cells_they_claim() {
    let f = load_from(&synth_dir(), "synth5");
    for lang in ir::ALL_LANGS {
        assert!(
            f.resolve_alt(Tier::Batch, lang).is_none(),
            "[{}] BATCH is unclaimed, so the base wins",
            lang.as_str()
        );
        assert_eq!(
            f.resolve_alt(Tier::Stream, lang).map(|a| a.name.as_str()),
            Some("synth5_ALT1"),
            "[{}] STREAM is claimed by the alternate",
            lang.as_str()
        );
    }

    // SYNTH6 is the mirror: BATCH claimed, STREAM on the base.
    let g = load_from(&synth_dir(), "synth6");
    for lang in ir::ALL_LANGS {
        assert_eq!(
            g.resolve_alt(Tier::Batch, lang).map(|a| a.name.as_str()),
            Some("synth6_ALT1")
        );
        assert!(g.resolve_alt(Tier::Stream, lang).is_none());
    }
}

#[test]
fn a_language_scoped_claim_reaches_only_that_language() {
    let f = wire_source(SRC_LANG_SCOPED());
    assert_eq!(f.alternates.len(), 1);
    assert_eq!(f.alternates[0].lang, LangClaim::Rust);
    assert!(f.resolve_alt(Tier::Batch, Lang::Rust).is_some());
    for other in [Lang::C, Lang::Java, Lang::CSharp] {
        assert!(
            f.resolve_alt(Tier::Batch, other).is_none(),
            "[{}] must fall through to the base",
            other.as_str()
        );
    }
    // ...and the resolved view carries that language's body, not the base's.
    let rust = f.resolved_for(Lang::Rust);
    let c = f.resolved_for(Lang::C);
    assert_ne!(
        format!("{:?}", rust.body),
        format!("{:?}", c.body),
        "the Rust batch body must differ from C's"
    );
}

/// `ALL_LANGUAGES` then a narrower claim below it: "everyone but C", with no
/// NOT operator. This is what last-match-wins buys over disjoint partitioning.
#[test]
fn a_narrower_claim_below_a_broad_one_carves_it_out() {
    let f = wire_source(SRC_BROAD_THEN_NARROW());
    assert_eq!(f.alternates.len(), 2);
    assert_eq!(
        f.resolve_alt(Tier::Batch, Lang::C).map(|a| a.name.as_str()),
        Some("sma_ALT2"),
        "C takes the later, narrower claim"
    );
    for other in [Lang::Rust, Lang::Java, Lang::CSharp] {
        assert_eq!(
            f.resolve_alt(Tier::Batch, other).map(|a| a.name.as_str()),
            Some("sma_ALT1"),
            "[{}] keeps the broad claim",
            other.as_str()
        );
    }
}

// ---------------------------------------------------------------------------
// Rejections — every one of these would otherwise be silent
// ---------------------------------------------------------------------------

macro_rules! rejects {
    ($name:ident, $src:expr, $needle:expr) => {
        #[test]
        fn $name() {
            let err = std::panic::catch_unwind(|| wire_source($src))
                .expect_err(concat!(stringify!($name), ": expected a hard error"));
            let msg = err
                .downcast_ref::<String>()
                .cloned()
                .or_else(|| err.downcast_ref::<&str>().map(|s| (*s).to_string()))
                .unwrap_or_default();
            assert!(
                msg.contains($needle),
                "wrong diagnostic: expected {:?} in {msg:?}",
                $needle
            );
        }
    };
}

rejects!(
    rejects_an_unknown_directive,
    SRC_UNKNOWN_DIRECTIVE(),
    "unknown PRAGMA directive `TA_ALTX`"
);
rejects!(
    rejects_a_known_directive_without_its_equals,
    SRC_MISSING_EQUALS(),
    "must be written `TA_ALT=<value>`"
);
rejects!(rejects_an_unknown_tier, SRC_BAD_TIER(), "unknown TA_ALT api tier `BATCHY`");
rejects!(rejects_an_unknown_language, SRC_BAD_LANG(), "unknown TA_ALT language `PYTHON`");
rejects!(
    rejects_an_alternate_with_no_pragma,
    SRC_ALT_NO_PRAGMA(),
    "carries 0 `PRAGMA TA_ALT=` decorations"
);
rejects!(
    rejects_a_pragma_on_the_base,
    SRC_PRAGMA_ON_BASE(),
    "only a `<name>_ALT<n>` function may carry a claim"
);
rejects!(
    rejects_a_fully_shadowed_alternate,
    SRC_SHADOWED(),
    "later alternates cover every one of those cells"
);
rejects!(
    rejects_an_alternate_shadowed_by_the_union_of_later_claims,
    SRC_UNION_SHADOWED(),
    "later alternates cover every one of those cells"
);
rejects!(
    rejects_alternates_that_shadow_the_base,
    SRC_BASE_SHADOWED(),
    "would never be used — an alternate is a specialization of the base"
);
rejects!(
    rejects_out_of_order_numbering,
    SRC_OUT_OF_ORDER(),
    "ascending and contiguous from 1 in file order"
);
rejects!(
    rejects_a_signature_mismatch,
    SRC_SIGNATURE_MISMATCH(),
    "must take the same parameters as `sma`"
);
rejects!(
    rejects_a_pragma_with_nothing_after_it,
    SRC_TRAILING_PRAGMA(),
    "must precede a function signature"
);
rejects!(
    rejects_a_pragma_inside_a_body,
    SRC_PRAGMA_IN_BODY(),
    "sits inside the body of"
);
rejects!(
    rejects_a_second_lookback,
    SRC_TWO_LOOKBACKS(),
    "a second lookback function in one file"
);
rejects!(
    rejects_an_unclassifiable_extra_function,
    SRC_EXTRA_FUNCTION(),
    "expected exactly one base function named `sma`"
);

/// Free-form `PRAGMA` prose is the future-directive escape hatch: accepted,
/// ignored, and — like every PRAGMA — never forwarded to a backend.
#[test]
fn free_form_pragma_is_ignored_and_never_emitted() {
    let f = wire_source(SRC_FREE_FORM());
    assert!(f.alternates.is_empty());
    let rendered = format!("{:?}", f.body) + &format!("{:?}", f.header_comments);
    assert!(
        !rendered.contains("PRAGMA"),
        "a PRAGMA comment reached the IR and would be emitted"
    );
}

/// An alternate is not a callable symbol. Left to the registry's generic
/// `<indicator>_<Suffix>` rule this would emit a call to `TA_SMA_ALT1`, which
/// exists in no backend — a link error instead of a generator error.
#[test]
fn an_alternate_is_not_a_call_target() {
    let registry = Registry::from_dir(&input_dir());
    let err = std::panic::catch_unwind(|| registry.resolve_call("sma_ALT1", Lang::C))
        .expect_err("calling an alternate must be a hard error");
    let msg = err
        .downcast_ref::<String>()
        .cloned()
        .unwrap_or_default();
    assert!(msg.contains("not callable"), "wrong diagnostic: {msg:?}");
    // The ordinary suffix rule is untouched.
    assert_eq!(registry.resolve_call("sma_lookback", Lang::C), "TA_SMA_Lookback");
}

// ---------------------------------------------------------------------------
// Emission — the selection-visibility gate
// ---------------------------------------------------------------------------

/// The generated code must name the alternate it was built from...
#[test]
fn the_winning_alternate_is_named_in_the_output() {
    let (c, rust, java) = synth_outputs("synth5");
    for (lang, text) in [("C", &c), ("Rust", &rust), ("Java", &java)] {
        assert!(
            text.contains("Using synth5_ALT1 for TA_ALT={STREAM,ALL_LANGUAGES}"),
            "{lang}: the stream section must name the alternate"
        );
    }
    let (c6, rust6, java6) = synth_outputs("synth6");
    for (lang, text) in [("C", &c6), ("Rust", &rust6), ("Java", &java6)] {
        assert!(
            text.contains("Using synth6_ALT1 for TA_ALT={BATCH,ALL_LANGUAGES}"),
            "{lang}: the batch section must name the alternate"
        );
    }
    // No marker where the base wins — otherwise 168 files gain a line that says
    // nothing, and the marker stops meaning "something was overridden here".
    let sma = load_from(&input_dir(), "sma");
    let registry = Registry::from_dir(&input_dir());
    let out = backends::c::generate(&sma, &enums(), &registry, &HelperRegistry::empty());
    assert!(!out.contains("TA_ALT="), "SMA declares no alternate and must carry no marker");
}

/// ...and the marker must be telling the truth.
///
/// A marker is rendered from the resolution result, so on its own it would
/// agree with a resolver that picked the wrong body. These check the emitted
/// CODE instead: the two fixtures differ in one observable way per tier — the
/// window-start form indexes an array by an offset expression, the streamable
/// form walks a trailing cursor — and SYNTH5/SYNTH6 place them on opposite
/// sides, so no single always-wrong resolver satisfies both.
#[test]
fn the_emitted_code_matches_the_marker() {
    // SYNTH5: base (window-start) in batch, alternate (trailing cursor) in stream.
    let (c, rust, java) = synth_outputs("synth5");
    for (lang, text, marker) in [
        ("C", &c, "/**** Streaming API *****/"),
        ("Rust", &rust, backends::rust_stream::SECTION_MARKER),
        ("Java", &java, backends::java_stream::SECTION_MARKER),
    ] {
        let (batch, stream) = halves(text, marker);
        assert!(
            batch.contains("nbInitialElementNeeded]"),
            "{lang} SYNTH5: batch lost the window-start indexing — the BATCH cell should \
             resolve to the base"
        );
        assert!(
            !stream.contains("nbInitialElementNeeded]"),
            "{lang} SYNTH5: the unstreamable window-start read reached the stream tier"
        );
        assert!(
            stream.contains("trailingIdx"),
            "{lang} SYNTH5: the stream tier is not the alternate's trailing-cursor walk"
        );
    }

    // SYNTH6: exactly the other way round.
    let (c6, rust6, java6) = synth_outputs("synth6");
    for (lang, text, marker) in [
        ("C", &c6, "/**** Streaming API *****/"),
        ("Rust", &rust6, backends::rust_stream::SECTION_MARKER),
        ("Java", &java6, backends::java_stream::SECTION_MARKER),
    ] {
        let (batch, stream) = halves(text, marker);
        assert!(
            batch.contains("nbInitialElementNeeded]"),
            "{lang} SYNTH6: batch is not the alternate's window-start form"
        );
        assert!(
            !stream.contains("nbInitialElementNeeded]"),
            "{lang} SYNTH6: the alternate's batch-only read reached the stream tier"
        );
        assert!(
            stream.contains("trailingIdx"),
            "{lang} SYNTH6: the stream tier should be the base's trailing-cursor walk"
        );
    }
}

/// The alternate is load-bearing, not decorative: without it SYNTH5's base is
/// genuinely not streamable, so the emission tests above are checking the
/// resolution rather than something that would hold anyway.
#[test]
fn synth5_does_not_stream_without_its_alternate() {
    let mut f = load_from(&synth_dir(), "synth5");
    let registry = Registry::from_dir(&synth_dir());
    assert!(
        ta_codegen_lib::streaming::validate_streamable(&f.resolved_for(Lang::C), &registry).is_ok(),
        "with the alternate, SYNTH5 streams"
    );
    f.alternates.clear();
    assert!(
        ta_codegen_lib::streaming::validate_streamable(&f.resolved_for(Lang::C), &registry).is_err(),
        "without it the base must NOT be streamable, or the fixture proves nothing"
    );
}

// ---------------------------------------------------------------------------
// Fixture sources
// ---------------------------------------------------------------------------

const HEAD: &str = "int sma_lookback(int optInTimePeriod)\n{\n   return (optInTimePeriod-1);\n}\n\n";

/// A minimal well-formed body with SMA's public signature.
fn body(name: &str, marker: i32) -> String {
    format!(
        "TA_RetCode {name}(int startIdx, int endIdx,\n   const double inReal[],\n   int optInTimePeriod,\n   int *outBegIdx, int *outNBElement,\n   double outReal[])\n{{\n   int i;\n   i = {marker};\n   outReal[0] = inReal[startIdx] + i;\n   *outBegIdx = startIdx;\n   *outNBElement = endIdx;\n   return TA_SUCCESS;\n}}\n"
    )
}

fn src(parts: &[&str]) -> String {
    format!("{HEAD}{}", parts.join("\n"))
}

macro_rules! src_const {
    ($name:ident, $body:expr) => {
        #[allow(non_snake_case)]
        fn $name() -> String {
            $body
        }
    };
}

src_const!(SRC_LANG_SCOPED, src(&[
    &body("sma", 1),
    "/* PRAGMA TA_ALT={BATCH,RUST} */",
    &body("sma_ALT1", 2),
]));
src_const!(SRC_BROAD_THEN_NARROW, src(&[
    &body("sma", 1),
    "/* PRAGMA TA_ALT={BATCH,ALL_LANGUAGES} */",
    &body("sma_ALT1", 2),
    "/* PRAGMA TA_ALT={BATCH,C} */",
    &body("sma_ALT2", 3),
]));
src_const!(SRC_UNKNOWN_DIRECTIVE, src(&[
    &body("sma", 1),
    "/* PRAGMA TA_ALTX={BATCH,C} */",
    &body("sma_ALT1", 2),
]));
src_const!(SRC_MISSING_EQUALS, src(&[
    &body("sma", 1),
    "/* PRAGMA TA_ALT {BATCH,C} */",
    &body("sma_ALT1", 2),
]));
src_const!(SRC_BAD_TIER, src(&[
    &body("sma", 1),
    "/* PRAGMA TA_ALT={BATCHY,C} */",
    &body("sma_ALT1", 2),
]));
src_const!(SRC_BAD_LANG, src(&[
    &body("sma", 1),
    "/* PRAGMA TA_ALT={BATCH,PYTHON} */",
    &body("sma_ALT1", 2),
]));
src_const!(SRC_ALT_NO_PRAGMA, src(&[&body("sma", 1), &body("sma_ALT1", 2)]));
src_const!(SRC_PRAGMA_ON_BASE, src(&[
    "/* PRAGMA TA_ALT={BATCH,C} */",
    &body("sma", 1),
]));
src_const!(SRC_SHADOWED, src(&[
    &body("sma", 1),
    "/* PRAGMA TA_ALT={BATCH,C} */",
    &body("sma_ALT1", 2),
    "/* PRAGMA TA_ALT={ALL_API,ALL_LANGUAGES} */",
    &body("sma_ALT2", 3),
]));
// ALT1's two cells are covered by ALT2 and ALT3 *together*, and by neither
// alone — the case a pairwise "is any single later alternate a superset?" test
// accepts. ALT1 wins nothing and would be silently dead.
src_const!(SRC_UNION_SHADOWED, src(&[
    &body("sma", 1),
    "/* PRAGMA TA_ALT={ALL_API,C} */",
    &body("sma_ALT1", 2),
    "/* PRAGMA TA_ALT={BATCH,C} */",
    &body("sma_ALT2", 3),
    "/* PRAGMA TA_ALT={STREAM,C} */",
    &body("sma_ALT3", 4),
]));
// Two alternates that between them claim every cell: legal individually
// (neither shadows the other), but the base is left unreachable.
src_const!(SRC_BASE_SHADOWED, src(&[
    &body("sma", 1),
    "/* PRAGMA TA_ALT={BATCH,ALL_LANGUAGES} */",
    &body("sma_ALT1", 2),
    "/* PRAGMA TA_ALT={STREAM,ALL_LANGUAGES} */",
    &body("sma_ALT2", 3),
]));
src_const!(SRC_OUT_OF_ORDER, src(&[
    &body("sma", 1),
    "/* PRAGMA TA_ALT={BATCH,C} */",
    &body("sma_ALT2", 2),
    "/* PRAGMA TA_ALT={BATCH,RUST} */",
    &body("sma_ALT1", 3),
]));
src_const!(SRC_SIGNATURE_MISMATCH, src(&[
    &body("sma", 1),
    "/* PRAGMA TA_ALT={BATCH,C} */",
    "TA_RetCode sma_ALT1(int startIdx, int endIdx,\n   const double inReal[],\n   int *outBegIdx, int *outNBElement,\n   double outReal[])\n{\n   *outBegIdx = startIdx;\n   *outNBElement = endIdx;\n   return TA_SUCCESS;\n}\n",
]));
src_const!(SRC_TRAILING_PRAGMA, src(&[
    &body("sma", 1),
    "/* PRAGMA TA_ALT={BATCH,C} */",
]));
src_const!(SRC_PRAGMA_IN_BODY, src(&[
    "TA_RetCode sma(int startIdx, int endIdx,\n   const double inReal[],\n   int optInTimePeriod,\n   int *outBegIdx, int *outNBElement,\n   double outReal[])\n{\n   int i;\n   /* PRAGMA TA_ALT={BATCH,C} */\n   i = 1;\n   outReal[0] = inReal[startIdx] + i;\n   *outBegIdx = startIdx;\n   *outNBElement = endIdx;\n   return TA_SUCCESS;\n}\n",
]));
src_const!(SRC_TWO_LOOKBACKS, format!(
    "{HEAD}int sma_lookback(int optInTimePeriod)\n{{\n   return 2;\n}}\n\n{}",
    body("sma", 1)
));
src_const!(SRC_EXTRA_FUNCTION, src(&[&body("sma", 1), &body("helperish", 2)]));
src_const!(SRC_FREE_FORM, src(&[
    "/* PRAGMA a note to whoever maintains the generator */",
    &body("sma", 1),
]));
