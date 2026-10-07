//! Typed taxonomy of builtin function calls shared by the language backends.
//!
//! Indicator source calls a handful of `<math.h>` functions (`sqrt`, `sin`,
//! `max`, …) by name as `Expr::FuncCall`. [`MathFn`] is the one classifier for
//! those names — never a per-backend `&[&str]` list, which is three copies free
//! to drift — so a name is recognised as a math builtin in exactly one place and
//! the compiler enforces that every backend handles each variant.
//!
//! Name aliases that render identically within every backend collapse into one
//! variant: `max`/`fmax` → [`MathFn::Max`], `min`/`fmin` → [`MathFn::Min`],
//! `fabs`/`ABS` → [`MathFn::Abs`]. Each backend starts from [`MathFn::canonical`]
//! and applies its own small remap.

use crate::ir::{BinOp, Expr};

/// A `<math.h>` builtin math function callable from indicator source.
#[derive(Clone, Copy)]
pub enum MathFn {
    Atan,
    Sqrt,
    Floor,
    Ceil,
    Log,
    Cos,
    Sin,
    Tan,
    Acos,
    Asin,
    Exp,
    Cosh,
    Sinh,
    Tanh,
    Log10,
    Abs,
    Max,
    Min,
}

impl MathFn {
    /// Classify a `FuncCall` name as a math builtin, or `None` if it is not one.
    /// The single source of truth that replaced the per-backend `MATH_FUNCTIONS`
    /// lists.
    pub fn from_name(name: &str) -> Option<Self> {
        Some(match name {
            "atan" => Self::Atan,
            "sqrt" => Self::Sqrt,
            "floor" => Self::Floor,
            "ceil" => Self::Ceil,
            "log" => Self::Log,
            "cos" => Self::Cos,
            "sin" => Self::Sin,
            "tan" => Self::Tan,
            "acos" => Self::Acos,
            "asin" => Self::Asin,
            "exp" => Self::Exp,
            "cosh" => Self::Cosh,
            "sinh" => Self::Sinh,
            "tanh" => Self::Tanh,
            "log10" => Self::Log10,
            "fabs" | "ABS" => Self::Abs,
            "max" | "fmax" => Self::Max,
            "min" | "fmin" => Self::Min,
            _ => return None,
        })
    }

    /// The canonical lowercase math name (`Abs` → `"abs"`, `Max` → `"max"`, …).
    /// Backends start from this and apply their own remaps where the language name
    /// differs.
    pub fn canonical(self) -> &'static str {
        match self {
            Self::Atan => "atan",
            Self::Sqrt => "sqrt",
            Self::Floor => "floor",
            Self::Ceil => "ceil",
            Self::Log => "log",
            Self::Cos => "cos",
            Self::Sin => "sin",
            Self::Tan => "tan",
            Self::Acos => "acos",
            Self::Asin => "asin",
            Self::Exp => "exp",
            Self::Cosh => "cosh",
            Self::Sinh => "sinh",
            Self::Tanh => "tanh",
            Self::Log10 => "log10",
            Self::Abs => "abs",
            Self::Max => "max",
            Self::Min => "min",
        }
    }
}

/// A TA-Lib "special" builtin: a magic call that every backend rewrites to its own
/// runtime construct rather than emitting verbatim. The names and the set are
/// identical across the C/Rust/Java backends and are checked before any other
/// dispatch; only the per-backend *rendering* differs (e.g. `UNSTABLE_PERIOD` →
/// `TA_GLOBALS_UNSTABLE(...)` in C, `this.unstableCount(...)` in Java,
/// `self.unstable_count(...)` in Rust). This enum is purely the shared classifier;
/// each backend matches it and supplies its own output.
#[derive(Clone, Copy)]
pub enum SpecialBuiltin {
    UnstablePeriod,
    /// `TA_UNSTABLE_AUTO(id, offset)`: `offset` under an Auto level of an id the function
    /// inherits, 0 under a count.
    UnstableAuto,
    IsZero,
    IsZeroScaled,
    IsZeroOrNeg,
    IsFinite,
    ArrayCopy,
    PerToK,
}

impl SpecialBuiltin {
    /// Classify a `FuncCall` name as a special builtin, or `None`.
    pub fn from_name(name: &str) -> Option<Self> {
        Some(match name {
            "UNSTABLE_PERIOD" => Self::UnstablePeriod,
            "UNSTABLE_AUTO" => Self::UnstableAuto,
            "IS_ZERO" => Self::IsZero,
            "IS_ZERO_SCALED" => Self::IsZeroScaled,
            "IS_ZERO_OR_NEG" => Self::IsZeroOrNeg,
            "IS_FINITE" => Self::IsFinite,
            "ARRAY_COPY" => Self::ArrayCopy,
            "PER_TO_K" => Self::PerToK,
            _ => return None,
        })
    }
}

/// A C standard-library function called from indicator source. The C backend emits
/// these verbatim (they are valid C); Java and Rust each rewrite them to a native
/// construct (`malloc` → `new T[]` / `vec![]`, `memcpy` → `System.arraycopy` /
/// slice copy, …). The set is shared so a new stdlib dependency is recognised in one
/// place. (`ARRAY_ALLOC` is a `ta_memory.h` macro, never a `FuncCall`, so it is not
/// a member.)
#[derive(Clone, Copy)]
pub enum StdlibFn {
    Sizeof,
    Malloc,
    Free,
    Memcpy,
    Memmove,
    Memset,
}

impl StdlibFn {
    /// Classify a `FuncCall` name as a C stdlib function, or `None`.
    pub fn from_name(name: &str) -> Option<Self> {
        Some(match name {
            "sizeof" => Self::Sizeof,
            "malloc" => Self::Malloc,
            "free" => Self::Free,
            "memcpy" => Self::Memcpy,
            "memmove" => Self::Memmove,
            "memset" => Self::Memset,
            _ => return None,
        })
    }
}


/// The Auto levels of the unstable-period setting, as `(X, K)`: the level's constant is
/// `INDEX_MAX + X`, `X` is its digit count and `K` its number of e-folds.
pub const UNSTABLE_AUTO_LEVELS: &[(i64, i64)] = &[(4, 10), (8, 19)];

/// The count expression of a `TA_UNSTABLE(id, count)` read, once per Auto level, with the
/// free names `K` and `X` bound to that level's values. `None` unless the read has both
/// arguments.
#[must_use]
pub fn unstable_level_counts(args: &[Expr]) -> Option<Vec<Expr>> {
    let [_, count] = args else { return None };
    Some(
        UNSTABLE_AUTO_LEVELS
            .iter()
            .map(|&(x, k)| {
                let subs = std::collections::HashMap::from([
                    ("K".to_string(), Expr::IntLiteral(k)),
                    ("X".to_string(), Expr::IntLiteral(x)),
                ]);
                fold_level_select(crate::helper_registry::substitute_expr(count, &subs))
            })
            .collect(),
    )
}

/// Wrap a rendered `TA_UNSTABLE(id, c4, c8)` read into the Auto-only offset: the same read
/// less the read whose counts are 0, which is the stored count under a count and 0 under a
/// level. `read` renders one read from its per-level counts.
pub fn unstable_auto_offset(counts: &[String], read: impl Fn(&[String]) -> String) -> String {
    let zeros = vec!["0".to_string(); counts.len()];
    format!("({} - {})", read(counts), read(&zeros))
}

/// `4 == 4 ? a : b` to `a`, inside sums, casts and call arguments: how a count that cannot be written in `K` picks
/// a per-level local (`X == 4 ? count4 : count8`) without leaving a constant condition in
/// the output.
fn fold_level_select(expr: Expr) -> Expr {
    let fold = |inner: Box<Expr>| Box::new(fold_level_select(*inner));
    match expr {
        Expr::Ternary(cond, then, other) => match fold_level_select(*cond) {
            Expr::BinOp(lhs, BinOp::Eq, rhs) => match (*lhs, *rhs) {
                (Expr::IntLiteral(left), Expr::IntLiteral(right)) => {
                    fold_level_select(if left == right { *then } else { *other })
                }
                (lhs, rhs) => Expr::Ternary(
                    Box::new(Expr::BinOp(Box::new(lhs), BinOp::Eq, Box::new(rhs))),
                    fold(then),
                    fold(other),
                ),
            },
            cond => Expr::Ternary(Box::new(cond), fold(then), fold(other)),
        },
        Expr::BinOp(lhs, op, rhs) => Expr::BinOp(fold(lhs), op, fold(rhs)),
        Expr::Cast(ty, inner) => Expr::Cast(ty, fold(inner)),
        Expr::FuncCall(name, args) => Expr::FuncCall(name, args.into_iter().map(fold_level_select).collect()),
        other => other,
    }
}
