//! The numerical-stability classification must match the *library's* behaviour, not just
//! be self-consistent.
//!
//! Ground truth below was measured, not reasoned: a probe linked against `libta-lib.a`
//! walked every function via `TA_ForEachFunc` and compared its lookback with all unstable
//! periods at 0 against all of them at 10 (`TA_SetUnstablePeriod` for each `TA_FuncUnstId`),
//! first at default parameters and then with every MAType parameter forced to EMA. A
//! function whose lookback moves is one whose output depends on how much history precedes
//! it. Re-run that probe if these expectations ever need revisiting.
//!
//! The point of pinning it here is that the generator derives stability *statically*, from
//! the call graph, and a static derivation can drift from the running library in both
//! directions — it over-approximated on SAR (which calls MINUS_DM with a literal period of
//! 1, below the threshold where MINUS_DM consults its unstable period) and on MACDEXT
//! (which delegates to MACD only when all three MA types are EMA) before the analysis was
//! narrowed to predicated lookback calls.

use std::collections::HashSet;
use std::path::Path;

use ta_codegen_lib::{ir::FuncDef, parser, stability};

fn load() -> Vec<FuncDef> {
    let base = Path::new(env!("CARGO_MANIFEST_DIR")).join("../input");
    let mut funcs = Vec::new();
    for e in std::fs::read_dir(&base).expect("read input dir").filter_map(Result::ok) {
        let dir = e.path();
        if !dir.is_dir() {
            continue;
        }
        let n = e.file_name().to_string_lossy().to_string();
        let (yaml, csrc) = (dir.join(format!("{n}.yaml")), dir.join(format!("{n}.c")));
        if !yaml.exists() || !csrc.exists() {
            continue;
        }
        let mut f = parser::yaml::parse_yaml(&yaml);
        let parsed = parser::c_source::parse_c_source(&csrc);
        parser::c_source::wire_parsed_source(&mut f, &parsed);
        funcs.push(f);
    }
    assert!(funcs.len() > 160, "expected the whole input tree, got {}", funcs.len());
    funcs
}

/// Functions that inherit an unstable period through a hard-coded inner call, and the
/// function each one inherits from. Measured: every one of these moves at default params.
/// KC is the only one with TWO sources (EMA for its centre line, ATR for its band), and it
/// shares the ATR one with SUPERTREND -- which inherits it through `atr_lookback()` alone,
/// with no call to `atr()` in the body at all.
const INHERITED: &[(&str, &str)] = &[
    ("ADOSC", "EMA"),
    ("ADXR", "ADX"),
    ("DEMA", "EMA"),
    ("KC", "ATR"),
    ("MACD", "EMA"),
    ("MACDFIX", "EMA"),
    ("SMI", "EMA"),
    ("STOCHRSI", "RSI"),
    ("SUPERTREND", "ATR"),
    ("TEMA", "EMA"),
    ("TRIX", "EMA"),
];

/// Functions whose stability is the caller's MA-type choice. Measured: BBANDS, MA,
/// MACDEXT, MAVP, STOCH and STOCHF are stable at their (SMA) defaults and unstable with
/// EMA; APO, PPO and PVO default to EMA and so move even at defaults.
const MATYPE_DEPENDENT: &[&str] =
    &["APO", "BBANDS", "MA", "MACDEXT", "MAVP", "PPO", "PVO", "STOCH", "STOCHF", "STOCHRSI"];

#[test]
fn classification_matches_the_measured_library() {
    let funcs = load();
    let st = stability::classify(&funcs);

    // Every function the YAML declares is intrinsically unstable, and no others.
    let declared: HashSet<&str> = funcs
        .iter()
        .filter(|f| f.flags.iter().any(|x| x == "unstable_period"))
        .map(|f| f.name.as_str())
        .collect();
    assert_eq!(declared.len(), 20, "the measured set of self-declaring functions is 20");
    for f in &funcs {
        assert_eq!(
            st[&f.name].intrinsic,
            declared.contains(f.name.as_str()),
            "{} intrinsic flag disagrees with its YAML",
            f.name
        );
    }

    // Inherited instability: exactly this set, each naming the right source.
    let computed: Vec<(String, Vec<String>)> = funcs
        .iter()
        .filter(|f| !st[&f.name].inherited_from.is_empty())
        .map(|f| (f.name.clone(), st[&f.name].inherited_from.clone()))
        .collect();
    let mut names: Vec<&str> = computed.iter().map(|(n, _)| n.as_str()).collect();
    names.sort_unstable();
    let expected: Vec<&str> = INHERITED.iter().map(|(n, _)| *n).collect();
    assert_eq!(names, expected, "set of transitively-unstable functions changed");
    for (name, from) in INHERITED {
        let got = &st[*name].inherited_from;
        assert!(got.iter().any(|g| g == from), "{name} should inherit from {from}, got {got:?}");
    }

    // MA-type-dependent: exactly this set.
    let mut got: Vec<&str> = funcs
        .iter()
        .filter(|f| st[&f.name].matype_dependent)
        .map(|f| f.name.as_str())
        .collect();
    got.sort_unstable();
    assert_eq!(got, MATYPE_DEPENDENT, "set of MA-type-dependent functions changed");

    // KC inherits from TWO different ids, and both matter: ta_regtest's UNSTABLE_MAP
    // sweeps the set it is given and leaves the rest at zero, so a leg whose id is
    // missing there never warms while the convergence envelope tightens around it.
    // The INHERITED row above can name only one source, so assert the pair here.
    {
        let kc = &st["KC"].inherited_from;
        assert!(
            kc.iter().any(|g| g == "EMA") && kc.iter().any(|g| g == "ATR"),
            "KC must inherit from both EMA and ATR, got {kc:?}"
        );
    }

    // SAR is the regression this analysis was narrowed for: it calls MINUS_DM (which owns
    // an unstable period) with a literal period of 1, so it inherits nothing.
    assert!(st["SAR"].inherited_from.is_empty(), "SAR must not inherit MINUS_DM's period");
    assert!(!st["SAR"].unconditional(), "SAR is start-independent");
    assert!(st["SMA"].inherited_from.is_empty() && !st["SMA"].matype_dependent);
}

/// The MA types themselves: what `/functions/matype` publishes must match the same source.
#[test]
fn ma_types_split_into_recursive_and_windowed() {
    let st = stability::classify(&load());
    for name in ["EMA", "KAMA", "MAMA", "T3", "DEMA", "TEMA"] {
        assert!(st[name].unconditional(), "{name} carries an unstable period");
    }
    for name in ["SMA", "WMA", "TRIMA", "HMA"] {
        assert!(!st[name].unconditional(), "{name} is a windowed average");
    }
}
