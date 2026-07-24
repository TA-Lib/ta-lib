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

/// Render one `= N` initializer line (`    NAME  =  N`), padding the name to
/// `width` and the value to 2 columns — the shared FuncUnstId/MAType style.
fn enum_line(c_name: &str, value: i32, width: usize, comma: &str) -> String {
    format!("    {c_name:<width$} = {value:2}{comma}\n")
}

/// Render a `typedef enum { ... } TA_<Name>;` block in explicit `= N` style.
/// The value is public ABI, so it is written into the header rather than implied
/// by list order; contiguity from 0 is asserted because the Java and Rust
/// backends index by declaration ORDER (enum ordinal / variant position).
/// The last variant drops its trailing comma (no sentinels follow).
fn render_enum(def: &EnumDef, type_name: &str) -> String {
    let width = def.variants.iter().map(|v| v.c_name.len()).max().unwrap_or(0);
    let mut block = String::from("typedef enum {\n");
    for (i, v) in def.variants.iter().enumerate() {
        assert_eq!(
            i32::try_from(i).unwrap(),
            v.value,
            "{type_name} values must be contiguous from 0 ({} breaks the sequence)",
            v.c_name
        );
        let comma = if i + 1 == def.variants.len() { "" } else { "," };
        block.push_str(&enum_line(&v.c_name, v.value, width, comma));
    }
    block.push_str(&format!("}} {type_name};\n"));
    block
}

/// Render the SECTION 1 body (between, not including, the marker lines) and splice
/// it into `ta_defs_path`, preserving everything outside the markers.
///
/// SECTION 1 owns two enums that are pure functions of `enums.yaml`: `TA_MAType`
/// and `TA_FuncUnstId` (the `TA_FUNC_UNST_ALL` / `_NONE` sentinels are structural
/// and appended here). Everything else in `ta_defs.h` is hand-written.
#[allow(clippy::implicit_hasher)]
pub fn generate(enums: &HashMap<String, EnumDef>, ta_defs_path: &Path) {
    let ma = enums
        .get("MAType")
        .expect("MAType enum missing from enums.yaml");
    let fu = enums
        .get("FuncUnstId")
        .expect("FuncUnstId enum missing from enums.yaml");

    // FuncUnstId carries two trailing sentinels after its variants; render it with
    // all-comma variants + the appended ALL/NONE lines.
    let fu_width = fu.variants.iter().map(|v| v.c_name.len()).max().unwrap_or(0);
    let mut fu_block = String::from("typedef enum {\n");
    for (i, v) in fu.variants.iter().enumerate() {
        assert_eq!(
            i32::try_from(i).unwrap(),
            v.value,
            "FuncUnstId values must be contiguous from 0 ({} breaks the sequence)",
            v.c_name
        );
        fu_block.push_str(&enum_line(&v.c_name, v.value, fu_width, ","));
    }
    fu_block.push_str(&enum_line("TA_FUNC_UNST_ALL", i32::try_from(fu.variants.len()).unwrap(), fu_width, ","));
    fu_block.push_str(&format!("    {:<fu_width$} = -1\n", "TA_FUNC_UNST_NONE"));
    fu_block.push_str("} TA_FuncUnstId;\n");

    // TA_MAType then TA_FuncUnstId, separated by a blank line.
    let block = format!("\n{}\n{}", render_enum(ma, "TA_MAType"), fu_block);

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
    super::write_if_changed(
        ta_defs_path,
        &new,
        "ta_defs.h (MAType + FuncUnstId)",
        ma.variants.len() + fu.variants.len(),
    );
}
