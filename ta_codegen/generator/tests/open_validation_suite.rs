//! Open/OpenAndFill argument-validation contract across all backends
//! (composed-open fusion, opener rejection ordering, per-input checks),
//! stream-handle candle-settings scoping, and the rust_abstract category
//! index. Split out of the former `backend_suite.rs`.

#[path = "common/mod.rs"]
mod common;

use common::{
    discover_indicators, extract_section, generate_all, load_enums, load_indicator,
    make_registry, try_load_indicator,
};
use std::collections::BTreeSet;
use ta_codegen_lib::backends;
use ta_codegen_lib::helper_registry::HelperRegistry;
use ta_codegen_lib::ir;
use ta_codegen_lib::streaming;

/// Every composed function's Open must FUSE its sub-calls: one
/// `OpenAndFillInternal` that both warms the sub-handle and fills the sub-call's
/// destination, instead of a warm pass plus a batch call recomputing the same
/// numbers (issue #192).
///
/// This needs its own gate because no value gate can see it. The fusion is
/// output-preserving by construction, so `stream_verify`, `--xlang-hash` and the
/// frozen-oracle suites all stay green whether or not it happens — a regression
/// would surface only as a benchmark drifting back towards ~1.7x, which nothing
/// fails on. What is pinned here is the SHAPE.
///
/// One sub-call is deliberately left unfused: STOCH's slow-K `TA_MA` writes its
/// own source in place, where a fused open would overwrite the buffer the
/// sub-handle's capture still has to read. `test_c_stoch_composed_stream_section`
/// pins which one; this test pins that it is the ONLY one.
#[test]
fn test_composed_open_fuses_every_sub_call() {
    const COMPOSED: &[&str] = &[
        "bbands", "macdext", "ppo", "pvo", "stddev",
        "stoch", "stochf", "adxr", "stochrsi", "apo",
    ];
    let registry = make_registry();
    let helpers = HelperRegistry::empty();
    let mut fused_total = 0usize;
    let mut unfused_total = 0usize;

    for name in COMPOSED {
        let (mut func, enums) = load_indicator(name);
        func.streaming = true;
        let c = backends::c::generate(&func, &enums, &registry, &helpers);
        let stream = &c[c.find("/**** Streaming API *****/").expect("stream section")..];

        let fused = stream.matches("_OpenAndFillInternal( &sub").count();
        let unfused = stream.matches("_OpenInternal( &sub").count();
        // The other two backends must reach the SAME split. They render the call
        // differently (Rust slices; Java passes the array itself wherever the
        // sub-range is already the whole array, #203) but the decision is shared
        // — `SubCallStep::is_fusable` — so a per-backend divergence here means one
        // emitter silently stopped fusing, which no value gate can see.
        // Anchored on the first ARGUMENT so these count call sites, not the
        // wrapper definitions (Rust sub-opens pass `&series[..n]`; both
        // definitions break the line right after the paren).
        let r = backends::rust_stream::generate(&func, &enums, &registry, &helpers);
        assert_eq!(
            (r.matches("_open_and_fill_internal(&").count(), r.matches("_open_internal(&").count()),
            (fused, unfused),
            "{name}: Rust fused/unfused split differs from C"
        );
        // Java's first argument is no longer a reliable anchor — since #203 a
        // sub-open passes a bare `inReal` where the copy carried nothing — so
        // count the lines that ASSIGN a `sub<n>` handle instead. That is what
        // separates a sub-open both from the wrapper definitions and from the
        // public `_Open`'s own delegation to `_OpenInternal`.
        let j = backends::java_stream::generate(&func, &enums, &registry, &helpers);
        let sub_opens = |needle: &str| {
            j.lines().filter(|l| l.contains("sub") && l.contains(needle)).count()
        };
        assert_eq!(
            (sub_opens("OpenAndFillInternal("), sub_opens("OpenInternal(")),
            (fused, unfused),
            "{name}: Java fused/unfused split differs from C"
        );
        assert!(
            fused + unfused > 0,
            "{name}: composed Open emitted no sub-stream open at all"
        );
        // A fused sub-open replaces the batch sub-call with `<var> = subRc;`, so
        // every fused sub must have left exactly one of those substitutions.
        assert_eq!(
            stream.matches("= subRc;").count(),
            fused,
            "{name}: fused sub-opens ({fused}) and retCode substitutions disagree \
             — either a batch sub-call survived the fusion, or one was dropped \
             without leaving a retCode behind for the transcribed error handling"
        );
        fused_total += fused;
        unfused_total += unfused;
    }

    assert_eq!(
        unfused_total, 1,
        "expected exactly ONE unfused sub-call across the composed tier (STOCH's \
         in-place slow-K); found {unfused_total}. A new unfused sub-call means \
         either an aliasing sub-call was added — legitimate, but record it here — \
         or the fusion silently stopped applying."
    );
    assert_eq!(
        fused_total, 17,
        "expected 17 fused sub-calls across the composed tier; found {fused_total}. \
         Adding a composed indicator moves this number: update it deliberately."
    );
}

/// Rule S5 on EVERY Rust public `OpenAndFill` — corpus-wide, because Rust's own
/// probe (`tests/stream_open_contract.rs`) names six functions and runs nightly.
///
/// Two clauses. The width has to come from the function's OWN lookback, not from
/// the history's length — `historyLen - lookback` is what the fill writes, and a
/// bound of `historyLen` would reject every correctly-sized call. And the
/// capacity has to precede the output-distinctness guard, which is the order the
/// specification lists (S5, then S6).
#[test]
fn rust_public_fill_bounds_every_output_against_its_own_lookback() {
    let registry = make_registry();
    let helpers = HelperRegistry::empty();
    let enums = load_enums();
    let mut checked = 0usize;

    for name in discover_indicators() {
        let Some((func, _)) = try_load_indicator(&name) else { continue };
        if !func.streaming {
            continue;
        }
        let src = backends::rust_lang::generate(&func, &enums, &registry, &helpers);
        let sn = func.name.to_lowercase();
        let entry = format!("pub fn {}_open_and_fill(", backends::common::snake_words(&func.name));
        let at = src
            .find(&entry)
            .unwrap_or_else(|| panic!("{}: no public OpenAndFill", func.name));
        // The PUBLIC frame only. Rust emits `<N>_OpenAndFillInternal` after it,
        // so a needle searched to end-of-file would be satisfied by the anchored
        // seam — which is exactly where this bound must NOT be.
        let frame_len = src[at..]
            .find("\n    }\n")
            .unwrap_or_else(|| panic!("{}: public OpenAndFill has no end", func.name));
        let body = &src[at..at + frame_len];
        let width = format!("let _guardOutLen = {}.len().saturating_sub(_guardLb);",
                            streaming::input_array_names(&func)[0]);
        let at_width = body.find(&width).unwrap_or_else(|| {
            panic!("{}: OpenAndFill does not derive the fill width from the history", func.name)
        });
        let lb = format!("let _guardLb = self.{}_Lookback(", sn.to_uppercase());
        assert!(
            body[..at_width].contains(&lb),
            "{}: the fill width is not read from the function's own lookback",
            func.name
        );
        // S5's INPUT half — B5's first clause, read on this range — and its
        // position: after S3 (the lookback's own rejection) and before the
        // output capacity, which is the order B5 states.
        let inputs = streaming::input_array_names(&func);
        if inputs.len() > 1 {
            let disagree: Vec<String> = inputs[1..]
                .iter()
                .map(|extra| format!("{extra}.len() != {}.len()", inputs[0]))
                .collect();
            let needle = format!("if {} {{", disagree.join(" || "));
            let at_in = body.find(&needle).unwrap_or_else(|| {
                panic!("{}: OpenAndFill does not require the inputs to be the history's length", func.name)
            });
            // S3 first, in whichever shape this tier spells it: the merged tiers
            // take their rejection from `<N>_Lookback(..)?`, the two exempt ones
            // validate each parameter inline. Either way a parameter fault is
            // answered before a length one.
            assert!(
                body[..at_in].contains(&lb)
                    || body[..at_in].contains("return Err(RetCode::BadParam);"),
                "{}: the input half is checked before the parameters",
                func.name
            );
            assert!(at_in < at_width, "{}: the input half follows the output width", func.name);
            let tail = &body[at_in + needle.len()..];
            assert!(
                tail.trim_start().starts_with("return Err(RetCode::BadParam);"),
                "{}: the input half does not reject",
                func.name
            );
        }
        for out in &func.outputs {
            // A `nullable` output is `Option<&mut [T]>` and is bounded only when
            // it was supplied (rule B6a); every other output is bounded flat.
            let needle = if out.is_nullable() {
                format!("if {}.as_deref().is_some_and(|o| o.len() < _guardOutLen) {{", out.name)
            } else {
                format!("if {}.len() < _guardOutLen {{", out.name)
            };
            let at_out = body.find(&needle).unwrap_or_else(|| {
                panic!("{}: OpenAndFill does not bound `{}`", func.name, out.name)
            });
            assert!(at_out > at_width, "{}: `{}` is bounded before the width exists", func.name, out.name);
            // …and the bound REJECTS. Pinning the condition alone passes against
            // an `if` with an empty body, which is the mutation this gate exists
            // to catch.
            let tail = &body[at_out + needle.len()..];
            assert!(
                tail.trim_start().starts_with("return Err(RetCode::BadParam);"),
                "{}: `{}`'s bound does not reject",
                func.name,
                out.name
            );
            if let Some(at_alias) = body.find(&format!("{}_p.as_ptr() ==", out.name)) {
                assert!(at_out < at_alias, "{}: S5 is specified ahead of S6", func.name);
            }
        }
        checked += 1;
    }
    assert!(checked >= 200, "only {checked} public fills inspected");
}

/// Rule S1 on EVERY C# public opener — corpus-wide, for the same reason as the
/// Java gate below.
///
/// C# is the backend where the probe/corpus gap is widest. Its live S1 is the
/// public frame's `IsEmpty` throw, not the core's `historyLen < 1` return, which
/// the frame makes unreachable from the public API — so
/// `scripts/check_stream_retcodes.py`, which reads the core, is not evidence
/// about the surface a caller touches. `StreamApiTest` probes it on `SMA`; this
/// is what covers the other 175.
///
/// Two clauses, because the first input means something the others do not: it
/// carries the history, so empty THERE is S1; a later one is a length
/// disagreement, which is the catch-all like every other argument fault.
#[test]
fn csharp_public_openers_reject_an_empty_history_as_an_index_fault() {
    let registry = make_registry();
    let helpers = HelperRegistry::empty();
    let enums = load_enums();
    let mut checked = 0usize;

    for name in discover_indicators() {
        let Some((func, _)) = try_load_indicator(&name) else { continue };
        if !func.streaming {
            continue;
        }
        let src = backends::csharp::generate(&func, &enums, &registry, &helpers);
        let base = func.name.to_uppercase();
        let ident = backends::common::pascal_words(&func.name);
        let inputs = streaming::input_array_names(&func);
        for (verb, entry) in [
            ("open", format!("public {ident}Stream {ident}Open( ")),
            ("openAndFill", format!("public {ident}Stream {ident}OpenAndFill( ")),
        ] {
            let at = src
                .find(&entry)
                .unwrap_or_else(|| panic!("{}: no public {verb}", func.name));
            let body = &src[at..];
            let history = &inputs[0];
            let s1 = format!(
                "if( {history}.IsEmpty ) throw new TaLibArgumentOutOfRangeException(nameof({history}), \"{base} {verb}: history is empty\", RetCode.OutOfRangeStartIndex);"
            );
            let at_s1 = body.find(&s1).unwrap_or_else(|| {
                panic!("{}: {verb} does not answer S1 on the history", func.name)
            });
            for extra in &inputs[1..] {
                let other = format!(
                    "if( {extra}.IsEmpty ) throw new TaLibArgumentException(\"{base} {verb}: {extra} is empty\", nameof({extra}), RetCode.BadParam);"
                );
                let at_other = body.find(&other).unwrap_or_else(|| {
                    panic!("{}: {verb} does not check `{extra}`", func.name)
                });
                assert!(
                    at_other > at_s1,
                    "{}: {verb} checks `{extra}` before the history",
                    func.name
                );
            }
            // Rule S5, and the width the fill's half is derived from. An
            // opener's `startIdx` is the constant 0, so B5's produced count
            // collapses to `historyLen - lookback` — read from the function's
            // OWN lookback, never from the history's width.
            let at_width = (verb == "openAndFill").then(|| {
                let width = format!(
                    "int guardOutLen = OpenFillCount(\"{base}\", \"openAndFill\", {history}.Length, {base}_Lookback("
                );
                body.find(&width).unwrap_or_else(|| {
                    panic!("{}: openAndFill does not derive the fill width from its own lookback", func.name)
                })
            });
            // S5's input half, at BOTH openers (issue #271 item 1): the two
            // used to answer the same fault differently, the fill naming the
            // leg and the plain open reporting a bare `BadParam` from the core.
            // At the fill it sits after S3 (the width's own rejection) and
            // before the output capacity — the order B5 states.
            for extra in &inputs[1..] {
                let needle = format!(
                    "RequireHistoryLength(\"{base}\", \"{verb}\", \"{extra}\", {extra}.Length, {history}.Length);"
                );
                let at_in = body.find(&needle).unwrap_or_else(|| {
                    panic!("{}: {verb} does not require `{extra}` to be the history's length", func.name)
                });
                assert!(at_in > at_width.unwrap_or(at_s1), "{}: {verb} checks `{extra}`'s length too early", func.name);
            }
            if verb == "openAndFill" {
                for out in &func.outputs {
                    // A `nullable` output is bounded only when it was supplied
                    // (rule B6a); the guard is part of the needle, so a
                    // regression to an unconditional bound — or to none — is a
                    // failure rather than a substring that still matches.
                    let bound = format!(
                        "RequireFillLength(\"{base}\", \"openAndFill\", \"{0}\", {0}.Length, guardOutLen);",
                        out.name
                    );
                    let needle = if out.is_nullable() {
                        format!("if( !{}.IsEmpty ) {bound}", out.name)
                    } else {
                        bound.clone()
                    };
                    let at_out = body.find(&needle).unwrap_or_else(|| {
                        panic!("{}: openAndFill does not bound `{}`", func.name, out.name)
                    });
                    if !out.is_nullable() {
                        assert!(
                            !body.contains(&format!("if( !{}.IsEmpty ) {bound}", out.name)),
                            "{}: `{}` is not nullable and must be bounded unconditionally",
                            func.name,
                            out.name
                        );
                    }
                    assert!(at_out > at_s1, "{}: `{}` is bounded before the history is checked", func.name, out.name);
                }
            }
            checked += 1;
        }
    }
    assert!(checked >= 352, "only {checked} public openers inspected");
}

/// Rule S4, then S1/S2, on EVERY Java public opener — corpus-wide, because a
/// probe on SMA says nothing about the other 175.
///
/// The order is the point, and the assertion is two-sided: the FIRST input's
/// null test precedes the index pair (a length is not readable from an array
/// that is not there), and **everything else** follows it — the remaining price
/// legs as much as the outputs. A one-sided version that only pinned the outputs
/// would pass against a frame that checks every leg up front, which reports the
/// leg where C reports the empty history.
///
/// An output is checked with `requireLength`, not `requireArgument`: one call
/// carries S4 and S5, exactly as the batch wrapper's does. Requiring the
/// capacity form here is what stops a fill's presence check from silently
/// reverting to presence alone.
#[test]
fn java_public_openers_check_arguments_then_the_index_pair() {
    let registry = make_registry();
    let helpers = HelperRegistry::empty();
    let enums = load_enums();
    let mut checked = 0usize;

    for name in discover_indicators() {
        let Some((func, _)) = try_load_indicator(&name) else { continue };
        if !func.streaming {
            continue;
        }
        let src = backends::java::generate(&func, &enums, &registry, &helpers);
        let base = func.name.to_uppercase();
        let ident_type = backends::common::pascal_words(&func.name);
        let ident_method = backends::common::camel_words(&func.name);
        for (verb, entry, with_outputs) in [
            ("open", format!("public {ident_type}Stream {ident_method}Open( "), false),
            ("openAndFill", format!("public {ident_type}Stream {ident_method}OpenAndFill( "), true),
        ] {
            let at = src
                .find(&entry)
                .unwrap_or_else(|| panic!("{}: no public {verb}", func.name));
            let body = &src[at..];
            let history = &streaming::input_array_names(&func)[0];
            let pair = body
                .find(&format!("requireHistory(\"{base} {verb}\", {history}.length);"))
                .unwrap_or_else(|| panic!("{}: {verb} does not check the index pair", func.name));

            let mut names: Vec<(String, bool)> = streaming::input_array_names(&func)
                .into_iter()
                .map(|i| (i, false))
                .collect();
            if with_outputs {
                names.extend(func.outputs.iter().map(|o| (o.name.clone(), true)));
            }
            for (arg, is_output) in &names {
                let needle = if *is_output {
                    format!("requireLength(\"{base} {verb}\", \"{arg}\", {arg}, guardOutLen);")
                } else {
                    format!("requireArgument(\"{base} {verb}\", \"{arg}\", {arg});")
                };
                let at_arg = body.find(&needle).unwrap_or_else(|| {
                    panic!("{}: {verb} does not check `{arg}`", func.name)
                });
                // Only the history is allowed in front of the pair.
                let expect_after = arg != history;
                assert_eq!(
                    at_arg > pair,
                    expect_after,
                    "{}: {verb}'s check of `{arg}` is on the wrong side of the index pair",
                    func.name
                );
            }
            // S5's width, derived from the lookback rather than from the
            // requested range: an opener's `startIdx` is the constant 0, so
            // B5's produced count collapses to `historyLen - lookback`. Only
            // the fill has one — the plain open writes nothing.
            let at_width = with_outputs.then(|| {
                let width = format!(
                    "int guardOutLen = openFillCount(\"{base} {verb}\", {history}.length, {base}_Lookback("
                );
                body.find(&width).unwrap_or_else(|| {
                    panic!("{}: openAndFill does not derive the fill width from its own lookback", func.name)
                })
            });
            // S5's input half, at BOTH openers (issue #271 item 1): the two
            // used to answer the same fault differently, the fill naming the
            // leg and the plain open reporting a bare `BadParam` from the core.
            // At the fill it sits after S3 (the width's own rejection) and
            // before the output capacity — the order B5 states.
            for extra in streaming::input_array_names(&func).iter().skip(1) {
                let needle = format!(
                    "requireHistoryLength(\"{base} {verb}\", \"{extra}\", {extra}.length, {history}.length);"
                );
                let at_in = body.find(&needle).unwrap_or_else(|| {
                    panic!("{}: {verb} does not require `{extra}` to be the history's length", func.name)
                });
                assert!(
                    at_in > at_width.unwrap_or(pair),
                    "{}: {verb} checks `{extra}`'s length too early",
                    func.name
                );
            }
            if with_outputs {
                for out in &func.outputs {
                    // A `nullable` output is bounded only when it was supplied
                    // (rule B6a). The guard is part of the needle: without it
                    // the bare call is a SUBSTRING of the guarded line, so the
                    // gate would keep passing while going blind on exactly the
                    // output the rule is about.
                    let bound = format!(
                        "requireLength(\"{base} {verb}\", \"{0}\", {0}, guardOutLen);",
                        out.name
                    );
                    let needle = if out.is_nullable() {
                        format!("if( {} != null ) {bound}", out.name)
                    } else {
                        bound.clone()
                    };
                    assert!(
                        body.contains(&needle),
                        "{}: openAndFill does not bound `{}`",
                        func.name,
                        out.name
                    );
                    if !out.is_nullable() {
                        assert!(
                            !body.contains(&format!("if( {} != null ) {bound}", out.name)),
                            "{}: `{}` is not nullable and must be bounded unconditionally",
                            func.name,
                            out.name
                        );
                    }
                }
            }
            checked += 1;
        }
    }
    assert!(checked >= 352, "only {checked} public openers inspected");
}

/// No opener answers the code its sub-call handed back.
///
/// The composed openers guard a sub-call on `retCode != SUCCESS || count == 0`.
/// Since #267 the error half is answered at the call site and folded away, so
/// the surviving half is the count test — reached with `retCode` holding
/// `Success`, and the arm returned it. Rust said `Err(RetCode::Success)`, a
/// contradiction that reached the public `<N>_Open` through `?`; Java and C#
/// minted a handle over an empty range. Rule S7 is what that shape is: a
/// history that cannot produce a value (issue #271 item 4).
///
/// Corpus-wide over the three ported backends, because the five sites are in
/// four functions and the next composed indicator would get the same body. C is
/// deliberately absent: it runs no cleanup sequence, so its guard still carries
/// the error half and `return retCode` there is the error propagation.
///
/// This is the absence half. That the arm answers the opener's code — rather
/// than losing the return altogether — is asserted directly on the pass, in
/// `ir_cleanup`'s own tests.
#[test]
fn an_opener_never_answers_the_code_its_sub_call_handed_back() {
    let registry = make_registry();
    let helpers = HelperRegistry::empty();
    let enums = load_enums();

    // The definition keyword that tells a definition from a call site, the
    // bare-code return this tier may not contain, and the substring that marks
    // an Open-family identifier in this backend's own casing (Rust keeps the
    // `_open` underscore; Java/C# dropped the underscore but kept `Open`
    // PascalCase, issue #278).
    let specs: [(&str, &str, &str, &str); 3] = [
        ("Rust", "pub(crate) fn", "return Err(retCode)", "_open"),
        ("Java", "private RetCode", "return retCode ;", "Open"),
        ("C#", "private RetCode", "return retCode ;", "Open"),
    ];

    let mut per_backend = [0usize; 3];
    for name in discover_indicators() {
        let Some((func, _)) = try_load_indicator(&name) else { continue };
        if !func.streaming {
            continue;
        }
        // Shipped functions only, and here for a sharper reason than the other
        // fixture filters: SYNTH13 asserts the OPPOSITE property on purpose. Its
        // Leg C puts a read of the code variable between the sub-call and its
        // guard ("the scan must stop there, so the guard survives everywhere"),
        // so the fold cannot drop it and the arm is emitted. It is dead — the
        // line above it assigns `Success` unconditionally, so the condition is
        // statically false and no `Err(Success)` can escape — but this sweep
        // reads text and cannot tell a dead arm from a live one. Teaching the
        // fold to drop a guard its own assignment disproves would fix it at the
        // source, buys nothing shipped, and would make the fixture assert
        // something it was written to deny.
        if name.starts_with("synth") {
            continue;
        }
        let sources = [
            backends::rust_lang::generate(&func, &enums, &registry, &helpers),
            backends::java::generate(&func, &enums, &registry, &helpers),
            backends::csharp::generate(&func, &enums, &registry, &helpers),
        ];
        for (b, src) in sources.iter().enumerate() {
            let (lang, def_kw, bare, needle) = specs[b];
            let mut at = 0;
            while let Some(i) = src[at..].find(needle) {
                let abs = at + i;
                at = abs + needle.len();
                let line_start = src[..abs].rfind('\n').map_or(0, |n| n + 1);
                if !src[line_start..abs].contains(def_kw) {
                    continue;
                }
                let body = strip_comments(body_after(src, abs));
                if body.is_empty() {
                    continue;
                }
                assert!(
                    !body.contains(bare),
                    "{lang} {}: an opener returns the sub-call's own code, which is `Success` \
                     wherever the fold answered the error half",
                    func.name
                );
                per_backend[b] += 1;
            }
        }
    }
    // Non-vacuity, per backend: a def-keyword that stopped matching would make
    // the whole sweep skip in silence.
    for (b, n) in per_backend.iter().enumerate() {
        assert!(*n >= 200, "{}: only {n} opener bodies inspected", specs[b].0);
    }
}

/// The `{`-matched body that follows `from`.
fn body_after(s: &str, from: usize) -> &str {
    let b = match s[from..].find('{') {
        Some(i) => from + i,
        None => return "",
    };
    let bytes = s.as_bytes();
    let (mut depth, mut j) = (0usize, b);
    while j < bytes.len() {
        match bytes[j] {
            b'{' => depth += 1,
            b'}' => {
                depth -= 1;
                if depth == 0 {
                    return &s[b..=j];
                }
            }
            _ => {}
        }
        j += 1;
    }
    &s[b..]
}

/// Comments removed, line by line so every slice stays on a char boundary
/// (these bodies carry em dashes). Prose is not code: "Trading for a
/// Living" in EFI's provenance note otherwise reads as a loop.
fn strip_comments(s: &str) -> String {
    let mut out = String::with_capacity(s.len());
    let mut in_block = false;
    for line in s.lines() {
        let mut rest = line;
        loop {
            if in_block {
                match rest.find("*/") {
                    Some(e) => {
                        rest = &rest[e + 2..];
                        in_block = false;
                    }
                    None => break,
                }
            } else {
                match (rest.find("/*"), rest.find("//")) {
                    (Some(a), Some(b)) if b < a => {
                        out.push_str(&rest[..b]);
                        break;
                    }
                    (Some(a), _) => {
                        out.push_str(&rest[..a]);
                        rest = &rest[a + 2..];
                        in_block = true;
                    }
                    (None, Some(b)) => {
                        out.push_str(&rest[..b]);
                        break;
                    }
                    (None, None) => {
                        out.push_str(rest);
                        break;
                    }
                }
            }
        }
        out.push('\n');
    }
    out
}

/// Every transcribed `_OpenImpl` rejects an anchor that lands past the history,
/// in all four backends, and does it before any loop can run.
///
/// The batch prologue has always rejected `endIdx < startIdx`; the streaming
/// prologue did not, and only 137 of the 174 transcribed bodies carry TA-Lib's
/// own "make sure there is still something to evaluate" preamble to make up for
/// it — a function with no lookback has nothing to clamp `startIdx` up to, so
/// its transcription never had the check. The other 37 compute
/// `nbBar = endIdx - startIdx + 1` and then run `while( nbBar != 0 )`: a
/// negative count never reaches zero, and the loop walks off the end of both
/// the inputs and the output. `TA_AD_OpenInternal` with `startIdx` 45 over 40
/// bars was an ASan stack-buffer-overflow; the same call panicked in Rust,
/// where the count is `usize`.
///
/// Two ORDER assertions, because presence alone is the weaker half of this and
/// a guard in the wrong place is exactly the bug:
///
///   * after the history-emptiness check — `historyLen - 1` is evaluated by the
///     guard itself, and in Rust `historyLen` is a `usize`;
///   * before the first loop in the body — a guard the loop has already run
///     past protects nothing.
#[test]
fn every_open_pass_rejects_an_anchor_past_the_history() {
    /// Every TRANSCRIBED `_OpenImpl` DEFINITION body in `src`. Two filters, and
    /// both are load-bearing. `_OpenImpl(` alone also matches the call sites
    /// every function has, and a body sliced from a call site is whatever block
    /// happens to follow it — so the definition keyword has to be on the same
    /// line. And in Java and C# the two exempt tiers (MA, MAVP) wear the same
    /// name over a hand-rolled body that is not the strided numerics and owns no
    /// anchor of its own, so the parameter list must carry `outStride`.
    fn open_impls<'a>(src: &'a str, def_kw: &str, needle: &str) -> Vec<&'a str> {
        let mut out = Vec::new();
        let mut at = 0;
        while let Some(i) = src[at..].find(needle) {
            let abs = at + i;
            at = abs + needle.len();
            let line_start = src[..abs].rfind('\n').map_or(0, |n| n + 1);
            let params_end = src[abs..].find('{').map_or(src.len(), |b| abs + b);
            if src[line_start..abs].contains(def_kw) && src[abs..params_end].contains("outStride") {
                out.push(body_after(src, abs));
            }
        }
        out
    }

    let registry = make_registry();
    let helpers = HelperRegistry::empty();
    let enums = load_enums();

    // guard text, the emptiness check it must follow, and the signature marker
    let specs: [(&str, &str, &str); 4] = [
        ("C", "if( startIdx > historyLen - 1 )", "if( historyLen < 1 ) return TA_OUT_OF_RANGE_START_INDEX;"),
        ("Rust", "if startIdx > endIdx {", ".is_empty()"),
        ("Java", "if( startIdx > endIdx ) {", "historyLen < 1"),
        ("C#", "if( startIdx > endIdx ) {", "historyLen < 1"),
    ];
    // The definition keyword, which is what tells a definition from a call site.
    let def_kws = ["static TA_RetCode", "pub(crate) fn", "private RetCode", "private RetCode"];
    // The `_OpenImpl` marker in this backend's own casing (issue #278): C keeps
    // `_OpenImpl(`, Rust lower-cases the whole family to `_open_impl(`, and
    // Java/C# dropped the underscore joiner but kept `OpenImpl(` PascalCase.
    let open_impl_needles = ["_OpenImpl(", "_open_impl(", "OpenImpl(", "OpenImpl("];

    let mut checked = 0usize;
    let mut per_backend = [0usize; 4];

    for name in discover_indicators() {
        let Some((func, _)) = try_load_indicator(&name) else { continue };
        let sources = [
            backends::c::generate(&func, &enums, &registry, &helpers),
            backends::rust_lang::generate(&func, &enums, &registry, &helpers),
            backends::java::generate(&func, &enums, &registry, &helpers),
            backends::csharp::generate(&func, &enums, &registry, &helpers),
        ];

        for (b, src) in sources.iter().enumerate() {
            let (lang, guard, empty_check) = specs[b];
            for body in open_impls(src, def_kws[b], open_impl_needles[b]) {
                if body.is_empty() {
                    continue;
                }
                let body = &strip_comments(body);
                let g = body.find(guard).unwrap_or_else(|| {
                    panic!("{lang} {}_OpenImpl: no anchor guard — an anchor past the history would run the body's loop with a negative count", func.name)
                });
                if let Some(e) = body.find(empty_check) {
                    assert!(
                        e < g,
                        "{lang} {}_OpenImpl: the anchor guard evaluates the history length, so it must come after the emptiness check",
                        func.name
                    );
                }
                // Arm-blind, and deliberately loose because of it: a DualMode
                // body has one guard/loop pair PER ARM, so comparing the first
                // of each across the whole body is the wrong shape there — add
                // `for(` to the needle and HMA reports a false positive on its
                // identity arm's fill loop, which its own clamp already guards.
                // The "a guard exists" assertion above is the total one; this is
                // a placement check that skips the 37 bodies matching no needle.
                let first_loop = ["while", "for "]
                    .iter()
                    .filter_map(|kw| body.find(kw))
                    .min();
                if let Some(l) = first_loop {
                    assert!(
                        g < l,
                        "{lang} {}_OpenImpl: the anchor guard sits after the first loop, which is where the unbounded walk happens",
                        func.name
                    );
                }
                per_backend[b] += 1;
                checked += 1;
            }
        }
    }

    // Non-vacuity: this must actually have looked at the corpus, in every
    // backend, not silently skip on a signature marker that stopped matching.
    for (b, n) in per_backend.iter().enumerate() {
        assert!(
            *n > 150,
            "{}: only {n} _OpenImpl bodies seen — the signature marker has drifted",
            specs[b].0
        );
    }
    assert!(checked > 600, "only {checked} bodies checked across four backends");
}

/// Every DECLARED input is checked in every backend (#260).
///
/// Seven candlestick legs are declared by their function and never indexed by
/// its body: `inHigh` and `inLow` on CDL3OUTSIDE, CDLENGULFING and
/// CDLXSIDEGAP3METHODS, and `inOpen` on CDLHIKKAKE. Rust, Java and C# used to
/// exempt exactly those from their argument checks, computing the set from the
/// body; C's NULL checks covered them like any other input. So
/// `TA_CDL3OUTSIDE(0, 251, open, NULL, NULL, close, ...)` was `TA_BAD_PARAM` in
/// C and a success in the other three — a three-way exemption, which is the
/// defect rather than which side was right.
///
/// A corpus sweep, because the exemption was DERIVED: it was never a list a
/// reviewer could read, so any future indicator could have joined it silently.
/// Each backend's own spelling of the check, since B4 and B5 are one condition
/// per backend — a NULL test in C, `requireLength` / `RequireLength` in Java and
/// C#, and **two** in Rust: the public tier's returned `BadParam`, which is what
/// a caller meets, and the `assert!` bound in `<N>_Impl`, which is what the
/// phantom-I/O sweep meets and what LLVM reads (#265; a cross-call meets the
/// public one too since #267).
///
/// Part two is what keeps part one honest. A sweep over every declared input
/// passes trivially once no input is ever dropped, so it cannot tell you the
/// interesting legs were ever at risk. So the seven are named, asserted to still
/// be unread by their body, and asserted to be checked anyway. If a body starts
/// reading one, the "still unread" half fails and the list gets revisited; if the
/// exemption comes back, the first half fails on all seven.
#[test]
fn every_declared_input_is_checked_in_every_backend() {
    let registry = make_registry();
    let helpers = HelperRegistry::empty();

    let mut scanned = 0usize;
    let mut no_inputs = 0usize;
    let mut legs_checked = 0usize;

    for name in discover_indicators() {
        let Some((func, enums)) = try_load_indicator(&name) else {
            continue;
        };
        if func.inputs.is_empty() {
            no_inputs += 1;
            continue;
        }
        let out = generate_all(&func, &enums);
        let csharp = backends::csharp::generate(&func, &enums, &registry, &helpers);
        scanned += 1;
        let f = &func.name;

        for input in &func.inputs {
            let n = &input.name;
            legs_checked += 1;
            // Each backend's own spelling, and how many entry points must carry
            // it: C emits the double prologue and its `TA_S_` float twin, Java
            // and C# emit a double and a float overload, Rust is generic and
            // emits one. A COUNT, not a `contains`: a filter re-applied to one
            // precision only would leave the other's copy to satisfy a
            // whole-file search.
            for (lang, src, needle, want) in [
                ("c", &out.c, format!("if( !{n} )"), 2usize),
                (
                    "rust",
                    &out.rust,
                    format!("assert!(_assertStart > endIdx || endIdx < {n}.len());"),
                    1,
                ),
                // The public tier's own bound, which is what a caller of the
                // crate actually meets (#265). Two needles for Rust, not one:
                // the assert above states the same thing to LLVM and stays in
                // `<N>_Impl` for the phantom-I/O sweep, so it would go on
                // satisfying this test after the caller-facing check was gone.
                ("rust-pub", &out.rust, format!("if {n}.len() < endIdx + 1 {{"), 1),
                (
                    "java",
                    &out.java,
                    format!("requireLength(\"{f}\", \"{n}\", {n}, guardInLen);"),
                    2,
                ),
                (
                    "csharp",
                    &csharp,
                    format!("RequireLength(\"{f}\", \"{n}\", {n}.Length, guardInLen);"),
                    2,
                ),
            ] {
                assert!(
                    src.matches(&needle).count() >= want,
                    "{lang}: {f} declares {n} and checks it on fewer than {want} entry \
                     point(s) — expected `{needle}`"
                );
            }
        }
    }

    // Every discovered indicator is accounted for: swept, or explicitly counted
    // as having no input to sweep. A floor would let a `continue` path drop a
    // fifth of the corpus and still pass.
    assert_eq!(
        scanned + no_inputs,
        discover_indicators().len(),
        "every discovered indicator is swept or counted ({scanned} + {no_inputs})"
    );
    // Literal floors: a derived one moves with whatever the scan happens to find.
    assert!(scanned >= 200, "only {scanned} indicators scanned");
    assert!(legs_checked >= 380, "only {legs_checked} declared input legs checked");

    // ---- part two: the seven legs the exemption used to drop ----
    let unread: [(&str, &[&str]); 4] = [
        ("cdl3outside", &["inHigh", "inLow"]),
        ("cdlengulfing", &["inHigh", "inLow"]),
        ("cdlxsidegap3methods", &["inHigh", "inLow"]),
        ("cdlhikkake", &["inOpen"]),
    ];
    let mut pairs = 0usize;
    for (indicator, legs) in unread {
        let (func, enums) = load_indicator(indicator);
        let out = generate_all(&func, &enums);
        let csharp = backends::csharp::generate(&func, &enums, &registry, &helpers);
        let f = &func.name;
        // The body proper: everything after the bounds-assert preamble, so the
        // asserts this test just demanded cannot themselves satisfy "is read".
        let body = extract_section(&out.rust, "let mut startIdx = startIdx;", &format!("pub fn {f}("));
        assert!(
            body.contains("inClose["),
            "{f}: the control leg inClose is not indexed — the body extraction is wrong"
        );
        for leg in legs {
            pairs += 1;
            assert!(
                !body.contains(&format!("{leg}[")),
                "{f}: {leg} is indexed by the body now; it is no longer an unread leg, so \
                 revisit this list"
            );
            // ...and checked regardless. Restated on the four backends here so a
            // sweep that silently stopped covering these still fails.
            assert!(out.c.contains(&format!("if( !{leg} )")), "c: {f}.{leg}");
            assert!(
                out.rust
                    .contains(&format!("assert!(_assertStart > endIdx || endIdx < {leg}.len());")),
                "rust: {f}.{leg}"
            );
            assert!(
                out.rust.contains(&format!("if {leg}.len() < endIdx + 1 {{")),
                "rust public tier: {f}.{leg}"
            );
            assert!(
                out.java
                    .contains(&format!("requireLength(\"{f}\", \"{leg}\", {leg}, guardInLen);")),
                "java: {f}.{leg}"
            );
            assert!(
                csharp.contains(&format!(
                    "RequireLength(\"{f}\", \"{leg}\", {leg}.Length, guardInLen);"
                )),
                "csharp: {f}.{leg}"
            );
        }
    }
    assert_eq!(pairs, 7, "the seven never-indexed legs of #260");
}


/// A stream handle carries exactly the candlestick settings its own step reads,
/// and no `Core` (issue #274).
///
/// The three rows are the three cases, and each is a control on the others: a
/// step that reads one setting, a step that reads five, and a step that reads
/// none. A handle that went back to embedding `Core` fails all three; one that
/// widened to "every setting" fails the first two on the count; one that
/// narrowed to nothing fails them on the field itself.
#[test]
fn a_stream_handle_carries_only_the_settings_its_step_reads() {
    let registry = make_registry();
    let helpers = HelperRegistry::empty();

    // (indicator, handle, the settings its step reads, in field order)
    let cases: [(&str, &str, &[&str]); 3] = [
        ("cdldoji", "CdldojiStream", &["cs_body_doji"]),
        (
            "cdladvanceblock",
            "CdladvanceblockStream",
            &[
                "cs_body_long",
                "cs_far",
                "cs_near",
                "cs_shadow_long",
                "cs_shadow_short",
            ],
        ),
        ("sma", "SmaStream", &[]),
    ];

    for (name, handle, settings) in cases {
        let (func, enums) = load_indicator(name);
        assert!(func.streaming, "{name} must carry the `stream` flag");
        let rust = backends::rust_lang::generate(&func, &enums, &registry, &helpers);

        let start = rust
            .find(&format!("pub struct {handle} {{"))
            .unwrap_or_else(|| panic!("{name}: no {handle} definition"));
        let body = &rust[start..start + rust[start..].find("\n}").expect("struct end")];

        assert!(
            !body.contains("core: Core,"),
            "{name}: {handle} still embeds a whole Core"
        );
        assert_eq!(
            body.matches(": CandleSetting,").count(),
            settings.len(),
            "{name}: {handle} carries the wrong number of settings\n{body}"
        );
        for field in settings {
            assert!(
                body.contains(&format!("{field}: CandleSetting,")),
                "{name}: {handle} is missing {field}"
            );
        }

        // The step takes them as parameters — it has no receiver to read them
        // through — and the call site hands over the handle's own fields.
        let params: String = settings
            .iter()
            .map(|f| format!(", {f}: &CandleSetting"))
            .collect();
        let args: String = settings.iter().map(|f| format!("&self.{f}, ")).collect();
        assert!(
            rust.contains(&format!(
                "fn {name}_step_impl(sp: &mut {handle}State{params},"
            )),
            "{name}: step signature does not take exactly its settings"
        );
        assert!(
            rust.contains(&format!(
                "Core::{name}_step_impl(&mut self.state, {args}"
            )),
            "{name}: `update` does not hand the step its settings"
        );
    }
}

/// A step unpacks its candle settings from its own parameters; only the batch
/// and `Open` tiers, which run on a `Core` receiver, read `self.candle_settings`
/// (issue #274).
#[test]
fn a_stream_step_reads_candle_settings_from_its_parameters() {
    let registry = make_registry();
    let helpers = HelperRegistry::empty();
    let (func, enums) = load_indicator("cdldoji");
    let rust = backends::rust_lang::generate(&func, &enums, &registry, &helpers);

    let step = rust
        .find("fn cdldoji_step_impl(")
        .expect("cdldoji renders a step");
    let step_body = &rust[step..step + rust[step..].find("\n    }").expect("step end")];

    assert!(
        step_body.contains("let BodyDoji_rangeType: i32 = cs_body_doji.range_type as i32;"),
        "the step must unpack from its parameter\n{step_body}"
    );
    assert!(
        !step_body.contains("self.candle_settings"),
        "the step has no Core receiver to read through\n{step_body}"
    );
    // The control: the batch tier still does, and is the reason the unpacking
    // emitter keeps both spellings.
    assert!(
        rust.contains("let BodyDoji_rangeType: i32 = self.candle_settings.body_doji.range_type as i32;"),
        "the batch tier must still read the Core it runs on"
    );
}


/// The handle's fixed-size accumulator fields: an array the BATCH body declares
/// with a literal size. Off the emitted code, never a name list.
fn csharp_accumulator_fields(section: &str, batch: &str) -> BTreeSet<String> {
    let mut out = BTreeSet::new();
    for line in section.lines() {
        let Some(d) = line.trim().strip_prefix("internal ") else { continue };
        let Some((ty, rest)) = d.split_once(' ') else { continue };
        // A field carries its initializer (`internal double[] x = [];`).
        let name = rest.trim_end_matches(';').split(" =").next().unwrap_or(rest);
        if !ty.ends_with("[]") || !name.chars().all(|c| c.is_alphanumeric() || c == '_') {
            continue;
        }
        let decl = format!("{ty} {name} = new {}[", ty.trim_end_matches("[]"));
        if batch.match_indices(&decl).any(|(i, _)| {
            batch[i + decl.len()..].split_once(']').is_some_and(|(n, _)| n.parse::<u32>().is_ok())
        }) {
            out.insert(name.to_string());
        }
    }
    out
}

/// No C# `Peek` copies the handle: every one runs a frame against `this`, and
/// what it allocates never grows with the period.
///
/// Structural for the same reason the C, Rust and Java sweeps are — a `Peek`
/// that copied and then wrote the copy would still answer correctly, so no
/// value gate can see the difference. What it costs is the flat-in-period cost
/// the frame is for.
///
/// The one allocation a frame is allowed is a fixed-size accumulator: a C#
/// array field is a reference, so a localized one must be cloned or the frame
/// would write the handle through it. "Fixed-size" is read off the emitted code
/// rather than asserted from a name list — the batch body declares exactly
/// these with a literal dimension (`new double[3]`), and a period-sized buffer
/// never is.
#[test]
fn no_csharp_peek_copies_the_handle() {
    /// The handle's own fields, read off the emitted class declarations.
    fn handle_fields(s: &str) -> BTreeSet<String> {
        s.lines()
            .filter_map(|l| l.trim().strip_prefix("internal "))
            .filter_map(|d| d.split_once(' '))
            .map(|(_, rest)| rest.trim_end_matches(';').split(" =").next().unwrap_or(rest))
            .filter(|n| !n.is_empty() && n.chars().all(|c| c.is_alphanumeric() || c == '_'))
            .map(str::to_string)
            .collect()
    }

    /// Every name `line` writes, with any subscript stripped, each paired with
    /// whether that write also DECLARES the name. Empty when it writes nothing.
    ///
    /// Every operator that stores, not just `=`. `x += e`, `x++` and `++x`
    /// reach the same slot `x = e` does, and none of them can declare a name,
    /// so each is a write to a name the frame must already have declared —
    /// which is the rule this sweep exists to enforce. The emitted frames carry
    /// all three shapes: `periodTotal += inReal;` is SMA's, `pkIdx0 = i++ &
    /// sp.xMask;` and `while( ++i <= today )` are the extrema and Hilbert
    /// tiers'. Reading `=` alone enforced the rule on one operator out of
    /// several and passed the rest through as if they wrote nothing.
    fn write_targets(line: &str) -> Vec<(&str, bool)> {
        /// The trailing name of `lhs`: `x` / `sp.x` / `x[i]` -> the name.
        fn name_of(lhs: &str) -> Option<&str> {
            // The declared name is the LAST token; strip its subscript there,
            // not over the whole left side — `double[] x = ...` carries a `[`
            // in the TYPE, and cutting at it would name the type instead.
            let last = lhs.trim().rsplit(' ').next()?;
            let last = last.split_once('[').map_or(last, |(h, _)| h);
            (!last.is_empty() && last.chars().all(|c| c.is_alphanumeric() || c == '_' || c == '.'))
                .then_some(last)
        }

        fn is_name_byte(c: u8) -> bool {
            c.is_ascii_alphanumeric() || c == b'_' || c == b'.'
        }

        /// The identifier ending at `end`, skipping back over a subscript so
        /// `buf[k]++` names `buf`. Empty when there is none.
        fn ident_before(s: &str, end: usize) -> &str {
            let b = s.as_bytes();
            let mut end = end;
            if end > 0 && b[end - 1] == b']' {
                let mut depth = 0usize;
                let mut k = end;
                while k > 0 {
                    k -= 1;
                    match b[k] {
                        b']' => depth += 1,
                        b'[' => {
                            depth -= 1;
                            if depth == 0 {
                                break;
                            }
                        }
                        _ => {}
                    }
                }
                if depth != 0 {
                    return "";
                }
                end = k;
            }
            let mut start = end;
            while start > 0 && is_name_byte(b[start - 1]) {
                start -= 1;
            }
            &s[start..end]
        }

        /// The identifier starting at `start`. Empty when there is none.
        fn ident_after(s: &str, start: usize) -> &str {
            let b = s.as_bytes();
            let mut end = start;
            while end < b.len() && is_name_byte(b[end]) {
                end += 1;
            }
            &s[start..end]
        }

        let l = line.trim().trim_end_matches(';').trim_end();
        let b = l.as_bytes();
        let mut out = Vec::new();

        // The assignment first, so a `for( int i = 0; ...; i++ )` header has
        // declared `i` before its own increment is read below.
        //
        // The first `=` that stores: `==`/`!=`/`<=`/`>=` compare, while
        // `<<=`/`>>=` store — which is why the `<`/`>` case looks one
        // character further back.
        let eq = (0..b.len()).find(|&i| {
            if b[i] != b'=' {
                return false;
            }
            let prev = i.checked_sub(1).map(|k| b[k]);
            let compare = b.get(i + 1) == Some(&b'=')
                || matches!(prev, Some(b'=' | b'!'))
                || (matches!(prev, Some(b'<' | b'>')) && i.checked_sub(2).map(|k| b[k]) != prev);
            !compare
        });
        if let Some(eq) = eq {
            const OPS: [char; 10] = ['+', '-', '*', '/', '%', '&', '|', '^', '<', '>'];
            let lhs = &l[..eq];
            let compound = lhs.ends_with(OPS);
            let lhs = lhs.trim_end_matches(OPS);
            // A declaration is `<type> <name> =`; a compound store never one.
            let declares = !compound && lhs.trim().split(' ').count() > 1;
            out.extend(name_of(lhs).map(|n| (n, declares)));
        }

        // `x++` / `++x`, anywhere on the line — an increment inside a `while`
        // or `if` header writes exactly as one on its own does.
        let mut i = 0usize;
        while i + 1 < b.len() {
            if !((b[i] == b'+' && b[i + 1] == b'+') || (b[i] == b'-' && b[i + 1] == b'-')) {
                i += 1;
                continue;
            }
            let before = ident_before(l, i);
            let name = if before.is_empty() { ident_after(l, i + 2) } else { before };
            if !name.is_empty() {
                out.push((name, false));
            }
            i += 2;
        }
        out
    }

    let registry = make_registry();
    let helpers = HelperRegistry::empty();
    let (mut swept, mut frames, mut writes) = (0usize, 0usize, 0usize);
    let mut fully_shadowed: BTreeSet<String> = BTreeSet::new();
    let mut offenders: Vec<String> = Vec::new();
    let mut bounded: BTreeSet<String> = BTreeSet::new();
    let mut fixtures = 0usize;

    for name in discover_indicators() {
        let (func, enums) = load_indicator(&name);
        if !func.streaming || !backends::csharp_stream::emits_stream(&func, &registry) {
            continue;
        }
        let batch = backends::csharp::generate(&func, &enums, &registry, &helpers);
        let s = backends::csharp_stream::generate(&func, &enums, &registry, &helpers);
        let mut at = 0usize;
        while let Some(k) = s[at..].find("\n      public ") {
            let start = at + k + 1;
            at = start + 1;
            let Some(nl) = s[start..].find('\n') else { break };
            if !s[start..start + nl].contains(" Peek( ") {
                continue;
            }
            let end = s[start..].find("\n      }").map_or(s.len(), |e| start + e);
            let peek = &s[start..end];
            swept += 1;
            if peek.contains("Stream sp = this;") {
                frames += 1;
            }
            let fields = handle_fields(&s);
            // Hoisted: the copy check consults it per line, and the
            // non-vacuity counter below still uses the same set.
            let accs = csharp_accumulator_fields(&s, &batch);
            // A `synth<n>` fixture is a construct probe copied into input/ by
            // scripts/synth_gate.py, never a shipped function.
            let fixture = name.starts_with("synth");
            if fixture {
                fixtures += 1;
            }
            let mut locals: BTreeSet<&str> = BTreeSet::new();
            for line in peek.lines() {
                let l = line.trim();
                if l.starts_with("//") || l.starts_with("/*") || l.starts_with('*') {
                    continue;
                }
                // A frame writes locals. A bare `cur_x = ...` whose name the
                // frame never DECLARED resolves to the handle field of that
                // name and commits it — which is what a composed output reached
                // only through an alias used to do, invisibly to every value
                // gate but C#'s `valueNeUpdate` leg.
                for (t, declared) in write_targets(l) {
                    if declared {
                        locals.insert(t);
                    } else if t.starts_with("sp.") || (fields.contains(t) && !locals.contains(t)) {
                        offenders.push(format!("{name}: writes the handle: {l}"));
                    } else {
                        writes += 1;
                    }
                }
                if l.contains("CopyFrom") || l.contains("peekScratch") {
                    offenders.push(format!("{name}: {l}"));
                    continue;
                }
                if !l.contains("new ") || l.starts_with("throw new") || l.starts_with("return new")
                {
                    continue;
                }
                // The one copy the doc comment above has always allowed, now
                // implemented rather than only described: a FIXED-SIZE
                // accumulator, sized off its own field
                // (`new int[sp.ring.Length]`) because the batch declared it
                // with a literal dimension. A period-sized buffer is never in
                // `accs`, so it cannot qualify.
                //
                // Still an offender for a SHIPPED function: the emitter reaches
                // the copy only where it cannot shadow the write in place, and
                // nothing shipped is in that position, so a shipped one that
                // started copying is a regression into the fallback.
                let copied = l
                    .split_once("new ")
                    .and_then(|(_, r)| r.split_once("[sp."))
                    .and_then(|(_, r)| r.strip_suffix(".Length];"));
                if let Some(f) = copied.filter(|f| accs.contains(*f)) {
                    if fixture {
                        bounded.insert(format!("{name}.{f}"));
                    } else {
                        offenders.push(format!(
                            "{name}: a SHIPPED Peek copies the accumulator {f} instead of \
                             shadowing the write: {l}"
                        ));
                    }
                    continue;
                }
                offenders.push(format!("{name}: {l}"));
            }
            // The frame must READ an accumulator: a field it never names is
            // no evidence about the copy either way.
            if accs.iter().any(|f| peek.contains(&format!("{f}["))) {
                fully_shadowed.insert(name.clone());
            }
        }
    }

    assert!(swept >= 200, "only {swept} Peek(s) swept");
    assert_eq!(frames, swept, "{frames} of {swept} Peek(s) run a frame");
    assert!(
        fully_shadowed.len() >= 21,
        "only {} handle(s) have a Peek frame that reads an accumulator — the sweep \
         is looking for something that is not there",
        fully_shadowed.len()
    );
    assert!(writes >= 500, "only {writes} local writes seen — the store sweep found nothing");
    assert!(offenders.is_empty(), "a Peek copies:\n{}", offenders.join("\n"));

    // Non-vacuity for the exemption, asserted only where it can be: on the
    // shipped corpus the set is EMPTY and must be. The fixtures are the only
    // thing that reaches the fallback.
    if fixtures > 0 {
        assert!(
            !bounded.is_empty(),
            "{fixtures} fixture(s) swept and none reached the bounded-accumulator \
             copy — the exemption is dead code and proves nothing"
        );
    }
}

/// The crate front page's category index (#179 D6) must list every indicator in
/// the corpus exactly once, under the group its own definition names.
///
/// Rustdoc gates the *links* — a `[`X`](Core::X)` naming a method that does not
/// exist is `rustdoc::broken_intra_doc_links`, and the nightly runs rustdoc with
/// `-D warnings`. Nothing gates an *omission*: a filter that quietly drops a
/// function leaves a page that still builds clean and simply never mentions it.
/// That is the failure this test exists for, so it counts rather than samples.
#[test]
fn rust_category_index_lists_every_function_once() {
    let funcs: Vec<ir::FuncDef> = discover_indicators()
        .iter()
        .map(|name| load_indicator(name).0)
        .collect();
    assert!(funcs.len() >= 200, "only {} indicators in the corpus", funcs.len());

    let index = backends::rust_doc::category_index(&funcs);

    // One bullet per function, spelled as the link rustdoc will resolve.
    for f in &funcs {
        let line = format!("//! * [`{0}`](Core::{0})", f.name);
        assert_eq!(
            index.matches(&line).count(),
            1,
            "{} must appear exactly once in the category index",
            f.name
        );
    }
    assert_eq!(
        index.matches("//! * [`").count(),
        funcs.len(),
        "the index must carry no entry beyond the corpus"
    );

    // Each heading's count is the number of bullets that follow it, so a reader
    // can trust "Pattern Recognition (61)" without counting the list.
    let mut heading_total = 0;
    for section in index.split("//! ## ").skip(1) {
        let (heading, body) = section.split_once('\n').expect("a heading ends its line");
        let (group, count) = heading.rsplit_once(" (").expect("a heading carries its count");
        let count: usize = count.trim_end_matches(')').parse().expect("a decimal count");
        let bullets = body.matches("//! * [`").count();
        assert_eq!(count, bullets, "{group} says {count} but lists {bullets}");
        assert_eq!(
            bullets,
            funcs.iter().filter(|f| f.group == group).count(),
            "{group} must list every function filed under it"
        );
        heading_total += bullets;
    }
    assert_eq!(heading_total, funcs.len(), "every function must land under a heading");
}
