//! The ride-along's structural gates.
//!
//! The runtime floors in `ta_regtest` can only see a server that ANSWERS. They
//! cannot see one that never emitted the leg, and they cannot see a comparison
//! deleted while its counter stays: `fill_bars` is emitted 201 times in the C
//! server and zero times in the other three, and its driver floor has read green
//! that way ever since. So the "every backend still compares" invariant is
//! pinned here, on emitted text, where a missing emitter is loud.

mod common;
use common::{discover_indicators, load_indicator};
use std::path::Path;
use ta_codegen_lib::{ir, parser};

fn corpus() -> (Vec<ir::FuncDef>, std::collections::HashMap<String, ir::EnumDef>) {
    let base = Path::new(env!("CARGO_MANIFEST_DIR")).join("../../ta_codegen/input");
    let enums = parser::enums::load_enums(&base.join("enums.yaml"));
    let funcs: Vec<ir::FuncDef> = discover_indicators()
        .iter()
        .map(|n| load_indicator(n).0)
        .collect();
    (funcs, enums)
}

/// Per backend: the needle that counts a ride comparison, and the one that
/// counts an emitted ride function. Both name the ride's OWN buffers, so a hit
/// cannot come from `stream_verify`, which compares the same way over `b0`/`f0`.
const COMPARE_NEEDLE: [(&str, &str, &str); 4] = [
    ("c", "sv_xtier_ne(sr_b", "static void sr_"),
    ("rust", "sv_xtier_ne(rb", "(core: &Core, params: &Value, endIdx:"),
    ("java", "svXtierNe(rb", "static void rideBody"),
    ("csharp", "SvXtierNe(rb", "static void RideBody"),
];

fn servers(
    funcs: &[ir::FuncDef],
    enums: &std::collections::HashMap<String, ir::EnumDef>,
) -> Vec<(&'static str, String)> {
    vec![
        ("c", ta_codegen_lib::server_gen::generate_c_server(funcs, enums)),
        ("rust", ta_codegen_lib::server_gen::generate_rust_server(funcs, enums)),
        ("java", ta_codegen_lib::server_gen::generate_java_server(funcs, enums)),
        ("csharp", ta_codegen_lib::server_gen::generate_csharp_server(funcs, enums)),
    ]
}

/// Every streaming function rides in every backend, and every one of its real
/// outputs is compared on all three sites (the open anchor, each update, and the
/// fill array). The expected count is DERIVED from the corpus, so it moves with
/// a new indicator instead of dating the day it was written.
#[test]
fn every_backend_compares_every_ride_output() {
    let (funcs, enums) = corpus();
    let streaming: Vec<&ir::FuncDef> = funcs.iter().filter(|f| f.streaming).collect();
    let want_fns = streaming.len();
    let want_cmps: usize = streaming
        .iter()
        .map(|f| {
            3 * f
                .outputs
                .iter()
                .filter(|o| o.param_type != ir::ParamType::Integer)
                .count()
        })
        .sum();
    assert!(want_fns > 0 && want_cmps > 0, "corpus has no streaming function");

    for (lang, src) in servers(&funcs, &enums) {
        let (_, cmp_needle, fn_needle) = COMPARE_NEEDLE
            .iter()
            .find(|(l, _, _)| *l == lang)
            .copied()
            .expect("needle for every backend");
        assert_eq!(
            src.matches(fn_needle).count(),
            want_fns,
            "{lang}: emitted ride functions != streaming functions"
        );
        assert_eq!(
            src.matches(cmp_needle).count(),
            want_cmps,
            "{lang}: ride comparisons != 3 per real output. A deleted comparison \
             leaves its bar counter climbing, so no runtime floor can see this."
        );
    }
}

/// The wire contract is one schema, not four. The driver fails closed on a
/// present `ride_ok` of 0 and skips on absence, so a backend spelling a field
/// differently reads as "not offered" and goes dark silently.
#[test]
fn every_backend_emits_the_same_ride_field_set() {
    let (funcs, enums) = corpus();
    const FIELDS: [&str; 8] = [
        "ride_ok",
        "ride_skip",
        "ride_dedup",
        "ride_open_bars",
        "ride_fill_bars",
        "ride_benign",
        "ride_m",
        "ride_lb",
    ];
    const DIAG: [&str; 5] = [
        "ride_leg",
        "ride_bar",
        "ride_out",
        "ride_batch",
        "ride_stream",
    ];
    for (lang, src) in servers(&funcs, &enums) {
        for f in FIELDS.iter().chain(DIAG.iter()) {
            assert!(
                src.contains(f),
                "{lang}: server never emits the `{f}` ride field"
            );
        }
    }
}

/// Benchmark neutrality, asserted rather than commented. `ta_bench` drives the
/// very same per-function handler, so the ride-along must be emitted strictly
/// after the point the elapsed time is taken. A one-line drift back inside the
/// loop would read as a uniformly slower indicator, never as an artifact.
#[test]
fn the_ride_along_is_emitted_after_the_timed_region() {
    let (funcs, enums) = corpus();
    // (backend, the statement that closes the measurement, the ride call site)
    let probes = [
        ("c", "long elapsed_ns =", "sr_SMA( json, endIdx"),
        ("rust", "let elapsed_ns =", "ride_sma(&core, params"),
        ("java", "long elapsedNs =", "rideSMA(core, json"),
        ("csharp", "long elapsedNs =", "RideSMA(core, p"),
    ];
    for (lang, src) in servers(&funcs, &enums) {
        let (_, timer, call) = probes
            .iter()
            .find(|(l, _, _)| *l == lang)
            .copied()
            .expect("probe for every backend");
        let t = src.find(timer).unwrap_or_else(|| panic!("{lang}: no `{timer}`"));
        let c = src.find(call).unwrap_or_else(|| panic!("{lang}: no `{call}`"));
        assert!(
            c > t,
            "{lang}: the ride-along call is emitted BEFORE the elapsed-time \
             statement, i.e. inside the measured region"
        );
    }
}

/// C only: the whole support block and every per-function body must sit inside
/// one `TA_REF_SERVE` guard. A partial guard still LINKS against the frozen
/// library -- the buffers and helpers name no stream symbol -- so it would build
/// green while adding BSS to the oracle every benchmark ratio divides by.
#[test]
fn the_c_ride_along_is_entirely_inside_the_ref_serve_guard() {
    let (funcs, enums) = corpus();
    let src = ta_codegen_lib::server_gen::generate_c_server(&funcs, &enums);
    let mut depth = 0i32;
    let mut guarded = true;
    for line in src.lines() {
        let t = line.trim_start();
        if t.starts_with("#ifndef TA_REF_SERVE") {
            depth += 1;
        } else if t.starts_with("#endif") && depth > 0 {
            depth -= 1;
        } else if depth == 0 && (t.contains("sr_") || t.contains("g_srKey") || t.contains("RIDE_")) {
            guarded = false;
            break;
        }
    }
    assert!(
        guarded,
        "a ride-along symbol is emitted outside `#ifndef TA_REF_SERVE`"
    );
}
