//! The YAML facts a documentation renderer injects at render time.
//!
//! `docs/ta_codegen_input_doc.md` splits the two halves of a function's documentation:
//! prose lives in `<name>.md` and **numbers live only in `<name>.yaml`**, injected when
//! the page is rendered. This module is the injection half — the single place that
//! resolves an [`OptInput`] into presentable facts, so the two render targets
//! ([`super::rust_doc`] and [`super::docs_site`]) can never disagree about a default
//! or a range. Each target assembles the pieces into its own markup.
//!
//! Two resolutions are the reason this is not a formatting one-liner:
//!
//! - **Sentinel bounds.** `range: [TA_REAL_MIN, TA_REAL_MAX]` reaches the IR as
//!   `TA_REAL_MIN`/`TA_REAL_MAX` (±3e37), which must read as "unbounded" rather
//!   than as `-1.7976931348623157e308`. A magnitude test, not an identity test:
//!   the C backend maps the same tokens to ±3e37, so a bound may arrive as either.
//! - **Enum defaults.** `default: 1` on an `enum:MAType` parameter is a *value*, not
//!   an index — APO/PPO/PVO default to `1 = EMA`. Always look the variant up by value.
//!
//! Two YAML fields are deliberately **not** surfaced. `suggested` is an optimizer sweep
//! hint rather than a constraint, and it does not survive contact with a reader: 8 of the
//! 120 parameters carry a degenerate `[0, 0, 0]`, and two (`ADOSC.optInFastPeriod`,
//! `SAREXT.optInOffsetOnReverse`) have a default lying outside their own sweep, so
//! rendering it as "typical" would be wrong on the page. `precision` is a UI hint for a
//! spin-box widget, not a property of the value.

use crate::ir::{EnumDef, OptInput, ParamType};
use std::collections::HashMap;

/// Above this magnitude a bound is one of the `TA_REAL_MIN`/`TA_REAL_MAX` sentinels
/// (±3e37) rather than a real constraint. The largest genuine bound is `100000`.
const SENTINEL_MAGNITUDE: f64 = 1e15;

/// The admissible range of a parameter, with the sentinel bounds resolved away.
#[derive(Debug, Clone, PartialEq, Eq)]
pub enum RangeMeta {
    /// Both bounds are real constraints, e.g. `1` to `100000`.
    Bounded(String, String),
    /// Only the lower bound constrains, e.g. `range: [0, TA_REAL_MAX]`.
    Min(String),
    /// Only the upper bound constrains.
    Max(String),
    /// Neither bound constrains — either both are sentinels (`optInNbDevUp`) or the
    /// YAML declares no range at all (every `enum:MAType` parameter).
    Unbounded,
}

impl RangeMeta {
    /// True when there is nothing to say about the range.
    #[must_use]
    pub fn is_unbounded(&self) -> bool {
        matches!(self, RangeMeta::Unbounded)
    }
}

/// One optional input's YAML facts, resolved and formatted for prose but not yet
/// wrapped in any target's markup.
#[derive(Debug, Clone)]
pub struct ParamMeta {
    /// `integer`, `real`, or the enum's own name (`MAType`).
    pub type_label: String,
    /// The default value, formatted for prose (`30`, `2`, `0.02`). For an enum this
    /// is the underlying integer; [`ParamMeta::default_variant`] carries the name.
    pub default: Option<String>,
    /// The admissible range.
    pub range: RangeMeta,
    /// For an enum parameter, every admissible `(value, short_name)`; empty otherwise.
    pub values: Vec<(i32, String)>,
    /// For an enum parameter, the short name of the default variant (`SMA`, `EMA`).
    pub default_variant: Option<String>,
}

/// Resolve one optional input's YAML metadata.
///
/// An enum whose type is absent from `enums.yaml` degrades to no values and no
/// default variant rather than panicking — a page missing a fact still renders.
#[allow(clippy::implicit_hasher)]
#[must_use]
pub fn param_meta(opt: &OptInput, enums: &HashMap<String, EnumDef>) -> ParamMeta {
    let mut values = Vec::new();
    let mut default_variant = None;

    let type_label = match &opt.param_type {
        ParamType::Integer => "integer".to_string(),
        ParamType::Real => "real".to_string(),
        ParamType::Enum(name) => {
            if let Some(def) = enums.get(name) {
                #[allow(clippy::cast_possible_truncation)]
                let default = opt.default.unwrap_or(0.0) as i32;
                for v in &def.variants {
                    values.push((v.value, v.short_name.clone()));
                    if v.value == default {
                        default_variant = Some(v.short_name.clone());
                    }
                }
            }
            name.clone()
        }
        // A price bundle is never an optional input; name it rather than assert.
        ParamType::Price(_) => "price".to_string(),
    };

    let integral = matches!(opt.param_type, ParamType::Integer | ParamType::Enum(_));
    let fmt = |v: f64| {
        if integral {
            #[allow(clippy::cast_possible_truncation)]
            let i = v as i64;
            i.to_string()
        } else {
            fmt_real(v)
        }
    };

    let range = match opt.range {
        None => RangeMeta::Unbounded,
        Some((lo, hi)) => match (
            lo.abs() < SENTINEL_MAGNITUDE,
            hi.abs() < SENTINEL_MAGNITUDE,
        ) {
            (true, true) => RangeMeta::Bounded(fmt(lo), fmt(hi)),
            (true, false) => RangeMeta::Min(fmt(lo)),
            (false, true) => RangeMeta::Max(fmt(hi)),
            (false, false) => RangeMeta::Unbounded,
        },
    };

    ParamMeta {
        type_label,
        default: opt.default.map(fmt),
        range,
        values,
        default_variant,
    }
}

/// Format an f64 for prose: a whole number without a decimal point (`2`), anything
/// else verbatim (`0.02`). Real-valued defaults are written as bare integers in the
/// YAML (`default: 2` on `optInNbDevUp`), so this is not merely cosmetic.
#[must_use]
pub fn fmt_real(v: f64) -> String {
    if is_integral(v) {
        format!("{v:.0}")
    } else {
        format!("{v}")
    }
}

/// True when `v` is an exactly-representable integer we can print without a fraction.
#[must_use]
pub fn is_integral(v: f64) -> bool {
    (v - v.trunc()).abs() < f64::EPSILON && v.abs() < 1e15
}

/// Give a description a terminal period unless it already ends in sentence
/// punctuation — the authored prose is ragged (`ULTOSC` ends one of three parameter
/// hints with a period and the others without).
#[must_use]
pub fn ensure_period(s: &str) -> String {
    let t = s.trim_end();
    if t.is_empty() || t.ends_with(['.', '!', '?', ')']) {
        t.to_string()
    } else {
        format!("{t}.")
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    use crate::ir::EnumVariant;

    fn opt(name: &str, pt: ParamType, range: Option<(f64, f64)>, default: f64) -> OptInput {
        OptInput {
            name: name.to_string(),
            param_type: pt,
            range,
            default: Some(default),
            display_name: None,
            hint: None,
            flags: vec![],
            suggested: None,
            precision: None,
        }
    }

    fn ma_type() -> HashMap<String, EnumDef> {
        let variants = [("SMA", 0), ("EMA", 1), ("WMA", 2)]
            .iter()
            .map(|(n, v)| EnumVariant {
                c_name: format!("TA_MAType_{n}"),
                pascal_name: (*n).to_string(),
                short_name: (*n).to_string(),
                value: *v,
            })
            .collect();
        let mut m = HashMap::new();
        m.insert(
            "MAType".to_string(),
            EnumDef {
                name: "MAType".to_string(),
                variants,
            },
        );
        m
    }

    #[test]
    fn integer_range_is_bounded() {
        let m = param_meta(
            &opt(
                "optInTimePeriod",
                ParamType::Integer,
                Some((1.0, 100_000.0)),
                30.0,
            ),
            &HashMap::new(),
        );
        assert_eq!(m.type_label, "integer");
        assert_eq!(m.default.as_deref(), Some("30"));
        assert_eq!(
            m.range,
            RangeMeta::Bounded("1".to_string(), "100000".to_string())
        );
    }

    #[test]
    fn sentinel_bounds_resolve_to_unbounded() {
        // BBANDS optInNbDevUp: `range: [TA_REAL_MIN, TA_REAL_MAX]`, `default: 2`.
        let m = param_meta(
            &opt("optInNbDevUp", ParamType::Real, Some((crate::backends::common::TA_REAL_MIN, crate::backends::common::TA_REAL_MAX)), 2.0),
            &HashMap::new(),
        );
        assert_eq!(m.default.as_deref(), Some("2"));
        assert_eq!(m.range, RangeMeta::Unbounded);
    }

    #[test]
    fn half_bounded_range_keeps_the_real_bound() {
        // SAR optInAcceleration: `range: [0, TA_REAL_MAX]`, `default: 0.02`.
        let m = param_meta(
            &opt(
                "optInAcceleration",
                ParamType::Real,
                Some((0.0, crate::backends::common::TA_REAL_MAX)),
                0.02,
            ),
            &HashMap::new(),
        );
        assert_eq!(m.default.as_deref(), Some("0.02"));
        assert_eq!(m.range, RangeMeta::Min("0".to_string()));
    }

    #[test]
    fn c_backend_sentinel_magnitude_also_resolves() {
        // The C mapping uses ±3e37 for the same tokens.
        let m = param_meta(
            &opt("optInX", ParamType::Real, Some((0.0, 3e37)), 1.0),
            &HashMap::new(),
        );
        assert_eq!(m.range, RangeMeta::Min("0".to_string()));
    }

    #[test]
    fn enum_default_is_looked_up_by_value_not_index() {
        // APO/PPO/PVO default to 1 = EMA, not to the first variant.
        let m = param_meta(
            &opt("optInMAType", ParamType::Enum("MAType".to_string()), None, 1.0),
            &ma_type(),
        );
        assert_eq!(m.type_label, "MAType");
        assert_eq!(m.default_variant.as_deref(), Some("EMA"));
        assert_eq!(m.default.as_deref(), Some("1"));
        assert_eq!(m.values.len(), 3);
        assert!(m.range.is_unbounded());
    }

    #[test]
    fn unknown_enum_degrades_instead_of_panicking() {
        let m = param_meta(
            &opt(
                "optInMAType",
                ParamType::Enum("Missing".to_string()),
                None,
                0.0,
            ),
            &HashMap::new(),
        );
        assert!(m.values.is_empty());
        assert!(m.default_variant.is_none());
    }

    #[test]
    fn real_formatting() {
        assert_eq!(fmt_real(2.0), "2");
        assert_eq!(fmt_real(0.02), "0.02");
        assert_eq!(fmt_real(0.3), "0.3");
    }

    #[test]
    fn period_is_added_only_when_missing() {
        assert_eq!(ensure_period("Time period"), "Time period.");
        assert_eq!(ensure_period("Time period."), "Time period.");
        assert_eq!(ensure_period("Usually 3 (see below)"), "Usually 3 (see below)");
        assert_eq!(ensure_period(""), "");
    }
}
