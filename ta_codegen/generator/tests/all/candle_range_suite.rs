//! Render pins for the candle range arms in the generated backends.
//!
//! `ta_candlerange` has more source texts than anyone expects. C is the
//! reference and spells the Shadows arm `TA_UPPERSHADOW + TA_LOWERSHADOW`
//! (`ta_utility.h`); the algebraically equal `(high - low) - |close - open|`
//! rounds differently on any bar whose low sits below half its high, which is
//! where Sterbenz' lemma stops making the subtractions exact (#217).
//!
//! Two of those texts live in `backends/rust_lang.rs` as hardcoded `format!`
//! strings, used whenever the helper call sits INSIDE a larger expression and
//! so cannot be hoisted to a `_candlerange_N` temporary rendered from
//! `input/helpers/candlestick.c`. Every shipped candlestick hits that path —
//! the call is inside an `if` condition — while SYNTH7/SYNTH8 do not, because
//! they assign the helper to a local. That is why fixing the helper body and
//! the Java/C# duplicates left Rust divergent with every gate green.
//!
//! These pins read the arms directly, so they fail on the rendering rather than
//! waiting for a value gate to notice a 1-ULP shift in a 3-valued output —
//! which, per #217, it essentially never does. Both renderings are covered: the
//! inline arms and the hoisted statement arms from the helper body.

use crate::common;
use std::collections::HashMap;
use std::path::PathBuf;
use ta_codegen_lib::{backends, ir, parser};

fn input_dir() -> PathBuf {
    PathBuf::from(env!("CARGO_MANIFEST_DIR")).join("../input")
}

fn load_indicator(name: &str) -> (ir::FuncDef, HashMap<String, ir::EnumDef>) {
    let dir = input_dir().join(name);
    let mut func = parser::yaml::parse_yaml(&dir.join(format!("{name}.yaml")));
    let parsed = parser::c_source::parse_c_source(&dir.join(format!("{name}.c")));
    parser::c_source::wire_parsed_source(&mut func, &parsed);
    let enums = parser::enums::load_enums(&input_dir().join("enums.yaml"));
    (func, enums)
}

/// Whitespace-collapsed so the pins survive whatever line breaks
/// `formatter.rs` chooses.
fn collapse(s: &str) -> String {
    s.split_whitespace().collect::<Vec<_>>().join(" ")
}

#[derive(Clone, Copy)]
enum Lang {
    Rust,
    Java,
    CSharp,
    Ts,
}

/// Generated source for one indicator, in one backend.
fn generated(name: &str, lang: Lang) -> String {
    let (func, enums) = load_indicator(name);
    let registry = common::make_registry();
    let helpers = common::make_helpers();
    let full = match lang {
        Lang::Rust => backends::rust_lang::generate(&func, &enums, registry, helpers),
        Lang::Java => backends::java::generate(&func, &enums, registry, helpers),
        Lang::CSharp => backends::csharp::generate(&func, &enums, registry, helpers),
        Lang::Ts => backends::ts::generate(&func, &enums, registry, helpers),
    };
    collapse(&full)
}

fn rust_source(name: &str) -> String {
    generated(name, Lang::Rust)
}

/// Every `match <SET>_rangeType { .. }` arm block in `src`, brace-balanced.
fn range_type_matches(src: &str) -> Vec<String> {
    let bytes = src.as_bytes();
    let mut out = Vec::new();
    let mut from = 0usize;
    while let Some(hit) = src[from..].find("_rangeType {") {
        let open = from + hit + "_rangeType ".len();
        let (mut depth, mut k) = (0usize, open);
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
        out.push(src[open..=k].to_string());
        from = open + 1;
    }
    out
}

/// Split one range-type block into its Shadows arm and its fall-through arm.
/// Each block has exactly one `2 =>` and one `_ =>`, and the Shadows arm runs
/// up to the fall-through.
fn shadows_and_default(block: &str) -> (String, String) {
    let a2 = block
        .find("2 =>")
        .unwrap_or_else(|| panic!("Shadows needs its own arm, not folded into `_`: {block}"));
    let d = block
        .rfind("_ =>")
        .unwrap_or_else(|| panic!("no fall-through arm: {block}"));
    assert!(a2 < d, "arms out of order: {block}");
    (block[a2..d].to_string(), block[d..].to_string())
}

/// `upper + lower` is the only spelling with two colour bounds and no `abs`;
/// the #217 form is the exact opposite on both counts. Index-agnostic, so it
/// holds at `i`, `i - 3`, `sp.lag2_inClose`, ... alike, and it reads the same
/// for the inline and the hoisted statement rendering.
fn assert_shadows_arm_is_upper_plus_lower(arm: &str, ctx: &str) {
    assert!(
        colour_helper_calls(arm) + colour_selects(arm) + branchy_colour_bounds(arm) >= 2,
        "{ctx}: Shadows arm has no max/min select, so it cannot be upper + lower \
         -- this is the #217 spelling: {arm}"
    );
    assert!(
        !arm.contains("abs"),
        "{ctx}: Shadows arm takes an absolute value; upper + lower does not \
         -- this is the #217 spelling: {arm}"
    );
}

fn assert_default_arm_is_zero(arm: &str, ctx: &str) {
    // TA_CANDLERANGE's innermost ternary falls through to 0, and
    // ta_global.c documents that an out-of-domain rangeType measures every
    // range as zero. Folding Shadows into `_` answers it with a shadow width.
    assert!(
        arm.contains("0.0") && !arm.contains(">=") && !arm.contains("abs"),
        "{ctx}: fall-through arm must be plain 0.0: {arm}"
    );
}

#[test]
fn rust_candlerange_spells_shadows_as_upper_plus_lower() {
    let s = rust_source("cdldoji");
    let blocks = range_type_matches(&s);
    assert!(
        !blocks.is_empty(),
        "cdldoji must render range-type matches -- if this fires, the hoisting \
         rules changed and these pins moved"
    );
    for block in &blocks {
        let (shadows, default) = shadows_and_default(block);
        assert_shadows_arm_is_upper_plus_lower(&shadows, "cdldoji");
        assert_default_arm_is_zero(&default, "cdldoji");
    }
}

#[test]
fn rust_covers_both_the_inline_and_the_hoisted_rendering() {
    // The bug lived only in the inline arms, because the hoisted ones render
    // from the helper body. A suite that saw just one of the two would have
    // missed it, so assert both shapes are actually present here.
    let s = rust_source("cdldoji");
    assert!(
        s.contains("_candlerange_"),
        "expected hoisted statement arms from input/helpers/candlestick.c"
    );
    assert!(
        s.contains("else { match BodyDoji_rangeType {"),
        "expected the inline arms -- the ta_candleaverage avgPeriod == 0 fallback"
    );
}

#[test]
fn java_and_csharp_also_spell_shadows_as_upper_plus_lower() {
    // The same arm has now been fixed twice in two different places: #217
    // caught java.rs and csharp.rs, and their hardcoded `format!` strings are
    // no more connected to the helper body than Rust's were. Pin all three so
    // the next duplicate cannot regress alone.
    for (lang, label) in [(Lang::Java, "java"), (Lang::CSharp, "csharp"), (Lang::Ts, "ts")] {
        let s = generated("cdldoji", lang);
        assert!(
            s.contains("(inHigh[i] - (((inClose[i]) >= (inOpen[i])) ? (inClose[i]) : (inOpen[i])))"),
            "{label}: upper shadow must be high - max(close, open) — see #217"
        );
        assert!(
            s.contains("((((inClose[i]) >= (inOpen[i])) ? (inOpen[i]) : (inClose[i])) - inLow[i])"),
            "{label}: lower shadow must be min(close, open) - low — see #217"
        );
    }
}

#[test]
fn no_candlestick_carries_the_divergent_shadows_spelling() {
    // Corpus sweep. The inline arms are reached wherever the helper call sits in
    // a condition, which is every candlestick, at whatever bar index that
    // pattern happens to read -- `i - 3` and `sp.lag2_inClose` included.
    let mut checked = 0usize;
    for entry in std::fs::read_dir(input_dir()).expect("input dir") {
        let dir = entry.expect("dir entry").path();
        let Some(name) = dir.file_name().map(|n| n.to_string_lossy().to_string()) else {
            continue;
        };
        if !name.starts_with("cdl") || !dir.join(format!("{name}.yaml")).exists() {
            continue;
        }
        let s = rust_source(&name);
        for block in range_type_matches(&s) {
            let (shadows, default) = shadows_and_default(&block);
            assert_shadows_arm_is_upper_plus_lower(&shadows, &name);
            assert_default_arm_is_zero(&default, &name);
        }
        checked += 1;
    }
    assert!(checked >= 55, "expected the candlestick corpus, saw {checked}");
}

/// No DIVSD by a selected 2.0 or 1.0 per candle average, inline or hoisted.
#[test]
fn rust_candleaverage_halves_by_multiplying() {
    let s = rust_source("cdldoji");
    assert!(
        s.contains("* (if (BodyDoji_rangeType) == 2 { 0.5 } else { 1.0 })"),
        "the Shadows average should halve by a multiply: {s}"
    );
    assert!(!s.contains("{ 2.0 } else { 1.0 }"), "no divide by a selected 2.0/1.0 may remain: {s}");
}

/// The TypeScript stream step reads its bars as scalars, so its candle range is
/// a second spelling the batch pins above cannot answer for.
#[test]
fn the_ts_stream_step_spells_shadows_as_upper_plus_lower() {
    let s = generated("cdldoji", Lang::Ts);
    let stream = &s[s.find("/**** Streaming API *****/").expect("cdldoji streams")..];
    assert!(
        stream.contains("(inHigh - (((inClose) >= (inOpen)) ? (inClose) : (inOpen)))"),
        "ts stream: upper shadow must be high - max(close, open) — see #217"
    );
    assert!(
        stream.contains("((((inClose) >= (inOpen)) ? (inOpen) : (inClose)) - inLow)"),
        "ts stream: lower shadow must be min(close, open) - low — see #217"
    );
}

/// Every `if A >= B { T } else { E }` in `s` whose arms are its own operands,
/// parentheses ignored.
fn branchy_colour_bounds(s: &str) -> usize {
    let bare = |x: &str| x.replace(['(', ')'], "").trim().to_string();
    let mut n = 0;
    let mut from = 0;
    while let Some(hit) = s[from..].find("if ") {
        let at = from + hit + 3;
        from = at;
        let Some((cond, rest)) = s[at..].split_once(" { ") else { break };
        let Some((t, rest)) = rest.split_once(" } else { ") else { continue };
        let Some((e, _)) = rest.split_once(" }") else { continue };
        if [cond, t, e].iter().any(|x| x.contains(['{', '}'])) {
            continue;
        }
        let Some((a, b)) = cond.split_once(" >= ") else { continue };
        let (a, b, t, e) = (bare(a), bare(b), bare(t), bare(e));
        n += usize::from((t == a && e == b) || (t == b && e == a));
    }
    n
}

/// Every `select_unpredictable(A > B, A, B)` / `(A < B, A, B)` in `s`.
fn colour_selects(s: &str) -> usize {
    let mut n = 0;
    for (at, _) in s.match_indices("select_unpredictable(") {
        let (mut depth, mut args, mut cur) = (0i32, Vec::new(), String::new());
        for ch in s[at + "select_unpredictable(".len()..].chars() {
            match ch {
                '(' | '[' => depth += 1,
                ')' | ']' if depth == 0 => break,
                ')' | ']' => depth -= 1,
                ',' if depth == 0 => {
                    args.push(std::mem::take(&mut cur).trim().to_string());
                    continue;
                }
                _ => {}
            }
            cur.push(ch);
        }
        args.push(cur.trim().to_string());
        if let [cond, t, e] = args.as_slice() {
            n += usize::from([" > ", " < "].iter().any(|op| *cond == format!("{t}{op}{e}")));
        }
    }
    n
}

/// The source of `fn <name>(`, up to the next `fn`.
fn fn_body<'s>(s: &'s str, name: &str) -> &'s str {
    let start = s.find(&format!("fn {name}(")).unwrap_or_else(|| panic!("no fn {name}"));
    let rest = &s[start + 3..];
    &rest[..rest.find(" fn ").unwrap_or(rest.len())]
}

/// Invocations of `c_color_max!(` / `c_color_min!(` in `s`, at an identifier
/// boundary.
fn colour_helper_calls(s: &str) -> usize {
    ["c_color_max!(", "c_color_min!("]
        .iter()
        .map(|h| s.match_indices(h).filter(|(i, _)| !s[..*i].ends_with(|c: char| c.is_alphanumeric() || c == '_')).count())
        .sum()
}

/// A candle's colour is a coin flip on real bars: the batch and open bodies
/// take each colour bound through the crate's macros, C's TA_COLOR_MAX/MIN,
/// and the stream step keeps the `if` form.
#[test]
fn no_rust_batch_or_open_candle_body_branches_on_the_colour() {
    let (mut bodies, mut calls, mut steps, mut step_ifs) = (0usize, 0usize, 0usize, 0usize);
    let mut scalars = 0usize;
    for entry in std::fs::read_dir(input_dir()).expect("input dir") {
        let dir = entry.expect("dir entry").path();
        let name = dir.file_name().expect("name").to_string_lossy().to_string();
        if !name.starts_with("cdl") || !dir.join(format!("{name}.yaml")).exists() {
            continue;
        }
        let s = rust_source(&name);
        let framed = s.contains(&format!("fn {name}_open_sink_frame("));
        let mut tiers = vec![format!("{name}_impl")];
        if framed {
            tiers.extend([format!("{name}_open_sink_frame"), format!("{name}_open_fill_frame")]);
        } else {
            tiers.push(format!("{name}_open_impl"));
        }
        for tier in tiers {
            // An FMA-dispatched tier's code sits in its `_scalar` body.
            let scalar = format!("{tier}_scalar");
            let fused = s.contains(&format!("fn {scalar}("));
            scalars += usize::from(fused);
            let body = fn_body(&s, if fused { &scalar } else { &tier });
            assert_eq!(branchy_colour_bounds(body), 0, "{tier} branches on the colour: {body}");
            assert_eq!(colour_selects(body), 0, "{tier} spells the colour select inline: {body}");
            calls += colour_helper_calls(body);
            bodies += 1;
        }
        for tier in [format!("{name}_step_impl"), "update".to_string(), "peek".to_string()] {
            let body = fn_body(&s, &tier);
            assert_eq!(colour_helper_calls(body), 0, "{name}: {tier} calls a colour helper: {body}");
            assert_eq!(colour_selects(body), 0, "{name}: {tier} selects the colour bound: {body}");
            step_ifs += branchy_colour_bounds(body);
            steps += 1;
        }
    }
    assert!(bodies >= 110, "expected the batch and open tiers of the candlestick corpus, saw {bodies}");
    assert!(scalars >= 15, "expected the FMA-dispatched candlesticks' `_scalar` bodies, saw {scalars}");
    assert!(calls >= bodies, "expected colour helpers in every body, saw {calls} in {bodies}");
    assert!(steps >= 165 && step_ifs >= steps, "expected the `if` bound in the steps, saw {step_ifs} in {steps}");
    assert_eq!(branchy_colour_bounds("x = (if (c) >= (o) { (o) } else { (c) });"), 1);
    assert_eq!(colour_helper_calls("x = c_color_min!((c), (o)) + my_c_color_max!(c, o) + c_color_max(c, o);"), 1);
}

/// x86-64 takes C's SSE2 operand order through a select; every other target
/// keeps the `if`, which already compiles without a branch there. Each arm is
/// the exact inline text, so a bound compiles as if spelled at its site.
#[test]
fn rust_colour_macros_select_on_x86_64_only() {
    let path = PathBuf::from(env!("CARGO_MANIFEST_DIR")).join("templates/rust/c_math.rs");
    let src = collapse(&std::fs::read_to_string(&path).expect("c_math.rs"));
    for (name, select, branch) in [
        ("c_color_max", "std::hint::select_unpredictable($o > $c, $o, $c)", "if $c >= $o { $c } else { $o }"),
        ("c_color_min", "std::hint::select_unpredictable($c < $o, $c, $o)", "if $c >= $o { $o } else { $c }"),
    ] {
        let arm = |cfg: &str, text: &str| {
            format!("#[cfg({cfg})] macro_rules! {name} {{ ($c:expr, $o:expr) => {{ {text} }}; }}")
        };
        for want in [arm("target_arch = \"x86_64\"", select), arm("not(target_arch = \"x86_64\")", branch)] {
            assert!(src.contains(&want), "{name} must be exactly: {want}");
        }
        assert!(src.contains(&format!("pub(crate) use {name};")), "{name} is not exported to the bodies");
        assert_eq!(src.matches(&format!("macro_rules! {name} ")).count(), 2, "{name}: one arm per cfg");
    }
}
