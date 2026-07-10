//! Generate `abstract_.rs` — the Rust function metadata registry (introspection layer).
//!
//! This is the Rust analog of C's `ta_abstract` (`TA_GetFuncInfo`,
//! `TA_Get*ParameterInfo`, `TA_ForEachFunc`). Instead of mirroring C's runtime
//! (linear strcmp name scan, opaque `void* dataSet`, heap-allocated string tables,
//! fn-pointer callbacks), it emits a **zero-cost, link-time-const registry**:
//! everything lives in `&'static`/`const` tables, the opaque dataSet becomes a
//! type-safe `OptDomain` enum, `FuncId` is a fieldless enum that doubles as the
//! dense index, enumeration is an iterator, and name lookup is a generated `match`.
//!
//! Reads only the IR (the same `FuncDef`/`EnumDef` `func_api_xml.rs` consumes), so
//! it is purely "another render target" over data already parsed.
#![allow(clippy::cast_possible_truncation, clippy::cast_sign_loss)]

use crate::ir::{EnumDef, FuncDef, Input, OptInput, Output, ParamType};
use super::common::ta_real_sentinel;
use std::collections::HashMap;
use std::fmt::Write as _;
use std::path::Path;

/// Generate `ta_codegen/output/rust/src/abstract_.rs` from the function defs.
///
/// Sorts alphabetically by name (so `FuncId` discriminants and the name `match`
/// are deterministic), then emits the registry, and writes only if changed.
#[allow(clippy::implicit_hasher)]
pub fn generate(funcs: &[FuncDef], enums: &HashMap<String, EnumDef>, out_base: &Path) {
    let mut sorted: Vec<&FuncDef> = funcs.iter().collect();
    sorted.sort_by(|a, b| a.name.cmp(&b.name));
    let n = sorted.len();

    let mut o = String::new();
    o.push_str(HEADER);

    // --- FuncId enum (fieldless; doubles as the dense index into FUNCS) ---
    o.push_str("#[derive(Debug, Clone, Copy, PartialEq, Eq, Hash, PartialOrd, Ord)]\n");
    o.push_str("#[repr(u16)]\n#[allow(non_camel_case_types)]\npub enum FuncId {\n");
    for f in &sorted {
        let _ = writeln!(o, "    {},", pascal_ident(&f.name));
    }
    o.push_str("}\n\n");

    let _ = writeln!(o, "impl FuncId {{");
    let _ = writeln!(o, "    /// Number of functions in the registry.");
    let _ = writeln!(o, "    pub const COUNT: usize = {n};");
    o.push_str("    /// Metadata for this function (O(1) index into the const table).\n");
    o.push_str("    #[inline] pub fn info(self) -> &'static FuncInfo { &FUNCS[self as usize] }\n");
    o.push_str("    /// Upper-case TA name, e.g. \"RSI\".\n");
    o.push_str("    #[inline] pub fn name(self) -> &'static str { FUNCS[self as usize].name }\n");
    o.push_str("}\n\n");

    // --- model types (fixed) ---
    o.push_str(MODEL);

    // --- the one flat master table ---
    let _ = writeln!(o, "/// All function metadata, indexed by [`FuncId`]. Link-time const, in `.rodata`.");
    let _ = writeln!(o, "pub static FUNCS: [FuncInfo; {n}] = [");
    for f in &sorted {
        emit_func(&mut o, f, enums);
    }
    o.push_str("];\n\n");

    // --- API surface (C-recognizable names, idiomatic shapes) ---
    emit_api(&mut o, &sorted);

    // --- TA_FunctionDescriptionXML analog (embeds the XML data file below) ---
    o.push_str(XML_FN);

    // --- committed regression tests for the registry's structural invariants ---
    o.push_str(REGISTRY_TESTS);

    // Write the XML data file embedded by function_description_xml() via include_str!.
    // Byte-identical to the repo-root ta_func_api.xml (same generator + same input),
    // which C's TA_FunctionDescriptionXML also bakes — so the two are equal.
    let xml = super::func_api_xml::generate_string(funcs);
    let xml_path = out_base.join("rust/src/ta_func_api.xml");
    super::write_if_changed(&xml_path, &xml, "ta_func_api.xml (rust embed)", n);

    let out_path = out_base.join("rust/src/abstract_api.rs");
    super::write_if_changed(&out_path, &o, "abstract_api.rs", n);
}

fn emit_func(o: &mut String, f: &FuncDef, enums: &HashMap<String, EnumDef>) {
    o.push_str("    FuncInfo {\n");
    let _ = writeln!(o, "        id: FuncId::{},", pascal_ident(&f.name));
    let _ = writeln!(o, "        name: {:?},", f.name);
    let camel = f.camel_case.as_deref().unwrap_or(&f.name);
    let _ = writeln!(o, "        camel_case_name: {camel:?},");
    let _ = writeln!(o, "        group: Group::{},", group_variant(&f.group));
    let hint = f.hint.as_deref().unwrap_or("");
    let _ = writeln!(o, "        hint: {hint:?},");
    let _ = writeln!(o, "        flags: FuncFlags({:#010x}),", func_flag_bits(&f.flags));

    o.push_str("        inputs: ");
    emit_inputs(o, &f.inputs);
    o.push_str(",\n");

    o.push_str("        opt_inputs: &[");
    for opt in &f.optional_inputs {
        emit_opt(o, opt, enums);
    }
    o.push_str("],\n");

    o.push_str("        outputs: &[");
    for out in &f.outputs {
        emit_output(o, out);
    }
    o.push_str("],\n");

    match unst_variant(&f.name) {
        Some(v) => {
            let _ = writeln!(o, "        unst_id: Some(FuncUnstId::{v}),");
        }
        None => o.push_str("        unst_id: None,\n"),
    }
    o.push_str("    },\n");
}

/// Emit the `inputs:` slice. The YAML parser expands a price input into individual
/// `inHigh`/`inLow`/`inClose`/... `Real` inputs; here we RE-COLLAPSE a contiguous run
/// of price-component inputs back into a single `InputType::Price` with an OHLCV flag
/// bitmask and the canonical `inPriceXXX` name, matching C's `ta_abstract`.
fn emit_inputs(o: &mut String, inputs: &[Input]) {
    o.push('&');
    o.push('[');
    let mut price: Vec<&str> = Vec::new();
    for inp in inputs {
        match &inp.param_type {
            ParamType::Price(components) => {
                flush_price(o, &mut price);
                let comps: Vec<&str> = components.iter().map(String::as_str).collect();
                let (name, bits) = canonical_price(&comps);
                let _ = write!(
                    o,
                    "InputInfo {{ param_name: {name:?}, kind: InputType::Price, flags: InputFlags({bits:#010x}) }}, "
                );
            }
            ParamType::Real => {
                if let Some(comp) = price_component_of(&inp.name) {
                    price.push(comp);
                } else {
                    flush_price(o, &mut price);
                    let _ = write!(
                        o,
                        "InputInfo {{ param_name: {:?}, kind: InputType::Real, flags: InputFlags(0) }}, ",
                        inp.name
                    );
                }
            }
            ParamType::Integer => {
                flush_price(o, &mut price);
                let _ = write!(
                    o,
                    "InputInfo {{ param_name: {:?}, kind: InputType::Integer, flags: InputFlags(0) }}, ",
                    inp.name
                );
            }
            ParamType::Enum(_) => {}
        }
    }
    flush_price(o, &mut price);
    o.push(']');
}

/// Price components in canonical OHLCV order: (yaml key, name letter, C flag bit).
const PRICE_ORDER: &[(&str, char, u32)] = &[
    ("open", 'O', 0x0000_0001),
    ("high", 'H', 0x0000_0002),
    ("low", 'L', 0x0000_0004),
    ("close", 'C', 0x0000_0008),
    ("volume", 'V', 0x0000_0010),
    ("openinterest", 'I', 0x0000_0020),
    ("timestamp", 'T', 0x0000_0040),
];

/// The price component an expanded input name refers to, if any.
fn price_component_of(input_name: &str) -> Option<&'static str> {
    match input_name {
        "inOpen" => Some("open"),
        "inHigh" => Some("high"),
        "inLow" => Some("low"),
        "inClose" => Some("close"),
        "inVolume" => Some("volume"),
        "inOpenInterest" => Some("openinterest"),
        "inTimestamp" => Some("timestamp"),
        _ => None,
    }
}

/// Canonical `inPriceXXX` name + OHLCV flag bits for a set of components,
/// reproducing C's `ta_abstract` price-input names (HLC, HLCV, OHLC, ...).
fn canonical_price(components: &[&str]) -> (String, u32) {
    let mut suffix = String::new();
    let mut bits = 0u32;
    for (key, letter, bit) in PRICE_ORDER {
        if components.contains(key) {
            suffix.push(*letter);
            bits |= *bit;
        }
    }
    (format!("inPrice{suffix}"), bits)
}

/// Flush a pending run of price components as one `Price` input.
fn flush_price(o: &mut String, price: &mut Vec<&str>) {
    if price.is_empty() {
        return;
    }
    let (name, bits) = canonical_price(price);
    let _ = write!(
        o,
        "InputInfo {{ param_name: {name:?}, kind: InputType::Price, flags: InputFlags({bits:#010x}) }}, "
    );
    price.clear();
}

fn emit_output(o: &mut String, out: &Output) {
    let kind = if out.param_type == ParamType::Integer { "Integer" } else { "Real" };
    let _ = write!(
        o,
        "OutputInfo {{ param_name: {:?}, kind: OutputType::{kind}, flags: OutputFlags({:#010x}) }}, ",
        out.name,
        output_flag_bits(&out.flags)
    );
}

fn emit_opt(o: &mut String, opt: &OptInput, enums: &HashMap<String, EnumDef>) {
    let display = opt.display_name.as_deref().unwrap_or(&opt.name);
    let hint = opt.hint.as_deref().unwrap_or("");
    let _ = write!(
        o,
        "OptInputInfo {{ param_name: {:?}, display_name: {:?}, hint: {:?}, flags: OptInputFlags({:#010x}), domain: ",
        opt.name,
        display,
        hint,
        opt_flag_bits(&opt.flags)
    );
    match &opt.param_type {
        ParamType::Real => emit_real_domain(o, opt),
        ParamType::Integer => emit_int_domain(o, opt),
        ParamType::Enum(name) => emit_enum_domain(o, opt, enums.get(name)),
        ParamType::Price(_) => {
            // Not expected for optional inputs; emit a harmless empty real range.
            o.push_str("OptDomain::RealRange { min: 0.0, max: 0.0, precision: 0, default: 0.0, suggested: (0.0, 0.0, 0.0) }");
        }
    }
    o.push_str(" }, ");
}

fn emit_real_domain(o: &mut String, opt: &OptInput) {
    let (min, max) = opt.range.unwrap_or((0.0, 0.0));
    let prec = opt.precision.unwrap_or(0);
    let def = ta_real_sentinel(opt.default.unwrap_or(0.0));
    let (s, e, i) = opt.suggested.unwrap_or((0.0, 0.0, 0.0));
    let _ = write!(
        o,
        "OptDomain::RealRange {{ min: {}, max: {}, precision: {}, default: {}, suggested: ({}, {}, {}) }}",
        fl(ta_real_sentinel(min)),
        fl(ta_real_sentinel(max)),
        prec,
        fl(def),
        fl(ta_real_sentinel(s)),
        fl(ta_real_sentinel(e)),
        fl(ta_real_sentinel(i)),
    );
}

fn emit_int_domain(o: &mut String, opt: &OptInput) {
    let (min, max) = opt.range.unwrap_or((0.0, 0.0));
    let (min, max) = (min as i32, max as i32);
    let def = opt.default.unwrap_or(0.0) as i32;
    // Integer suggested values: prefer the explicit YAML `suggested` hints
    // (matching ta_def_ui.c). Only when they are absent do we fall back to max
    // for all three — func_api_xml applies that legacy fallback unconditionally.
    let (s, e, i) = match opt.suggested {
        Some((a, b, c)) => (a as i32, b as i32, c as i32),
        None => (max, max, max),
    };
    let _ = write!(
        o,
        "OptDomain::IntegerRange {{ min: {min}, max: {max}, default: {def}, suggested: ({s}, {e}, {i}) }}"
    );
}

fn emit_enum_domain(o: &mut String, opt: &OptInput, enum_def: Option<&EnumDef>) {
    let def = opt.default.unwrap_or(0.0) as i64;
    o.push_str("OptDomain::IntegerList { values: &[");
    if let Some(ed) = enum_def {
        for v in &ed.variants {
            let _ = write!(o, "({}, {:?}), ", i64::from(v.value), v.short_name);
        }
    }
    let _ = write!(o, "], default: {def} }}");
}

fn emit_api(o: &mut String, sorted: &[&FuncDef]) {
    o.push_str(
        "/// Resolve a function name (e.g. \"RSI\") to its [`FuncId`].\n\
         ///\n\
         /// Uses a generated `match` — see the module-level docs for why this is O(1)\n\
         /// and faster than C's linear `strcmp` scan, with zero allocation/dependencies.\n",
    );
    o.push_str("pub fn get_func_handle(name: &str) -> Option<FuncId> {\n    Some(match name {\n");
    for f in sorted {
        let _ = writeln!(o, "        {:?} => FuncId::{},", f.name, pascal_ident(&f.name));
    }
    o.push_str("        _ => return None,\n    })\n}\n\n");

    o.push_str("/// C-style variant returning the familiar `RetCode` error channel.\n");
    o.push_str("pub fn get_func_handle_rc(name: &str) -> Result<FuncId, crate::RetCode> {\n");
    o.push_str("    get_func_handle(name).ok_or(crate::RetCode::BadParam)\n}\n\n");

    o.push_str("/// Function metadata for a handle (infallible — `FuncId` cannot be invalid).\n");
    o.push_str("#[inline] pub fn get_func_info(handle: FuncId) -> &'static FuncInfo { handle.info() }\n");
    o.push_str("/// Required-input metadata by index (`None` if out of range).\n");
    o.push_str("#[inline] pub fn get_input_parameter_info(handle: FuncId, index: usize) -> Option<&'static InputInfo> { handle.info().inputs.get(index) }\n");
    o.push_str("/// Optional-input metadata by index (`None` if out of range).\n");
    o.push_str("#[inline] pub fn get_opt_input_parameter_info(handle: FuncId, index: usize) -> Option<&'static OptInputInfo> { handle.info().opt_inputs.get(index) }\n");
    o.push_str("/// Output metadata by index (`None` if out of range).\n");
    o.push_str("#[inline] pub fn get_output_parameter_info(handle: FuncId, index: usize) -> Option<&'static OutputInfo> { handle.info().outputs.get(index) }\n\n");

    o.push_str("/// Iterate metadata for every function (idiomatic replacement for C's `TA_ForEachFunc`).\n");
    o.push_str("#[inline] pub fn funcs() -> impl Iterator<Item = &'static FuncInfo> + Clone { FUNCS.iter() }\n");
    o.push_str("/// Iterate functions in a given group.\n");
    o.push_str("#[inline] pub fn funcs_in_group(group: Group) -> impl Iterator<Item = &'static FuncInfo> + Clone { FUNCS.iter().filter(move |f| f.group == group) }\n");
    o.push_str("/// C-style enumeration wrapper, for porters who prefer the callback shape.\n");
    o.push_str("pub fn for_each_func<F: FnMut(&'static FuncInfo)>(mut f: F) { for fi in FUNCS.iter() { f(fi); } }\n\n");
    o.push_str("/// All function groups (no allocation, unlike C's `TA_GroupTableAlloc`).\n");
    o.push_str("#[inline] pub fn groups() -> &'static [Group] { Group::ALL }\n");
}

// --- name → identifier / variant helpers ---

/// Convert a TA name (`HT_DCPERIOD`, `CDL2CROWS`, `T3`) into a valid, unique
/// UpperCamelCase Rust identifier for a `FuncId` variant.
fn pascal_ident(name: &str) -> String {
    name.split('_')
        .map(|seg| {
            let lower = seg.to_lowercase();
            let mut chars = lower.chars();
            match chars.next() {
                Some(c) => c.to_uppercase().collect::<String>() + chars.as_str(),
                None => String::new(),
            }
        })
        .collect()
}

/// Map a group string (from YAML) to its `Group` enum variant identifier.
/// Panics on an unknown group so a new/typo'd group fails codegen loudly.
fn group_variant(g: &str) -> &'static str {
    match g {
        "Cycle Indicators" => "CycleIndicators",
        "Math Operators" => "MathOperators",
        "Math Transform" => "MathTransform",
        "Momentum Indicators" => "MomentumIndicators",
        "Overlap Studies" => "OverlapStudies",
        "Pattern Recognition" => "PatternRecognition",
        "Price Transform" => "PriceTransform",
        "Statistic Functions" => "StatisticFunctions",
        "Volatility Indicators" => "VolatilityIndicators",
        "Volume Indicators" => "VolumeIndicators",
        other => panic!("rust_abstract: unknown group '{other}'"),
    }
}

/// Map an unstable-period function name to its `FuncUnstId` variant (the one
/// metadata that IS a stable public contract, kept byte-for-byte aligned to C).
fn unst_variant(name: &str) -> Option<&'static str> {
    Some(match name {
        "ADX" => "Adx",
        "ADXR" => "Adxr",
        "ATR" => "Atr",
        "CMO" => "Cmo",
        "DX" => "Dx",
        "EMA" => "Ema",
        "HT_DCPERIOD" => "HtDcPeriod",
        "HT_DCPHASE" => "HtDcPhase",
        "HT_PHASOR" => "HtPhasor",
        "HT_SINE" => "HtSine",
        "HT_TRENDLINE" => "HtTrendline",
        "HT_TRENDMODE" => "HtTrendMode",
        "KAMA" => "Kama",
        "MAMA" => "Mama",
        "MINUS_DI" => "MinusDI",
        "MINUS_DM" => "MinusDM",
        "NATR" => "Natr",
        "PLUS_DI" => "PlusDI",
        "PLUS_DM" => "PlusDM",
        "RSI" => "Rsi",
        "STOCHRSI" => "StochRsi",
        "T3" => "T3",
        _ => return None,
    })
}

// --- flag bitmask helpers (exact C values from include/ta_abstract.h) ---

fn func_flag_bits(flags: &[String]) -> u32 {
    let mut b = 0u32;
    for f in flags {
        match f.as_str() {
            "overlap" => b |= 0x0100_0000,
            "stream" => b |= 0x0200_0000, // TA_FUNC_FLG_STREAM
            "volume" => b |= 0x0400_0000,
            "unstable_period" => b |= 0x0800_0000,
            "candlestick" => b |= 0x1000_0000,
            _ => {}
        }
    }
    b
}

fn opt_flag_bits(flags: &[String]) -> u32 {
    let mut b = 0u32;
    for f in flags {
        match f.as_str() {
            "percent" => b |= 0x0010_0000,
            "degree" => b |= 0x0020_0000,
            "currency" => b |= 0x0040_0000,
            "advanced" => b |= 0x0100_0000,
            _ => {}
        }
    }
    b
}

fn output_flag_bits(flags: &[String]) -> u32 {
    let mut b = 0u32;
    for f in flags {
        match f.as_str() {
            "line" => b |= 0x0000_0001,
            "dot_line" => b |= 0x0000_0002,
            "dash_line" => b |= 0x0000_0004,
            "dot" => b |= 0x0000_0008,
            "histogram" => b |= 0x0000_0010,
            "pattern_bool" => b |= 0x0000_0020,
            "pattern_bull_bear" => b |= 0x0000_0040,
            "pattern_strength" => b |= 0x0000_0080,
            "positive" => b |= 0x0000_0100,
            "negative" => b |= 0x0000_0200,
            "zero" => b |= 0x0000_0400,
            "upper_limit" => b |= 0x0000_0800,
            "lower_limit" => b |= 0x0000_1000,
            _ => {}
        }
    }
    b
}

/// Format an f64 as a valid Rust literal (Debug yields e.g. `2.0`, `0.1`, `3e37`).
fn fl(v: f64) -> String {
    format!("{v:?}")
}

/// `function_description_xml()` — the Rust analog of C's `TA_FunctionDescriptionXML()`.
/// Embeds the generated `ta_func_api.xml` data file at compile time.
const XML_FN: &str = r#"
/// Rust analog of C's `TA_FunctionDescriptionXML()` — the full machine-readable XML
/// description of every function. Byte-identical to the generated `ta_func_api.xml`
/// (embedded via `include_str!`), which C's `TA_FunctionDescriptionXML` also bakes.
pub fn function_description_xml() -> &'static str {
    include_str!("ta_func_api.xml")
}
"#;

/// Generated `#[cfg(test)]` invariant tests for the registry. These guard the
/// crate's public introspection API and the internal consistency of the tables
/// (FuncId<->index<->name, group coverage, param-index bounds, negative paths) —
/// the surface the metadata-parity RPC test does NOT exercise. Run with
/// `cargo test` in the generated crate.
const REGISTRY_TESTS: &str = r#"
#[cfg(test)]
mod registry_tests {
    use super::*;

    #[test]
    fn count_matches_table_and_indices_align() {
        assert_eq!(FuncId::COUNT, FUNCS.len());
        for (i, f) in FUNCS.iter().enumerate() {
            assert_eq!(f.id as usize, i, "FuncId discriminant must equal its FUNCS index");
        }
    }

    #[test]
    fn name_handle_roundtrip() {
        for f in FUNCS.iter() {
            assert_eq!(get_func_handle(f.name), Some(f.id), "handle lookup for {}", f.name);
            assert_eq!(f.id.name(), f.name);
            assert_eq!(get_func_info(f.id).name, f.name);
        }
    }

    #[test]
    fn unknown_name_is_none() {
        assert_eq!(get_func_handle("definitely_not_a_ta_func"), None);
        assert!(get_func_handle_rc("definitely_not_a_ta_func").is_err());
    }

    #[test]
    fn groups_cover_every_func() {
        assert_eq!(groups(), Group::ALL);
        for f in FUNCS.iter() {
            assert!(Group::ALL.contains(&f.group), "{} group not in Group::ALL", f.name);
            assert!(!f.group.as_str().is_empty());
        }
    }

    #[test]
    fn funcs_in_group_partitions_all() {
        let total: usize = Group::ALL.iter().map(|g| funcs_in_group(*g).count()).sum();
        assert_eq!(total, FuncId::COUNT);
        assert_eq!(funcs().count(), FuncId::COUNT);
    }

    #[test]
    fn param_index_bounds() {
        for f in FUNCS.iter() {
            assert!(get_input_parameter_info(f.id, f.nb_input()).is_none());
            assert!(get_opt_input_parameter_info(f.id, f.nb_opt_input()).is_none());
            assert!(get_output_parameter_info(f.id, f.nb_output()).is_none());
            if f.nb_input() > 0 { assert!(get_input_parameter_info(f.id, 0).is_some()); }
            if f.nb_output() > 0 { assert!(get_output_parameter_info(f.id, 0).is_some()); }
        }
    }

    #[test]
    fn for_each_func_visits_all() {
        let mut n = 0;
        for_each_func(|_| n += 1);
        assert_eq!(n, FuncId::COUNT);
    }

    #[test]
    fn function_description_xml_is_sane() {
        let xml = function_description_xml();
        assert!(xml.starts_with("<?xml"));
        assert!(xml.contains("<FinancialFunctions>"));
        assert!(xml.len() > 500);
    }
}
"#;

const HEADER: &str = r"//! TA-Lib function metadata registry — the Rust abstract / introspection layer.
//!
//! GENERATED by ta_codegen (`backends/rust_abstract.rs`) — do not edit by hand.
//!
//! Rust analog of C's `ta_abstract` (`TA_GetFuncInfo`, `TA_Get*ParameterInfo`,
//! `TA_ForEachFunc`), implemented as a **zero-cost, link-time-const registry**:
//!   * all metadata is `&'static`/`const` in `.rodata` — zero heap, zero runtime init;
//!   * the opaque C `void* dataSet` + type tag becomes a type-safe [`OptDomain`] enum
//!     (illegal states unrepresentable, no unchecked cast);
//!   * [`FuncId`] is a fieldless enum that doubles as the dense index into [`FUNCS`]
//!     (no opaque handle, no magic-number validity check);
//!   * enumeration returns an iterator instead of C's fn-pointer + `void*` callback;
//!   * no heap allocation and no `*Alloc`/`*Free` pairs anywhere.
//!
//! Public names mirror C (`get_func_info`, `get_*_parameter_info`, `for_each_func`,
//! `groups`) so C porters are at home; the shapes are idiomatic Rust.
//!
//! ## Why name lookup uses a generated `match` (and not a hash map / `phf` / strcmp)
//!  * Lookup is a **cold path** — function discovery at setup, never per-bar compute.
//!  * rustc/LLVM lower a 161-arm `&str` `match` to a length-bucketed + leading-byte
//!    dispatch (not a comparison chain): effectively O(1), entirely in `.rodata`,
//!    zero allocation, zero dependencies.
//!  * For reference, C's `TA_GetFuncHandle` is an O(n) linear `strcmp` within a
//!    26-way first-letter bucket (up to 67 compares for the `CDL*` bucket) plus
//!    several pointer hops per entry. The generated `match` is strictly less work.
#![allow(clippy::all)]
#![allow(non_camel_case_types)]

use crate::FuncUnstId;

";

const MODEL: &str = r#"/// Function group (closed set — replaces C's runtime group-string table + linear `getGroupId`).
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
#[repr(u8)]
pub enum Group {
    CycleIndicators,
    MathOperators,
    MathTransform,
    MomentumIndicators,
    OverlapStudies,
    PatternRecognition,
    PriceTransform,
    StatisticFunctions,
    VolatilityIndicators,
    VolumeIndicators,
}

impl Group {
    /// All groups, in canonical order.
    pub const ALL: &'static [Group] = &[
        Group::CycleIndicators,
        Group::MathOperators,
        Group::MathTransform,
        Group::MomentumIndicators,
        Group::OverlapStudies,
        Group::PatternRecognition,
        Group::PriceTransform,
        Group::StatisticFunctions,
        Group::VolatilityIndicators,
        Group::VolumeIndicators,
    ];
    /// Canonical display string (matches C's `TA_GroupString` / the YAML `group`).
    pub const fn as_str(self) -> &'static str {
        match self {
            Group::CycleIndicators => "Cycle Indicators",
            Group::MathOperators => "Math Operators",
            Group::MathTransform => "Math Transform",
            Group::MomentumIndicators => "Momentum Indicators",
            Group::OverlapStudies => "Overlap Studies",
            Group::PatternRecognition => "Pattern Recognition",
            Group::PriceTransform => "Price Transform",
            Group::StatisticFunctions => "Statistic Functions",
            Group::VolatilityIndicators => "Volatility Indicators",
            Group::VolumeIndicators => "Volume Indicators",
        }
    }
}

/// Required-input data kind (C: `TA_Input_Price`/`Real`/`Integer`).
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
#[repr(u8)]
pub enum InputType { Price, Real, Integer }

/// Output data kind (C: `TA_Output_Real`/`Integer`).
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
#[repr(u8)]
pub enum OutputType { Real, Integer }

macro_rules! flag_newtype {
    ($name:ident { $($cn:ident = $cv:expr),* $(,)? }) => {
        #[derive(Debug, Clone, Copy, PartialEq, Eq)]
        pub struct $name(pub u32);
        impl $name {
            $(pub const $cn: Self = Self($cv);)*
            /// Raw bits.
            #[inline] pub const fn bits(self) -> u32 { self.0 }
            /// True if all bits of `other` are set.
            #[inline] pub const fn contains(self, other: Self) -> bool { self.0 & other.0 == other.0 }
        }
    };
}

flag_newtype!(FuncFlags {
    OVERLAP = 0x0100_0000,
    STREAM = 0x0200_0000,
    VOLUME = 0x0400_0000,
    UNSTABLE_PERIOD = 0x0800_0000,
    CANDLESTICK = 0x1000_0000,
});
flag_newtype!(InputFlags {
    PRICE_OPEN = 0x0000_0001,
    PRICE_HIGH = 0x0000_0002,
    PRICE_LOW = 0x0000_0004,
    PRICE_CLOSE = 0x0000_0008,
    PRICE_VOLUME = 0x0000_0010,
    PRICE_OPENINTEREST = 0x0000_0020,
    PRICE_TIMESTAMP = 0x0000_0040,
});
flag_newtype!(OptInputFlags {
    IS_PERCENT = 0x0010_0000,
    IS_DEGREE = 0x0020_0000,
    IS_CURRENCY = 0x0040_0000,
    ADVANCED = 0x0100_0000,
});
flag_newtype!(OutputFlags {
    LINE = 0x0000_0001,
    DOT_LINE = 0x0000_0002,
    DASH_LINE = 0x0000_0004,
    DOT = 0x0000_0008,
    HISTO = 0x0000_0010,
    PATTERN_BOOL = 0x0000_0020,
    PATTERN_BULL_BEAR = 0x0000_0040,
    PATTERN_STRENGTH = 0x0000_0080,
    POSITIVE = 0x0000_0100,
    NEGATIVE = 0x0000_0200,
    ZERO = 0x0000_0400,
    UPPER_LIMIT = 0x0000_0800,
    LOWER_LIMIT = 0x0000_1000,
});

/// A required input parameter.
#[derive(Debug, Clone, Copy)]
pub struct InputInfo {
    pub param_name: &'static str,
    pub kind: InputType,
    pub flags: InputFlags,
}

/// An output parameter.
#[derive(Debug, Clone, Copy)]
pub struct OutputInfo {
    pub param_name: &'static str,
    pub kind: OutputType,
    pub flags: OutputFlags,
}

/// The domain of an optional input — type-safe replacement for C's `void* dataSet` + type tag.
#[derive(Debug, Clone, Copy)]
pub enum OptDomain {
    RealRange { min: f64, max: f64, precision: u8, default: f64, suggested: (f64, f64, f64) },
    IntegerRange { min: i32, max: i32, default: i32, suggested: (i32, i32, i32) },
    RealList { values: &'static [(f64, &'static str)], default: f64 },
    IntegerList { values: &'static [(i64, &'static str)], default: i64 },
}

/// An optional input parameter.
#[derive(Debug, Clone, Copy)]
pub struct OptInputInfo {
    pub param_name: &'static str,
    pub display_name: &'static str,
    pub hint: &'static str,
    pub flags: OptInputFlags,
    pub domain: OptDomain,
}

/// Metadata for one TA-Lib function (C: `TA_FuncInfo` + its parameter tables).
#[derive(Debug, Clone, Copy)]
pub struct FuncInfo {
    pub id: FuncId,
    pub name: &'static str,
    pub camel_case_name: &'static str,
    pub group: Group,
    pub hint: &'static str,
    pub flags: FuncFlags,
    pub inputs: &'static [InputInfo],
    pub opt_inputs: &'static [OptInputInfo],
    pub outputs: &'static [OutputInfo],
    /// Stable unstable-period id (the one metadata kept aligned to C); `None` if N/A.
    pub unst_id: Option<FuncUnstId>,
}

impl FuncInfo {
    #[inline] pub const fn nb_input(&self) -> usize { self.inputs.len() }
    #[inline] pub const fn nb_opt_input(&self) -> usize { self.opt_inputs.len() }
    #[inline] pub const fn nb_output(&self) -> usize { self.outputs.len() }
}

"#;
