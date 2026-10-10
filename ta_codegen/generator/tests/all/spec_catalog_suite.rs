//! The flag catalog of the spec page `/spec/abstract/`, read off disk and held
//! to the C header, the three generated ports and the corpus, with the flag
//! types `/spec/` names.
//!
//! The page is hand-written and claims to list every flag. Nothing else reads
//! it, so a flag added to the header, dropped by a port, or first set by a
//! function would leave it wrong with every other gate green.

use std::collections::{BTreeMap, BTreeSet};
use std::path::{Path, PathBuf};

use crate::common::all_abstract_rows;

const PAGE: &str = "website/src/spec/abstract/README.md";
const NAMES_PAGE: &str = "website/src/spec/README.md";
const NAMES_HEADING: &str = "### Metadata flags {#flag-names}";
const UNUSED: &str = "No function sets it";
const ABSENT: &str = "Java and C# have no member for it";

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
    let (mut out, mut aliases) = (BTreeMap::new(), Vec::new());
    for line in body.lines().map(str::trim).filter(|l| !l.starts_with(['/', '*', '@'])) {
        let Some((name, value)) = line.split_once(" = ") else { continue };
        let name = name.rsplit(' ').next().unwrap();
        // C#'s empty word, which the page states in prose rather than in a row.
        if name == "None" && value.trim_end_matches(',') == "0" {
            continue;
        }
        let target = value.trim_end_matches([';', ',']);
        if target.chars().all(|c| c.is_ascii_uppercase() || c == '_') {
            aliases.push((name, target));
            continue;
        }
        let v = hex(value).unwrap_or_else(|| panic!("{what}: `{line}` is not `NAME = 0x...`"));
        out.insert(name.to_string(), v);
    }
    assert!(!out.is_empty(), "{what}: no member parsed");
    for (name, target) in aliases {
        assert!(out.contains_key(target), "{what}: `{name}` names `{target}`, which is not a member");
    }
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

    // `| `TA_X` | meaning |` on the catalog page; `| owner | `TA_X_` | rust, java | c# |`
    // under the names page's flag heading.
    let cells_of = |line: &str| -> Vec<String> {
        line.trim().trim_matches('|').split(" | ").map(|c| c.trim().to_string()).collect()
    };
    let mut catalog: BTreeMap<String, String> = BTreeMap::new();
    for line in page.lines().filter(|l| l.starts_with("| `TA_")) {
        let cells = cells_of(line);
        assert_eq!(cells.len(), 2, "{PAGE}: a catalog row is a flag and its meaning: {line}");
        let name = cells[0].trim_matches('`').to_string();
        assert!(catalog.insert(name.clone(), cells[1].clone()).is_none(), "{PAGE}: `{name}` has two rows");
    }
    let names_page = read(NAMES_PAGE);
    let section = names_page
        .split_once(NAMES_HEADING)
        .unwrap_or_else(|| panic!("{NAMES_PAGE}: no `{NAMES_HEADING}`"))
        .1;
    let section = &section[..section.find("\n#").unwrap_or(section.len())];
    let mut types: Vec<Vec<String>> = section
        .lines()
        .map(cells_of)
        .filter(|cells| cells.len() == 4 && cells[1].starts_with("`TA_"))
        .map(|cells| cells[1..].iter().map(|c| c.trim_matches('`').to_string()).collect())
        .collect();
    types.sort();
    let mut defined: Vec<Vec<String>> = families
        .iter()
        .inspect(|f| assert_eq!(f.rust, f.java, "{NAMES_PAGE} gives Rust and Java one column"))
        .map(|f| vec![f.c_prefix.to_string(), f.rust.to_string(), f.csharp.to_string()])
        .collect();
    defined.sort();
    assert_eq!(types, defined, "{NAMES_PAGE}: the flag type rows are not the families the ports define");

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

    let (mut checked, mut marked, mut absent) = (0, 0, 0);
    for fam in &families {
        let mut c: BTreeMap<String, u32> = BTreeMap::new();
        for line in header.lines() {
            let mut it = line.split_whitespace();
            if it.next() != Some("#define") {
                continue;
            }
            let (Some(name), Some(value)) = (it.next(), it.next()) else { continue };
            let alias = value.starts_with(fam.c_prefix) && it.next().is_none_or(|t| t.starts_with("/*"));
            if name.starts_with(fam.c_prefix) && !alias {
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

        let ports: [(&str, &str, BTreeMap<String, u32>); 3] = [
            (
                "Rust",
                fam.rust,
                members(&rust_src, &format!("\n    {} {{\n", fam.rust), "});", fam.rust),
            ),
            (
                "Java",
                fam.java,
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
                members(&csharp_src, &format!("public enum {} : uint", fam.csharp), "\n}", fam.csharp),
            ),
        ];

        // A port's members are C's flags by value, one member per flag; only
        // Java and C# may lack one, together, and the catalog row says so.
        let mut lacking: Vec<BTreeSet<&String>> = Vec::new();
        for (lang, ty, port) in &ports {
            let values: BTreeSet<u32> = port.values().copied().collect();
            assert_eq!(values.len(), port.len(), "{lang} {ty}: two members carry one value");
            let stray: Vec<&String> = port.iter().filter(|(_, v)| !c.values().any(|d| d == *v)).map(|(m, _)| m).collect();
            assert!(stray.is_empty(), "{lang} {ty} members that carry no {}* value: {stray:?}", fam.c_prefix);
            lacking.push(c.iter().filter(|(_, v)| !values.contains(v)).map(|(n, _)| n).collect());
        }
        assert!(lacking[0].is_empty(), "Rust {} lacks {:?}", fam.rust, lacking[0]);
        assert_eq!(lacking[1], lacking[2], "Java and C# lack different {}* flags", fam.c_prefix);
        for c_name in c.keys() {
            assert_eq!(
                catalog[c_name].contains(ABSENT),
                lacking[1].contains(c_name),
                "{PAGE}: `{c_name}` must say \"{ABSENT}\" exactly when Java and C# lack it"
            );
            absent += usize::from(lacking[1].contains(c_name));
        }

        for (c_name, c_value) in &c {
            let unused = fam.used & c_value == 0;
            assert_eq!(
                catalog[c_name].contains(UNUSED),
                unused,
                "{PAGE}: `{c_name}` must say \"{UNUSED}\" exactly when no function carries it"
            );
            marked += usize::from(unused);
            checked += 1;
        }
    }
    assert_eq!(checked, catalog.len(), "{PAGE}: a catalog row belongs to no flag family");
    assert!(checked >= 30 && marked > 0 && marked < checked, "checked {checked} flags, {marked} of them unused");
    assert!(absent > 0 && absent < checked, "{absent} of {checked} flags absent from Java and C#");
}
