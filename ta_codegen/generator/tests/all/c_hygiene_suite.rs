//! Properties of the committed C and of the headers it ships with, both read
//! off DISK rather than re-rendered.
//!
//! The hygiene half: the committed C is a fixed point of the hygiene pass.
//!
//! Read off DISK rather than re-rendered, so it fails for a C producer that
//! never routed its text through the phase — which is the whole maintenance
//! contract: a new emitter inherits the cleanup, and one that bypasses it says
//! so here instead of shipping a cast that claims a name is unused when the
//! block below reads it.

use std::path::{Path, PathBuf};

use ta_codegen_lib::backends::c_hygiene::scrub_void_casts;

fn repo_root() -> PathBuf {
    Path::new(env!("CARGO_MANIFEST_DIR")).join("../..")
}

/// Every `.c` under a directory the generator owns.
fn generated_c() -> Vec<PathBuf> {
    let root = repo_root();
    let mut files = Vec::new();
    for dir in ["src/ta_func", "src/ta_abstract", "ta_codegen/output/c/tools"] {
        let mut stack = vec![root.join(dir)];
        while let Some(d) = stack.pop() {
            let Ok(rd) = std::fs::read_dir(&d) else { continue };
            for e in rd.flatten() {
                let p = e.path();
                if p.is_dir() {
                    stack.push(p);
                } else if p.extension().is_some_and(|x| x == "c") {
                    files.push(p);
                }
            }
        }
    }
    files.sort();
    files
}

#[test]
fn no_committed_c_file_casts_away_a_name_its_block_reads() {
    let files = generated_c();
    assert!(files.len() >= 200, "only {} generated .c file(s) found", files.len());
    let (mut casts, mut with_casts) = (0usize, 0usize);
    let mut offenders: Vec<String> = Vec::new();
    for p in &files {
        let src = std::fs::read_to_string(p).expect("readable");
        let n = src.matches("(void)").count();
        casts += n;
        with_casts += usize::from(n > 0);
        if scrub_void_casts(&src) != src {
            offenders.push(p.display().to_string());
        }
    }
    // A corpus with no casts left would satisfy the sweep saying nothing.
    assert!(
        casts > 0 && with_casts > 0,
        "{casts} cast(s) across {with_casts} file(s) — the sweep has nothing to judge"
    );
    assert!(
        offenders.is_empty(),
        "{} file(s) still cast away a name their own block reads — route the producer's \
         text through `c_hygiene::scrub_void_casts`:\n{}",
        offenders.len(),
        offenders.join("\n")
    );
}

/// Every function a shipped header DECLARES carries `TA_LIB_API`.
///
/// The attribute is the whole export mechanism on every platform: on Windows
/// `TA_LIB_API` is `__declspec(dllexport)`, and on ELF/Mach-O the library builds
/// `-fvisibility=hidden`, so a prototype without it is simply missing from the
/// shipped library. `abi.py`'s export gate catches the same omission from
/// the built artifact; this catches it without a build, on the PR gate.
///
/// Over the INSTALL set, read from CMake's `LIB_HEADERS`, not a glob of
/// `include/*.h`: `ta_config.h` is generated and untracked, so a glob sweeps a
/// build artifact in a configured tree and not in a fresh checkout. #57 was in
/// a hand-written header, and the streaming tier spells the attribute in nine
/// separate emitter literals, so neither half is structurally safe alone.
///
/// One shape it cannot see: a function RETURNING a function pointer
/// (`void (*TA_GetHook( int ))( int );`) is indistinguishable here from a
/// function-pointer declarator and is skipped. No such declaration exists in
/// this API, and adding one would need this rule revisited.
///
/// Split into STATEMENTS, not scanned line by line. A line sweep silently skips
/// whatever it does not recognise as a declaration start, and every blind spot
/// is ordinary header style: indented inside an `#ifdef`, behind a leading
/// `/* ... */`, written `extern int f(void);`, a space before the parameter
/// list. Braces terminate a statement too, or `extern "C" {` swallows
/// everything up to the first `;`.
#[test]
fn every_declared_function_in_a_shipped_header_is_exported() {
    // Empty, and it is an exact SET: a prototype arriving without the attribute
    // is as loud as an exemption being dropped. Nothing is exempt any more --
    // the retired TA_Set/GetCompatibility pair carries `TA_LIB_API` again, so
    // the library exports what its headers promise on every platform.
    let expected_exempt: std::collections::BTreeSet<String> =
        std::collections::BTreeSet::new();

    let mut headers = 0usize;
    let mut exempt: std::collections::BTreeSet<String> = std::collections::BTreeSet::new();

    for file in installed_headers() {
        let path = repo_root().join("include").join(&file);
        let text = std::fs::read_to_string(&path)
            .unwrap_or_else(|e| panic!("{} is in LIB_HEADERS but unreadable: {e}", path.display()));
        // A brace ends a statement as well as a `;`.
        let src = strip_preprocessor(&strip_comments(&text)).replace(['{', '}'], ";");
        let mut seen_here = 0usize;
        let mut exempt_here = 0usize;
        let mut attributed_here = 0usize;
        for chunk in src.split(';') {
            let mut t = chunk.split_whitespace().collect::<Vec<_>>().join(" ");
            // `extern "C"` alone is the linkage wrapper; `extern "C" <proto>` is
            // a declaration wearing one, and must still be checked.
            if let Some(rest) = t.strip_prefix("extern \"") {
                match rest.split_once('"') {
                    Some((_, after)) => t = after.trim().to_string(),
                    None => continue,
                }
            }
            let Some(paren) = t.find('(') else { continue };
            // Counted on every chunk that has a parameter list, BEFORE the
            // classification below: that keeps the cross-check independent of
            // the rules that decide what a prototype is — which is where a
            // parse regression lives — while leaving out an exported DATA
            // symbol, which carries the attribute and is no prototype.
            attributed_here += t.matches("TA_LIB_API").count();
            let head = t[..paren].trim_end();
            // Three things wearing a parameter list that are not prototypes, each
            // of which would otherwise red the gate on a routine header change:
            // a function-POINTER declarator (`TA_RetCode ( *cb )( int )`, which
            // reports under its return TYPE), an internal `static inline` helper
            // (which must NOT be exported), and any parenthesised initializer
            // such as an enum value (no identifier before the `(` at all).
            // A `static inline` DEFINITION also leaves body chunks behind, since
            // its braces became `;` — and a body statement wears a call the way a
            // prototype wears a parameter list. A prototype always has a return
            // type, so its head always contains a space; an initializer and a
            // statement keyword never head one. All three are dropped.
            if t.starts_with("typedef")
                || t[paren + 1..].trim_start().starts_with('*')
                || head.contains('=')
                || !head.contains(' ')
                || t.split(' ').next().is_some_and(|w| {
                    matches!(
                        w,
                        "static"
                            | "inline"
                            | "__inline"
                            | "__forceinline"
                            | "return"
                            | "if"
                            | "else"
                            | "for"
                            | "while"
                            | "do"
                            | "switch"
                    )
                })
                || !head.ends_with(|c: char| c.is_alphanumeric() || c == '_')
            {
                continue;
            }
            seen_here += 1;
            if t.contains("TA_LIB_API") {
                continue;
            }
            let name = head
                .rsplit(|c: char| !(c.is_alphanumeric() || c == '_'))
                .next()
                .filter(|s| !s.is_empty())
                .unwrap_or("<unnamed>");
            exempt.insert(format!("{file}:{name}"));
            exempt_here += 1;
        }
        if seen_here > 0 {
            headers += 1;
        }
        // The non-vacuity check, and it is an IDENTITY rather than a floor: the
        // same header measured a second, independent way. A raw token count does
        // not care how the statement split went, so a parse that quietly stopped
        // recognising a whole family of prototypes — 402 `_Clone`/`_Advance`
        // declarations, say — moves `seen_here` and leaves this untouched. A
        // floor with slack in it cannot see that; this cannot miss it.
        assert_eq!(
            seen_here - exempt_here,
            attributed_here,
            "{file}: swept {seen_here} prototype(s) of which {exempt_here} lack the attribute, \
             but the file carries a different number of TA_LIB_API tokens — the statement split \
             is no longer seeing what the header declares"
        );
    }

    assert!(headers >= 3, "only {headers} header(s) contributed a prototype");

    assert_eq!(
        exempt, expected_exempt,
        "the set of shipped prototypes WITHOUT TA_LIB_API moved. A new one is declared in a \
         shipped header but missing from the built library on every platform, which is a link \
         failure in somebody else's build (CHANGELOG #57); one leaving means a deliberate \
         exemption was dropped, which widens the shipped surface"
    );
}

/// The headers CMake installs — `LIB_HEADERS`, which is what "shipped" means.
fn installed_headers() -> Vec<String> {
    let raw = std::fs::read_to_string(repo_root().join("CMakeLists.txt")).expect("CMakeLists.txt");
    // CMake comments first: a `#` one inside the block carries a `)` often
    // enough, and the block would then end at it with the cross-check below
    // computed on the same truncated slice, unable to see the truncation.
    let cmake: String = raw
        .lines()
        .map(|l| l.split('#').next().unwrap_or(""))
        .collect::<Vec<_>>()
        .join("\n");
    let at = cmake.find("set(LIB_HEADERS").expect("LIB_HEADERS");
    let end = cmake[at..].find(')').expect("LIB_HEADERS closes") + at;
    let block = &cmake[at..end];
    // Split on the SEPARATORS, not on lines: two entries on one line would
    // otherwise collapse to the last, and a line-based count would agree with
    // the collapse, dropping a header in silence — the same
    // invisible-on-every-dev-platform failure this test exists to prevent.
    let names: Vec<String> = block
        .split(|c: char| c.is_whitespace() || c == '"')
        .filter_map(|tok| tok.rsplit_once("/include/"))
        .map(|(_, n)| n.to_string())
        .collect();
    // Counted off the `.h` tokens, not off `/include/`: keying both sides on
    // the same literal lets an entry stored anywhere else drop from the list and
    // from the check together.
    let listed = block.split_whitespace().filter(|t| t.contains(".h")).count();
    assert_eq!(
        names.len(),
        listed,
        "LIB_HEADERS names {listed} header path(s) but {} parsed: {names:?}",
        names.len()
    );
    assert!(names.len() >= 5, "LIB_HEADERS parsed to {names:?} — the list shrank");
    names
}

/// Comments blanked, newlines kept. Literal-aware: a `/*` inside a string is
/// not a comment, and reading it as one swallows real declarations silently.
fn strip_comments(src: &str) -> String {
    let b = src.as_bytes();
    let mut out = String::with_capacity(src.len());
    let mut i = 0usize;
    while i < b.len() {
        if b[i] == b'/' && i + 1 < b.len() && b[i + 1] == b'*' {
            i += 2;
            while i < b.len() && !(b[i] == b'*' && i + 1 < b.len() && b[i + 1] == b'/') {
                if b[i] == b'\n' {
                    out.push('\n');
                }
                i += 1;
            }
            i = (i + 2).min(b.len());
        } else if b[i] == b'/' && i + 1 < b.len() && b[i + 1] == b'/' {
            while i < b.len() && b[i] != b'\n' {
                i += 1;
            }
        } else if b[i] == b'"' || (b[i] == b'\'' && closes_as_char_literal(b, i)) {
            let quote = b[i];
            out.push(quote as char);
            i += 1;
            while i < b.len() && b[i] != quote {
                if b[i] == b'\\' && i + 1 < b.len() {
                    out.push(b[i] as char);
                    i += 1;
                }
                out.push(b[i] as char);
                i += 1;
            }
            if i < b.len() {
                out.push(quote as char);
                i += 1;
            }
        } else {
            out.push(b[i] as char);
            i += 1;
        }
    }
    out
}

/// Directive lines blanked, backslash continuations included — a multi-line
/// `#define` otherwise leaks its body into the statement split.
fn strip_preprocessor(src: &str) -> String {
    let mut out = String::with_capacity(src.len());
    let mut continued = false;
    for line in src.lines() {
        if continued || line.trim_start().starts_with('#') {
            continued = line.trim_end().ends_with('\\');
        } else {
            out.push_str(line);
        }
        out.push('\n');
    }
    out
}

/// A `'` opens a char literal only if one closes it within a few bytes on the
/// same line. Otherwise it is an apostrophe — `#error can't build without X` —
/// and treating it as a literal turns comment recognition off for the rest of
/// the file.
fn closes_as_char_literal(b: &[u8], at: usize) -> bool {
    let mut i = at + 1;
    let end = (at + 5).min(b.len());
    while i < end {
        match b[i] {
            b'\n' => return false,
            b'\\' => i += 2,
            b'\'' => return true,
            _ => i += 1,
        }
    }
    false
}

/// The functions whose own source calls a transcendental math routine, or that
/// reach one through a function they call by name. MA is followed in neither
/// direction: which average it runs is the caller's choice, and the harness
/// decides that per call.
fn transcendental_functions() -> std::collections::BTreeSet<String> {
    use std::collections::{BTreeMap, BTreeSet};
    use ta_codegen_lib::backends::builtins::MathFn;
    use ta_codegen_lib::ir::Expr;

    let names = crate::common::discover_indicators();
    let known: BTreeSet<String> = names.iter().map(|n| n.to_uppercase()).collect();
    let mut direct: BTreeSet<String> = BTreeSet::new();
    let mut calls: BTreeMap<String, BTreeSet<String>> = BTreeMap::new();
    for name in &names {
        let (func, _) = crate::common::load_indicator(name);
        let upper = func.name.to_uppercase();
        let mut seen: BTreeSet<String> = BTreeSet::new();
        let alternates = func.alternates.iter().flat_map(|a| a.body.iter());
        for stmt in func.body.iter().chain(func.private_body.iter()).chain(alternates) {
            ta_codegen_lib::streaming::walk_stmt_exprs(stmt, &mut |e| {
                ta_codegen_lib::streaming::walk_expr(e, &mut |e| {
                    if let Expr::FuncCall(callee, _) = e {
                        seen.insert(callee.clone());
                    }
                });
            });
        }
        for callee in &seen {
            if matches!(
                MathFn::from_name(callee),
                Some(
                    MathFn::Atan
                        | MathFn::Log
                        | MathFn::Cos
                        | MathFn::Sin
                        | MathFn::Tan
                        | MathFn::Acos
                        | MathFn::Asin
                        | MathFn::Exp
                        | MathFn::Cosh
                        | MathFn::Sinh
                        | MathFn::Tanh
                        | MathFn::Log10
                )
            ) {
                direct.insert(upper.clone());
            }
            let target = callee.trim_start_matches("TA_").to_uppercase();
            if known.contains(&target) && target != "MA" && upper != "MA" && target != upper {
                calls.entry(upper.clone()).or_default().insert(target);
            }
        }
    }
    let mut all = direct;
    loop {
        let more: Vec<String> = calls
            .iter()
            .filter(|(f, callees)| !all.contains(*f) && callees.iter().any(|c| all.contains(c)))
            .map(|(f, _)| f.clone())
            .collect();
        if more.is_empty() {
            return all;
        }
        all.extend(more);
    }
}

/// `CODEGEN_TRANSCENDENTAL[]` is what drops a Java or C# comparison from bitwise
/// to a tolerance. A name it should not hold loosens a gate in silence, so the
/// list must be exactly what the sources say.
#[test]
fn the_transcendental_list_is_what_the_sources_call() {
    let derived = transcendental_functions();
    let harness = std::fs::read_to_string(
        repo_root().join("src/tools/ta_regtest/test_codegen.c"),
    )
    .expect("test_codegen.c");
    let at = harness
        .find("CODEGEN_TRANSCENDENTAL[] = {")
        .expect("CODEGEN_TRANSCENDENTAL[]");
    let end = harness[at..].find("};").expect("the array closes") + at;
    let listed: std::collections::BTreeSet<String> = harness[at..end]
        .split('"')
        .skip(1)
        .step_by(2)
        .map(str::to_string)
        .collect();
    assert!(derived.len() >= 20, "derived only {derived:?}");
    assert_eq!(
        listed, derived,
        "CODEGEN_TRANSCENDENTAL[] in test_codegen.c no longer matches the functions \
         that call a transcendental routine"
    );

    // The published list is the same one.
    let page = std::fs::read_to_string(repo_root().join("website/src/spec/versions/README.md"))
        .expect("the versions page");
    let from = page.find("The functions that do are ").expect("the published list")
        + "The functions that do are ".len();
    let to = page[from..].find(", and the averages").expect("the list's end") + from;
    let published: std::collections::BTreeSet<String> = page[from..to]
        .replace(" and ", ", ")
        .split(", ")
        .map(|n| n.trim().to_string())
        .collect();
    assert_eq!(
        published, derived,
        "the transcendental functions listed on website/src/spec/versions/README.md \
         no longer match the sources"
    );
}

/// The flags the three build systems must agree on, and the ones none may use.
/// `-ffp-contract=off` is the value contract, and the cross-language bit
/// compares see only CMake's and the generator's builds. The other three change
/// no value, so no test of any build can see one go missing.
///
/// Each needle is a statement that sets the flag, not the flag's name: a build
/// file also names a flag in the message that probes for it.
#[test]
fn the_three_build_systems_carry_the_same_flags() {
    let strip = |text: String, marker: &str| -> String {
        text.lines()
            .map(|l| l.split(marker).next().unwrap_or(""))
            .collect::<Vec<_>>()
            .join("\n")
    };
    let read = |rel: &str| std::fs::read_to_string(repo_root().join(rel)).expect(rel);
    let files = [
        (
            "CMakeLists.txt",
            strip(read("CMakeLists.txt"), "#"),
            vec![
                "add_compile_options(-ffp-contract=off)",
                "add_compile_options(-fno-math-errno)",
                "foreach(flag -falign-functions=64 -falign-loops=64)",
                "add_compile_options(${flag})",
            ],
        ),
        (
            "configure.ac",
            strip(read("configure.ac"), "#"),
            vec![
                "CFLAGS=\"$CFLAGS -ffp-contract=off\"",
                "CFLAGS=\"$CFLAGS -fno-math-errno\"",
                "for ta_align in -falign-functions=64 -falign-loops=64;",
                "CFLAGS=\"$CFLAGS $ta_align\"",
            ],
        ),
        (
            "ta_codegen/generator/src/main.rs",
            strip(read("ta_codegen/generator/src/main.rs"), "//"),
            vec![
                "\"-ffp-contract=off\",",
                "\"-fno-math-errno\",",
                "&[\"-falign-functions=64\", \"-falign-loops=64\"]",
            ],
        ),
    ];
    for (name, text, needles) in &files {
        for needle in needles {
            assert!(text.contains(needle), "{name} no longer has `{needle}`");
        }
        for banned in ["-ffast-math", "-Ofast", "-funsafe-math-optimizations"] {
            assert!(!text.contains(banned), "{name} passes {banned}");
        }
    }
}
