//! Generates the TypeScript indicator modules: one ES module per indicator under
//! `ta_codegen/output/ts/library/src/func/`, exporting free functions.

use std::cell::{Cell, RefCell};
use std::collections::{BTreeMap, BTreeSet, HashMap, HashSet};
use std::fmt::Write as _;
use std::path::Path;

use crate::candle_settings::detect_candle_settings;
use crate::helper_registry::{hoist_block_helpers, try_inline_expr, HelperRegistry};
use crate::ir::{
    BinOp, CircBuf, EnumDef, Expr, FuncDef, Lang, LookbackExpr, OptInput, Output, ParamType, Statement, Tier,
    VarType,
};
use crate::parser::enums::lookup_variant;
use crate::registry::Registry;

use super::builtins::{MathFn, SpecialBuiltin, StdlibFn};
use super::common::{
    camel_words, contains_alloc_err_return, expr_directly_contains_candle_call, find_sizeof_type, pascal_words,
};
use super::ctype::{CType, IntFix, TypeEnv};
use super::expr_walk::{binop_prec, expr_prec, wrap_child, wrap_inlined, ExprEmitter, ATOMIC_PREC};
use super::fma::{self, FmaVarSets};
use super::java::{
    bool_ternary_collapse, build_matype_map, circbuf_arrays, collect_address_of_vars,
    collect_double_address_of_vars, cross_call_split, unst_variant_name, BoolTernaryCollapse, JAVA_CANDLE_FNS,
};
use super::stmt_walk::StatementEmitter;

/// Words this backend cannot render as an identifier (see [`crate::naming`]):
/// the ECMAScript reserved words, the strict-mode future reserved words (every
/// module is strict), the binding names strict mode forbids (`arguments`,
/// `eval`), and the global value names a local could shadow without an error.
pub(crate) const RESERVED_WORDS: &[&str] = &[
    "arguments", "await", "break", "case", "catch", "class", "const", "continue",
    "debugger", "default", "delete", "do", "else", "enum", "eval", "export",
    "extends", "false", "finally", "for", "function", "if", "implements",
    "import", "in", "Infinity", "instanceof", "interface", "let", "NaN", "new",
    "null", "package", "private", "protected", "public", "return", "static",
    "super", "switch", "this", "throw", "true", "try", "typeof", "undefined",
    "var", "void", "while", "with", "yield",
];

/// `name` as a TS binding: a reserved word gains a trailing `_`. Function names
/// are the case that needs it: `VAR` is a C API name that cannot change, and its
/// lowerCamel `var` is a hard keyword in JS where Java and C# treat it as
/// contextual.
#[must_use]
pub fn ts_ident(name: &str) -> String {
    if RESERVED_WORDS.contains(&name) {
        format!("{name}_")
    } else {
        name.to_string()
    }
}

/// The public batch function's binding name.
#[must_use]
pub fn func_ident(func_name: &str) -> String {
    ts_ident(&camel_words(func_name))
}

/// Every symbol a function module exports, in one place: the emitter, the
/// barrel, the server and the metadata registry all spell them from here.
pub struct TsNames {
    /// File stem under `src/func/` (`var`).
    pub module: String,
    /// Public batch function (`var_`): `(inputs.., options?) => <N>Result`.
    pub public: String,
    /// `@internal` C-shaped batch tier (`varPositional`): validates, copies
    /// non-`Float64Array` input, throws `TALibError`, returns `OutRange`.
    pub positional: String,
    /// `@internal` body (`varImpl`): `(core, startIdx, endIdx, ins.., opts..,
    /// outBegIdx, outNBElement, outs..) => RetCode`.
    pub body: String,
    /// `(options?) => number`.
    pub lookback: String,
    /// `@internal` `(opts.., core = Core.DEFAULT) => number`.
    pub lookback_positional: String,
    /// `(outputIdx, options?) => number`.
    pub display_shift: String,
    /// `@internal` `(opts.., outputIndex, core = Core.DEFAULT) => number`.
    pub display_shift_positional: String,
    /// The batch call's options interface (`SmaOptions`).
    pub options_type: String,
    /// The batch call's result interface (`SmaResult`).
    pub result_type: String,
    /// Streaming handle class (`SmaStream`).
    pub stream_class: String,
    /// Sink a multi-output handle writes and returns (`MacdOut`).
    pub out_class: String,
    /// `(inputs.., options?) => <N>Stream`.
    pub open: String,
    /// `@internal` `(ins.., opts.., core = Core.DEFAULT) => <N>Stream`.
    pub open_positional: String,
    /// `(inputs.., options?) => <N>Result & { stream }`.
    pub open_and_fill: String,
    /// `@internal` `(ins.., opts.., outs.., core = Core.DEFAULT) => <N>Stream`.
    pub open_and_fill_positional: String,
    /// `@internal` `(core, ins.., startIdx, opts..) => <N>Stream`.
    pub open_internal: String,
    /// `@internal` `(core, ins.., startIdx, opts.., outBegIdx, outNBElement, outs..) => <N>Stream`.
    pub open_and_fill_internal: String,
}

#[must_use]
pub fn ts_names(func_name: &str) -> TsNames {
    let base = camel_words(func_name);
    let pascal = pascal_words(func_name);
    TsNames {
        module: base.clone(),
        public: ts_ident(&base),
        positional: format!("{base}Positional"),
        body: format!("{base}Impl"),
        lookback: format!("{base}Lookback"),
        lookback_positional: format!("{base}LookbackPositional"),
        display_shift: format!("{base}DisplayShift"),
        display_shift_positional: format!("{base}DisplayShiftPositional"),
        options_type: format!("{pascal}Options"),
        result_type: format!("{pascal}Result"),
        stream_class: format!("{pascal}Stream"),
        out_class: format!("{pascal}Out"),
        open: format!("{base}Open"),
        open_positional: format!("{base}OpenPositional"),
        open_and_fill: format!("{base}OpenAndFill"),
        open_and_fill_positional: format!("{base}OpenAndFillPositional"),
        open_internal: format!("{base}OpenInternal"),
        open_and_fill_internal: format!("{base}OpenAndFillInternal"),
    }
}

/// The words of a C identifier: `_` separates, a capital starts a word unless
/// it continues an acronym, and digits stay with the word before them
/// (`optInSlowK_MAType` is `optIn Slow K MA Type`, `outWT1` is `out WT1`).
fn c_words(name: &str) -> Vec<String> {
    let chars: Vec<char> = name.chars().collect();
    let mut words: Vec<String> = Vec::new();
    let mut cur = String::new();
    for (i, &c) in chars.iter().enumerate() {
        if c == '_' {
            if !cur.is_empty() {
                words.push(std::mem::take(&mut cur));
            }
            continue;
        }
        if c.is_ascii_uppercase() && !cur.is_empty() {
            let prev = chars[i - 1];
            let next_lower = chars.get(i + 1).is_some_and(char::is_ascii_lowercase);
            if prev.is_ascii_lowercase() || prev.is_ascii_digit() || (prev.is_ascii_uppercase() && next_lower) {
                words.push(std::mem::take(&mut cur));
            }
        }
        cur.push(c);
    }
    if !cur.is_empty() {
        words.push(cur);
    }
    // `ROCMAType` names an MA type like `FastMAType`: case cannot split two
    // acronyms, so the `MA` before `Type` is split off by name.
    let mut k = 0;
    while k + 1 < words.len() {
        let w = &words[k];
        if words[k + 1] == "Type" && w.len() > 2 && w.ends_with("MA") && w.chars().all(|c| c.is_ascii_uppercase()) {
            let head = w[..w.len() - 2].to_string();
            words[k] = "MA".to_string();
            words.insert(k, head);
            k += 1;
        }
        k += 1;
    }
    words
}

fn camel_join(words: &[String]) -> String {
    let mut out = String::new();
    for (i, w) in words.iter().enumerate() {
        let lower = w.to_ascii_lowercase();
        if i == 0 {
            out.push_str(&lower);
        } else {
            let mut cs = lower.chars();
            if let Some(f) = cs.next() {
                out.push(f.to_ascii_uppercase());
                out.push_str(cs.as_str());
            }
        }
    }
    out
}

/// An optional parameter's options key: `optInFastMAType` is `fastMaType`.
#[must_use]
pub fn param_key(opt_name: &str) -> String {
    let rest = opt_name.strip_prefix("optIn").unwrap_or(opt_name);
    camel_join(&c_words(rest))
}

/// An output's name in a multi-output result, a buffer key and a sink field:
/// `out` kept, the type word dropped when other words follow
/// (`outRealUpperBand` is `outUpperBand`, `outMACDSignal` is `outMacdSignal`).
#[must_use]
pub fn output_key(out_name: &str) -> String {
    let mut words = c_words(out_name);
    if words.len() > 2 && matches!(words[1].as_str(), "Real" | "Integer") {
        words.remove(1);
    }
    camel_join(&words)
}

/// The options key of an output buffer: a single output keeps its C name.
#[must_use]
pub fn buffer_key(func: &FuncDef, out: &Output) -> String {
    if func.outputs.len() == 1 {
        out.name.clone()
    } else {
        output_key(&out.name)
    }
}

/// An output's key in the batch result: `values` for a single output.
#[must_use]
pub fn result_key(func: &FuncDef, out: &Output) -> String {
    if func.outputs.len() == 1 {
        "values".to_string()
    } else {
        output_key(&out.name)
    }
}

/// A plain input's parameter name: `inReal0` is `real0`.
#[must_use]
pub fn input_param_name(input_name: &str) -> String {
    let rest = input_name.strip_prefix("in").unwrap_or(input_name);
    let mut cs = rest.chars();
    let name = cs.next().map_or_else(String::new, |f| f.to_ascii_lowercase().to_string() + cs.as_str());
    ts_ident(&name)
}

/// The name of the one object parameter a price group is passed as.
pub const PRICES_PARAM: &str = "prices";

/// One positional parameter of the idiomatic API: a plain input, or a price
/// group as one object.
pub struct ApiInput<'a> {
    pub name: String,
    /// The signature's type.
    pub ty: String,
    /// The expanded inputs it carries, each with the expression that reads it.
    pub legs: Vec<(&'a crate::ir::Input, String)>,
    pub is_price: bool,
}

/// `func`'s inputs as the idiomatic API takes them, in the Abstract API's
/// grouping.
///
/// # Panics
/// On two price groups, which would need two names.
#[must_use]
pub fn api_inputs(func: &FuncDef) -> Vec<ApiInput<'_>> {
    use super::price_bundle::{group, Grouped};
    let mut out = Vec::new();
    for g in group(&func.inputs) {
        match g {
            Grouped::Single(i) => {
                let name = input_param_name(&i.name);
                out.push(ApiInput { ty: "ArrayLike<number>".to_string(), legs: vec![(i, name.clone())], name, is_price: false });
            }
            Grouped::Price(comps) => {
                assert!(!out.iter().any(|a: &ApiInput| a.is_price), "{}: two price groups", func.name);
                let mut keys = Vec::new();
                let mut legs = Vec::new();
                for i in comps {
                    let key = i.price.expect("a price component").component.token();
                    keys.push(format!("{key}: ArrayLike<number>"));
                    legs.push((i, format!("{PRICES_PARAM}.{key}")));
                }
                out.push(ApiInput { name: PRICES_PARAM.to_string(), ty: format!("{{ {} }}", keys.join("; ")), legs, is_price: true });
            }
        }
    }
    let names: Vec<&str> = out.iter().map(|a| a.name.as_str()).collect();
    assert!(
        names.iter().enumerate().all(|(k, n)| !names[..k].contains(n) && !matches!(*n, "options" | "o")),
        "{}: input parameter names collide: {names:?}",
        func.name
    );
    out
}

/// Every options key and every result key of `func` is distinct.
///
/// # Panics
/// When two collide: a parameter, a range key, `core` or a buffer key; or two
/// outputs, `begIdx` or `stream` in the result.
pub fn assert_unique_keys(func: &FuncDef) {
    let mut keys: Vec<String> = func.optional_inputs.iter().map(|o| param_key(&o.name)).collect();
    keys.extend(["startIdx", "endIdx", "core"].map(String::from));
    keys.extend(func.outputs.iter().map(|o| buffer_key(func, o)));
    for (k, key) in keys.iter().enumerate() {
        assert!(!keys[..k].contains(key), "{}: options key `{key}` is not unique", func.name);
    }
    let mut results: Vec<String> = func.outputs.iter().map(|o| result_key(func, o)).collect();
    results.extend(["begIdx", "stream"].map(String::from));
    for (k, key) in results.iter().enumerate() {
        assert!(!results[..k].contains(key), "{}: result key `{key}` is not unique", func.name);
    }
}

/// The module file stem of an indicator: its lowerCamel name.
#[must_use]
pub fn module_stem(func: &FuncDef) -> String {
    camel_words(&func.name)
}

type CallImports = RefCell<BTreeMap<String, BTreeSet<String>>>;

/// One indicator module: the `@internal` positional tiers and body, then the
/// public functions over them.
#[allow(clippy::implicit_hasher)]
#[must_use]
pub fn generate(
    func: &FuncDef,
    enums: &HashMap<String, EnumDef>,
    registry: &Registry,
    helpers: &HelperRegistry,
) -> String {
    let resolved = func.resolved_for(Lang::Ts);
    let func: &FuncDef = &resolved;
    let module = module_stem(func);
    let calls: CallImports = RefCell::new(BTreeMap::new());
    let svc = Services { enums, registry, helpers, calls: &calls, module: &module };

    let mut body = String::new();
    body.push_str(&gen_lookback(func, &svc));
    body.push_str(&gen_display_shift(func, &svc));
    if func.has_explicit_private {
        body.push_str(&gen_impl(func, true, &svc));
    }
    body.push_str(&gen_impl(func, false, &svc));
    body.push_str(&gen_param_ranges(func, enums));
    body.push_str(&gen_public(func));
    body.push_str(&gen_api_types(func, &svc));
    body.push_str(&gen_api_lookback(func));
    body.push_str(&gen_api_display_shift(func));
    body.push_str(&gen_api_batch(func, &svc));
    if super::ts_stream::emits_stream(func, registry) {
        body.push_str(&super::ts_stream::generate(func, &svc));
    }
    let body = super::ts_prune::prune(&body);

    let mut out = String::new();
    for block in &func.header_comments {
        out.push_str(&super::stmt_walk::block_comment(block, 0));
        out.push('\n');
    }
    out.push_str(&render_imports(&body, &calls.borrow(), &module));
    if let Some(m) = func.alt_marker(Tier::Batch, Lang::Ts) {
        let _ = write!(out, "\n/* {m} */\n");
    }
    out.push_str(&body);
    out
}

const RUNTIME_IMPORTS: &[(&str, &str)] = &[
    ("Core", "../runtime/core.ts"),
    ("CandleSettingType", "../runtime/core.ts"),
    ("RetCode", "../runtime/errors.ts"),
    ("MInteger", "../runtime/mInteger.ts"),
    ("OutRange", "../runtime/outRange.ts"),
    ("fma", "../runtime/fma.ts"),
    ("fmaSlow", "../runtime/fma.ts"),
    ("clampedStart", "../runtime/checks.ts"),
    ("failure", "../runtime/checks.ts"),
    ("float64", "../runtime/checks.ts"),
    ("requireIndexRange", "../runtime/checks.ts"),
    ("requireInt32", "../runtime/checks.ts"),
    ("requireReal", "../runtime/checks.ts"),
    ("requireLength", "../runtime/checks.ts"),
    ("requireOutput", "../runtime/checks.ts"),
    ("requireArgument", "../runtime/checks.ts"),
    ("requireSeries", "../runtime/checks.ts"),
    ("coreOf", "../runtime/checks.ts"),
    ("requireHistory", "../runtime/checks.ts"),
    ("requireHistoryLength", "../runtime/checks.ts"),
    ("openFillCount", "../runtime/checks.ts"),
    ("insufficientHistory", "../runtime/checks.ts"),
    ("streamFailure", "../runtime/checks.ts"),
    ("nonFiniteBar", "../runtime/checks.ts"),
    ("badSink", "../runtime/checks.ts"),
    ("overlaps", "../runtime/checks.ts"),
    ("overlapsPartly", "../runtime/checks.ts"),
    ("sharedMemory", "../runtime/checks.ts"),
    ("rejectedParams", "../runtime/checks.ts"),
    ("optionsOf", "../runtime/checks.ts"),
    ("orDefault", "../runtime/checks.ts"),
    ("batchCapacity", "../runtime/checks.ts"),
    ("fillCapacity", "../runtime/checks.ts"),
    ("MAType", "../enums.ts"),
    ("MATypes", "../enums.ts"),
    ("FuncUnstId", "../enums.ts"),
];

fn mentions(text: &str, word: &str) -> bool {
    let is_ident = |c: char| c.is_ascii_alphanumeric() || c == '_' || c == '$';
    text.match_indices(word).any(|(i, _)| {
        let before = text[..i].chars().next_back();
        let after = text[i + word.len()..].chars().next();
        !before.is_some_and(is_ident) && !after.is_some_and(is_ident) && before != Some('.')
    })
}

fn render_imports(body: &str, calls: &BTreeMap<String, BTreeSet<String>>, own: &str) -> String {
    let code: String = strip_ts_comments(body);
    let mut by_path: BTreeMap<&str, Vec<&str>> = BTreeMap::new();
    for (sym, path) in RUNTIME_IMPORTS {
        if mentions(&code, sym) {
            by_path.entry(path).or_default().push(sym);
        }
    }
    let mut out = String::new();
    for (path, mut syms) in by_path {
        syms.sort_unstable();
        let _ = writeln!(out, "import {{ {} }} from '{path}';", syms.join(", "));
    }
    let mut aliases = String::new();
    for (module, syms) in calls {
        if module == own {
            for (sym, alias) in syms.iter().filter_map(|e| e.split_once(" as ")) {
                let _ = writeln!(aliases, "const {alias} = {sym};");
            }
            continue;
        }
        let syms: Vec<&str> = syms.iter().map(String::as_str).collect();
        let _ = writeln!(out, "import {{ {} }} from './{module}.ts';", syms.join(", "));
    }
    out.push_str(&aliases);
    out
}

/// `text` without its `/* */` and `//` comments, so a word in a carried source
/// comment imports nothing.
fn strip_ts_comments(text: &str) -> String {
    let mut out = String::with_capacity(text.len());
    let mut rest = text;
    while !rest.is_empty() {
        let block = rest.find("/*");
        let line = rest.find("//");
        match (block, line) {
            (Some(b), l) if l.is_none_or(|l| b < l) => {
                out.push_str(&rest[..b]);
                rest = rest[b + 2..].find("*/").map_or("", |e| &rest[b + 2 + e + 2..]);
            }
            (_, Some(l)) => {
                out.push_str(&rest[..l]);
                rest = rest[l..].find('\n').map_or("", |e| &rest[l + e..]);
            }
            _ => {
                out.push_str(rest);
                rest = "";
            }
        }
    }
    out
}

pub(crate) struct Services<'a> {
    pub(crate) enums: &'a HashMap<String, EnumDef>,
    pub(crate) registry: &'a Registry,
    pub(crate) helpers: &'a HelperRegistry,
    calls: &'a CallImports,
    pub(crate) module: &'a str,
}

impl Services<'_> {
    /// Imports `sym` from the module of registry key `key`.
    pub(crate) fn import(&self, key: &str, sym: &str) {
        let module = camel_words(&self.registry.name_of(key));
        if module != self.module {
            self.calls.borrow_mut().entry(module).or_default().insert(sym.to_string());
        }
    }
}

pub(crate) struct TsCtx<'a> {
    pub(crate) svc: &'a Services<'a>,
    pub(crate) env: TypeEnv<'a>,
    /// `int` locals passed by address: `MInteger`.
    pub(crate) address_of_vars: HashSet<String>,
    /// `double` locals passed by address: a one-element `Float64Array`.
    pub(crate) double_address_of_vars: HashSet<String>,
    /// Set while an address-of argument renders, so a boxed local names its box.
    raw_refs: Cell<bool>,
    own_counter: Cell<usize>,
    /// Shared by every context rendering into one function scope, so two
    /// hoisted temps never take one name.
    shared_counter: Option<&'a Cell<usize>>,
    fma: Option<FmaVarSets>,
    fma_ref: Option<&'a FmaVarSets>,
    fma_inline: Cell<FmaInline>,
    loop_depth: Cell<usize>,
    /// Set while a `for` header renders, where a block cannot go.
    in_for_header: Cell<bool>,
    matype_map: HashMap<String, String>,
    /// The C return type of the function being rendered.
    pub(crate) ret: CType,
}

const INT_MIN: &str = "-2147483648";

impl<'a> TsCtx<'a> {
    fn new(svc: &'a Services<'a>, body: &[Statement], outputs: &[Output], fuse: bool) -> Self {
        let mut address_of_vars = collect_address_of_vars(body);
        let double_address_of_vars = collect_double_address_of_vars(body, &address_of_vars);
        for n in &double_address_of_vars {
            address_of_vars.remove(n);
        }
        let env = TypeEnv::new(svc.helpers, svc.registry, svc.enums);
        env.seed_decls(body);
        env.seed_candles(&detect_candle_settings(body));
        let fma = fuse.then(|| fma::build_fma_var_sets(body, outputs, &fma::INDEX_PARAM_SEEDS));
        let fma_inline = fma.as_ref().map_or(FmaInline::Off, |fs| fma_inline_mode(body, fs, FmaInline::InLoops));
        TsCtx {
            svc,
            env,
            address_of_vars,
            double_address_of_vars,
            raw_refs: Cell::new(false),
            own_counter: Cell::new(0),
            shared_counter: None,
            fma,
            fma_ref: None,
            fma_inline: Cell::new(fma_inline),
            loop_depth: Cell::new(0),
            in_for_header: Cell::new(false),
            matype_map: build_matype_map(svc.enums),
            ret: CType::Int32,
        }
    }

    /// A context for stream code: the caller declares every name it renders,
    /// and the fused sites come from the stream source's own sets.
    pub(crate) fn for_stream(
        svc: &'a Services<'a>,
        body: &[Statement],
        fma: &'a FmaVarSets,
        counter: &'a Cell<usize>,
    ) -> Self {
        let mut address_of_vars = collect_address_of_vars(body);
        let double_address_of_vars = collect_double_address_of_vars(body, &address_of_vars);
        for n in &double_address_of_vars {
            address_of_vars.remove(n);
        }
        let env = TypeEnv::new(svc.helpers, svc.registry, svc.enums);
        env.seed_decls(body);
        TsCtx {
            svc,
            env,
            address_of_vars,
            double_address_of_vars,
            raw_refs: Cell::new(false),
            own_counter: Cell::new(0),
            shared_counter: Some(counter),
            fma: None,
            fma_ref: Some(fma),
            fma_inline: Cell::new(fma_inline_mode(body, fma, FmaInline::InLoops)),
            loop_depth: Cell::new(0),
            in_for_header: Cell::new(false),
            matype_map: build_matype_map(svc.enums),
            ret: CType::Int32,
        }
    }

    fn counter(&self) -> &Cell<usize> {
        self.shared_counter.unwrap_or(&self.own_counter)
    }

    fn fma_sets(&self) -> Option<&FmaVarSets> {
        self.fma_ref.or(self.fma.as_ref())
    }

    /// For a frame that runs once per bar as a whole, `body` being all of it:
    /// a stream step or peek.
    pub(crate) fn inline_fma_per_bar(&self, body: &[Statement]) {
        if let Some(fs) = self.fma_sets() {
            self.fma_inline.set(fma_inline_mode(body, fs, FmaInline::Everywhere));
        }
    }

    fn inlines_fma_here(&self) -> bool {
        !self.in_for_header.get()
            && match self.fma_inline.get() {
                FmaInline::Off => false,
                FmaInline::InLoops => self.loop_depth.get() > 0,
                FmaInline::Everywhere => true,
            }
    }

    fn in_loop<T>(&self, f: impl FnOnce() -> T) -> T {
        self.loop_depth.set(self.loop_depth.get() + 1);
        let r = f();
        self.loop_depth.set(self.loop_depth.get() - 1);
        r
    }

    /// `value` with every fused site it evaluates unconditionally taken out,
    /// innermost first; `None` when it has none, or when evaluating them ahead
    /// of the rest of the statement could change what it reads.
    fn take_fma_sites(&self, target: Option<&Expr>, value: &Expr) -> Option<(Expr, Vec<FmaSite>)> {
        if !fma::EMIT_FMA || !self.inlines_fma_here() || !reorder_safe(target, value) {
            return None;
        }
        let fs = self.fma_sets()?.view();
        let mut sites = Vec::new();
        let rest = self.take_sites(value, &fs, &mut sites);
        (!sites.is_empty()).then_some((rest, sites))
    }

    fn take_sites(&self, e: &Expr, fs: &fma::FmaCtx, sites: &mut Vec<FmaSite>) -> Expr {
        let take = |x: &Expr, sites: &mut Vec<FmaSite>| Box::new(self.take_sites(x, fs, sites));
        match e {
            Expr::BinOp(left, op, right) => {
                if let Some(operands) = fma::fuse_operands(left, op, right, fs) {
                    let a = *take(operands.0, sites);
                    let b = *take(operands.1, sites);
                    let c = *take(operands.2, sites);
                    let id = self.counter().get();
                    self.counter().set(id + 1);
                    let temp = format!("_fma{id}");
                    self.env.declare(&temp, CType::Real);
                    sites.push(FmaSite { temp: temp.clone(), a, b, c });
                    return Expr::Var(temp);
                }
                if matches!(op, BinOp::And | BinOp::Or) {
                    return e.clone();
                }
                let left = take(left, sites);
                Expr::BinOp(left, op.clone(), take(right, sites))
            }
            Expr::ArrayAccess(name, idx) => Expr::ArrayAccess(name.clone(), take(idx, sites)),
            Expr::Cast(t, inner) => Expr::Cast(t.clone(), take(inner, sites)),
            Expr::Not(inner) => Expr::Not(take(inner, sites)),
            Expr::Neg(inner) => Expr::Neg(take(inner, sites)),
            _ => e.clone(),
        }
    }

    /// Each site as a block running `fma`'s fast path; the last stores straight
    /// into `direct` when one is given.
    fn fma_blocks(&self, sites: &[FmaSite], direct: Option<&str>, indent: usize) -> String {
        let pad = " ".repeat(indent);
        let mut out = String::new();
        for (i, s) in sites.iter().enumerate() {
            let dest = match direct {
                Some(d) if i + 1 == sites.len() => d.to_string(),
                _ => {
                    let _ = writeln!(out, "{pad}let {}: number;", s.temp);
                    s.temp.clone()
                }
            };
            out.push_str(&fma_fast_path_block(&self.num(&s.a), &self.num(&s.b), &self.num(&s.c), &dest, &pad));
        }
        out
    }

    fn ty(&self, e: &Expr) -> CType {
        self.env.type_of(e)
    }

    pub(crate) fn expr(&self, e: &Expr) -> String {
        TsExpr { c: self }.walk(e)
    }

    /// `e` as a JS number: a `Bool` becomes 0 or 1.
    pub(crate) fn num(&self, e: &Expr) -> String {
        if self.ty(e) == CType::Bool {
            format!("({} ? 1 : 0)", self.expr(e))
        } else {
            self.expr(e)
        }
    }

    /// `e` as a JS boolean: C's truth test of a number is `!== 0`.
    pub(crate) fn cond(&self, e: &Expr) -> String {
        let t = self.ty(e);
        assert!(!t.is_pointer(), "TS: pointer used as a condition: {e:?}");
        if t == CType::Bool {
            self.expr(e)
        } else {
            let s = self.expr(e);
            let s = if expr_prec(e) <= binop_prec(&BinOp::Eq) { format!("({s})") } else { s };
            format!("{s} !== 0")
        }
    }

    /// `e` stored into a scalar of type `dest`, with C's implicit conversion.
    pub(crate) fn value_for(&self, dest: CType, e: &Expr) -> String {
        if self.env.implicit_trunc(dest, e) {
            let s = self.expr(e);
            let s = if expr_prec(e) <= binop_prec(&BinOp::BitwiseOr) { format!("({s})") } else { s };
            format!("({s} | 0)")
        } else if dest.is_pointer() {
            self.expr(e)
        } else {
            self.num(e)
        }
    }

    /// `e` as an operand of `op`, parenthesized where the JS grammar needs it.
    fn operand(&self, e: &Expr, op: &BinOp, is_right: bool) -> String {
        let pp = binop_prec(op);
        if matches!(op, BinOp::And | BinOp::Or) {
            if self.ty(e) != CType::Bool {
                return self.cond(e);
            }
            return wrap_child(self.expr(e), e, pp, is_right);
        }
        if self.ty(e) == CType::Bool && !matches!(op, BinOp::Eq | BinOp::NotEq) {
            return self.num(e);
        }
        wrap_child(self.expr(e), e, pp, is_right)
    }

    fn declare_hoisted(&self, hoisted: &[(String, VarType, Vec<Statement>)]) {
        for (temp, vt, body) in hoisted {
            self.env.declare(temp, CType::of_var_type(vt));
            self.env.seed_decls(body);
        }
    }

    pub(crate) fn hoist(&self, e: &Expr) -> (Expr, Vec<(String, VarType, Vec<Statement>)>) {
        let mut hoisted = Vec::new();
        let mut cnt = self.counter().get();
        let new = hoist_block_helpers(e, self.svc.helpers, &mut hoisted, &mut cnt, JAVA_CANDLE_FNS);
        self.counter().set(cnt);
        self.declare_hoisted(&hoisted);
        (new, hoisted)
    }

    pub(crate) fn stmt(&self, s: &Statement, indent: usize) -> String {
        TsStmt { c: self }.walk_stmt(s, indent)
    }
}

/// Where a context renders fused sites as `fma`'s fast path copied in rather
/// than as calls.
#[derive(Clone, Copy, PartialEq, Eq)]
enum FmaInline {
    Off,
    InLoops,
    Everywhere,
}

/// V8 stops inlining calls into one optimized function once their bytecode
/// totals about 920 bytes: three calls of `fma`. A batch body with this many
/// fused sites in its loops, or a stream step or peek with this many in all,
/// gets the fast path copied in, so none is left a call that boxes its
/// operands and result. Fewer stay calls: the copy makes a step too big for
/// `update` to inline, a call per bar where V8 would have inlined them all.
const FMA_INLINE_MIN_SITES: usize = 3;

struct FmaSite {
    temp: String,
    a: Expr,
    b: Expr,
    c: Expr,
}

/// `mode` when `body` has [`FMA_INLINE_MIN_SITES`] fused sites in the code it
/// covers: loop bodies for `InLoops`, all of it for `Everywhere`.
fn fma_inline_mode(body: &[Statement], fs: &FmaVarSets, mode: FmaInline) -> FmaInline {
    let n = count_fused_sites(body, &fs.view(), mode == FmaInline::Everywhere);
    if n >= FMA_INLINE_MIN_SITES {
        mode
    } else {
        FmaInline::Off
    }
}

fn count_fused_sites(stmts: &[Statement], fs: &fma::FmaCtx, counted: bool) -> usize {
    let mut n = 0;
    let mut sites_in = |e: &Expr| {
        crate::streaming::walk_expr(e, &mut |x| {
            if let Expr::BinOp(l, op, r) = x {
                if fma::fuse_operands(l, op, r, fs).is_some() {
                    n += 1;
                }
            }
        });
    };
    let mut nested = 0;
    for s in stmts {
        match s {
            Statement::While { body, .. }
            | Statement::DoWhile { body, .. }
            | Statement::For { body, .. }
            | Statement::ForC { body, .. } => nested += count_fused_sites(body, fs, true),
            Statement::If { condition, then_body, else_body, .. } => {
                if counted {
                    sites_in(condition);
                }
                nested += count_fused_sites(then_body, fs, counted) + count_fused_sites(else_body, fs, counted);
            }
            Statement::Switch { cases, default, .. } => {
                for body in cases.iter().map(|(_, b)| b).chain([default]) {
                    nested += count_fused_sites(body, fs, counted);
                }
            }
            Statement::Block { body } => nested += count_fused_sites(body, fs, counted),
            _ if counted => crate::streaming::walk_stmt_exprs(s, &mut sites_in),
            _ => {}
        }
    }
    n + nested
}

/// Whether a statement's fused sites can run ahead of the rest of it: no call,
/// and at most one increment or decrement, of a variable read nowhere else.
fn reorder_safe(target: Option<&Expr>, value: &Expr) -> bool {
    let mut call = false;
    let mut stepped = Vec::new();
    let mut reads: HashMap<String, usize> = HashMap::new();
    for e in target.into_iter().chain([value]) {
        crate::streaming::walk_expr(e, &mut |x| match x {
            Expr::FuncCall(..) => call = true,
            Expr::PostIncrement(i) | Expr::PostDecrement(i) | Expr::PreIncrement(i) | Expr::PreDecrement(i) => {
                stepped.push(i.as_ref().clone());
            }
            Expr::Var(n) | Expr::ArrayAccess(n, _) | Expr::PointerDeref(n) => *reads.entry(n.clone()).or_default() += 1,
            _ => {}
        });
    }
    !call
        && match stepped.as_slice() {
            [] => true,
            [Expr::Var(v)] => reads.get(v) == Some(&1),
            _ => false,
        }
}

/// The runtime's `fma`, whose body every inlined fused site runs: editing it
/// and regenerating changes both.
const FMA_TS: &str = include_str!("../../../output/ts/library/src/runtime/fma.ts");

fn fma_fast_path() -> Vec<&'static str> {
    let head = "export function fma(a: number, b: number, c: number): number {\n";
    let start = FMA_TS.find(head).unwrap_or_else(|| panic!("runtime/fma.ts has no `{}`", head.trim_end())) + head.len();
    FMA_TS[start..].lines().take_while(|l| *l != "}").collect()
}

/// `fma(a, b, c)` stored into `dest`, as a block running [`fma_fast_path`]
/// with its parameters and locals under a `_`.
fn fma_fast_path_block(a: &str, b: &str, c: &str, dest: &str, pad: &str) -> String {
    let mut locals = BTreeSet::new();
    let mut lines = Vec::new();
    for line in fma_fast_path() {
        let line = line.strip_prefix("  ").unwrap_or(line);
        lines.push(match line.strip_prefix("return ") {
            Some(rest) => format!("{dest} = {}", local_names_prefixed(rest, &mut locals)),
            None => local_names_prefixed(line, &mut locals),
        });
    }
    for text in [a, b, c, dest] {
        let mut seen = BTreeSet::new();
        local_names_prefixed(text, &mut seen);
        let clash: Vec<&str> = seen.iter().filter_map(|n| n.strip_prefix('_')).filter(|n| locals.contains(*n)).collect();
        assert!(clash.is_empty(), "an fma operand names {clash:?}, which the inlined fast path shadows");
    }
    let mut out = format!("{pad}{{\n{pad}  const _a: number = {a}, _b: number = {b}, _c: number = {c};\n");
    for line in lines {
        let _ = writeln!(out, "{pad}  {line}");
    }
    let _ = writeln!(out, "{pad}}}");
    out
}

/// `text` with every name but `const`, `let` and `fmaSlow` given a leading
/// `_`; the prefixed names go into `names`.
fn local_names_prefixed(text: &str, names: &mut BTreeSet<String>) -> String {
    let is_ident = |x: char| x.is_ascii_alphanumeric() || x == '_';
    let mut out = String::with_capacity(text.len() + 32);
    let mut rest = text;
    while let Some(c) = rest.chars().next() {
        let len = if is_ident(c) { rest.find(|x| !is_ident(x)).unwrap_or(rest.len()) } else { c.len_utf8() };
        let tok = &rest[..len];
        if (c.is_ascii_alphabetic() || c == '_') && !matches!(tok, "const" | "let" | "fmaSlow") {
            names.insert(format!("_{tok}"));
            out.push('_');
        }
        out.push_str(tok);
        rest = &rest[len..];
    }
    out
}

fn ts_scalar_decl(t: CType) -> &'static str {
    match t {
        CType::RealPtr => "Float64Array",
        CType::IntPtr => "Int32Array",
        _ => "number",
    }
}

/// `let` declaration of a local with no initializer of its own. Scalars start
/// at 0 so the compiler's definite-assignment analysis never decides for C.
pub(crate) fn decl_line(name: &str, vt: &VarType, ctx: &TsCtx) -> String {
    if ctx.address_of_vars.contains(name) && matches!(vt, VarType::Integer | VarType::Index) {
        return format!("const {name} = new MInteger();");
    }
    if ctx.double_address_of_vars.contains(name) {
        return format!("const {name} = new Float64Array(1);");
    }
    match vt {
        VarType::Real | VarType::Integer | VarType::Index => format!("let {name} = 0;"),
        VarType::RetCodeType => format!("let {name}: RetCode = RetCode.SUCCESS;"),
        VarType::RealArray(n) => format!("const {name} = new Float64Array({n});"),
        VarType::IntArray(n) => format!("const {name} = new Int32Array({n});"),
        VarType::RealPointer => format!("let {name}!: Float64Array;"),
        VarType::IntPointer => format!("let {name}!: Int32Array;"),
    }
}

pub(crate) fn opt_ts_type(opt: &OptInput) -> String {
    match &opt.param_type {
        ParamType::Real | ParamType::Integer => "number".to_string(),
        ParamType::Enum(name) => format!("{name} | typeof Core.INTEGER_DEFAULT"),
        ParamType::Price(_) => unreachable!("Price expanded during parsing"),
    }
}

pub(crate) fn array_ts_type(t: &ParamType) -> &'static str {
    if *t == ParamType::Real {
        "Float64Array"
    } else {
        "Int32Array"
    }
}

/// Each optional parameter's default and range checks; `fail` is what a
/// rejection returns. An enum parameter is a number at run time, so it is
/// range-checked as C checks it.
#[allow(clippy::cast_possible_truncation)]
pub(crate) fn emit_opt_param_validation(func: &FuncDef, fail: &str, enums: &HashMap<String, EnumDef>) -> String {
    let mut out = String::new();
    for opt in &func.optional_inputs {
        let Some(default_val) = opt.default else { continue };
        let name = &opt.name;
        match &opt.param_type {
            ParamType::Integer => {
                let _ = write!(out, "  if ({name} === Core.INTEGER_DEFAULT) {{\n    {name} = {};\n  }}", default_val as i32);
                if let Some((min, max)) = opt.range {
                    let _ = write!(out, " else if ({name} < {} || {name} > {}) {{\n    return {fail};\n  }}", min as i32, max as i32);
                }
                out.push('\n');
            }
            ParamType::Real => {
                let lit = ts_literal(default_val);
                let _ = write!(out, "  if ({name} === Core.REAL_DEFAULT) {{\n    {name} = {lit};\n  }}");
                if let Some((min, max)) = opt.range {
                    let cond = super::common::real_range_reject(
                        name,
                        &super::common::real_bound_literal(min, "Core."),
                        &super::common::real_bound_literal(max, "Core."),
                        false,
                    );
                    let _ = write!(out, " else if ({cond}) {{\n    return {fail};\n  }}");
                }
                out.push('\n');
            }
            ParamType::Enum(enum_name) => {
                let extra = super::common::enum_default_variant(enums, enum_name)
                    .map(|v| format!(" || {name} === {enum_name}.{}", v.name))
                    .unwrap_or_default();
                let val = super::common::enum_member_literal(enums, enum_name, default_val as i32)
                    .unwrap_or_else(|| (default_val as i32).to_string());
                let _ = write!(out, "  if ({name} === Core.INTEGER_DEFAULT{extra}) {{\n    {name} = {val};\n  }}");
                if let Some((min, max)) = super::common::int_bound_exprs(opt, enums, Lang::Ts) {
                    let _ = write!(out, " else if ({name} < {min} || {name} > {max}) {{\n    return {fail};\n  }}");
                }
                out.push('\n');
            }
            ParamType::Price(_) => {}
        }
    }
    out
}

/// A public entry's own rejection of a non-int32 integer or non-number real
/// argument (D13), as the code `fail` it answers.
pub(crate) fn arg_type_checks(func: &FuncDef, fail: &str) -> String {
    let mut out = String::new();
    for opt in &func.optional_inputs {
        let cond = match opt.param_type {
            ParamType::Integer | ParamType::Enum(_) => format!("({0} | 0) !== {0}", opt.name),
            ParamType::Real => format!("typeof {} !== 'number'", opt.name),
            ParamType::Price(_) => continue,
        };
        let _ = writeln!(out, "  if ({cond}) {{\n    return {fail};\n  }}");
    }
    out
}

/// The throwing form of [`arg_type_checks`]; `entry` names the entry point.
pub(crate) fn require_arg_types(func: &FuncDef, entry: &str) -> String {
    let mut out = String::new();
    for o in &func.optional_inputs {
        let (helper, cast) = match &o.param_type {
            ParamType::Integer => ("requireInt32", String::new()),
            ParamType::Enum(_) => ("requireInt32", format!(" as {}", opt_ts_type(o))),
            ParamType::Real => ("requireReal", String::new()),
            ParamType::Price(_) => continue,
        };
        let _ = writeln!(out, "  {0} = {helper}('{entry}', '{1}', {0}){cast};", o.name, param_key(&o.name));
    }
    out
}

/// What a message calls an input: the public argument that carries it
/// (`real`, `prices.high`).
pub fn input_label(func: &FuncDef, input: &str) -> String {
    api_leg_args(func, &[input.to_string()]).remove(0)
}

/// What a message calls an output: its buffer key.
pub fn output_label(func: &FuncDef, output: &str) -> String {
    let out = func.outputs.iter().find(|o| o.name == output).unwrap_or_else(|| panic!("{}: no output `{output}`", func.name));
    buffer_key(func, out)
}

/// The module constant [`param_range_rejection`] reads.
pub(crate) const PARAM_RANGES: &str = "PARAM_RANGES";

/// `PARAM_RANGES`: per parameter, in check order, what the shared
/// `rejectedParams` needs to name the one a negative lookback refused. Each
/// row mirrors [`emit_opt_param_validation`]: a bound or sentinel that differs
/// from the check names the wrong parameter, or none, without failing.
#[allow(clippy::cast_possible_truncation)]
fn gen_param_ranges(func: &FuncDef, enums: &HashMap<String, EnumDef>) -> String {
    if func.optional_inputs.is_empty() {
        return String::new();
    }
    let fmt_real = |v: f64| {
        if v.to_bits() == super::common::TA_REAL_MIN.to_bits() {
            "-3e37".to_string()
        } else if v.to_bits() == super::common::TA_REAL_MAX.to_bits() {
            "3e37".to_string()
        } else {
            super::doc_meta::fmt_real(v)
        }
    };
    let mut rows = Vec::new();
    for opt in &func.optional_inputs {
        let checked = match (&opt.param_type, opt.default) {
            (_, None) => None,
            (ParamType::Integer, _) => {
                opt.range.map(|(lo, hi)| ((lo as i32).to_string(), (hi as i32).to_string(), format!("in {}..{}", lo as i32, hi as i32)))
            }
            (ParamType::Enum(name), _) => super::common::int_bound_exprs(opt, enums, Lang::Ts).map(|(lo, hi)| {
                let accepted = opt.range.map_or_else(|| format!("a {name}"), |(l, h)| format!("in {}..{}", l as i32, h as i32));
                (lo, hi, accepted)
            }),
            (ParamType::Real, _) => opt.range.map(|(lo, hi)| {
                (
                    super::common::real_bound_literal(lo, "Core."),
                    super::common::real_bound_literal(hi, "Core."),
                    format!("in {}..{}", fmt_real(lo), fmt_real(hi)),
                )
            }),
            (ParamType::Price(_), _) => continue,
        };
        let (min, max, accepted) = checked.unwrap_or_else(|| ("0".to_string(), "0".to_string(), String::new()));
        rows.push(format!("['{}', {}, {min}, {max}, '{accepted}']", param_key(&opt.name), sentinel(opt)));
    }
    format!("\nconst {PARAM_RANGES} = [{}] as const;\n", rows.join(", "))
}

/// The throw for a negative lookback, by an entry named `entry` with the
/// parameters in scope under their own names; `None` for a function without
/// parameters, whose lookback never rejects.
pub(crate) fn param_range_rejection(func: &FuncDef, entry: &str) -> Option<String> {
    (!func.optional_inputs.is_empty()).then(|| format!("rejectedParams('{entry}', {PARAM_RANGES}, {})", opt_args(func).join(", ")))
}

fn opt_params(func: &FuncDef) -> Vec<String> {
    func.optional_inputs.iter().map(|o| format!("{}: {}", o.name, opt_ts_type(o))).collect()
}

fn opt_args(func: &FuncDef) -> Vec<String> {
    func.optional_inputs.iter().map(|o| o.name.clone()).collect()
}

fn gen_lookback(func: &FuncDef, svc: &Services) -> String {
    let names = ts_names(&func.name);
    let mut params = opt_params(func);
    params.push("core: Core = Core.DEFAULT".to_string());
    let mut body = arg_type_checks(func, "-1");
    body.push_str(&emit_opt_param_validation(func, "-1", svc.enums));
    match &func.lookback {
        Some(LookbackExpr::Literal(n)) => {
            let _ = writeln!(body, "  return {n};");
        }
        Some(LookbackExpr::ParamMinus(p, off)) => {
            let _ = writeln!(body, "  return {p} - {off};");
        }
        Some(LookbackExpr::Code(stmts)) => body.push_str(&render_scalar_body(func, stmts, svc, false)),
        None => body.push_str("  return 0;\n"),
    }
    format!(
        "\n/** @internal */\nexport function {lb}({}): number {{\n{body}}}\n",
        params.join(", "),
        lb = names.lookback_positional,
    )
}

fn gen_display_shift(func: &FuncDef, svc: &Services) -> String {
    let names = ts_names(&func.name);
    let idx = crate::ir::DISPLAY_SHIFT_INDEX_PARAM;
    let mut params = opt_params(func);
    params.push(format!("{idx}: number"));
    params.push("core: Core = Core.DEFAULT".to_string());
    let mut body = String::new();
    if !func.optional_inputs.is_empty() {
        let mut args = opt_args(func);
        args.push("core".to_string());
        let _ = writeln!(body, "  if ({}({}) < 0) {{\n    return {INT_MIN};\n  }}", names.lookback_positional, args.join(", "));
    }
    if func.display_shift.is_some() {
        body.push_str(&emit_opt_param_validation(func, INT_MIN, svc.enums));
    }
    let _ = writeln!(
        body,
        "  if (({idx} | 0) !== {idx} || {idx} < 0 || {idx} >= {}) {{\n    return {INT_MIN};\n  }}",
        func.outputs.len()
    );
    let unshifted = func.unshifted_outputs();
    if func.display_shift.is_some() && !unshifted.is_empty() {
        let tests: Vec<String> = unshifted.iter().map(|i| format!("{idx} === {i}")).collect();
        let _ = writeln!(body, "  if ({}) {{\n    return 0;\n  }}", tests.join(" || "));
    }
    match &func.display_shift {
        Some(stmts) => body.push_str(&render_scalar_body(func, stmts, svc, true)),
        None => body.push_str("  return 0;\n"),
    }
    format!(
        "\n/** @internal */\nexport function {ds}({}): number {{\n{body}}}\n",
        params.join(", "),
        ds = names.display_shift_positional,
    )
}

/// A lookback or display-shift body: integer index arithmetic over the
/// optional parameters.
fn render_scalar_body(func: &FuncDef, stmts: &[Statement], svc: &Services, display_shift: bool) -> String {
    let ctx = TsCtx::new(svc, stmts, &func.outputs, false);
    ctx.env.seed_opts(func);
    if display_shift {
        ctx.env.declare(crate::ir::DISPLAY_SHIFT_INDEX_PARAM, CType::Int32);
    }
    let mut out = String::new();
    for s in stmts {
        if let Statement::VarDecl { var_type, name, .. } = s {
            let _ = writeln!(out, "  {}", decl_line(name, var_type, &ctx));
        }
    }
    out.push_str(&emit_candle_unpacking(&detect_candle_settings(stmts)));
    for s in stmts {
        if let Statement::VarDecl { var_type, name, init: Some(init) } = s {
            let _ = writeln!(out, "  {name} = {};", ctx.value_for(CType::of_var_type(var_type), init));
        }
    }
    for s in stmts {
        if !matches!(s, Statement::VarDecl { .. }) {
            out.push_str(&ctx.stmt(s, 2));
        }
    }
    out
}

pub(crate) fn emit_candle_unpacking(settings: &BTreeSet<String>) -> String {
    let mut out = String::new();
    for s in settings {
        let member = crate::candle_settings::pascal_to_snake_case(s).to_uppercase();
        let at = format!("core.candleSettings[CandleSettingType.{member}]");
        let _ = writeln!(out, "  const {s}_rangeType: number = {at}.rangeType;");
        let _ = writeln!(out, "  const {s}_avgPeriod = {at}.avgPeriod;");
        let _ = writeln!(out, "  const {s}_factor = {at}.factor;");
    }
    out
}

pub(crate) fn cleaned(body: &[Statement], registry: &Registry) -> Vec<Statement> {
    let admits = |f: &str, a: &[Expr]| cross_call_split(f, a, registry).is_some();
    let folded = super::ir_cleanup::drop_answered_cross_call_guards(body, &admits, None);
    let folded = super::ir_cleanup::drop_deallocation(&folded);
    super::ir_cleanup::drop_inert_guards(&folded)
}

/// The signature of a body tier: `(core, startIdx, endIdx, ins.., opts..,
/// [private extras..], outBegIdx, outNBElement, outs..)`.
fn impl_params(func: &FuncDef, private: bool) -> Vec<String> {
    let mut params = vec!["core: Core".to_string(), "startIdx: number".to_string(), "endIdx: number".to_string()];
    for i in &func.inputs {
        params.push(format!("{}: {}", i.name, array_ts_type(&i.param_type)));
    }
    for o in &func.optional_inputs {
        params.push(format!("{}: number", o.name));
    }
    if private {
        for (n, _) in &func.private_extra_params {
            params.push(format!("{n}: number"));
        }
    }
    params.push("outBegIdx: MInteger".to_string());
    params.push("outNBElement: MInteger".to_string());
    for o in &func.outputs {
        params.push(format!("{}: {}", o.name, array_ts_type(&o.param_type)));
    }
    params
}

/// C pointer equality over two typed arrays (H15): the same buffer at the
/// same offset. `===` would miss two views of one buffer.
pub(crate) fn same_start(a: &str, b: &str) -> String {
    format!("({a}.buffer === {b}.buffer && {a}.byteOffset === {b}.byteOffset)")
}

/// The body's rejection of two outputs at one address. Every pair, cross-typed
/// too: unlike Java's arrays, an Int32Array and a Float64Array can view one
/// buffer, and C compares through `const void *`.
pub(crate) fn shared_output_condition(func: &FuncDef) -> Option<String> {
    let mut pairs = Vec::new();
    for (i, a) in func.outputs.iter().enumerate() {
        for b in &func.outputs[i + 1..] {
            pairs.push(same_start(&a.name, &b.name));
        }
    }
    (!pairs.is_empty()).then(|| pairs.join(" || "))
}

/// The public tier's overlap rejection, C#'s rule: outputs share no byte, and an
/// output shares none with a converted input unless it is that input in place,
/// which only a real output can be. Last of the checks, so a range or length
/// fault on the same call answers its own code.
fn batch_overlap_pairs(func: &FuncDef) -> Vec<(&'static str, String, String, String, String)> {
    let mut pairs = Vec::new();
    for o in &func.outputs {
        let test = if o.param_type == ParamType::Real { "overlapsPartly" } else { "overlaps" };
        for i in &func.inputs {
            pairs.push((test, o.name.clone(), format!("{}F64", i.name), output_label(func, &o.name), input_label(func, &i.name)));
        }
    }
    for (k, a) in func.outputs.iter().enumerate() {
        for b in &func.outputs[k + 1..] {
            pairs.push(("overlaps", a.name.clone(), b.name.clone(), output_label(func, &a.name), output_label(func, &b.name)));
        }
    }
    pairs
}

/// The body tier, or with `private` the declared `<n>Private` variant (no
/// validation prologue, extra parameters).
fn gen_impl(func: &FuncDef, private: bool, svc: &Services) -> String {
    let names = ts_names(&func.name);
    let name = if private { format!("{}Private", names.module) } else { names.body.clone() };
    let raw = if private { &func.private_body } else { &func.body };
    let folded = cleaned(raw, svc.registry);
    let body: &[Statement] = &folded;
    let ctx = TsCtx::new(svc, body, &func.outputs, true);
    ctx.env.seed_signature(func);

    let mut out = String::new();
    let _ = writeln!(out, "\n/** @internal */\nexport function {name}(\n  {},\n): RetCode {{", impl_params(func, private).join(",\n  "));

    for stmt in body {
        if let Statement::CircBuf(CircBuf::Prolog { id, layout, static_size }) = stmt {
            for (arr, t) in circbuf_arrays(id, layout) {
                let ty = ts_scalar_decl(if CType::of_var_type(&t) == CType::Real { CType::RealPtr } else { CType::IntPtr });
                let _ = writeln!(out, "  let {arr}!: {ty};");
            }
            let _ = writeln!(out, "  let {id}_Idx = 0;");
            let _ = writeln!(out, "  let maxIdx_{id} = ({static_size}) - 1;");
            continue;
        }
        if let Statement::VarDecl { var_type, name, .. } = stmt {
            let _ = writeln!(out, "  {}", decl_line(name, var_type, &ctx));
        }
    }
    out.push_str(&emit_candle_unpacking(&detect_candle_settings(body)));

    if !private {
        out.push_str("  if (startIdx < 0 || startIdx > Core.INDEX_MAX) {\n    return RetCode.OUT_OF_RANGE_START_INDEX;\n  }\n");
        out.push_str("  if (endIdx < 0 || endIdx > Core.INDEX_MAX || endIdx < startIdx) {\n    return RetCode.OUT_OF_RANGE_END_INDEX;\n  }\n");
        out.push_str(&emit_opt_param_validation(func, "RetCode.BAD_PARAM", svc.enums));
        if let Some(cond) = shared_output_condition(func) {
            let _ = writeln!(out, "  if ({cond}) {{\n    return RetCode.BAD_PARAM;\n  }}");
        }
    }

    for stmt in body {
        if let Statement::VarDecl { var_type, name, init: Some(init) } = stmt {
            let (init, hoisted) = ctx.hoist(init);
            out.push_str(&render_hoisted_blocks(&hoisted, 2, &ctx));
            let target = if ctx.address_of_vars.contains(name) {
                format!("{name}.value")
            } else if ctx.double_address_of_vars.contains(name) {
                format!("{name}[0]")
            } else {
                name.clone()
            };
            let _ = writeln!(out, "  {target} = {};", ctx.value_for(CType::of_var_type(var_type), &init));
        }
    }
    for stmt in body {
        if !matches!(stmt, Statement::VarDecl { .. }) {
            out.push_str(&ctx.stmt(stmt, 2));
        }
    }
    out.push_str("}\n");
    out
}

/// The public batch tier: argument checks in Java's order with the type tests
/// first (D13), each non-`Float64Array` input copied once (D5), the body, and
/// a throw for a rejection.
fn gen_public(func: &FuncDef) -> String {
    let names = ts_names(&func.name);
    let canonical = &func.name;
    let mut params = vec!["startIdx: number".to_string(), "endIdx: number".to_string()];
    for i in &func.inputs {
        params.push(format!("{}: ArrayLike<number>", i.name));
    }
    params.extend(opt_params(func));
    for o in &func.outputs {
        params.push(format!("{}: {}", o.name, array_ts_type(&o.param_type)));
    }
    params.push("core: Core = Core.DEFAULT".to_string());

    let mut b = String::new();
    for n in ["startIdx", "endIdx"] {
        let _ = writeln!(b, "  {n} = requireInt32('{canonical}', '{n}', {n});");
    }
    b.push_str(&require_arg_types(func, canonical));
    let _ = writeln!(b, "  requireIndexRange('{canonical}', startIdx, endIdx);");
    if !func.inputs.is_empty() || !func.outputs.is_empty() {
        let mut lb = opt_args(func);
        lb.push("core".to_string());
        let lookback = format!("{}({})", names.lookback_positional, lb.join(", "));
        let lookback = if let Some(reject) = param_range_rejection(func, canonical) {
            let _ = writeln!(b, "  const guardLookback = {lookback};\n  if (guardLookback < 0) {{\n    throw {reject};\n  }}");
            "guardLookback".to_string()
        } else {
            lookback
        };
        let _ = writeln!(b, "  const guardStart = clampedStart('{canonical}', startIdx, {lookback});");
        if !func.inputs.is_empty() {
            b.push_str("  const guardInLen = endIdx + 1;\n");
        }
        if !func.outputs.is_empty() {
            b.push_str("  const guardOutLen = guardStart > endIdx ? 0 : endIdx - guardStart + 1;\n");
        }
        for i in &func.inputs {
            let _ = writeln!(b, "  requireLength('{canonical}', '{}', {}, guardInLen);", input_label(func, &i.name), i.name);
        }
        for o in &func.outputs {
            let _ = writeln!(b, "  requireOutput('{canonical}', '{}', {}, guardOutLen, {});", output_label(func, &o.name), o.name, array_ts_type(&o.param_type));
        }
    }
    for i in &func.inputs {
        let _ = writeln!(b, "  const {0}F64 = float64({0});", i.name);
    }
    for (test, a, b_arr, a_label, b_label) in batch_overlap_pairs(func) {
        let _ = writeln!(b, "  if ({test}({a}, {b_arr})) {{\n    throw sharedMemory('{canonical}', '{a_label}', '{b_label}');\n  }}");
    }
    b.push_str("  const outBegIdx = new MInteger();\n  const outNBElement = new MInteger();\n");
    let mut args = vec!["core".to_string(), "startIdx".to_string(), "endIdx".to_string()];
    for i in &func.inputs {
        args.push(format!("{}F64", i.name));
    }
    args.extend(opt_args(func));
    args.push("outBegIdx".to_string());
    args.push("outNBElement".to_string());
    args.extend(func.outputs.iter().map(|o| o.name.clone()));
    let _ = writeln!(b, "  const retCode = {}({});", names.body, args.join(", "));
    let _ = writeln!(b, "  if (retCode !== RetCode.SUCCESS) {{\n    throw failure('{canonical}', retCode);\n  }}");
    b.push_str("  return new OutRange(outBegIdx.value, outNBElement.value);\n");
    format!("\n/** @internal */\nexport function {}(\n  {},\n): OutRange {{\n{b}}}\n", names.positional, params.join(",\n  "))
}

// ---------------------------------------------------------------------------
// The idiomatic API: inputs, then one optional options object
// ---------------------------------------------------------------------------

/// The sentinel an omitted parameter becomes before the positional tier sees it.
fn sentinel(opt: &OptInput) -> &'static str {
    match opt.param_type {
        ParamType::Real => "Core.REAL_DEFAULT",
        _ => "Core.INTEGER_DEFAULT",
    }
}

/// The idiomatic signature's input parameters.
pub(crate) fn api_input_sig(func: &FuncDef) -> Vec<String> {
    api_inputs(func).iter().map(|a| format!("{}: {}", a.name, a.ty)).collect()
}

/// The expression reading each expanded input, in `names` order.
pub fn api_leg_args(func: &FuncDef, names: &[String]) -> Vec<String> {
    let inputs = api_inputs(func);
    let legs: Vec<&(&crate::ir::Input, String)> = inputs.iter().flat_map(|a| &a.legs).collect();
    names
        .iter()
        .map(|n| legs.iter().find(|(i, _)| &i.name == n).unwrap_or_else(|| panic!("{}: no input `{n}`", func.name)).1.clone())
        .collect()
}

/// A price object must exist before its legs are read. When the batch call
/// defaults `endIdx` it reads the first series' length, so it first checks
/// every series present and non-empty, or it would report an index the caller
/// never passed; given an `endIdx`, the positional tier checks in the order
/// every backend shares (the range first). An opener leaves its legs to the
/// positional tier, whose order puts rS2 ahead of an absent leg.
/// Then each parameter (an omitted one as its sentinel) and `core`.
pub(crate) fn api_prologue(func: &FuncDef, entry: &str, batch: bool) -> String {
    let mut s = String::new();
    let inputs = api_inputs(func);
    for a in inputs.iter().filter(|a| a.is_price) {
        let _ = writeln!(s, "  requireArgument('{entry}', '{0}', {0});", a.name);
    }
    let _ = writeln!(s, "  const o = optionsOf('{entry}', options);");
    if batch && !inputs.is_empty() {
        s.push_str("  if (o.endIdx === undefined) {\n");
        for a in &inputs {
            if a.is_price {
                for (_, expr) in &a.legs {
                    let _ = writeln!(s, "    requireSeries('{entry}', '{expr}', {expr});");
                }
            } else {
                let _ = writeln!(s, "    requireSeries('{entry}', '{0}', {0});", a.name);
            }
        }
        s.push_str("  }\n");
    }
    for opt in &func.optional_inputs {
        let _ = writeln!(s, "  const {} = orDefault(o.{}, {});", opt.name, param_key(&opt.name), sentinel(opt));
    }
    let _ = writeln!(s, "  const core = coreOf('{entry}', o.core);");
    s
}

/// The lookback over the prologue's locals.
pub(crate) fn api_lookback_call(func: &FuncDef) -> String {
    let mut args = opt_args(func);
    args.push("core".to_string());
    format!("{}({})", ts_names(&func.name).lookback_positional, args.join(", "))
}

/// Each output buffer: the caller's, or one of `capacity` elements.
pub(crate) fn api_buffers(func: &FuncDef) -> String {
    let mut s = String::new();
    for out in &func.outputs {
        let _ = writeln!(
            s,
            "  const {0} = o.{1} === undefined ? new {2}(capacity) : o.{1};",
            out.name,
            buffer_key(func, out),
            array_ts_type(&out.param_type)
        );
    }
    s
}

/// The result's output keys, each a view of its buffer trimmed to `count`.
pub(crate) fn api_result_fields(func: &FuncDef, count: &str) -> Vec<String> {
    func.outputs.iter().map(|out| format!("{}: {}.subarray(0, {count})", result_key(func, out), out.name)).collect()
}

fn gen_api_types(func: &FuncDef, svc: &Services) -> String {
    let names = ts_names(&func.name);
    assert_unique_keys(func);
    let mut s = String::new();
    let _ = write!(s, "\n{}export interface {} {{\n", super::ts_doc::options_docs(func), names.options_type);
    for opt in &func.optional_inputs {
        s.push_str(&super::ts_doc::member(&super::ts_doc::param_text(func, opt, svc.enums)));
        let _ = writeln!(s, "  {}?: {} | undefined;", param_key(&opt.name), opt_ts_type(opt));
    }
    for (key, ty, text) in [
        ("startIdx", "number", super::ts_doc::START_IDX_KEY),
        ("endIdx", "number", super::ts_doc::END_IDX_KEY),
        ("core", "Core", super::ts_doc::CORE_KEY),
    ] {
        s.push_str(&super::ts_doc::member(text));
        let _ = writeln!(s, "  {key}?: {ty} | undefined;");
    }
    for out in &func.outputs {
        s.push_str(&super::ts_doc::member(&super::ts_doc::buffer_text(func, out)));
        let _ = writeln!(s, "  {}?: {} | undefined;", buffer_key(func, out), array_ts_type(&out.param_type));
    }
    s.push_str("}\n");
    let _ = write!(s, "\n{}export interface {} {{\n", super::ts_doc::result_docs(func), names.result_type);
    s.push_str(&super::ts_doc::member(super::ts_doc::BEG_IDX_FIELD));
    s.push_str("  begIdx: number;\n");
    for out in &func.outputs {
        s.push_str(&super::ts_doc::member(&super::ts_doc::result_field_text(func, out)));
        let _ = writeln!(s, "  {}: {};", result_key(func, out), array_ts_type(&out.param_type));
    }
    s.push_str("}\n");
    s
}

fn gen_api_lookback(func: &FuncDef) -> String {
    let names = ts_names(&func.name);
    let entry = format!("{} lookback", func.name);
    let mut args: Vec<String> = func
        .optional_inputs
        .iter()
        .map(|opt| format!("orDefault(o.{}, {})", param_key(&opt.name), sentinel(opt)))
        .collect();
    args.push(format!("coreOf('{entry}', o.core)"));
    format!(
        "\n{}export function {}(options?: {}): number {{\n  const o = optionsOf('{entry}', options);\n  return {}({});\n}}\n",
        super::ts_doc::lookback_docs(func),
        names.lookback,
        names.options_type,
        names.lookback_positional,
        args.join(", ")
    )
}

fn gen_api_display_shift(func: &FuncDef) -> String {
    let names = ts_names(&func.name);
    let idx = crate::ir::DISPLAY_SHIFT_INDEX_PARAM;
    let entry = format!("{} displayShift", func.name);
    let mut args: Vec<String> = func
        .optional_inputs
        .iter()
        .map(|opt| format!("orDefault(o.{}, {})", param_key(&opt.name), sentinel(opt)))
        .collect();
    args.push(idx.to_string());
    args.push(format!("coreOf('{entry}', o.core)"));
    format!(
        "\n{}export function {}({idx}: number, options?: {}): number {{\n  const o = optionsOf('{entry}', options);\n  return {}({});\n}}\n",
        super::ts_doc::display_shift_docs(func),
        names.display_shift,
        names.options_type,
        names.display_shift_positional,
        args.join(", ")
    )
}

/// The public batch function: options resolved to the positional tier's
/// arguments, each missing buffer allocated, and views of the values written.
fn gen_api_batch(func: &FuncDef, svc: &Services) -> String {
    let names = ts_names(&func.name);
    let canonical = &func.name;
    let in_names: Vec<String> = func.inputs.iter().map(|i| i.name.clone()).collect();
    let legs = api_leg_args(func, &in_names);
    let mut sig = api_input_sig(func);
    sig.push(format!("options?: {}", names.options_type));

    let mut b = api_prologue(func, canonical, true);
    b.push_str("  const startIdx = orDefault(o.startIdx, 0);\n");
    match legs.first() {
        Some(first) => {
            let _ = writeln!(b, "  const endIdx = o.endIdx === undefined ? {first}.length - 1 : o.endIdx;");
        }
        None => b.push_str("  const endIdx = orDefault(o.endIdx, 0);\n"),
    }
    let mut cap_args = vec!["startIdx".to_string(), "endIdx".to_string(), api_lookback_call(func)];
    cap_args.extend(legs.iter().cloned());
    let _ = writeln!(b, "  const capacity = batchCapacity({});", cap_args.join(", "));
    b.push_str(&api_buffers(func));
    let mut args = vec!["startIdx".to_string(), "endIdx".to_string()];
    args.extend(legs);
    args.extend(opt_args(func));
    args.extend(func.outputs.iter().map(|o| o.name.clone()));
    args.push("core".to_string());
    let _ = writeln!(b, "  const range = {}({});", names.positional, args.join(", "));
    let mut fields = vec!["begIdx: range.begIdx".to_string()];
    fields.extend(api_result_fields(func, "range.count"));
    let _ = writeln!(b, "  return {{ {} }};", fields.join(", "));
    format!(
        "\n{}export function {}({}): {} {{\n{b}}}\n",
        super::ts_doc::batch_docs(func, svc.enums, svc.registry),
        names.public,
        sig.join(", "),
        names.result_type,
    )
}

pub(crate) fn render_hoisted_blocks(hoisted: &[(String, VarType, Vec<Statement>)], indent: usize, ctx: &TsCtx) -> String {
    let pad = " ".repeat(indent);
    let mut out = String::new();
    for (temp, vt, body) in hoisted {
        let _ = writeln!(out, "{pad}{}", decl_line(temp, vt, ctx));
        for s in body {
            if let Statement::VarDecl { var_type, name, init } = s {
                match (var_type, init) {
                    (VarType::RealArray(_) | VarType::IntArray(_), _) | (_, None) => {
                        let _ = writeln!(out, "{pad}{}", decl_line(name, var_type, ctx));
                    }
                    (_, Some(init)) => {
                        let (init, inner) = ctx.hoist(init);
                        out.push_str(&render_hoisted_blocks(&inner, indent, ctx));
                        let t = CType::of_var_type(var_type);
                        let ann = if t.is_pointer() { format!(": {}", ts_scalar_decl(t)) } else { String::new() };
                        let _ = writeln!(out, "{pad}let {name}{ann} = {};", ctx.value_for(t, &init));
                    }
                }
            }
        }
        for s in body {
            if !matches!(s, Statement::VarDecl { .. }) {
                out.push_str(&ctx.stmt(s, indent));
            }
        }
    }
    out
}

struct TsStmt<'a> {
    c: &'a TsCtx<'a>,
}

impl TsStmt<'_> {
    fn render_if_tail(&self, then_body: &[Statement], else_body: &[Statement], indent: usize) -> String {
        let pad = " ".repeat(indent);
        let mut out = String::new();
        for s in then_body {
            out.push_str(&self.walk_stmt(s, indent + 2));
        }
        if else_body.is_empty() {
            let _ = writeln!(out, "{pad}}}");
            return out;
        }
        let code_start = else_body.iter().position(|s| !matches!(s, Statement::Comment(_))).unwrap_or(else_body.len());
        if else_body.len() - code_start == 1 && matches!(else_body.get(code_start), Some(Statement::If { .. })) {
            let inner = self.walk_stmt(&else_body[code_start], indent);
            if inner.trim_start().starts_with("if (") {
                for c in &else_body[..code_start] {
                    out.push_str(&self.walk_stmt(c, indent));
                }
                let _ = write!(out, "{pad}}} else ");
                out.push_str(inner.trim_start());
                return out;
            }
        }
        let _ = writeln!(out, "{pad}}} else {{");
        for s in else_body {
            out.push_str(&self.walk_stmt(s, indent + 2));
        }
        let _ = writeln!(out, "{pad}}}");
        out
    }

    /// The scalar type a store into `target` converts to, or `None` for an
    /// `Int32Array` element, whose store truncates by itself.
    fn dest_type(&self, target: &Expr) -> Option<CType> {
        match target {
            Expr::Var(n) if n == "outBegIdx" || n == "outNBElement" => Some(CType::Int32),
            Expr::ArrayAccess(..) | Expr::PointerDeref(_) => match self.c.ty(target) {
                CType::Int32 => None,
                t => Some(t),
            },
            _ => Some(self.c.ty(target)),
        }
    }

    fn store(&self, target: &Expr, value: &Expr) -> String {
        match self.dest_type(target) {
            Some(t) => self.c.value_for(t, value),
            None => self.c.num(value),
        }
    }
}

fn render_forc_part(stmt: &Statement, ctx: &TsCtx) -> String {
    let one = |s: &Statement| ctx.stmt(s, 0).trim().trim_end_matches(';').to_string();
    match stmt {
        Statement::Block { body } => body.iter().map(one).collect::<Vec<_>>().join(", "),
        _ => one(stmt),
    }
}

fn render_assign_target(expr: &Expr, ctx: &TsCtx) -> String {
    match expr {
        Expr::Var(name) if name == "outBegIdx" || name == "outNBElement" => format!("{name}.value"),
        Expr::Var(name) => {
            if ctx.address_of_vars.contains(name) {
                format!("{name}.value")
            } else if ctx.double_address_of_vars.contains(name) {
                format!("{name}[0]")
            } else {
                name.clone()
            }
        }
        _ => ctx.expr(expr),
    }
}

fn render_return_expr(expr: &Expr, ctx: &TsCtx) -> String {
    if let Expr::Var(name) = expr {
        let mapped = match name.as_str() {
            "SUCCESS" => Some("RetCode.SUCCESS"),
            "BadParam" => Some("RetCode.BAD_PARAM"),
            "InsufficientHistory" => Some("RetCode.INSUFFICIENT_HISTORY"),
            "OutOfRangeEndIndex" => Some("RetCode.OUT_OF_RANGE_END_INDEX"),
            "OutOfRangeStartIndex" => Some("RetCode.OUT_OF_RANGE_START_INDEX"),
            _ => None,
        };
        if let Some(m) = mapped {
            return m.to_string();
        }
    }
    ctx.value_for(ctx.ret, expr)
}

impl StatementEmitter for TsStmt<'_> {
    fn comment(&self, lines: &[String], indent: usize) -> String {
        super::stmt_walk::block_comment(lines, indent)
    }

    fn circ_buf(&self, op: &CircBuf, indent: usize) -> String {
        let pad = " ".repeat(indent);
        match op {
            CircBuf::Prolog { .. } | CircBuf::Destroy { .. } => String::new(),
            CircBuf::Next { id } => {
                format!("{pad}{id}_Idx++;\n{pad}if ({id}_Idx > maxIdx_{id}) {{\n{pad}  {id}_Idx = 0;\n{pad}}}\n")
            }
            CircBuf::Init { id, layout, size } => {
                let sz = self.c.value_for(CType::Int32, size);
                let mut s = format!("{pad}if ({sz} < 1) {{\n{pad}  return RetCode.INTERNAL_ERROR;\n{pad}}}\n");
                for (arr, t) in circbuf_arrays(id, layout) {
                    let _ = writeln!(s, "{pad}{arr} = new {}({sz});", circbuf_ctor(&t));
                }
                let _ = writeln!(s, "{pad}maxIdx_{id} = {sz} - 1;");
                let _ = writeln!(s, "{pad}{id}_Idx = 0;");
                s
            }
            CircBuf::InitLocalOnly { id, layout } => {
                let mut s = String::new();
                for (arr, t) in circbuf_arrays(id, layout) {
                    let _ = writeln!(s, "{pad}{arr} = new {}(maxIdx_{id} + 1);", circbuf_ctor(&t));
                }
                s
            }
        }
    }

    fn var_decl(&self, var_type: &VarType, name: &str, init: &Option<Expr>, indent: usize) -> String {
        let pad = " ".repeat(indent);
        let Some(init) = init else {
            return format!("{pad}{}\n", decl_line(name, var_type, self.c));
        };
        if matches!(var_type, VarType::RealArray(_) | VarType::IntArray(_)) {
            return format!("{pad}{}\n", decl_line(name, var_type, self.c));
        }
        let (init, hoisted) = self.c.hoist(init);
        let mut out = render_hoisted_blocks(&hoisted, indent, self.c);
        let init = match self.c.take_fma_sites(None, &init) {
            Some((rest, sites)) => {
                out.push_str(&self.c.fma_blocks(&sites, None, indent));
                rest
            }
            None => init,
        };
        let t = CType::of_var_type(var_type);
        let ann = match var_type {
            VarType::RetCodeType => ": RetCode".to_string(),
            _ if t.is_pointer() => format!(": {}", ts_scalar_decl(t)),
            _ => String::new(),
        };
        let _ = writeln!(out, "{pad}let {name}{ann} = {};", self.c.value_for(t, &init));
        out
    }

    fn assign(&self, target: &Expr, value: &Expr, compound: bool, indent: usize) -> String {
        let pad = " ".repeat(indent);
        let c = self.c;
        if let Expr::FuncCall(fname, cargs) = value {
            if c.svc.registry.contains(fname) {
                if let Some(block) = render_cross_indicator_call(fname, cargs, indent, c) {
                    let t = render_assign_target(target, c);
                    return format!("{block}{pad}{t} = RetCode.SUCCESS;\n");
                }
            }
        }
        let (new_value, hoisted) = c.hoist(value);
        let new_value = if fma::EMIT_FMA { fma::canonicalize_accumulator_add(target, &new_value) } else { new_value };
        let mut out = render_hoisted_blocks(&hoisted, indent, c);
        if !compound {
            if let Some((rest, sites)) = c.take_fma_sites(Some(target), &new_value) {
                let last = &sites[sites.len() - 1].temp;
                if matches!(&rest, Expr::Var(v) if v == last) && self.store(target, &rest) == *last {
                    let t = render_assign_target(target, c);
                    out.push_str(&c.fma_blocks(&sites, Some(&t), indent));
                    return out;
                }
                out.push_str(&c.fma_blocks(&sites, None, indent));
                let t = render_assign_target(target, c);
                let v = self.store(target, &rest);
                let _ = writeln!(out, "{pad}{t} = {v};");
                return out;
            }
        }

        if compound {
            if let (Expr::Var(tname), Expr::BinOp(left, op, right)) = (target, &new_value) {
                if matches!(left.as_ref(), Expr::Var(l) if l == tname) {
                    let tt = c.ty(target);
                    // An int target keeps C's conversion only through the plain form.
                    let expands = tt == CType::Int32
                        && (matches!(op, BinOp::Div | BinOp::Mul) || c.ty(right) == CType::Real);
                    let op_str = match op {
                        BinOp::Add => "+=",
                        BinOp::Sub => "-=",
                        BinOp::Mul => "*=",
                        BinOp::Div => "/=",
                        BinOp::BitwiseAnd => "&=",
                        BinOp::BitwiseOr => "|=",
                        BinOp::BitwiseXor => "^=",
                        BinOp::Shl => "<<=",
                        BinOp::Shr => ">>=",
                        _ => "",
                    };
                    if !op_str.is_empty() && !expands {
                        let t = render_assign_target(target, c);
                        let r = c.num(right);
                        let _ = writeln!(out, "{pad}{t} {op_str} {r};");
                        return out;
                    }
                }
            }
        }
        let t = render_assign_target(target, c);
        let v = self.store(target, &new_value);
        let _ = writeln!(out, "{pad}{t} = {v};");
        out
    }

    fn expr_stmt(&self, e: &Expr, indent: usize) -> String {
        let pad = " ".repeat(indent);
        if matches!(e, Expr::Var(_)) {
            return String::new();
        }
        if let Expr::FuncCall(fname, args) = e {
            if let Some(helper) = self.c.svc.helpers.get(fname) {
                if let Some(inlined) = try_inline_expr(helper, args) {
                    if matches!(inlined, Expr::Var(_)) {
                        return String::new();
                    }
                }
            }
            let rendered = render_func_call(fname, args, self.c);
            if rendered.is_empty() {
                return String::new();
            }
            return format!("{pad}{rendered};\n");
        }
        String::new()
    }

    fn while_loop(&self, condition: &Expr, body: &[Statement], indent: usize) -> String {
        let pad = " ".repeat(indent);
        let (cond, hoisted) = self.c.hoist(condition);
        let mut out = render_hoisted_blocks(&hoisted, indent, self.c);
        let _ = writeln!(out, "{pad}while ({}) {{", self.c.cond(&cond));
        self.c.in_loop(|| {
            for s in body {
                out.push_str(&self.walk_stmt(s, indent + 2));
            }
        });
        let _ = writeln!(out, "{pad}}}");
        out
    }

    fn do_while(&self, condition: &Expr, body: &[Statement], indent: usize) -> String {
        let pad = " ".repeat(indent);
        let (cond, hoisted) = self.c.hoist(condition);
        let mut out = format!("{pad}do {{\n");
        self.c.in_loop(|| {
            for s in body {
                out.push_str(&self.walk_stmt(s, indent + 2));
            }
        });
        out.push_str(&render_hoisted_blocks(&hoisted, indent + 2, self.c));
        let _ = writeln!(out, "{pad}}} while ({});", self.c.cond(&cond));
        out
    }

    fn if_stmt(
        &self,
        condition: &Expr,
        then_body: &[Statement],
        else_body: &[Statement],
        cond_comments: &[Option<Vec<String>>],
        indent: usize,
    ) -> String {
        let (condition, _) = super::builtins::strip_unlikely(condition);
        let pad = " ".repeat(indent);
        let c = self.c;
        if contains_alloc_err_return(then_body) {
            return String::new();
        }
        // V8 keeps a branch here, which mispredicts on every bar of an unordered
        // series; a boolean to number is a setcc. Integer only: a real counter
        // would turn -0 into +0.
        if let Some(var) = counted_increment(then_body, else_body) {
            if cond_comments.is_empty() && !has_side_effect(condition) && c.ty(&Expr::Var(var.clone())) == CType::Int32 {
                let (new_condition, hoisted) = c.hoist(condition);
                let mut out = render_hoisted_blocks(&hoisted, indent, c);
                let _ = writeln!(out, "{pad}{} += +({});", c.num(&Expr::Var(var)), c.cond(&new_condition));
                return out;
            }
        }
        if let Expr::BinOp(left, BinOp::And, right) = condition {
            if expr_directly_contains_candle_call(left) && expr_directly_contains_candle_call(right) {
                let (outer_comments, inner_comments) = super::common::split_cond_comments(left, right, cond_comments);
                let inner_if = Statement::If {
                    condition: *right.clone(),
                    then_body: then_body.to_vec(),
                    else_body: else_body.to_vec(),
                    cond_comments: inner_comments,
                };
                let outer_if = Statement::If {
                    condition: *left.clone(),
                    then_body: vec![inner_if],
                    else_body: else_body.to_vec(),
                    cond_comments: outer_comments,
                };
                return self.walk_stmt(&outer_if, indent);
            }
        }
        if !cond_comments.is_empty() {
            let counter0 = c.counter().get();
            let (new_condition, hoisted) = c.hoist(condition);
            let operand = |o: &Expr, op: &BinOp, is_right: bool| c.operand(o, op, is_right);
            let wraps = |o: &Expr, op: &BinOp, is_right: bool| {
                let (cp, pp) = (expr_prec(o), binop_prec(op));
                cp < pp || (cp == pp && is_right)
            };
            let flat = c.cond(&new_condition);
            if let Some(cond_text) = super::stmt_walk::render_cond_tree(
                &new_condition,
                cond_comments,
                &super::stmt_walk::CondHooks { operand: &operand, wraps: &wraps },
                &flat,
                &" ".repeat(indent + 4),
                ")",
                true,
            ) {
                let mut out = render_hoisted_blocks(&hoisted, indent, c);
                let _ = write!(out, "{pad}if (");
                out.push_str(&cond_text);
                let _ = writeln!(out, "{pad}{{");
                out.push_str(&self.render_if_tail(then_body, else_body, indent));
                return out;
            }
            c.counter().set(counter0);
        }
        let (new_condition, hoisted) = c.hoist(condition);
        let mut out = render_hoisted_blocks(&hoisted, indent, c);
        let _ = writeln!(out, "{pad}if ({}) {{", c.cond(&new_condition));
        out.push_str(&self.render_if_tail(then_body, else_body, indent));
        out
    }

    fn return_stmt(&self, value: &Option<Expr>, indent: usize) -> String {
        let pad = " ".repeat(indent);
        if let Some(Expr::FuncCall(fname, cargs)) = value {
            if self.c.svc.registry.contains(fname) {
                if let Some(block) = render_cross_indicator_call(fname, cargs, indent, self.c) {
                    return format!("{block}{pad}return RetCode.SUCCESS;\n");
                }
            }
        }
        match value {
            Some(e) => format!("{pad}return {};\n", render_return_expr(e, self.c)),
            None => format!("{pad}return;\n"),
        }
    }

    fn for_loop(&self, var: &str, count: &Expr, body: &[Statement], indent: usize) -> String {
        let pad = " ".repeat(indent);
        let mut out = format!("{pad}for ({var} = {}; {var} > 0; {var}--) {{\n", self.c.value_for(CType::Int32, count));
        self.c.in_loop(|| {
            for s in body {
                out.push_str(&self.walk_stmt(s, indent + 2));
            }
        });
        let _ = writeln!(out, "{pad}}}");
        out
    }

    fn for_c(&self, init: &Statement, condition: &Expr, update: &Statement, body: &[Statement], indent: usize) -> String {
        let pad = " ".repeat(indent);
        self.c.in_for_header.set(true);
        let init_str = render_forc_part(init, self.c);
        let update_str = render_forc_part(update, self.c);
        self.c.in_for_header.set(false);
        let (cond, hoisted) = self.c.hoist(condition);
        let mut out = render_hoisted_blocks(&hoisted, indent, self.c);
        let _ = writeln!(out, "{pad}for ({}; {}; {}) {{", init_str.trim(), self.c.cond(&cond), update_str.trim());
        self.c.in_loop(|| {
            for s in body {
                out.push_str(&self.walk_stmt(s, indent + 2));
            }
        });
        let _ = writeln!(out, "{pad}}}");
        out
    }

    fn switch(&self, expr: &Expr, cases: &[(String, Vec<Statement>)], default: &[Statement], indent: usize) -> String {
        let pad = " ".repeat(indent);
        let (e, hoisted) = self.c.hoist(expr);
        let mut out = render_hoisted_blocks(&hoisted, indent, self.c);
        let _ = writeln!(out, "{pad}switch ({}) {{", self.c.num(&e));
        // Each case is its own block: JS scopes a `let` to the whole switch.
        for (label, body) in cases {
            let _ = writeln!(out, "{pad}  case {}: {{", render_switch_label(label, self.c.svc.enums));
            for s in body {
                out.push_str(&self.walk_stmt(s, indent + 4));
            }
            let _ = writeln!(out, "{pad}    break;\n{pad}  }}");
        }
        if !default.is_empty() {
            let _ = writeln!(out, "{pad}  default: {{");
            for s in default {
                out.push_str(&self.walk_stmt(s, indent + 4));
            }
            let _ = writeln!(out, "{pad}    break;\n{pad}  }}");
        }
        let _ = writeln!(out, "{pad}}}");
        out
    }
}

fn circbuf_ctor(t: &VarType) -> &'static str {
    if matches!(t, VarType::Integer | VarType::Index) {
        "Int32Array"
    } else {
        "Float64Array"
    }
}

pub(crate) fn render_switch_label(label: &str, enums: &HashMap<String, EnumDef>) -> String {
    match lookup_variant(label, enums) {
        Some((enum_name, variant)) => format!("{enum_name}.{}", variant.name),
        None => label.to_string(),
    }
}

/// The literal spelling: shortest round-trip text (H13).
pub(crate) fn ts_literal(f: f64) -> String {
    let s = format!("{f:?}");
    assert!(s.parse::<f64>().is_ok_and(|v| v.to_bits() == f.to_bits()), "literal {f} does not round-trip as {s}");
    s
}

struct TsExpr<'a> {
    c: &'a TsCtx<'a>,
}

impl ExprEmitter for TsExpr<'_> {
    fn literal(&self, f: f64) -> String {
        ts_literal(f)
    }

    fn var(&self, name: &str) -> String {
        let mapped = match name {
            "BAD_PARAM" => "RetCode.BAD_PARAM".to_string(),
            "SUCCESS" => "RetCode.SUCCESS".to_string(),
            "ALLOC_ERR" => "RetCode.ALLOC_ERR".to_string(),
            "INTERNAL_ERROR" => "RetCode.INTERNAL_ERROR".to_string(),
            _ => self.c.matype_map.get(name).cloned().unwrap_or_else(|| name.to_string()),
        };
        if self.c.raw_refs.get() {
            mapped
        } else if self.c.address_of_vars.contains(name) {
            format!("{mapped}.value")
        } else if self.c.double_address_of_vars.contains(name) {
            format!("{mapped}[0]")
        } else {
            mapped
        }
    }

    fn array_access(&self, name: &str, idx: &Expr) -> String {
        format!("{name}[{}]", self.c.num(idx))
    }

    fn binop(&self, left: &Expr, op: &BinOp, right: &Expr) -> String {
        let c = self.c;
        if fma::EMIT_FMA {
            if let Some(fs) = c.fma_sets() {
                if let Some((a, b, cc)) = fma::fuse_operands(left, op, right, &fs.view()) {
                    return format!("fma({}, {}, {})", c.num(a), c.num(b), c.num(cc));
                }
            }
        }
        if matches!(op, BinOp::Eq | BinOp::NotEq) && c.ty(left).is_pointer() {
            let same = same_start(&c.expr(left), &c.expr(right));
            return if matches!(op, BinOp::Eq) { same } else { format!("!{same}") };
        }
        let op_str = match op {
            BinOp::Add => "+",
            BinOp::Sub => "-",
            BinOp::Mul => "*",
            BinOp::Div => "/",
            BinOp::Mod => "%",
            BinOp::LessEq => "<=",
            BinOp::Less => "<",
            BinOp::Greater => ">",
            BinOp::GreaterEq => ">=",
            BinOp::Eq => "===",
            BinOp::NotEq => "!==",
            BinOp::And => "&&",
            BinOp::Or => "||",
            BinOp::BitwiseOr => "|",
            BinOp::BitwiseXor => "^",
            BinOp::BitwiseAnd => "&",
            BinOp::Shr => ">>",
            BinOp::Shl => "<<",
        };
        // `===` over a Bool and a number compares the number with 0 or 1.
        let eq_mixed = matches!(op, BinOp::Eq | BinOp::NotEq) && (c.ty(left) == CType::Bool) != (c.ty(right) == CType::Bool);
        let side = |e: &Expr, is_right: bool| {
            if eq_mixed && c.ty(e) == CType::Bool {
                c.num(e)
            } else {
                c.operand(e, op, is_right)
            }
        };
        let text = format!("{} {op_str} {}", side(left, false), side(right, true));
        match c.env.int_fix(&Expr::BinOp(Box::new(left.clone()), op.clone(), Box::new(right.clone()))) {
            Some(IntFix::TruncDiv | IntFix::TruncMod | IntFix::WrapMul) => format!("(({text}) | 0)"),
            _ => text,
        }
    }

    fn cast(&self, var_type: &VarType, inner: &Expr) -> String {
        let c = self.c;
        let it = c.ty(inner);
        let atomic = |s: String| if expr_prec(inner) < ATOMIC_PREC { format!("({s})") } else { s };
        match var_type {
            VarType::Integer | VarType::Index if it == CType::Real => {
                let s = c.expr(inner);
                let s = if expr_prec(inner) <= binop_prec(&BinOp::BitwiseOr) { format!("({s})") } else { s };
                format!("({s} | 0)")
            }
            _ if it == CType::Bool => c.num(inner),
            _ => atomic(c.expr(inner)),
        }
    }

    fn not(&self, inner: &Expr) -> String {
        let s = self.c.cond(inner);
        if self.c.ty(inner) != CType::Bool || expr_prec(inner) < ATOMIC_PREC {
            format!("!({s})")
        } else {
            format!("!{s}")
        }
    }

    fn neg(&self, inner: &Expr) -> String {
        let t = self.c.ty(inner).numeric();
        let s = self.c.num(inner);
        let bare = match inner {
            Expr::Var(_) | Expr::ArrayAccess(..) | Expr::FuncCall(..) => true,
            Expr::Literal(f) => f.is_sign_positive(),
            Expr::IntLiteral(i) => *i >= 0,
            _ => false,
        };
        let negated = if bare && !s.starts_with('-') { format!("-{s}") } else { format!("-({s})") };
        if t == CType::Int32 {
            format!("({negated} | 0)")
        } else {
            negated
        }
    }

    fn bitwise_not(&self, inner: &Expr) -> String {
        let s = self.c.num(inner);
        if expr_prec(inner) < ATOMIC_PREC {
            format!("~({s})")
        } else {
            format!("~{s}")
        }
    }

    fn func_call(&self, name: &str, args: &[Expr]) -> String {
        render_func_call(name, args, self.c)
    }

    fn pointer_deref(&self, name: &str) -> String {
        if self.c.double_address_of_vars.contains(name) {
            format!("{name}[0]")
        } else {
            format!("{name}.value")
        }
    }

    fn address_of(&self, inner: &Expr) -> String {
        let prev = self.c.raw_refs.replace(true);
        let s = self.c.expr(inner);
        self.c.raw_refs.set(prev);
        s
    }

    fn post_increment(&self, inner: &Expr) -> String {
        format!("{}++", self.walk(inner))
    }

    fn post_decrement(&self, inner: &Expr) -> String {
        format!("{}--", self.walk(inner))
    }

    fn pre_increment(&self, inner: &Expr) -> String {
        format!("++{}", self.walk(inner))
    }

    fn pre_decrement(&self, inner: &Expr) -> String {
        format!("--{}", self.walk(inner))
    }

    fn ternary(&self, cond: &Expr, then_expr: &Expr, else_expr: &Expr) -> String {
        let c = self.c;
        match bool_ternary_collapse(then_expr, else_expr) {
            Some(BoolTernaryCollapse::Cond) => return c.cond(cond),
            Some(BoolTernaryCollapse::Negated) => return format!("!({})", c.cond(cond)),
            None => {}
        }
        let whole = c.ty(&Expr::Ternary(Box::new(cond.clone()), Box::new(then_expr.clone()), Box::new(else_expr.clone())));
        let branch = |e: &Expr| {
            let s = if whole == CType::Bool || whole.is_pointer() { c.expr(e) } else { c.num(e) };
            if matches!(e, Expr::Ternary(..)) {
                format!("({s})")
            } else {
                s
            }
        };
        let cs = c.cond(cond);
        let cs = if matches!(cond, Expr::BinOp(..) | Expr::Ternary(..)) && c.ty(cond) == CType::Bool { format!("({cs})") } else { cs };
        format!("{cs} ? {} : {}", branch(then_expr), branch(else_expr))
    }
}

/// `count` of a `memcpy`-family call as an element count: `n * sizeof(T)` is `n`.
fn strip_sizeof(e: &Expr) -> Expr {
    let is_sizeof = |x: &Expr| matches!(x, Expr::FuncCall(n, _) if n == "sizeof");
    match e {
        Expr::BinOp(l, BinOp::Mul, r) if is_sizeof(r) => strip_sizeof(l),
        Expr::BinOp(l, BinOp::Mul, r) if is_sizeof(l) => strip_sizeof(r),
        Expr::Cast(_, inner) if matches!(inner.as_ref(), Expr::BinOp(_, BinOp::Mul, _)) => strip_sizeof(inner),
        _ if is_sizeof(e) => Expr::IntLiteral(1),
        _ => e.clone(),
    }
}

/// `(array, offset)` of a `memcpy`-family pointer argument.
fn decompose_array_ref(expr: &Expr, ctx: &TsCtx) -> (String, String) {
    match expr {
        Expr::AddressOf(inner) => match inner.as_ref() {
            Expr::ArrayAccess(name, off) => (name.clone(), ctx.num(off)),
            _ => panic!("TS: address of a non-element in a copy: {expr:?}"),
        },
        Expr::Var(name) => (name.clone(), "0".to_string()),
        _ => panic!("TS: unsupported copy operand: {expr:?}"),
    }
}

fn plus(off: &str, count: &str, count_expr: &Expr) -> String {
    if off == "0" {
        count.to_string()
    } else if expr_prec(count_expr) < ATOMIC_PREC {
        format!("{off} + ({count})")
    } else {
        format!("{off} + {count}")
    }
}

/// The candle helpers rendered inline, as Java does; term for term
/// `ta_utility.h`'s macros.
fn try_render_candle_ternary(fname: &str, args: &[Expr], ctx: &TsCtx) -> Option<String> {
    let r = |e: &Expr| ctx.num(e);
    match fname {
        "ta_candlerange" if args.len() == 5 => {
            let (rt, open, high, low, close) = (r(&args[0]), r(&args[1]), r(&args[2]), r(&args[3]), r(&args[4]));
            Some(format!(
                "(({rt} === 0) ? (Math.abs({close} - {open})) \
                 : (({rt} === 1) ? ({high} - {low}) \
                 : (({rt} === 2) ? (({high} - ((({close}) >= ({open})) ? ({close}) : ({open}))) \
                 + (((({close}) >= ({open})) ? ({open}) : ({close})) - {low})) \
                 : 0.0)))"
            ))
        }
        "ta_candleaverage" if args.len() == 8 => {
            let (rt, avg_period, factor, sum) = (r(&args[0]), r(&args[1]), r(&args[2]), r(&args[3]));
            let cr_args: Vec<Expr> = std::iter::once(args[0].clone()).chain(args[4..8].iter().cloned()).collect();
            let candlerange = try_render_candle_ternary("ta_candlerange", &cr_args, ctx)?;
            Some(format!(
                "(({factor} * ((({avg_period} !== 0) \
                 ? ({sum} / {avg_period}) : {candlerange}) \
                 / (({rt} === 2) ? 2.0 : 1.0))))"
            ))
        }
        _ => None,
    }
}

/// The C `max`/`min` macro: `((a) > (b)) ? (a) : (b)`, operands in C's order.
/// `Math.max`/`Math.min` differ from it on NaN and on signed zeros (H7).
fn select_macro(op: &str, args: &[Expr], ctx: &TsCtx) -> String {
    let [a, b] = args else { panic!("max/min takes two arguments") };
    for x in [a, b] {
        assert!(!has_side_effect(x), "TS: max/min would evaluate a side effect twice: {x:?}");
    }
    let wrap = |e: &Expr| {
        let s = ctx.num(e);
        if expr_prec(e) < ATOMIC_PREC {
            format!("({s})")
        } else {
            s
        }
    };
    let (a, b) = (wrap(a), wrap(b));
    format!("({a} {op} {b} ? {a} : {b})")
}

/// The variable an `if` with no `else` adds one to and does nothing else with.
fn counted_increment(then_body: &[Statement], else_body: &[Statement]) -> Option<String> {
    if !else_body.is_empty() {
        return None;
    }
    let [s] = then_body else { return None };
    match s {
        Statement::Expr(Expr::PostIncrement(v) | Expr::PreIncrement(v)) => match v.as_ref() {
            Expr::Var(n) => Some(n.clone()),
            _ => None,
        },
        Statement::Assign { target: Expr::Var(n), value: Expr::BinOp(l, BinOp::Add, r), .. }
            if matches!(l.as_ref(), Expr::Var(m) if m == n) && matches!(r.as_ref(), Expr::IntLiteral(1)) =>
        {
            Some(n.clone())
        }
        _ => None,
    }
}

fn has_side_effect(e: &Expr) -> bool {
    match e {
        Expr::PostIncrement(_) | Expr::PostDecrement(_) | Expr::PreIncrement(_) | Expr::PreDecrement(_) => true,
        Expr::BinOp(l, _, r) => has_side_effect(l) || has_side_effect(r),
        Expr::Cast(_, i) | Expr::Not(i) | Expr::Neg(i) | Expr::BitwiseNot(i) | Expr::AddressOf(i) | Expr::ArrayAccess(_, i) => {
            has_side_effect(i)
        }
        Expr::Ternary(a, b, c) => has_side_effect(a) || has_side_effect(b) || has_side_effect(c),
        Expr::FuncCall(_, args) => args.iter().any(has_side_effect),
        Expr::Literal(_) | Expr::IntLiteral(_) | Expr::Var(_) | Expr::PointerDeref(_) => false,
    }
}

#[allow(clippy::too_many_lines)]
fn render_func_call(fname: &str, args: &[Expr], ctx: &TsCtx) -> String {
    assert!(!super::eager_select::is_pseudo_call(fname), "{fname} is rendered by C and Rust only");
    let svc = ctx.svc;
    if let Some(helper) = svc.helpers.get(fname) {
        if let Some(inlined) = ctx.env.inline_helper(helper, args) {
            return wrap_inlined(ctx.expr(&inlined), &inlined);
        }
    }
    if let Some(t) = try_render_candle_ternary(fname, args, ctx) {
        return t;
    }
    if let Some(b) = SpecialBuiltin::from_name(fname) {
        let r = |e: &Expr| ctx.num(e);
        return match b {
            SpecialBuiltin::Unlikely => {
                let [c] = args else { panic!("UNLIKELY takes one condition") };
                format!("({})", ctx.expr(c))
            }
            SpecialBuiltin::UnstablePeriod | SpecialBuiltin::UnstableAuto => {
                let Some(Expr::Var(id)) = args.first() else { panic!("an unstable-period read takes an id and a count") };
                let variant = unst_variant_name(id);
                let counts = super::builtins::unstable_level_counts(args).expect("an unstable-period read takes an id and a count");
                let counts: Vec<String> = counts.iter().map(|e| ctx.value_for(CType::Int32, e)).collect();
                let read = |c: &[String]| format!("core.unstableCount(FuncUnstId.{variant}, {})", c.join(", "));
                if matches!(b, SpecialBuiltin::UnstableAuto) {
                    super::builtins::unstable_auto_offset(&counts, false, read)
                } else {
                    read(&counts)
                }
            }
            pred @ (SpecialBuiltin::IsZero | SpecialBuiltin::IsZeroScaled | SpecialBuiltin::IsZeroOrNeg | SpecialBuiltin::IsFinite) => {
                let rendered: Vec<String> = args.iter().map(r).collect();
                ts_predicate_expr(pred, &rendered)
            }
            SpecialBuiltin::ArrayCopy => {
                let [dst, dst_off, src, src_off, count] = args else { panic!("ARRAY_COPY takes five arguments") };
                let (so, n) = (r(src_off), r(count));
                format!("{}.set({}.subarray({so}, ({so}) + ({n})), {})", ctx.expr(dst), ctx.expr(src), r(dst_off))
            }
            SpecialBuiltin::PerToK => format!("(2.0 / ({} + 1.0))", wrap_inlined(r(&args[0]), &args[0])),
        };
    }
    if let Some(mf) = MathFn::from_name(fname) {
        return match mf {
            MathFn::Max => select_macro(">", args, ctx),
            MathFn::Min => select_macro("<", args, ctx),
            _ => {
                let rendered: Vec<String> = args.iter().map(|a| ctx.num(a)).collect();
                format!("Math.{}({})", mf.canonical(), rendered.join(", "))
            }
        };
    }
    if let Some(s) = StdlibFn::from_name(fname) {
        return match s {
            StdlibFn::Sizeof => "1".to_string(),
            StdlibFn::Malloc => {
                let ctor = if find_sizeof_type(&args[0]).as_deref() == Some("int") { "Int32Array" } else { "Float64Array" };
                format!("new {ctor}({})", ctx.value_for(CType::Int32, &strip_sizeof(&args[0])))
            }
            StdlibFn::Free => unreachable!("free() reached the TS renderer: `ir_cleanup::drop_deallocation` runs on every body"),
            StdlibFn::Memcpy | StdlibFn::Memmove => {
                let (dst, dst_off) = decompose_array_ref(&args[0], ctx);
                let (src, src_off) = decompose_array_ref(&args[1], ctx);
                let count = strip_sizeof(&args[2]);
                let n = ctx.value_for(CType::Int32, &count);
                // `set` copies through a temporary when the two views share a buffer.
                format!("{dst}.set({src}.subarray({src_off}, {}), {dst_off})", plus(&src_off, &n, &count))
            }
            StdlibFn::Memset => {
                assert!(matches!(args[1], Expr::IntLiteral(0)), "TS: memset to a non-zero byte: {args:?}");
                let (arr, off) = decompose_array_ref(&args[0], ctx);
                let count = strip_sizeof(&args[2]);
                let n = ctx.value_for(CType::Int32, &count);
                format!("{arr}.fill(0, {off}, {})", plus(&off, &n, &count))
            }
        };
    }
    render_registry_call(fname, args, ctx)
}

/// A call to another indicator's lookback or private tier, passing `core` on.
fn render_registry_call(fname: &str, args: &[Expr], ctx: &TsCtx) -> String {
    let svc = ctx.svc;
    let Some((key, suffix)) = svc.registry.split_call(fname) else {
        panic!("TS: call to unknown function `{fname}`")
    };
    let sym = note_import(ctx, &key, &svc.registry.resolve_call(fname, Lang::Ts));
    let def = svc.registry.def(&key).unwrap_or_else(|| panic!("TS: no definition for `{key}`"));
    if suffix.as_deref() == Some("lookback") {
        let mut rendered = opt_call_args(def, args, ctx);
        rendered.push("core".to_string());
        return format!("{sym}({})", rendered.join(", "));
    }
    let mut rendered = vec!["core".to_string()];
    rendered.extend(args.iter().map(|a| ctx.value_for(ctx.ty(a), a)));
    format!("{sym}({})", rendered.join(", "))
}

/// Records the import of `sym` and returns the name the call site uses. A
/// local of the same name shadows an import (Java keeps methods and locals
/// apart), so the call then goes through an alias.
pub(crate) fn note_import(ctx: &TsCtx, key: &str, sym: &str) -> String {
    let module = camel_words(&ctx.svc.registry.name_of(key));
    let mut local = sym.to_string();
    while ctx.env.lookup(&local).is_some() {
        local.push('_');
    }
    // A module's own lookback, past a local of the same name, goes through a
    // module-scope alias: a self-import is a cycle Rollup warns about.
    if module != ctx.svc.module || local != sym {
        let entry = if local == sym { sym.to_string() } else { format!("{sym} as {local}") };
        ctx.svc.calls.borrow_mut().entry(module).or_default().insert(entry);
    }
    local
}

/// The optional-parameter arguments of a call, each converted to its slot.
fn opt_call_args(def: &FuncDef, args: &[Expr], ctx: &TsCtx) -> Vec<String> {
    assert_eq!(args.len(), def.optional_inputs.len(), "TS: {} takes {} optional arguments", def.name, def.optional_inputs.len());
    def.optional_inputs.iter().zip(args).map(|(o, a)| opt_arg(o, a, ctx)).collect()
}

pub(crate) fn opt_arg(o: &OptInput, a: &Expr, ctx: &TsCtx) -> String {
    match &o.param_type {
        ParamType::Enum(name) => format!("({} as {name})", ctx.value_for(CType::Int32, a)),
        p => ctx.value_for(CType::of_param(p), a),
    }
}

/// A cross-indicator call to the callee's public tier, whose rejection
/// throws; the out-meta pair binds from the returned range.
fn render_cross_indicator_call(fname: &str, args: &[Expr], indent: usize, ctx: &TsCtx) -> Option<String> {
    let split = cross_call_split(fname, args, ctx.svc.registry)?;
    let pad = " ".repeat(indent);
    let def = ctx.svc.registry.def(fname)?;
    let sym = note_import(ctx, fname, &ctx.svc.registry.resolve_call(fname, Lang::Ts));
    let lead = &args[..split];
    let n_in = def.inputs.len();
    let n_opt = def.optional_inputs.len();
    assert_eq!(lead.len(), 2 + n_in + n_opt, "TS: cross-call to {fname} has {} leading arguments", lead.len());
    let mut call_args: Vec<String> = Vec::new();
    for a in &lead[..2] {
        call_args.push(ctx.value_for(CType::Int32, a));
    }
    for a in &lead[2..2 + n_in] {
        call_args.push(ctx.expr(a));
    }
    for (o, a) in def.optional_inputs.iter().zip(&lead[2 + n_in..]) {
        call_args.push(opt_arg(o, a, ctx));
    }
    for a in &args[split + 2..] {
        call_args.push(ctx.expr(a));
    }
    call_args.push("core".to_string());
    let n = ctx.counter().get();
    ctx.counter().set(n + 1);
    let tmp = format!("_xr{n}");
    let beg = out_meta_target(&args[split], ctx);
    let nb = out_meta_target(&args[split + 1], ctx);
    Some(format!(
        "{pad}const {tmp} = {sym}({});\n{pad}{beg}.value = {tmp}.begIdx;\n{pad}{nb}.value = {tmp}.count;\n",
        call_args.join(", ")
    ))
}

fn out_meta_target(arg: &Expr, ctx: &TsCtx) -> String {
    match arg {
        Expr::AddressOf(inner) => match inner.as_ref() {
            Expr::Var(n) => n.clone(),
            other => ctx.expr(other),
        },
        Expr::Var(n) => n.clone(),
        other => ctx.expr(other),
    }
}

/// The values a function module exports publicly, in barrel order. The
/// `@internal` tiers (`<n>Impl`, `<n>Positional`, `<n>OpenInternal`, ...) are
/// exported from their module but never listed here.
#[must_use]
pub fn public_exports(func: &FuncDef) -> Vec<String> {
    let n = ts_names(&func.name);
    let mut names = vec![n.public, n.lookback, n.display_shift];
    if func.streaming {
        names.extend([n.open, n.open_and_fill, n.stream_class]);
        if func.outputs.len() > 1 {
            names.push(n.out_class);
        }
    }
    names
}

/// The types a function module exports publicly, for `export type`.
#[must_use]
pub fn public_type_exports(func: &FuncDef) -> Vec<String> {
    let n = ts_names(&func.name);
    vec![n.options_type, n.result_type]
}

#[allow(clippy::implicit_hasher)]
#[must_use]
pub fn render_statement(
    stmt: &Statement,
    indent: usize,
    enums: &HashMap<String, EnumDef>,
    registry: &Registry,
    helpers: &HelperRegistry,
    env_seed: &[(&str, CType)],
) -> String {
    let calls: CallImports = RefCell::new(BTreeMap::new());
    let svc = Services { enums, registry, helpers, calls: &calls, module: "" };
    let ctx = TsCtx::new(&svc, std::slice::from_ref(stmt), &[], false);
    for (n, t) in env_seed {
        ctx.env.declare(n, *t);
    }
    ctx.stmt(stmt, indent)
}

/// `library/package.json` (version from `VERSION`), `library/LICENSE` and
/// `library/src/index.ts`. The barrel carries explicit lists: `export *` would
/// leak `@internal` names at run time, which `stripInternal` strips from the
/// types only.
///
/// `LICENSE` is a committed copy because `npm pack` takes nothing from outside
/// the package directory, and a copy the build wrote would make the tree
/// publishable only after a build.
pub fn generate_package_files(funcs: &[FuncDef], root: &Path, lib: &Path) {
    let version = std::fs::read_to_string(root.join("VERSION"))
        .expect("cannot read VERSION")
        .trim()
        .to_string();
    crate::emit::write_if_changed(lib.join("package.json"), render_package_json(&version))
        .unwrap();
    let license = std::fs::read_to_string(root.join("LICENSE")).expect("cannot read LICENSE");
    crate::emit::write_if_changed(lib.join("LICENSE"), license).unwrap();
    crate::emit::write_if_changed(lib.join("src/index.ts"), render_index(funcs)).unwrap();
}

/// The npm package name (D2). The one place it is spelled.
pub const PACKAGE_NAME: &str = "ta-lib";

#[must_use]
pub fn render_package_json(version: &str) -> String {
    format!(
        r#"{{
  "name": "{PACKAGE_NAME}",
  "version": "{version}",
  "description": "Technical analysis library: 200+ indicators (SMA, EMA, RSI, MACD, Bollinger Bands, ATR, Stochastic, candlestick patterns). The official TypeScript port of TA-Lib, verified against the C reference.",
  "keywords": [
    "ta-lib",
    "talib",
    "technical-analysis",
    "technical-indicators",
    "indicators",
    "candlestick-patterns",
    "streaming",
    "finance",
    "trading"
  ],
  "homepage": "https://ta-lib.org",
  "type": "module",
  "sideEffects": false,
  "exports": {{
    ".": {{
      "types": "./dist/index.d.ts",
      "default": "./dist/index.js"
    }},
    "./metadata": {{
      "types": "./dist/metadata/index.d.ts",
      "default": "./dist/metadata/index.js"
    }}
  }},
  "types": "./dist/index.d.ts",
  "typesVersions": {{
    "*": {{
      "metadata": [
        "./dist/metadata/index.d.ts"
      ]
    }}
  }},
  "engines": {{
    "node": ">=22.12.0"
  }},
  "files": [
    "dist"
  ],
  "repository": {{
    "type": "git",
    "url": "git+https://github.com/TA-Lib/ta-lib.git",
    "directory": "ta_codegen/output/ts/library"
  }},
  "license": "BSD-3-Clause"
}}
"#
    )
}

#[must_use]
pub fn render_index(funcs: &[FuncDef]) -> String {
    use std::fmt::Write as _;
    let mut s = String::from(super::ta_abstract_c::LICENSE);
    s.push_str("\n/* Generated by ta_codegen \u{2014} do not edit. */\n\n");
    s.push_str("export { TALibError, RetCode } from './runtime/errors.ts';\n");
    s.push_str("export type { FailureRetCode, RetCodeName } from './runtime/errors.ts';\n");
    s.push_str("export { OutRange } from './runtime/outRange.ts';\n");
    s.push_str("export { Core, CoreBuilder, CandleSetting, CandleSettingType, RangeType } from './runtime/core.ts';\n");
    s.push_str("export { MAType, FuncUnstId } from './enums.ts';\n");
    let mut sorted: Vec<&FuncDef> = funcs.iter().collect();
    sorted.sort_by_key(|f| module_stem(f));
    for f in sorted {
        let names = public_exports(f);
        if names.is_empty() {
            continue;
        }
        let _ = writeln!(s, "export {{ {} }} from './func/{}.ts';", names.join(", "), module_stem(f));
        let _ = writeln!(s, "export type {{ {} }} from './func/{}.ts';", public_type_exports(f).join(", "), module_stem(f));
    }
    s
}

/// The TypeScript form of a near-zero builtin, shared by the indicator bodies
/// and the server's `eval_predicate`. `Math.abs` is exact, so these compare
/// like C's macros on every value, ±0 and NaN included.
#[must_use]
pub fn ts_predicate_expr(which: SpecialBuiltin, args: &[String]) -> String {
    match (which, args) {
        (SpecialBuiltin::IsZero, [x, ..]) => format!("((-0.00000000000001 < {x}) && ({x} < 0.00000000000001))"),
        (SpecialBuiltin::IsZeroScaled, [x, s]) => format!("(Math.abs({x}) <= 0.00000000000001 * ({s}))"),
        (SpecialBuiltin::IsZeroOrNeg, [x, ..]) => format!("({x} < 0.00000000000001)"),
        (SpecialBuiltin::IsFinite, [x, ..]) => format!("(Number.isFinite({x}))"),
        _ => "false".to_string(),
    }
}

/// What the structural gate checks at one site.
#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub enum IntAxis {
    /// An `int / int`, `int % int`, `int * int`, integer negation or `(int)` of a real.
    Fix(IntFix),
    /// A `/` whose result is real.
    RealDiv,
    /// A value stored into an `int` scalar local.
    IntLocal,
    /// A value returned by a function whose C return type is `int`.
    IntReturn,
    /// A value passed for an `int` parameter.
    IntArg,
}

/// One site the type pass classified, and the text the emitter gave it.
#[derive(Clone, Debug)]
pub struct IntSite {
    pub func: String,
    pub axis: IntAxis,
    pub rendered: String,
    /// For an implicit site: the value is real, so C truncates it.
    pub truncates: bool,
    /// For a `/`: `fma.rs`'s name-based classifier calls it real. Independent
    /// of the type pass, so a wrong verdict there disagrees here.
    pub real_by_name: bool,
    /// Found in a stream frame rather than in a batch-tier body.
    pub stream: bool,
}

/// Every integer-semantics site of `func`'s batch body, private body,
/// lookback, display shift and stream frames, rendered exactly as the emitter
/// renders them.
#[allow(clippy::implicit_hasher)]
#[must_use]
pub fn int_semantics_sites(
    func: &FuncDef,
    enums: &HashMap<String, EnumDef>,
    registry: &Registry,
    helpers: &HelperRegistry,
) -> Vec<IntSite> {
    let resolved = func.resolved_for(Lang::Ts);
    let func: &FuncDef = &resolved;
    let module = module_stem(func);
    let calls: CallImports = RefCell::new(BTreeMap::new());
    let svc = Services { enums, registry, helpers, calls: &calls, module: &module };
    let mut sites = Vec::new();
    let mut bodies: Vec<(Vec<Statement>, bool)> = vec![(cleaned(&func.body, registry), true)];
    if func.has_explicit_private {
        bodies.push((cleaned(&func.private_body, registry), true));
    }
    for (body, signature) in bodies {
        let ctx = TsCtx::new(&svc, &body, &func.outputs, true);
        if signature {
            ctx.env.seed_signature(func);
        }
        SiteWalk { c: &ctx, func: &func.name, out: RefCell::new(Vec::new()) }.stmts(&body, &mut sites);
    }
    let scalar: Vec<(Vec<Statement>, bool)> = [
        match &func.lookback {
            Some(LookbackExpr::Code(s)) => Some((s.clone(), false)),
            _ => None,
        },
        func.display_shift.clone().map(|s| (s, true)),
    ]
    .into_iter()
    .flatten()
    .collect();
    for (body, ds) in scalar {
        let ctx = TsCtx::new(&svc, &body, &func.outputs, false);
        ctx.env.seed_opts(func);
        if ds {
            ctx.env.declare(crate::ir::DISPLAY_SHIFT_INDEX_PARAM, CType::Int32);
        }
        SiteWalk { c: &ctx, func: &func.name, out: RefCell::new(Vec::new()) }.stmts(&body, &mut sites);
    }
    if super::ts_stream::emits_stream(func, registry) {
        let found: RefCell<Vec<IntSite>> = RefCell::new(Vec::new());
        let visit = |ctx: &TsCtx, body: &[Statement]| {
            let mut got = Vec::new();
            SiteWalk { c: ctx, func: &func.name, out: RefCell::new(Vec::new()) }.stmts(body, &mut got);
            found.borrow_mut().extend(got.into_iter().map(|s| IntSite { stream: true, ..s }));
        };
        let _ = super::ts_stream::generate_visiting(func, &svc, Some(&visit));
        sites.extend(found.into_inner());
    }
    sites
}

/// The sites of every helper body, typed in the helper's own scope.
#[allow(clippy::implicit_hasher)]
#[must_use]
pub fn helper_int_semantics_sites(
    enums: &HashMap<String, EnumDef>,
    registry: &Registry,
    helpers: &HelperRegistry,
) -> Vec<IntSite> {
    let calls: CallImports = RefCell::new(BTreeMap::new());
    let svc = Services { enums, registry, helpers, calls: &calls, module: "" };
    let mut sites = Vec::new();
    for h in helpers.iter() {
        let mut ctx = TsCtx::new(&svc, &h.body, &[], false);
        ctx.ret = CType::of_var_type(&h.return_type);
        ctx.env.seed_helper(h);
        SiteWalk { c: &ctx, func: &h.name, out: RefCell::new(Vec::new()) }.stmts(&h.body, &mut sites);
    }
    sites
}

struct SiteWalk<'a> {
    c: &'a TsCtx<'a>,
    func: &'a str,
    out: RefCell<Vec<IntSite>>,
}

impl SiteWalk<'_> {
    fn push(&self, axis: IntAxis, rendered: String, truncates: bool, real_by_name: bool) {
        self.out.borrow_mut().push(IntSite { func: self.func.to_string(), axis, rendered, truncates, real_by_name, stream: false });
    }

    fn each(&self, stmts: &[Statement]) {
        for s in stmts {
            self.stmt(s);
        }
    }

    fn stmts(&self, stmts: &[Statement], sites: &mut Vec<IntSite>) {
        for s in stmts {
            self.stmt(s);
        }
        sites.append(&mut self.out.borrow_mut());
    }

    fn top(&self, e: &Expr) -> Expr {
        let (e, hoisted) = self.c.hoist(e);
        for (_, _, body) in &hoisted {
            for s in body {
                self.stmt(s);
            }
        }
        self.expr(&e);
        e
    }

    fn implicit(&self, axis: IntAxis, dest: CType, value: &Expr) {
        if dest == CType::Int32 {
            self.push(axis, self.c.value_for(dest, value), self.c.env.implicit_trunc(dest, value), false);
        }
    }

    fn stmt(&self, s: &Statement) {
        let c = self.c;
        match s {
            Statement::VarDecl { var_type, init, .. } => {
                if let Some(init) = init {
                    let v = self.top(init);
                    self.implicit(IntAxis::IntLocal, CType::of_var_type(var_type), &v);
                }
            }
            Statement::Assign { target, value, .. } => {
                if let Expr::FuncCall(f, args) = value {
                    if c.svc.registry.contains(f) {
                        self.call_args(f, args);
                        return;
                    }
                }
                self.expr(target);
                let v = self.top(value);
                let scalar = match target {
                    Expr::Var(n) if n == "outBegIdx" || n == "outNBElement" => Some(CType::Int32),
                    Expr::Var(_) => Some(c.ty(target)),
                    _ => None,
                };
                if let Some(t) = scalar {
                    self.implicit(IntAxis::IntLocal, t, &v);
                }
            }
            Statement::Return { value: Some(v) } => {
                if let Expr::FuncCall(f, args) = v {
                    if c.svc.registry.contains(f) {
                        self.call_args(f, args);
                        return;
                    }
                }
                let v = self.top(v);
                self.implicit(IntAxis::IntReturn, c.ret, &v);
            }
            Statement::If { condition, then_body, else_body, .. } => {
                self.top(condition);
                self.each(then_body);
                self.each(else_body);
            }
            Statement::While { condition, body } | Statement::DoWhile { condition, body } => {
                self.top(condition);
                self.each(body);
            }
            Statement::For { count, body, .. } => {
                self.top(count);
                self.each(body);
            }
            Statement::ForC { init, condition, update, body } => {
                self.stmt(init);
                self.top(condition);
                self.stmt(update);
                self.each(body);
            }
            Statement::Switch { expr, cases, default } => {
                self.top(expr);
                for (_, b) in cases {
                    self.each(b);
                }
                self.each(default);
            }
            Statement::Block { body } => self.each(body),
            Statement::Expr(e) => {
                self.top(e);
            }
            Statement::CircBuf(CircBuf::Init { size, .. }) => {
                self.top(size);
            }
            Statement::Return { value: None }
            | Statement::UnrollHint { .. }
            | Statement::Break
            | Statement::Continue
            | Statement::CircBuf(_)
            | Statement::Comment(_) => {}
        }
    }

    /// A cross-call's arguments, each against its slot in the callee.
    fn call_args(&self, fname: &str, args: &[Expr]) {
        let c = self.c;
        let Some(def) = c.svc.registry.def(fname) else { return };
        let Some(split) = cross_call_split(fname, args, c.svc.registry) else { return };
        let lead = &args[..split];
        for a in args {
            self.expr(a);
        }
        for a in &lead[..2] {
            self.implicit(IntAxis::IntArg, CType::Int32, a);
        }
        let n_in = def.inputs.len();
        for (o, a) in def.optional_inputs.iter().zip(&lead[2 + n_in..]) {
            self.implicit(IntAxis::IntArg, CType::of_param(&o.param_type), a);
        }
    }

    fn expr(&self, e: &Expr) {
        let c = self.c;
        match e {
            Expr::BinOp(l, op, r) => {
                if let Some(fix) = c.env.int_fix(e) {
                    self.push(IntAxis::Fix(fix), c.expr(e), false, self.real_by_name(e));
                } else if matches!(op, BinOp::Div) {
                    self.push(IntAxis::RealDiv, c.expr(e), false, self.real_by_name(e));
                }
                self.expr(l);
                self.expr(r);
            }
            Expr::Neg(i) | Expr::Cast(_, i) => {
                if let Some(fix) = c.env.int_fix(e) {
                    self.push(IntAxis::Fix(fix), c.expr(e), false, false);
                }
                self.expr(i);
            }
            Expr::Not(i)
            | Expr::BitwiseNot(i)
            | Expr::AddressOf(i)
            | Expr::PostIncrement(i)
            | Expr::PostDecrement(i)
            | Expr::PreIncrement(i)
            | Expr::PreDecrement(i)
            | Expr::ArrayAccess(_, i) => self.expr(i),
            Expr::Ternary(cond, then_e, else_e) => {
                self.expr(cond);
                self.expr(then_e);
                self.expr(else_e);
            }
            Expr::FuncCall(name, args) => {
                if matches!(SpecialBuiltin::from_name(name), Some(SpecialBuiltin::UnstablePeriod | SpecialBuiltin::UnstableAuto)) {
                    // The count is a template over the Auto level; what renders is each level's.
                    for count in super::builtins::unstable_level_counts(args).unwrap_or_default() {
                        self.implicit(IntAxis::IntArg, CType::Int32, &count);
                        self.expr(&count);
                    }
                    return;
                }
                if let Some(h) = c.svc.helpers.get(name) {
                    for (p, a) in h.params.iter().zip(args) {
                        self.implicit(IntAxis::IntArg, CType::of_var_type(&p.var_type), a);
                    }
                    if let Some(inlined) = c.env.inline_helper(h, args) {
                        self.expr(&inlined);
                        return;
                    }
                } else if let Some((key, Some(suffix))) = c.svc.registry.split_call(name) {
                    if suffix == "lookback" {
                        if let Some(def) = c.svc.registry.def(&key) {
                            for (o, a) in def.optional_inputs.iter().zip(args) {
                                self.implicit(IntAxis::IntArg, CType::of_param(&o.param_type), a);
                            }
                        }
                    }
                }
                for a in args {
                    self.expr(a);
                }
            }
            Expr::Literal(_) | Expr::IntLiteral(_) | Expr::Var(_) | Expr::PointerDeref(_) => {}
        }
    }

    fn real_by_name(&self, e: &Expr) -> bool {
        let e = &max_min_as_sum(e);
        self.c.fma_sets().map_or_else(
            || fma::expr_is_float_typed(e, None),
            |fs| fma::expr_is_float_typed(e, Some(&fs.view())),
        )
    }
}

/// `max(a, b)` as `a + b`: the classifier calls every `max` real, while C's
/// macro takes its operands' usual conversion, which a sum has too.
fn max_min_as_sum(expr: &Expr) -> Expr {
    let boxed = |x: &Expr| Box::new(max_min_as_sum(x));
    match expr {
        Expr::FuncCall(name, args) if args.len() == 2 && matches!(MathFn::from_name(name), Some(MathFn::Max | MathFn::Min)) => {
            Expr::BinOp(boxed(&args[0]), BinOp::Add, boxed(&args[1]))
        }
        Expr::FuncCall(name, args) => Expr::FuncCall(name.clone(), args.iter().map(max_min_as_sum).collect()),
        Expr::BinOp(lhs, op, rhs) => Expr::BinOp(boxed(lhs), op.clone(), boxed(rhs)),
        Expr::Cast(ty, inner) => Expr::Cast(ty.clone(), boxed(inner)),
        Expr::Neg(inner) => Expr::Neg(boxed(inner)),
        Expr::Ternary(cond, then_e, else_e) => Expr::Ternary(boxed(cond), boxed(then_e), boxed(else_e)),
        other => other.clone(),
    }
}

