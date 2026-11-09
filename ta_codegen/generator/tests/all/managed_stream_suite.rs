//! Java, C# and TypeScript stream-emitter properties that no runtime gate can see.
//!
//! The managed backends share a shape C and Rust do not: a peek frame
//! carries the transition in locals rather than in the handle. Two properties
//! follow from that, and each is invisible to `stream_verify` because each is
//! about text the emitter did NOT produce -- an absent seed, an ordering.
//! A gate on absence has to be swept, and it has to prove it swept something.

use crate::common;
use std::collections::{BTreeSet, HashMap};
use std::path::PathBuf;
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
    render_stream(&func, &enums, lang)
}

fn render_stream(func: &ir::FuncDef, enums: &HashMap<String, ir::EnumDef>, lang: &str) -> String {
    let registry = common::make_registry();
    let helpers = common::make_helpers();
    let full = match lang {
        "c" => backends::c_stream::generate(func, enums, registry, helpers),
        "rust" => backends::rust_lang::generate(func, enums, registry, helpers),
        "java" => backends::java::generate(func, enums, registry, helpers),
        "csharp" => backends::csharp::generate(func, enums, registry, helpers),
        "ts" => backends::ts::generate(func, enums, registry, helpers),
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
/// stores together (issue #353, gated in rust_stream_suite). No frame in the
/// corpus keeps a seed, so one that grows back fails. The control keeps one,
/// its only store sitting in a loop body the IR cannot prove runs, so the
/// needle is proven to match a kept seed in each backend.
#[test]
fn no_managed_peek_seeds_a_dead_output_local() {
    let (mut control, enums) = load("imi");
    let parsed = parser::c_source::parse_c_source_str(LOOP_STORE_CONTROL);
    parser::c_source::wire_parsed_source(&mut control, &parsed);
    for (lang, needle) in [("java", " peek("), ("csharp", " Peek("), ("ts", " peek(")] {
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
        assert!(
            seeded.is_empty(),
            "{lang}: peek frames seeding an output local from the handle: {seeded:?}"
        );
        let body = body_of(&render_stream(&control, &enums, lang), needle);
        assert!(
            body.contains("cur_outReal = sp.cur_outReal;"),
            "{lang}: the control's kept seed no longer matches the needle"
        );
    }
}

/// IMI's metadata over a body whose only output store is inside the window loop.
const LOOP_STORE_CONTROL: &str = r#"
int imi_lookback(int optInTimePeriod)
{
   return optInTimePeriod - 1;
}

TA_RetCode imi(int startIdx, int endIdx,
   const double inOpen[],
   const double inClose[],
   int optInTimePeriod,
   int *outBegIdx, int *outNBElement,
   double outReal[])
{
   int lookback, outIdx = 0;

   lookback = imi_lookback( optInTimePeriod );
   if(startIdx < lookback)
      startIdx = lookback;
   if( startIdx > endIdx ) {
      *outBegIdx = 0;
      *outNBElement = 0;
      return TA_SUCCESS;
   }
   *outBegIdx = startIdx;
   while (startIdx <= endIdx) {
      double sum = .0;
      int i;
      for (i = startIdx - (optInTimePeriod - 1); i <= startIdx; i++) {
         sum += inClose[i] - inOpen[i];
         outReal[outIdx] = sum;
      }
      startIdx++;
      outIdx++;
   }
   *outNBElement = outIdx;
   return TA_SUCCESS;
}
"#;
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
    for lang in ["java", "csharp", "ts"] {
        let mut with_subs: std::collections::BTreeSet<String> = std::collections::BTreeSet::new();
        for name in streaming_funcs() {
            let upper = name.to_uppercase();
            let base = if lang == "csharp" {
                backends::common::pascal_words(&upper)
            } else {
                backends::common::camel_words(&upper)
            };
            let s = section(&name, lang);
            let def = if lang == "ts" { format!("function {base}StepImpl(") } else { format!("void {base}StepImpl(") };
            let body = body_of(&s, &def);
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
            ["alligator", "bbands", "gator", "ichimoku", "kc", "kdj", "kstext", "macdext",
             "sqzmom", "stoch", "stochf", "stochrsi"]
                .into_iter()
                .collect();
        assert_eq!(
            shipped, expected,
            "the set of multi-output handles driving a sub-stream moved in {lang} — the pin is \
             stale or the sweep has gone vacuous"
        );
    }
}

/// Whether a code line stores into the handle (`sp.`) or the tape: any
/// assignment operator after the whole target, or an increment on either side.
fn handle_or_tape_store(line: &str) -> bool {
    let l = line.trim();
    if l.starts_with("//") || l.starts_with("/*") || l.starts_with('*') {
        return false;
    }
    let b = l.as_bytes();
    let ident = |c: u8| c.is_ascii_alphanumeric() || c == b'_' || c == b'.';
    for prefix in ["sp.", "tape["] {
        for (at, _) in l.match_indices(prefix) {
            if at > 0 && ident(b[at - 1]) {
                continue;
            }
            let mut k = at + prefix.len() - usize::from(prefix.ends_with('['));
            while k < b.len() && ident(b[k]) {
                k += 1;
            }
            while k < b.len() && b[k] == b'[' {
                let mut depth = 0usize;
                while k < b.len() {
                    match b[k] {
                        b'[' => depth += 1,
                        b']' => depth -= 1,
                        _ => {}
                    }
                    k += 1;
                    if depth == 0 {
                        break;
                    }
                }
            }
            let after = l[k..].trim_start();
            let before = l[..at].trim_end();
            let assigns = !after.starts_with("==")
                && ["=", "+=", "-=", "*=", "/=", "%=", "&=", "|=", "^=", "<<=", ">>="]
                    .iter()
                    .any(|op| after.starts_with(op));
            if assigns
                || after.starts_with("++")
                || after.starts_with("--")
                || before.ends_with("++")
                || before.ends_with("--")
            {
                return true;
            }
        }
    }
    false
}

/// No managed peek a period bank reaches (#445) writes its handle or the bank's
/// tape: the arms' and MA's tape peeks, and MAVP's own. Java cannot mark the
/// tape read-only and neither language can mark the handle read-only, and a
/// tape store at the incoming bar's slot is invisible to every value gate: the
/// next update overwrites it before any read.
#[test]
fn no_managed_tape_peek_writes_the_handle_or_the_tape() {
    for store in [
        "sp.periodTotal += x;",
        "tape[(tapeBase - k) & tapeMask] = inReal;",
        "++sp.x;",
        "if( ++sp.hilbertIdx == 3 ) {",
        "sp.x <<= 1;",
        "if( c ) sp.x = 1;",
        "a = 1; sp.x = 2;",
        "sp.buf[sp.i++] = v;",
    ] {
        assert!(handle_or_tape_store(store), "missed a store: {store}");
    }
    for read in [
        "if( sp.x == 2 ) {",
        "y = sp.x <= 3;",
        "double v = core.maPeekTape(sp.bank[slot], sp.tape, 1, sp.tapeMask, x);",
        "double v = sp.bank.length == 0 ? core.mavpEvalWindow(sp, x, cp) : core.maPeekTape(sp.bank[slot], sp.tape, 1, sp.tapeMask, x);",
        "win[i] = sp.tape[(sp.tapePos + sp.tapeMask + 2 - lb + i) & sp.tapeMask];",
    ] {
        assert!(!handle_or_tape_store(read), "a read taken for a store: {read}");
    }

    let registry = common::make_registry();
    let members: Vec<String> = streaming_funcs()
        .into_iter()
        .filter(|n| registry.in_tape_set(n))
        .collect();
    assert!(!members.is_empty(), "the tape set is empty");
    let mut offenders = Vec::new();
    for lang in ["java", "csharp", "ts"] {
        let def_kw = if lang == "ts" { "export function " } else { "private " };
        let mut swept = 0usize;
        for name in &members {
            let src = section(name, lang);
            let mut found = false;
            for (at, _) in src.match_indices("PeekTape") {
                let line_start = src[..at].rfind('\n').map_or(0, |k| k + 1);
                if !src[line_start..at].trim_start().starts_with(def_kw) {
                    continue;
                }
                found = true;
                for l in body_of(&src[line_start..], "PeekTape").lines() {
                    if handle_or_tape_store(l) {
                        offenders.push(format!("{lang} {name}: {}", l.trim()));
                    }
                }
            }
            swept += usize::from(found);
        }
        assert_eq!(swept, members.len(), "{lang}: {swept} of {} tape peeks found", members.len());
        let bank = section("mavp", lang);
        let needle = match lang {
            "java" => "public double peek(",
            "csharp" => "public double Peek(",
            _ => "  peek(real: number, periods: number): number {",
        };
        let peek = body_of(&bank, needle);
        assert!(peek.contains("sp.tape"), "{lang}: MAVP's peek no longer reads the tape");
        for l in peek.lines() {
            if handle_or_tape_store(l) {
                offenders.push(format!("{lang} mavp: {}", l.trim()));
            }
        }
        // A window-mode peek evaluates through this helper.
        let eval = match lang {
            "java" => "private double mavpEvalWindow(",
            "csharp" => "internal double MavpEvalWindow(",
            _ => "function mavpEvalWindow(",
        };
        assert!(bank.contains(eval), "{lang}: MAVP's window evaluation not found");
        for l in body_of(&bank, eval).lines() {
            if handle_or_tape_store(l) {
                offenders.push(format!("{lang} mavp window: {}", l.trim()));
            }
        }
    }
    assert!(offenders.is_empty(), "a peek writes the handle or the tape:\n{}", offenders.join("\n"));
}

/// No public peek of any managed stream stores into its handle. A store that
/// leaves every value right, a scratch field say, passes each value gate, and
/// neither language can mark the handle read-only.
#[test]
fn no_managed_peek_writes_its_handle() {
    let funcs = streaming_funcs();
    let mut offenders = Vec::new();
    for (lang, needle) in [("java", " peek("), ("csharp", " Peek("), ("ts", " peek(")] {
        let mut swept = 0usize;
        let mut handle_reads = 0usize;
        for name in &funcs {
            let src = section(name, lang);
            let mut found = false;
            for (at, _) in src.match_indices(needle) {
                let line_start = src[..at].rfind('\n').map_or(0, |k| k + 1);
                // A TypeScript method has no modifier: the line holds only indent.
                let head = src[line_start..at].trim_start();
                if (lang == "ts" && !head.is_empty()) || (lang != "ts" && !head.starts_with("public ")) {
                    continue;
                }
                found = true;
                for l in body_of(&src[line_start..], needle).lines() {
                    if handle_or_tape_store(l) {
                        offenders.push(format!("{lang} {name}: {}", l.trim()));
                    }
                    // A committing call writes the handle with no target of its own.
                    if lang == "ts" && [".update(", "StepTape(", "StepImpl("].iter().any(|c| l.contains(c)) {
                        offenders.push(format!("{lang} {name} commits: {}", l.trim()));
                    }
                    handle_reads += usize::from(l.contains("sp."));
                }
            }
            swept += usize::from(found);
        }
        assert_eq!(swept, funcs.len(), "{lang}: {swept} of {} public peeks found", funcs.len());
        assert!(handle_reads > 0, "{lang}: no peek body reads its handle through `sp.`");
    }
    assert!(offenders.is_empty(), "a peek writes its handle:\n{}", offenders.join("\n"));
}

/// Every delay-line allocation, in Open and in C's Clone, holds `max(cap, 1)`
/// slots: a depth-0 line has capacity 0, and its push and its load still touch
/// slot 0. A managed clone copies the buffer it was given, which already holds
/// that many.
#[test]
fn every_delay_line_allocation_holds_at_least_one_slot() {
    let registry = common::make_registry();
    let mut corpus_lines = 0usize;
    let mut texts: HashMap<&str, String> = HashMap::new();
    for name in streaming_funcs() {
        let (func, enums) = load(&name);
        let resolved = func.resolved_for(ir::Lang::C);
        if let Ok(ta_codegen_lib::streaming::StreamPlan::Composed(cp)) =
            ta_codegen_lib::streaming::validate_streamable(&resolved, registry)
        {
            corpus_lines += cp.lines.len();
        }
        for lang in ["c", "rust", "java", "csharp", "ts"] {
            texts.entry(lang).or_default().push_str(&render_stream(&func, &enums, lang));
        }
    }
    assert!(corpus_lines >= 1, "no function has a delay line: the gate checks nothing");
    for (lang, text) in &texts {
        let (mut max_form, mut kept) = (0usize, 0usize);
        for line in text.lines() {
            let mut rest = line;
            while let Some(at) = rest.find("lagBuf") {
                let tail = &rest[at + "lagBuf".len()..];
                let k: String = tail.chars().take_while(char::is_ascii_digit).collect();
                rest = &tail[k.len()..];
                let preceded = line[..line.len() - tail.len() - "lagBuf".len()]
                    .chars()
                    .last()
                    .is_some_and(|c| c.is_alphanumeric() || c == '_');
                let Some(rhs) = rest.strip_prefix(" = ") else { continue };
                if k.is_empty() || preceded {
                    continue;
                }
                let allocates = ["TA_Malloc", "vec![", "new double[", "new Float64Array("]
                    .iter()
                    .any(|t| rhs.starts_with(t) || rhs.starts_with(&format!("(double *){t}")));
                if !allocates {
                    continue;
                }
                let max = match *lang {
                    "c" => [
                        format!("(double *)TA_Malloc( sizeof(double) * (size_t)(lagCap{k} > 0 ? lagCap{k} : 1) );"),
                        "(double *)TA_Malloc( sizeof(double) * copyN );".to_string(),
                    ],
                    "rust" => [format!("vec![0.0_f64; lagCap{k}.max(1)];"), String::new()],
                    "java" => [format!("new double[Math.max(lagCap{k}, 1)];"), String::new()],
                    "csharp" => [
                        format!("new double[Math.Max(lagCap{k}, 1)];"),
                        format!("new double[other.lagBuf{k}.Length];"),
                    ],
                    _ => [format!("new Float64Array(lagCap{k} > 0 ? lagCap{k} : 1);"), String::new()],
                };
                if rhs == max[0] {
                    max_form += 1;
                } else if !max[1].is_empty() && rhs == max[1] {
                    kept += 1;
                } else {
                    panic!("{lang}: a delay-line allocation does not hold max(cap, 1) slots: `{line}`");
                }
            }
        }
        assert_eq!(max_form, corpus_lines, "{lang}: one max(cap, 1) allocation per line in Open");
        if *lang == "c" {
            let full: usize = (0..corpus_lines)
                .map(|k| {
                    text.matches(&format!(
                        "{{ size_t copyN = (size_t)(sp->lagCap{k} > 0 ? sp->lagCap{k} : 1);"
                    ))
                    .count()
                })
                .sum();
            assert!(
                full == corpus_lines && kept == corpus_lines,
                "c: every line's Clone copies max(cap, 1) slots"
            );
        }
    }
}
