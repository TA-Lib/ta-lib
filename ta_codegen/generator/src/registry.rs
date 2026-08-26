use std::collections::HashMap;
use std::path::Path;

/// Target language for cross-call resolution — the same enum `PRAGMA TA_ALT`
/// resolves against, re-exported here for the many `registry::Lang` call sites.
/// One type: a second copy would be a second answer to "what are the backends".
pub use crate::ir::Lang;

/// Registry of discovered indicators, used for cross-function call resolution.
///
/// Scans `ta_codegen/input/` subdirectories to discover indicator names,
/// then translates prefix-free calls (e.g. `sma_lookback`) to
/// language-specific names (e.g. `TA_SMA_Lookback` for C).
pub struct Registry {
    /// Sorted by descending length so longest-match wins in `parse_func_name`.
    indicators: Vec<String>,
    /// Maps a lowercase indicator dir-name (`ma`, `willr`) to the YAML `name:`
    /// it declares (`MA`, `WILLR`) — the one identity every backend spells
    /// verbatim.
    names: HashMap<String, String>,
    /// Per-indicator signature facts for the streaming dispatch/composed
    /// analysis (stream flag + input/opt/output counts), from the YAML.
    callee_sigs: HashMap<String, crate::streaming::CalleeSig>,
    /// Per-indicator output names in signature order (`ma` -> `["outReal"]`,
    /// `mama` -> `["outMAMA", "outFAMA"]`) — the Java stream emitter routes
    /// dispatch OutSlots through named `cur_*` fields / `Value` members.
    callee_out_names: HashMap<String, Vec<String>>,
}

impl Registry {
    /// Build a registry by scanning `base_dir` for subdirectories containing `.yaml` files.
    pub fn from_dir(base_dir: &Path) -> Self {
        let mut indicators = Vec::new();
        let mut names = HashMap::new();
        let mut callee_sigs = HashMap::new();
        let mut callee_out_names = HashMap::new();

        if let Ok(entries) = std::fs::read_dir(base_dir) {
            for entry in entries.filter_map(std::result::Result::ok) {
                let path = entry.path();
                if !path.is_dir() {
                    continue;
                }
                let dir_name = entry.file_name().to_string_lossy().to_string();
                // Same skip the loaders apply, from the same predicate: a
                // non-indicator directory must not reach `parse_yaml`, whose
                // checks are written for indicator definitions.
                if crate::parser::yaml::is_reserved_dir(&dir_name) {
                    continue;
                }
                let yaml_path = path.join(format!("{dir_name}.yaml"));
                if yaml_path.exists() {
                    let fd = crate::parser::yaml::parse_yaml(&yaml_path);
                    names.insert(dir_name.clone(), fd.name.clone());
                    callee_sigs.insert(
                        dir_name.clone(),
                        crate::streaming::callee_sig_of(&fd),
                    );
                    callee_out_names.insert(
                        dir_name.clone(),
                        fd.outputs.iter().map(|o| o.name.clone()).collect(),
                    );
                    indicators.push(dir_name);
                }
            }
        }

        // Sort by descending length so longest-match wins (e.g. "stochrsi" before "stoch")
        indicators.sort_by(|a, b| b.len().cmp(&a.len()).then(a.cmp(b)));

        Registry { indicators, names, callee_sigs, callee_out_names }
    }

    /// The output names of an indicator in signature order (empty if unknown).
    pub(crate) fn callee_outputs(&self, key: &str) -> &[String] {
        self.callee_out_names.get(key).map_or(&[][..], Vec::as_slice)
    }

    /// Which of an indicator's outputs are `nullable` — index-aligned with
    /// [`Self::callee_outputs`], empty if unknown. A cross-call may hand `NULL`
    /// to a slot only where this is true (rule B6a).
    pub(crate) fn callee_out_nullable(&self, key: &str) -> &[bool] {
        self.callee_sigs.get(key).map_or(&[][..], |s| s.out_nullable.as_slice())
    }

    /// The declared name of an indicator dir-name — what every backend spells
    /// verbatim, and what C prefixes with `TA_`.
    pub(crate) fn name_of(&self, key: &str) -> String {
        self.names.get(key).cloned().unwrap_or_else(|| key.to_uppercase())
    }

    /// Check if an indicator exists in the registry.
    pub fn contains(&self, name: &str) -> bool {
        self.indicators.iter().any(|n| n == name)
    }

    /// Resolve a prefix-free function call to a language-specific name.
    ///
    /// Given `sma_lookback` or bare `sma`, parses out the indicator name
    /// and function type, then maps to the target language's naming convention.
    ///
    /// Bare indicator names (e.g. `sma`) are cross-indicator composition calls
    /// and resolve to the guarded entry point in each language.
    ///
    /// Returns the original name unchanged if the indicator is not found.
    pub fn resolve_call(&self, func_name: &str, lang: Lang) -> String {
        // Bare indicator names resolve to the public entry point.
        //
        // This is safe structurally: a composite's lookback is *defined* as its
        // callee's lookback plus the extra history it needs, so a startIdx already
        // clamped to the composite's lookback lands exactly on the callee's.
        //
        // All four resolve it to the PUBLIC entry point, which is what C has
        // always done (`TA_MA` is C's public API, declared in ta_func.h). Java
        // (#236 step 3) and Rust (#267) bind the returned `OutRange` rather than
        // C-shaped out-params, which is what puts the callee's argument checks on
        // the composed path. C# needs no change of name at all: its two tiers are
        // overloads, and dropping the two `out int` arguments selects the public
        // one.
        if self.contains(func_name) {
            let name = self.name_of(func_name);
            return match lang {
                Lang::Rust | Lang::CSharp | Lang::Java => name,
                Lang::C => format!("TA_{name}"),
            };
        }

        let Some((indicator, suffix)) = self.parse_func_name(func_name) else {
            return func_name.to_string();
        };

        // An alternate is generator input, not a symbol: there is one `TA_MIN`,
        // and `PRAGMA TA_ALT` decides only which body it is built from. Left to
        // the generic rule below this would emit a call to `TA_MIN_ALT1`, which
        // exists in no backend — a link error instead of a generator error, and
        // in a body that is emitted for four languages while the claim may cover
        // one. Reject it at the single choke point every call passes through.
        assert!(
            !(suffix.starts_with("ALT") && suffix[3..].bytes().all(|b| b.is_ascii_digit())
                && suffix.len() > 3),
            "call to `{func_name}`: an alternate implementation is not callable. Call \
             `{indicator}(...)` — it resolves to whichever body `PRAGMA TA_ALT` selected \
             for the tier and language being generated."
        );

        // One rule for every backend: `<NAME>_<Suffix>`, and C alone prefixes
        // `TA_`.
        let base = format!("{}_{}", self.name_of(&indicator), capitalize(&suffix));
        match lang {
            Lang::C => format!("TA_{base}"),
            Lang::Rust | Lang::Java | Lang::CSharp => base,
        }
    }

    /// Parse a prefix-free function name into (indicator, suffix).
    /// e.g. "`sma_lookback`" -> ("sma", "lookback")
    fn parse_func_name(&self, func_name: &str) -> Option<(String, String)> {
        // Try matching known indicators by checking if func_name starts with
        // an indicator name followed by underscore
        for indicator in &self.indicators {
            let prefix = format!("{indicator}_");
            if func_name.starts_with(&prefix) {
                let suffix = func_name[prefix.len()..].to_string();
                return Some((indicator.clone(), suffix));
            }
        }
        None
    }

}

impl crate::streaming::CalleeLookup for Registry {
    fn callee(&self, name: &str) -> Option<crate::streaming::CalleeSig> {
        self.callee_sigs.get(name).cloned()
    }
}

/// Capitalize the first letter of a string.
fn capitalize(s: &str) -> String {
    let mut chars = s.chars();
    match chars.next() {
        None => String::new(),
        Some(c) => c.to_uppercase().collect::<String>() + chars.as_str(),
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn test_registry_discovers_indicators() {
        let base = Path::new(env!("CARGO_MANIFEST_DIR")).join("../../ta_codegen/input");
        let registry = Registry::from_dir(&base);
        assert!(registry.contains("sma"));
        assert!(registry.contains("rsi"));
        assert!(registry.contains("ema"));
    }

    #[test]
    fn test_registry_resolves_cross_calls() {
        let base = Path::new(env!("CARGO_MANIFEST_DIR")).join("../../ta_codegen/input");
        let registry = Registry::from_dir(&base);

        // One rule everywhere: `<NAME>_<Suffix>`, C alone prefixing `TA_`.
        assert_eq!(
            registry.resolve_call("sma_lookback", Lang::C),
            "TA_SMA_Lookback"
        );
        assert_eq!(registry.resolve_call("sma_lookback", Lang::Rust), "SMA_Lookback");
        assert_eq!(registry.resolve_call("sma_lookback", Lang::Java), "SMA_Lookback");
        assert_eq!(
            registry.resolve_call("sma_lookback", Lang::CSharp),
            "SMA_Lookback"
        );

        // Bare indicator names resolve to the PUBLIC entry point — the one a
        // user can reach — in every backend. Java's call sites bind the returned
        // OutRange rather than C-shaped MInteger out-params (#236 step 3), which
        // is what puts the callee's argument checks on the composed path. C#
        // needs no name change at all: its two tiers are overloads, and dropping
        // the two `out int` arguments is what selects the public one.
        assert_eq!(registry.resolve_call("ema", Lang::C), "TA_EMA");
        assert_eq!(registry.resolve_call("ema", Lang::Rust), "EMA");
        assert_eq!(registry.resolve_call("ema", Lang::Java), "EMA");
        assert_eq!(registry.resolve_call("ema", Lang::CSharp), "EMA");

        // `_private` is resolved by the same rule as any other suffix. Spelled
        // against `ema` because this is a pure name transformation and every
        // indicator is a valid subject; the construct itself is carried by the
        // SYNTH4 gate fixture (input_synth/README.md).
        assert_eq!(registry.resolve_call("ema_private", Lang::C), "TA_EMA_Private");
        assert_eq!(registry.resolve_call("ema_private", Lang::Rust), "EMA_Private");
        assert_eq!(registry.resolve_call("ema_private", Lang::Java), "EMA_Private");
        assert_eq!(
            registry.resolve_call("ema_private", Lang::CSharp),
            "EMA_Private"
        );

        // The names that used to be hand-mangled per backend are now verbatim.
        assert_eq!(registry.resolve_call("ma", Lang::Java), "MA");
        assert_eq!(registry.resolve_call("willr", Lang::Java), "WILLR");
        assert_eq!(registry.resolve_call("stochf", Lang::Java), "STOCHF");
        assert_eq!(registry.resolve_call("ma_lookback", Lang::Java), "MA_Lookback");
        assert_eq!(registry.resolve_call("willr", Lang::CSharp), "WILLR");
        assert_eq!(
            registry.resolve_call("ht_dcperiod", Lang::Rust),
            "HT_DCPERIOD"
        );

        // `stochrsi` and `stoch` are distinct identifiers in every backend —
        // longest-match parsing must not fold one into the other.
        assert_eq!(
            registry.resolve_call("stochrsi_lookback", Lang::CSharp),
            "STOCHRSI_Lookback"
        );
        assert_eq!(
            registry.resolve_call("stoch_lookback", Lang::CSharp),
            "STOCH_Lookback"
        );
    }

    #[test]
    fn test_registry_unknown_func_returns_unchanged() {
        let base = Path::new(env!("CARGO_MANIFEST_DIR")).join("../../ta_codegen/input");
        let registry = Registry::from_dir(&base);
        assert_eq!(
            registry.resolve_call("unknown_func", Lang::C),
            "unknown_func"
        );
    }

    #[test]
    fn test_registry_bare_name_resolves_to_guarded() {
        let base = Path::new(env!("CARGO_MANIFEST_DIR")).join("../../ta_codegen/input");
        let registry = Registry::from_dir(&base);
        // Bare indicator names resolve to the PUBLIC entry point, in every
        // backend, which is what C has always done (#236 step 3).
        assert_eq!(registry.resolve_call("sma", Lang::C), "TA_SMA");
        assert_eq!(registry.resolve_call("sma", Lang::Rust), "SMA");
        assert_eq!(registry.resolve_call("sma", Lang::Java), "SMA");
        assert_eq!(registry.resolve_call("sma", Lang::CSharp), "SMA");
        // `sma` declares no Private variant, but the suffix rule is uniform.
        assert_eq!(registry.resolve_call("sma_private", Lang::C), "TA_SMA_Private");
    }

    #[test]
    fn test_registry_does_not_include_non_dirs() {
        let base = Path::new(env!("CARGO_MANIFEST_DIR")).join("../../ta_codegen/input");
        let registry = Registry::from_dir(&base);
        // enums.yaml is a file at the top level, not a directory
        assert!(!registry.contains("enums"));
    }
}
