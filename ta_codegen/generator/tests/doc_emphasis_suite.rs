//! The canonical `input/<name>/<name>.md` is Markdown. Rust's doc target is
//! Markdown too, so rustdoc renders it; Java's is HTML and C#'s is XML, and
//! whatever those emitters do not convert reaches the reader as punctuation —
//! in the published javadoc, the NuGet XML docs and every IDE hover.
//!
//! Nothing else can see this class. `javadoc -Xdoclint:all,-missing` (what
//! `pom.xml` enforces) and `csc` under `-warnaserror` with
//! `GenerateDocumentationFile` both accept literal Markdown without a
//! diagnostic, and `regen-check` only pins that the committed output matches the
//! emitter — so it agrees with whatever is emitted. This suite is the gate.
//!
//! The two escapers ask `backends::common::emphasis_open` what counts, so the
//! HTML and XML targets cannot come to disagree about it while each keeps its
//! own tags. Rust asks nothing: it never called them.
//!
//! Scope: inline emphasis, which is what the emitters now convert. It sweeps the
//! *emitted doc blocks* rather than the generated files, because a generated
//! file also carries the C source's own changelog header verbatim (`IMI`'s
//! `Fix #112: ... a *successful* call`), which is not Markdown and must not be
//! converted. Reading the emitter's output directly is what tells those apart.
//!
//! Not in scope, and deliberately: the `## Formula` block, which
//! `split_formula_note` hands to the emitters as preformatted text whenever the
//! section carries no closing `$$` (148 of 150 sections). Its backticks and
//! Markdown links are a separate defect with a separate blast radius.

use std::collections::HashMap;
use std::path::PathBuf;
use ta_codegen_lib::helper_registry::HelperRegistry;
use ta_codegen_lib::registry::Registry;
use ta_codegen_lib::{backends, ir, parser};

fn input_dir() -> PathBuf {
    PathBuf::from(env!("CARGO_MANIFEST_DIR")).join("../input")
}

/// Every indicator directory that carries a canonical doc file. A function
/// without one contributes no prose, so it cannot carry this defect.
fn documented_indicators() -> Vec<String> {
    let mut v: Vec<String> = std::fs::read_dir(input_dir())
        .expect("input dir")
        .filter_map(Result::ok)
        .filter(|e| e.path().is_dir())
        .filter_map(|e| {
            let name = e.file_name().to_string_lossy().to_string();
            let dir = e.path();
            (dir.join(format!("{name}.yaml")).exists()
                && dir.join(format!("{name}.c")).exists()
                && dir.join(format!("{name}.md")).exists())
            .then_some(name)
        })
        .collect();
    v.sort();
    v
}

fn load(name: &str) -> (ir::FuncDef, HashMap<String, ir::EnumDef>) {
    let dir = input_dir().join(name);
    let mut func = parser::yaml::parse_yaml(&dir.join(format!("{name}.yaml")));
    let parsed = parser::c_source::parse_c_source(&dir.join(format!("{name}.c")));
    parser::c_source::wire_parsed_source(&mut func, &parsed);
    func.doc = parser::doc_md::parse_doc_md(&dir.join(format!("{name}.md")));
    let enums = parser::enums::load_enums(&input_dir().join("enums.yaml"));
    (func, enums)
}

/// Both doc surfaces of one function: the guarded entry point and the lookback,
/// in both precisions. The single-precision overload is a separate render, and
/// #386's link conversion shipped correct in one and stale in the other, so
/// checking only the double overload would have missed half of it.
fn rendered_docs(name: &str, registry: &Registry) -> Vec<(String, String)> {
    let (func, enums) = load(name);
    let upper = func.name.to_uppercase();
    let mut out = Vec::new();
    for single in [false, true] {
        let tag = if single { "float" } else { "double" };
        out.push((
            format!("java {upper} guarded/{tag}"),
            backends::java_doc::guarded_docs(&func, &upper, single, &enums, registry),
        ));
        out.push((
            format!("csharp {upper} guarded/{tag}"),
            backends::csharp_doc::guarded_docs(&func, &upper, single, &enums),
        ));
    }
    out.push((
        format!("java {upper} lookback"),
        backends::java_doc::lookback_docs(&func, &upper, &enums),
    ));
    out.push((
        format!("csharp {upper} lookback"),
        backends::csharp_doc::lookback_docs(&func, &upper, &enums),
    ));
    out
}

/// The doc-comment lines of a rendered block, with the comment marker stripped
/// and preformatted blocks dropped.
///
/// Both strips matter. A Javadoc line carries its own leading `*`, which reads
/// as an emphasis opener if it is left on — the first version of this sweep
/// reported 982 italics in the Java output on that alone. And a formula block is
/// preformatted, so its asterisks are math; leaving it in would make this gate
/// fail on the defect it is not fixing.
fn prose_lines(block: &str) -> Vec<String> {
    let mut out = Vec::new();
    let mut in_pre = false;
    for raw in block.lines() {
        let s = raw.trim();
        let body = if let Some(b) = s.strip_prefix("///") {
            b
        } else if let Some(b) = s.strip_prefix('*') {
            b
        } else {
            continue;
        };
        // Order matters: the close marker is on its own line, and `<code>` is a
        // prefix of nothing else here.
        if body.contains("}</pre>") || body.contains("</code>") {
            in_pre = false;
            continue;
        }
        if body.contains("<pre>{@code") || body.contains("<code>") {
            in_pre = true;
            continue;
        }
        if in_pre {
            continue;
        }
        out.push(body.to_string());
    }
    out
}

/// Blank out `{@code ...}` / `{@link ...}` / `<c>...</c>` spans, and the URL of
/// an `<a href>` / `<see href>`. An asterisk inside a code span is source text
/// the author wrote (`optInROC*Period`), not markup, and must survive the
/// emitter untouched — so it must not be reported here either. A link's
/// destination is a URL, where `*` is a path character; its label is prose and
/// stays visible, because the emitters recurse into it.
fn mask_code_spans(line: &str) -> String {
    let mut out = String::new();
    let chars: Vec<char> = line.chars().collect();
    let mut i = 0;
    while i < chars.len() {
        for (open, close) in [
            ("{@code ", "}"),
            ("{@link ", "}"),
            ("<c>", "</c>"),
            ("<a href=\"", "\""),
            ("<see href=\"", "\""),
        ] {
            let rest: String = chars[i..].iter().collect();
            if rest.starts_with(open) {
                let end = rest.find(close).map_or(chars.len() - i, |p| p + close.len());
                out.extend(std::iter::repeat_n('\u{0}', end));
                i += end;
                break;
            }
        }
        if i >= chars.len() {
            break;
        }
        if out.chars().count() < i {
            continue;
        }
        out.push(chars[i]);
        i += 1;
    }
    out
}

/// Positions of a run of exactly `len` asterisks, ignoring masked spans.
fn emphasis_runs(line: &str, len: usize) -> usize {
    let chars: Vec<char> = line.chars().collect();
    let mut n = 0;
    let mut i = 0;
    while i < chars.len() {
        if chars[i] == '*' {
            let run = chars[i..].iter().take_while(|&&c| c == '*').count();
            if run == len {
                n += 1;
            }
            i += run;
        } else {
            i += 1;
        }
    }
    n
}

/// Every rendered Java/C# doc line that still carries a Markdown emphasis
/// delimiter, as `label | line`.
fn emphasis_survivors() -> Vec<String> {
    let registry = Registry::from_dir(&input_dir());
    let mut out = Vec::new();
    for name in documented_indicators() {
        for (label, block) in rendered_docs(&name, &registry) {
            for line in prose_lines(&block) {
                let masked = mask_code_spans(&line);
                // A `*` that pairs is emphasis the emitter should have
                // converted; an odd count means a literal asterisk, which is
                // prose. Count runs rather than characters so `**` is one event.
                let singles = emphasis_runs(&masked, 1);
                let doubles = emphasis_runs(&masked, 2);
                if doubles > 0 || singles > 1 {
                    out.push(format!("{label} | {}", line.trim()));
                }
            }
        }
    }
    out
}

/// The gate. A paired `*`/`**` in a rendered Java or C# doc line is a Markdown
/// delimiter the reader sees as punctuation.
#[test]
fn no_markdown_emphasis_reaches_the_java_or_csharp_docs() {
    let survivors = emphasis_survivors();
    assert!(
        survivors.is_empty(),
        "{} rendered doc line(s) still carry Markdown emphasis — the HTML and XML \
         targets render markup, not Markdown:\n  {}",
        survivors.len(),
        survivors.join("\n  ")
    );
}

/// The sweep above is only worth its runtime if it can see the defect. Feed the
/// escapers the shapes #404 measured and require each to come back as a tag.
///
/// Without this, a `prose_lines` that silently matched nothing — the failure
/// mode that made the first draft of this file report 982 phantom italics, in
/// the other direction — would read as a clean sweep.
#[test]
fn the_sweep_would_see_a_reintroduced_delimiter() {
    let registry = Registry::from_dir(&input_dir());
    // CMOU carries `**plain moving-window sums**` and `*The New Technical
    // Trader*`; ADR carries `*within*`. Both are in the corpus, so a regression
    // in the escapers turns these assertions red.
    for (name, expected) in [("cmou", "<b>plain moving-window sums</b>"), ("adr", "<i>within</i>")] {
        let blocks = rendered_docs(name, &registry);
        let java = blocks
            .iter()
            .find(|(l, _)| l.starts_with("java"))
            .map(|(_, b)| b.clone())
            .expect("java block");
        assert!(
            java.contains(expected),
            "{name}: expected {expected} in the rendered Java doc — if the corpus \
             reworded this line, repoint the probe at another emphasis span rather \
             than deleting the check.\n{java}"
        );
    }

    // And that the sweep's own reader would report a delimiter if one appeared:
    // the same line with the tags left as Markdown must be flagged.
    let lines = prose_lines(" * a **bold** span\n * plain text\n");
    assert_eq!(lines.len(), 2, "prose_lines must see both lines: {lines:?}");
    assert_eq!(emphasis_runs(&mask_code_spans(&lines[0]), 2), 2, "the `**` pair must count");
    assert_eq!(emphasis_runs(&mask_code_spans(&lines[1]), 1), 0);
}

/// A code span's asterisks are the author's text. Nothing in the corpus asserts
/// this today, and the natural fix — convert every `*` — would silently rewrite
/// parameter names inside `{@code ...}`.
#[test]
fn a_code_span_keeps_its_asterisks() {
    let registry = Registry::from_dir(&input_dir());
    let blocks = rendered_docs("coppock", &registry);
    let java = blocks
        .iter()
        .find(|(l, _)| l.starts_with("java"))
        .map(|(_, b)| b.clone())
        .expect("java block");
    assert!(
        java.contains("optInROC*Period"),
        "COPPOCK's prose names optInROC*Period; the emitter must leave it alone.\n{java}"
    );
}

/// The emphasis arm runs before the `[` arm in both escapers, so it is the one
/// thing that could swallow a link on its way past. The inline-link conversion
/// landed in e46e97c0a, after the emphasis work was branched, and a rebase that
/// dropped it would leave `](` in the docs with every test still green — the
/// link conversion has its own unit tests in each escaper, and none of them
/// would notice an emphasis scanner consuming the bracket first.
#[test]
fn links_still_become_anchors() {
    let registry = Registry::from_dir(&input_dir());
    let blocks = rendered_docs("cdl2crows", &registry);
    for (label, block) in &blocks {
        if !block.contains("thepatternsite.com") {
            continue;
        }
        let expected = if label.starts_with("java") {
            "<a href=\"https://thepatternsite.com/TwoCrows.html\">"
        } else {
            "<see href=\"https://thepatternsite.com/TwoCrows.html\">"
        };
        assert!(
            block.contains(expected),
            "{label}: the Markdown link must still convert; found no {expected}\n{block}"
        );
        // `](` and not `]`: the prose legitimately carries `{@code double[]}`
        // and `endIdx - startIdx + 1`. Only the two together are a link target.
        assert!(!block.contains("]("), "{label}: a Markdown link target survived\n{block}");
    }
}

/// Rust must not change: rustdoc renders Markdown, so emphasis belongs in its
/// output as authored. This is the reverse direction of the gate above, and it
/// is what keeps a future "convert everywhere" from regressing the one backend
/// that is already right.
#[test]
fn rust_keeps_its_markdown() {
    let (func, enums) = load("cmou");
    let registry = Registry::from_dir(&input_dir());
    // `from_dir` takes the input root, not the helper subdirectory, and says so
    // loudly when given the wrong base.
    let helpers = HelperRegistry::from_dir(&input_dir());
    let src = backends::rust_lang::generate(&func, &enums, &registry, &helpers);
    assert!(
        src.contains("**plain moving-window sums**"),
        "CMOU's Rust doc must keep the authored Markdown"
    );
}
