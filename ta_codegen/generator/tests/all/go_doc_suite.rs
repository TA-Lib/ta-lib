//! The Go doc comments against the declarations they sit on. `go vet` reads no
//! doc text, so a parameter documented under a name the signature no longer has
//! reaches pkg.go.dev with every gate green.

use std::collections::BTreeSet;

use crate::common;
use ta_codegen_lib::backends;

/// The `// ` comment block directly above the line starting with `decl`, and
/// that line.
fn doc_above<'a>(src: &'a str, decl: &str) -> Option<(Vec<&'a str>, &'a str)> {
    let lines: Vec<&str> = src.lines().collect();
    let at = lines.iter().position(|l| l.starts_with(decl))?;
    let mut doc: Vec<&str> =
        lines[..at].iter().rev().take_while(|l| l.starts_with("//")).map(|l| l.trim_start_matches("//")).collect();
    doc.reverse();
    Some((doc, lines[at]))
}

/// The parameter names of a one-line Go signature: `(a, b int, c []float64)`.
fn params(sig: &str) -> Vec<String> {
    let open = sig.find(") ").map_or(0, |i| i + 2);
    let rest = &sig[open..];
    let args = &rest[rest.find('(').expect("an argument list") + 1..rest.find(')').expect("a closing paren")];
    let mut names = Vec::new();
    for group in args.split(", ") {
        let name = group.split_whitespace().next().unwrap_or_default();
        if !name.is_empty() {
            names.push(name.to_string());
        }
    }
    names
}

/// Identifiers shaped like a TA-Lib argument (`inReal`, `optInTimePeriod`,
/// `outMACD`) in the doc text.
fn argument_words(doc: &[&str]) -> BTreeSet<String> {
    let re = regex::Regex::new(r"\b(in|optIn|out)[A-Z][A-Za-z0-9_]*\b").unwrap();
    doc.iter().flat_map(|l| re.find_iter(l).map(|m| m.as_str().to_string()).collect::<Vec<_>>()).collect()
}

/// An indicator with its canonical `.md`, which `common::load_indicator` leaves out.
fn load(name: &str) -> (ta_codegen_lib::ir::FuncDef, std::collections::HashMap<String, ta_codegen_lib::ir::EnumDef>) {
    let (mut func, enums) = common::load_indicator(name);
    let md = std::path::Path::new(env!("CARGO_MANIFEST_DIR")).join(format!("../input/{name}/{name}.md"));
    func.doc = ta_codegen_lib::parser::doc_md::parse_doc_md(&md);
    (func, enums)
}

struct Corpus {
    funcs: Vec<(ta_codegen_lib::ir::FuncDef, String)>,
}

fn corpus() -> Corpus {
    let registry = common::make_registry();
    let helpers = common::make_helpers();
    let funcs = common::discover_indicators()
        .into_iter()
        .map(|name| {
            let (func, enums) = load(&name);
            let src = backends::go::generate(&func, &enums, registry, helpers);
            (func, src)
        })
        .collect();
    Corpus { funcs }
}

/// The batch method lists every parameter of its signature, in order, and
/// nothing else.
#[test]
fn go_public_entry_documents_exactly_its_parameters() {
    let mut checked = 0;
    for (func, src) in corpus().funcs {
        let (doc, sig) = doc_above(&src, &format!("func (c *Core) {}(", func.name))
            .unwrap_or_else(|| panic!("{}: no batch method", func.name));
        let documented: Vec<String> = doc
            .iter()
            .filter_map(|l| l.strip_prefix("   - "))
            .filter_map(|l| l.split_once(':').map(|(n, _)| n.to_string()))
            .collect();
        let mut expected = params(sig);
        expected.retain(|p| p != "startIdx" && p != "endIdx");
        assert_eq!(documented, expected, "{}: the doc's lists disagree with the signature\n{sig}", func.name);
        assert!(!expected.is_empty(), "{}: parsed no parameters", func.name);
        checked += 1;
    }
    assert!(checked > 200, "only {checked} functions");
}

/// Every other generated doc names only arguments its declaration has, or the
/// function's outputs, which the stream methods return.
#[test]
fn go_docs_name_only_arguments_that_exist() {
    let mut checked = 0;
    for (func, src) in corpus().funcs {
        let outputs: BTreeSet<String> = func.outputs.iter().map(|o| o.name.clone()).collect();
        let name = &func.name;
        let mut decls = vec![format!("func (c *Core) {name}Lookback(")];
        if src.contains(&format!("type {name}Stream struct")) {
            decls.push(format!("func (c *Core) {name}Open("));
            decls.push(format!("func (c *Core) {name}OpenAndFill("));
            for m in ["OutRange", "Advance", "Update", "Peek", "Value", "Clone"] {
                decls.push(format!("func (sp *{name}Stream) {m}("));
            }
        }
        for decl in &decls {
            let (doc, sig) = doc_above(&src, decl).unwrap_or_else(|| panic!("{name}: no `{decl}`"));
            assert!(!doc.is_empty(), "{name}: `{decl}` has no doc comment");
            let method = decl.trim_end_matches('(').rsplit([' ', ')']).next().unwrap();
            assert!(doc[0].starts_with(&format!(" {method} ")), "{name}: `{decl}` doc does not open with its name");
            let known: BTreeSet<String> = params(sig).into_iter().chain(outputs.iter().cloned()).collect();
            let stray: Vec<String> = argument_words(&doc).difference(&known).cloned().collect();
            assert!(stray.is_empty(), "{name}: `{decl}` documents {stray:?}, which it does not take");
            checked += 1;
        }
    }
    assert!(checked > 2000, "only {checked} declarations");
}

/// A Go doc comment is plain text: Markdown reaches the reader as punctuation.
#[test]
fn no_markdown_reaches_the_go_docs() {
    let mut checked = 0;
    for (func, src) in corpus().funcs {
        for line in src.lines().filter(|l| l.starts_with("// ") && !l.starts_with("// Code generated")) {
            for mark in ["`", "**", "]("] {
                assert!(!line.contains(mark), "{}: `{mark}` in a doc line: {line}", func.name);
            }
            checked += 1;
        }
    }
    assert!(checked > 5000, "only {checked} doc lines");
}

/// A link to another function's page becomes a doc link to its method, and the
/// C spellings in the prose take their Go names.
#[test]
fn prose_links_and_c_names_take_their_go_form() {
    let registry = common::make_registry();
    let helpers = common::make_helpers();
    let (ac, enums) = load("ac");
    let src = backends::go::generate(&ac, &enums, registry, helpers);
    let flat = src.replace("\n// ", " ");
    assert!(flat.contains("Awesome Oscillator ([Core.AO])"), "AC's link to AO must become a doc link");
    let (bp, enums) = load("swak_bp");
    let doc = backends::go_doc::batch_docs(&bp, &enums, registry).replace("\n// ", " ");
    assert!(doc.contains("passes as Core.MEDPRICE output"), "SWAK_BP's TA_MEDPRICE must read Core.MEDPRICE:\n{doc}");
}

/// The sweeps above stand on the doc block reader; a reader that found nothing
/// would pass them all.
#[test]
fn the_doc_reader_sees_the_sma_contract() {
    let registry = common::make_registry();
    let helpers = common::make_helpers();
    let (sma, enums) = load("sma");
    let src = backends::go::generate(&sma, &enums, registry, helpers);
    let (doc, sig) = doc_above(&src, "func (c *Core) SMA(").unwrap();
    assert_eq!(params(sig), ["startIdx", "endIdx", "inReal", "optInTimePeriod", "outReal"]);
    let text = doc.join("\n");
    for needle in [
        "https://ta-lib.org/functions/sma.",
        "   - optInTimePeriod:",
        "BegIdx+k",
        "[BadParam]",
        "[Core.SMALookback]",
    ] {
        assert!(text.contains(needle), "SMA's doc lacks `{needle}`:\n{text}");
    }
    assert_eq!(argument_words(&doc).len(), 3, "{:?}", argument_words(&doc));
}
