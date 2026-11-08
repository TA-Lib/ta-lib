//! The release ledger `ta_codegen/input/releases.yaml` and the `Added in` line it
//! puts on every function page (#585). The ledger is derived from the tags by
//! `scripts/function_releases.py` and kept whole by `scripts/sync.py`; the page
//! rule (wrapper version from 0.8.1 on, none for the untagged row at `VERSION`)
//! lives in `docs_site`, so this is where both are pinned against the corpus.

use std::collections::{BTreeMap, HashMap, HashSet};
use std::path::{Path, PathBuf};

use crate::common;
use ta_codegen_lib::backends::docs_site::{self, Releases};
use ta_codegen_lib::parser;

pub(crate) fn repo_root() -> PathBuf {
    Path::new(env!("CARGO_MANIFEST_DIR")).join("../..")
}

fn ledger() -> BTreeMap<String, Vec<String>> {
    let path = repo_root().join("ta_codegen/input/releases.yaml");
    let text = std::fs::read_to_string(&path).unwrap_or_else(|e| panic!("{}: {e}", path.display()));
    serde_yaml::from_str(&text).unwrap_or_else(|e| panic!("{}: {e}", path.display()))
}

fn version() -> String {
    std::fs::read_to_string(repo_root().join("VERSION")).unwrap().trim().to_string()
}

/// One row per function, no function in two rows.
#[test]
fn the_ledger_covers_every_corpus_function_once() {
    let corpus: HashSet<String> = common::discover_indicators().iter().map(|n| n.to_uppercase()).collect();
    assert!(corpus.len() > 200, "corpus discovery found {}", corpus.len());
    let ledger = ledger();
    let mut seen: HashMap<&str, &str> = HashMap::new();
    for (release, names) in &ledger {
        for name in names {
            if let Some(other) = seen.insert(name, release) {
                panic!("{name} is in both {other} and {release}");
            }
            assert!(corpus.contains(name), "{name} ({release}) is not in ta_codegen/input");
        }
    }
    let listed: HashSet<String> = seen.keys().map(|n| (*n).to_string()).collect();
    let mut missing: Vec<&String> = corpus.difference(&listed).collect();
    missing.sort();
    assert!(missing.is_empty(), "not in releases.yaml (run scripts/sync.py): {missing:?}");
}

/// `Releases::check` is what stops `generate`: the ledger row and a ta-lib-python
/// group page for every function's `group:`.
#[test]
fn every_corpus_function_passes_the_generate_gate() {
    let root = repo_root();
    let releases = Releases::load(&root);
    let mut groups = HashSet::new();
    for name in common::discover_indicators() {
        let func = parser::yaml::parse_yaml(&root.join(format!("ta_codegen/input/{name}/{name}.yaml")));
        groups.insert(func.group.clone());
        releases.check(&func).unwrap_or_else(|e| panic!("{e}"));
    }
    assert!(groups.len() >= 10, "groups seen: {groups:?}");
    let mut unknown = parser::yaml::parse_yaml(&root.join("ta_codegen/input/sma/sma.yaml"));
    unknown.group = "Nonesuch".to_string();
    let err = releases.check(&unknown).unwrap_err();
    assert!(err.contains("Nonesuch") && err.contains("ta-lib-python"), "{err}");
    let mut unlisted = parser::yaml::parse_yaml(&root.join("ta_codegen/input/sma/sma.yaml"));
    unlisted.name = "NONESUCH".to_string();
    let err = releases.check(&unlisted).unwrap_err();
    assert!(err.contains("scripts/sync.py"), "{err}");
}

/// Render the website pages of `names` through `docs_site::generate` under a throwaway
/// root holding only what the renderer reads: the ledger, `VERSION` and the pages'
/// sources. Returns each page's text.
pub(crate) fn render(names: &[&str]) -> HashMap<String, String> {
    let real = repo_root();
    let root = std::env::temp_dir().join(format!("ta_added_in_{}_{}", std::process::id(), names.join("_")));
    let _ = std::fs::remove_dir_all(&root);
    std::fs::create_dir_all(root.join("ta_codegen/input")).unwrap();
    std::fs::copy(real.join("VERSION"), root.join("VERSION")).unwrap();
    std::fs::copy(real.join("ta_codegen/input/releases.yaml"), root.join("ta_codegen/input/releases.yaml")).unwrap();
    let mut funcs = Vec::new();
    let mut enums = HashMap::new();
    for name in names {
        let dir = name.to_lowercase();
        std::fs::create_dir_all(root.join("ta_codegen/input").join(&dir)).unwrap();
        std::fs::copy(
            real.join(format!("ta_codegen/input/{dir}/{dir}.md")),
            root.join(format!("ta_codegen/input/{dir}/{dir}.md")),
        )
        .unwrap();
        let (func, e) = common::load_indicator(&dir);
        funcs.push(func);
        enums = e;
    }
    docs_site::generate(&funcs, &enums, &root);
    let pages = names
        .iter()
        .map(|name| {
            let dir = name.to_lowercase();
            let page = std::fs::read_to_string(root.join(format!("website/src/functions/{dir}.md")))
                .unwrap_or_else(|e| panic!("{dir}.md was not rendered: {e}"));
            ((*name).to_string(), page)
        })
        .collect();
    let _ = std::fs::remove_dir_all(&root);
    pages
}

fn implementation_section(page: &str) -> &str {
    let at = page.find("## Implementation").expect("page has an Implementation section");
    let rest = &page[at..];
    let end = rest[2..].find("\n## ").map_or(rest.len(), |i| i + 2);
    &rest[..end]
}

/// Before 0.8.1 the wrapper's version says nothing about the function set, so those
/// pages name the C release alone; the Python line still names the call and the group page.
#[test]
fn pages_from_the_first_releases_name_the_c_release_alone() {
    let pages = render(&["SMA", "ACCBANDS"]);
    let sma = implementation_section(&pages["SMA"]);
    assert!(sma.contains("\nAdded in 0.4.0.\n"), "{sma}");
    assert!(!sma.contains("(ta-lib-python"), "{sma}");
    assert!(sma.contains("Python: `talib.SMA`, see [ta-lib-python](https://ta-lib.github.io/ta-lib-python/func_groups/overlap_studies.html)."), "{sma}");
    assert!(sma.contains("available for R and more"), "{sma}");
    let accbands = implementation_section(&pages["ACCBANDS"]);
    assert!(accbands.contains("\nAdded in 0.5.0.\n"), "{accbands}");
    assert!(!accbands.contains("(ta-lib-python"), "{accbands}");
}

/// From 0.8.1 on, the tagged release carries the wrapper version and the Python line.
#[test]
fn pages_from_a_tracked_release_carry_the_wrapper_version() {
    let pages = render(&["HMA"]);
    let hma = implementation_section(&pages["HMA"]);
    assert!(hma.contains("\nAdded in 0.8.1 (ta-lib-python 0.8.1).\n"), "{hma}");
    assert!(
        hma.contains(
            "Python: `talib.HMA` (ta-lib-python 0.8.1 or later), see \
             [ta-lib-python](https://ta-lib.github.io/ta-lib-python/func_groups/overlap_studies.html)."
        ),
        "{hma}"
    );
    assert!(hma.contains("available for R and more"), "{hma}");
    assert_eq!(hma.matches("Python").count(), 1, "the section names Python once:\n{hma}");
}

/// The row at `VERSION` is the one not tagged yet: the page names the release it will
/// ship in and promises the wrapper from that version on, nothing about the wrapper's
/// own release.
#[test]
fn the_pending_row_names_version_without_a_wrapper_release() {
    let version = version();
    let ledger = ledger();
    let Some(pending) = ledger.get(&version) else {
        eprintln!("no row at VERSION {version}: nothing pending, nothing to check");
        return;
    };
    let name = pending.first().expect("the pending row is not empty");
    let pages = render(&[name]);
    let section = implementation_section(&pages[name.as_str()]);
    assert!(section.contains(&format!("\nAdded in {version}.\n")), "{section}");
    assert!(section.contains(&format!("Python: `talib.{name}` (ta-lib-python {version} or later), see ")), "{section}");
    assert!(!section.contains(&format!("(ta-lib-python {version}).")), "{section}");
}

/// The ledger's rows are `M.m.p` versions, and the rule's threshold is a real release.
#[test]
fn ledger_rows_are_releases_in_order() {
    let ledger = ledger();
    let keys: Vec<(u32, u32, u32)> = ledger
        .keys()
        .map(|k| {
            let p: Vec<u32> = k.split('.').map(|x| x.parse().unwrap_or_else(|_| panic!("row `{k}`"))).collect();
            assert_eq!(p.len(), 3, "row `{k}`");
            (p[0], p[1], p[2])
        })
        .collect();
    assert!(keys.contains(&(0, 4, 0)) && keys.contains(&(0, 8, 1)), "{keys:?}");
    for row in ledger.values() {
        assert!(!row.is_empty());
        let mut sorted = row.clone();
        sorted.sort();
        assert_eq!(*row, sorted, "names are sorted within a row");
    }
}
