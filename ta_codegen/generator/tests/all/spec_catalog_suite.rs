//! The flag catalog of the spec page `/spec/abstract/`, read off disk and held
//! to the C header, the three generated ports and the corpus.
//!
//! The page is hand-written and claims to list every flag. Nothing else reads
//! it, so a flag added to the header, renamed in a port, or first set by a
//! function would leave it wrong with every other gate green.

use std::collections::{BTreeMap, BTreeSet};
use std::path::{Path, PathBuf};

use crate::common::all_abstract_rows;

const PAGE: &str = "website/src/spec/abstract/README.md";
const UNUSED: &str = "No function sets it";

fn repo_root() -> PathBuf {
    Path::new(env!("CARGO_MANIFEST_DIR")).join("../..")
}

fn read(rel: &str) -> String {
    std::fs::read_to_string(repo_root().join(rel)).unwrap_or_else(|e| panic!("cannot read {rel}: {e}"))
}

/// `0x0100_0000,` / `0x01000000;` / `0x01000000` to its value.
fn hex(token: &str) -> Option<u32> {
    let bare = token.trim().trim_end_matches([',', ';']).replace('_', "");
    u32::from_str_radix(bare.strip_prefix("0x")?, 16).ok()
}

/// `NAME = 0x...` members between `open` and the first `close` after it.
fn members(text: &str, open: &str, close: &str, what: &str) -> BTreeMap<String, u32> {
    let at = text.find(open).unwrap_or_else(|| panic!("{what}: no `{open}`"));
    let body = &text[at + open.len()..];
    let body = &body[..body.find(close).unwrap_or_else(|| panic!("{what}: no `{close}` after `{open}`"))];
    let mut out = BTreeMap::new();
    for line in body.lines().map(str::trim).filter(|l| !l.starts_with(['/', '*'])) {
        let Some((name, value)) = line.split_once(" = ") else { continue };
        let name = name.rsplit(' ').next().unwrap();
        // C#'s empty word, which the page states in prose rather than in a row.
        if name == "None" && value.trim_end_matches(',') == "0" {
            continue;
        }
        let v = hex(value).unwrap_or_else(|| panic!("{what}: `{line}` is not `NAME = 0x...`"));
        out.insert(name.to_string(), v);
    }
    assert!(!out.is_empty(), "{what}: no member parsed");
    out
}

struct Family {
    c_prefix: &'static str,
    rust: &'static str,
    java: &'static str,
    csharp: &'static str,
    used: u32,
}

#[test]
fn the_spec_flag_catalog_is_complete() {
    let rows = all_abstract_rows();
    let or = |f: &dyn Fn(&ta_codegen_lib::backends::abstract_rows::FuncRow) -> u32| rows.iter().fold(0, |a, r| a | f(r));
    let families = [
        Family {
            c_prefix: "TA_FUNC_FLG_",
            rust: "FuncFlags",
            java: "FuncFlags",
            csharp: "FuncFlags",
            used: or(&|r| r.flags),
        },
        Family {
            c_prefix: "TA_IN_PRICE_",
            rust: "InputFlags",
            java: "InputFlags",
            csharp: "PriceComponents",
            used: or(&|r| r.inputs.iter().fold(0, |a, i| a | i.flags)),
        },
        Family {
            c_prefix: "TA_OPTIN_",
            rust: "OptInputFlags",
            java: "OptInputFlags",
            csharp: "OptInputFlags",
            used: or(&|r| r.opt_inputs.iter().fold(0, |a, o| a | o.flags)),
        },
        Family {
            c_prefix: "TA_OUT_",
            rust: "OutputFlags",
            java: "OutputFlags",
            csharp: "OutputFlags",
            used: or(&|r| r.outputs.iter().fold(0, |a, o| a | o.flags)),
        },
    ];

    let header = read("include/ta_abstract.h");
    let rust_src = read("ta_codegen/output/rust/library/src/abstract_api.rs");
    let csharp_src = read("ta_codegen/output/csharp/library/src/metadata/Vocabulary.g.cs");
    let page = read(PAGE);

    // The catalog rows: `| `TA_X` | rust | java | c# | meaning |`.
    let mut catalog: BTreeMap<String, Vec<String>> = BTreeMap::new();
    for line in page.lines().filter(|l| l.starts_with("| `TA_")) {
        let cells: Vec<String> = line.trim().trim_matches('|').split(" | ").map(|c| c.trim().to_string()).collect();
        assert_eq!(cells.len(), 5, "{PAGE}: a catalog row needs five cells: {line}");
        let name = cells[0].trim_matches('`').to_string();
        assert!(catalog.insert(name.clone(), cells).is_none(), "{PAGE}: `{name}` has two rows");
    }

    // Four families and no fifth, or "every flag" stops being true unseen.
    for line in header.lines() {
        let mut it = line.split_whitespace();
        if let (Some("#define"), Some(name), Some(_value)) = (it.next(), it.next(), it.next()) {
            assert!(
                families.iter().any(|f| name.starts_with(f.c_prefix)),
                "include/ta_abstract.h: `{name}` belongs to no flag family this test reads"
            );
        }
    }
    let java_dir = repo_root().join("ta_codegen/output/java/library/src/main/java/io/github/talib/metadata");
    let java_flag_files = std::fs::read_dir(&java_dir)
        .unwrap_or_else(|e| panic!("{}: {e}", java_dir.display()))
        .filter(|e| e.as_ref().unwrap().file_name().to_string_lossy().ends_with("Flags.java"))
        .count();
    assert_eq!(rust_src.matches("\nflag_newtype!(").count(), families.len(), "a Rust flag family this test does not read");
    assert_eq!(csharp_src.matches("[Flags]").count(), families.len(), "a C# flag family this test does not read");
    assert_eq!(java_flag_files, families.len(), "a Java flag family this test does not read");

    let (mut checked, mut marked) = (0, 0);
    for fam in &families {
        let mut c: BTreeMap<String, u32> = BTreeMap::new();
        for line in header.lines() {
            let mut it = line.split_whitespace();
            if it.next() != Some("#define") {
                continue;
            }
            let (Some(name), Some(value)) = (it.next(), it.next()) else { continue };
            if name.starts_with(fam.c_prefix) && !value.starts_with(fam.c_prefix) {
                c.insert(name.to_string(), hex(value).unwrap_or_else(|| panic!("{name}: not a hex literal")));
            }
        }
        assert!(!c.is_empty(), "no {}* define in the header", fam.c_prefix);

        let listed: BTreeSet<&String> = catalog.keys().filter(|k| k.starts_with(fam.c_prefix)).collect();
        assert_eq!(
            listed,
            c.keys().collect::<BTreeSet<_>>(),
            "{PAGE}: the {}* rows are not the header's {}* defines",
            fam.c_prefix,
            fam.c_prefix
        );

        let ports: [(&str, &str, &str, BTreeMap<String, u32>); 3] = [
            (
                "Rust",
                fam.rust,
                "::",
                members(&rust_src, &format!("\n    {} {{\n", fam.rust), "});", fam.rust),
            ),
            (
                "Java",
                fam.java,
                ".",
                members(
                    &read(&format!(
                        "ta_codegen/output/java/library/src/main/java/io/github/talib/metadata/{}.java",
                        fam.java
                    )),
                    "public final class",
                    "\n}",
                    fam.java,
                ),
            ),
            (
                "C#",
                fam.csharp,
                ".",
                members(&csharp_src, &format!("public enum {} : uint", fam.csharp), "\n}", fam.csharp),
            ),
        ];

        for (cell, (lang, ty, sep, port)) in ports.iter().enumerate() {
            let mut named = BTreeSet::new();
            for (c_name, c_value) in &c {
                let spelling = catalog[c_name][cell + 1].as_str();
                if spelling == "none" {
                    assert!(
                        !port.values().any(|v| v == c_value),
                        "{PAGE}: `{c_name}` says {lang} has none, but {ty} has a member with its value"
                    );
                    continue;
                }
                let member = spelling
                    .trim_matches('`')
                    .strip_prefix(&format!("{ty}{sep}"))
                    .unwrap_or_else(|| panic!("{PAGE}: `{c_name}`: the {lang} cell `{spelling}` is not a {ty} member"));
                let value = port
                    .get(member)
                    .unwrap_or_else(|| panic!("{PAGE}: `{c_name}`: {lang} has no {ty}{sep}{member}"));
                assert_eq!(value, c_value, "{lang} {ty}{sep}{member} does not carry the value of {c_name}");
                named.insert(member.to_string());
            }
            let unlisted: Vec<&String> = port.keys().filter(|m| !named.contains(*m)).collect();
            assert!(unlisted.is_empty(), "{PAGE}: {lang} {ty} members without a catalog row: {unlisted:?}");
        }

        for (c_name, c_value) in &c {
            let unused = fam.used & c_value == 0;
            assert_eq!(
                catalog[c_name][4].contains(UNUSED),
                unused,
                "{PAGE}: `{c_name}` must say \"{UNUSED}\" exactly when no function carries it"
            );
            marked += usize::from(unused);
            checked += 1;
        }
    }
    assert_eq!(checked, catalog.len(), "{PAGE}: a catalog row belongs to no flag family");
    assert!(checked >= 30 && marked > 0 && marked < checked, "checked {checked} flags, {marked} of them unused");
}
