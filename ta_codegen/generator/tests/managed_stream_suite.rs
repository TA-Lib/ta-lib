//! Java and C# stream-emitter properties that no runtime gate can see.
//!
//! The two managed backends share a shape the other two do not: a peek frame
//! carries the transition in locals rather than in the handle. Two properties
//! follow from that, and each is invisible to `stream_verify` because each is
//! about text the emitter did NOT produce -- an absent seed, an ordering.
//! A gate on absence has to be swept, and it has to prove it swept something.

use std::collections::{BTreeSet, HashMap};
use std::path::PathBuf;
use ta_codegen_lib::helper_registry::HelperRegistry;
use ta_codegen_lib::registry::Registry;
use ta_codegen_lib::{backends, ir, parser};

fn input_dir() -> PathBuf {
    PathBuf::from(env!("CARGO_MANIFEST_DIR")).join("../input")
}

fn load(name: &str) -> (ir::FuncDef, HashMap<String, ir::EnumDef>) {
    let dir = input_dir().join(name);
    let mut func = parser::yaml::parse_yaml(&dir.join(format!("{name}.yaml")));
    let parsed = parser::c_source::parse_c_source(&dir.join(format!("{name}.c")));
    parser::c_source::wire_parsed_source(&mut func, &parsed);
    let enums = parser::enums::load_enums(&input_dir().join("enums.yaml"));
    (func, enums)
}

/// Every directory under `input/` that declares a stream.
fn streaming_funcs() -> Vec<String> {
    let mut out = Vec::new();
    for entry in std::fs::read_dir(input_dir()).expect("input dir") {
        let entry = entry.expect("dir entry");
        if !entry.path().is_dir() {
            continue;
        }
        let name = entry.file_name().to_string_lossy().to_string();
        if !entry.path().join(format!("{name}.yaml")).is_file() {
            continue;
        }
        let func = parser::yaml::parse_yaml(&entry.path().join(format!("{name}.yaml")));
        if func.streaming {
            out.push(name);
        }
    }
    out.sort();
    assert!(out.len() >= 200, "the corpus sweep found only {} functions", out.len());
    out
}

fn section(name: &str, lang: &str) -> String {
    let (func, enums) = load(name);
    let registry = Registry::from_dir(&input_dir());
    let helpers = HelperRegistry::from_dir(&input_dir());
    let full = match lang {
        "c" => backends::c_stream::generate(&func, &enums, &registry, &helpers),
        "rust" => backends::rust_lang::generate(&func, &enums, &registry, &helpers),
        "java" => backends::java::generate(&func, &enums, &registry, &helpers),
        "csharp" => backends::csharp::generate(&func, &enums, &registry, &helpers),
        other => panic!("unknown backend {other}"),
    };
    match full.find("/**** Streaming API *****/") {
        Some(at) => full[at..].to_string(),
        None => full,
    }
}

/// The body of the first definition whose signature line matches `needle`,
/// brace-balanced.
fn body_of(src: &str, needle: &str) -> String {
    let i = src
        .find(needle)
        .unwrap_or_else(|| panic!("no definition matching {needle:?}"));
    let j = src[i..].find('{').expect("definition has a body") + i;
    let bytes = src.as_bytes();
    let (mut depth, mut k) = (0usize, j);
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
    src[j..=k].to_string()
}

/// A peek frame does not seed an output local from the handle when its own
/// body provably overwrites it before any read (issue #343): a peek commits
/// nothing, so the previous bar's output is never an input to the transition,
/// and the seed was one dead field load per output per call. Swept over both
/// managed backends: C writes the output through an out-param and has no such
/// local, and Rust deletes the wholly dead local outright — declaration and
/// stores together (issue #353, gated in rust_stream_suite). Exact-set in both
/// directions: a frame that grows a seed back fails, and so does a change
/// that silently drops the one seed the analysis deliberately keeps — IMI,
/// whose sole store sits inside the period loop, and the IR cannot prove a
/// loop body runs.
#[test]
fn no_managed_peek_seeds_a_dead_output_local() {
    for (lang, needle) in [("java", " peek("), ("csharp", " Peek(")] {
        let mut swept = 0usize;
        let mut seedless = 0usize;
        let mut seeded: BTreeSet<String> = BTreeSet::new();
        for name in streaming_funcs() {
            let (func, _) = load(&name);
            let sect = section(&name, lang);
            if !sect.contains(needle) {
                continue;
            }
            let body = body_of(&sect, needle);
            swept += 1;
            let mut any = false;
            for out in &func.outputs {
                let seed = format!("cur_{} = sp.cur_{};", out.name, out.name);
                if body.contains(&seed) {
                    any = true;
                    seeded.insert(name.clone());
                }
            }
            if !any {
                seedless += 1;
            }
        }
        // Non-vacuity floors: the sweep must have found the corpus AND the
        // subject. A needle that stops matching peeks would zero `swept`; an
        // emitter that re-grew every seed would zero `seedless`.
        assert!(swept >= 200, "{lang}: swept only {swept} peek frames");
        assert!(seedless >= 200, "{lang}: only {seedless} seed-free frames");
        let expected: BTreeSet<String> = ["imi".to_string()].into_iter().collect();
        assert_eq!(
            seeded, expected,
            "{lang}: peek frames seeding an output local from the handle"
        );
    }
}
/// `value(out)` must name the last COMMITTED bar on every exit, the throwing
/// ones included. Since #310 it reads `cur_*` straight through to the caller's
/// sink, so the fields ARE the answer — which is only sound if a throw out of
/// the middle of a bar leaves them on the PREVIOUS bar, and that holds because
/// a step writes its `sp.cur_<out>` fields last, after every sub-stream call.
///
/// The one thing that can throw mid-bar is a sub-stream rejecting a computed
/// intermediate (the composed tier's documented hole), so the property to pin is
/// exactly: no sub call after the first `cur_*` write. Without it a rejection
/// leaves the fields a mix of two bars and the next `value(out)` hands that
/// mixture out as a reading.
#[test]
fn no_throwing_sub_call_follows_the_cur_capture_in_a_managed_step() {
    for lang in ["java", "csharp"] {
        let mut with_subs: std::collections::BTreeSet<String> = std::collections::BTreeSet::new();
        for name in streaming_funcs() {
            let upper = name.to_uppercase();
            let base = if lang == "java" {
                backends::common::camel_words(&upper)
            } else {
                backends::common::pascal_words(&upper)
            };
            let s = section(&name, lang);
            let body = body_of(&s, &format!("void {base}StepImpl("));
            // Only the multi-output handles hold a cache, and only they can
            // publish a half-written bar. A single-output `value()` is a field
            // read, and the dispatch tier's `sp.cur_outReal = sub.update(..)`
            // puts the call textually after the field it assigns while still
            // being atomic.
            if load(&name).0.outputs.len() < 2 {
                continue;
            }
            let Some(first_cur) = body.find("sp.cur_") else {
                continue;
            };
            let last_sub = ["sp.sub", "subOut"]
                .iter()
                .filter_map(|p| body.rfind(p))
                .max();
            if let Some(last_sub) = last_sub {
                with_subs.insert(name.to_string());
                assert!(
                    last_sub < first_cur,
                    "{name}: a sub-stream call runs after the first cur_* write in {lang}, so a \
                     rejection there would leave the fields a mix of two bars and the \
                     next value read would hand that mixture out as a reading:\n{body}"
                );
            }
        }
        // The property is only load-bearing where a sub exists to throw, so the
        // sweep has to have found some — pinned as an exact SET, not a count, so a
        // function leaving it is as loud as one joining.
        //
        // Over the SHIPPED corpus only. `scripts/synth_gate.py` copies its fixtures
        // into input/, and one of them (SYNTH14) is multi-output, composed and
        // streamable, so it legitimately joins this set there — a run under the
        // synth gate would otherwise redden with a message naming shipped
        // functions and nothing to do with the change under test.
        let shipped: std::collections::BTreeSet<&str> =
            with_subs.iter().map(String::as_str).filter(|n| !n.starts_with("synth")).collect();
        let expected: std::collections::BTreeSet<&str> =
            ["bbands", "kc", "kdj", "macdext", "stoch", "stochf", "stochrsi"].into_iter().collect();
        assert_eq!(
            shipped, expected,
            "the set of multi-output handles driving a sub-stream moved in {lang} — the pin is \
             stale or the sweep has gone vacuous"
        );
    }
}