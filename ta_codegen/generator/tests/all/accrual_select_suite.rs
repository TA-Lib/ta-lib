//! A coin-flip accumulation in C, Rust, Java and C#: the addend is selected,
//! except in a peek. TypeScript keeps the branch everywhere.

use crate::common;
use ta_codegen_lib::backends;

type Emit = fn(&ta_codegen_lib::ir::FuncDef, &std::collections::HashMap<String, ta_codegen_lib::ir::EnumDef>) -> String;

/// Each backend, a spelling only its selected addend has, and how many of
/// OBV's tiers carry one: batch, open, the stream step, and the
/// single-precision batch where the backend has one.
fn backends_and_spellings() -> [(&'static str, Emit, &'static str, usize); 5] {
    [
        ("C", |f, e| backends::c::generate(f, e, common::make_registry(), common::make_helpers()), "TA_ADDEND_IF_GT(", 4),
        ("Rust", |f, e| backends::rust_lang::generate(f, e, common::make_registry(), common::make_helpers()), "select_unpredictable(tempReal > ", 3),
        ("Java", |f, e| backends::java::generate(f, e, common::make_registry(), common::make_helpers()), "^ Long.MIN_VALUE) & ((tempReal > ", 4),
        ("C#", |f, e| backends::csharp::generate(f, e, common::make_registry(), common::make_helpers()), "Pick(MaskLt(", 4),
        ("TS", |f, e| backends::ts::generate(f, e, common::make_registry(), common::make_helpers()), "-0.0", 0),
    ]
}

/// OBV raises or lowers its sum on whether the close rose, a coin flip. Every
/// tier that carries the sum selects the volume where a backend selects at all;
/// the peek alone keeps its `if` on the close. HT_DCPHASE's sign tests and AD's
/// and PVT's divisor guards keep their branches.
#[test]
fn a_coin_flip_accumulation_selects_its_addend() {
    let (obv, enums) = common::load_indicator("obv");
    for (lang, emit, spelling, tiers) in backends_and_spellings() {
        let src = emit(&obv, &enums);
        let selected = src.lines().filter(|l| l.contains(spelling) && l.contains("prevOBV")).count();
        let branched = src.lines().filter(|l| l.trim_start().starts_with("if") && l.contains("tempReal >")).count();
        if tiers == 0 {
            assert!(selected == 0 && branched > 0, "{lang}: OBV's accumulation must keep its branch");
        } else {
            assert_eq!(selected, tiers, "{lang}: OBV's tiers that select their addend\n{src}");
            assert_eq!(branched, 1, "{lang}: OBV branches on the close outside its peek, or its peek selects");
        }
    }
    for name in ["ad", "pvt", "ht_dcphase", "ht_sine"] {
        let (func, enums) = common::load_indicator(name);
        for (lang, emit, spelling, _) in backends_and_spellings() {
            if lang != "TS" {
                assert!(!emit(&func, &enums).contains(spelling), "{lang}: {name} selects an addend");
            }
        }
    }
}
