//! Assembles the shipped `java/library/src/io/github/talib/Core.java` GENCODE section
//! from the per-indicator Java methods — ta_codegen's takeover of gen_code's role
//! for the shipped Java library.
//!
//! The per-indicator method text is the SAME output the JSON-RPC Java server
//! inlines (so numerical correctness is already proven by `ta_regtest --codegen`);
//! here it is spliced between the `GENCODE SECTION 1` markers, preserving the
//! hand-written scaffolding (license, immutable fields, builder entry points,
//! candle/unstable-period accessors) outside them. Each indicator contributes a
//! public lookback, the package-private `xxxInternal` core (and, where
//! applicable, `xxxPrivate`), plus the public `OutRange`-returning `xxx`
//! wrapper over it.

use std::collections::HashMap;
use std::path::Path;

use crate::helper_registry::HelperRegistry;
use crate::ir::{EnumDef, FuncDef};
use crate::registry::Registry;

const CORE_START: &str = "/**** START GENCODE SECTION 1 - DO NOT DELETE THIS LINE ****/";
const CORE_END: &str = "/**** END GENCODE SECTION 1 - DO NOT DELETE THIS LINE ****/";

/// Splice the generated indicator methods into `Core.java`'s GENCODE section.
#[allow(clippy::implicit_hasher)]
pub fn generate_core(
    funcs: &[FuncDef],
    enums: &HashMap<String, EnumDef>,
    registry: &Registry,
    helpers: &HelperRegistry,
    core_path: &Path,
) {
    let mut section = String::new();
    for func in funcs {
        section.push_str(&super::java::generate(func, enums, registry, helpers));
    }

    let existing = std::fs::read_to_string(core_path)
        .unwrap_or_else(|e| panic!("reading {}: {e}", core_path.display()));
    let start = existing
        .find(CORE_START)
        .unwrap_or_else(|| panic!("START GENCODE marker missing in {}", core_path.display()));
    // Keep everything up to and including the START marker line (the marker text
    // is preceded by its hand-written 3-space indent, which stays untouched).
    let after_start = existing[start..]
        .find('\n')
        .map_or(existing.len(), |i| start + i + 1);
    let end = existing
        .find(CORE_END)
        .unwrap_or_else(|| panic!("END GENCODE marker missing in {}", core_path.display()));
    // Resume at the start of the END marker line, preserving its indent + the rest.
    let end_line = existing[..end].rfind('\n').map_or(0, |i| i + 1);

    let new = format!("{}{}{}", &existing[..after_start], section, &existing[end_line..]);
    super::write_if_changed(core_path, &new, "Core.java (GENCODE)", funcs.len());
}

// `CoreAnnotated.java` and the `meta/annotation/` reflection island used to be
// generated here, along with a private copy of the flag bit-values for their
// annotations. All three went with the idiomatic-signature break: the annotations
// were emitted against the old `RetCode` + `MInteger` shape, and the flag copy had
// no other consumer — Java's abstract table takes its bits from
// `rust_abstract`'s helpers, which `flag_sync` still gates.
