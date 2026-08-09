use std::collections::HashMap;
use std::path::Path;

use serde::Deserialize;

use crate::ir::{EnumDef, EnumVariant};

#[derive(Deserialize)]
struct YamlVariant {
    name: String,
    value: i32,
}

#[derive(Deserialize)]
struct YamlEnum {
    c_prefix: String,
    variants: Vec<YamlVariant>,
}

/// Load enum definitions from `enums.yaml`.
///
/// Returns a map from enum name (e.g. `"MAType"`) to its definition.
pub fn load_enums(path: &Path) -> HashMap<String, EnumDef> {
    let content = std::fs::read_to_string(path)
        .unwrap_or_else(|e| panic!("Failed to read {}: {}", path.display(), e));

    let raw: HashMap<String, YamlEnum> = serde_yaml::from_str(&content)
        .unwrap_or_else(|e| panic!("Failed to parse {}: {}", path.display(), e));

    raw.into_iter()
        .map(|(name, spec)| {
            let variants: Vec<EnumVariant> = spec
                .variants
                .into_iter()
                .map(|v| EnumVariant {
                    c_name: format!("{}{}", spec.c_prefix, v.name),
                    name: v.name,
                    value: v.value,
                })
                .collect();
            check_unique(&name, &variants, path);
            (
                name.clone(),
                EnumDef { name, c_prefix: spec.c_prefix, variants },
            )
        })
        .collect()
}

/// A variant's `name` and `value` are both identities: the name is the
/// identifier every backend emits, the value is the pinned ABI integer. A
/// duplicate in either is caught here, at parse time, because downstream it
/// surfaces only as a foreign-compiler error — a repeated `name` reaches javac
/// as a duplicate enum constant, and a repeated `value` reaches nothing at all
/// when the contiguity check happens to still pass.
fn check_unique(enum_name: &str, variants: &[EnumVariant], path: &Path) {
    let mut errors = Vec::new();
    for (i, a) in variants.iter().enumerate() {
        for b in &variants[i + 1..] {
            if a.name.eq_ignore_ascii_case(&b.name) {
                errors.push(format!("  duplicate name: {} and {}", a.name, b.name));
            }
            if a.value == b.value {
                errors.push(format!(
                    "  duplicate value {}: {} and {}",
                    a.value, a.name, b.name
                ));
            }
        }
    }
    assert!(
        errors.is_empty(),
        "{}: enum {enum_name} is not well formed:\n{}",
        path.display(),
        errors.join("\n")
    );
}

/// Look up an enum variant by its case label (e.g. `"MAType_SMA"`).
///
/// Returns `(enum_name, &EnumVariant)` if found, or `None` if the label
/// doesn't match any known enum variant.
#[allow(clippy::implicit_hasher)]
pub fn lookup_variant<'a>(
    label: &str,
    enums: &'a HashMap<String, EnumDef>,
) -> Option<(&'a str, &'a EnumVariant)> {
    // Labels have the format `EnumName_VARIANT`, e.g. `MAType_SMA`
    for (enum_name, enum_def) in enums {
        let prefix = format!("{enum_name}_");
        if label.starts_with(&prefix) {
            let short = &label[prefix.len()..];
            if let Some(variant) = enum_def.variants.iter().find(|v| v.name == short) {
                return Some((enum_name, variant));
            }
        }
    }
    None
}
