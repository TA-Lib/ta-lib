//! Site ids for C's `TA_INTERNAL_ERROR(<id>)`.
//!
//! `TA_INTERNAL_ERROR(Id)` (`include/ta_common.h`) is `5000 + Id`, and
//! `TA_SetRetCodeInfo` traps the whole 5000..5999 band, so an id is 1..=999 and
//! nothing wider will ever be reportable. Every internal-error guard the C
//! backend emits into `src/ta_func/*.c` takes one from this ledger, so the code
//! a caller sees names the guard that fired rather than the class it belongs to.
//!
//! The ledger is read from and written back to
//! `ta_codegen/input/internal_error_ids.yaml`, which is what makes an id stable:
//! a key that already has a number keeps it forever, and a new site takes `next`.
//! Sequence-position schemes were the alternative and all share one defect —
//! inserting an indicator renumbers every site after it, so the number in a bug
//! report from one release names a different guard in the next.
//!
//! A key is `<FUNCTION>.<guard kind>[.<state field>]` and names one *guard*, not
//! one line: the double-precision body, the `TA_S_` body and the streaming
//! `Open` all render the same guard, so all three share its id.

use std::cell::RefCell;
use std::collections::{BTreeMap, BTreeSet};
use std::path::Path;

/// Ids 1..=180 are the block hand-allocated before the generator existed —
/// `ta_regtest`'s per-test codes, `ta_abstract`'s 1..5, and `CIRCBUF_INIT`'s 137
/// in `ta_common/ta_memory.h`. Generated sites start above it so no shipped
/// number changes meaning.
pub const LEGACY_LAST: u32 = 180;

/// The largest id `TA_SetRetCodeInfo` still reports as an internal error.
pub const MAX_ID: u32 = 999;

const LEDGER_HEADER: &str = "\
# TA_INTERNAL_ERROR site ids for the generated C function tier.
#
# TA_INTERNAL_ERROR(Id) (include/ta_common.h) returns 5000+Id, and
# TA_SetRetCodeInfo traps the whole 5000..5999 band, so an id is 1..999.
# Every guard the generator emits into src/ta_func/*.c takes one from here, so
# the code a caller sees names the guard that fired, not just its class.
#
# A key is <FUNCTION>.<guard kind>[.<state field>] and names one guard, not one
# line: the double-precision body, the TA_S_ body and the streaming Open all
# render the same guard and all three share its id.
#
# `next` is the high-water mark for the whole tree, hand-written sites included
# -- take it and raise it. Ids 1..180 are the block allocated by hand before
# the generator existed (ta_regtest's per-test codes, ta_abstract's 1..5, and
# CIRCBUF_INIT's 137) and are not listed. An id is never reused: `next` only
# rises, so a retired site's number retires with it.
#
# Written by `ta_codegen generate`. Do not renumber by hand -- the number is
# the only thing a bug report carries, and it has to mean the same guard in the
# next release as it did in the last.
";

#[derive(serde::Deserialize)]
struct LedgerFile {
    next: u32,
    #[serde(default)]
    sites: BTreeMap<String, u32>,
}

struct Ledger {
    next: u32,
    sites: BTreeMap<String, u32>,
    /// Keys this run actually asked for, so a full run can drop the entries of
    /// indicators that no longer exist. A partial run never prunes.
    used: BTreeSet<String>,
    /// The function currently being emitted, set by [`FuncScope`].
    func: Option<String>,
}

thread_local! {
    /// The ledger for this process. The guards are emitted deep in the C
    /// statement walker and in the stream emitter's helper tree, neither of
    /// which is handed a context object -- the same reason `c::CIRCBUF_ORDER`
    /// exists. Codegen runs one function at a time, so the current-function
    /// name can live here too.
    ///
    /// Unlike the two thread-locals in `backends::c`, this one accumulates
    /// across the WHOLE run rather than one function: generating on more than
    /// one thread would give each its own ledger and save an empty one. That is
    /// loud rather than silent -- every id in the tree would move at once, and
    /// `regen-check` compares committed bytes -- but it is why this cannot
    /// simply follow generation onto a thread pool.
    static LEDGER: RefCell<Ledger> = const {
        RefCell::new(Ledger {
            next: LEGACY_LAST + 1,
            sites: BTreeMap::new(),
            used: BTreeSet::new(),
            func: None,
        })
    };
}

/// Read the ledger. A missing file starts an empty one at `LEGACY_LAST + 1`;
/// a malformed one is fatal, because silently starting over would renumber
/// every site in the tree.
pub fn load(path: &Path) {
    let Ok(text) = std::fs::read_to_string(path) else {
        eprintln!(
            "note: {} not found -- allocating internal-error site ids from scratch",
            path.display()
        );
        return;
    };
    let parsed: LedgerFile = serde_yaml::from_str(&text)
        .unwrap_or_else(|e| panic!("{}: malformed internal-error id ledger: {e}", path.display()));
    LEDGER.with(|l| {
        let mut l = l.borrow_mut();
        // `next` is authoritative, but never below what the file already hands
        // out: a hand-edited entry above it would otherwise be allocated twice.
        let high = parsed.sites.values().copied().max().unwrap_or(0);
        l.next = parsed.next.max(LEGACY_LAST + 1).max(high + 1);
        l.sites = parsed.sites;
    });
}

/// Write the ledger back. `prune` drops the entries no site asked for this run,
/// so it is for a run that re-emitted every C file and no other: a `--func=`
/// filter, or a `--backend=` that left C out, asks for a fraction of the keys or
/// none, and pruning there would free every id it did not happen to look at.
pub fn save(path: &Path, prune: bool) {
    let body = LEDGER.with(|l| {
        let mut l = l.borrow_mut();
        if prune {
            let used = l.used.clone();
            l.sites.retain(|k, _| used.contains(k));
        }
        let mut s = String::new();
        s.push_str(LEDGER_HEADER);
        s.push_str(&format!("next: {}\n", l.next));
        s.push_str("sites:\n");
        for (k, id) in &l.sites {
            s.push_str(&format!("  {k}: {id}\n"));
        }
        s
    });
    let unchanged = std::fs::read_to_string(path).is_ok_and(|old| old == body);
    crate::emit::write_if_changed(path, &body)
        .unwrap_or_else(|e| panic!("{}: cannot write internal-error id ledger: {e}", path.display()));
    if !unchanged {
        println!("  internal-error site ids -> {}", path.display());
    }
}

/// Names the function whose C file is being emitted, for the span of one
/// `backends::c::generate` call. Every key requested inside is prefixed with it.
///
/// Nests: the stream section installs its own so it can also be generated on its
/// own (the backend suite does), and restores the enclosing name on the way out.
pub struct FuncScope {
    prev: Option<String>,
}

impl FuncScope {
    pub fn new(func_name: &str) -> Self {
        let prev = LEDGER.with(|l| l.borrow_mut().func.replace(func_name.to_uppercase()));
        Self { prev }
    }
}

impl Drop for FuncScope {
    fn drop(&mut self) {
        let prev = self.prev.take();
        LEDGER.with(|l| l.borrow_mut().func = prev);
    }
}

/// The id for one guard of the function in scope, e.g. `site("circbuf.dRing")`.
/// Idempotent: the batch, `TA_S_` and streaming renders of one guard all land on
/// the same key and get the same number back.
pub fn site(kind: &str) -> u32 {
    LEDGER.with(|l| {
        let mut l = l.borrow_mut();
        let func = l
            .func
            .clone()
            .expect("internal_error_ids::site called outside a FuncScope");
        let key = format!("{func}.{kind}");
        l.used.insert(key.clone());
        if let Some(id) = l.sites.get(&key) {
            return *id;
        }
        let id = l.next;
        assert!(
            id <= MAX_ID,
            "internal-error ids exhausted: TA_SetRetCodeInfo only reports 5000..{}, \
             so an id above {MAX_ID} is unreportable (allocating for {key})",
            5000 + MAX_ID
        );
        l.next = id + 1;
        l.sites.insert(key, id);
        id
    })
}
