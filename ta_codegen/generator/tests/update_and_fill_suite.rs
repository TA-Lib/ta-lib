//! `UpdateAndFill` (issue #246): `n` closed bars in one call, in all four
//! backends.
//!
//! The entry point is `n` back-to-back `Update`s and nothing else, so what has
//! to be pinned is not what it computes — `stream_verify` compares every value
//! it writes against batch in all four servers — but the two things that make
//! "and nothing else" true, and that no value comparison can see:
//!
//! 1. **The per-bar rejection is `Update`'s own test, indexed.** The check lives
//!    in the public `Update`, not in the shared step, so the filler re-emits it
//!    rather than paying a call per bar. Re-emitting means two texts that can
//!    drift, so the text is derived from `Update`'s and compared.
//! 2. **It is inside the loop, not a pre-scan.** A whole-array scan before the
//!    loop would satisfy every "it rejects" assertion in the tree and quietly
//!    turn the documented partial commit into all-or-nothing. The shape is
//!    pinned by ORDER — the check sits between the loop head and the step — and
//!    by the check appearing exactly once.
//!
//! Both are cross-backend, so they live together here rather than one copy per
//! per-language suite.

use std::collections::HashMap;
use std::path::PathBuf;
use ta_codegen_lib::helper_registry::HelperRegistry;
use ta_codegen_lib::registry::Registry;
use ta_codegen_lib::streaming;
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
    assert!(out.len() > 150, "the corpus sweep found only {} functions", out.len());
    out
}

fn section(name: &str, lang: &str) -> String {
    let (func, enums) = load(name);
    let registry = Registry::from_dir(&input_dir());
    let helpers = HelperRegistry::from_dir(&input_dir().join("helpers"));
    let full = match lang {
        "c" => backends::c_stream::generate(&func, &enums, &registry, &helpers),
        "rust" => backends::rust_lang::generate(&func, &enums, &registry, &HelperRegistry::empty()),
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

/// The functions that reach every emitter `UpdateAndFill` is generated from:
/// the shared step loop (SMA, and MINUS_DI for a multi-input dual-mode body),
/// the fallible composed step (BBANDS), an integer output over four inputs
/// (CDLDOJI), the dispatch tier's hand-rolled body (MA) and the period bank's
/// (MAVP).
const TIERS: [(&str, &str); 6] = [
    ("sma", "SMA"),
    ("minus_di", "MINUS_DI"),
    ("bbands", "BBANDS"),
    ("cdldoji", "CDLDOJI"),
    ("ma", "MA"),
    ("mavp", "MAVP"),
];

#[test]
fn every_streaming_function_emits_it_in_every_backend() {
    let mut seen = 0usize;
    for name in streaming_funcs() {
        let upper = name.to_uppercase();
        for (lang, needle) in [
            ("c", format!("TA_RetCode TA_{upper}_UpdateAndFill( TA_{upper}_Stream *stream")),
            ("rust", "pub fn update_and_fill(&mut self".to_string()),
            ("java", "public void updateAndFill(".to_string()),
            ("csharp", "public void UpdateAndFill(".to_string()),
        ] {
            let s = section(&name, lang);
            assert!(
                s.contains(&needle),
                "{name}: the {lang} backend emitted no UpdateAndFill"
            );
            seen += 1;
        }
    }
    assert!(seen > 600, "the sweep made only {seen} assertions");
}

/// The per-bar test is `Update`'s, with `[i]` on each bar. Derived from
/// `Update`'s own emitted text rather than spelled again here, so the two
/// cannot drift into disagreeing about which slots are checked.
#[test]
fn the_per_bar_check_is_updates_own_test_indexed() {
    let mut checked = 0usize;
    for (name, upper) in TIERS {
        let (func, _) = load(name);
        let bars = streaming::input_array_names(&func);
        assert!(!bars.is_empty(), "{name}: no bar inputs to check");

        // C: the two conditions, one indexed.
        let c = section(name, "c");
        let want_c: Vec<String> = bars.iter().map(|b| format!("!TA_IS_FINITE( {b} )")).collect();
        let want_ci: Vec<String> =
            bars.iter().map(|b| format!("!TA_IS_FINITE( {b}[i] )")).collect();
        let upd = body_of(&c, &format!("TA_RetCode TA_{upper}_Update( "));
        let fill = body_of(&c, &format!("TA_RetCode TA_{upper}_UpdateAndFill( "));
        // The reject BODY is pinned by `out_range_advance_suite` (it counts the
        // bar it turns down); what matters here is only that the two conditions
        // are the same test.
        assert!(
            upd.contains(&format!("if( {} )", want_c.join(" || "))),
            "{name}: C Update does not carry the expected finite test:\n{upd}"
        );
        assert!(
            fill.contains(&format!("if( {} )", want_ci.join(" || "))),
            "{name}: C UpdateAndFill's per-bar test is not Update's, indexed:\n{fill}"
        );

        // Rust / Java / C#: same relationship, each in its own spelling.
        let r = section(name, "rust");
        let rf = body_of(&r, "pub fn update_and_fill(&mut self");
        let want_r: Vec<String> = bars.iter().map(|b| format!("!{b}[i].is_finite()")).collect();
        assert!(
            rf.contains(&format!("if {} {{", want_r.join(" || "))),
            "{name}: Rust's per-bar test is not the indexed form:\n{rf}"
        );

        let j = section(name, "java");
        let jf = body_of(&j, "public void updateAndFill(");
        let want_j: Vec<String> = bars
            .iter()
            .map(|b| format!("!Double.isFinite({b}[i])"))
            .collect();
        assert!(
            jf.contains(&format!("if( {} )", want_j.join(" || "))),
            "{name}: Java's per-bar test is not the indexed form:\n{jf}"
        );

        let cs = section(name, "csharp");
        let csf = body_of(&cs, "public void UpdateAndFill(");
        let want_cs: Vec<String> = bars
            .iter()
            .map(|b| format!("!double.IsFinite({b}[i])"))
            .collect();
        assert!(
            csf.contains(&format!("if( {} )", want_cs.join(" || "))),
            "{name}: C#'s per-bar test is not the indexed form:\n{csf}"
        );
        checked += 4;
    }
    assert!(checked == 24, "the sweep made {checked} comparisons");
}

/// The check sits INSIDE the one loop, ahead of the step, and the count advance
/// sits after it — which together are what make a rejection commit the bars
/// before it. A pre-scan hoisted above the loop would leave every "it rejects"
/// assertion in the tree green while turning the contract into all-or-nothing,
/// so the shape is pinned by order, not by presence.
#[test]
fn the_check_is_inside_the_loop_and_not_a_pre_scan() {
    for (name, upper) in TIERS {
        let (func, _) = load(name);
        let bars = streaming::input_array_names(&func);
        let first = &bars[0];

        let c = body_of(
            &section(name, "c"),
            &format!("TA_RetCode TA_{upper}_UpdateAndFill( "),
        );
        let test_c = format!("!TA_IS_FINITE( {first}[i] )");
        // Every loop head is followed by its own check and then its own advance.
        // The dispatch tier has two, because its identity arm is loop-invariant
        // and hoisted out of the switch into a loop of its own.
        let loops = if upper == "MA" { 2 } else { 1 };
        let heads: Vec<usize> = c.match_indices("for( i = 0; i < barCount; i++ )").map(|(i, _)| i).collect();
        let checks: Vec<usize> = c.match_indices(test_c.as_str()).map(|(i, _)| i).collect();
        let advances: Vec<usize> = c.match_indices("outRangeCount < TA_MAX_INDEX").map(|(i, _)| i).collect();
        assert_eq!(heads.len(), loops, "{name}: C's UpdateAndFill has {} bar loops, expected {loops}", heads.len());
        assert_eq!(
            checks.len(),
            loops,
            "{name}: C emits the per-bar test {} times for {loops} loop(s) — a pre-scan or a missing check",
            checks.len()
        );
        // Two advances per loop: the rejected bar counts itself (rule U3), then
        // the committed one counts itself.
        assert_eq!(advances.len(), 2 * loops, "{name}: C advances the count {} times for {loops} loop(s)", advances.len());
        for k in 0..loops {
            assert!(
                heads[k] < checks[k] && checks[k] < advances[2 * k] && advances[2 * k] < advances[2 * k + 1],
                "{name}: C loop {k} is not head -> check -> reject advance -> commit advance (a pre-scan, or a bar counted before it is checked)"
            );
        }

        for (lang, needle, loop_head, test) in [
            (
                "rust",
                "pub fn update_and_fill(&mut self",
                "for i in 0..barCount {",
                format!("!{first}[i].is_finite()"),
            ),
            (
                "java",
                "public void updateAndFill(",
                "for( int i = 0; i < barCount; i++ ) {",
                format!("!Double.isFinite({first}[i])"),
            ),
            (
                "csharp",
                "public void UpdateAndFill(",
                "for( int i = 0; i < barCount; i++ )",
                format!("!double.IsFinite({first}[i])"),
            ),
        ] {
            let body = body_of(&section(name, lang), needle);
            let head = body.find(loop_head).expect("the bar loop");
            let check = body.find(&test).expect("the per-bar test");
            assert!(
                head < check,
                "{name}: {lang} hoists the finite test above the loop (a pre-scan):\n{body}"
            );
            assert_eq!(
                body.matches(&test).count(),
                1,
                "{name}: {lang} emits the per-bar test more than once:\n{body}"
            );
        }
    }
}

/// Java caches the multi-output `Value` so `value()` allocates nothing, so the
/// cache has to be refreshed on the way out of a PARTIAL fill too — otherwise
/// `value()` names a bar before the ones the call committed, and disagrees with
/// `outRange()` by exactly the committed bars.
///
/// The cache is gone (#310): a multi-output `update`/`peek`/`value` writes a
/// caller-owned `<N>Out`, so there is no stored instance to keep fresh and
/// nothing for the `finally` to publish. This replaces the refresh test rather
/// than deleting it — the property worth pinning is now the ABSENCE, swept over
/// both managed backends so neither grows one back.
#[test]
fn no_managed_handle_caches_the_multi_output_value() {
    for (func, lang, verb) in [
        ("bbands", "java", "public void updateAndFill("),
        ("macd", "java", "public void updateAndFill("),
        ("stoch", "java", "public void updateAndFill("),
        ("bbands", "csharp", "public void UpdateAndFill("),
    ] {
        let sect = section(func, lang);
        let fill = body_of(&sect, verb);
        assert!(
            !fill.contains("cachedValue") && !fill.contains("finally"),
            "{func}/{lang} updateAndFill still keeps a cache fresh:\n{fill}"
        );
        // The whole handle, not just the filler: a cache re-grown anywhere else
        // would leave this passing while the allocation is back.
        assert!(
            !sect.contains("cachedValue"),
            "{func}/{lang} still declares or writes a cached value"
        );
    }

    // Non-vacuity: these are multi-output handles, so they DO have an out type
    // to have cached. A single-output handle proves nothing here.
    for (func, lang, ty) in [("bbands", "java", "BbandsOut"), ("bbands", "csharp", "BbandsValue")] {
        assert!(
            section(func, lang).contains(ty),
            "{func}/{lang} has no {ty}, so its lack of a cache is not evidence"
        );
    }
}

/// C is the only backend with a bar count and the only one that has to reject
/// an aliased output by hand; the managed three see lengths and can reject a
/// short output, which C cannot. Pinned per backend so a capability is not
/// quietly dropped from the one language that has it.
#[test]
fn each_backend_carries_the_guards_it_can_express() {
    let c = body_of(
        &section("sma", "c"),
        "TA_RetCode TA_SMA_UpdateAndFill( ",
    );
    assert!(c.contains("if( barCount < 0 ) return TA_BAD_PARAM;"), "C: negative count:\n{c}");
    assert!(
        c.contains("(const void *)outReal == (const void *)inReal"),
        "C: the aliasing rejection:\n{c}"
    );

    let r = body_of(&section("sma", "rust"), "pub fn update_and_fill(&mut self");
    assert!(r.contains("outReal.len() < barCount"), "Rust: short output:\n{r}");

    let j = body_of(&section("sma", "java"), "public void updateAndFill(");
    assert!(j.contains("outReal.length < barCount"), "Java: short output:\n{j}");
    assert!(j.contains("(Object)outReal == (Object)inReal"), "Java: aliasing:\n{j}");

    let cs = body_of(&section("sma", "csharp"), "public void UpdateAndFill(");
    assert!(cs.contains("outReal.Length < barCount"), "C#: short output:\n{cs}");
    assert!(cs.contains("outReal.Overlaps(inReal)"), "C#: overlap:\n{cs}");

    // Ragged inputs: only reachable where there is more than one input series.
    let jd = body_of(&section("minus_di", "java"), "public void updateAndFill(");
    assert!(jd.contains("inLow.length != barCount"), "Java: ragged inputs:\n{jd}");
    let rd = body_of(&section("minus_di", "rust"), "pub fn update_and_fill(&mut self");
    assert!(rd.contains("inLow.len() != inHigh.len()"), "Rust: ragged inputs:\n{rd}");
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
fn no_throwing_sub_call_follows_the_cur_capture_in_a_java_step() {
    let mut with_subs: std::collections::BTreeSet<String> = std::collections::BTreeSet::new();
    for name in streaming_funcs() {
        let base = backends::common::camel_words(&name.to_uppercase());
        let s = section(&name, "java");
        let body = body_of(&s, &format!("void {base}StepImpl("));
        // Only the multi-output handles hold a cache, and only they can publish
        // a half-written bar. A single-output `value()` is a field read, and the
        // dispatch tier's `sp.cur_outReal = sub.update(..)` puts the call
        // textually after the field it assigns while still being atomic.
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
                "{name}: a sub-stream call runs after the first cur_* write, so a \
                 rejection there would leave the fields a mix of two bars and \
                 updateAndFill's finally would cache a half-written value:\n{body}"
            );
        }
    }
    // The property is only load-bearing where a sub exists to throw, so the
    // sweep has to have found some — pinned as an exact SET, not a count, so a
    // function leaving it is as loud as one joining.
    //
    // Over the SHIPPED corpus only. `scripts/synth_gate.py` copies its fixtures
    // into input/, and one of them (SYNTH14) is multi-output, composed and
    // streamable, so it legitimately joins this set there. A literal that
    // counted it would turn a correct tree red with a message naming six
    // shipped functions and nothing to do with the change under test — the
    // failure mode `StreamSmokeTest` records against corpus literals, and the
    // reason this one is filtered rather than widened.
    let shipped: std::collections::BTreeSet<&str> =
        with_subs.iter().map(String::as_str).filter(|n| !n.starts_with("synth")).collect();
    let expected: std::collections::BTreeSet<&str> =
        ["bbands", "kc", "macdext", "stoch", "stochf", "stochrsi"].into_iter().collect();
    assert_eq!(
        shipped, expected,
        "the set of multi-output handles driving a sub-stream moved — the pin is \
         stale or the sweep has gone vacuous"
    );
}
