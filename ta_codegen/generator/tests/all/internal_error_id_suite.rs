//! Every internal-error return in the shipped C function tier names its guard.
//!
//! `TA_INTERNAL_ERROR` used to be returned two ways: the batch tier expanded
//! `CIRCBUF_INIT`'s `TA_INTERNAL_ERROR(137)` and the streaming tier returned the
//! bare member, so `== TA_INTERNAL_ERROR` -- the obvious test -- matched one
//! tier and missed the other (issue #259). Every site carries an id now, which
//! makes `>= TA_INTERNAL_ERROR` the only correct test *and* makes the number
//! worth reading: it names one guard.
//!
//! This suite reads the committed `src/ta_func/*.c` rather than generating, so
//! it fails on a tree whose ledger and sources have drifted apart -- a
//! hand-edited id, a ledger reverted without its regenerate, an id allocated
//! twice.

use std::collections::{BTreeMap, BTreeSet};
use std::path::{Path, PathBuf};

fn repo_root() -> PathBuf {
    Path::new(env!("CARGO_MANIFEST_DIR")).join("../..")
}

/// `(file stem without the `ta_` prefix, id)` for every `TA_INTERNAL_ERROR(<id>)`
/// in the shipped function tier, plus the files that returned a bare one.
fn scan_ta_func() -> (Vec<(String, u32)>, Vec<String>, usize) {
    let dir = repo_root().join("src/ta_func");
    let mut sites = Vec::new();
    let mut bare = Vec::new();
    let mut files = 0usize;
    let mut entries: Vec<PathBuf> = std::fs::read_dir(&dir)
        .expect("src/ta_func must exist")
        .filter_map(Result::ok)
        .map(|e| e.path())
        .filter(|p| p.extension().is_some_and(|x| x == "c"))
        .collect();
    entries.sort();
    for path in entries {
        files += 1;
        let stem = path.file_stem().unwrap().to_string_lossy().to_string();
        let func = stem.strip_prefix("ta_").unwrap_or(&stem).to_string();
        let text = std::fs::read_to_string(&path).unwrap();
        for (i, _) in text.match_indices("TA_INTERNAL_ERROR") {
            let rest = &text[i + "TA_INTERNAL_ERROR".len()..];
            let digits: String = rest
                .strip_prefix('(')
                .unwrap_or("")
                .chars()
                .take_while(char::is_ascii_digit)
                .collect();
            if digits.is_empty() {
                bare.push(format!("{}: {}", path.display(), &text[i..(i + 40).min(text.len())]));
            } else {
                sites.push((func.clone(), digits.parse().unwrap()));
            }
        }
    }
    (sites, bare, files)
}

fn load_ledger() -> (u32, BTreeMap<String, u32>) {
    #[derive(serde::Deserialize)]
    struct LedgerFile {
        next: u32,
        sites: BTreeMap<String, u32>,
    }
    let path = repo_root().join("ta_codegen/input/internal_error_ids.yaml");
    let text = std::fs::read_to_string(&path).expect("the id ledger must be committed");
    let f: LedgerFile = serde_yaml::from_str(&text).expect("the id ledger must parse");
    (f.next, f.sites)
}

/// The scan has to be able to fail: a typo in the directory or the needle would
/// otherwise report a clean sweep over nothing. These floors are well under the
/// current corpus (176 functions, 222 guards) and exist only to catch a scan
/// that found nothing at all -- not to pin a count that grows with the library.
#[test]
fn scan_is_not_vacuous() {
    let (sites, _, files) = scan_ta_func();
    assert!(files >= 200, "only {files} .c files scanned in src/ta_func");
    assert!(sites.len() > 100, "only {} internal-error sites found", sites.len());
    let (_, ledger) = load_ledger();
    assert!(ledger.len() > 100, "only {} ledger entries", ledger.len());
}

#[test]
fn no_site_returns_the_bare_member() {
    let (_, bare, _) = scan_ta_func();
    assert!(
        bare.is_empty(),
        "these return TA_INTERNAL_ERROR without a site id, so `== TA_INTERNAL_ERROR` \
         matches them and misses every other site:\n{}",
        bare.join("\n")
    );
}

#[test]
fn every_id_is_reportable_and_clear_of_the_hand_allocated_block() {
    let (sites, _, _) = scan_ta_func();
    for (func, id) in &sites {
        assert!(
            *id > ta_codegen_lib::internal_error_ids::LEGACY_LAST,
            "ta_{func}.c uses id {id}, inside the block hand-allocated before the generator"
        );
        assert!(
            *id <= ta_codegen_lib::internal_error_ids::MAX_ID,
            "ta_{func}.c uses id {id}: TA_SetRetCodeInfo only reports 5000..5999"
        );
    }
}

#[test]
fn one_id_names_one_guard() {
    let (sites, _, _) = scan_ta_func();
    let (next, ledger) = load_ledger();

    // No number is handed out twice.
    let mut by_id: BTreeMap<u32, Vec<&String>> = BTreeMap::new();
    for (key, id) in &ledger {
        by_id.entry(*id).or_default().push(key);
    }
    let shared: Vec<_> = by_id.iter().filter(|(_, k)| k.len() > 1).collect();
    assert!(shared.is_empty(), "ids allocated to more than one guard: {shared:?}");

    // A guard belongs to one function, so its id can only appear in that
    // function's file -- the property that lets a bug report's number be
    // grepped straight to a single line.
    let owner: BTreeMap<u32, String> = ledger
        .iter()
        .map(|(k, id)| (*id, k.split('.').next().unwrap().to_string()))
        .collect();
    let mut wrong_file = Vec::new();
    let mut unknown = BTreeSet::new();
    for (func, id) in &sites {
        match owner.get(id) {
            None => {
                unknown.insert(*id);
            }
            Some(o) if !o.eq_ignore_ascii_case(func) => {
                wrong_file.push(format!("id {id} is {o}'s but appears in ta_{func}.c"));
            }
            Some(_) => {}
        }
    }
    assert!(unknown.is_empty(), "ids in src/ta_func with no ledger entry: {unknown:?}");
    assert!(wrong_file.is_empty(), "{}", wrong_file.join("\n"));

    // And nothing is reserved for a guard that no longer exists: a full
    // `generate` prunes, so a leftover means the ledger was edited by hand or
    // the tree was not regenerated.
    let live: BTreeSet<u32> = sites.iter().map(|(_, id)| *id).collect();
    let dead: Vec<_> = ledger.iter().filter(|(_, id)| !live.contains(id)).collect();
    assert!(dead.is_empty(), "ledger entries with no site in src/ta_func: {dead:?}");

    let high = ledger.values().copied().max().unwrap_or(0);
    assert!(next > high, "ledger `next` is {next}, at or below the allocated high-water mark {high}");
}
