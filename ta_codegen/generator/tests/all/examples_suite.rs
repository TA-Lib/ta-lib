//! The `## Example` section of every function page (#585): five tabs, each the shown
//! half of a sample whose full copy a language's suite compiles and runs. These pin
//! the page text to those compiled copies over the whole corpus, so the page can only
//! show what ran.

use std::collections::HashMap;

use crate::common;
use ta_codegen_lib::backends::{self, docs_site};
use ta_codegen_lib::ir;

fn corpus() -> (Vec<ir::FuncDef>, HashMap<String, ir::EnumDef>) {
    let mut funcs = Vec::new();
    let mut enums = HashMap::new();
    for name in common::discover_indicators() {
        let (f, e) = common::load_indicator(&name);
        funcs.push(f);
        enums = e;
    }
    (funcs, enums)
}

fn tab<'a>(tabs: &'a [(&str, &str, Vec<String>)], title: &str) -> &'a [String] {
    &tabs.iter().find(|(t, _, _)| *t == title).unwrap_or_else(|| panic!("no {title} tab")).2
}

/// The lines of the first fenced block under `# Examples` in a generated module's
/// rustdoc, with the `# ` lines rustdoc hides removed.
fn doctest_shown_lines(module: &str) -> Vec<String> {
    let docs: Vec<&str> = module.lines().map(str::trim_start).filter_map(|l| l.strip_prefix("///")).collect();
    let at = docs.iter().position(|l| l.trim() == "# Examples").expect("a # Examples heading");
    let open = at + docs[at..].iter().position(|l| l.trim() == "```").expect("an opening fence");
    docs[open + 1..]
        .iter()
        .take_while(|l| l.trim() != "```")
        .map(|l| l.strip_prefix(' ').unwrap_or(l))
        .filter(|l| !l.starts_with("# "))
        .map(String::from)
        .collect()
}

/// Every `needles` line occurs in `hay` after `from`, in order, each at a line start
/// after indentation. Returns the position after the last one.
fn occur_in_order(hay: &str, from: usize, needles: &[String], what: &str) -> usize {
    let mut pos = from;
    for needle in needles.iter().filter(|n| !n.is_empty()) {
        let mut search = pos;
        loop {
            let Some(rel) = hay[search..].find(needle.as_str()) else {
                panic!("{what}: `{needle}` not found in order");
            };
            let at = search + rel;
            let line_start = hay[..at].rfind('\n').map_or(0, |i| i + 1);
            if hay[line_start..at].chars().all(|c| c == ' ') {
                pos = at + needle.len();
                break;
            }
            search = at + 1;
        }
    }
    pos
}

#[test]
fn the_rust_tab_is_the_doctests_shown_lines() {
    let (funcs, enums) = corpus();
    let registry = common::make_registry();
    let helpers = common::make_helpers();
    let mut checked = 0;
    for func in &funcs {
        let tabs = docs_site::example_tabs(func, &enums).unwrap_or_else(|| panic!("{}: no example", func.name));
        let module = backends::rust_lang::generate(func, &enums, registry, helpers);
        assert_eq!(doctest_shown_lines(&module), tab(&tabs, "Rust"), "{}: Rust tab vs doctest", func.name);
        checked += 1;
    }
    assert!(checked > 200, "only {checked} functions");
}

#[test]
fn the_typescript_tab_is_the_batch_example_and_the_test_runs_it() {
    let (funcs, enums) = corpus();
    let files = backends::ts_doc::render_example_tests(&funcs, &enums);
    let all: String = files.values().cloned().collect();
    let mut checked = 0;
    for func in &funcs {
        let tabs = docs_site::example_tabs(func, &enums).unwrap();
        let example = backends::ts_doc::batch_example(func, &enums).unwrap();
        assert_eq!(example.source(), tab(&tabs, "TypeScript"), "{}: TypeScript tab", func.name);
        let title = format!("test('{}: {} example'", func.name, backends::ts::ts_names(&func.name).public);
        let at = all.find(&title).unwrap_or_else(|| panic!("{title} missing"));
        occur_in_order(&all, at, &example.body_text(), &func.name);
        assert!(example.body.iter().any(|l| !l.shown), "{}: no hidden fill", func.name);
        checked += 1;
    }
    assert!(checked > 200, "only {checked} functions");
}

#[test]
fn the_java_csharp_and_c_tabs_are_compiled_verbatim_by_their_suites() {
    let (funcs, enums) = corpus();
    let java = backends::java_examples::render_test(&funcs, &enums);
    let csharp = backends::csharp_examples::render_test(&funcs, &enums);
    let c = backends::c_examples::render_program(&funcs, &enums);
    let mut checked = 0;
    for func in &funcs {
        let tabs = docs_site::example_tabs(func, &enums).unwrap();
        let name = &func.name;
        for (lang, text, head, snippet) in [
            (
                "Java",
                &java,
                format!("static void {}() {{", backends::common::camel_words(name)),
                backends::java_examples::example(func, &enums).unwrap(),
            ),
            (
                "C\\#",
                &csharp,
                format!("private static void {}()", backends::common::pascal_words(name)),
                backends::csharp_examples::example(func, &enums).unwrap(),
            ),
            ("C", &c, format!("static int example_{name}( void )"), backends::c_examples::example(func, &enums).unwrap()),
        ] {
            assert_eq!(snippet.shown(), tab(&tabs, lang), "{name}: {lang} tab");
            for import in &snippet.imports {
                assert!(text.contains(import), "{name}: {lang} suite lacks `{import}`");
            }
            let at = text.find(&head).unwrap_or_else(|| panic!("{name}: no {lang} method `{head}`"));
            // The body as compiled: the shown lines in order, with the hidden ones between.
            occur_in_order(text, at, &snippet.body_text(), &format!("{name} {lang}"));
            assert!(snippet.body.iter().any(|l| !l.shown), "{name}: {lang} sample has no hidden check");
        }
        checked += 1;
    }
    assert!(checked > 200, "only {checked} functions");
}

/// One shape in every language: the arrays named after the inputs, every parameter at
/// its default, the last value printed.
#[test]
fn every_tab_names_the_inputs_and_prints_the_last_value() {
    let (funcs, enums) = corpus();
    let mut cdl = 0;
    for func in &funcs {
        let tabs = docs_site::example_tabs(func, &enums).unwrap();
        assert_eq!(
            tabs.iter().map(|(t, f, _)| (*t, *f)).collect::<Vec<_>>(),
            [("C", "c"), ("Rust", "rust"), ("Java", "java"), ("C\\#", "csharp"), ("TypeScript", "ts")]
        );
        let vars: Vec<&str> = func
            .inputs
            .iter()
            .map(|i| match i.name.as_str() {
                "inOpen" => "open",
                "inHigh" => "high",
                "inLow" => "low",
                "inClose" => "close",
                "inVolume" => "volume",
                "inPeriods" => "periods",
                "inReal" => "series",
                "inReal0" => "series0",
                "inReal1" => "series1",
                other => panic!("{}: input {other} has no example variable", func.name),
            })
            .collect();
        if func.is_candlestick() {
            assert_eq!(vars, ["open", "high", "low", "close"], "{}", func.name);
            cdl += 1;
        }
        for (title, _, lines) in &tabs {
            let text = lines.join("\n");
            for var in &vars {
                let decl = lines.iter().filter(|l| l.contains(&format!(" {var} ")) || l.contains(&format!(" {var}[252]"))).count();
                assert!(decl >= 1, "{} {title}: no declaration of `{var}`:\n{text}", func.name);
            }
            assert!(text.contains("252"), "{} {title}: no 252-bar array", func.name);
            assert!(text.contains("values from bar"), "{} {title}: nothing printed", func.name);
            assert_eq!(text.matches("values from bar").count(), 1, "{} {title}: one printed line", func.name);
        }
        // Defaults, not hard-coded numbers: the first optional parameter's default is in the call.
        if let Some(opt) = func.optional_inputs.first() {
            let default = opt.default.unwrap_or(0.0);
            if opt.param_type == ir::ParamType::Integer {
                let lit = format!("{}", default as i64);
                for (title, _, lines) in &tabs {
                    assert!(lines.iter().any(|l| l.contains(&lit)), "{} {title}: default {lit} missing", func.name);
                }
            }
        }
    }
    assert!(cdl > 50, "only {cdl} candlestick functions");
}

/// Enum parameters print the named constant, spelled as each language spells it.
#[test]
fn enum_parameters_are_named_in_every_language() {
    let (func, enums) = common::load_indicator("bbands");
    let tabs = docs_site::example_tabs(&func, &enums).unwrap();
    for (title, needle) in [
        ("C", "TA_MAType_SMA"),
        ("Rust", "MAType::SMA"),
        ("Java", "MAType.SMA"),
        ("C\\#", "MAType.SMA"),
        ("TypeScript", "maType: MAType.SMA"),
    ] {
        assert!(tab(&tabs, title).iter().any(|l| l.contains(needle)), "{title}: no `{needle}`");
    }
    assert!(tab(&tabs, "Rust").iter().any(|l| l == "use ta_lib::{Core, MAType};"));
    assert!(tab(&tabs, "Java").contains(&"import io.github.talib.MAType;".to_string()));
}

/// The section sits between Properties and Implementation, and an authored one is refused.
#[test]
fn the_page_carries_the_section_once_and_refuses_an_authored_one() {
    let pages = crate::added_in_suite::render(&["BBANDS"]);
    let page = &pages["BBANDS"];
    let headings: Vec<&str> = page.lines().filter(|l| l.starts_with("## ")).collect();
    let at = headings.iter().position(|h| *h == "## Example").expect("an Example section");
    assert_eq!(headings[at - 1], "## Properties", "{headings:?}");
    assert_eq!(headings[at + 1], "## Implementation", "{headings:?}");
    assert_eq!(page.matches("::: code-tabs#lang").count(), 1);
    assert_eq!(page.matches("\n@tab ").count(), 5);

    let real = crate::added_in_suite::repo_root();
    let root = std::env::temp_dir().join(format!("ta_example_authored_{}", std::process::id()));
    let _ = std::fs::remove_dir_all(&root);
    std::fs::create_dir_all(root.join("ta_codegen/input/sma")).unwrap();
    std::fs::copy(real.join("VERSION"), root.join("VERSION")).unwrap();
    std::fs::copy(real.join("ta_codegen/input/releases.yaml"), root.join("ta_codegen/input/releases.yaml")).unwrap();
    let mut md = std::fs::read_to_string(real.join("ta_codegen/input/sma/sma.md")).unwrap();
    md.push_str("\n## Example\n\nhand-written\n");
    std::fs::write(root.join("ta_codegen/input/sma/sma.md"), md).unwrap();
    let (sma, _) = common::load_indicator("sma");
    let errors = docs_site::validate_docs(&[sma], &root).unwrap_err();
    let _ = std::fs::remove_dir_all(&root);
    assert!(errors.iter().any(|e| e.contains("`## Example` is generated")), "{errors:?}");
}
