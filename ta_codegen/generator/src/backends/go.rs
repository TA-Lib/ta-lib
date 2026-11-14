//! Generates the Go package: one file per indicator under `ta_codegen/output/go/`,
//! each holding methods on `*Core`.
//!
//! Go has no `x++` expression, no ternary and no implicit conversion, so every
//! statement is lowered at render time: an expression yields the statements that
//! must run before it (`pre`) and after it (`post`), and an operand C evaluates
//! lazily (a ternary arm, the right side of `&&`/`||`) gets its own scope so its
//! statements land inside the branch that evaluates it. A block-bodied helper is
//! the exception: it is hoisted ahead of the statement even from a lazy operand.

use std::cell::Cell;
use std::collections::{BTreeSet, HashMap, HashSet};
use std::fmt::Write as _;

use crate::candle_settings::detect_candle_settings;
use crate::helper_registry::{hoist_block_helpers, try_inline_expr, HelperRegistry};
use crate::ir::{
    BinOp, CircBuf, EnumDef, Expr, FuncDef, Lang, LookbackExpr, OptInput, ParamType, Statement, Tier, VarType,
};
use crate::parser::enums::lookup_variant;
use crate::registry::Registry;

use super::builtins::{MathFn, SpecialBuiltin, StdlibFn};
use super::common::{camel_words, contains_alloc_err_return, find_sizeof_type};
use super::ctype::{CType, TypeEnv};
use super::fma::{self, FmaVarSets};
use super::go_fmt::{GExpr, GOp};
use super::ts::strip_sizeof;
use super::java::{bool_ternary_collapse, circbuf_arrays, cross_call_split, unst_variant_name, BoolTernaryCollapse, JAVA_CANDLE_FNS};

/// Go's keywords: the naming gate's word list for this backend.
pub(crate) const RESERVED_WORDS: &[&str] = &[
    "break", "case", "chan", "const", "continue", "default", "defer", "else", "fallthrough", "for", "func", "go",
    "goto", "if", "import", "interface", "map", "package", "range", "return", "select", "struct", "switch", "type",
    "var",
];

/// Names a local must not shadow in a generated body: the predeclared
/// identifiers, the receiver, the `math` package, and what the runtime and the
/// enums export. A local spelled like one gains a trailing `_`.
const SHADOWED: &[&str] = &[
    "any", "append", "bool", "byte", "cap", "clear", "close", "comparable", "complex", "complex128", "complex64",
    "copy", "delete", "error", "false", "float32", "float64", "imag", "int", "int16", "int32", "int64", "int8",
    "iota", "len", "make", "max", "min", "new", "nil", "panic", "print", "println", "real", "recover", "rune",
    "string", "true", "uint", "uint16", "uint32", "uint64", "uint8", "uintptr", "c", "math", "unsafe", "b2i", "f2i",
    "maxF", "minF", "maxI", "minI", "isZero", "isZeroScaled", "isZeroOrNeg", "isFinite", "candleRange",
    "candleAverage", "sameStart", "overlaps", "overlapsPartly", "fitsInt32", "requiredOut", "Core", "OutRange",
    "RetCode", "MAType", "FuncUnstId", "IndexMax", "IntegerDefault", "RealDefault", "RealMin", "RealMax",
];

/// The generated files this backend writes beside the per-function ones, and
/// the hand-written runtime.
pub(crate) const KEEP: &[&str] =
    &["core.go", "doc.go", "retcode.go", "runtime.go", "enums.go", "example_test.go", "talib_test.go"];

#[must_use]
pub fn go_ident(name: &str) -> String {
    if RESERVED_WORDS.contains(&name) || SHADOWED.contains(&name) {
        format!("{name}_")
    } else {
        name.to_string()
    }
}

/// The file stem of an indicator: its name lower-cased.
///
/// # Panics
/// When Go would read the name as a build constraint (`_test`, `_<GOOS>`,
/// `_<GOARCH>`), ignore the file (a leading `_` or `.`), or when it collides
/// with a runtime file. Go applies any of these without a diagnostic.
#[must_use]
pub fn file_stem(func_name: &str) -> String {
    let stem = func_name.to_lowercase();
    if let Some(reason) = stem_problem(&stem) {
        panic!("Go: function `{func_name}` would be written to `{stem}.go`, which {reason}");
    }
    stem
}

const GOOS: &[&str] = &[
    "aix", "android", "darwin", "dragonfly", "freebsd", "hurd", "illumos", "ios", "js", "linux", "nacl", "netbsd",
    "openbsd", "plan9", "solaris", "wasip1", "windows", "zos",
];
const GOARCH: &[&str] = &[
    "386", "amd64", "amd64p32", "arm", "armbe", "arm64", "arm64be", "loong64", "mips", "mipsle", "mips64",
    "mips64le", "mips64p32", "mips64p32le", "ppc", "ppc64", "ppc64le", "riscv", "riscv64", "s390", "s390x", "sparc",
    "sparc64", "wasm",
];

pub(crate) fn stem_problem(stem: &str) -> Option<String> {
    if stem.starts_with('_') || stem.starts_with('.') {
        return Some("Go ignores".to_string());
    }
    if KEEP.contains(&format!("{stem}.go").as_str()) {
        return Some("is a runtime file".to_string());
    }
    let parts: Vec<&str> = stem.split('_').collect();
    let last = parts[parts.len() - 1];
    if parts.len() > 1 && last == "test" {
        return Some("Go builds only under `go test`".to_string());
    }
    if parts.len() > 1 && (GOOS.contains(&last) || GOARCH.contains(&last)) {
        return Some(format!("Go builds only for `{last}`"));
    }
    None
}

/// Every name an indicator's Go file defines.
pub struct GoNames {
    /// The batch call (`SMA`).
    pub public: String,
    /// The public lookback (`SMALookback`).
    pub lookback_public: String,
    /// The C-shaped body (`smaImpl`).
    pub body: String,
    /// The C-shaped lookback the bodies call (`smaLookback`).
    pub lookback: String,
    /// The declared private variant (`smaPrivate`).
    pub private: String,
}

#[must_use]
pub fn go_names(func_name: &str) -> GoNames {
    let camel = camel_words(func_name);
    GoNames {
        public: func_name.to_string(),
        lookback_public: format!("{func_name}Lookback"),
        body: format!("{camel}Impl"),
        lookback: format!("{camel}Lookback"),
        private: format!("{camel}Private"),
    }
}

/// The module manifest: the module root is `ta_codegen/output/go` itself.
pub const GO_MOD: &str = "module ta-lib.github.io/go\n\ngo 1.22\n";

/// The JSON-RPC server's manifest, building against the package beside it.
pub const GO_TOOLS_MOD: &str =
    "module ta-lib.github.io/go/tools\n\ngo 1.22\n\nrequire ta-lib.github.io/go v0.0.0\n\nreplace ta-lib.github.io/go => ../\n";

/// The first line of every generated file, in the form `go generate` and
/// linters recognise.
pub const GENERATED_HEADER: &str = "// Code generated by ta_codegen. DO NOT EDIT.";

/// The license and the package clause every generated file opens with.
#[must_use]
pub fn file_head() -> String {
    format!("{GENERATED_HEADER}\n\n{}package talib\n", super::ta_abstract_c::LICENSE)
}

#[allow(clippy::implicit_hasher)]
#[must_use]
pub fn generate(func: &FuncDef, enums: &HashMap<String, EnumDef>, registry: &Registry, helpers: &HelperRegistry) -> String {
    let resolved = func.resolved_for(Lang::Go);
    let func: &FuncDef = &resolved;
    let _ = file_stem(&func.name);
    let svc = Services { enums, registry, helpers };

    let mut body = String::new();
    body.push_str(&gen_lookback(func, &svc));
    if func.has_explicit_private {
        body.push_str(&gen_impl(func, true, &svc));
    }
    body.push_str(&gen_impl(func, false, &svc));
    body.push_str(&gen_public_lookback(func, svc.enums));
    body.push_str(&gen_public(func, &svc));
    if super::go_stream::emits_stream(func, registry) {
        body.push_str(&super::go_stream::generate(func, &svc));
    }
    let body = super::go_prune::prune(&body);

    let mut out = file_head();
    if body.contains("math.") {
        out.push_str("\nimport \"math\"\n");
    }
    if let Some(m) = func.alt_marker(Tier::Batch, Lang::Go) {
        let _ = write!(out, "\n// {m}\n");
    }
    out.push_str(&body);
    out
}

pub(super) struct Services<'a> {
    pub(super) enums: &'a HashMap<String, EnumDef>,
    pub(super) registry: &'a Registry,
    pub(super) helpers: &'a HelperRegistry,
}

/// The statements an expression needs around it.
#[derive(Default)]
pub(super) struct Scope {
    pub(super) pre: Vec<String>,
    pub(super) post: Vec<String>,
}

impl Scope {
    pub(super) fn is_empty(&self) -> bool {
        self.pre.is_empty() && self.post.is_empty()
    }
}

pub(super) fn indented(lines: &[String]) -> impl Iterator<Item = String> + '_ {
    lines.iter().map(|l| format!("\t{l}"))
}

/// What a body returns: a `RetCode` (an `Impl`), an `int32` (a lookback), a
/// value beside a nil error (a peek frame), or an error alone (a step that
/// drives sub-streams).
#[derive(Clone, Copy, PartialEq, Eq)]
pub(super) enum Ret {
    Code,
    Int,
    Value(CType),
    Fallible,
}

pub(super) struct Ctx<'a> {
    svc: &'a Services<'a>,
    func: &'a str,
    pub(super) env: TypeEnv<'a>,
    fma: Option<FmaVarSets>,
    fma_ref: Option<&'a FmaVarSets>,
    counter: Cell<usize>,
    /// Shared by every context rendering into one function, so two temps
    /// never take one name.
    shared_counter: Option<&'a Cell<usize>>,
    /// Fixed-size array locals (`[N]float64`), which become slices as values.
    pub(super) arrays: HashSet<String>,
    /// Locals of C type `TA_RetCode`.
    pub(super) codes: HashSet<String>,
    pub(super) ret: Ret,
}

const RETCODE_NAMES: &[(&str, &str)] = &[
    ("SUCCESS", "Success"),
    ("BAD_PARAM", "BadParam"),
    ("ALLOC_ERR", "AllocErr"),
    ("INTERNAL_ERROR", "InternalError"),
    ("BadParam", "BadParam"),
    ("InsufficientHistory", "InsufficientHistory"),
    ("OutOfRangeEndIndex", "OutOfRangeEndIndex"),
    ("OutOfRangeStartIndex", "OutOfRangeStartIndex"),
];

fn retcode_name(name: &str) -> Option<&'static str> {
    RETCODE_NAMES.iter().find(|(c, _)| *c == name).map(|(_, g)| *g)
}

pub(super) fn go_type(t: CType) -> &'static str {
    match t {
        CType::Int32 => "int32",
        CType::Real => "float64",
        CType::Bool => "bool",
        CType::RealPtr => "[]float64",
        CType::IntPtr => "[]int32",
    }
}

pub(super) fn var_go_type(vt: &VarType) -> String {
    match vt {
        VarType::Real => "float64".to_string(),
        VarType::Integer | VarType::Index => "int32".to_string(),
        VarType::RetCodeType => "RetCode".to_string(),
        VarType::RealPointer => "[]float64".to_string(),
        VarType::IntPointer => "[]int32".to_string(),
        VarType::RealArray(n) => format!("[{n}]float64"),
        VarType::IntArray(n) => format!("[{n}]int32"),
    }
}

fn arith(l: CType, r: CType) -> CType {
    if l.numeric() == CType::Real || r.numeric() == CType::Real {
        CType::Real
    } else {
        CType::Int32
    }
}

/// The enum constant a C name such as `TA_MAType_SMA` spells in Go.
fn enum_const(name: &str, enums: &HashMap<String, EnumDef>) -> Option<String> {
    enums.values().find_map(|e| e.variants.iter().find(|v| v.c_name == name).map(|v| format!("{}{}", e.name, v.name)))
}

/// A literal real as Go spells it: shortest round-trip text, sign apart.
pub(super) fn real_lit(f: f64) -> GExpr {
    if f == 0.0 && f.is_sign_negative() {
        return GExpr::call("math.Copysign", vec![GExpr::name("0"), GExpr::name("-1")]);
    }
    let s = format!("{:?}", f.abs());
    assert!(s.parse::<f64>().is_ok_and(|v| v.to_bits() == f.abs().to_bits()), "literal {f} does not round-trip as {s}");
    if f < 0.0 {
        GExpr::unary("-", GExpr::name(s))
    } else {
        GExpr::name(s)
    }
}

fn int_lit(i: i64) -> GExpr {
    if i < 0 {
        GExpr::unary("-", GExpr::name(i.unsigned_abs().to_string()))
    } else {
        GExpr::name(i.to_string())
    }
}

/// True for an expression of literals only. Go folds an untyped constant
/// expression exactly and rounds once, where C rounds each literal and each
/// operation: `0.67*360.0` is a different double in the two.
fn is_const(e: &Expr) -> bool {
    match e {
        Expr::Literal(_) | Expr::IntLiteral(_) => true,
        Expr::Neg(i) | Expr::Cast(_, i) => is_const(i),
        Expr::BinOp(l, _, r) => is_const(l) && is_const(r),
        _ => false,
    }
}

/// Every variable an expression reads, counted.
fn count_vars(e: &Expr, counts: &mut HashMap<String, usize>) {
    crate::streaming::walk_expr(e, &mut |x| {
        if let Expr::Var(n) | Expr::ArrayAccess(n, _) | Expr::PointerDeref(n) = x {
            *counts.entry(n.clone()).or_default() += 1;
        }
    });
}

fn stepped_vars(e: &Expr, out: &mut Vec<String>) {
    crate::streaming::walk_expr(e, &mut |x| {
        if let Expr::PostIncrement(i) | Expr::PostDecrement(i) | Expr::PreIncrement(i) | Expr::PreDecrement(i) = x {
            if let Expr::Var(n) = i.as_ref() {
                out.push(n.clone());
            }
        }
    });
}

pub(super) fn strip_paren(g: GExpr) -> GExpr {
    match g {
        GExpr::Paren(inner) => strip_paren(*inner),
        other => other,
    }
}

impl<'a> Ctx<'a> {
    fn new(svc: &'a Services<'a>, func: &'a str, body: &[Statement], outputs: &[crate::ir::Output], fuse: bool, ret: Ret) -> Self {
        let env = TypeEnv::new(svc.helpers, svc.registry, svc.enums);
        env.seed_decls(body);
        env.seed_candles(&detect_candle_settings(body));
        let mut arrays = HashSet::new();
        let mut codes = HashSet::new();
        collect_decl_kinds(body, &mut arrays, &mut codes);
        Ctx {
            svc,
            func,
            env,
            fma: fuse.then(|| fma::build_fma_var_sets(body, outputs, &fma::INDEX_PARAM_SEEDS)),
            fma_ref: None,
            counter: Cell::new(0),
            shared_counter: None,
            arrays,
            codes,
            ret,
        }
    }

    /// A context for stream code: the caller declares every name it renders
    /// beyond `body`'s own, and the fused sites are the stream source's.
    pub(super) fn for_stream(
        svc: &'a Services<'a>,
        func: &'a str,
        body: &[Statement],
        fma: &'a FmaVarSets,
        counter: &'a Cell<usize>,
        ret: Ret,
    ) -> Self {
        let env = TypeEnv::new(svc.helpers, svc.registry, svc.enums);
        env.seed_decls(body);
        let mut arrays = HashSet::new();
        let mut codes = HashSet::new();
        collect_decl_kinds(body, &mut arrays, &mut codes);
        Ctx { svc, func, env, fma: None, fma_ref: Some(fma), counter: Cell::new(0), shared_counter: Some(counter), arrays, codes, ret }
    }

    fn fma_sets(&self) -> Option<&FmaVarSets> {
        self.fma_ref.or(self.fma.as_ref())
    }

    fn counter(&self) -> &Cell<usize> {
        self.shared_counter.unwrap_or(&self.counter)
    }

    pub(super) fn fail(&self, what: &str) -> ! {
        panic!("Go backend, function {}: {what}", self.func)
    }

    pub(super) fn temp(&self, prefix: &str) -> String {
        let n = self.counter().get();
        self.counter().set(n + 1);
        format!("_{prefix}{n}")
    }

    /// A stepped variable is read nowhere else in its statement, so moving the step
    /// before or after the statement is exact. Helpers are substituted after this
    /// check, so it does not cover a step inside a helper call's argument.
    pub(super) fn check_steps(&self, exprs: &[&Expr]) {
        let mut stepped = Vec::new();
        let mut counts = HashMap::new();
        for e in exprs {
            stepped_vars(e, &mut stepped);
            count_vars(e, &mut counts);
        }
        for v in stepped {
            if counts.get(&v).copied().unwrap_or(0) != 1 {
                self.fail(&format!("`{v}` is stepped and read again in one statement"));
            }
        }
    }

    pub(super) fn hoist(&self, e: &Expr) -> (Expr, Vec<(String, VarType, Vec<Statement>)>) {
        let mut hoisted = Vec::new();
        let mut cnt = self.counter().get();
        let new = hoist_block_helpers(e, self.svc.helpers, &mut hoisted, &mut cnt, JAVA_CANDLE_FNS);
        self.counter().set(cnt);
        for (temp, vt, body) in &hoisted {
            self.env.declare(temp, CType::of_var_type(vt));
            self.env.seed_decls(body);
        }
        (new, hoisted)
    }

    pub(super) fn render_hoisted(&self, hoisted: &[(String, VarType, Vec<Statement>)], out: &mut Vec<String>) {
        for (temp, vt, body) in hoisted {
            out.push(format!("var {} {}", go_ident(temp), var_go_type(vt)));
            for s in body {
                if let Statement::VarDecl { var_type, name, init } = s {
                    out.push(format!("var {} {}", go_ident(name), var_go_type(var_type)));
                    if let (Some(init), false) = (init, matches!(var_type, VarType::RealArray(_) | VarType::IntArray(_))) {
                        let (init, inner) = self.hoist(init);
                        self.render_hoisted(&inner, out);
                        let mut sc = Scope::default();
                        let v = self.value_for(CType::of_var_type(var_type), &init, &mut sc);
                        out.extend(sc.pre);
                        out.push(format!("{} = {}", go_ident(name), v.render()));
                        out.extend(sc.post);
                    }
                }
            }
            for s in body {
                if !matches!(s, Statement::VarDecl { .. }) {
                    self.stmt(s, out);
                }
            }
        }
    }

    /// The initializer of every top-level declaration in `body`, in order.
    pub(super) fn initializers(&self, body: &[Statement], lines: &mut Vec<String>) {
        for stmt in body {
            if let Statement::VarDecl { var_type, name, init: Some(init) } = stmt {
                if matches!(var_type, VarType::RealArray(_) | VarType::IntArray(_)) {
                    continue;
                }
                self.check_steps(&[init]);
                let (init, hoisted) = self.hoist(init);
                self.render_hoisted(&hoisted, lines);
                let mut sc = Scope::default();
                let v = self.store_value(&Expr::Var(name.clone()), CType::of_var_type(var_type), &init, &mut sc);
                lines.extend(sc.pre);
                lines.push(format!("{} = {}", go_ident(name), v.render()));
                lines.extend(sc.post);
            }
        }
    }

    /// `e` as a value of type `to` that needs no statement of its own.
    pub(super) fn pure(&self, e: &Expr, to: Option<CType>) -> GExpr {
        let mut sc = Scope::default();
        let g = match to {
            Some(t) => self.lower_as(e, t, &mut sc),
            None => self.lower(e, &mut sc).0,
        };
        if !sc.is_empty() {
            self.fail(&format!("an expression that needs statements where one cannot go: {e:?}"));
        }
        g
    }

    // ---- expressions -------------------------------------------------------

    fn conv(&self, g: GExpr, from: CType, to: CType) -> GExpr {
        match (from, to) {
            (a, b) if a == b => g,
            (CType::Int32, CType::Real) => match g {
                GExpr::Name(s) if s.parse::<i64>().is_ok() => GExpr::name(format!("{s}.0")),
                GExpr::Unary("-", inner) if matches!(inner.as_ref(), GExpr::Name(s) if s.parse::<i64>().is_ok()) => {
                    let GExpr::Name(s) = *inner else { unreachable!() };
                    GExpr::unary("-", GExpr::name(format!("{s}.0")))
                }
                g => GExpr::call("float64", vec![g]),
            },
            (CType::Real, CType::Int32) => GExpr::call("f2i", vec![g]),
            (CType::Bool, CType::Int32) => GExpr::call("b2i", vec![g]),
            (CType::Bool, CType::Real) => GExpr::call("float64", vec![GExpr::call("b2i", vec![g])]),
            (CType::Int32 | CType::Real, CType::Bool) => GExpr::bin(g, GOp::Ne, GExpr::name("0")),
            _ => self.fail(&format!("cannot convert {from:?} to {to:?}")),
        }
    }

    pub(super) fn lower_as(&self, e: &Expr, to: CType, sc: &mut Scope) -> GExpr {
        let (g, t) = self.lower(e, sc);
        if t.is_pointer() && to.is_pointer() {
            return g;
        }
        self.conv(g, t, to)
    }

    pub(super) fn cond(&self, e: &Expr, sc: &mut Scope) -> GExpr {
        self.lower_as(e, CType::Bool, sc)
    }

    /// `e` stored into a scalar of type `dest`, with C's implicit conversion.
    pub(super) fn value_for(&self, dest: CType, e: &Expr, sc: &mut Scope) -> GExpr {
        self.lower_as(e, dest, sc)
    }

    #[allow(clippy::too_many_lines)]
    pub(super) fn lower(&self, e: &Expr, sc: &mut Scope) -> (GExpr, CType) {
        match e {
            Expr::Literal(f) => (real_lit(*f), CType::Real),
            Expr::IntLiteral(i) => (int_lit(*i), CType::Int32),
            Expr::Var(name) => self.var(name),
            Expr::ArrayAccess(name, idx) => {
                let i = self.lower_as(idx, CType::Int32, sc);
                let t = match self.env.lookup(name) {
                    Some(CType::RealPtr) => CType::Real,
                    Some(CType::IntPtr) => CType::Int32,
                    other => self.fail(&format!("`{name}` is indexed but is {other:?}")),
                };
                (GExpr::index(GExpr::name(go_ident(name)), i), t)
            }
            Expr::PointerDeref(name) => {
                let t = match self.env.lookup(name) {
                    Some(CType::IntPtr) => CType::Int32,
                    Some(CType::RealPtr) => CType::Real,
                    other => self.fail(&format!("`*{name}` dereferences {other:?}")),
                };
                (GExpr::unary("*", GExpr::name(go_ident(name))), t)
            }
            Expr::AddressOf(inner) => match inner.as_ref() {
                Expr::ArrayAccess(name, off) => {
                    let t = self.env.lookup(name).unwrap_or_else(|| self.fail(&format!("`&{name}[..]` of an undeclared name")));
                    let o = self.lower_as(off, CType::Int32, sc);
                    (GExpr::slice(GExpr::name(go_ident(name)), Some(o), None), t)
                }
                other => self.fail(&format!("address of {other:?} outside a cross-call out-parameter")),
            },
            Expr::BinOp(l, op, r) => self.binop(l, op, r, sc),
            Expr::Cast(vt, inner) => {
                let to = CType::of_var_type(vt);
                if to.is_pointer() {
                    return self.lower(inner, sc);
                }
                let (g, t) = self.lower(inner, sc);
                if t == to {
                    return (g, t);
                }
                (self.conv(g, t, to), to)
            }
            Expr::Not(inner) => (GExpr::not(self.cond(inner, sc)), CType::Bool),
            Expr::Neg(inner) => {
                if matches!(inner.as_ref(), Expr::Literal(f) if *f == 0.0) {
                    return (real_lit(-0.0), CType::Real);
                }
                let (g, t) = self.lower(inner, sc);
                let n = t.numeric();
                (GExpr::unary("-", self.conv(g, t, n)), n)
            }
            Expr::BitwiseNot(inner) => (GExpr::unary("^", self.lower_as(inner, CType::Int32, sc)), CType::Int32),
            Expr::FuncCall(name, args) => self.call(name, args, sc),
            Expr::PostIncrement(inner) | Expr::PostDecrement(inner) => {
                let (g, t) = self.step_target(inner);
                let op = if matches!(e, Expr::PostIncrement(_)) { "++" } else { "--" };
                sc.post.push(format!("{}{op}", g.render()));
                (g, t)
            }
            Expr::PreIncrement(inner) | Expr::PreDecrement(inner) => {
                let (g, t) = self.step_target(inner);
                let op = if matches!(e, Expr::PreIncrement(_)) { "++" } else { "--" };
                sc.pre.push(format!("{}{op}", g.render()));
                (g, t)
            }
            Expr::Ternary(c, t, f) => self.ternary(c, t, f, sc),
        }
    }

    fn step_target(&self, inner: &Expr) -> (GExpr, CType) {
        let Expr::Var(n) = inner else { self.fail(&format!("a step of {inner:?}, not of a variable")) };
        let t = self.env.lookup(n).unwrap_or_else(|| self.fail(&format!("`{n}` has no declared type")));
        (GExpr::name(go_ident(n)), t)
    }

    fn var(&self, name: &str) -> (GExpr, CType) {
        if let Some(g) = retcode_name(name) {
            return (GExpr::name(g), CType::Int32);
        }
        if let Some(c) = enum_const(name, self.svc.enums) {
            return (GExpr::call("int32", vec![GExpr::name(c)]), CType::Int32);
        }
        let t = self.env.lookup(name).unwrap_or_else(|| self.fail(&format!("`{name}` has no declared type")));
        let g = GExpr::name(go_ident(name));
        if self.arrays.contains(name) {
            return (GExpr::slice(g, None, None), t);
        }
        (g, t)
    }

    fn binop(&self, l: &Expr, op: &BinOp, r: &Expr, sc: &mut Scope) -> (GExpr, CType) {
        if fma::EMIT_FMA {
            if let Some(fs) = self.fma_sets() {
                if let Some(operands) = fma::fuse_operands(l, op, r, &fs.view()) {
                    let args = [operands.0, operands.1, operands.2].iter().map(|x| self.lower_as(x, CType::Real, sc)).collect();
                    return (GExpr::call("math.FMA", args), CType::Real);
                }
            }
        }
        if matches!(op, BinOp::And | BinOp::Or) {
            return (self.logical(l, op, r, sc), CType::Bool);
        }
        let (lg, lt) = self.lower(l, sc);
        let (rg, rt) = self.lower(r, sc);
        if matches!(op, BinOp::Eq | BinOp::NotEq) && (lt.is_pointer() || rt.is_pointer()) {
            if !(lt.is_pointer() && rt.is_pointer()) {
                self.fail("a pointer compared with a number");
            }
            let same = GExpr::call("sameStart", vec![lg, rg]);
            return (if matches!(op, BinOp::Eq) { same } else { GExpr::not(same) }, CType::Bool);
        }
        if lt.is_pointer() || rt.is_pointer() {
            self.fail("pointer arithmetic");
        }
        let gop = match op {
            BinOp::Add => GOp::Add,
            BinOp::Sub => GOp::Sub,
            BinOp::Mul => GOp::Mul,
            BinOp::Div => GOp::Quo,
            BinOp::Mod => GOp::Rem,
            BinOp::Shl => GOp::Shl,
            BinOp::Shr => GOp::Shr,
            BinOp::BitwiseAnd => GOp::And,
            BinOp::BitwiseOr => GOp::Or,
            BinOp::BitwiseXor => GOp::Xor,
            BinOp::Eq => GOp::Eq,
            BinOp::NotEq => GOp::Ne,
            BinOp::Less => GOp::Lt,
            BinOp::LessEq => GOp::Le,
            BinOp::Greater => GOp::Gt,
            BinOp::GreaterEq => GOp::Ge,
            BinOp::And | BinOp::Or => unreachable!(),
        };
        match op {
            BinOp::Add | BinOp::Sub | BinOp::Mul | BinOp::Div => {
                let t = arith(lt, rt);
                let (lg, rg) = (self.conv(lg, lt, t), self.conv(rg, rt, t));
                let folded = t == CType::Real && is_const(l) && is_const(r);
                let g = if folded { GExpr::bin(typed(lg), gop, typed(rg)) } else { GExpr::bin(lg, gop, rg) };
                if t == CType::Real && (*op == BinOp::Mul || folded) {
                    return (round(g), t);
                }
                (g, t)
            }
            BinOp::Mod | BinOp::Shl | BinOp::Shr | BinOp::BitwiseAnd | BinOp::BitwiseOr | BinOp::BitwiseXor => {
                if lt.numeric() != CType::Int32 || rt.numeric() != CType::Int32 {
                    self.fail("an integer operator over a real");
                }
                (GExpr::bin(self.conv(lg, lt, CType::Int32), gop, self.conv(rg, rt, CType::Int32)), CType::Int32)
            }
            _ => {
                let both_bool = lt == CType::Bool && rt == CType::Bool && matches!(op, BinOp::Eq | BinOp::NotEq);
                let t = if both_bool { CType::Bool } else { arith(lt, rt) };
                (GExpr::bin(self.conv(lg, lt, t), gop, self.conv(rg, rt, t)), CType::Bool)
            }
        }
    }

    fn logical(&self, l: &Expr, op: &BinOp, r: &Expr, sc: &mut Scope) -> GExpr {
        let lg = self.cond(l, sc);
        let mut rs = Scope::default();
        let rg = self.cond(r, &mut rs);
        let gop = if *op == BinOp::And { GOp::LAnd } else { GOp::LOr };
        if rs.is_empty() {
            return GExpr::bin(lg, gop, rg);
        }
        let tmp = self.temp("B");
        sc.pre.push(format!("{tmp} := {}", lg.render()));
        sc.pre.push(if *op == BinOp::And { format!("if {tmp} {{") } else { format!("if !{tmp} {{") });
        sc.pre.extend(indented(&rs.pre));
        sc.pre.push(format!("\t{tmp} = {}", rg.render()));
        sc.pre.extend(indented(&rs.post));
        sc.pre.push("}".to_string());
        GExpr::name(tmp)
    }

    fn ternary(&self, c: &Expr, t: &Expr, f: &Expr, sc: &mut Scope) -> (GExpr, CType) {
        match bool_ternary_collapse(t, f) {
            Some(BoolTernaryCollapse::Cond) => return (self.cond(c, sc), CType::Bool),
            Some(BoolTernaryCollapse::Negated) => return (GExpr::not(self.cond(c, sc)), CType::Bool),
            None => {}
        }
        let cg = self.cond(c, sc);
        let mut ts = Scope::default();
        let (tg, tt) = self.lower(t, &mut ts);
        let mut fs = Scope::default();
        let (fg, ft) = self.lower(f, &mut fs);
        let ty = if tt == CType::Bool && ft == CType::Bool {
            CType::Bool
        } else if tt.is_pointer() || ft.is_pointer() {
            if tt != ft {
                self.fail("a ternary over a pointer and something else");
            }
            tt
        } else {
            arith(tt, ft)
        };
        let tg = if ty.is_pointer() { tg } else { self.conv(tg, tt, ty) };
        let fg = if ty.is_pointer() { fg } else { self.conv(fg, ft, ty) };
        let tmp = self.temp("T");
        sc.pre.push(format!("var {tmp} {}", go_type(ty)));
        sc.pre.push(format!("if {} {{", strip_paren(cg).render()));
        sc.pre.extend(indented(&ts.pre));
        sc.pre.push(format!("\t{tmp} = {}", tg.render()));
        sc.pre.extend(indented(&ts.post));
        sc.pre.push("} else {".to_string());
        sc.pre.extend(indented(&fs.pre));
        sc.pre.push(format!("\t{tmp} = {}", fg.render()));
        sc.pre.extend(indented(&fs.post));
        sc.pre.push("}".to_string());
        (GExpr::name(tmp), ty)
    }

    #[allow(clippy::too_many_lines)]
    pub(super) fn call(&self, fname: &str, args: &[Expr], sc: &mut Scope) -> (GExpr, CType) {
        if super::eager_select::is_pseudo_call(fname) {
            self.fail(&format!("{fname} is rendered by C and Rust only"));
        }
        let svc = self.svc;
        if let Some(helper) = svc.helpers.get(fname) {
            if let Some(inlined) = self.env.inline_helper(helper, args) {
                return self.lower(&inlined, sc);
            }
        }
        match fname {
            "ta_candlerange" if args.len() == 5 => {
                let mut a = vec![self.lower_as(&args[0], CType::Int32, sc)];
                for x in &args[1..] {
                    a.push(self.lower_as(x, CType::Real, sc));
                }
                return (GExpr::call("candleRange", a), CType::Real);
            }
            "ta_candleaverage" if args.len() == 8 => {
                let mut a = vec![self.lower_as(&args[0], CType::Int32, sc), self.lower_as(&args[1], CType::Int32, sc)];
                for x in &args[2..] {
                    a.push(self.lower_as(x, CType::Real, sc));
                }
                return (GExpr::call("candleAverage", a), CType::Real);
            }
            _ => {}
        }
        if let Some(b) = SpecialBuiltin::from_name(fname) {
            return match b {
                SpecialBuiltin::Unlikely => {
                    let [c] = args else { self.fail("UNLIKELY takes one condition") };
                    self.lower(c, sc)
                }
                SpecialBuiltin::UnstablePeriod | SpecialBuiltin::UnstableAuto => self.unstable(b, args, sc),
                SpecialBuiltin::IsZero | SpecialBuiltin::IsZeroOrNeg | SpecialBuiltin::IsFinite => {
                    let f = match b {
                        SpecialBuiltin::IsZero => "isZero",
                        SpecialBuiltin::IsZeroOrNeg => "isZeroOrNeg",
                        _ => "isFinite",
                    };
                    (GExpr::call(f, vec![self.lower_as(&args[0], CType::Real, sc)]), CType::Bool)
                }
                SpecialBuiltin::IsZeroScaled => {
                    let [x, s] = args else { self.fail("IS_ZERO_SCALED takes two arguments") };
                    let x = self.lower_as(x, CType::Real, sc);
                    let s = self.lower_as(s, CType::Real, sc);
                    (GExpr::call("isZeroScaled", vec![x, s]), CType::Bool)
                }
                SpecialBuiltin::ArrayCopy => {
                    let [dst, dst_off, src, src_off, count] = args else { self.fail("ARRAY_COPY takes five arguments") };
                    let d = self.lower(dst, sc).0;
                    let doff = self.lower_as(dst_off, CType::Int32, sc);
                    let s = self.lower(src, sc).0;
                    let soff = self.lower_as(src_off, CType::Int32, sc);
                    let n = self.lower_as(count, CType::Int32, sc);
                    (copy_call(d, doff, s, soff, n), CType::Int32)
                }
                SpecialBuiltin::PerToK => {
                    let x = self.lower_as(&args[0], CType::Real, sc);
                    let den = GExpr::bin(x, GOp::Add, GExpr::name("1.0")).paren();
                    (GExpr::bin(GExpr::name("2.0"), GOp::Quo, den), CType::Real)
                }
            };
        }
        if let Some(mf) = MathFn::from_name(fname) {
            return match mf {
                MathFn::Max | MathFn::Min => {
                    let [a, b] = args else { self.fail("max/min takes two arguments") };
                    let (ag, at) = self.lower(a, sc);
                    let (bg, bt) = self.lower(b, sc);
                    let t = arith(at, bt);
                    let f = match (mf, t) {
                        (MathFn::Max, CType::Real) => "maxF",
                        (MathFn::Min, CType::Real) => "minF",
                        (MathFn::Max, _) => "maxI",
                        _ => "minI",
                    };
                    (GExpr::call(f, vec![self.conv(ag, at, t), self.conv(bg, bt, t)]), t)
                }
                _ => {
                    let a: Vec<GExpr> = args.iter().map(|x| self.lower_as(x, CType::Real, sc)).collect();
                    (GExpr::call(format!("math.{}", math_name(mf)), a), CType::Real)
                }
            };
        }
        if let Some(s) = StdlibFn::from_name(fname) {
            return match s {
                StdlibFn::Sizeof => (GExpr::name("1"), CType::Int32),
                StdlibFn::Malloc => {
                    let int = find_sizeof_type(&args[0]).as_deref() == Some("int");
                    let n = self.lower_as(&strip_sizeof(&args[0]), CType::Int32, sc);
                    let (ty, t) = if int { ("[]int32", CType::IntPtr) } else { ("[]float64", CType::RealPtr) };
                    (GExpr::call("make", vec![GExpr::name(ty), n]), t)
                }
                StdlibFn::Free => self.fail("free() reached the renderer: `ir_cleanup::drop_deallocation` runs on every body"),
                StdlibFn::Memcpy | StdlibFn::Memmove => {
                    let (d, doff) = self.array_ref(&args[0], sc);
                    let (s, soff) = self.array_ref(&args[1], sc);
                    let n = self.lower_as(&strip_sizeof(&args[2]), CType::Int32, sc);
                    (copy_call(d, doff, s, soff, n), CType::Int32)
                }
                StdlibFn::Memset => {
                    if !matches!(args[1], Expr::IntLiteral(0)) {
                        self.fail("memset to a non-zero byte");
                    }
                    let (d, off) = self.array_ref(&args[0], sc);
                    let n = self.lower_as(&strip_sizeof(&args[2]), CType::Int32, sc);
                    let hi = if off.is_name("0") { n } else { GExpr::bin(off.clone(), GOp::Add, n) };
                    (GExpr::call("clear", vec![GExpr::slice(d, Some(off), Some(hi))]), CType::Int32)
                }
            };
        }
        self.registry_call(fname, args, sc)
    }

    fn array_ref(&self, e: &Expr, sc: &mut Scope) -> (GExpr, GExpr) {
        match e {
            Expr::AddressOf(inner) => match inner.as_ref() {
                Expr::ArrayAccess(name, off) => (GExpr::name(go_ident(name)), self.lower_as(off, CType::Int32, sc)),
                _ => self.fail(&format!("address of a non-element in a copy: {e:?}")),
            },
            Expr::Var(name) => (GExpr::name(go_ident(name)), GExpr::name("0")),
            _ => self.fail(&format!("unsupported copy operand: {e:?}")),
        }
    }

    fn unstable(&self, b: SpecialBuiltin, args: &[Expr], sc: &mut Scope) -> (GExpr, CType) {
        let Some(Expr::Var(id)) = args.first() else { self.fail("an unstable-period read takes an id and a count") };
        let id = GExpr::name(format!("FuncUnstId{}", unst_variant_name(id)));
        let counts = super::builtins::unstable_level_counts(args).unwrap_or_else(|| self.fail("an unstable-period read takes an id and a count"));
        let read = |c: Vec<GExpr>| {
            let mut a = vec![id.clone()];
            a.extend(c);
            GExpr::call("c.unstableCount", a)
        };
        if matches!(b, SpecialBuiltin::UnstablePeriod) {
            let c: Vec<GExpr> = counts.iter().map(|e| self.lower_as(e, CType::Int32, sc)).collect();
            return (read(c), CType::Int32);
        }
        let fill = |v: &str| vec![GExpr::name(v); counts.len()];
        let on = read(fill("1"));
        let off = read(fill("0"));
        let mut arm = Scope::default();
        let c: Vec<GExpr> = counts.iter().map(|e| self.lower_as(e, CType::Int32, &mut arm)).collect();
        let counted = GExpr::bin(read(c), GOp::Sub, off.clone());
        let tmp = self.temp("T");
        sc.pre.push(format!("var {tmp} int32"));
        sc.pre.push(format!("if {} {{", GExpr::bin(on, GOp::Ne, off).render()));
        sc.pre.extend(indented(&arm.pre));
        sc.pre.push(format!("\t{tmp} = {}", counted.render()));
        sc.pre.extend(indented(&arm.post));
        sc.pre.push("}".to_string());
        (GExpr::name(tmp), CType::Int32)
    }

    /// A call to another indicator's C-shaped lookback or private tier.
    fn registry_call(&self, fname: &str, args: &[Expr], sc: &mut Scope) -> (GExpr, CType) {
        let svc = self.svc;
        let Some((key, suffix)) = svc.registry.split_call(fname) else {
            self.fail(&format!("call to unknown function `{fname}`"))
        };
        let def = svc.registry.def(&key).unwrap_or_else(|| self.fail(&format!("no definition for `{key}`")));
        let sym = svc.registry.resolve_call(fname, Lang::Go);
        if suffix.as_deref() == Some("lookback") {
            if args.len() != def.optional_inputs.len() {
                self.fail(&format!("{} takes {} optional arguments", def.name, def.optional_inputs.len()));
            }
            let a: Vec<GExpr> = def.optional_inputs.iter().zip(args).map(|(o, x)| self.lower_as(x, impl_param_ctype(o), sc)).collect();
            return (GExpr::call(format!("c.{sym}"), a), CType::Int32);
        }
        let a: Vec<GExpr> = args.iter().map(|x| self.lower(x, sc).0).collect();
        (GExpr::call(format!("c.{sym}"), a), CType::Int32)
    }

    // ---- statements --------------------------------------------------------

    pub(super) fn stmts(&self, body: &[Statement], out: &mut Vec<String>) {
        for s in body {
            self.stmt(s, out);
        }
    }

    pub(super) fn block(&self, body: &[Statement]) -> Vec<String> {
        let mut v = Vec::new();
        self.stmts(body, &mut v);
        v
    }

    #[allow(clippy::too_many_lines)]
    pub(super) fn stmt(&self, s: &Statement, out: &mut Vec<String>) {
        match s {
            Statement::VarDecl { var_type, name, init } => {
                out.push(format!("var {} {}", go_ident(name), var_go_type(var_type)));
                if matches!(var_type, VarType::RealArray(_) | VarType::IntArray(_)) {
                    return;
                }
                if let Some(init) = init {
                    self.check_steps(&[init]);
                    let (init, hoisted) = self.hoist(init);
                    self.render_hoisted(&hoisted, out);
                    let mut sc = Scope::default();
                    let v = self.store_value(&Expr::Var(name.clone()), CType::of_var_type(var_type), &init, &mut sc);
                    out.extend(sc.pre);
                    out.push(format!("{} = {}", go_ident(name), v.render()));
                    out.extend(sc.post);
                }
            }
            Statement::Assign { target, value, compound } => self.assign(target, value, *compound, out),
            Statement::Expr(e) => self.expr_stmt(e, out),
            Statement::While { condition, body } => {
                self.check_steps(&[condition]);
                let mut sc = Scope::default();
                let c = self.cond(condition, &mut sc);
                let b = self.block(body);
                if sc.is_empty() {
                    out.push(format!("for {} {{", strip_paren(c).render()));
                } else {
                    out.push("for {".to_string());
                    out.extend(indented(&self.break_unless(c, sc)));
                }
                out.extend(indented(&b));
                out.push("}".to_string());
            }
            Statement::DoWhile { condition, body } => {
                let b = self.block(body);
                self.check_steps(&[condition]);
                let mut sc = Scope::default();
                let c = self.cond(condition, &mut sc);
                if continues(body) {
                    if !sc.is_empty() {
                        self.fail("a do-while whose condition has side effects and whose body continues");
                    }
                    let tmp = self.temp("D");
                    out.push(format!("for {tmp} := true; {tmp}; {tmp} = {} {{", c.render()));
                    out.extend(indented(&b));
                    out.push("}".to_string());
                    return;
                }
                out.push("for {".to_string());
                out.extend(indented(&b));
                out.extend(indented(&self.break_unless(c, sc)));
                out.push("}".to_string());
            }
            Statement::For { var, count, body } => {
                self.check_steps(&[count]);
                let mut sc = Scope::default();
                let n = self.value_for(CType::Int32, count, &mut sc);
                if !sc.post.is_empty() {
                    self.fail("a countdown loop whose count has a side effect after it");
                }
                out.extend(sc.pre);
                let v = go_ident(var);
                out.push(format!("for {v} = {}; {v} > 0; {v}-- {{", n.render()));
                out.extend(indented(&self.block(body)));
                out.push("}".to_string());
            }
            Statement::ForC { init, condition, update, body } => self.for_c(init, condition, update, body, out),
            Statement::If { condition, then_body, else_body, .. } => self.if_stmt(condition, then_body, else_body, out),
            Statement::Return { value } => self.return_stmt(value.as_ref(), out),
            Statement::Break => out.push("break".to_string()),
            Statement::Continue => out.push("continue".to_string()),
            Statement::Switch { expr, cases, default } => {
                self.check_steps(&[expr]);
                let mut sc = Scope::default();
                let (e, _) = self.lower(expr, &mut sc);
                if !sc.post.is_empty() {
                    self.fail("a switch subject with a side effect after it");
                }
                out.extend(sc.pre);
                out.push(format!("switch {} {{", strip_paren(e).render()));
                for (label, body) in cases {
                    out.push(format!("case {}:", self.switch_label(label)));
                    out.extend(indented(&self.block(strip_trailing_break(body))));
                }
                if !default.is_empty() {
                    out.push("default:".to_string());
                    out.extend(indented(&self.block(strip_trailing_break(default))));
                }
                out.push("}".to_string());
            }
            Statement::Block { body } => self.stmts(body, out),
            Statement::CircBuf(op) => self.circ_buf(op, out),
            Statement::UnrollHint { .. } | Statement::Comment(_) => {}
        }
    }

    /// The lines that evaluate `c` and leave the loop when it is false.
    fn break_unless(&self, c: GExpr, sc: Scope) -> Vec<String> {
        let mut v = sc.pre;
        if sc.post.is_empty() {
            v.push(format!("if {} {{", GExpr::not(c).render()));
        } else {
            let tmp = self.temp("C");
            v.push(format!("{tmp} := {}", c.render()));
            v.extend(sc.post);
            v.push(format!("if !{tmp} {{"));
        }
        v.push("\tbreak".to_string());
        v.push("}".to_string());
        v
    }

    fn switch_label(&self, label: &str) -> String {
        match lookup_variant(label, self.svc.enums) {
            Some((enum_name, variant)) => format!("int32({enum_name}{})", variant.name),
            None => label.to_string(),
        }
    }

    fn for_c(&self, init: &Statement, condition: &Expr, update: &Statement, body: &[Statement], out: &mut Vec<String>) {
        let init_lines = self.block(std::slice::from_ref(init));
        let header_init = match init_lines.as_slice() {
            [one] if is_simple_stmt(one) => one.clone(),
            _ => {
                out.extend(init_lines);
                String::new()
            }
        };
        let post = self.for_post(update);
        self.check_steps(&[condition]);
        let mut sc = Scope::default();
        let c = self.cond(condition, &mut sc);
        let b = self.block(body);
        if sc.is_empty() {
            let c = strip_paren(c).render();
            out.push(if post.is_empty() { format!("for {header_init}; {c}; {{") } else { format!("for {header_init}; {c}; {post} {{") });
            out.extend(indented(&b));
        } else {
            out.push(if post.is_empty() { format!("for {header_init}; ; {{") } else { format!("for {header_init}; ; {post} {{") });
            out.extend(indented(&self.break_unless(c, sc)));
            out.extend(indented(&b));
        }
        out.push("}".to_string());
    }

    /// A `for` post statement: one simple statement, or a parallel assignment
    /// for C's comma-separated updates when none reads an earlier update's target.
    fn for_post(&self, update: &Statement) -> String {
        let parts: Vec<&Statement> = match update {
            Statement::Block { body } => body.iter().collect(),
            s => vec![s],
        };
        if parts.is_empty() {
            return String::new();
        }
        if parts.len() == 1 {
            let lines = self.block(std::slice::from_ref(parts[0]));
            return match lines.as_slice() {
                [one] if is_simple_stmt(one) => one.clone(),
                [] => String::new(),
                _ => self.fail("a for-loop update that needs statements of its own"),
            };
        }
        let mut lhs = Vec::new();
        let mut rhs = Vec::new();
        let mut targets: Vec<&str> = Vec::new();
        for p in &parts {
            let Statement::Assign { target: Expr::Var(t), value, .. } = p else {
                self.fail("a comma-separated for-loop update that is not an assignment")
            };
            let mut counts = HashMap::new();
            count_vars(value, &mut counts);
            if targets.iter().any(|earlier| counts.contains_key(*earlier)) {
                self.fail("a comma-separated for-loop update that reads an earlier update's target");
            }
            targets.push(t);
            let mut sc = Scope::default();
            let ty = self.env.lookup(t).unwrap_or_else(|| self.fail(&format!("`{t}` has no declared type")));
            let v = self.value_for(ty, value, &mut sc);
            if !sc.is_empty() {
                self.fail("a comma-separated for-loop update with side effects");
            }
            lhs.push(GExpr::name(go_ident(t)).render_at(2));
            rhs.push(v.render_at(2));
        }
        format!("{} = {}", lhs.join(", "), rhs.join(", "))
    }

    fn if_stmt(&self, condition: &Expr, then_body: &[Statement], else_body: &[Statement], out: &mut Vec<String>) {
        if contains_alloc_err_return(then_body) {
            return;
        }
        let (condition, _) = super::builtins::strip_unlikely(condition);
        self.check_steps(&[condition]);
        let (condition, hoisted) = self.hoist(condition);
        self.render_hoisted(&hoisted, out);
        let mut sc = Scope::default();
        let c = self.cond(&condition, &mut sc);
        out.extend(sc.pre);
        let c = if sc.post.is_empty() {
            strip_paren(c)
        } else {
            let tmp = self.temp("C");
            out.push(format!("{tmp} := {}", c.render()));
            out.extend(sc.post);
            GExpr::name(tmp)
        };
        out.push(format!("if {} {{", c.render()));
        out.extend(indented(&self.block(then_body)));
        if else_body.is_empty() {
            out.push("}".to_string());
            return;
        }
        let code: Vec<&Statement> = else_body.iter().filter(|s| !matches!(s, Statement::Comment(_))).collect();
        if let [inner @ Statement::If { .. }] = code.as_slice() {
            let lines = self.block(std::slice::from_ref(*inner));
            if lines.first().is_some_and(|l| l.starts_with("if ")) {
                out.push(format!("}} else {}", lines[0]));
                out.extend(lines[1..].iter().cloned());
                return;
            }
            out.push("} else {".to_string());
            out.extend(indented(&lines));
            out.push("}".to_string());
            return;
        }
        out.push("} else {".to_string());
        out.extend(indented(&self.block(else_body)));
        out.push("}".to_string());
    }

    fn return_stmt(&self, value: Option<&Expr>, out: &mut Vec<String>) {
        let Some(e) = value else {
            out.push(if self.ret == Ret::Fallible { "return nil" } else { "return" }.to_string());
            return;
        };
        if let Expr::FuncCall(fname, args) = e {
            if self.svc.registry.contains(fname) && self.cross_call(fname, args, out) {
                out.push("return Success".to_string());
                return;
            }
        }
        self.check_steps(&[e]);
        let (e, hoisted) = self.hoist(e);
        self.render_hoisted(&hoisted, out);
        let mut sc = Scope::default();
        let g = match self.ret {
            Ret::Int => self.value_for(CType::Int32, &e, &mut sc),
            Ret::Code => self.code_value(&e, &mut sc),
            Ret::Value(t) => self.value_for(t, &e, &mut sc),
            Ret::Fallible => self.fail("a value returned from a step"),
        };
        if !sc.post.is_empty() {
            self.fail("a returned value with a side effect after it");
        }
        out.extend(sc.pre);
        if matches!(self.ret, Ret::Value(_)) {
            out.push(format!("return {}, nil", g.render()));
        } else {
            out.push(format!("return {}", g.render()));
        }
    }

    /// `e` where a `RetCode` is wanted.
    pub(super) fn code_value(&self, e: &Expr, sc: &mut Scope) -> GExpr {
        let is_code = match e {
            Expr::Var(n) => retcode_name(n).is_some() || self.codes.contains(n),
            _ => false,
        };
        let g = self.value_for(CType::Int32, e, sc);
        if is_code {
            g
        } else {
            GExpr::call("RetCode", vec![g])
        }
    }

    /// The value stored into `target`, converted to its type `dest`.
    pub(super) fn store_value(&self, target: &Expr, dest: CType, value: &Expr, sc: &mut Scope) -> GExpr {
        if let Expr::Var(n) = target {
            if self.codes.contains(n) {
                return self.code_value(value, sc);
            }
        }
        self.value_for(dest, value, sc)
    }

    fn target(&self, target: &Expr, sc: &mut Scope) -> (GExpr, CType) {
        match target {
            Expr::Var(n) if n == "outBegIdx" || n == "outNBElement" => (GExpr::unary("*", GExpr::name(n.clone())), CType::Int32),
            Expr::Var(n) => {
                let t = self.env.lookup(n).unwrap_or_else(|| self.fail(&format!("`{n}` has no declared type")));
                (GExpr::name(go_ident(n)), t)
            }
            other => self.lower(other, sc),
        }
    }

    fn assign(&self, target: &Expr, value: &Expr, compound: bool, out: &mut Vec<String>) {
        if let Expr::FuncCall(fname, args) = value {
            if self.svc.registry.contains(fname) && self.cross_call(fname, args, out) {
                let mut sc = Scope::default();
                let (t, _) = self.target(target, &mut sc);
                out.push(format!("{} = Success", t.render()));
                return;
            }
        }
        self.check_steps(&[target, value]);
        let (value, hoisted) = self.hoist(value);
        let value = if fma::EMIT_FMA { fma::canonicalize_accumulator_add(target, &value) } else { value };
        self.render_hoisted(&hoisted, out);
        let mut sc = Scope::default();
        if compound {
            if let (Expr::Var(tname), Expr::BinOp(left, op, right)) = (target, &value) {
                if matches!(left.as_ref(), Expr::Var(l) if l == tname) {
                    if let Some(line) = self.compound(target, op, right, &mut sc) {
                        out.extend(sc.pre);
                        out.push(line);
                        out.extend(sc.post);
                        return;
                    }
                }
            }
        }
        let (t, tt) = self.target(target, &mut sc);
        let v = self.store_value(target, tt, &value, &mut sc);
        out.extend(sc.pre);
        out.push(format!("{} = {}", t.render(), v.render()));
        out.extend(sc.post);
    }

    /// C's `t op= right` with the target a variable: `right` alone is fused,
    /// as C and every other port render it. A real product is still stored
    /// through a conversion, and an `int` target taking a real goes through C's
    /// truncating conversion.
    fn compound(&self, target: &Expr, op: &BinOp, right: &Expr, sc: &mut Scope) -> Option<String> {
        let gop = match op {
            BinOp::Add => GOp::Add,
            BinOp::Sub => GOp::Sub,
            BinOp::Mul => GOp::Mul,
            BinOp::Div => GOp::Quo,
            BinOp::BitwiseAnd => GOp::And,
            BinOp::BitwiseOr => GOp::Or,
            BinOp::BitwiseXor => GOp::Xor,
            BinOp::Shl => GOp::Shl,
            BinOp::Shr => GOp::Shr,
            _ => return None,
        };
        let (t, tt) = self.target(target, sc);
        let (r, rt) = self.lower(right, sc);
        let ts = t.render();
        match tt {
            CType::Real => {
                let r = self.conv(r, rt, CType::Real);
                if gop == GOp::Mul {
                    Some(format!("{ts} = {}", round(GExpr::bin(t, GOp::Mul, r)).render()))
                } else {
                    Some(format!("{ts} {}= {}", gop.token(), r.render()))
                }
            }
            CType::Int32 if rt == CType::Real => {
                let mut v = GExpr::bin(self.conv(t, CType::Int32, CType::Real), gop, r);
                if gop == GOp::Mul {
                    v = round(v);
                }
                Some(format!("{ts} = {}", GExpr::call("f2i", vec![v]).render()))
            }
            CType::Int32 => Some(format!("{ts} {}= {}", gop.token(), self.conv(r, rt, CType::Int32).render())),
            _ => None,
        }
    }

    fn expr_stmt(&self, e: &Expr, out: &mut Vec<String>) {
        match e {
            Expr::Var(_) => {}
            Expr::FuncCall(name, _) if name == super::stmt_walk::OPAQUE => {}
            Expr::FuncCall(fname, args) => {
                if let Some(helper) = self.svc.helpers.get(fname) {
                    if matches!(try_inline_expr(helper, args), Some(Expr::Var(_))) {
                        return;
                    }
                }
                self.check_steps(&[e]);
                let mut sc = Scope::default();
                let (g, _) = self.call(fname, args, &mut sc);
                out.extend(sc.pre);
                out.push(g.render());
                out.extend(sc.post);
            }
            Expr::PostIncrement(i) | Expr::PreIncrement(i) | Expr::PostDecrement(i) | Expr::PreDecrement(i) => {
                let (g, _) = self.step_target(i);
                let op = if matches!(e, Expr::PostIncrement(_) | Expr::PreIncrement(_)) { "++" } else { "--" };
                out.push(format!("{}{op}", g.render()));
            }
            other => self.fail(&format!("an expression statement {other:?}")),
        }
    }

    fn circ_buf(&self, op: &CircBuf, out: &mut Vec<String>) {
        match op {
            CircBuf::Prolog { .. } | CircBuf::Destroy { .. } => {}
            CircBuf::Next { id } => {
                out.push(format!("{id}_Idx++"));
                out.push(format!("if {id}_Idx > maxIdx_{id} {{"));
                out.push(format!("\t{id}_Idx = 0"));
                out.push("}".to_string());
            }
            CircBuf::Init { id, layout, size } => {
                let mut sc = Scope::default();
                let sz = self.value_for(CType::Int32, size, &mut sc);
                if !sc.is_empty() {
                    self.fail("a CIRCBUF size with side effects");
                }
                let sz = sz.render();
                out.push(format!("if {sz} < 1 {{"));
                out.push("\treturn InternalError".to_string());
                out.push("}".to_string());
                for (arr, t) in circbuf_arrays(id, layout) {
                    out.push(format!("{} = make({}, {sz})", go_ident(&arr), circbuf_type(&t)));
                }
                out.push(format!("maxIdx_{id} = {sz} - 1"));
                out.push(format!("{id}_Idx = 0"));
            }
            CircBuf::InitLocalOnly { id, layout } => {
                for (arr, t) in circbuf_arrays(id, layout) {
                    out.push(format!("{} = make({}, maxIdx_{id}+1)", go_ident(&arr), circbuf_type(&t)));
                }
            }
        }
    }

    /// A call to another indicator's public tier. Its error returns from the
    /// body as the code it carries; the range binds the out-parameters.
    fn cross_call(&self, fname: &str, args: &[Expr], out: &mut Vec<String>) -> bool {
        let reg = self.svc.registry;
        let Some(split) = cross_call_split(fname, args, reg) else { return false };
        let Some(def) = reg.def(fname) else { return false };
        let n_in = def.inputs.len();
        let lead = &args[..split];
        if lead.len() != 2 + n_in + def.optional_inputs.len() {
            self.fail(&format!("cross-call to {fname} has {} leading arguments", lead.len()));
        }
        self.check_steps(&args.iter().collect::<Vec<_>>());
        let mut sc = Scope::default();
        let mut a = Vec::new();
        for x in &lead[..2] {
            a.push(as_int(self.lower_as(x, CType::Int32, &mut sc)));
        }
        for x in &lead[2..2 + n_in] {
            a.push(self.lower(x, &mut sc).0);
        }
        for (o, x) in def.optional_inputs.iter().zip(&lead[2 + n_in..]) {
            a.push(match &o.param_type {
                ParamType::Integer => as_int(self.lower_as(x, CType::Int32, &mut sc)),
                ParamType::Enum(name) => GExpr::call(name.clone(), vec![self.lower_as(x, CType::Int32, &mut sc)]),
                ParamType::Real => self.lower_as(x, CType::Real, &mut sc),
                ParamType::Price(_) => unreachable!("Price expanded during parsing"),
            });
        }
        for x in &args[split + 2..] {
            a.push(self.lower(x, &mut sc).0);
        }
        if !sc.post.is_empty() {
            self.fail("a cross-call argument with a side effect after it");
        }
        out.extend(sc.pre);
        let n = self.counter().get();
        self.counter().set(n + 1);
        let (r, err) = (format!("_X{n}"), format!("_E{n}"));
        let call = GExpr::call(format!("c.{}", reg.resolve_call(fname, Lang::Go)), a);
        out.push(format!("{r}, {err} := {}", call.render()));
        out.push(format!("if {err} != nil {{"));
        out.push(format!("\treturn {err}.(RetCode)"));
        out.push("}".to_string());
        let beg = self.out_meta(&args[split]);
        let nb = self.out_meta(&args[split + 1]);
        out.push(format!("{beg} = int32({r}.BegIdx)"));
        out.push(format!("{nb} = int32({r}.NBElement)"));
        true
    }

    fn out_meta(&self, arg: &Expr) -> String {
        let name = match arg {
            Expr::AddressOf(inner) => match inner.as_ref() {
                Expr::Var(n) => return go_ident(n),
                other => self.fail(&format!("an out-parameter {other:?}")),
            },
            Expr::Var(n) => n,
            other => self.fail(&format!("an out-parameter {other:?}")),
        };
        match self.env.lookup(name) {
            Some(CType::IntPtr) => format!("*{}", go_ident(name)),
            _ => go_ident(name),
        }
    }
}

/// An `int32` value where the public tier takes an `int`.
pub(super) fn as_int(g: GExpr) -> GExpr {
    match &g {
        GExpr::Name(s) if s.parse::<i64>().is_ok() => g,
        _ => GExpr::call("int", vec![g]),
    }
}

/// `g` as a typed constant, which Go rounds to a double as C rounds a literal,
/// and which keeps every operation over it rounded.
fn typed(g: GExpr) -> GExpr {
    match &g {
        GExpr::Call(f, _) if f == "float64" => g,
        _ => GExpr::call("float64", vec![g]),
    }
}

/// `float64(g)`: the explicit rounding that keeps Go from fusing a product.
fn round(g: GExpr) -> GExpr {
    GExpr::call("float64", vec![g])
}

fn math_name(mf: MathFn) -> &'static str {
    match mf {
        MathFn::Atan => "Atan",
        MathFn::Sqrt => "Sqrt",
        MathFn::Floor => "Floor",
        MathFn::Ceil => "Ceil",
        MathFn::Log => "Log",
        MathFn::Cos => "Cos",
        MathFn::Sin => "Sin",
        MathFn::Tan => "Tan",
        MathFn::Acos => "Acos",
        MathFn::Asin => "Asin",
        MathFn::Exp => "Exp",
        MathFn::Cosh => "Cosh",
        MathFn::Sinh => "Sinh",
        MathFn::Tanh => "Tanh",
        MathFn::Log10 => "Log10",
        MathFn::Abs => "Abs",
        MathFn::Max | MathFn::Min => unreachable!("rendered as a select"),
    }
}

pub(super) fn copy_call(d: GExpr, doff: GExpr, s: GExpr, soff: GExpr, n: GExpr) -> GExpr {
    let dst = if doff.is_name("0") { d } else { GExpr::slice(d, Some(doff), None) };
    let hi = if soff.is_name("0") { n } else { GExpr::bin(soff.clone(), GOp::Add, n) };
    let lo = if soff.is_name("0") { None } else { Some(soff) };
    GExpr::call("copy", vec![dst, GExpr::slice(s, lo, Some(hi))])
}

pub(super) fn circbuf_type(t: &VarType) -> &'static str {
    if matches!(t, VarType::Integer | VarType::Index) {
        "[]int32"
    } else {
        "[]float64"
    }
}

fn strip_trailing_break(body: &[Statement]) -> &[Statement] {
    match body.last() {
        Some(Statement::Break) => &body[..body.len() - 1],
        _ => body,
    }
}

/// Whether `body` holds a `continue` for the loop it is the body of.
fn continues(body: &[Statement]) -> bool {
    body.iter().any(|s| match s {
        Statement::Continue => true,
        Statement::If { then_body, else_body, .. } => continues(then_body) || continues(else_body),
        Statement::Block { body } => continues(body),
        Statement::Switch { cases, default, .. } => cases.iter().any(|(_, b)| continues(b)) || continues(default),
        _ => false,
    })
}

/// A line Go accepts as a `for` clause: an assignment or a step, one line.
pub(super) fn is_simple_stmt(line: &str) -> bool {
    !line.starts_with("var ") && !line.starts_with("if ") && !line.starts_with("for ") && !line.ends_with('{') && !line.contains(":=")
}

pub(super) fn collect_decl_kinds(stmts: &[Statement], arrays: &mut HashSet<String>, codes: &mut HashSet<String>) {
    for s in stmts {
        match s {
            Statement::VarDecl { var_type, name, .. } => match var_type {
                VarType::RealArray(_) | VarType::IntArray(_) => {
                    arrays.insert(name.clone());
                }
                VarType::RetCodeType => {
                    codes.insert(name.clone());
                }
                _ => {}
            },
            Statement::If { then_body, else_body, .. } => {
                collect_decl_kinds(then_body, arrays, codes);
                collect_decl_kinds(else_body, arrays, codes);
            }
            Statement::While { body, .. } | Statement::DoWhile { body, .. } | Statement::For { body, .. } | Statement::Block { body } => {
                collect_decl_kinds(body, arrays, codes);
            }
            Statement::ForC { init, update, body, .. } => {
                collect_decl_kinds(std::slice::from_ref(init), arrays, codes);
                collect_decl_kinds(std::slice::from_ref(update), arrays, codes);
                collect_decl_kinds(body, arrays, codes);
            }
            Statement::Switch { cases, default, .. } => {
                for (_, b) in cases {
                    collect_decl_kinds(b, arrays, codes);
                }
                collect_decl_kinds(default, arrays, codes);
            }
            _ => {}
        }
    }
}

// ---- signatures and tiers --------------------------------------------------

pub(super) fn impl_param_ctype(o: &OptInput) -> CType {
    CType::of_param(&o.param_type)
}

pub(super) fn impl_opt_type(o: &OptInput) -> &'static str {
    match o.param_type {
        ParamType::Real => "float64",
        _ => "int32",
    }
}

pub(super) fn public_opt_type(o: &OptInput) -> String {
    match &o.param_type {
        ParamType::Real => "float64".to_string(),
        ParamType::Integer => "int".to_string(),
        ParamType::Enum(name) => name.clone(),
        ParamType::Price(_) => unreachable!("Price expanded during parsing"),
    }
}

pub(super) fn array_type(t: &ParamType) -> &'static str {
    if *t == ParamType::Real {
        "[]float64"
    } else {
        "[]int32"
    }
}

/// `a, b T` for consecutive parameters of one type, as Go writes them.
pub(super) fn param_list(params: &[(String, String)]) -> String {
    let mut groups: Vec<(Vec<&str>, &str)> = Vec::new();
    for (n, t) in params {
        match groups.last_mut() {
            Some((names, ty)) if *ty == t.as_str() => names.push(n),
            _ => groups.push((vec![n], t)),
        }
    }
    groups.iter().map(|(names, ty)| format!("{} {ty}", names.join(", "))).collect::<Vec<_>>().join(", ")
}

/// Each optional parameter's default and range checks, `fail` being what a
/// rejection returns.
#[allow(clippy::cast_possible_truncation)]
pub(super) fn opt_validation(func: &FuncDef, fail: &str, enums: &HashMap<String, EnumDef>) -> Vec<String> {
    let mut out = Vec::new();
    for opt in &func.optional_inputs {
        let Some(default_val) = opt.default else { continue };
        let name = &opt.name;
        match &opt.param_type {
            ParamType::Integer => {
                out.push(format!("if {name} == IntegerDefault {{"));
                out.push(format!("\t{name} = {}", default_val as i32));
                if let Some((min, max)) = opt.range {
                    out.push(format!("}} else if {name} < {} || {name} > {} {{", min as i32, max as i32));
                    out.push(format!("\treturn {fail}"));
                }
                out.push("}".to_string());
            }
            ParamType::Real => {
                out.push(format!("if {name} == RealDefault {{"));
                out.push(format!("\t{name} = {}", real_lit(default_val).render()));
                if let Some((min, max)) = opt.range {
                    let lo = super::common::real_bound_named(min, "", "RealMin", "RealMax");
                    let hi = super::common::real_bound_named(max, "", "RealMin", "RealMax");
                    out.push(format!("}} else if !({name} >= {lo} && {name} <= {hi}) {{"));
                    out.push(format!("\treturn {fail}"));
                }
                out.push("}".to_string());
            }
            ParamType::Enum(enum_name) => {
                let extra = super::common::enum_default_variant(enums, enum_name)
                    .map(|v| format!(" || {name} == int32({enum_name}{})", v.name))
                    .unwrap_or_default();
                let val = enums
                    .get(enum_name)
                    .and_then(|e| e.variants.iter().find(|v| v.value == default_val as i32))
                    .map_or_else(|| (default_val as i32).to_string(), |v| format!("int32({enum_name}{})", v.name));
                out.push(format!("if {name} == IntegerDefault{extra} {{"));
                out.push(format!("\t{name} = {val}"));
                if let Some((min, max)) = super::common::int_bound_exprs(opt, enums, Lang::Go) {
                    out.push(format!("}} else if {name} < {min} || {name} > {max} {{"));
                    out.push(format!("\treturn {fail}"));
                }
                out.push("}".to_string());
            }
            ParamType::Price(_) => {}
        }
    }
    out
}

pub(super) fn emit_candle_unpacking(settings: &BTreeSet<String>) -> Vec<String> {
    let mut out = Vec::new();
    for s in settings {
        let at = format!("c.candleSettings[CandleSettingType{s}]");
        out.push(format!("{s}_rangeType := int32({at}.RangeType)"));
        out.push(format!("{s}_avgPeriod := int32({at}.AvgPeriod)"));
        out.push(format!("{s}_factor := {at}.Factor"));
    }
    out
}

pub(super) fn func_text(sig: &str, lines: &[String]) -> String {
    let mut s = format!("\n{sig} {{\n");
    for l in lines {
        let _ = writeln!(s, "\t{l}");
    }
    s.push_str("}\n");
    s
}

fn gen_lookback(func: &FuncDef, svc: &Services) -> String {
    let names = go_names(&func.name);
    let params: Vec<(String, String)> = func.optional_inputs.iter().map(|o| (o.name.clone(), impl_opt_type(o).to_string())).collect();
    let mut lines = opt_validation(func, "-1", svc.enums);
    match &func.lookback {
        Some(LookbackExpr::Literal(n)) => lines.push(format!("return {n}")),
        Some(LookbackExpr::ParamMinus(p, off)) => lines.push(format!("return {p} - {off}")),
        Some(LookbackExpr::Code(stmts)) => {
            let ctx = Ctx::new(svc, &func.name, stmts, &func.outputs, false, Ret::Int);
            ctx.env.seed_opts(func);
            for s in stmts {
                if let Statement::VarDecl { var_type, name, .. } = s {
                    lines.push(format!("var {} {}", go_ident(name), var_go_type(var_type)));
                }
            }
            lines.extend(emit_candle_unpacking(&detect_candle_settings(stmts)));
            for s in stmts {
                if let Statement::VarDecl { var_type, name, init: Some(init) } = s {
                    let mut sc = Scope::default();
                    let v = ctx.value_for(CType::of_var_type(var_type), init, &mut sc);
                    lines.extend(sc.pre);
                    lines.push(format!("{} = {}", go_ident(name), v.render()));
                    lines.extend(sc.post);
                }
            }
            for s in stmts {
                if !matches!(s, Statement::VarDecl { .. }) {
                    ctx.stmt(s, &mut lines);
                }
            }
        }
        None => lines.push("return 0".to_string()),
    }
    func_text(&format!("func (c *Core) {}({}) int32", names.lookback, param_list(&params)), &lines)
}

fn gen_impl(func: &FuncDef, private: bool, svc: &Services) -> String {
    let names = go_names(&func.name);
    let name = if private { names.private.clone() } else { names.body.clone() };
    let raw = if private { &func.private_body } else { &func.body };
    let folded = super::ts::cleaned(raw, svc.registry);
    let body: &[Statement] = &folded;
    let ctx = Ctx::new(svc, &func.name, body, &func.outputs, true, Ret::Code);
    ctx.env.seed_signature(func);

    let mut params = vec![("startIdx".to_string(), "int32".to_string()), ("endIdx".to_string(), "int32".to_string())];
    for i in &func.inputs {
        params.push((i.name.clone(), array_type(&i.param_type).to_string()));
    }
    for o in &func.optional_inputs {
        params.push((o.name.clone(), impl_opt_type(o).to_string()));
    }
    if private {
        for (n, t) in &func.private_extra_params {
            params.push((n.clone(), if t == "int" { "int32".to_string() } else { "float64".to_string() }));
        }
    }
    params.push(("outBegIdx".to_string(), "*int32".to_string()));
    params.push(("outNBElement".to_string(), "*int32".to_string()));
    for o in &func.outputs {
        params.push((o.name.clone(), array_type(&o.param_type).to_string()));
    }

    let mut lines = Vec::new();
    for stmt in body {
        if let Statement::CircBuf(CircBuf::Prolog { id, layout, static_size }) = stmt {
            for (arr, t) in circbuf_arrays(id, layout) {
                lines.push(format!("var {} {}", go_ident(&arr), circbuf_type(&t)));
            }
            lines.push(format!("var {id}_Idx int32"));
            lines.push(format!("var maxIdx_{id} int32"));
            lines.push(format!("maxIdx_{id} = {}", static_size - 1));
            continue;
        }
        if let Statement::VarDecl { var_type, name, .. } = stmt {
            lines.push(format!("var {} {}", go_ident(name), var_go_type(var_type)));
        }
    }
    lines.extend(emit_candle_unpacking(&detect_candle_settings(body)));
    if !private {
        lines.push("if startIdx < 0 || startIdx > IndexMax {".to_string());
        lines.push("\treturn OutOfRangeStartIndex".to_string());
        lines.push("}".to_string());
        lines.push("if endIdx < 0 || endIdx > IndexMax || endIdx < startIdx {".to_string());
        lines.push("\treturn OutOfRangeEndIndex".to_string());
        lines.push("}".to_string());
        lines.extend(opt_validation(func, "BadParam", svc.enums));
        let mut pairs = Vec::new();
        for (i, a) in func.outputs.iter().enumerate() {
            for b in func.outputs[i + 1..].iter().filter(|b| array_type(&b.param_type) == array_type(&a.param_type)) {
                pairs.push(format!("sameStart({}, {})", a.name, b.name));
            }
        }
        if !pairs.is_empty() {
            lines.push(format!("if {} {{", pairs.join(" || ")));
            lines.push("\treturn BadParam".to_string());
            lines.push("}".to_string());
        }
    }
    ctx.initializers(body, &mut lines);
    for stmt in body {
        if !matches!(stmt, Statement::VarDecl { .. }) {
            ctx.stmt(stmt, &mut lines);
        }
    }
    func_text(&format!("func (c *Core) {name}({}) RetCode", param_list(&params)), &lines)
}

fn gen_public_lookback(func: &FuncDef, enums: &HashMap<String, EnumDef>) -> String {
    let names = go_names(&func.name);
    let params: Vec<(String, String)> = func.optional_inputs.iter().map(|o| (o.name.clone(), public_opt_type(o))).collect();
    let mut lines = Vec::new();
    for o in func.optional_inputs.iter().filter(|o| o.param_type == ParamType::Integer) {
        lines.push(format!("if !fitsInt32({}) {{", o.name));
        lines.push("\treturn 0, BadParam".to_string());
        lines.push("}".to_string());
    }
    let args: Vec<String> = func.optional_inputs.iter().map(impl_arg).collect();
    lines.push(format!("lookback := c.{}({})", names.lookback, args.join(", ")));
    lines.push("if lookback < 0 {".to_string());
    lines.push("\treturn 0, BadParam".to_string());
    lines.push("}".to_string());
    lines.push("return int(lookback), nil".to_string());
    let doc = super::go_doc::lookback_docs(func, enums);
    format!("{doc}{}", func_text(&format!("func (c *Core) {}({}) (int, error)", names.lookback_public, param_list(&params)), &lines))
}

/// A public parameter as the C-shaped tier takes it.
pub(super) fn impl_arg(o: &OptInput) -> String {
    match o.param_type {
        ParamType::Real => o.name.clone(),
        _ => format!("int32({})", o.name),
    }
}

/// The public batch tier: the argument contract in Rust's order (index range,
/// parameters, every buffer length), then C#'s overlap rule, then the body.
fn gen_public(func: &FuncDef, svc: &Services) -> String {
    let names = go_names(&func.name);
    let mut params = vec![("startIdx".to_string(), "int".to_string()), ("endIdx".to_string(), "int".to_string())];
    for i in &func.inputs {
        params.push((i.name.clone(), array_type(&i.param_type).to_string()));
    }
    for o in &func.optional_inputs {
        params.push((o.name.clone(), public_opt_type(o)));
    }
    for o in &func.outputs {
        params.push((o.name.clone(), array_type(&o.param_type).to_string()));
    }
    let fail = |code: &str| vec![format!("\treturn OutRange{{}}, {code}"), "}".to_string()];
    let mut l = vec!["if startIdx < 0 || startIdx > IndexMax {".to_string()];
    l.extend(fail("OutOfRangeStartIndex"));
    l.push("if endIdx < 0 || endIdx > IndexMax || endIdx < startIdx {".to_string());
    l.extend(fail("OutOfRangeEndIndex"));
    for o in func.optional_inputs.iter().filter(|o| o.param_type == ParamType::Integer) {
        l.push(format!("if !fitsInt32({}) {{", o.name));
        l.extend(fail("BadParam"));
    }
    let args: Vec<String> = func.optional_inputs.iter().map(impl_arg).collect();
    l.push(format!("lookback := int(c.{}({}))", names.lookback, args.join(", ")));
    l.push("if lookback < 0 {".to_string());
    l.extend(fail("BadParam"));
    for i in &func.inputs {
        l.push(format!("if len({}) < endIdx+1 {{", i.name));
        l.extend(fail("BadParam"));
    }
    if !func.outputs.is_empty() {
        l.push("outLen := requiredOut(startIdx, endIdx, lookback)".to_string());
    }
    for o in &func.outputs {
        l.push(format!("if len({0}) == 0 || len({0}) < outLen {{", o.name));
        l.extend(fail("BadParam"));
    }
    let mut overlap = Vec::new();
    for o in &func.outputs {
        for i in func.inputs.iter().filter(|i| array_type(&i.param_type) == array_type(&o.param_type)) {
            let partly = o.param_type == ParamType::Real && i.param_type == ParamType::Real;
            overlap.push(format!("{}({}, {})", if partly { "overlapsPartly" } else { "overlaps" }, o.name, i.name));
        }
    }
    for (k, a) in func.outputs.iter().enumerate() {
        for b in func.outputs[k + 1..].iter().filter(|b| array_type(&b.param_type) == array_type(&a.param_type)) {
            overlap.push(format!("overlaps({}, {})", a.name, b.name));
        }
    }
    if !overlap.is_empty() {
        l.push(format!("if {} {{", overlap.join(" || ")));
        l.extend(fail("BadParam"));
    }
    l.push("var outBegIdx, outNBElement int32".to_string());
    let mut call = vec!["int32(startIdx)".to_string(), "int32(endIdx)".to_string()];
    call.extend(func.inputs.iter().map(|i| i.name.clone()));
    call.extend(func.optional_inputs.iter().map(impl_arg));
    call.push("&outBegIdx".to_string());
    call.push("&outNBElement".to_string());
    call.extend(func.outputs.iter().map(|o| o.name.clone()));
    l.push(format!("if retCode := c.{}({}); retCode != Success {{", names.body, call.join(", ")));
    l.push("\treturn OutRange{}, retCode".to_string());
    l.push("}".to_string());
    l.push("return OutRange{BegIdx: int(outBegIdx), NBElement: int(outNBElement)}, nil".to_string());
    let doc = super::go_doc::batch_docs(func, svc.enums, svc.registry);
    format!("{doc}{}", func_text(&format!("func (c *Core) {}({}) (OutRange, error)", names.public, param_list(&params)), &l))
}

/// The near-zero predicates as `runtime.go` computes them: the indicators call
/// those helpers, which no other package can reach, so the server evaluates the
/// same expressions.
#[must_use]
pub fn go_predicate_expr(which: SpecialBuiltin, x: &str, scale: &str) -> String {
    match which {
        SpecialBuiltin::IsZero => format!("-0.00000000000001 < {x} && {x} < 0.00000000000001"),
        SpecialBuiltin::IsZeroScaled => format!("math.Abs({x}) <= float64(0.00000000000001*{scale})"),
        SpecialBuiltin::IsZeroOrNeg => format!("{x} < 0.00000000000001"),
        _ => unreachable!("not a near-zero predicate"),
    }
}

#[cfg(test)]
mod tests {
    use super::{go_predicate_expr, stem_problem, SpecialBuiltin};

    #[test]
    fn the_server_predicates_are_the_runtime_ones() {
        let runtime = std::fs::read_to_string(concat!(env!("CARGO_MANIFEST_DIR"), "/../output/go/runtime.go")).unwrap();
        for which in [SpecialBuiltin::IsZero, SpecialBuiltin::IsZeroScaled, SpecialBuiltin::IsZeroOrNeg] {
            let body = format!("\treturn {}\n", go_predicate_expr(which, "x", "scale"));
            assert!(runtime.contains(&body), "runtime.go no longer returns `{}`", body.trim());
        }
    }

    #[test]
    fn a_stem_go_would_build_constrain_or_ignore_is_refused() {
        for stem in ["sma_test", "ht_linux", "x_amd64", "y_windows", "z_arm64", "_x", "core", "enums"] {
            assert!(stem_problem(stem).is_some(), "{stem}");
        }
        for stem in ["sma", "ht_dcperiod", "test", "linux", "cdl3blackcrows", "swak_2php"] {
            assert!(stem_problem(stem).is_none(), "{stem}");
        }
    }
}
