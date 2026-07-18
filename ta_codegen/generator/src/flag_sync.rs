//! Flag-map synchronization gate.
//!
//! The YAML flag vocabulary (function flags like `stream`, opt-input flags
//! like `percent`, output flags like `nullable`) surfaces in FIVE generated
//! places that historically drift apart when a flag is added:
//!
//! 1. `backends/ta_abstract_c.rs` — C constant names (`TA_FUNC_FLG_STREAM`)
//! 2. `backends/rust_abstract.rs` — numeric bits for the Rust abstract table
//!    (also reused by `backends/java_abstract.rs` for the Java server table)
//! 3. `backends/java_shipped.rs` — numeric bits for `CoreAnnotated` annotations
//! 4. `backends/func_api_xml.rs` — XML display labels
//! 5. `include/ta_abstract.h` — the C ABI's authoritative `#define`s
//!
//! The tests below make `include/ta_abstract.h` the single authority: every
//! `TA_FUNC_FLG_*` / `TA_OPTIN_*` / `TA_OUT_*` constant must be known — with
//! the same bit value where the surface is numeric — by every generator
//! surface. **Adding a new flag (e.g. `TA_FUNC_FLG_START_DEPENDENT`) fails
//! `cargo test` here until ALL surfaces learn it**; the failure message lists
//! exactly what to update. A surface knowing a name the header doesn't define
//! (the `TA_OPTIN_IS_ADVANCED` drift this gate was born from) fails the
//! name-equality assert.

#[cfg(test)]
mod tests {
    use std::collections::BTreeMap;
    use std::path::Path;

    /// Reverse direction: every key a surface carries must still exist in the
    /// header (catches stale entries after a flag rename/removal). The XML
    /// tables enumerate the full key vocabulary, so driving the match-based
    /// surfaces from them covers all five.
    #[test]
    fn no_stale_surface_keys() {
        let func = header_flags("TA_FUNC_FLG_");
        for (key, _) in crate::backends::func_api_xml::FUNC_FLAGS {
            let c = crate::backends::ta_abstract_c::func_flag_to_c(key)
                .unwrap_or_else(|| panic!("ta_abstract_c does not know func flag `{key}`"));
            assert!(func.contains_key(c), "stale func flag `{key}` -> `{c}` not in header");
            assert_ne!(crate::backends::rust_abstract::func_flag_bits(&one(key)), 0, "stale `{key}` in rust_abstract");
            assert_ne!(crate::backends::java_shipped::func_flags_value(&one(key)), 0, "stale `{key}` in java_shipped");
        }
        let opt = header_flags("TA_OPTIN_");
        for (key, _) in crate::backends::func_api_xml::OPT_INPUT_FLAGS {
            let c = crate::backends::ta_abstract_c::opt_flag_to_c(key)
                .unwrap_or_else(|| panic!("ta_abstract_c does not know opt flag `{key}`"));
            assert!(opt.contains_key(c), "stale opt flag `{key}` -> `{c}` not in header");
            assert_ne!(crate::backends::rust_abstract::opt_flag_bits(&one(key)), 0, "stale `{key}` in rust_abstract");
            assert_ne!(crate::backends::java_shipped::opt_input_flags_value(&one(key)), 0, "stale `{key}` in java_shipped");
        }
        let out = header_flags("TA_OUT_");
        for (key, _) in crate::backends::func_api_xml::OUTPUT_FLAGS {
            let c = crate::backends::ta_abstract_c::output_flag_to_c(key)
                .unwrap_or_else(|| panic!("ta_abstract_c does not know output flag `{key}`"));
            assert!(out.contains_key(c), "stale output flag `{key}` -> `{c}` not in header");
            assert_ne!(crate::backends::rust_abstract::output_flag_bits(&one(key)), 0, "stale `{key}` in rust_abstract");
            assert_ne!(crate::backends::java_shipped::output_flags_value(&one(key)), 0, "stale `{key}` in java_shipped");
        }
    }

    /// Header C-name → YAML flag key, including the historical irregular
    /// spellings. A NEW header constant with no entry here fails loudly with
    /// instructions (this map is test-only bookkeeping, not a sixth surface —
    /// it carries no bit values).
    fn yaml_key(c_name: &str) -> Option<&'static str> {
        Some(match c_name {
            "TA_FUNC_FLG_OVERLAP" => "overlap",
            "TA_FUNC_FLG_STREAM" => "stream",
            "TA_FUNC_FLG_VOLUME" => "volume",
            "TA_FUNC_FLG_UNST_PER" => "unstable_period",
            "TA_FUNC_FLG_CANDLESTICK" => "candlestick",
            // Landed on dev via #127 while this branch was in flight; the
            // entry is inert until the merged header carries the constant.
            "TA_FUNC_FLG_START_DEP" => "start_dependent",
            "TA_OPTIN_IS_PERCENT" => "percent",
            "TA_OPTIN_IS_DEGREE" => "degree",
            "TA_OPTIN_IS_CURRENCY" => "currency",
            "TA_OPTIN_ADVANCED" => "advanced",
            "TA_OUT_LINE" => "line",
            "TA_OUT_DOT_LINE" => "dot_line",
            "TA_OUT_DASH_LINE" => "dash_line",
            "TA_OUT_DOT" => "dot",
            "TA_OUT_HISTO" => "histogram",
            "TA_OUT_PATTERN_BOOL" => "pattern_bool",
            "TA_OUT_PATTERN_BULL_BEAR" => "pattern_bull_bear",
            "TA_OUT_PATTERN_STRENGTH" => "pattern_strength",
            "TA_OUT_POSITIVE" => "positive",
            "TA_OUT_NEGATIVE" => "negative",
            "TA_OUT_ZERO" => "zero",
            "TA_OUT_UPPER_LIMIT" => "upper_limit",
            "TA_OUT_LOWER_LIMIT" => "lower_limit",
            "TA_OUT_NULLABLE" => "nullable",
            _ => return None,
        })
    }

    /// Parse `#define TA_<prefix>… 0x…` constants out of include/ta_abstract.h.
    fn header_flags(prefix: &str) -> BTreeMap<String, u32> {
        let header = Path::new(env!("CARGO_MANIFEST_DIR")).join("../../include/ta_abstract.h");
        let text = std::fs::read_to_string(&header)
            .unwrap_or_else(|e| panic!("cannot read {}: {e}", header.display()));
        let mut out = BTreeMap::new();
        for line in text.lines() {
            let mut it = line.split_whitespace();
            if it.next() != Some("#define") {
                continue;
            }
            let (Some(name), Some(val)) = (it.next(), it.next()) else {
                continue;
            };
            if !name.starts_with(prefix) {
                continue;
            }
            // Accept `0x…` and `(0x…)`; anything else (decimal, shifts,
            // multi-token expressions) fails LOUDLY — a silently skipped
            // define is exactly the coverage hole this gate exists to close.
            let bare = val.trim_start_matches('(').trim_end_matches(')');
            let parsed = bare
                .strip_prefix("0x")
                .and_then(|hex| u32::from_str_radix(hex, 16).ok());
            let Some(v) = parsed else {
                panic!(
                    "unparseable flag define `{name}` (value token `{val}`) in {}:                      use a plain hex literal, or teach flag_sync::header_flags the new form",
                    header.display()
                );
            };
            out.insert(name.to_string(), v);
        }
        assert!(
            !out.is_empty(),
            "no {prefix}* constants parsed from {} — header moved?",
            header.display()
        );
        out
    }

    fn one(key: &str) -> Vec<String> {
        vec![key.to_string()]
    }

    #[test]
    fn function_flags_in_sync_across_all_surfaces() {
        for (c_name, bits) in header_flags("TA_FUNC_FLG_") {
            let key = yaml_key(&c_name).unwrap_or_else(|| {
                panic!(
                    "new function flag `{c_name}` in include/ta_abstract.h: add its YAML \
                     key to flag_sync::yaml_key AND to ALL surfaces (ta_abstract_c::\
                     func_flag_to_c, rust_abstract::func_flag_bits, java_shipped::\
                     func_flags_value, func_api_xml::FUNC_FLAGS)"
                )
            });
            assert_eq!(
                crate::backends::ta_abstract_c::func_flag_to_c(key),
                Some(c_name.as_str()),
                "ta_abstract_c::func_flag_to_c out of sync for `{key}`"
            );
            assert_eq!(
                crate::backends::rust_abstract::func_flag_bits(&one(key)),
                bits,
                "rust_abstract::func_flag_bits out of sync for `{key}` (also feeds java_abstract)"
            );
            assert_eq!(
                crate::backends::java_shipped::func_flags_value(&one(key)),
                bits,
                "java_shipped::func_flags_value out of sync for `{key}`"
            );
            assert!(
                crate::backends::func_api_xml::FUNC_FLAGS.iter().any(|(k, _)| *k == key),
                "func_api_xml::FUNC_FLAGS missing `{key}`"
            );
        }
    }

    #[test]
    fn opt_input_flags_in_sync_across_all_surfaces() {
        for (c_name, bits) in header_flags("TA_OPTIN_") {
            let key = yaml_key(&c_name).unwrap_or_else(|| {
                panic!(
                    "new opt-input flag `{c_name}` in include/ta_abstract.h: add its YAML \
                     key to flag_sync::yaml_key AND to ALL surfaces (ta_abstract_c::\
                     opt_flag_to_c, rust_abstract::opt_flag_bits, java_shipped::\
                     opt_input_flags_value, func_api_xml::OPT_INPUT_FLAGS)"
                )
            });
            assert_eq!(
                crate::backends::ta_abstract_c::opt_flag_to_c(key),
                Some(c_name.as_str()),
                "ta_abstract_c::opt_flag_to_c out of sync for `{key}`"
            );
            assert_eq!(
                crate::backends::rust_abstract::opt_flag_bits(&one(key)),
                bits,
                "rust_abstract::opt_flag_bits out of sync for `{key}` (also feeds java_abstract)"
            );
            assert_eq!(
                crate::backends::java_shipped::opt_input_flags_value(&one(key)),
                bits,
                "java_shipped::opt_input_flags_value out of sync for `{key}`"
            );
            assert!(
                crate::backends::func_api_xml::OPT_INPUT_FLAGS.iter().any(|(k, _)| *k == key),
                "func_api_xml::OPT_INPUT_FLAGS missing `{key}`"
            );
        }
    }

    #[test]
    fn output_flags_in_sync_across_all_surfaces() {
        for (c_name, bits) in header_flags("TA_OUT_") {
            let key = yaml_key(&c_name).unwrap_or_else(|| {
                panic!(
                    "new output flag `{c_name}` in include/ta_abstract.h: add its YAML \
                     key to flag_sync::yaml_key AND to ALL surfaces (ta_abstract_c::\
                     output_flag_to_c, rust_abstract::output_flag_bits, java_shipped::\
                     output_flags_value, func_api_xml::OUTPUT_FLAGS)"
                )
            });
            assert_eq!(
                crate::backends::ta_abstract_c::output_flag_to_c(key),
                Some(c_name.as_str()),
                "ta_abstract_c::output_flag_to_c out of sync for `{key}`"
            );
            assert_eq!(
                crate::backends::rust_abstract::output_flag_bits(&one(key)),
                bits,
                "rust_abstract::output_flag_bits out of sync for `{key}` (also feeds java_abstract)"
            );
            assert_eq!(
                crate::backends::java_shipped::output_flags_value(&one(key)),
                bits,
                "java_shipped::output_flags_value out of sync for `{key}`"
            );
            assert!(
                crate::backends::func_api_xml::OUTPUT_FLAGS.iter().any(|(k, _)| *k == key),
                "func_api_xml::OUTPUT_FLAGS missing `{key}`"
            );
        }
    }
}
