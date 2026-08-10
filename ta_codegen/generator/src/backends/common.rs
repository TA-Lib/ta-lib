//! Pure IR-inspection helpers shared by the language backends.
//!
//! These walk the backend-agnostic [`Statement`]/[`Expr`] AST and contain no
//! language-specific output, so every backend shares one copy instead of
//! re-implementing them. New backends import from here rather than copy-pasting.

use crate::ir::{BinOp, EnumDef, EnumVariant, Expr, Statement};
use std::collections::HashMap;

// TA-Lib's parameter sentinels, mirroring include/ta_defs.h. The "use the
// default" value deliberately sits one step OUTSIDE the legal range so
// `x == DEFAULT` can never collide with real data — hence -4e37 vs -3e37, and
// i32::MIN vs i32::MIN+1. DBL_MIN/DBL_MAX cannot express that scheme (there is
// no representable double outside them to spare), which is why these are fixed
// decimals. Values are pinned by the ta_defs_sentinels_match test below.
pub const TA_REAL_MIN: f64 = -3e37;
pub const TA_REAL_MAX: f64 = 3e37;
pub const TA_REAL_DEFAULT: f64 = -4e37;
pub const TA_INTEGER_MIN: i32 = i32::MIN + 1;
pub const TA_INTEGER_MAX: i32 = i32::MAX;
pub const TA_INTEGER_DEFAULT: i32 = i32::MIN;

/// Render a real range bound: the `REAL_MIN`/`REAL_MAX` sentinels by name, anything
/// else as a literal. `prefix` is the backend's namespace (`"TA_"` for C, empty for
/// Rust and Java, whose crate and package already namespace them) — so the generated
/// sources name the constant instead of repeating ±3e37.
#[must_use]
#[allow(clippy::float_cmp)] // these are exact sentinel values, not measurements
pub fn real_bound_literal(v: f64, prefix: &str) -> String {
    if v == TA_REAL_MIN {
        format!("{prefix}REAL_MIN")
    } else if v == TA_REAL_MAX {
        format!("{prefix}REAL_MAX")
    } else {
        format!("{v:e}")
    }
}

/// The name every enum uses for its "let the function choose" member (#182).
pub(crate) const ENUM_DEFAULT_VARIANT: &str = "DEFAULT";

/// The enum's `DEFAULT` member, when its type declares one.
///
/// A parameter typed by such an enum accepts two spellings of "use the documented
/// default": the generic `TA_INTEGER_DEFAULT` sentinel every integer parameter
/// takes, and this member. Both resolve to the parameter's own `default:`, which
/// differs per parameter — so every backend's validation prologue substitutes it
/// before the body runs, and no switch arm ever sees it.
#[must_use]
pub(crate) fn enum_default_variant<'a>(
    enums: &'a HashMap<String, EnumDef>,
    enum_name: &str,
) -> Option<&'a EnumVariant> {
    enums
        .get(enum_name)?
        .variants
        .iter()
        .find(|v| v.name == ENUM_DEFAULT_VARIANT)
}

/// Prose for a `MAType` member that does not name a moving average.
///
/// `DISABLED` (#93) and `DEFAULT` (#182) sit in the same list as SMA and EMA but
/// select a behaviour rather than an average, so every surface that describes the
/// members one by one would otherwise call them moving averages. Two entries, in
/// one place, rather than a `doc:` field on a schema whose members are otherwise
/// pure name-and-value: nothing here can go stale silently, because a member with
/// no entry gets the "moving average" wording that is right for every real one.
#[must_use]
pub(crate) fn ma_pseudo_member_doc(variant: &str) -> Option<&'static str> {
    match variant {
        "DISABLED" => Some("Not a moving average: the input is copied through unchanged."),
        ENUM_DEFAULT_VARIANT => Some(
            "Not a moving average: selects the documented default of whichever \
             parameter it is passed to.",
        ),
        _ => None,
    }
}

/// The range the validation prologue should enforce for an optional parameter:
/// its declared `range:`, or — for an `enum:` parameter, which declares none —
/// the span of its members.
#[must_use]
#[allow(clippy::implicit_hasher)]
pub fn effective_range(
    opt: &crate::ir::OptInput,
    enums: &HashMap<String, EnumDef>,
) -> Option<(f64, f64)> {
    if let Some(r) = opt.range {
        return Some(r);
    }
    match &opt.param_type {
        crate::ir::ParamType::Enum(name) => {
            enum_value_bounds(enums, name).map(|(lo, hi)| (f64::from(lo), f64::from(hi)))
        }
        _ => None,
    }
}

/// The inclusive value range an `enum:` parameter accepts: `0..=max member`.
///
/// A choice list declares no `range:` in YAML, so until now nothing rejected a
/// value outside it and each body decided for itself — which is how a lookback
/// came to answer a usable number for parameters its own function rejects. The
/// prologue is where the two tiers agree by construction: one emitter, two
/// failure literals.
///
/// Derived from the members, so an appended one widens the gate automatically.
#[must_use]
pub(crate) fn enum_value_bounds(
    enums: &HashMap<String, EnumDef>,
    enum_name: &str,
) -> Option<(i32, i32)> {
    let e = enums.get(enum_name)?;
    let lo = e.variants.iter().map(|v| v.value).min()?;
    let hi = e.variants.iter().map(|v| v.value).max()?;
    // A span only describes the member set while the values are contiguous, and
    // the prologue gate this feeds is `< lo || > hi` -- a reserved slot would sit
    // inside the span and reach the body's dispatch again, re-arming exactly the
    // defect the gate exists to close. MAType is asserted contiguous by three
    // emitters, but each of those names it explicitly; this helper is generic, so
    // it checks here rather than inheriting an invariant keyed to another type.
    let n = i32::try_from(e.variants.len()).unwrap_or(i32::MAX);
    assert!(
        hi - lo + 1 == n,
        "enum {enum_name} has a gap in its values ({lo}..={hi} over {n} members): the \
         validation prologue gates on the span, which would admit the missing value. \
         Emit a membership test instead of widening this."
    );
    Some((lo, hi))
}

/// `Type.MEMBER` for `value`, when the enum declares a member with that value.
///
/// Java and C# spell a qualified enum member identically, so both render from
/// here. They differ only in the fallback for a value no member carries: C# can
/// cast, Java cannot — hence `Option` rather than a shared default.
#[must_use]
pub(crate) fn enum_member_literal(
    enums: &HashMap<String, EnumDef>,
    enum_name: &str,
    value: i32,
) -> Option<String> {
    let v = enums.get(enum_name)?.variants.iter().find(|v| v.value == value)?;
    Some(format!("{enum_name}.{}", v.name))
}

/// The candlestick helper functions (`ta_candlerange`, `ta_candleaverage`) whose
/// calls are unpacked/hoisted before the surrounding expression is rendered.
pub const CANDLE_FNS: &[&str] = &["ta_candlerange", "ta_candleaverage"];

/// True if `expr` directly contains a call to a candlestick helper, stopping the
/// walk at logical `&&`/`||` operators (which begin a separate condition in the
/// chain).
// The `And|Or => false` arm is intentionally separate from the catch-all: it must
// precede the general `BinOp` arm to stop the walk at logical operators.
#[allow(clippy::match_same_arms)]
pub fn expr_directly_contains_candle_call(expr: &Expr) -> bool {
    match expr {
        Expr::FuncCall(name, args) => {
            CANDLE_FNS.contains(&name.as_str())
                || args.iter().any(expr_directly_contains_candle_call)
        }
        // Stop at logical operators — those are separate conditions in the chain
        Expr::BinOp(_, BinOp::And | BinOp::Or, _) => false,
        Expr::BinOp(l, _, r) => {
            expr_directly_contains_candle_call(l) || expr_directly_contains_candle_call(r)
        }
        Expr::Ternary(c, t, e) => {
            expr_directly_contains_candle_call(c)
                || expr_directly_contains_candle_call(t)
                || expr_directly_contains_candle_call(e)
        }
        Expr::Cast(_, inner)
        | Expr::Not(inner)
        | Expr::BitwiseNot(inner)
        | Expr::AddressOf(inner)
        | Expr::PostIncrement(inner)
        | Expr::PostDecrement(inner)
        | Expr::PreIncrement(inner)
        | Expr::PreDecrement(inner) => expr_directly_contains_candle_call(inner),
        Expr::ArrayAccess(_, idx) => expr_directly_contains_candle_call(idx),
        Expr::Var(_) | Expr::Literal(_) | Expr::IntLiteral(_) | Expr::PointerDeref(_) => false,
    }
}

/// PascalCase a single word: lowercase it, then upper-case the first character
/// (`"SMA"` → `"Sma"`, `"rsi"` → `"Rsi"`). For multi-segment `snake_case` names,
/// backends use their own underscore-splitting `pascal_words` instead.
pub fn pascal_word(s: &str) -> String {
    let lower = s.to_lowercase();
    let mut chars = lower.chars();
    match chars.next() {
        None => String::new(),
        Some(c) => c.to_uppercase().collect::<String>() + chars.as_str(),
    }
}

/// True if any statement is `return ALLOC_ERR;`.
pub fn contains_alloc_err_return(stmts: &[Statement]) -> bool {
    // "Err(RetCode::AllocErr)" is the Rust stream tier's pre-mapped form of the
    // same return (`map_return_code` runs before rendering); no batch IR ever
    // carries it, so recognizing it is batch-invariant.
    stmts.iter().any(|s| matches!(s, Statement::Return { value: Some(Expr::Var(name)) }
        if name == "ALLOC_ERR" || name == "Err(RetCode::AllocErr)"))
}

/// If `expr` is (or recursively contains) a `sizeof(TYPE)`, return the type name.
pub fn find_sizeof_type(expr: &Expr) -> Option<String> {
    match expr {
        Expr::FuncCall(name, args) if name == "sizeof" => args.first().and_then(|a| match a {
            Expr::Var(type_name) => Some(type_name.clone()),
            _ => None,
        }),
        Expr::BinOp(left, _, right) => find_sizeof_type(left).or_else(|| find_sizeof_type(right)),
        Expr::Cast(_, inner) => find_sizeof_type(inner),
        _ => None,
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    /// The sentinels above are a copy of `include/ta_defs.h`, which is hand-written
    /// public ABI. Nothing else notices when the two drift: a wrong value silently
    /// widens or narrows every generated range check (that is exactly how
    /// `TA_REAL_MAX` came to be emitted as `f64::MAX`). Pin them to the header.
    #[test]
    #[allow(clippy::float_cmp)] // pinning exact literals is the point
    fn ta_defs_sentinels_match() {
        let hdr = std::fs::read_to_string(
            std::path::Path::new(env!("CARGO_MANIFEST_DIR")).join("../../include/ta_defs.h"),
        )
        .expect("include/ta_defs.h");
        for (macro_name, expected) in [
            ("TA_REAL_MIN", "(-3e+37)"),
            ("TA_REAL_MAX", "(3e+37)"),
            ("TA_REAL_DEFAULT", "(-4e+37)"),
            ("TA_INTEGER_MIN", "(INT_MIN+1)"),
            ("TA_INTEGER_MAX", "(INT_MAX)"),
            ("TA_INTEGER_DEFAULT", "(INT_MIN)"),
        ] {
            let needle = format!("#define {macro_name} ");
            let line = hdr
                .lines()
                .find(|l| l.starts_with(&needle))
                .unwrap_or_else(|| panic!("{macro_name} not found in ta_defs.h"));
            assert_eq!(
                line[needle.len()..].trim(),
                expected,
                "{macro_name} changed in ta_defs.h; update backends::common to match"
            );
        }
        // And that our Rust values are those literals.
        assert_eq!(TA_REAL_MIN, -3e37);
        assert_eq!(TA_REAL_MAX, 3e37);
        assert_eq!(TA_REAL_DEFAULT, -4e37);
        assert_eq!(TA_INTEGER_MIN, i32::MIN + 1);
        assert_eq!(TA_INTEGER_MAX, i32::MAX);
        assert_eq!(TA_INTEGER_DEFAULT, i32::MIN);
    }
}
