//! Typed taxonomy of builtin function calls shared by the language backends.
//!
//! Indicator source calls a handful of `<math.h>` functions (`sqrt`, `sin`,
//! `max`, …) by name as `Expr::FuncCall`. Each backend used to classify those
//! names against its own `const MATH_FUNCTIONS: &[&str]` list — three copies that
//! had already drifted (e.g. only the Rust list carried a stray lowercase `abs`).
//! [`MathFn`] replaces all three with one classifier, so a name is recognised as a
//! math builtin in exactly one place and the compiler enforces that every backend
//! handles each variant.
//!
//! Name aliases that render identically within every backend collapse into one
//! variant: `max`/`fmax` → [`MathFn::Max`], `min`/`fmin` → [`MathFn::Min`],
//! `fabs`/`ABS` → [`MathFn::Abs`]. Each backend starts from [`MathFn::canonical`]
//! and applies its own small remap (C uses `fmax`/`fmin`/`fabs`; Java prefixes
//! `Math.`; Rust emits method calls and uses `ln` for `Log`).

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
    /// differs (e.g. C maps `Max`/`Min`/`Abs` to `fmax`/`fmin`/`fabs`).
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
/// `TA_GLOBALS_UNSTABLE_PERIOD(...)` in C, `this.unstablePeriod[...]` in Java,
/// `self.unstable_period[...]` in Rust). This enum is purely the shared classifier;
/// each backend matches it and supplies its own output.
#[derive(Clone, Copy)]
pub enum SpecialBuiltin {
    UnstablePeriod,
    Compatibility,
    IsZero,
    IsZeroScaled,
    IsZeroOrNeg,
    ArrayCopy,
    PerToK,
}

impl SpecialBuiltin {
    /// Classify a `FuncCall` name as a special builtin, or `None`.
    pub fn from_name(name: &str) -> Option<Self> {
        Some(match name {
            "UNSTABLE_PERIOD" => Self::UnstablePeriod,
            "COMPATIBILITY" => Self::Compatibility,
            "IS_ZERO" => Self::IsZero,
            "IS_ZERO_SCALED" => Self::IsZeroScaled,
            "IS_ZERO_OR_NEG" => Self::IsZeroOrNeg,
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
