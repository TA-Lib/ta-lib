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
const COMPARE_NEEDLE: [(&str, &str, &str, &str); 4] = [
    ("c", "sv_xtier_ne(sr_b", "!= sr_ib", "static void sr_"),
    ("rust", "sv_xtier_ne(rb", "!= rib", "(core: &Core, params: &Value, endIdx:"),
    ("java", "svXtierNe(rb", "!= rib", "static void rideBody"),
    ("csharp", "SvXtierNe(rb", "!= rib", "static void RideBody"),
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
    let count_outputs = |want_int: bool| -> usize {
        streaming
            .iter()
            .map(|f| {
                3 * f
                    .outputs
                    .iter()
                    .filter(|o| (o.param_type == ir::ParamType::Integer) == want_int)
                    .count()
            })
            .sum()
    };
    let want_cmps = count_outputs(false);
    let want_int_cmps = count_outputs(true);
    assert!(want_fns > 0 && want_cmps > 0, "corpus has no streaming function");
    // The integer arm is the ONLY comparison the 66 candlestick functions have.
    assert!(
        want_int_cmps > 0,
        "corpus has no integer output: the integer floor below would be vacuous"
    );

    for (lang, src) in servers(&funcs, &enums) {
        let (_, cmp_needle, int_needle, fn_needle) = COMPARE_NEEDLE
            .iter()
            .find(|(l, _, _, _)| *l == lang)
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
        assert_eq!(
            src.matches(int_needle).count(),
            want_int_cmps,
            "{lang}: ride comparisons != 3 per INTEGER output. Every candlestick \
             function has only integer outputs, so dropping this arm leaves them \
             riding and comparing nothing, with full bar counts."
        );
    }
}

/// The wire contract is one schema, not four. The driver fails closed on a
/// present `ride_ok` of 0 and skips on absence, so a backend spelling a field
/// differently reads as "not offered" and goes dark silently.
#[test]
fn every_backend_emits_the_same_ride_field_set() {
    let (funcs, enums) = corpus();
    const FIELDS: [&str; 12] = [
        "ride_ok",
        "ride_skip",
        "ride_dedup",
        "ride_open_bars",
        "ride_fill_bars",
        "ride_benign",
        "ride_m",
        "ride_lb",
        "ride_rej",
        "ride_rc_batch",
        "ride_rc_open",
        "ride_rc_fill",
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

/// The dedup cache is one table shared by every function, so the key must carry
/// the function's IDENTITY. Three backends folded only the name's LENGTH, which
/// is equal for whole families of same-shaped indicators: they collided, took
/// the dedup branch, never opened a stream, and re-reported the first arrival's
/// bar counts, which the driver credits. Every floor stayed green while 48 of
/// 201 functions in Rust and 35 in Java and C# never rode at all.
#[test]
fn every_backend_keys_the_dedup_cache_by_function_identity() {
    let (funcs, enums) = corpus();
    let streaming: Vec<&ir::FuncDef> = funcs.iter().filter(|f| f.streaming).collect();
    // (backend, the key line around the function's own name)
    let key_form: [(&str, &str, &str); 4] = [
        ("c", "srKey = fuzz_hash_bytes(srKey, \"TA_", "\","),
        ("rust", "key = ride_mix_str(key, \"TA_", "\");"),
        ("java", "hash = rideMixStr(hash, \"TA_", "\");"),
        ("csharp", "hash = RideMixStr(hash, \"TA_", "\");"),
    ];
    for (lang, src) in servers(&funcs, &enums) {
        let (_, pre, post) = key_form
            .iter()
            .find(|(l, _, _)| *l == lang)
            .copied()
            .expect("key form for every backend");
        for f in &streaming {
            let needle = format!("{pre}{}{post}", f.name.to_uppercase());
            assert!(
                src.contains(&needle),
                "{lang}: TA_{} does not fold its own name into the dedup key, so it \
                 shares a cache slot with every same-shaped function and silently \
                 stops riding",
                f.name.to_uppercase()
            );
        }
        assert_eq!(
            src.matches(pre).count(),
            streaming.len(),
            "{lang}: dedup keys naming a function != streaming functions"
        );
    }
}

/// A cached verdict is only reusable while the ambient state a stream snapshots
/// at Open is unchanged. C hashes that state directly; the other three fold a
/// generation counter, which is a thing to forget -- Java and C# declared it,
/// folded it, and never incremented it, so every candle-settings and
/// unstable-period sweep reused a verdict computed under the previous settings.
///
/// A NEW ambient mutator must be added to this list, or it will be forgotten the
/// same way.
#[test]
fn every_ambient_mutation_invalidates_the_ride_cache() {
    let (funcs, enums) = corpus();
    const MUTATORS: [&str; 3] = [
        "set_unstable_period",
        "set_candle_settings",
        "restore_candle_default_settings",
    ];
    // (backend, how that backend bumps the generation; C hashes the live state
    //  instead, so its key names sr_ambient and no counter exists to forget)
    let bump: [(&str, &str); 4] = [
        ("c", "sr_ambient("),
        ("rust", "RIDE_GEN.with(|g| g.set("),
        ("java", "rideGen++"),
        ("csharp", "rideGen++"),
    ];
    for (lang, src) in servers(&funcs, &enums) {
        let (_, needle) = bump
            .iter()
            .find(|(l, _)| *l == lang)
            .copied()
            .expect("bump form for every backend");
        if lang == "c" {
            assert!(
                src.contains(needle),
                "c: the ride key no longer hashes the ambient state"
            );
            continue;
        }
        assert_eq!(
            src.matches(needle).count(),
            MUTATORS.len(),
            "{lang}: the ride generation is bumped {} time(s), not once per ambient \
             mutator ({:?}). A cached verdict then survives the state change it was \
             computed under.",
            src.matches(needle).count(),
            MUTATORS
        );
        for m in MUTATORS {
            let at = src
                .find(m)
                .unwrap_or_else(|| panic!("{lang}: no `{m}` handler"));
            let window = &src[at..(at + 2000).min(src.len())];
            assert!(
                window.contains(needle),
                "{lang}: the `{m}` handler does not bump the ride generation"
            );
        }
    }
}

/// Benchmark neutrality, asserted rather than commented. `ta_bench` drives the
/// very same per-function handler, so every ride call must be emitted strictly
/// after the point its OWN handler takes the elapsed time.
///
/// Counting, not a first-match offset: a global `find` of the timer matches the
/// FIRST handler in the file, which precedes every later handler's ride call no
/// matter where it sits, so the check passed for any placement inside them.
/// Here the file is walked in order, and the running ride-call count may never
/// run ahead of `calls_per_handler * timers_seen`. A call that moves before its
/// own timer breaks that at exactly the offset it moved to.
#[test]
fn the_ride_along_is_emitted_after_the_timed_region() {
    let (funcs, enums) = corpus();
    // (backend, the statement that closes the measurement, ride-call line prefix,
    //  the argument text every ride call carries)
    let probes = [
        ("c", "long elapsed_ns =", "sr_", "( json, endIdx", "sr_body"),
        ("rust", "let elapsed_ns =", "ride_", "(&core, params", "ride_body"),
        ("java", "long elapsedNs =", "ride", "(core, json", "rideBody"),
        ("csharp", "long elapsedNs =", "Ride", "(core, p", "RideBody"),
    ];
    for (lang, src) in servers(&funcs, &enums) {
        let (_, timer, call_prefix, call_args, inner) = probes
            .iter()
            .find(|(l, _, _, _, _)| *l == lang)
            .copied()
            .expect("probe for every backend");

        let is_call =
            |t: &str| t.starts_with(call_prefix) && t.contains(call_args) && !t.starts_with(inner);
        let timers = src.lines().filter(|l| l.contains(timer)).count();
        let calls = src.lines().filter(|l| is_call(l.trim_start())).count();
        assert!(timers > 0 && calls > 0, "{lang}: no timer or no ride call found");
        assert_eq!(
            calls % timers,
            0,
            "{lang}: {calls} ride calls over {timers} timed handlers is not a whole \
             number per handler. If a handler legitimately stopped riding, this \
             check needs to learn about it rather than be loosened."
        );
        let per_handler = calls / timers;

        let (mut timers_seen, mut calls_seen) = (0usize, 0usize);
        for (i, line) in src.lines().enumerate() {
            if line.contains(timer) {
                timers_seen += 1;
            }
            if is_call(line.trim_start()) {
                calls_seen += 1;
                assert!(
                    calls_seen <= per_handler * timers_seen,
                    "{lang}: line {}: a ride call is emitted BEFORE its own \
                     handler's elapsed-time statement, i.e. inside the measured \
                     region",
                    i + 1
                );
            }
        }
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
        } else if depth == 0
            && (t.contains("sr_")
                || t.contains("SR_")
                || t.contains("g_sr")
                || t.contains("RIDE_"))
        {
            guarded = false;
            break;
        }
    }
    assert!(
        guarded,
        "a ride-along symbol is emitted outside `#ifndef TA_REF_SERVE`"
    );
}

/// A rejection is a property of the CALL, so batch, `Open` and `OpenAndFill`
/// owe the same answer on it. The value comparison cannot see this: the leg it
/// lives on only runs once the batch reference has SUCCEEDED, so every rejection
/// the corpus makes used to leave the ride with nothing to say.
///
/// Both halves are pinned, because either alone is satisfiable by dead text: a
/// comparison with no call compares two zeros for ever, and a call with no
/// comparison is a rejection nobody read.
#[test]
fn every_backend_compares_the_rejection_code_on_all_three_entry_points() {
    let (funcs, enums) = corpus();
    let want_fns = funcs.iter().filter(|f| f.streaming).count();
    // (backend, the reject-leg Open call, its OpenAndFill, the two comparisons)
    let probes: [(&str, &str, &str, &str, &str); 4] = [
        (
            "c",
            "_Open( &srHR,",
            "_OpenAndFill( &srHR,",
            "srCmp = srRcO == srRcB;",
            "srCmp = srRcF == srRcB;",
        ),
        (
            "rust",
            "r.rc_open = match core.",
            "r.rc_fill = match core.",
            "let mut cmp = r.rc_open == r.rc_batch;",
            "cmp = r.rc_fill == r.rc_batch;",
        ),
        (
            "java",
            "r.rcOpen = rideCode(",
            "r.rcFill = rideCode(",
            "boolean cmpO = r.rcOpen == r.rcBatch",
            "boolean cmpF = r.rcFill == r.rcBatch",
        ),
        (
            "csharp",
            "r.RcOpen = RideCode(",
            "r.RcFill = RideCode(",
            "bool cmpO = r.RcOpen == r.RcBatch",
            "bool cmpF = r.RcFill == r.RcBatch",
        ),
    ];
    for (lang, src) in servers(&funcs, &enums) {
        let (_, open_call, fill_call, open_cmp, fill_cmp) = probes
            .iter()
            .find(|(l, _, _, _, _)| *l == lang)
            .copied()
            .expect("probe for every backend");
        // Both halves of the verdict, spelled WITH their condition. A bare
        // `r.rej++;` is a substring of the guarded form, so it would count the
        // same after the guard is stripped -- and an unguarded bump plus a
        // deleted mismatch flag is the one edit that leaves the runtime floor
        // reading full while the leg compares nothing.
        let verdict: &[(&str, usize)] = match lang {
            "c" => &[
                ("if( srCmp ) srRej++;", 2),
                ("if( !srCmp ) { srOk = 0; srLeg = 3; }", 2),
            ],
            "rust" => &[
                ("if cmp { r.rej += 1; }", 2),
                ("if !cmp { r.ok = false; r.leg = 3; }", 2),
            ],
            "java" => &[
                ("if (cmpO) r.rej++;", 1),
                ("if (cmpF) r.rej++;", 1),
                ("if (!cmpO) { r.ok = false;", 1),
                ("if (!cmpF) { r.ok = false;", 1),
            ],
            _ => &[
                ("if (cmpO) r.Rej++;", 1),
                ("if (cmpF) r.Rej++;", 1),
                ("if (!cmpO) { r.Ok = false;", 1),
                ("if (!cmpF) { r.Ok = false;", 1),
            ],
        };
        for (needle, per_fn) in verdict {
            assert_eq!(
                src.matches(needle).count(),
                want_fns * per_fn,
                "{lang}: `{needle}` appears {} time(s), not {per_fn} per streaming \
                 function. The runtime floor reads the count this guards, so a \
                 bump that outlives its comparison reads as full coverage of a \
                 leg that stopped comparing.",
                src.matches(needle).count()
            );
        }
        for needle in [open_call, fill_call, open_cmp, fill_cmp] {
            assert_eq!(
                src.matches(needle).count(),
                want_fns,
                "{lang}: `{needle}` appears {} time(s), not once per streaming \
                 function. A rejection the batch tier reports and the streaming \
                 tiers do not is then invisible.",
                src.matches(needle).count()
            );
        }
    }
}

/// Java and C# report a rejection by THROWING, and what a caller catches is the
/// class, not the code. The two are separate facts: most sites derive the class
/// from the code through one total map, but several guards construct their
/// exception directly, so two tiers can agree on the code and still hand the
/// caller different types.
#[test]
fn both_throwing_backends_compare_the_exception_class() {
    let (funcs, enums) = corpus();
    let want_fns = funcs.iter().filter(|f| f.streaming).count();
    // (backend, the three captures -- one per entry point, the two comparisons)
    let probes: [(&str, [&str; 3], &str, &str); 2] = [
        (
            "java",
            [
                "clsB = _e.getClass().getName()",
                "clsO = _e.getClass().getName()",
                "clsF = _e.getClass().getName()",
            ],
            "&& clsO.equals(clsB);",
            "&& clsF.equals(clsB);",
        ),
        (
            "csharp",
            [
                "clsB = _e.GetType().FullName",
                "clsO = _e.GetType().FullName",
                "clsF = _e.GetType().FullName",
            ],
            "&& clsO == clsB;",
            "&& clsF == clsB;",
        ),
    ];
    for (lang, src) in servers(&funcs, &enums) {
        let Some((_, captures, open_cmp, fill_cmp)) =
            probes.iter().find(|(l, _, _, _)| *l == lang).copied()
        else {
            continue;
        };
        // One capture per entry point: batch, Open, OpenAndFill.
        for capture in captures {
            assert_eq!(
                src.matches(capture).count(),
                want_fns,
                "{lang}: `{capture}` appears {} time(s), not once per streaming \
                 function -- a class nobody captured is a class nobody compared",
                src.matches(capture).count()
            );
        }
        for needle in [open_cmp, fill_cmp] {
            assert_eq!(
                src.matches(needle).count(),
                want_fns,
                "{lang}: `{needle}` appears {} time(s), not once per streaming \
                 function. Two tiers then agree on the code while raising \
                 different exceptions, which is what a caller actually sees.",
                src.matches(needle).count()
            );
        }
    }
}

/// The reject leg hands all three entry points the SAME range. The success leg
/// deliberately shortens the replay, and carrying that shortening over would be
/// silent: `Open` on `lookback + 1` bars is a call on ZERO bars when the
/// lookback is what was rejected, so it answers the range code rather than the
/// parameter one and every rejection reads as a divergence.
#[test]
fn the_reject_leg_gives_every_entry_point_the_caller_s_own_range() {
    let (funcs, enums) = corpus();
    let want_fns = funcs.iter().filter(|f| f.streaming).count();
    // (backend, the two reject-leg lines, the range every one must carry). Java
    // and C# put the call and its catch on one line, so the assignment anchors
    // both.
    let probes: [(&str, [&str; 2], &str); 4] = [
        ("c", ["_Open( &srHR,", "_OpenAndFill( &srHR,"], ", srM,"),
        (
            "rust",
            ["r.rc_open = match core.", "r.rc_fill = match core."],
            "[..m]",
        ),
        ("java", ["r.rcOpen = rideCode(", "r.rcFill = rideCode("], ", m)"),
        (
            "csharp",
            ["r.RcOpen = RideCode(", "r.RcFill = RideCode("],
            ".AsSpan(0, m)",
        ),
    ];
    for (lang, src) in servers(&funcs, &enums) {
        let (_, anchor, range) = probes
            .iter()
            .find(|(l, _, _)| *l == lang)
            .copied()
            .expect("probe for every backend");
        let mut seen = 0usize;
        for line in src.lines() {
            if anchor.iter().any(|a| line.contains(a)) {
                seen += 1;
                assert!(
                    line.contains(range),
                    "{lang}: a reject-leg call does not carry `{range}`, so the \
                     three entry points no longer see the same input:\n{line}"
                );
            }
        }
        assert_eq!(
            seen,
            want_fns * 2,
            "{lang}: {seen} reject-leg call line(s), not two per streaming function"
        );
    }
}
