//! Generates the `FuncUnstId` enum (GENCODE SECTION 1) inside the hand-written
//! public header `include/ta_defs.h` — gen_code's last remaining role for that
//! file (canonical cutover: retire gen_code's C side).
//!
//! Only the marked SECTION 1 is regenerated; the rest of `ta_defs.h` is
//! hand-written and left byte-for-byte intact. The list of unstable-period
//! function IDs is the `FuncUnstId` enum in `ta_codegen/input/enums.yaml`; the
//! `TA_FUNC_UNST_ALL` / `_NONE` sentinels are structural and appended here.

use crate::ir::EnumDef;
use std::collections::HashMap;
use std::path::Path;

const START: &str = "/**** START GENCODE SECTION 1 - DO NOT DELETE THIS LINE ****/";
const END: &str = "/**** END GENCODE SECTION 1 - DO NOT DELETE THIS LINE ****/";

/// Render the SECTION 1 body (between, not including, the marker lines) and splice
/// it into `ta_defs_path`, preserving everything outside the markers.
#[allow(clippy::implicit_hasher)]
pub fn generate(enums: &HashMap<String, EnumDef>, ta_defs_path: &Path) {
    let fu = enums
        .get("FuncUnstId")
        .expect("FuncUnstId enum missing from enums.yaml");

    // Explicit `= N` initializers, TA_MAType-style: the value is public ABI
    // (unstablePeriod[] is indexed by it; retired UNUSED_* slots keep theirs),
    // so write the contract into the header instead of implying it by list
    // order. Contiguity from 0 is still asserted because the Java and Rust
    // backends index by declaration ORDER (enum ordinal / variant position).
    let width = fu.variants.iter().map(|v| v.c_name.len()).max().unwrap_or(0);

    let mut block = String::new();
    block.push('\n');
    block.push_str("typedef enum {\n");
    for (i, v) in fu.variants.iter().enumerate() {
        assert_eq!(
            i32::try_from(i).unwrap(),
            v.value,
            "FuncUnstId values must be contiguous from 0 ({} breaks the sequence)",
            v.c_name
        );
        block.push_str(&format!("    {:<width$} = {:2},\n", v.c_name, v.value));
    }
    block.push_str(&format!("    {:<width$} = {:2},\n", "TA_FUNC_UNST_ALL", fu.variants.len()));
    block.push_str(&format!("    {:<width$} = -1\n", "TA_FUNC_UNST_NONE"));
    block.push_str("} TA_FuncUnstId;\n");

    let existing = std::fs::read_to_string(ta_defs_path)
        .unwrap_or_else(|e| panic!("reading {}: {e}", ta_defs_path.display()));
    let s = existing
        .find(START)
        .unwrap_or_else(|| panic!("START GENCODE marker missing in {}", ta_defs_path.display()))
        + START.len();
    let e = existing
        .find(END)
        .unwrap_or_else(|| panic!("END GENCODE marker missing in {}", ta_defs_path.display()));
    let e_line = existing[..e].rfind('\n').map_or(0, |i| i + 1);

    let new = format!("{}\n{}\n{}", &existing[..s], block, &existing[e_line..]);
    super::write_if_changed(ta_defs_path, &new, "ta_defs.h (FuncUnstId)", fu.variants.len());
}
