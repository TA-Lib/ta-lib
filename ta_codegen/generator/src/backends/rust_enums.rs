//! Generates the shipped Rust `MAType` enum from `enums.yaml` — the same source
//! of truth as C's `include/ta_defs.h`, `MAType.java` and `MAType.cs`.
//!
//! The rendered text is spliced into the generated `ta_func/mod.rs` (see
//! `generate_rust_crate_scaffolding`), so it needs no module of its own and
//! cannot be orphaned by the clean step.
//!
//! Two shape differences from the other backends are deliberate:
//!
//! - **No `#[repr]`.** The crate has no FFI (`forbid(unsafe_code)`, no
//!   `extern "C"`, no transmute), so the layout is unobservable; the explicit
//!   discriminants are what carry the pinned ABI values through `as i32`.
//! - **`#[non_exhaustive]`.** `MAType` is append-only, so without it every
//!   appended member would be a breaking change for every downstream `match`.
//!   It buys nothing in-crate: the generated dispatch already ends in a
//!   wildcard that rejects, so an appended member compiles and lands there.

use crate::ir::EnumDef;
use std::collections::HashMap;

/// Render the `MAType` enum plus its `TryFrom<i32>`.
///
/// Returns an empty string when the enum is absent, so a caller can splice
/// unconditionally.
#[allow(clippy::implicit_hasher)]
#[must_use]
pub fn render_matype(enums: &HashMap<String, EnumDef>) -> String {
    let Some(ma) = enums.get("MAType") else {
        return String::new();
    };

    let mut s = String::new();
    s.push_str(
        "\n/// Moving-average type selected by an `optInMAType` parameter.\n\
         ///\n\
         /// The values are pinned ABI, shared with C's `TA_MAType` and the Java and\n\
         /// C# `MAType`; the list is append-only. Convert a raw value in with\n\
         /// [`TryFrom<i32>`](MAType::try_from), and out with `as i32`.\n\
         #[derive(Debug, Clone, Copy, PartialEq, Eq, Hash)]\n\
         #[non_exhaustive]\n\
         #[allow(non_camel_case_types)]\n\
         pub enum MAType {\n",
    );
    for v in &ma.variants {
        let doc = super::common::ma_pseudo_member_doc(&v.name)
            .map_or_else(|| format!("The `{}` moving average.", v.c_name), str::to_string);
        s.push_str(&format!("    /// {doc}\n"));
        s.push_str(&format!("    {} = {},\n", v.name, v.value));
    }
    s.push_str("}\n");

    // The sentinel arm is emitted only when no member already carries that
    // value, so a future `enums.yaml` that pins one there cannot produce a
    // duplicate-pattern build error.
    let sentinel_collides = ma
        .variants
        .iter()
        .any(|v| v.value == super::common::TA_INTEGER_DEFAULT);
    let sentinel_arm = if sentinel_collides {
        String::new()
    } else {
        super::common::enum_default_variant(enums, "MAType").map_or_else(String::new, |d| {
            format!("            i32::MIN => Self::{},\n", d.name)
        })
    };

    s.push_str(
        "\nimpl TryFrom<i32> for MAType {\n    \
         type Error = RetCode;\n\n    \
         /// Convert a raw parameter value, as the abstract layer and the JSON-RPC\n    \
         /// server hold it.\n    ///\n    \
         /// `i32::MIN` — C's `TA_INTEGER_DEFAULT` — resolves to the `DEFAULT`\n    \
         /// member, so a value arriving through the abstract layer still selects\n    \
         /// that parameter's documented default exactly as it does in C. Every\n    \
         /// other out-of-domain value is `BadParam`, which is what keeps the\n    \
         /// rejection in this crate rather than in a caller that wraps it.\n    \
         ///\n    \
         /// # Errors\n    ///\n    \
         /// [`RetCode::BadParam`] if the value names no member.\n    \
         fn try_from(value: i32) -> Result<Self, Self::Error> {\n        \
         Ok(match value {\n",
    );
    for v in &ma.variants {
        s.push_str(&format!("            {} => Self::{},\n", v.value, v.name));
    }
    s.push_str(&sentinel_arm);
    s.push_str(
        "            _ => return Err(RetCode::BadParam),\n        \
         })\n    }\n}\n",
    );
    s
}
