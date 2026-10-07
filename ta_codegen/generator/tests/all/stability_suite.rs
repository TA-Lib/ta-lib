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

use std::collections::{BTreeSet, HashSet};
use std::path::Path;

use regex::Regex;
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
    assert!(funcs.len() >= 200, "expected the whole input tree, got {}", funcs.len());
    funcs
}

/// Functions that inherit an unstable period through a hard-coded inner call, and the
/// function each one inherits from. Measured: every one of these moves at default params
/// except CRSI, whose default 101-bar rank lookback outlasts both RSI legs at an unstable
/// period of 10; it moves at (14,14,20), 21 -> 25.
/// KC is the only one with TWO sources (EMA for its centre line, ATR for its band), and it
/// shares the ATR one with SUPERTREND -- which inherits it through `atr_lookback()` alone,
/// with no call to `atr()` in the body at all.
/// STC is the one row that also declares an id of its own.
const INHERITED: &[(&str, &str)] = &[
    ("ADOSC", "EMA"),
    ("ADXR", "ADX"),
    ("CKSP", "ATR"),
    ("CRSI", "RSI"),
    ("CVI", "EMA"),
    ("DEMA", "EMA"),
    ("EFI", "EMA"),
    ("ERI", "EMA"),
    ("KC", "ATR"),
    ("MACD", "EMA"),
    ("MACDFIX", "EMA"),
    ("MASSI", "EMA"),
    ("PSO", "EMA"),
    ("RVIR", "RVI"),
    ("SMI", "EMA"),
    ("STC", "EMA"),
    ("STOCHRSI", "RSI"),
    ("SUPERTREND", "ATR"),
    ("TEMA", "EMA"),
    ("TRIX", "EMA"),
    ("TSI", "EMA"),
    ("ZLEMA", "EMA"),
];

/// Functions whose stability is the caller's MA-type choice. Measured: BBANDS, MA,
/// MACDEXT, MAVP, STOCH and STOCHF are stable at their (SMA) defaults and unstable with
/// EMA; APO, PPO, PVO and KDJ default to a recursive average and so move even at
/// defaults.
const MATYPE_DEPENDENT: &[&str] = &[
    "APO", "BBANDS", "BBW", "KDJ", "KSTEXT", "MA", "MACDEXT", "MAVP", "PERCENTB", "PPO",
    "PVO", "STOCH", "STOCHF", "STOCHRSI",
];

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
    assert_eq!(declared.len(), 33, "the measured set of self-declaring functions is 33");
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

    // Owning an id must not shadow an inherited one: ta_regtest sweeps only the ids it is given.
    assert_eq!(st["STC"].inherited_from, ["EMA"], "STC also inherits EMA's period");

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
    for name in ["EMA", "KAMA", "MAMA", "T3", "DEMA", "TEMA", "RMA", "VIDYA"] {
        assert!(st[name].unconditional(), "{name} carries an unstable period");
    }
    for name in ["SMA", "WMA", "TRIMA", "HMA", "ALMA"] {
        assert!(!st[name].unconditional(), "{name} is a windowed average");
    }
}

/// ta_regtest's nightly range sweep fails a function that moves across `startIdx` without
/// an `UNSTABLE_MAP` row, so the map is a measured list. The static derivation, which is
/// what the website publishes, must explain every row and give each function it derives a
/// row.
///
/// A path-dependent function's row serves only the stream K-leg, since the range sweep
/// skips it before reading the map; without one, the K-leg never runs it warm. A
/// MAType-dependent function's rows depend on which MA types its tests select, which the
/// call graph cannot see, so they are allowed, not required.
#[test]
fn derivation_agrees_with_ta_regtest_unstable_map() {
    let funcs = load();
    let st = stability::classify(&funcs);
    let src_path =
        Path::new(env!("CARGO_MANIFEST_DIR")).join("../../src/tools/ta_regtest/test_codegen.c");
    let src = std::fs::read_to_string(&src_path).expect("read test_codegen.c");
    let start = src
        .find("static const UnstableLookup UNSTABLE_MAP[] = {")
        .expect("UNSTABLE_MAP not found in test_codegen.c");
    let body = &src[start..start + src[start..].find("\n};").expect("UNSTABLE_MAP end")];
    let code = Regex::new(r"(?s)/\*.*?\*/").unwrap().replace_all(body, "");
    let rows: Vec<(String, String)> = Regex::new(r#"\{\s*"(\w+)"\s*,\s*TA_FUNC_UNST_(\w+)\s*\}"#)
        .unwrap()
        .captures_iter(&code)
        .map(|c| (c[1].to_string(), c[2].to_string()))
        .collect();
    assert_eq!(
        rows.len(),
        code.matches("TA_FUNC_UNST_").count(),
        "an UNSTABLE_MAP row escaped the parser; the row format changed"
    );
    assert!(rows.iter().any(|r| r.0 == "EMA" && r.1 == "EMA"), "UNSTABLE_MAP parse is vacuous");
    let map: BTreeSet<(String, String)> = rows.into_iter().collect();

    let mut required = BTreeSet::new();
    for f in &funcs {
        let s = &st[&f.name];
        let mut ids: Vec<String> = s.inherited_from.clone();
        if s.intrinsic {
            ids.push(f.name.clone());
        }
        required.extend(ids.into_iter().map(|id| (f.name.clone(), id)));
    }
    let missing: Vec<_> = required.difference(&map).collect();
    assert!(missing.is_empty(), "derived unstable, but no UNSTABLE_MAP row: {missing:?}");
    let unexplained: Vec<_> =
        map.difference(&required).filter(|r| !st[&r.0].matype_dependent).collect();
    assert!(
        unexplained.is_empty(),
        "UNSTABLE_MAP rows the derivation calls start-independent: {unexplained:?}"
    );
}

fn unstable_read_gate_with(
    funcs: &[FuncDef],
    helpers: &ta_codegen_lib::helper_registry::HelperRegistry,
) -> Vec<String> {
    let base = Path::new(env!("CARGO_MANIFEST_DIR")).join("../input");
    let enums = parser::enums::load_enums(&base.join("enums.yaml"));
    stability::validate_unstable_reads(funcs, helpers, &enums).err().unwrap_or_default()
}

fn unstable_read_gate(funcs: &[FuncDef]) -> Vec<String> {
    let base = Path::new(env!("CARGO_MANIFEST_DIR")).join("../input");
    unstable_read_gate_with(funcs, &ta_codegen_lib::helper_registry::HelperRegistry::from_dir(&base))
}

fn lookback_stmts(f: &FuncDef) -> Vec<ta_codegen_lib::ir::Statement> {
    match &f.lookback {
        Some(ta_codegen_lib::ir::LookbackExpr::Code(s)) => s.clone(),
        other => panic!("{}: lookback is not code: {other:?}", f.name),
    }
}

#[test]
fn unstable_read_gate_accepts_the_corpus() {
    assert_eq!(unstable_read_gate(&load()), Vec::<String>::new());
}

/// One mutation per refusal, each on the real corpus, each asserted to be the only
/// thing reported.
#[test]
fn unstable_read_gate_refuses_each_misplaced_read() {
    use ta_codegen_lib::ir::{BinOp, Expr, LookbackExpr, Statement};
    let corpus = load();
    let at = |n: &str| corpus.iter().position(|f| f.name == n).expect(n);
    let (ema, sma) = (at("EMA"), at("SMA"));
    let read = lookback_stmts(&corpus[ema]);
    let Some(Statement::Return { value: Some(read_expr) }) = read.last().cloned() else {
        panic!("EMA's lookback no longer ends on a return")
    };
    let only = |errs: Vec<String>, needle: &str| {
        assert_eq!(errs.len(), 1, "expected one error containing {needle:?}, got {errs:#?}");
        assert!(errs[0].contains(needle), "{:?} lacks {needle:?}", errs[0]);
    };
    let mutated = |edit: &dyn Fn(&mut Vec<FuncDef>)| {
        let mut funcs = corpus.clone();
        edit(&mut funcs);
        unstable_read_gate(&funcs)
    };

    // Every place outside a lookback.
    only(mutated(&|f| f[sma].body.extend(read.clone())), "SMA: the body reads");
    only(
        mutated(&|f| {
            f[sma].has_explicit_private = true;
            f[sma].private_body.extend(read.clone());
        }),
        "SMA: the private body reads",
    );
    only(mutated(&|f| f[sma].display_shift = Some(read.clone())), "SMA: the display shift reads");
    let with_alt = corpus.iter().position(|f| !f.alternates.is_empty()).expect("a function with an _ALT body");
    only(mutated(&|f| f[with_alt].alternates[0].body.extend(read.clone())), "an alternate body reads");
    only(
        mutated(&|f| f[sma].private_param_init.push(("x".into(), read_expr.clone()))),
        "SMA: a private parameter initializer reads",
    );

    // A statement kind the expression walker does not enter.
    only(
        mutated(&|f| {
            f[sma].body.push(Statement::CircBuf(ta_codegen_lib::ir::CircBuf::Init {
                id: "ring".into(),
                layout: ta_codegen_lib::ir::CircBufLayout::Plain(ta_codegen_lib::ir::VarType::Real),
                size: read_expr.clone(),
            }));
        }),
        "SMA: the body reads",
    );

    // A helper.
    let helper = ta_codegen_lib::ir::HelperDef {
        name: "ta_probe".into(),
        return_type: ta_codegen_lib::ir::VarType::Integer,
        params: Vec::new(),
        body: read.clone(),
    };
    only(
        unstable_read_gate_with(&corpus, &ta_codegen_lib::helper_registry::HelperRegistry::from_defs(vec![helper])),
        "helpers/ta_probe: reads",
    );

    // Another function's id in a lookback.
    only(
        mutated(&|f| f[sma].lookback = Some(LookbackExpr::Code(read.clone()))),
        "SMA: the lookback reads TA_FUNC_UNST_EMA; write TA_UNSTABLE",
    );

    // A flag with no read, which also leaves the id unread.
    let errs = mutated(&|f| f[ema].lookback = f[sma].lookback.clone());
    assert_eq!(errs.len(), 2, "{errs:#?}");
    assert!(errs.iter().any(|e| e.contains("EMA: flagged unstable_period, but its lookback does not read")));
    assert!(errs.iter().any(|e| e.contains("TA_FUNC_UNST_EMA: no lookback reads this id")));

    // A read with no flag.
    only(
        mutated(&|f| f[ema].flags.retain(|x| x != "unstable_period")),
        "EMA: its lookback reads TA_FUNC_UNST_EMA, but it is not flagged",
    );

    // Two reads in one expression, and two on one path in two statements.
    let twice = Expr::BinOp(Box::new(read_expr.clone()), BinOp::Add, Box::new(read_expr.clone()));
    let errs = mutated(&|f| f[ema].lookback = Some(LookbackExpr::Code(vec![Statement::Return { value: Some(twice.clone()) }])));
    assert!(errs.iter().any(|e| e.contains("EMA: one lookback expression reads an unstable id more than once")), "{errs:#?}");
    let dx = at("DX");
    assert!(format!("{:?}", corpus[dx].lookback).matches("Return {").count() > 1, "DX's lookback has lost its second return");
    for target in [ema, dx] {
        let own = corpus[target].name.clone();
        only(
            mutated(&|f| {
                let mut stmts = lookback_stmts(&f[target]);
                let extra = Expr::FuncCall("UNSTABLE_PERIOD".into(), vec![Expr::Var(format!("FUNC_UNST_{own}")), Expr::IntLiteral(0)]);
                stmts.insert(0, Statement::Expr(extra));
                f[target].lookback = Some(LookbackExpr::Code(stmts));
            }),
            &format!("{own}: the lookback reads an unstable id more than once on a path"),
        );
    }

    // A read in a lookback statement the expression walker does not enter.
    let errs = mutated(&|f| {
        let mut stmts = lookback_stmts(&f[sma]);
        stmts.insert(
            0,
            Statement::CircBuf(ta_codegen_lib::ir::CircBuf::Init {
                id: "ring".into(),
                layout: ta_codegen_lib::ir::CircBufLayout::Plain(ta_codegen_lib::ir::VarType::Real),
                size: read_expr.clone(),
            }),
        );
        f[sma].lookback = Some(LookbackExpr::Code(stmts));
    });
    assert!(errs.iter().any(|e| e.contains("SMA: the lookback reads an unstable id where the gate cannot name it")), "{errs:#?}");

    // A read whose first argument is not a bare id, and one with no Auto rule.
    let Expr::BinOp(l, op, r) = read_expr.clone() else { panic!("EMA's lookback is no longer a sum") };
    let Expr::FuncCall(callee, read_args) = *r else { panic!("EMA's lookback no longer ends on its read") };
    assert_eq!(read_args.len(), 2, "EMA's read has lost its Auto rule");
    for (bad_args, needle) in [
        (vec![Expr::IntLiteral(5), read_args[1].clone()], "EMA: the lookback reads TA_FUNC_UNST_<2 argument(s)>"),
        (vec![read_args[0].clone()], "EMA: the lookback reads TA_FUNC_UNST_<1 argument(s)>"),
    ] {
        let odd = Expr::BinOp(l.clone(), op.clone(), Box::new(Expr::FuncCall(callee.clone(), bad_args)));
        let errs = mutated(&|f| f[ema].lookback = Some(LookbackExpr::Code(vec![Statement::Return { value: Some(odd.clone()) }])));
        assert!(errs.iter().any(|e| e.contains(needle)), "{errs:#?}");
    }
}
