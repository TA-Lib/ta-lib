//! Only an ACCEPTED bar advances the range, in all four backends (#384).
//!
//! `Update` refuses a non-finite bar, writes no state, and moves no count: the
//! rejection costs the caller nothing but the call. Counting a bar the caller
//! decided not to feed is the `advance` call's job — `TA_<N>_Advance`,
//! `advance()`, `advance()`, `Advance()` — which is what leaves the retry
//! expressible.
//!
//! `stream_verify` never feeds a non-finite bar, so it cannot see this at all.
//! The per-backend stream suites in C, Java and C# can. What this suite adds
//! over them is reach: one check, all four backends, every streaming function,
//! on the PR gate — where the Java and C# suites are nightly-only. Three things
//! are pinned here, and the last is the one most likely to regress silently:
//!
//! 1. No advance between a `Update`'s finite test and the rejection it guards —
//!    the window an advance would be re-added into.
//! 2. The accepted bar still advances, after the finite test. A "fix" that
//!    deleted the advance outright, or hoisted it above the presence guards,
//!    fails here.
//! 3. No `Peek` advances anything, anywhere. A peek that moved the count is a
//!    peek that wrote the handle, which is the whole guarantee of the receiver
//!    being `const`/`&self`.
//!
//! `advance` is emitted per handle in all four backends (#387) and swept here
//! in all four. `every_c_handle_answers_its_own_range` below covers what only C
//! has: a range READER whose two out-parameters can be paired the wrong way
//! round.

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
    assert!(out.len() >= 200, "the corpus sweep found only {} functions", out.len());
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
fn finite_test(lang: &str, bars: &[String]) -> String {
    let terms: Vec<String> = bars
        .iter()
        .map(|b| match lang {
            "c" => format!("!TA_IS_FINITE( {b} )"),
            "rust" => format!("!{b}.is_finite()"),
            "java" => format!("!Double.isFinite({b})"),
            "csharp" => format!("!double.IsFinite({b})"),
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

/// Between each finite test and the rejection it guards there is no advance at
/// all. Searching forward from the test is what makes the window the reject
/// block itself — the one place an advance could be re-added and still be
/// reached only by a refused bar.
///
/// Both anchors are asserted present, because a window that matched nothing
/// would satisfy "no advance in it" for the wrong reason.
fn no_advance_on_any_reject(
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
            0,
            "{what}: the refused bar is counted. A rejection changes nothing at all \
             (#384) — counting a bar the caller declined to feed is what \
             the advance call is for:\n{body}"
        );
    }
    sites.len()
}

/// The per-handle `advance`, in every backend. Matched on its signature and on
/// the saturating guard in its body, so an accessor that lost the `MAX_INDEX`
/// bound (#180) does not read as present.
fn advance_entry_sig(lang: &str, upper: &str) -> Box<dyn Fn(&str) -> bool> {
    match lang {
        "c" => {
            let c = format!("TA_RetCode TA_{upper}_Advance( ");
            Box::new(move |l: &str| l.starts_with("TA_LIB_API ") && l.contains(&c))
        }
        "rust" => Box::new(|l: &str| l.starts_with("pub fn advance(&mut self) {")),
        "java" => Box::new(|l: &str| l.starts_with("public void advance() {")),
        "csharp" => Box::new(|l: &str| l.starts_with("public void Advance()")),
        other => panic!("unknown backend {other}"),
    }
}

/// The corpus sweep. One test rather than four so the four backends are
/// generated once, not four times over.
#[test]
fn only_an_accepted_bar_advances_the_range() {
    let (mut updates, mut peeks, mut guards) = (0usize, 0usize, 0usize);
    let mut advancers = 0usize;
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
            let scalar = finite_test(lang, &bars);
            updates += no_advance_on_any_reject(
                &format!("{name}/{lang} Update"),
                &upd,
                &scalar,
                guard,
                reject,
            );
            // The accepted bar IS still counted, so deleting the advance
            // outright fails here rather than passing the check above.
            let advances = positions(&upd, guard);
            assert!(
                !advances.is_empty(),
                "{name}: {lang} Update no longer counts the accepted bar:\n{upd}"
            );
            // U1/U2: the presence guards answer before any bar is looked at, and
            // a call that never reached the series must not move its count.
            let at_test = upd.find(&scalar).expect("the finite test");
            assert!(
                advances[0] > at_test,
                "{name}: {lang} Update advances before it has even tested the bar — a \
                 presence guard is counting a bar that was never handed over:\n{upd}"
            );

            guards += 1;

            // The call that DOES count a skipped bar, and the only one left that
            // moves the range without a bar.
            let adv = body_of(&s, advance_entry_sig(lang, &upper));
            assert!(
                adv.contains(guard),
                "{name}: {lang} advance() does not move the count under the \
                 MAX_INDEX guard:\n{adv}"
            );
            advancers += 1;

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
    assert!(peeks >= 700, "only {peeks} Peek bodies were checked");
    assert!(guards >= 700, "only {guards} non-advancing guard checks were made");
    // Its own counter, not a share of the others: a sweep that stopped reaching
    // `advance` would still saturate the Update and Peek ones.
    assert!(advancers >= 800, "only {advancers} advance() bodies were checked");
    println!(
        "checked {updates} Update reject sites, {peeks} peeks, {guards} guards, \
         {advancers} advance() bodies across {} backends",
        LANGS.len()
    );
}

/// C's range READER, which the sweep above has no counterpart for in the other
/// three backends: `out_range()` hands back one value, while `TA_<N>_OutRange`
/// fills two out-parameters that a swapped pair would populate silently — the
/// count would read as a begIdx and every comparison would still be an int.
///
/// The whole prototype and the whole guard are matched, not a prefix and a bare
/// `return`: a body with only `if( !stream )` still returns `TA_BAD_PARAM` and
/// still assigns both fields, so a narrowed guard reads green against anything
/// weaker — and writing through a NULL out-parameter is the class the typed
/// accessors exist to make unreachable. `OUT_META` in `indicator_variants_suite`
/// pins the batch tier's pair the same way, for the same bug.
#[test]
fn every_c_handle_answers_its_own_range() {
    let mut readers = 0usize;
    for name in streaming_funcs() {
        let upper = name.to_uppercase();
        let s = section(&name, "c");
        assert!(
            !s.contains("TA_StreamOutRange") && !s.contains("TA_StreamAdvance"),
            "{name}: the void * accessor is still emitted"
        );

        let want = format!(
            "TA_LIB_API TA_RetCode TA_{upper}_OutRange( const TA_{upper}_Stream *stream, \
             int *outBegIdx, int *outNBElement )"
        );
        let body = body_of(&s, move |l: &str| l == want);
        assert!(
            body.contains("*outBegIdx = stream->outRangeBegIdx;")
                && body.contains("*outNBElement = stream->outRangeCount;"),
            "{name}: OutRange does not answer the head in its declared order:\n{body}"
        );
        assert!(
            body.contains("if( !stream || !outBegIdx || !outNBElement ) return TA_BAD_PARAM;"),
            "{name}: OutRange does not reject all three NULLs:\n{body}"
        );

        let adv = body_of(&s, advance_entry_sig("c", &upper));
        assert!(
            adv.contains("if( !stream ) return TA_BAD_PARAM;"),
            "{name}: Advance takes a NULL handle:\n{adv}"
        );
        readers += 1;
    }
    // Its own counter, not a share of the sweep's: that one saturates on the
    // three backends this test cannot see.
    assert!(readers >= 200, "only {readers} C range accessor pairs were checked");
    println!("checked {readers} C OutRange/Advance pairs");
}
