//! FMA fusion-site consistency guard.
//!
//! Every backend fuses `a*b + c` at the sites the shared `fma::fuse_operands`
//! detector selects (see `backends/fma.rs`, PR #96). Cross-language bit-parity
//! therefore depends on the three independent translations (C, Rust, Java)
//! identifying the *same* sites. The Rust backend threads its own render context
//! into the detector; C and Java rebuild the equivalent name-sets from the same
//! IR body via `build_fma_var_sets`, but — because they emit typed declarations
//! and don't otherwise track variable types — they seed the range/count index
//! vars uniformly with the unguarded set instead of the guarded/unguarded split
//! the Rust context uses per variant.
//!
//! This test proves that seed difference is immaterial: for every function body,
//! `fuse_operands` makes the identical decision at every `a*b+c` site whether the
//! context was built with the guarded (2) or unguarded (4) index-param seeds. So
//! C/Java (uniform unguarded seeds) fuse exactly the sites Rust does (per-variant
//! seeds). It also confirms the 26 fusion-candidate functions actually fuse.

use std::cell::Cell;
use std::path::Path;

use ta_codegen_lib::backends::fma;
use ta_codegen_lib::ir::{self, Expr, Statement};
use ta_codegen_lib::parser;
use ta_codegen_lib::streaming;

/// Load a function fully wired the way production does (body, private_body,
/// has_explicit_private, lookback).
fn load_func_full(name: &str) -> ir::FuncDef {
    let base = Path::new(env!("CARGO_MANIFEST_DIR"));
    let yaml_path = base.join(format!("../../ta_codegen/input/{name}/{name}.yaml"));
    let c_path = base.join(format!("../../ta_codegen/input/{name}/{name}.c"));
    let mut func_def = parser::yaml::parse_yaml(&yaml_path);
    let parsed = parser::c_source::parse_c_source(&c_path);
    parser::c_source::wire_parsed_source(&mut func_def, &parsed);
    func_def
}

/// Discover every indicator directory under `ta_codegen/input/` (a subdir that
/// contains `<name>/<name>.c` and `<name>/<name>.yaml`; skips `helpers/`, etc.).
fn all_function_names() -> Vec<String> {
    let base = Path::new(env!("CARGO_MANIFEST_DIR")).join("../../ta_codegen/input");
    let mut names = Vec::new();
    for entry in std::fs::read_dir(&base).expect("input dir") {
        let entry = entry.expect("dir entry");
        if !entry.file_type().map(|t| t.is_dir()).unwrap_or(false) {
            continue;
        }
        let name = entry.file_name().to_string_lossy().to_string();
        let dir = entry.path();
        if dir.join(format!("{name}.c")).is_file() && dir.join(format!("{name}.yaml")).is_file() {
            names.push(name);
        }
    }
    names.sort();
    names
}

/// Walk every `a*b+c` site in `body` and, under two FMA contexts, tally how many
/// fuse under each and how many disagree. Returns `(fused_a, fused_b, disagreements)`.
fn compare_fusion(body: &[Statement], a: &fma::FmaCtx, b: &fma::FmaCtx) -> (usize, usize, usize) {
    let fused_a = Cell::new(0usize);
    let fused_b = Cell::new(0usize);
    let disagree = Cell::new(0usize);
    for stmt in body {
        streaming::walk_stmt_exprs(stmt, &mut |top| {
            streaming::walk_expr(top, &mut |sub| {
                if let Expr::BinOp(l, op, r) = sub {
                    let fa = fma::fuse_operands(l, op, r, a).is_some();
                    let fb = fma::fuse_operands(l, op, r, b).is_some();
                    if fa {
                        fused_a.set(fused_a.get() + 1);
                    }
                    if fb {
                        fused_b.set(fused_b.get() + 1);
                    }
                    if fa != fb {
                        disagree.set(disagree.get() + 1);
                    }
                }
            });
        });
    }
    (fused_a.get(), fused_b.get(), disagree.get())
}

/// Total fused sites in a body under one context (guarded seeds).
fn fused_count(body: &[Statement], ctx: &fma::FmaCtx) -> usize {
    let n = Cell::new(0usize);
    for stmt in body {
        streaming::walk_stmt_exprs(stmt, &mut |top| {
            streaming::walk_expr(top, &mut |sub| {
                if let Expr::BinOp(l, op, r) = sub {
                    if fma::fuse_operands(l, op, r, ctx).is_some() {
                        n.set(n.get() + 1);
                    }
                }
            });
        });
    }
    n.get()
}

/// Guarded vs unguarded index-param seeds must yield identical fusion decisions
/// at every site of every function body — the property that lets C/Java seed
/// uniformly yet fuse exactly the sites Rust does per variant.
#[test]
fn fma_fusion_is_seed_invariant_across_all_functions() {
    let mut checked = 0usize;
    for name in all_function_names() {
        let f = load_func_full(&name);
        for body in [&f.body, &f.private_body] {
            let g = fma::build_fma_var_sets(body, &f.outputs, &fma::GUARDED_INDEX_SEEDS);
            let u = fma::build_fma_var_sets(body, &f.outputs, &fma::UNGUARDED_INDEX_SEEDS);
            let (fa, fb, disagree) = compare_fusion(body, &g.view(), &u.view());
            assert_eq!(
                disagree, 0,
                "{name}: {disagree} site(s) fuse differently under guarded vs unguarded seeds \
                 (guarded fused {fa}, unguarded fused {fb}) — C/Java would desync from Rust"
            );
        }
        checked += 1;
    }
    assert!(checked >= 150, "expected ~161 functions, only checked {checked}");
}

/// The known fusion-candidate functions must actually fuse at least one site
/// (guards against the detector silently going dark for all of them).
#[test]
fn fma_fusion_fires_for_known_candidates() {
    // The 26 functions whose generated code contains a fused site (matches the
    // Rust `mul_add` / C `fma()` / Java `Math.fma` inventory).
    const FUSING: &[&str] = &[
        "adosc", "bbands", "cdlabandonedbaby", "cdlmorningdojistar", "cdlmorningstar",
        "cdlpiercing", "cdlthrusting", "dema", "ht_dcperiod", "ht_dcphase", "ht_phasor",
        "ht_sine", "ht_trendline", "ht_trendmode", "kama", "linearreg", "macd", "macdfix",
        "mama", "sar", "sarext", "t3", "tema", "trix", "tsf", "wclprice",
    ];
    for name in FUSING {
        let f = load_func_full(name);
        let u = fma::build_fma_var_sets(&f.private_body, &f.outputs, &fma::UNGUARDED_INDEX_SEEDS);
        let n = fused_count(&f.private_body, &u.view());
        assert!(n > 0, "{name}: expected >=1 fused a*b+c site, found 0");
    }
}
