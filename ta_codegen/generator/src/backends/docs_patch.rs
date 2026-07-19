//! Maintains small generated *regions* inside otherwise hand-written website pages.
//!
//! This is the counterpart to [`super::docs_site`], and the split is deliberate:
//! `docs_site` **owns whole files** under `website/src/functions/` (overwritten
//! wholesale on every run), whereas this module **edits a marked region** of a page
//! whose surrounding prose is hand-written and must survive byte-for-byte. Keeping
//! the two apart keeps the "do not hand-edit" hazard confined to a region a reader
//! can actually see in the source.
//!
//! The mechanism mirrors [`super::ta_defs`], which already splices the `FuncUnstId`
//! enum into the hand-written `include/ta_defs.h`. The difference is the marker
//! syntax: HTML comments, which markdown passes through and the browser never
//! renders, so the page reads normally on ta-lib.org.
//!
//! ```markdown
//! <!-- ta_codegen:begin unstable-func-list -->
//! `ADX`, `ADXR`, ...
//! <!-- ta_codegen:end unstable-func-list -->
//! ```
//!
//! A missing or out-of-order marker pair is a hard error rather than a silent skip:
//! if someone restructures a page and drops the markers, the generator must say so
//! instead of quietly ceasing to maintain the region.
//!
//! Writes go through [`super::write_if_changed`], so an unchanged region leaves the
//! file's mtime alone — required for the regen oracle (fresh `generate` + git-clean
//! check) to stay green.
//!
//! To add a fragment: write a renderer returning the region body, then add one
//! [`Fragment`] row in [`generate`] and the marker pair to the page.

use crate::ir::EnumDef;
use std::collections::HashMap;
use std::path::Path;

/// One generated region inside a hand-written page.
struct Fragment {
    /// Repo-relative path of the page holding the marked region.
    page: &'static str,
    /// Marker id, as it appears in `<!-- ta_codegen:begin <id> -->`.
    id: &'static str,
    /// Human-readable label for the status line.
    label: &'static str,
    /// Rendered region body (without the surrounding marker lines).
    body: String,
    /// Item count, for the status line.
    count: usize,
}

/// Refresh every generated region in the hand-written website pages.
#[allow(clippy::implicit_hasher)]
pub fn generate(enums: &HashMap<String, EnumDef>, root: &Path) {
    let unstable = unstable_func_list(enums);

    let fragments = [Fragment {
        page: "website/src/api/unstable-period/README.md",
        id: "unstable-func-list",
        label: "unstable-period page (function list)",
        count: unstable.0,
        body: unstable.1,
    }];

    for fragment in &fragments {
        fragment.apply(root);
    }
}

impl Fragment {
    /// Splice `body` between this fragment's markers, leaving the rest of the page intact.
    fn apply(&self, root: &Path) {
        let path = root.join(self.page);
        let begin = format!("<!-- ta_codegen:begin {} -->", self.id);
        let end = format!("<!-- ta_codegen:end {} -->", self.id);

        let existing = std::fs::read_to_string(&path)
            .unwrap_or_else(|e| panic!("reading {}: {e}", path.display()));

        let s = existing
            .find(&begin)
            .unwrap_or_else(|| panic!("marker `{begin}` missing in {}", self.page))
            + begin.len();
        let e = existing
            .find(&end)
            .unwrap_or_else(|| panic!("marker `{end}` missing in {}", self.page));
        assert!(s < e, "marker `{begin}` appears after `{end}` in {}", self.page);

        // Rewind to the start of the end-marker's line so the splice cannot
        // accumulate blank lines across runs.
        let e_line = existing[..e].rfind('\n').map_or(0, |i| i + 1);

        let new = format!("{}\n{}\n{}", &existing[..s], self.body, &existing[e_line..]);
        super::write_if_changed(&path, &new, self.label, self.count);
    }
}

/// The unstable-period function names, as an inline code-formatted list.
///
/// Source of truth is the `FuncUnstId` enum in `ta_codegen/input/enums.yaml` — the
/// same input that produces the enum in `include/ta_defs.h`, so the page and the
/// public ABI cannot drift apart. The `TA_FUNC_UNST_ALL` / `_NONE` sentinels are not
/// in `variants` (the C backend appends them structurally). Reserved `UNUSED_<n>`
/// slots are: they hold ordinals for functions since reclassified as stable, kept so
/// the ABI numbering is stable, and must not be advertised as usable ids.
///
/// Returns `(count, body)`.
fn unstable_func_list(enums: &HashMap<String, EnumDef>) -> (usize, String) {
    let fu = enums
        .get("FuncUnstId")
        .expect("FuncUnstId enum missing from enums.yaml");

    let names: Vec<String> = fu
        .variants
        .iter()
        .map(|v| {
            v.c_name
                .strip_prefix("TA_FUNC_UNST_")
                .unwrap_or_else(|| panic!("FuncUnstId variant `{}` lacks the TA_FUNC_UNST_ prefix", v.c_name))
        })
        .filter(|name| !name.starts_with("UNUSED_"))
        .map(|name| format!("`{name}`"))
        .collect();

    (names.len(), format!("{}.", names.join(", ")))
}
