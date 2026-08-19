//! Pure IR-inspection helpers shared by the language backends.
//!
//! These walk the backend-agnostic [`Statement`]/[`Expr`] AST and contain no
//! language-specific output, so every backend shares one copy instead of
//! re-implementing them. New backends import from here rather than copy-pasting.

use crate::ir::{BinOp, EnumDef, EnumVariant, Expr, FuncDef, ParamType, Statement};
use crate::registry::Lang;
use std::collections::{BTreeSet, HashMap};

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

/// The rejection condition for a real optional parameter's declared range.
///
/// Spelled `!(x >= min && x <= max)` rather than the obvious `x < min || x > max`.
/// The two are identical for every finite `x` — and every declared bound in the
/// corpus is finite, ±3e37 at the widest — but they differ on NaN, and that
/// difference is the whole point: `NaN < min` and `NaN > max` are BOTH false, so
/// the obvious spelling silently ACCEPTS a NaN parameter and computes with it.
/// The inverted form costs the same two comparisons and rejects NaN along with
/// the infinities.
///
/// One spelling everywhere — batch, lookback and streaming. The streaming tier
/// got it first (it cannot afford a NaN entering a retained handle), but there
/// was never a reason for the batch tier to accept a parameter it has no
/// meaning for; "batch does not filter" is a statement about INPUT SERIES, not
/// about parameters.
///
/// `paren` parenthesizes each comparison, which is how the Rust backend has
/// always spelled it.
#[must_use]
pub(crate) fn real_range_reject(name: &str, lo: &str, hi: &str, paren: bool) -> String {
    if paren {
        format!("!(({name} >= {lo}) && ({name} <= {hi}))")
    } else {
        format!("!({name} >= {lo} && {name} <= {hi})")
    }
}

/// Render a real range bound: the `REAL_MIN`/`REAL_MAX` sentinels by name, anything
/// else as a literal. `prefix` is the backend's namespace — `"TA_"` for C, `"Self::"`
/// for Rust (they are associated consts on `Core`, as they are static members of
/// Java's `Core`, and the emission site is inside `impl Core`), empty for Java, whose
/// package already namespaces them — so the generated sources name the constant
/// instead of repeating ±3e37.
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

/// The enum types some function declares an optional parameter with.
///
/// Those are the only enums that get generated limit constants, because they are
/// the only ones whose value crosses the API boundary as a caller-supplied
/// integer and therefore has to be range-checked. `FuncUnstId` is deliberately
/// excluded by the same rule: no parameter is typed with it, and its `ALL`
/// wildcard sits far outside the member span, so limits derived from the members
/// would describe a domain its API does not have.
#[must_use]
pub fn param_enum_names(funcs: &[FuncDef]) -> BTreeSet<String> {
    funcs
        .iter()
        .flat_map(|f| &f.optional_inputs)
        .filter_map(|opt| match &opt.param_type {
            ParamType::Enum(name) => Some(name.clone()),
            _ => None,
        })
        .collect()
}

/// How `lang` spells the generated inclusive value-limit constants of an `enum:`
/// parameter type — `(min, max)`, or `None` when that backend declares none.
///
/// This function is the single registry of both halves: *whether* a backend has
/// the constants and *what they are called*. The emitter that declares them and
/// every emitter that references them read the same answer, so a range check can
/// never name a constant the enum surface did not emit, and appending a member
/// moves the declaration and every use together.
///
/// C and C# have them because both surface the parameter as a plain integer a
/// caller can put any value in, so both generate a range check that needs a
/// bound to name. Java and Rust have neither the check nor the constants: their
/// `MAType` is a real enum, the only values reachable through the typed API are
/// members, and Rust's `TryFrom<i32>` is itself the domain gate — a second
/// spelling of the domain there would disagree with it about the
/// `TA_INTEGER_DEFAULT` sentinel, which `TryFrom` accepts. A future backend that
/// does need one adds an arm here and a declaration in its enum emitter.
#[must_use]
pub fn enum_limit_names_of(def: &EnumDef, lang: Lang) -> Option<(String, String)> {
    match lang {
        // Macros rather than enumerators: an enumerator would join the member
        // list and be an `optInMAType` a caller could pass. The prefix is
        // upper-cased -- `TA_MAType_` -> `TA_MATYPE_` -- so the macro cannot be
        // read as one of the members it bounds, which the shared prefix would
        // otherwise suggest. Uniform, not a MAType special case: an already
        // upper-case prefix like `TA_FUNC_UNST_` passes through unchanged.
        Lang::C => {
            let p = def.c_prefix.to_uppercase();
            Some((format!("{p}MIN"), format!("{p}MAX")))
        }
        // A C# enum cannot hold static members, so they live in the sibling
        // static class the `FuncUnstIds.Count` companion already established.
        Lang::CSharp => Some((format!("{}s.Min", def.name), format!("{}s.Max", def.name))),
        Lang::Java | Lang::Rust => None,
    }
}

/// [`enum_limit_names_of`] keyed by type name, for the emitters that hold the
/// map rather than the definition.
#[must_use]
pub(crate) fn enum_limit_names(
    enums: &HashMap<String, EnumDef>,
    enum_name: &str,
    lang: Lang,
) -> Option<(String, String)> {
    enum_limit_names_of(enums.get(enum_name)?, lang)
}

/// The inclusive bounds an integer-valued optional parameter's validation
/// prologue must compare against, already spelled for `lang`: a declared
/// `range:` renders as literals, an `enum:` domain names that enum's generated
/// limit constants instead of repeating their values.
///
/// # Panics
///
/// If `lang` asks for an `enum:` parameter's bounds but declares no limit
/// constants for that enum. Emitting the literal span instead would silently
/// reintroduce the per-call-site copy of the bound this exists to remove, so it
/// fails the generate rather than the review.
#[must_use]
#[allow(clippy::implicit_hasher)]
// Integer optional-param ranges are stored as `f64` in the IR; casting the
// integer-valued ones for literal emission is exact, not truncating.
#[allow(clippy::cast_possible_truncation)]
pub fn int_bound_exprs(
    opt: &crate::ir::OptInput,
    enums: &HashMap<String, EnumDef>,
    lang: Lang,
) -> Option<(String, String)> {
    if let Some((lo, hi)) = opt.range {
        return Some(((lo as i32).to_string(), (hi as i32).to_string()));
    }
    let ParamType::Enum(name) = &opt.param_type else {
        return None;
    };
    // Asserts contiguity, so a gapped enum fails here rather than admitting the
    // missing value through the span the constants describe.
    enum_value_bounds(enums, name)?;
    Some(enum_limit_names(enums, name, lang).unwrap_or_else(|| {
        panic!(
            "{lang:?} range-checks the enum parameter {} but declares no limit \
             constants for {name}: add the arm in common::enum_limit_names and the \
             declaration in that backend's enum emitter.",
            opt.name
        )
    }))
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
pub fn enum_value_bounds_of(e: &EnumDef) -> Option<(i32, i32)> {
    let enum_name = &e.name;
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

/// [`enum_value_bounds_of`] keyed by type name.
#[must_use]
pub(crate) fn enum_value_bounds(
    enums: &HashMap<String, EnumDef>,
    enum_name: &str,
) -> Option<(i32, i32)> {
    enum_value_bounds_of(enums.get(enum_name)?)
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

/// The declared inputs a function's body actually indexes.
///
/// Four candlestick patterns take an OHLC leg they never read (cdl3outside's
/// `inHigh`/`inLow`, for instance). A bounds check on such a leg would reject a
/// caller who passed a short — or, in Java, a null — array for a series the
/// algorithm ignores, while proving nothing about the accesses the body makes.
/// Detected on the comment-stripped IR, so a name mentioned only in prose
/// cannot count.
///
/// One source of truth for the two backends that bound their array accesses:
/// the Rust `assert!` preamble and the Java argument checks must agree on which
/// legs are load-bearing, or the same call is a panic in one language and a
/// success in the other.
#[must_use]
pub(crate) fn indexed_input_names(func: &FuncDef) -> BTreeSet<String> {
    let body_repr = format!("{:?}", super::stmt_walk::strip_comments(&func.body));
    func.inputs
        .iter()
        .filter(|i| body_repr.contains(&format!("\"{}\"", i.name)))
        .map(|i| i.name.clone())
        .collect()
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
