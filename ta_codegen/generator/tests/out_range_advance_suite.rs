//! A rejected bar is still counted (rule U3), in all four backends.
//!
//! `Update` refuses a non-finite bar and writes no state — but the bar happened
//! and occupies a position in the series, so the handle's `OutRange` count moves
//! anyway. That is what keeps two handles driven off one feed positionally
//! aligned when only one of them rejects a bar, and it is what makes a partial
//! `UpdateAndFill` readable: the count ends ON the offending bar.
//!
//! `stream_verify` never feeds a non-finite bar, so it cannot see this at all.
//! The per-backend stream suites in C, Java and C# can. What this suite adds
//! over them is reach: one check, all four backends, all 176 functions, on the
//! PR gate — where the Java and C# suites are nightly-only. Three things are
//! pinned here, and the third is the one most likely to regress silently:
//!
//! 1. Every `Update` advances between its finite test and the rejection.
//! 2. Every `UpdateAndFill` does the same at EVERY per-bar site — the dispatch
//!    tier has two, because its identity arm is a loop of its own.
//! 3. No `Peek` advances anything, anywhere. A peek that moved the count is a
//!    peek that wrote the handle, which is the whole guarantee of the receiver
//!    being `const`/`&self`.
//!
//! And the rejections that must NOT advance stay put: the handle/output presence
//! guards, and `UpdateAndFill`'s pre-loop length, count and aliasing checks. Only
//! the per-bar finite test counts a bar it turned down.

use std::collections::HashMap;
use std::path::PathBuf;
use ta_codegen_lib::helper_registry::HelperRegistry;
use ta_codegen_lib::registry::Registry;
use ta_codegen_lib::streaming;
use ta_codegen_lib::{backends, ir, parser};

const LANGS: [&str; 4] = ["c", "rust", "java", "csharp"];

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
    let helpers = HelperRegistry::from_dir(&input_dir());
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

/// The brace-balanced body of the first DEFINITION whose signature line matches
/// `sig`. Matched on the trimmed line rather than on a bare substring, so a call
/// to the same-named entry point on a sub-handle cannot be mistaken for it.
fn body_of(src: &str, sig: impl Fn(&str) -> bool) -> String {
    let mut at = 0usize;
    let start = loop {
        let end = src[at..].find('\n').map(|i| at + i).unwrap_or(src.len());
        if sig(src[at..end].trim()) {
            break at;
        }
        assert!(end < src.len(), "no definition matched");
        at = end + 1;
    };
    let j = src[start..].find('{').expect("definition has a body") + start;
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

/// The one spelling per backend of the saturating advance, of the bare
/// increment, and of a rejection. The advance is matched on its guard: an
/// increment that lost the `MAX_INDEX` bound is a defect of its own (#180) and
/// must not read as an advance here.
fn spellings(lang: &str) -> (&'static str, &'static str, &'static str) {
    match lang {
        "c" => ("outRangeCount < TA_MAX_INDEX", "outRangeCount++", "return TA_BAD_PARAM;"),
        "rust" => (
            "self.out.count < Core::MAX_INDEX",
            "out.count += 1",
            "return Err(RetCode::BadParam);",
        ),
        "java" => (
            "this.outRangeCount < MAX_INDEX",
            "outRangeCount++",
            "throw new TaLibArgumentException(",
        ),
        "csharp" => (
            "outRangeCount < Core.MAX_INDEX",
            "outRangeCount++",
            "throw Core.StreamFailure(",
        ),
        other => panic!("unknown backend {other}"),
    }
}

/// The full non-finite test, as that backend spells it — every bar input, joined
/// exactly as the emitter joins them, so an anchor here cannot drift off a
/// renamed or re-ordered condition and start matching nothing.
fn finite_test(lang: &str, bars: &[String], indexed: bool) -> String {
    let ix = if indexed { "[i]" } else { "" };
    let terms: Vec<String> = bars
        .iter()
        .map(|b| match lang {
            "c" => format!("!TA_IS_FINITE( {b}{ix} )"),
            "rust" => format!("!{b}{ix}.is_finite()"),
            "java" => format!("!Double.isFinite({b}{ix})"),
            "csharp" => format!("!double.IsFinite({b}{ix})"),
            other => panic!("unknown backend {other}"),
        })
        .collect();
    terms.join(" || ")
}

fn entry_sig(lang: &str, upper: &str, verb: &str) -> Box<dyn Fn(&str) -> bool> {
    let (c, rust, java, csharp) = match verb {
        "update" => (
            format!("TA_RetCode TA_{upper}_Update( "),
            "pub fn update(".to_string(),
            " update( ".to_string(),
            " Update( ".to_string(),
        ),
        "fill" => (
            format!("TA_RetCode TA_{upper}_UpdateAndFill( "),
            "pub fn update_and_fill(".to_string(),
            " updateAndFill( ".to_string(),
            " UpdateAndFill( ".to_string(),
        ),
        "peek" => (
            format!("TA_RetCode TA_{upper}_Peek( "),
            "pub fn peek(".to_string(),
            " peek( ".to_string(),
            " Peek( ".to_string(),
        ),
        other => panic!("unknown entry point {other}"),
    };
    match lang {
        "c" => Box::new(move |l: &str| l.starts_with("TA_LIB_API ") && l.contains(&c)),
        "rust" => Box::new(move |l: &str| l.starts_with(&rust)),
        "java" => Box::new(move |l: &str| l.starts_with("public ") && l.contains(&java)),
        "csharp" => Box::new(move |l: &str| l.starts_with("public ") && l.contains(&csharp)),
        other => panic!("unknown backend {other}"),
    }
}

fn positions(hay: &str, needle: &str) -> Vec<usize> {
    hay.match_indices(needle).map(|(i, _)| i).collect()
}

/// Between each finite test and the rejection it guards there is exactly one
/// advance. Searching forward from the test is what places the advance INSIDE
/// the reject block: an advance hoisted above the `if` would land before the
/// anchor and read as missing, which is the answer we want — the accepted bar
/// must not be counted twice.
fn advance_sits_on_every_reject(
    what: &str,
    body: &str,
    test: &str,
    guard: &str,
    reject: &str,
) -> usize {
    let sites = positions(body, test);
    assert!(
        !sites.is_empty(),
        "{what}: the emitted finite test {test:?} is gone, so this gate is measuring nothing:\n{body}"
    );
    for p in &sites {
        let r = body[*p..]
            .find(reject)
            .unwrap_or_else(|| panic!("{what}: no rejection follows the finite test:\n{body}"))
            + p;
        let window = &body[*p..r];
        assert_eq!(
            window.matches(guard).count(),
            1,
            "{what}: the rejected bar is not counted exactly once before the rejection \
             — rule U3 says a bar that happened is counted even when it is refused:\n{body}"
        );
    }
    sites.len()
}

/// The corpus sweep. One test rather than four so the four backends are
/// generated once, not four times over.
#[test]
fn a_rejected_bar_is_counted_by_update_and_by_the_filler_and_never_by_peek() {
    let (mut updates, mut fills, mut peeks, mut guards) = (0usize, 0usize, 0usize, 0usize);
    let mut no_bars = Vec::new();
    for name in streaming_funcs() {
        let upper = name.to_uppercase();
        let (func, _) = load(&name);
        let bars = streaming::input_array_names(&func);
        if bars.is_empty() {
            // No bar input is no U3: there is nothing to test for finiteness and
            // so no rejection that could count a bar.
            no_bars.push(name);
            continue;
        }
        for lang in LANGS {
            let s = section(&name, lang);
            let (guard, increment, reject) = spellings(lang);

            let upd = body_of(&s, entry_sig(lang, &upper, "update"));
            let scalar = finite_test(lang, &bars, false);
            updates += advance_sits_on_every_reject(
                &format!("{name}/{lang} Update"),
                &upd,
                &scalar,
                guard,
                reject,
            );
            // The accepted bar is still counted too, so a "fix" that merely
            // moved the one advance onto the reject path fails here.
            assert!(
                positions(&upd, guard).len() >= 2,
                "{name}: {lang} Update no longer counts BOTH the accepted and the \
                 rejected bar:\n{upd}"
            );
            // U1/U2: the presence guards answer before any bar is looked at, and
            // a call that never reached the series must not move its count.
            let first_advance = positions(&upd, guard)[0];
            let at_test = upd.find(&scalar).expect("the finite test");
            assert!(
                first_advance > at_test,
                "{name}: {lang} Update advances before it has even tested the bar — a \
                 presence guard is counting a bar that was never handed over:\n{upd}"
            );

            let fill = body_of(&s, entry_sig(lang, &upper, "fill"));
            let indexed = finite_test(lang, &bars, true);
            fills += advance_sits_on_every_reject(
                &format!("{name}/{lang} UpdateAndFill"),
                &fill,
                &indexed,
                guard,
                reject,
            );
            // U4-U7: the pre-loop count, length, output and aliasing rejections
            // happen before the first bar loop and must leave the count alone.
            let at_first_test = fill.find(&indexed).expect("the per-bar test");
            assert!(
                positions(&fill, guard)[0] > at_first_test,
                "{name}: {lang} UpdateAndFill advances before its first per-bar test — a \
                 pre-loop guard (short output, ragged inputs, aliasing) is counting a bar \
                 the call never took in:\n{fill}"
            );
            guards += 2;

            let peek = body_of(&s, entry_sig(lang, &upper, "peek"));
            assert!(
                !peek.contains(guard) && !peek.contains(increment),
                "{name}: {lang} Peek moves the range. A peek that counts a bar is a peek \
                 that writes the handle, and the receiver being const/&self is the whole \
                 contract:\n{peek}"
            );
            peeks += 1;
        }
    }
    assert!(
        no_bars.is_empty(),
        "these streaming functions take no bar input, so the sweep silently skipped them \
         — decide what U3 means for them and widen the sweep rather than leaving a hole: \
         {no_bars:?}"
    );
    // Own counters: a refactor that stops reaching these entry points has to
    // fail here rather than pass by checking nothing.
    assert!(updates >= 700, "only {updates} Update reject sites were checked");
    assert!(fills > updates, "only {fills} UpdateAndFill reject sites for {updates} Updates — the dispatch tier's second bar loop is gone");
    assert!(peeks >= 700, "only {peeks} Peek bodies were checked");
    assert!(guards >= 1400, "only {guards} non-advancing guard checks were made");
    println!(
        "checked {updates} Update / {fills} UpdateAndFill reject sites, {peeks} peeks, \
         {guards} guards across {} backends",
        LANGS.len()
    );
}

/// The two tiers that hand-roll their own entry-point bodies, pinned by name and
/// by SITE COUNT. The sweep above would stay green if a tier collapsed its two
/// bar loops into one, or grew a third that forgot the advance; this will not.
#[test]
fn the_hand_rolled_tiers_advance_at_every_bar_loop() {
    // MA dispatches, and hoists its period-1/DISABLED identity arm into a bar
    // loop of its own in C — two per-bar sites there, one everywhere else.
    // MAVP banks a sub-handle per distinct period and has a single loop.
    let mut checked = 0usize;
    for (name, upper, c_fill_sites) in [("ma", "MA", 2usize), ("mavp", "MAVP", 1usize)] {
        let (func, _) = load(name);
        let bars = streaming::input_array_names(&func);
        for lang in LANGS {
            let s = section(name, lang);
            let (guard, increment, reject) = spellings(lang);
            let want = if lang == "c" { c_fill_sites } else { 1 };

            let upd = body_of(&s, entry_sig(lang, upper, "update"));
            assert_eq!(
                advance_sits_on_every_reject(
                    &format!("{name}/{lang} Update"),
                    &upd,
                    &finite_test(lang, &bars, false),
                    guard,
                    reject
                ),
                1,
                "{name}: {lang} Update has more than one finite test"
            );

            let fill = body_of(&s, entry_sig(lang, upper, "fill"));
            assert_eq!(
                advance_sits_on_every_reject(
                    &format!("{name}/{lang} UpdateAndFill"),
                    &fill,
                    &finite_test(lang, &bars, true),
                    guard,
                    reject
                ),
                want,
                "{name}: {lang} UpdateAndFill has the wrong number of per-bar reject \
                 sites — a bar loop was added or merged, and one of them may now count \
                 nothing:\n{fill}"
            );

            let peek = body_of(&s, entry_sig(lang, upper, "peek"));
            assert!(
                !peek.contains(guard) && !peek.contains(increment),
                "{name}: {lang} Peek moves the range:\n{peek}"
            );
            checked += 1;
        }
    }
    assert_eq!(checked, 8, "the hand-rolled tiers were checked {checked} times, expected 8");
}
