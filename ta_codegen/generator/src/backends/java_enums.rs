//! Generates the shipped Java `FuncUnstId` enum from the `FuncUnstId` list in
//! `ta_codegen/input/enums.yaml` — the same source of truth as the C
//! `include/ta_defs.h` enum (canonical cutover: take over gen_code's Java role).
//!
//! Only the `public enum FuncUnstId { ... }` body is regenerated; the hand-written
//! license header + package line are preserved verbatim (there are no GENCODE
//! markers in this file, so the `public enum FuncUnstId` declaration is the split
//! point). The Java sentinels are named `All` / `None` — referenced as
//! `FuncUnstId.All` by `Core.java` — distinct from the C enum's
//! `FuncUnstAll` / `FuncUnstNone`.

use crate::ir::EnumDef;
use std::collections::HashMap;
use std::path::Path;

/// The shipped enum declaration; everything before it (license + package) is
/// hand-written and preserved.
const ENUM_DECL: &str = "public enum FuncUnstId";

#[allow(clippy::implicit_hasher)]
pub fn generate(enums: &HashMap<String, EnumDef>, path: &Path) {
    let fu = enums
        .get("FuncUnstId")
        .expect("FuncUnstId enum missing from enums.yaml");

    let existing = std::fs::read_to_string(path)
        .unwrap_or_else(|e| panic!("reading {}: {e}", path.display()));
    let split = existing
        .find(ENUM_DECL)
        .unwrap_or_else(|| panic!("'{ENUM_DECL}' missing in {}", path.display()));
    let head = &existing[..split];

    // Each variant's ordinal IS the index into unstablePeriod[], and for the
    // function ids the ordinal equals the C value. `All` is the one exception:
    // it carries C's pinned 0x7FFFFFFF, which no ordinal can express, so every
    // constant declares its value explicitly and `value()` exposes it.
    // `COUNT` sizes the table -- it is what `All` used to be worth.
    let mut body = String::from("public enum FuncUnstId {\n");
    body.push('\t');
    body.push('\n');
    for (i, v) in fu.variants.iter().enumerate() {
        body.push_str(&format!("\t  /* {i:03} */  {}({i}),\n", v.pascal_name));
    }
    body.push('\n');
    body.push_str("\t  /** Wildcard: sets the unstable period for every function at once.\n");
    body.push_str("\t   *  Pinned, so adding an indicator can never move it. */\n");
    body.push_str("\t             All(65535);\n");
    body.push('\n');
    body.push_str("\t/** Number of function ids — the size of the unstable-period table.\n");
    body.push_str("\t *  Not an id, and not {@link #All}. Mirrors C's TA_FUNC_UNST_COUNT. */\n");
    body.push_str(&format!("\tpublic static final int COUNT = {};\n", fu.variants.len()));
    body.push('\n');
    body.push_str("\tprivate final int value;\n");
    body.push('\n');
    body.push_str("\tFuncUnstId(int value) { this.value = value; }\n");
    body.push('\n');
    body.push_str("\t/** The C {@code TA_FuncUnstId} value for this id. */\n");
    body.push_str("\tpublic int value() { return value; }\n");
    body.push_str("};\n");

    super::write_if_changed(path, &format!("{head}{body}"), "FuncUnstId.java", fu.variants.len());
}

/// The shipped MAType enum declaration; everything before it (license + package)
/// is hand-written and preserved.
const MATYPE_DECL: &str = "public enum MAType";

/// Generate the shipped Java `MAType` enum from the `MAType` list in
/// `enums.yaml` — the same source of truth as the C `include/ta_defs.h` enum.
///
/// Only the `public enum MAType { ... }` body is regenerated; the hand-written
/// license header + package line are preserved verbatim (split at the
/// `public enum MAType` declaration). The variant declaration ORDER is the enum
/// ordinal that indexes `optInMAType`; a generation-time assert that the ordinal
/// equals `value:` catches a future non-contiguous edit before it silently
/// reorders the ABI.
#[allow(clippy::implicit_hasher)]
pub fn generate_matype(enums: &HashMap<String, EnumDef>, path: &Path) {
    let ma = enums
        .get("MAType")
        .expect("MAType enum missing from enums.yaml");

    let existing = std::fs::read_to_string(path)
        .unwrap_or_else(|e| panic!("reading {}: {e}", path.display()));
    let split = existing
        .find(MATYPE_DECL)
        .unwrap_or_else(|| panic!("'{MATYPE_DECL}' missing in {}", path.display()));
    let head = &existing[..split];

    let mut body = String::from("public enum MAType \n{\n");
    for (i, v) in ma.variants.iter().enumerate() {
        assert_eq!(
            i32::try_from(i).unwrap(),
            v.value,
            "MAType values must be contiguous from 0 ({} breaks the sequence)",
            v.c_name
        );
        let comma = if i + 1 == ma.variants.len() { "" } else { "," };
        body.push_str(&format!("   {}{comma}\n", v.pascal_name));
    }
    body.push_str("};\n");

    super::write_if_changed(path, &format!("{head}{body}"), "MAType.java", ma.variants.len());
}
