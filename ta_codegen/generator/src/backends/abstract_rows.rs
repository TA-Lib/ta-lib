//! The backend-neutral `ta_abstract` row model.
//!
//! Every language that ships an introspection registry describes the *same*
//! facts: a function's name, group, hint, flag bits, its inputs (with price
//! components already folded into one bundle), its optional parameters and
//! their domains, its outputs, and its unstable-period id. Those facts are
//! derived here, **once**, and rendered by [`rust_abstract`](super::rust_abstract),
//! [`java_abstract`](super::java_abstract), [`java_metadata`](super::java_metadata)
//! and [`csharp_metadata`](super::csharp_metadata).
//!
//! ## Why the model is typed rather than a bag of `i32` codes
//!
//! The flat shape this replaces carried a raw `ty: i32` per parameter and a
//! field for every domain's data, `ty` selecting which fields meant anything.
//! Two renderers over that shape had already disagreed about the same code:
//! `java_abstract`'s `_ =>` arm rendered `ty == 1` (`RealList`) as an
//! `IntegerRange`, while `java_metadata` named it `REAL_LIST`. Neither was
//! caught, because no shipped function declares a real list — the disagreement
//! was latent, waiting for the first one. [`OptDomain`] makes that
//! unrepresentable: the domain *is* the data, every renderer matches
//! exhaustively, and a new domain fails compilation in each of them.
//!
//! ## What is deliberately NOT rendered from these rows
//!
//! The **C** abstract layer (`ta_abstract_c`) and **`func_api_xml`** derive their
//! values independently, and that is load-bearing rather than an oversight:
//! `test_abstract.c` proves each language server's metadata against the C
//! library's `ta_abstract` at run time. If C's tables were built from these rows
//! too, that gate would compare a generator against itself and could no longer
//! fail on a wrong row. C stays the independent oracle. (The two pieces C *does*
//! share — [`price_bundle`](super::price_bundle) for input folding, and the flag
//! vocabulary pinned by `flag_sync` against `include/ta_abstract.h` — are shared
//! precisely because an independent gate already covers them.)
//!
//! A row carries no derived name: there is one identity, `name`, and every
//! backend spells it verbatim, so there is nothing per-backend left to store or
//! to drift from the real signature.

use std::collections::HashMap;

use super::price_bundle;
use crate::ir::{EnumDef, FuncDef, Input, OptInput, Output, ParamType, PriceComponent};

// ---------------------------------------------------------------------------
// Closed vocabularies
// ---------------------------------------------------------------------------

/// A function's group. Closed set: the YAML `group:` field may name nothing else.
#[derive(Debug, Clone, Copy, PartialEq, Eq, PartialOrd, Ord)]
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
    /// All groups, in canonical (display-string alphabetical) order.
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

    /// Parse the YAML `group:` string. Panics on an unknown group so a new or
    /// typo'd one fails codegen loudly instead of reaching a renderer.
    pub fn parse(g: &str) -> Self {
        match g {
            "Cycle Indicators" => Group::CycleIndicators,
            "Math Operators" => Group::MathOperators,
            "Math Transform" => Group::MathTransform,
            "Momentum Indicators" => Group::MomentumIndicators,
            "Overlap Studies" => Group::OverlapStudies,
            "Pattern Recognition" => Group::PatternRecognition,
            "Price Transform" => Group::PriceTransform,
            "Statistic Functions" => Group::StatisticFunctions,
            "Volatility Indicators" => Group::VolatilityIndicators,
            "Volume Indicators" => Group::VolumeIndicators,
            other => panic!("abstract_rows: unknown group '{other}'"),
        }
    }

    /// The canonical display string (C's `TA_GroupString`, and the YAML `group:`).
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

    /// The identifier a generated enum variant uses (same spelling in Rust, Java
    /// and C#, so one accessor serves all three).
    pub const fn ident(self) -> &'static str {
        match self {
            Group::CycleIndicators => "CycleIndicators",
            Group::MathOperators => "MathOperators",
            Group::MathTransform => "MathTransform",
            Group::MomentumIndicators => "MomentumIndicators",
            Group::OverlapStudies => "OverlapStudies",
            Group::PatternRecognition => "PatternRecognition",
            Group::PriceTransform => "PriceTransform",
            Group::StatisticFunctions => "StatisticFunctions",
            Group::VolatilityIndicators => "VolatilityIndicators",
            Group::VolumeIndicators => "VolumeIndicators",
        }
    }
}

/// What a required input carries. Mirrors C's `TA_InputParameterType`.
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum InputKind {
    /// A bundle of OHLCV components; the row's `flags` say which.
    Price,
    Real,
    Integer,
}

impl InputKind {
    /// The C type code (`TA_Input_Price` = 0, `Real` = 1, `Integer` = 2).
    pub const fn c_code(self) -> i32 {
        match self {
            InputKind::Price => 0,
            InputKind::Real => 1,
            InputKind::Integer => 2,
        }
    }
}

/// What an output carries. Mirrors C's `TA_OutputParameterType`.
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum OutputKind {
    Real,
    Integer,
}

impl OutputKind {
    /// The C type code (`TA_Output_Real` = 0, `Integer` = 1).
    pub const fn c_code(self) -> i32 {
        match self {
            OutputKind::Real => 0,
            OutputKind::Integer => 1,
        }
    }
}

/// The domain of an optional parameter — the type-safe replacement for C's
/// `void *dataSet` plus a separate type tag.
///
/// `RealList` is declared by no shipped function. It is modelled anyway (rather
/// than left to a catch-all arm) so that the day one appears, every renderer
/// fails to compile instead of silently emitting it as an integer range.
#[derive(Debug, Clone, PartialEq)]
pub enum OptDomain {
    RealRange {
        min: f64,
        max: f64,
        precision: i32,
        default: f64,
        /// `(suggested_start, suggested_end, suggested_increment)`
        suggested: (f64, f64, f64),
    },
    RealList {
        values: Vec<(f64, String)>,
        default: f64,
    },
    IntegerRange {
        min: i32,
        max: i32,
        default: i32,
        /// `(suggested_start, suggested_end, suggested_increment)`
        suggested: (i32, i32, i32),
    },
    IntegerList {
        values: Vec<(i64, String)>,
        default: i64,
    },
}

impl OptDomain {
    /// The C type code (`RealRange` = 0, `RealList` = 1, `IntegerRange` = 2,
    /// `IntegerList` = 3).
    pub const fn c_code(&self) -> i32 {
        match self {
            OptDomain::RealRange { .. } => 0,
            OptDomain::RealList { .. } => 1,
            OptDomain::IntegerRange { .. } => 2,
            OptDomain::IntegerList { .. } => 3,
        }
    }

    /// The default value as a `double`, which is how C's
    /// `TA_OptInputParameterInfo.defaultValue` carries every domain's default.
    #[allow(clippy::cast_precision_loss)]
    pub fn default_as_f64(&self) -> f64 {
        match self {
            OptDomain::RealRange { default, .. } | OptDomain::RealList { default, .. } => *default,
            OptDomain::IntegerRange { default, .. } => f64::from(*default),
            OptDomain::IntegerList { default, .. } => *default as f64,
        }
    }

    /// C's `TA_IntegerList`/`TA_RealList` rendered as the `value=name;...` string
    /// `test_abstract.c` compares, and the shipped Java/C# registries expose.
    /// Empty for the range domains.
    pub fn value_list_string(&self) -> String {
        use std::fmt::Write as _;
        let mut s = String::new();
        match self {
            OptDomain::IntegerList { values, .. } => {
                for (i, (v, name)) in values.iter().enumerate() {
                    if i > 0 {
                        s.push(';');
                    }
                    let _ = write!(s, "{v}={name}");
                }
            }
            OptDomain::RealList { values, .. } => {
                for (i, (v, name)) in values.iter().enumerate() {
                    if i > 0 {
                        s.push(';');
                    }
                    let _ = write!(s, "{v}={name}");
                }
            }
            OptDomain::RealRange { .. } | OptDomain::IntegerRange { .. } => {}
        }
        s
    }
}

// ---------------------------------------------------------------------------
// The rows
// ---------------------------------------------------------------------------

/// One required input, with any price components already folded into a bundle.
#[derive(Debug, Clone)]
pub struct InputRow {
    pub kind: InputKind,
    pub param_name: String,
    /// For [`InputKind::Price`], the OHLCV component bitmask; otherwise 0.
    pub flags: u32,
    /// For [`InputKind::Price`], the components in the order the generated
    /// *signature* takes them — which is the declaration order, not the
    /// canonical OHLCV order [`flags`](Self::flags) is built from. Empty
    /// otherwise.
    ///
    /// Carried because a renderer that has to unfold the bundle back into
    /// argument positions (a dynamic-dispatch thunk) would otherwise re-derive
    /// it from `FuncDef` — a second fold whose agreement with this row's own
    /// slot numbering nothing checks. Both `java_metadata::dispatch_class` and
    /// the C# thunks used to do that; they read this field now.
    ///
    /// Canonical OHLCV order is irrelevant to it: this is the DECLARATION
    /// order, which is the order the generated signature takes the arrays in,
    /// and `flags` carries the canonicalised bitmask separately.
    pub signature_components: Vec<PriceComponent>,
}

/// One output.
#[derive(Debug, Clone)]
pub struct OutputRow {
    pub kind: OutputKind,
    pub param_name: String,
    pub flags: u32,
}

/// One optional parameter.
#[derive(Debug, Clone)]
pub struct OptRow {
    pub param_name: String,
    pub display_name: String,
    /// Per-parameter help text; `""` when the YAML declares none.
    pub hint: String,
    pub flags: u32,
    pub domain: OptDomain,
}

/// The unstable-period identity of a function that has one.
///
/// Derived from `enums.yaml` by name (`TA_FUNC_UNST_<NAME>`), the same
/// derivation the servers use — not a second hand-maintained name table.
#[derive(Debug, Clone)]
pub struct UnstRow {
    pub value: i32,
    pub c_name: String,
    pub name: String,
}

/// Everything the abstract layer knows about one function.
#[derive(Debug, Clone)]
pub struct FuncRow {
    pub name: String,
    pub group: Group,
    pub hint: String,
    pub flags: u32,
    pub inputs: Vec<InputRow>,
    pub opt_inputs: Vec<OptRow>,
    pub outputs: Vec<OutputRow>,
    pub unst: Option<UnstRow>,
}

/// Build every function's row, name-sorted — the order every registry enumerates in.
#[allow(clippy::implicit_hasher)]
pub fn rows(funcs: &[FuncDef], enums: &HashMap<String, EnumDef>) -> Vec<FuncRow> {
    let mut sorted: Vec<&FuncDef> = funcs.iter().collect();
    sorted.sort_by(|a, b| a.name.cmp(&b.name));
    sorted.into_iter().map(|f| row(f, enums)).collect()
}

#[allow(clippy::implicit_hasher)]
fn row(f: &FuncDef, enums: &HashMap<String, EnumDef>) -> FuncRow {
    FuncRow {
        name: f.name.clone(),
        group: Group::parse(&f.group),
        hint: f.hint.clone().unwrap_or_default(),
        flags: func_flag_bits(&f.flags),
        inputs: input_rows(&f.inputs),
        opt_inputs: f.optional_inputs.iter().map(|o| opt_row(o, enums)).collect(),
        outputs: output_rows(&f.outputs),
        unst: unst_row(&f.name, enums),
    }
}

/// Fold the parser's expanded price components back into a single `Price` input
/// with an OHLCV bitmask and the canonical `inPriceXXX` name, via the shared
/// [`price_bundle::group`] the C abstract backend also uses.
fn input_rows(inputs: &[Input]) -> Vec<InputRow> {
    let mut out = Vec::new();
    for grouped in price_bundle::group(inputs) {
        match grouped {
            price_bundle::Grouped::Price(bundle) => {
                let signature_components = price_bundle::signature_order(&bundle);
                let components = price_bundle::canonical(&signature_components);
                out.push(InputRow {
                    kind: InputKind::Price,
                    param_name: price_bundle::canonical_name(&components),
                    flags: price_bundle::flags(&components),
                    signature_components,
                });
            }
            price_bundle::Grouped::Single(inp) => match &inp.param_type {
                ParamType::Real => out.push(InputRow {
                    kind: InputKind::Real,
                    param_name: inp.name.clone(),
                    flags: 0,
                    signature_components: Vec::new(),
                }),
                // An `Enum` in *input* position is `TA_Input_Integer` to C
                // (`emit_input_descriptor`), so it must be one here too — dropping
                // it gave the managed backends a different input arity than the
                // ABI reference. Dormant: no shipped function has one (#164).
                ParamType::Integer | ParamType::Enum(_) => out.push(InputRow {
                    kind: InputKind::Integer,
                    param_name: inp.name.clone(),
                    flags: 0,
                    signature_components: Vec::new(),
                }),
                // A bare `Price` cannot survive the fold; say so rather than
                // silently shortening the input list.
                ParamType::Price(_) => panic!(
                    "input '{}' is a bare Price that price_bundle::group did not fold",
                    inp.name
                ),
            },
        }
    }
    out
}

fn output_rows(outputs: &[Output]) -> Vec<OutputRow> {
    outputs
        .iter()
        .map(|out| OutputRow {
            kind: if out.param_type == ParamType::Integer {
                OutputKind::Integer
            } else {
                OutputKind::Real
            },
            param_name: out.name.clone(),
            flags: output_flag_bits(&out.flags),
        })
        .collect()
}

/// Decimal places of a suggested increment — the fallback C's `get_precision`
/// applies when the YAML declares no explicit `precision`.
#[allow(clippy::cast_possible_truncation, clippy::cast_possible_wrap)]
fn precision_of(increment: f64) -> i32 {
    let s = format!("{increment}");
    match s.split_once('.') {
        Some(_) => {
            let trimmed = s.trim_end_matches('0');
            trimmed.find('.').map_or(0, |d| (trimmed.len() - d - 1) as i32)
        }
        None => 0,
    }
}

#[allow(clippy::implicit_hasher, clippy::cast_possible_truncation)]
fn opt_row(opt: &OptInput, enums: &HashMap<String, EnumDef>) -> OptRow {
    let domain = match &opt.param_type {
        ParamType::Real => {
            // The absent-YAML fallbacks must AGREE with the C emitter's
            // (`emit_custom_real_opt_input`, `get_precision`) or the managed
            // backends would describe a slot differently than the ABI reference
            // the moment a field is left out. They are restated here rather than
            // shared, so C keeps its own derivation and the metadata gate can
            // still fail (#164). Every shipped real declares its range and
            // suggested triple, and the 7 that omit `precision` have increment 0,
            // which both rules map to 0 — so nothing observable moves today.
            let (min, max) = opt.range.unwrap_or((0.0, f64::MAX));
            OptDomain::RealRange {
                min,
                max,
                precision: opt
                    .precision
                    .unwrap_or_else(|| opt.suggested.map_or(2, |(_, _, inc)| precision_of(inc))),
                default: opt.default.unwrap_or(0.0),
                suggested: opt.suggested.unwrap_or((0.0, 0.0, 0.0)),
            }
        }
        ParamType::Integer => {
            // Same reconciliation, against `emit_custom_int_opt_input`.
            let (min, max) = opt.range.unwrap_or((1.0, 100_000.0));
            let (min, max) = (min as i32, max as i32);
            let suggested = match opt.suggested {
                Some((a, b, c)) => (a as i32, b as i32, c as i32),
                None => (min, max.min(200), 1),
            };
            OptDomain::IntegerRange {
                min,
                max,
                default: opt.default.unwrap_or(0.0) as i32,
                suggested,
            }
        }
        ParamType::Enum(name) => OptDomain::IntegerList {
            values: enums.get(name).map_or_else(Vec::new, |ed| {
                ed.variants
                    .iter()
                    .map(|v| (i64::from(v.value), v.name.clone()))
                    .collect()
            }),
            default: opt.default.unwrap_or(0.0) as i64,
        },
        // Not expected for an optional input; a harmless empty real range.
        ParamType::Price(_) => OptDomain::RealRange {
            min: 0.0,
            max: 0.0,
            precision: 0,
            default: 0.0,
            suggested: (0.0, 0.0, 0.0),
        },
    };
    OptRow {
        param_name: opt.name.clone(),
        display_name: opt.display_name.clone().unwrap_or_else(|| opt.name.clone()),
        hint: opt.hint.clone().unwrap_or_default(),
        flags: opt_flag_bits(&opt.flags),
        domain,
    }
}

/// Resolve a function's unstable-period identity from `enums.yaml`.
///
/// `expect`, not `?`: an absent enum would hand every function `None`, quietly
/// emitting registries with no unstable-period wiring at all.
#[allow(clippy::implicit_hasher)]
fn unst_row(name: &str, enums: &HashMap<String, EnumDef>) -> Option<UnstRow> {
    let target = format!("TA_FUNC_UNST_{name}");
    enums
        .get("FuncUnstId")
        .expect("FuncUnstId enum missing from enums.yaml")
        .variants
        .iter()
        .find(|v| v.c_name == target)
        .map(|v| UnstRow {
            value: v.value,
            c_name: v.c_name.clone(),
            name: v.name.clone(),
        })
}

// ---------------------------------------------------------------------------
// Flag bitmasks (exact C values from include/ta_abstract.h; pinned by flag_sync)
// ---------------------------------------------------------------------------

pub fn func_flag_bits(flags: &[String]) -> u32 {
    let mut b = 0u32;
    for f in flags {
        match f.as_str() {
            "overlap" => b |= 0x0100_0000,
            "stream" => b |= 0x0200_0000, // TA_FUNC_FLG_STREAM
            "volume" => b |= 0x0400_0000,
            "unstable_period" => b |= 0x0800_0000,
            "candlestick" => b |= 0x1000_0000,
            "path_dependent" => b |= 0x2000_0000, // TA_FUNC_FLG_PATH_DEP
            _ => {}
        }
    }
    b
}

pub fn opt_flag_bits(flags: &[String]) -> u32 {
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

pub fn output_flag_bits(flags: &[String]) -> u32 {
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
            "nullable" => b |= 0x0000_2000,
            _ => {}
        }
    }
    b
}
