//! The C symbol is the search key a migrating caller types, so every generated
//! Rust item that has a C counterpart must carry it as a `#[doc(alias)]`.
//!
//! Nothing gated this surface before #179 D8: the stream emitter wrote seven of
//! the spellings and the batch tier wrote none, and no test could tell the
//! difference. The sweep below is an EXACT-SET check per function rather than a
//! floor, because both failure directions are real — a missing alias is a symbol
//! docs.rs search cannot answer, and a stray one points the reader at an item
//! that does not implement it.

use std::collections::{BTreeSet, HashMap};
use std::path::PathBuf;
use ta_codegen_lib::helper_registry::HelperRegistry;
use ta_codegen_lib::registry::Registry;
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

/// Every indicator directory: a `<name>.yaml` next to a `<name>.c`.
fn indicators() -> Vec<String> {
    let mut v: Vec<String> = std::fs::read_dir(input_dir())
        .expect("input dir")
        .filter_map(Result::ok)
        .filter(|e| e.path().is_dir())
        .filter_map(|e| {
            let name = e.file_name().to_string_lossy().to_string();
            let dir = e.path();
            (dir.join(format!("{name}.yaml")).exists() && dir.join(format!("{name}.c")).exists())
                .then_some(name)
        })
        .collect();
    v.sort();
    v
}

fn rust_source(name: &str) -> (ir::FuncDef, String) {
    let (func, enums) = load_indicator(name);
    let registry = Registry::from_dir(&input_dir());
    let helpers = HelperRegistry::empty();
    let src = backends::rust_lang::generate(&func, &enums, &registry, &helpers);
    (func, src)
}

/// Every `#[doc(alias = "TA_...")]` value in `src`, with its multiplicity, so a
/// repeat is visible rather than folded away by the set. rustdoc emits no
/// diagnostic for a repeated alias on one item (checked on 1.97), so this is the
/// only thing that would report one.
fn c_symbol_aliases(src: &str) -> Vec<String> {
    let mut out = Vec::new();
    for line in src.lines() {
        let t = line.trim();
        if let Some(rest) = t.strip_prefix("#[doc(alias = \"") {
            if let Some(v) = rest.strip_suffix("\")]") {
                if v.starts_with("TA_") {
                    out.push(v.to_string());
                }
            }
        }
    }
    out
}

/// The attribute lines attached to the item whose signature line starts with
/// `sig` — the contiguous run of `///` and `#[...]` lines directly above it.
/// Adjacency is the point: an alias anywhere in the file satisfies a `contains`
/// check even when it landed on the wrong item.
fn attrs_above(src: &str, sig: &str) -> Vec<String> {
    let lines: Vec<&str> = src.lines().collect();
    let i = lines
        .iter()
        .position(|l| l.trim_start().starts_with(sig))
        .unwrap_or_else(|| panic!("no signature line starting with {sig:?}"));
    let mut attrs = Vec::new();
    for l in lines[..i].iter().rev() {
        let t = l.trim();
        if t.starts_with("#[") {
            attrs.push(t.to_string());
        } else if !t.starts_with("///") {
            break;
        }
    }
    attrs
}

/// Per function: the set of C symbols its generated Rust must name, and nothing
/// else.
fn expected(func: &ir::FuncDef) -> BTreeSet<String> {
    let n = &func.name;
    let mut want: BTreeSet<String> = [format!("TA_{n}"), format!("TA_{n}_Lookback")].into();
    if func.streaming {
        for verb in ["Stream", "Open", "OpenAndFill", "Update", "Peek", "Value", "OutRange", "Advance"] {
            want.insert(format!("TA_{n}_{verb}"));
        }
    }
    want
}

#[test]
fn every_rust_function_names_exactly_its_own_c_symbols() {
    let names = indicators();
    assert!(names.len() >= 200, "expected the whole input tree, got {}", names.len());

    let (mut streamed, mut total) = (0usize, 0usize);
    for name in &names {
        let (func, src) = rust_source(name);
        let aliases = c_symbol_aliases(&src);
        total += aliases.len();

        let seen: BTreeSet<String> = aliases.iter().cloned().collect();
        assert_eq!(
            seen.len(),
            aliases.len(),
            "{name}: a C-symbol alias is emitted twice: {aliases:?}"
        );
        assert_eq!(seen, expected(&func), "{name}: C-symbol aliases are not the expected set");

        if func.streaming {
            streamed += 1;
        }
    }

    // Floors, so an emitter that stopped writing aliases altogether cannot make
    // this pass by making every expected set empty.
    assert!(streamed >= 200, "expected a streaming tier on nearly every function, got {streamed}");
    assert!(total > 1700, "too few C-symbol aliases across the corpus: {total}");
}

#[test]
fn the_batch_and_lookback_aliases_sit_on_the_functions_they_name() {
    for name in &indicators() {
        let (func, src) = rust_source(name);
        let n = &func.name;

        let batch = attrs_above(&src, &format!("pub fn {n}("));
        assert!(
            batch.contains(&format!("#[doc(alias = \"TA_{n}\")]")),
            "{name}: the batch entry point does not carry TA_{n}; attrs were {batch:?}"
        );

        let lb = attrs_above(&src, &format!("pub fn {n}_Lookback("));
        assert!(
            lb.contains(&format!("#[doc(alias = \"TA_{n}_Lookback\")]")),
            "{name}: the lookback does not carry TA_{n}_Lookback; attrs were {lb:?}"
        );

        // The two range accessors, per handle since #387. Set membership above
        // cannot see which method an alias sits on.
        if func.streaming {
            let or = attrs_above(&src, "pub fn out_range(");
            assert!(
                or.contains(&format!("#[doc(alias = \"TA_{n}_OutRange\")]")),
                "{name}: out_range does not carry TA_{n}_OutRange; attrs were {or:?}"
            );

            let adv = attrs_above(&src, "pub fn advance(");
            assert!(
                adv.contains(&format!("#[doc(alias = \"TA_{n}_Advance\")]")),
                "{name}: advance does not carry TA_{n}_Advance; attrs were {adv:?}"
            );
        }
    }
}
