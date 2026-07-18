/// A complete function definition (metadata + logic).
#[derive(Debug, Clone)]
pub struct FuncDef {
    pub name: String,
    pub group: String,
    pub description: Option<String>,
    pub camel_case: Option<String>,
    pub hint: Option<String>,
    pub flags: Vec<String>,
    pub inputs: Vec<Input>,
    pub optional_inputs: Vec<OptInput>,
    pub outputs: Vec<Output>,
    pub lookback: Option<LookbackExpr>,
    pub body: Vec<Statement>,
    /// Body for the private variant (explicit from _private function, or auto-derived from body).
    pub private_body: Vec<Statement>,
    /// Extra parameters on the private variant beyond the public API
    /// (e.g., EMA's k factor). Each entry is (param_name, c_type).
    pub private_extra_params: Vec<(String, String)>,
    /// Init expressions for private extra params, extracted from the guarded body's
    /// VarDecls. Used by backends to generate S_ variant bodies (which inline the
    /// private body with extra params as local variables instead of function params).
    /// Each entry is (param_name, init_expr).
    pub private_param_init: Vec<(String, Expr)>,
    /// True when the C source explicitly defines foo_private().
    pub has_explicit_private: bool,
    /// File-level comment blocks that preceded the first function in the input
    /// `.c` (e.g. the `List of contributors` / `Change history` block). Emitted
    /// verbatim after the generated banner, before the function definitions.
    /// Each entry is one comment block, itself a list of content lines.
    pub header_comments: Vec<Vec<String>>,
    /// Canonical human documentation parsed from `ta_codegen/input/<name>/<name>.md`
    /// (see docs/ta_codegen_input_doc.md). Prose only — numbers stay in the YAML
    /// and are injected at render time.
    pub doc: Option<DocDef>,
    /// True when the YAML `flags:` list contains `stream`: generate the
    /// streaming API for this function (docs/streaming-api-proposal.md; the
    /// flag maps to TA_FUNC_FLG_STREAM in ta_abstract). Derived convenience
    /// mirror of `flags` — no flag = no stream code. The generator fails
    /// `generate` if a flagged function is no longer analyzable (the
    /// maintenance-coupling gate).
    pub streaming: bool,
}

/// IR-derived streamability tier (internal — never authored by hand; the
/// `stream-census` subcommand reports it). Stage 1 ships T1/T2; later stages
/// extend this enum (T3 rings, T4a/T4b extrema, TC composed).
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum StreamTier {
    /// Pure per-bar map — no cross-bar state.
    T1,
    /// Scalar recurrence — O(1) carried scalars and/or bounded lag reads.
    T2,
    /// Fixed trailing window — ring buffer(s) of O(period).
    T3,
    /// Window extrema — cached-index automaton over a ring (amortized O(1),
    /// worst-case O(period) per bar, exactly like batch).
    T4,
}

impl StreamTier {
    #[must_use]
    pub fn as_str(self) -> &'static str {
        match self {
            Self::T1 => "T1",
            Self::T2 => "T2",
            Self::T3 => "T3",
            Self::T4 => "T4",
        }
    }
}

/// Parsed canonical documentation (`<name>.md`) for one function.
///
/// Mirrors the section list in docs/ta_codegen_input_doc.md. All strings are
/// markdown as authored; renderers escape/adapt per target (rustdoc, mkdocs, ...).
#[derive(Debug, Clone, Default)]
pub struct DocDef {
    /// `## Summary` — what the function is and how to read its output.
    pub summary: String,
    /// `## Formula` — brief high-level formula, verbatim lines.
    pub formula: Option<String>,
    /// `## Notes` — variations from the original indicator, one entry per bullet.
    pub notes: Vec<String>,
    /// `## Inputs` — `(name, description)` per input, keyed by the YAML input
    /// name (price bundles appear as e.g. `inPriceOHLC`, not their components).
    pub inputs: Vec<(String, String)>,
    /// `## Outputs` — `(name, description)` per output.
    pub outputs: Vec<(String, String)>,
    /// `## Parameters` — `(name, description)` per optional input.
    pub params: Vec<(String, String)>,
    /// `## Aliases` — alternative names / abbreviation expansions.
    pub aliases: Vec<String>,
    /// `## See Also` — related TA-Lib function names.
    pub see_also: Vec<String>,
    /// `## References` — book/site citations, one entry per bullet.
    pub references: Vec<String>,
}

#[derive(Debug, Clone)]
pub struct Input {
    pub name: String,
    pub param_type: ParamType,
}

#[derive(Debug, Clone)]
pub struct OptInput {
    pub name: String,
    pub param_type: ParamType,
    pub range: Option<(f64, f64)>,
    pub default: Option<f64>,
    pub display_name: Option<String>,
    pub hint: Option<String>,
    pub flags: Vec<String>,
    /// Optimization hints: [`suggested_start`, `suggested_end`, `suggested_increment`]
    pub suggested: Option<(f64, f64, f64)>,
    /// Number of decimal digits for UI display (only for `TA_RealRange`).
    pub precision: Option<i32>,
}

#[derive(Debug, Clone)]
pub struct Output {
    pub name: String,
    pub param_type: ParamType,
    pub flags: Vec<String>,
}

impl Output {
    /// A nullable output may be passed `NULL` (C) meaning "compute but don't
    /// write this output". Carried as the `nullable` flag so it surfaces through
    /// `ta_abstract` (`TA_OUT_NULLABLE`), where a binding/introspection consumer
    /// can discover it. The C backend NULL-guards every write and skips the
    /// output's NULL-validation; the streaming dispatch passes NULL for it. See
    /// MAMA's `outFAMA`, discarded by MA/BBANDS/STOCH… when `MAType == MAMA`.
    #[must_use]
    pub fn is_nullable(&self) -> bool {
        self.flags.iter().any(|f| f == "nullable")
    }
}

#[derive(Debug, Clone, PartialEq)]
pub enum ParamType {
    Real,
    Integer,
    /// An enum parameter type, e.g. `Enum("MAType")`.
    /// The string is the enum name as defined in `enums.yaml`.
    Enum(String),
    /// Price input: expands to multiple array parameters (e.g., high, low, close).
    /// The vector contains component names like "open", "high", "low", "close", "volume".
    Price(Vec<String>),
}

/// An enum type definition (loaded from `enums.yaml`).
#[derive(Debug, Clone)]
pub struct EnumDef {
    pub name: String,
    pub variants: Vec<EnumVariant>,
}

/// A single variant of an enum type.
#[derive(Debug, Clone)]
pub struct EnumVariant {
    /// C constant name, e.g. `TA_MAType_SMA`
    pub c_name: String,
    /// `PascalCase` name used in Java/.NET, e.g. `Sma`
    pub pascal_name: String,
    /// `UPPER_CASE` short name used in source labels, e.g. `SMA`
    pub short_name: String,
    /// Integer value
    pub value: i32,
}

#[derive(Debug, Clone)]
pub enum LookbackExpr {
    Literal(i32),
    ParamMinus(String, i32),
    /// Complex lookback body (parsed from multi-line YAML string).
    /// Contains the full lookback function body as statements.
    Code(Vec<Statement>),
}

// --- Logic AST ---

/// Element layout of a CIRCBUF (circular FIFO buffer).
#[derive(Debug, Clone)]
pub enum CircBufLayout {
    /// `CIRCBUF_PROLOG`: a single buffer stored under exactly `<id>`. Scalar elem type.
    Plain(VarType),
    /// `CIRCBUF_PROLOG_CLASS`: one parallel buffer per struct field, stored as `<id>_<field>`
    /// (matches the existing `CIRCBUF_REF` flatten). Each entry is `(field_name, scalar_type)`.
    Class(Vec<(String, VarType)>),
}

/// A circular FIFO buffer operation (the `CIRCBUF_*` macros from `ta_memory.h`).
/// Lowered per-backend: C stack-first hybrid (static array + heap fallback),
/// Rust `Vec`, Java `new[]`, .NET no-op (P/Invoke has no body).
#[derive(Debug, Clone)]
pub enum CircBuf {
    /// `CIRCBUF_PROLOG(Id,Type,N)` / `_PROLOG_CLASS`. Declares storage, `<id>_Idx`, `maxIdx_<id>`.
    Prolog {
        id: String,
        layout: CircBufLayout,
        static_size: i64,
    },
    /// `CIRCBUF_INIT(Id,Type,size)` / `_INIT_CLASS`. Runtime-sized (heap iff size > static_size).
    Init {
        id: String,
        layout: CircBufLayout,
        size: Expr,
    },
    /// `CIRCBUF_INIT_LOCAL_ONLY(Id,Type)`. Always the static capacity; no runtime size.
    InitLocalOnly { id: String, layout: CircBufLayout },
    /// `CIRCBUF_NEXT(Id)`: advance the index with wraparound.
    Next { id: String },
    /// `CIRCBUF_DESTROY(Id)`: C frees each heap buffer iff allocated; other backends no-op.
    Destroy { id: String, layout: CircBufLayout },
}

#[derive(Debug, Clone)]
pub enum Statement {
    VarDecl {
        var_type: VarType,
        name: String,
        init: Option<Expr>,
    },
    Assign {
        target: Expr,
        value: Expr,
        /// True if originally written as a compound assignment (+=, -=, etc.)
        compound: bool,
    },
    While {
        condition: Expr,
        body: Vec<Statement>,
    },
    /// do { body } while(condition);
    DoWhile {
        condition: Expr,
        body: Vec<Statement>,
    },
    /// Countdown for loop: `for( var = count; var > 0; var-- ) { body }`
    /// Renders as `for var in (1..=count).rev()` in Rust.
    For {
        var: String,
        count: Expr,
        body: Vec<Statement>,
    },
    If {
        condition: Expr,
        then_body: Vec<Statement>,
        else_body: Vec<Statement>,
        /// Per-operand trailing comments for a top-level `&&`-chain condition,
        /// indexed to match the flattened left-assoc operand order. Empty unless
        /// the source annotated the conditions inline (e.g. CDL patterns:
        /// `TA_CANDLECOLOR(i-3)==1 && // white`). When non-empty and the condition
        /// is not candle-split, backends render the condition multi-line with each
        /// operand's comment inline. Purely cosmetic — never affects behavior.
        cond_comments: Vec<Option<Vec<String>>>,
    },
    Return {
        value: Option<Expr>,
    },
    Break,
    Continue,
    /// switch(expr) { case Value: stmts; break; ... default: stmts; }
    Switch {
        expr: Expr,
        cases: Vec<(String, Vec<Statement>)>,
        default: Vec<Statement>,
    },
    /// C-style for loop: for(init; cond; update) { body }
    ForC {
        init: Box<Statement>,
        condition: Expr,
        update: Box<Statement>,
        body: Vec<Statement>,
    },
    /// A block of statements (used for `ARRAY_COPY` expansion in some backends).
    Block {
        body: Vec<Statement>,
    },
    /// A statement that evaluates an expression purely for its side effects,
    /// e.g. a void function or macro call (`SAR_ROUNDING(x);`, `TRUE_RANGE(...);`).
    /// The expression's value is discarded.
    Expr(Expr),
    /// Circular FIFO buffer op (`CIRCBUF_*`). Lowered per-backend (see [`CircBuf`]).
    CircBuf(CircBuf),
    /// A source comment carried verbatim from the input `.c`, rendered
    /// positionally immediately before the statement it preceded. Each entry is
    /// one content line (comment delimiters and leading `*` prefixes stripped);
    /// backends re-wrap it in their own comment syntax. Emits no executable code
    /// and never affects behavior — it is trivia threaded through for fidelity.
    Comment(Vec<String>),
}

#[derive(Debug, Clone, PartialEq)]
pub enum VarType {
    Real,
    Integer,
    Index,
    RetCodeType,
    /// Pointer to double: `double *buf`
    RealPointer,
    /// Pointer to int: `int *ptr`
    IntPointer,
    /// Fixed-size array of doubles: `double buf[30]`
    RealArray(String),
    /// Fixed-size array of ints: `int buf[3]`
    IntArray(String),
}

#[derive(Debug, Clone)]
pub enum Expr {
    Literal(f64),
    IntLiteral(i64),
    Var(String),
    ArrayAccess(String, Box<Expr>),
    BinOp(Box<Expr>, BinOp, Box<Expr>),
    Cast(VarType, Box<Expr>),
    Not(Box<Expr>),
    /// Function/builtin call: `UNSTABLE_PERIOD(RSI)`, `IS_ZERO(x)`, `ARRAY_COPY`(...),
    /// `RSI_Lookback(params`...), etc.
    FuncCall(String, Vec<Expr>),
    /// Pointer dereference: *outBegIdx
    PointerDeref(String),
    /// Address-of: &var or &var[idx]
    AddressOf(Box<Expr>),
    /// Post-increment: var++ (evaluates to var, then increments)
    PostIncrement(Box<Expr>),
    /// Post-decrement: var-- (evaluates to var, then decrements)
    PostDecrement(Box<Expr>),
    /// Pre-increment: ++var (increments, then evaluates to new value)
    PreIncrement(Box<Expr>),
    /// Pre-decrement: --var (decrements, then evaluates to new value)
    PreDecrement(Box<Expr>),
    /// Ternary: condition ? `then_expr` : `else_expr`
    Ternary(Box<Expr>, Box<Expr>, Box<Expr>),
}

#[derive(Debug, Clone)]
pub enum BinOp {
    Add,
    Sub,
    Mul,
    Div,
    Mod, // % operator
    LessEq,
    Less,
    Greater,
    GreaterEq,
    Eq,
    NotEq,
    And,
    Shr, // >> right shift
    Shl, // << left shift
    Or,
    BitwiseOr, // | bitwise OR
}

/// A helper function definition (parsed from ta_codegen/input/helpers/*.c).
/// Helpers are inlined at call sites during code generation.
#[derive(Debug, Clone)]
pub struct HelperDef {
    pub name: String,
    pub return_type: VarType,
    pub params: Vec<HelperParam>,
    pub body: Vec<Statement>,
}

/// A parameter of a helper function.
#[derive(Debug, Clone)]
pub struct HelperParam {
    pub name: String,
    pub var_type: VarType,
}
