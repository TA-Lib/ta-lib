//! Generates `src/ta_common/ta_retcode.c` (the `TA_SetRetCodeInfo` lookup table)
//! — gen_code's last role for that file (canonical cutover: retire gen_code's C
//! side).
//!
//! The dynamic part is the `retCodeInfoTable[]` array, built from the shipped
//! source `src/ta_common/ta_retcode.csv` (`id,enumStr,infoStr` per line). The
//! static scaffolding (license, struct, `TA_SetRetCodeInfo`) lives in the
//! template `ta_codegen/generator/templates/c/ta_retcode.c.template`, with the table region
//! marked by [`MARKER`]. The `0xFFFF` / `TA_UNKNOWN_ERR` sentinel is appended as
//! the final (comma-less) entry, matching gen_code.

use std::path::Path;

const MARKER: &str = "%%RETCODE_INFO_TABLE%%";

/// Splice the csv-derived `retCodeInfoTable[]` rows into the template and write
/// the result to `out_path`.
pub fn generate(template_path: &Path, csv_path: &Path, out_path: &Path) {
    let template = std::fs::read_to_string(template_path)
        .unwrap_or_else(|e| panic!("reading {}: {e}", template_path.display()));
    let csv = std::fs::read_to_string(csv_path)
        .unwrap_or_else(|e| panic!("reading {}: {e}", csv_path.display()));

    let mut rows: Vec<String> = Vec::new();
    for line in csv.lines() {
        if line.trim().is_empty() {
            continue;
        }
        let mut parts = line.splitn(3, ',');
        let id = parts.next().unwrap_or_default();
        let enum_str = parts.next().unwrap_or_default();
        let info_str = parts.next().unwrap_or_default();
        rows.push(format!(
            "         {{(TA_RetCode){id},\"{enum_str}\",\"{info_str}\"}}"
        ));
    }
    // gen_code always emits TA_UNKNOWN_ERR (0xFFFF) as the last table entry; it is
    // not in the csv. It is comma-less because it is the final element.
    rows.push("         {(TA_RetCode)0xFFFF,\"TA_UNKNOWN_ERR\",\"Unknown Error\"}".to_string());

    let table = rows.join(",\n");
    let content = template.replace(MARKER, &table);
    super::write_if_changed(out_path, &content, "ta_retcode.c", rows.len());
}
