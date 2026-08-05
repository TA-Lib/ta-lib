use std::collections::HashMap;
use std::path::Path;

/// Target language for cross-call resolution.
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum Lang {
    C,
    Rust,
    Java,
    CSharp,
}

/// Registry of discovered indicators, used for cross-function call resolution.
///
/// Scans `ta_codegen/input/` subdirectories to discover indicator names,
/// then translates prefix-free calls (e.g. `sma_lookback`) to
/// language-specific names (e.g. `TA_SMA_Lookback` for C).
pub struct Registry {
    /// Sorted by descending length so longest-match wins in `parse_func_name`.
    indicators: Vec<String>,
    /// Maps a lowercase indicator dir-name (e.g. `ma`, `willr`, `stochf`) to its
    /// Java camelCase base method name (e.g. `movingAverage`, `willR`, `stochF`),
    /// derived from the YAML `camel_case` field. Captures the historical
    /// irregular/typo names so cross-indicator Java calls resolve to the same
    /// method name the backend emits for the definition.
    java_names: HashMap<String, String>,
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
        let mut java_names = HashMap::new();
        let mut callee_sigs = HashMap::new();
        let mut callee_out_names = HashMap::new();

        if let Ok(entries) = std::fs::read_dir(base_dir) {
            for entry in entries.filter_map(std::result::Result::ok) {
                let path = entry.path();
                if !path.is_dir() {
                    continue;
                }
                let dir_name = entry.file_name().to_string_lossy().to_string();
                let yaml_path = path.join(format!("{dir_name}.yaml"));
                if yaml_path.exists() {
                    // Read the `camel_case` field so Java cross-calls match the
                    // backend's method names (incl. historical irregular spellings).
                    let fd = crate::parser::yaml::parse_yaml(&yaml_path);
                    let java_name = fd
                        .camel_case
                        .clone()
                        .map_or_else(|| to_camel_case(&dir_name), |cc| lower_first(&cc));
                    java_names.insert(dir_name.clone(), java_name);
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

        Registry { indicators, java_names, callee_sigs, callee_out_names }
    }

    /// Create an empty registry (for rendering self-contained expressions that
    /// reference no indicators or helpers, e.g. the boolean-builtin predicates
    /// emitted into the JSON-RPC servers for the cross-language eval_predicate test).
    pub fn empty() -> Self {
        Registry {
            indicators: Vec::new(),
            java_names: HashMap::new(),
            callee_sigs: HashMap::new(),
            callee_out_names: HashMap::new(),
        }
    }

    /// The output names of an indicator in signature order (empty if unknown).
    pub(crate) fn callee_outputs(&self, key: &str) -> &[String] {
        self.callee_out_names.get(key).map_or(&[][..], Vec::as_slice)
    }

    /// The Java camelCase base method name for an indicator dir-name, falling back
    /// to a naive camel-case conversion if the indicator carries no `camel_case`.
    pub(crate) fn java_base(&self, key: &str) -> String {
        self.java_names
            .get(key)
            .cloned()
            .unwrap_or_else(|| to_camel_case(key))
    }

    /// The C# PascalCase base method name for an indicator dir-name: the same
    /// `camel_case` the Java name comes from, with the first letter raised. It
    /// must not be derived from the dir-name directly — that yields `Willr`
    /// where the API surface is `WillR`.
    pub(crate) fn csharp_base(&self, key: &str) -> String {
        capitalize(&self.java_base(key))
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
        // Handle _private suffix: maps to the Private variant (double-only in C,
        // generic in Rust). No double/float routing — always the same function.
        if let Some(base) = func_name.strip_suffix("_private") {
            if self.contains(base) {
                return match lang {
                    Lang::Rust => format!("{base}_private"),
                    Lang::C => format!("TA_{}_Private", base.to_uppercase()),
                    Lang::Java => format!("{}Private", self.java_base(base)),
                    Lang::CSharp => format!("{}Private", self.csharp_base(base)),
                };
            }
        }

        // Bare indicator names resolve to the guarded entry point.
        //
        // This is safe structurally: a composite's lookback is *defined* as its
        // callee's lookback plus the extra history it needs, so a startIdx already
        // clamped to the composite's lookback lands exactly on the callee's.
        //
        // Java routes to the package-private guarded core (`…Internal`), not the
        // public `…` wrapper: the callers pass the C-shaped MInteger out-params,
        // and going through the wrapper would allocate a throwaway pair per call.
        if self.contains(func_name) {
            return match lang {
                Lang::Rust => func_name.to_string(),
                Lang::C => format!("TA_{}", func_name.to_uppercase()),
                Lang::Java => format!("{}Internal", self.java_base(func_name)),
                // Managed C# shape. Unlike Java there is no `…Internal` split:
                // partial classes let the core be `internal` in the same type, so
                // cross-indicator calls target the entry point directly.
                Lang::CSharp => self.csharp_base(func_name),
            };
        }

        let Some((indicator, suffix)) = self.parse_func_name(func_name) else {
            return func_name.to_string();
        };

        match lang {
            Lang::Rust => func_name.to_string(),
            Lang::C => to_c_name(&indicator, &suffix),
            Lang::Java => format!("{}{}", self.java_base(&indicator), capitalize(&suffix)),
            Lang::CSharp => format!("{}{}", self.csharp_base(&indicator), capitalize(&suffix)),
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

/// Convert to C naming: `sma_lookback` -> `TA_SMA_Lookback`
fn to_c_name(indicator: &str, suffix: &str) -> String {
    let upper = indicator.to_uppercase();
    // Capitalize first letter of suffix
    let cap_suffix = capitalize(suffix);
    format!("TA_{upper}_{cap_suffix}")
}

/// Lowercase the first character of a `PascalCase` name: `MovingAverage` -> `movingAverage`,
/// `CdlHignWave` -> `cdlHignWave`, `WillR` -> `willR`. The rest is preserved verbatim.
fn lower_first(s: &str) -> String {
    let mut chars = s.chars();
    match chars.next() {
        None => String::new(),
        Some(c) => c.to_lowercase().collect::<String>() + chars.as_str(),
    }
}

/// Convert `snake_case` to camelCase: `sma_lookback` -> `smaLookback`
fn to_camel_case(name: &str) -> String {
    let parts: Vec<&str> = name.split('_').collect();
    let mut result = String::new();
    for (i, part) in parts.iter().enumerate() {
        if i == 0 {
            result.push_str(part);
        } else {
            result.push_str(&capitalize(part));
        }
    }
    result
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

        // C backend: lookback suffixed calls
        assert_eq!(
            registry.resolve_call("sma_lookback", Lang::C),
            "TA_SMA_Lookback"
        );

        // C backend: bare indicator names resolve to the guarded entry point
        assert_eq!(registry.resolve_call("sma", Lang::C), "TA_SMA");
        assert_eq!(registry.resolve_call("ema", Lang::C), "TA_EMA");

        // C backend: _private suffix resolves to Private (double-only)
        assert_eq!(registry.resolve_call("ema_private", Lang::C), "TA_EMA_Private");

        // Rust backend (bare names resolve to the guarded fn)
        assert_eq!(registry.resolve_call("ema", Lang::Rust), "ema");
        assert_eq!(
            registry.resolve_call("ema_lookback", Lang::Rust),
            "ema_lookback"
        );
        // Rust: _private stays as _private (generic, handles both f32/f64)
        assert_eq!(registry.resolve_call("ema_private", Lang::Rust), "ema_private");

        // Java backend: bare names resolve to the package-private *guarded* core,
        // not the public OutRange wrapper — cross-indicator callers pass the
        // C-shaped MInteger out-params straight through, so routing through the
        // wrapper would allocate a throwaway MInteger pair per call.
        assert_eq!(
            registry.resolve_call("ema_lookback", Lang::Java),
            "emaLookback"
        );
        assert_eq!(registry.resolve_call("ema", Lang::Java), "emaInternal");
        assert_eq!(registry.resolve_call("ema_private", Lang::Java), "emaPrivate");

        // Java backend: irregular/typo names come from the YAML `camel_case` field,
        // not a naive lowercase of the C name.
        assert_eq!(registry.resolve_call("ma", Lang::Java), "movingAverageInternal");
        assert_eq!(registry.resolve_call("willr", Lang::Java), "willRInternal");
        assert_eq!(registry.resolve_call("stochf", Lang::Java), "stochFInternal");
        assert_eq!(
            registry.resolve_call("ma_lookback", Lang::Java),
            "movingAverageLookback"
        );

        // C# backend: PascalCase, bare names resolve to the guarded entry point.
        assert_eq!(
            registry.resolve_call("sma_lookback", Lang::CSharp),
            "SmaLookback"
        );
        assert_eq!(registry.resolve_call("sma", Lang::CSharp), "Sma");
        assert_eq!(registry.resolve_call("ema_private", Lang::CSharp), "EmaPrivate");
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
        // Bare indicator names resolve to the guarded entry point
        assert_eq!(registry.resolve_call("sma", Lang::C), "TA_SMA");
        // _private is an orthogonal variant with its own arm.
        assert_eq!(registry.resolve_call("sma_private", Lang::C), "TA_SMA_Private");
        // For Rust, bare names resolve to the guarded fn
        assert_eq!(registry.resolve_call("sma", Lang::Rust), "sma");
        assert_eq!(registry.resolve_call("sma", Lang::Java), "smaInternal");
        assert_eq!(registry.resolve_call("sma", Lang::CSharp), "Sma");
    }

    #[test]
    fn test_registry_does_not_include_non_dirs() {
        let base = Path::new(env!("CARGO_MANIFEST_DIR")).join("../../ta_codegen/input");
        let registry = Registry::from_dir(&base);
        // enums.yaml is a file at the top level, not a directory
        assert!(!registry.contains("enums"));
    }
}
