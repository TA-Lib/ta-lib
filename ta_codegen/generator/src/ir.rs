/// A complete function definition (metadata + logic).
#[derive(Debug, Clone)]
pub struct FuncDef {
    pub name: String,
    pub group: String,
    pub description: Option<String>,
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
    /// Alternate implementations declared in the input `.c` as
    /// `<name>_ALT<n>`, in file order (see [`AltDef`]). Empty for all but a
    /// handful of functions.
    pub alternates: Vec<AltDef>,
    /// Set only by [`FuncDef::resolved_for`], and only when an alternate wins
    /// the `(STREAM, lang)` cell: the body the streaming tier is derived from.
    ///
    /// A separate field rather than an overwrite of `private_body`, because
    /// `private_body` is a *clone* of `body` for every function that declares no
    /// `_private`, and code that edits `body` afterwards (the dispatch sabotage
    /// tests, any future rewrite pass) would find its edit silently ignored by
    /// the stream analyzers. `None` keeps [`FuncDef::stream_source`] on exactly
    /// the selection every analyzer already made.
    pub resolved_stream_body: Option<Vec<Statement>>,
}

/// The API tier an alternate claims. `AllApi` appears only in a *claim*;
/// resolution is always asked about a concrete [`Tier`].
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum ApiClaim {
    Batch,
    Stream,
    AllApi,
}

/// The language an alternate claims. `AllLanguages` appears only in a *claim*;
/// resolution is always asked about a concrete [`Lang`].
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum LangClaim {
    C,
    Rust,
    Java,
    CSharp,
    AllLanguages,
}

/// One concrete API tier — what a caller resolves against.
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum Tier {
    Batch,
    Stream,
}

/// One concrete backend language — what a caller resolves against.
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum Lang {
    C,
    Rust,
    Java,
    CSharp,
}

/// Every concrete backend language, for the sweeps that must hold in all of
/// them (the generate-time streamability gate, shadowing analysis).
pub const ALL_LANGS: [Lang; 4] = [Lang::C, Lang::Rust, Lang::Java, Lang::CSharp];

/// [`FuncDef::resolved_for`] over a whole slice — the resolution point for the
/// generators that take the entire function list at once (the JSON-RPC servers,
/// the bench binaries). Borrowed and free while no function declares an
/// alternate.
#[must_use]
pub fn resolve_all(funcs: &[FuncDef], lang: Lang) -> std::borrow::Cow<'_, [FuncDef]> {
    if funcs.iter().all(|f| f.alternates.is_empty()) {
        return std::borrow::Cow::Borrowed(funcs);
    }
    std::borrow::Cow::Owned(
        funcs
            .iter()
            .map(|f| f.resolved_for(lang).into_owned())
            .collect(),
    )
}

/// Every concrete API tier.
pub const ALL_TIERS: [Tier; 2] = [Tier::Batch, Tier::Stream];

impl Tier {
    #[must_use]
    pub fn as_str(self) -> &'static str {
        match self {
            Self::Batch => "BATCH",
            Self::Stream => "STREAM",
        }
    }
}

impl Lang {
    #[must_use]
    pub fn as_str(self) -> &'static str {
        match self {
            Self::C => "C",
            Self::Rust => "RUST",
            Self::Java => "JAVA",
            Self::CSharp => "CSHARP",
        }
    }

    /// The backend key (`backend.name()`) this language corresponds to.
    #[must_use]
    pub fn from_backend_name(name: &str) -> Option<Self> {
        match name {
            "c" => Some(Self::C),
            "rust" => Some(Self::Rust),
            "java" => Some(Self::Java),
            "csharp" => Some(Self::CSharp),
            _ => None,
        }
    }
}

impl ApiClaim {
    #[must_use]
    pub fn parse(s: &str) -> Option<Self> {
        match s {
            "BATCH" => Some(Self::Batch),
            "STREAM" => Some(Self::Stream),
            "ALL_API" => Some(Self::AllApi),
            _ => None,
        }
    }

    #[must_use]
    pub fn as_str(self) -> &'static str {
        match self {
            Self::Batch => "BATCH",
            Self::Stream => "STREAM",
            Self::AllApi => "ALL_API",
        }
    }

    #[must_use]
    pub fn covers(self, tier: Tier) -> bool {
        match self {
            Self::AllApi => true,
            Self::Batch => tier == Tier::Batch,
            Self::Stream => tier == Tier::Stream,
        }
    }
}

impl LangClaim {
    #[must_use]
    pub fn parse(s: &str) -> Option<Self> {
        match s {
            "C" => Some(Self::C),
            "RUST" => Some(Self::Rust),
            "JAVA" => Some(Self::Java),
            "CSHARP" => Some(Self::CSharp),
            "ALL_LANGUAGES" => Some(Self::AllLanguages),
            _ => None,
        }
    }

    #[must_use]
    pub fn as_str(self) -> &'static str {
        match self {
            Self::C => "C",
            Self::Rust => "RUST",
            Self::Java => "JAVA",
            Self::CSharp => "CSHARP",
            Self::AllLanguages => "ALL_LANGUAGES",
        }
    }

    #[must_use]
    pub fn covers(self, lang: Lang) -> bool {
        match self {
            Self::AllLanguages => true,
            Self::C => lang == Lang::C,
            Self::Rust => lang == Lang::Rust,
            Self::Java => lang == Lang::Java,
            Self::CSharp => lang == Lang::CSharp,
        }
    }
}

/// An alternate implementation: a whole second body for the same function,
/// declared in the input `.c` as `<name>_ALT<n>` under a
/// `/* PRAGMA TA_ALT={<api>,<lang>} */` decoration.
///
/// An alternate is **generator input only** — it never becomes a symbol in any
/// backend. There is still exactly one `TA_MIN`, one `TA_MIN_Open`, and a
/// cross-indicator call to `min(...)` resolves the same way it always did; the
/// claim decides only which body the generator transcribes into those existing
/// functions. It must be strictly functionally identical to the base, and the
/// accepted justification for carrying one is a significant performance gain.
#[derive(Debug, Clone)]
pub struct AltDef {
    /// The declared C name, e.g. `min_ALT1`.
    pub name: String,
    /// The `<n>` in `_ALT<n>`; ascending and contiguous from 1 in file order,
    /// so a diff that reorders two alternates fails the gate rather than
    /// silently changing which one wins.
    pub index: u32,
    pub api: ApiClaim,
    pub lang: LangClaim,
    pub body: Vec<Statement>,
}

impl FuncDef {
    /// The alternate that wins one concrete `(tier, language)` cell, or `None`
    /// when the base body does.
    ///
    /// The whole rule: **later declarations override earlier ones for every
    /// cell they claim**, with the base as the implicit position-0 entry
    /// claiming `{ALL_API,ALL_LANGUAGES}`. No specificity table — position
    /// decides, the way `.gitignore` and the CSS cascade decide.
    #[must_use]
    pub fn resolve_alt(&self, tier: Tier, lang: Lang) -> Option<&AltDef> {
        self.alternates
            .iter()
            .rev()
            .find(|a| a.api.covers(tier) && a.lang.covers(lang))
    }

    /// The body the batch emitters render for `lang`.
    #[must_use]
    pub fn batch_body(&self, lang: Lang) -> &[Statement] {
        self.resolve_alt(Tier::Batch, lang)
            .map_or(&self.body[..], |a| &a.body[..])
    }

    /// The alternate body claiming `(STREAM, lang)`, if one does.
    #[must_use]
    pub fn stream_alt_body(&self, lang: Lang) -> Option<&[Statement]> {
        self.resolve_alt(Tier::Stream, lang).map(|a| &a.body[..])
    }

    /// The body the streaming tier is derived from.
    ///
    /// Naming the selection once is what makes `PRAGMA TA_ALT={STREAM,...}`
    /// reachable: the six analyzers each spelled this out inline, and a site
    /// left behind would silently analyze the batch body. Absent a resolved
    /// alternate it is exactly the choice they all made.
    #[must_use]
    pub fn stream_source(&self) -> &[Statement] {
        if let Some(b) = &self.resolved_stream_body {
            return b;
        }
        if self.has_explicit_private {
            &self.private_body
        } else {
            &self.body
        }
    }

    /// One line naming the alternate that won a cell, for the generated output.
    ///
    /// `None` when the base won — which is every cell of all but a handful of
    /// functions, and emitting a "using the base body" line everywhere would
    /// churn 168 files' output to say nothing. The backends wrap it in their own
    /// comment syntax.
    #[must_use]
    pub fn alt_marker(&self, tier: Tier, lang: Lang) -> Option<String> {
        self.resolve_alt(tier, lang).map(|a| {
            format!(
                "Using {} for TA_ALT={{{},{}}}",
                a.name,
                a.api.as_str(),
                a.lang.as_str()
            )
        })
    }

    /// This definition with its bodies already resolved for one backend
    /// language: `body` becomes the winner of `(BATCH, lang)` and
    /// [`stream_source`](Self::stream_source) the winner of `(STREAM, lang)`.
    ///
    /// **This is the single resolution point.** Everything downstream — the six
    /// stream analyzers, the four batch emitters, the FMA fusion-set builders,
    /// the CIRCBUF prolog lookup, candle-settings detection — keeps reading
    /// `body` / `stream_source()` and is correct without knowing alternates exist.
    /// The alternative was teaching ~20 scattered selection sites about the
    /// language, where *missing one is silent* (it would quietly render the
    /// base), which is the failure class this construct exists to remove.
    /// `alternates` is carried through unchanged so the emitters can name the
    /// winner in the generated output.
    ///
    /// Borrowed and free for the 167 functions that declare no alternate.
    #[must_use]
    pub fn resolved_for(&self, lang: Lang) -> std::borrow::Cow<'_, Self> {
        if self.alternates.is_empty() {
            return std::borrow::Cow::Borrowed(self);
        }
        let mut out = self.clone();
        // Pin BOTH tiers, even where the base wins. `stream_source` falls back
        // to `body` when nothing claims STREAM, and `body` is about to become
        // the *batch* winner — so resolving only the claimed side would hand the
        // streaming tier a batch alternate it was never meant to see.
        out.resolved_stream_body = Some(
            self.stream_alt_body(lang)
                .unwrap_or_else(|| self.stream_source())
                .to_vec(),
        );
        out.body = self.batch_body(lang).to_vec();
        // Keep `private_body` mirroring `body`, the invariant the parser
        // establishes for every function that declares no `_private`. Several
        // batch-side consumers read it as "the body" — the Rust `_private`
        // renderer, which is where the issue-#160 negative-cast gate runs, and
        // the C/Java single-precision variants — and a BATCH alternate that left
        // it pointing at the base would have them validate and render a body
        // that is no longer emitted. The streaming tier is unaffected: with
        // alternates present, `stream_source()` always reads
        // `resolved_stream_body`, which is set unconditionally above.
        if !self.has_explicit_private {
            out.private_body.clone_from(&out.body);
        }
        std::borrow::Cow::Owned(out)
    }
}

/// IR-derived streamability tier (internal — never authored by hand; the
/// `stream-census` subcommand reports it). All four ship. Extrema are a single
/// `T4`: the design doc's T4a/T4b split was scoped but never built, since both
/// halves ended up transcribing the same automaton.
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum StreamTier {
    /// Pure per-bar map — no cross-bar state.
    T1,
    /// Scalar recurrence — O(1) carried scalars and/or bounded lag reads.
    T2,
    /// Fixed trailing window — ring buffer(s) of O(period).
    T3,
    /// Window extrema — cached-index automaton over a ring, transcribed
    /// verbatim from batch, so the cost profile is batch's: O(1) per bar while
    /// the cached extremum sits away from the trailing edge, O(period) per bar
    /// while it sits on it. Not amortized O(1) — see issue #147.
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
    /// Prose that follows the formula's closing `$$` — the sentence defining its
    /// symbols. Split out by the parser because every backend renders the formula
    /// as preformatted text, and a sentence left inside that block renders as
    /// code (rustdoc ```` ```text ````, javadoc `<pre>{@code`, C# `<code>`).
    /// `None` for the formulas that carry no display math, which is most of them.
    pub formula_note: Option<String>,
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

/// One component of a `ta_abstract` price bar.
///
/// `ta_abstract` describes a price bar as a *single* `TA_Input_Price` parameter whose
/// `flags` word names the components it needs (`TA_IN_PRICE_HIGH|...`). That grouping is
/// public ABI — `include/ta_abstract.h` has carried it since the 2002 initial revision,
/// and wrappers read it: ta-lib-python turns `inPriceHLC` + flags into
/// `{'prices': ['high','low','close']}`. The generated *functions*, by contrast, take one
/// array per component (`TA_STOCH(startIdx, endIdx, inHigh, inLow, inClose, ...)`).
///
/// This enum is the single definition of the two representations' correspondence, so the
/// flag bit, the `inPriceXXX` suffix letter and the argument name cannot drift apart
/// across the four backends that each used to carry their own copy.
///
/// `TA_IN_PRICE_TIMESTAMP` (0x40) is deliberately absent: `TA_PricePtrs`
/// (`src/ta_abstract/ta_frame_priv.h`) has no timestamp field, so a bundle declaring one
/// could never have produced compiling C. Declaring `timestamp` in `price_components:`
/// now fails loudly in the parser rather than generating a broken frame.
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum PriceComponent {
    Open,
    High,
    Low,
    Close,
    Volume,
    OpenInterest,
}

impl PriceComponent {
    /// The YAML `price_components:` token.
    #[must_use]
    pub fn token(self) -> &'static str {
        match self {
            PriceComponent::Open => "open",
            PriceComponent::High => "high",
            PriceComponent::Low => "low",
            PriceComponent::Close => "close",
            PriceComponent::Volume => "volume",
            PriceComponent::OpenInterest => "openInterest",
        }
    }

    /// The generated argument name this component expands to.
    #[must_use]
    pub fn input_name(self) -> &'static str {
        match self {
            PriceComponent::Open => "inOpen",
            PriceComponent::High => "inHigh",
            PriceComponent::Low => "inLow",
            PriceComponent::Close => "inClose",
            PriceComponent::Volume => "inVolume",
            PriceComponent::OpenInterest => "inOpenInterest",
        }
    }

    /// The `TA_IN_PRICE_*` flag bit (`include/ta_abstract.h`) — public ABI.
    #[must_use]
    pub fn flag_bit(self) -> u32 {
        match self {
            PriceComponent::Open => 0x0000_0001,
            PriceComponent::High => 0x0000_0002,
            PriceComponent::Low => 0x0000_0004,
            PriceComponent::Close => 0x0000_0008,
            PriceComponent::Volume => 0x0000_0010,
            PriceComponent::OpenInterest => 0x0000_0020,
        }
    }

    /// The `TA_IN_PRICE_*` flag macro name, for the generated C.
    #[must_use]
    pub fn flag_macro(self) -> &'static str {
        match self {
            PriceComponent::Open => "TA_IN_PRICE_OPEN",
            PriceComponent::High => "TA_IN_PRICE_HIGH",
            PriceComponent::Low => "TA_IN_PRICE_LOW",
            PriceComponent::Close => "TA_IN_PRICE_CLOSE",
            PriceComponent::Volume => "TA_IN_PRICE_VOLUME",
            PriceComponent::OpenInterest => "TA_IN_PRICE_OPENINTEREST",
        }
    }

    /// Human-readable name, as `ta_func_api.xml` spells it.
    #[must_use]
    pub fn display_name(self) -> &'static str {
        match self {
            PriceComponent::Open => "Open",
            PriceComponent::High => "High",
            PriceComponent::Low => "Low",
            PriceComponent::Close => "Close",
            PriceComponent::Volume => "Volume",
            PriceComponent::OpenInterest => "Open Interest",
        }
    }

    /// The letter this component contributes to an `inPriceXXX` name (`High` -> `H`).
    #[must_use]
    pub fn suffix_letter(self) -> char {
        match self {
            PriceComponent::Open => 'O',
            PriceComponent::High => 'H',
            PriceComponent::Low => 'L',
            PriceComponent::Close => 'C',
            PriceComponent::Volume => 'V',
            PriceComponent::OpenInterest => 'I',
        }
    }

    /// Parse a YAML `price_components:` token.
    #[must_use]
    pub fn from_token(token: &str) -> Option<Self> {
        match token {
            "open" => Some(PriceComponent::Open),
            "high" => Some(PriceComponent::High),
            "low" => Some(PriceComponent::Low),
            "close" => Some(PriceComponent::Close),
            "volume" => Some(PriceComponent::Volume),
            "openInterest" | "openinterest" => Some(PriceComponent::OpenInterest),
            _ => None,
        }
    }
}

/// An input argument's membership in a price bundle.
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub struct PriceRef {
    /// Ordinal of the bundle within this function's input list (0 = the first bundle).
    /// Carrying it explicitly is what lets `ta_abstract` rebuild `TA_Input_Price` from a
    /// declaration instead of guessing from a run of consecutive component *names* —
    /// a guess that would regroup wrongly, and so silently change the public abstract
    /// API, for a standalone `inVolume` or a `real` input placed between components.
    pub group: usize,
    /// Which component of that bundle this argument carries.
    pub component: PriceComponent,
}

/// One argument of the generated function signature.
#[derive(Debug, Clone)]
pub struct Input {
    pub name: String,
    pub param_type: ParamType,
    /// `Some` when this argument is one component of a `ta_abstract` price bundle.
    /// The YAML declares the bundle (`type: price` + `price_components:`) and
    /// `parser::yaml` expands it into one `Input` per component, tagging each with the
    /// bundle it came from; `backends::price_bundle` folds them back for `ta_abstract`.
    pub price: Option<PriceRef>,
}

impl Input {
    /// A plain (non-bundle) input argument.
    #[must_use]
    pub fn new(name: impl Into<String>, param_type: ParamType) -> Self {
        Input {
            name: name.into(),
            param_type,
            price: None,
        }
    }
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
    /// C constant prefix, e.g. `TA_MAType_`. C is the only backend that adds
    /// anything to a variant's name.
    pub c_prefix: String,
    pub variants: Vec<EnumVariant>,
}

/// A single variant of an enum type.
#[derive(Debug, Clone)]
pub struct EnumVariant {
    /// The variant identity, e.g. `SMA` or `HT_DCPERIOD`. Spelled verbatim by
    /// Rust, Java and C#, and used as the source case label (`MAType_SMA`).
    pub name: String,
    /// C constant name, e.g. `TA_MAType_SMA`. Composed from the enum's
    /// `c_prefix` — derived, never authored.
    pub c_name: String,
    /// Pinned integer value. Part of the ABI: identical in every backend and
    /// append-only.
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
/// Rust `Vec`, Java `new[]`, C# `new[]`.
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
    /// `TA_UNROLL(n)` — unroll hint for the loop on the next line. Written in
    /// the input C source and carried through so the C backend can re-emit the
    /// macro (`ta_utility.h` expands it to a GCC/Clang unroll pragma, and to
    /// nothing on every other compiler). Advisory only: no backend may let it
    /// change what is computed, and Rust/Java/C# drop it entirely.
    UnrollHint {
        count: u32,
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

#[derive(Debug, Clone, PartialEq)]
pub enum Expr {
    Literal(f64),
    IntLiteral(i64),
    Var(String),
    ArrayAccess(String, Box<Expr>),
    BinOp(Box<Expr>, BinOp, Box<Expr>),
    Cast(VarType, Box<Expr>),
    Not(Box<Expr>),
    /// Bitwise complement: `~x` (integer-valued, unlike the boolean [`Expr::Not`])
    BitwiseNot(Box<Expr>),
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

#[derive(Debug, Clone, PartialEq, Eq)]
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
    BitwiseOr,  // | bitwise OR
    BitwiseXor, // ^ bitwise XOR
    BitwiseAnd, // & bitwise AND
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
