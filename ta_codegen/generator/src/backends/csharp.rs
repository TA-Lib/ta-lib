//! Generates the shipped C# indicator implementations — a port of `java.rs` over
//! the shared `ExprEmitter` / `StatementEmitter` walkers. One complete,
//! standalone `Core_<NAME>.cs` per indicator (`public partial class Core`), so
//! unlike Java there is no fragment splicing and `generate --func=X` is correct.
//!
//! # Pinned decisions
//!
//! - **Public array surface is `double[]`, not `Span<double>`.** Three separate
//!   mechanisms are array *reference equality*: BBANDS' scratch election
//!   (`if (inReal == outRealUpperBand)`), the output-distinctness reject (#108),
//!   and the `float[]`-overload comparison fold. `Span<T>` has no `==`, so
//!   choosing spans breaks all three at once and drags Rust's `ScratchElection`
//!   pass into C#. A span-based facade can sit on top later, where reference
//!   identity does not apply.
//!
//! - **Cores return `RetCode`; only the public wrapper throws.** Cross-indicator
//!   callees return RetCode that callers *inspect* (MA, BBANDS, MAVP, STOCHRSI,
//!   SAR). Throwing cores would make those inspection sites dead code the
//!   renderer has to delete. Keeping RetCode internally means zero body changes.
//!   Unlike Java there is no `…UnguardedInternal` naming split: C#'s `internal`
//!   accessibility plus overloading (the cores carry the two `out int` params
//!   the public wrappers do not) lets the cores share the public names.
//!
//! - **Scratch buffers are `new double[n]`, not `ArrayPool`, in v1.** ArrayPool
//!   has no lifetime story in the current IR lowering: Java renders
//!   `CircBuf::Destroy` and `free()` as the empty string because it is GC'd, and
//!   11 malloc'ing functions plus 8 runtime-sized CIRCBUF functions each have
//!   ~10 early-return paths that would every one need a matching `Return`.
//!   Revisit as a perf task only after the bitwise gate is green.
//!
//! - **`out int outBegIdx, out int outNBElement`, with a generator-emitted
//!   `outBegIdx = 0; outNBElement = 0;` seeding prologue.** Every guarded core
//!   returns `RetCode.OutOfRangeStartIndex` before either is assigned, which is
//!   CS0177 on ~670 generated methods. The seeding must not perturb any emitted
//!   value (the C server's locals and Java's `MInteger`s are zero-initialized
//!   the same way). C's `&localBegIdx` call arguments render as `out localBegIdx`
//!   over a plain `int` local — none of Java's `MInteger` boxing; a `double`
//!   taken by address still needs the `double[1]` wrapping, because the callee
//!   parameter it binds to is an output *array*.
//!
//! - **Qualified switch labels (`case MAType.Sma:`)** — the exact inverse of
//!   `render_java_switch_label`, which strips the qualifier because qualified
//!   labels are Java 21+. Locals are declared with initializers (as in Java), so
//!   C# definite assignment does not additionally force `default:` arms.
//!
//! - **The float-overload comparison fold is a statement-level fold here.** Java
//!   renders a `float[] == double[]` identity test as a literal `false`; C#
//!   additionally promotes the then-arm of `if (false)` to a CS0162
//!   unreachable-code error under `-warnaserror`, so the shared
//!   [`compat_fold`](super::compat_fold) engine folds the whole `if` away
//!   (dead-code removal only — never a value change).
//!
//! - **Method names are PascalCase**, i.e. the YAML `camel_case` verbatim
//!   (historical spellings like `WillR`, `StochF`, `CdlHignWave` preserved).
//!   This must agree with `Lang::CSharp` in `registry.rs` or every
//!   cross-indicator call targets a method that does not exist — pinned by the
//!   `csharp_method_names_agree_with_registry` test below.

use std::cell::Cell;
use std::collections::{HashMap, HashSet};
use std::fmt::Write as _;

use crate::candle_settings::{detect_candle_settings, emit_csharp_unpacking};
use crate::helper_registry::{hoist_block_helpers, try_inline_expr, HelperRegistry};
use crate::ir::{
    BinOp, CircBuf, EnumDef, Expr, FuncDef, LookbackExpr, ParamType, Statement, VarType,
};
use crate::parser::enums::lookup_variant;
use crate::registry::{Lang, Registry};
use super::builtins::{MathFn, SpecialBuiltin, StdlibFn};
use super::common::{
    contains_alloc_err_return, expr_directly_contains_candle_call, find_sizeof_type, CANDLE_FNS,
};
use super::compat_fold::{fold_compat_cond, fold_cond, CondFold};
use super::expr_walk::{binop_prec, expr_prec, wrap_child, wrap_inlined, ExprEmitter};
use super::fma::{self, FmaVarSets};
use super::java::{
    bool_ternary_collapse, build_matype_map, circbuf_arrays, collect_address_of_vars,
    collect_double_address_of_vars, collect_matype_vars, is_boolean_expr, to_java_method_name,
    unst_pascal_name, BoolTernaryCollapse,
};
use super::stmt_walk::StatementEmitter;

/// Per-render state for the C# backend, mirroring `JavaRenderCtx`.
///
/// There is no `address_of_vars` set: an `int` taken by address stays a plain
/// `int` local and only its `AddressOf` call-argument site changes (to
/// `out name`), so nothing about how the *name* renders depends on it.
pub(crate) struct CsRenderCtx<'a> {
    pub(crate) single_precision: bool,
    /// Real-typed locals taken by address — bound to a callee's output *array*
    /// parameter, so declared `double[1]` and read/written as `name[0]`.
    pub(crate) double_address_of_vars: &'a HashSet<String>,
    pub(crate) float_input_params: &'a HashSet<String>,
    pub(crate) inline_counter: &'a Cell<usize>,
    /// FMA fusion name-sets for the body being rendered (`None` disables
    /// fusion). Same detector as C/Rust/Java, so the four backends fuse
    /// identical sites; `Math.FusedMultiplyAdd` is IEEE correctly-rounded.
    pub(crate) fma: Option<&'a FmaVarSets>,
    /// `TA_MAType_SMA` → `MAType.Sma`, derived from enums.yaml (same map Java
    /// builds — the rendering is identical in both languages).
    pub(crate) matype_map: HashMap<String, String>,
}

/// C# type name for a scalar or pointer `VarType`.
pub(crate) fn cs_type_str(var_type: &VarType) -> &'static str {
    match var_type {
        VarType::Real => "double",
        VarType::Integer | VarType::Index => "int",
        VarType::RetCodeType => "RetCode",
        VarType::RealPointer | VarType::RealArray(_) => "double[]",
        VarType::IntPointer | VarType::IntArray(_) => "int[]",
    }
}

/// C# scalar element type for a CIRCBUF buffer (`double` / `int`).
fn cs_circbuf_elem(t: &VarType) -> &'static str {
    if matches!(t, VarType::Integer) {
        "int"
    } else {
        "double"
    }
}

/// Convert a function to its C# `PascalCase` method base name: the YAML
/// `camel_case` verbatim (`MovingAverage`, `WillR`, `CdlHignWave` — historical
/// spellings preserved for one-to-one diffability against the Java fragments).
/// Falls back to capitalizing the Java camelCase derivation when the YAML field
/// is absent.
pub(crate) fn to_csharp_method_name(name: &str, camel_case: Option<&str>) -> String {
    if let Some(cc) = camel_case {
        return cc.to_string();
    }
    let camel = to_java_method_name(name, None);
    let mut chars = camel.chars();
    match chars.next() {
        None => String::new(),
        Some(c) => c.to_uppercase().collect::<String>() + chars.as_str(),
    }
}

/// Leaf classifier for the single-precision comparison fold: an `==`/`!=`
/// between two bare identifiers of which exactly one is a `float[]` input
/// parameter can never hold (the other side is a `double[]`), and C# rejects
/// the comparison outright — so the enclosing statement folds.
fn float_cmp_leaf(expr: &Expr, float_inputs: &HashSet<String>) -> Option<bool> {
    let Expr::BinOp(l, op @ (BinOp::Eq | BinOp::NotEq), r) = expr else {
        return None;
    };
    let (Expr::Var(ln), Expr::Var(rn)) = (l.as_ref(), r.as_ref()) else {
        return None;
    };
    let li = float_inputs.contains(ln.as_str());
    let ri = float_inputs.contains(rn.as_str());
    // Exactly one side is a float[] input <=> the comparison is cross-typed.
    if li == ri {
        return None;
    }
    Some(matches!(op, BinOp::NotEq))
}

#[allow(clippy::implicit_hasher)]
pub fn generate(
    func: &FuncDef,
    enums: &HashMap<String, EnumDef>,
    registry: &Registry,
    helpers: &HelperRegistry,
) -> String {
    let mut out = String::new();
    out.push_str(super::ta_abstract_c::LICENSE);
    let _ = writeln!(
        out,
        "\n/* Generated by ta_codegen — do not edit. Source: ta_codegen/input/{}/ */\n",
        func.name.to_lowercase()
    );
    out.push_str("using System;\n\n");
    // Generated code carries C's exact statement sequence, so the non-primary
    // variants legitimately hold locals whose last read was in a folded branch
    // (the Rust crate allows `unused_assignments` for the same reason). Scoped
    // to the generated files — the hand-written scaffolding stays fully linted.
    out.push_str("#pragma warning disable CS0219 // variable assigned but never used\n\n");
    out.push_str("namespace TALib;\n\n");
    out.push_str("public partial class Core\n{\n");
    // File-level comments carried from the input .c (e.g. contributors/history).
    for block in &func.header_comments {
        out.push_str(&super::stmt_walk::block_comment(block, 3));
    }
    out.push_str(&gen_lookback(func, enums, registry, helpers));
    if func.has_explicit_private {
        out.push_str(&gen_private(func, false, enums, registry, helpers)); // double
        out.push_str(&gen_private(func, true, enums, registry, helpers)); // float overload
    }
    // Internal cores keep the RetCode + out-int shape: the JSON-RPC server calls
    // these directly, so the harness's retCode ints and output hashes are
    // untouched by the public surface below.
    out.push_str(&gen_func(func, false, false, enums, registry, helpers)); // double guarded
    out.push_str(&gen_func(func, false, true, enums, registry, helpers)); // double unguarded
    out.push_str(&gen_func(func, true, false, enums, registry, helpers)); // float guarded
    out.push_str(&gen_func(func, true, true, enums, registry, helpers)); // float unguarded
    // Public surface: OutRange-returning wrappers over the cores above.
    out.push_str(&gen_public_wrapper(func, false, false, enums));
    out.push_str(&gen_public_wrapper(func, false, true, enums));
    out.push_str(&gen_public_wrapper(func, true, false, enums));
    out.push_str(&gen_public_wrapper(func, true, true, enums));
    out.push_str("}\n");
    out
}

/// Optional-parameter validation prologue (C#): map the `int.MinValue` /
/// `TA_REAL_DEFAULT` sentinels to the documented default value, then reject
/// out-of-range values. One source of truth for both variants: guarded
/// functions fail with `RetCode.BadParam`, lookback functions fail with `-1`.
///
/// Enum params (e.g. `MAType`) are deliberately NOT handled here, and the C#
/// type system is not the reason. A C# enum is an `int` with names: `(MAType)
/// int.MinValue` is a legal value, and the generated JSON-RPC server produces
/// exactly that — `(MAType)GetInt(p, "optInMAType", 0)`. What is missing is the
/// sentinel substitution C emits for every `enum:MAType` optional input:
///
/// ```c
/// if( (int)optInMAType == (int)0x80000000 )   /* ta_MA.c, ta_BBANDS.c, ... */
///    optInMAType = 0;
/// ```
///
/// Without it `MovingAverage(.., (MAType)int.MinValue, ..)` falls through the
/// switch to `default: BadParam` where C returns a 30-bar SMA, and the matching
/// lookback returns 0 rather than 29.
///
/// This is a pre-existing, backend-wide gap shared with Rust and Java, tracked
/// in `ta_codegen/generator/CLAUDE.md` under "Known Code Quality Issues"; the
/// sentinel sweep in `test_codegen.c` excludes `IntegerList` for the same
/// reason. Fixing it is a cross-backend change, not a C#-local one.
// Integer optional-param defaults/ranges are `f64` in the IR; the integer-valued
// casts to `i32` for literal emission are exact, not truncating.
#[allow(clippy::cast_possible_truncation)]
fn emit_opt_param_validation(func: &FuncDef, fail: &str) -> String {
    let mut out = String::new();
    for opt in &func.optional_inputs {
        match &opt.param_type {
            ParamType::Integer => {
                if let Some(default_val) = opt.default {
                    out.push_str(&format!(
                        "      if( {name} == int.MinValue ) {{\n         {name} = {val};\n      }}",
                        name = opt.name,
                        val = default_val as i32
                    ));
                    if let Some((min, max)) = opt.range {
                        let min_i = min as i32;
                        let max_i = max as i32;
                        out.push_str(&format!(
                            " else if( {name} < {min_i} || {name} > {max_i} ) {{\n         return {fail};\n      }}",
                            name = opt.name
                        ));
                    }
                    out.push('\n');
                }
            }
            ParamType::Real => {
                if let Some(default_val) = opt.default {
                    out.push_str(&format!(
                        "      if( {name} == TA_REAL_DEFAULT ) {{\n         {name} = {val:e};\n      }}",
                        name = opt.name,
                        val = default_val
                    ));
                    // Every declared bound is checked (see backends::c).
                    if let Some((min, max)) = opt.range {
                        out.push_str(&format!(
                            " else if( {name} < {lo} || {name} > {hi} ) {{\n         return {fail};\n      }}",
                            name = opt.name,
                            lo = super::common::real_bound_literal(min, "TA_"),
                            hi = super::common::real_bound_literal(max, "TA_")
                        ));
                    }
                    out.push('\n');
                }
            }
            ParamType::Enum(_) | ParamType::Price(_) => {}
        }
    }
    out
}

fn opt_param_type_str(opt: &crate::ir::OptInput) -> &str {
    match &opt.param_type {
        ParamType::Real => "double",
        ParamType::Integer => "int",
        ParamType::Enum(name) => name.as_str(),
        ParamType::Price(_) => unreachable!("Price expanded during parsing"),
    }
}

fn gen_lookback(
    func: &FuncDef,
    enums: &HashMap<String, EnumDef>,
    registry: &Registry,
    helpers: &HelperRegistry,
) -> String {
    let name = to_csharp_method_name(&func.name, func.camel_case.as_deref());

    let param_str = if func.optional_inputs.is_empty() {
        " ".to_string()
    } else {
        let params: Vec<String> = func
            .optional_inputs
            .iter()
            .map(|opt| format!("{} {}", opt_param_type_str(opt), opt.name))
            .collect();
        format!(" {} ", params.join(", "))
    };

    // Same param validation as the guarded function, with the lookback
    // bad-param contract: out-of-range returns -1.
    let validation = emit_opt_param_validation(func, "-1");

    let body = match &func.lookback {
        Some(LookbackExpr::Literal(n)) => format!("{validation}      return {n};"),
        Some(LookbackExpr::ParamMinus(param, offset)) => {
            format!("{validation}      return {param} - {offset};")
        }
        Some(LookbackExpr::Code(stmts)) => {
            format!("{validation}{}", render_lookback_code(stmts, enums, registry, helpers))
        }
        None => format!("{validation}      return 0;"),
    };

    let docs = super::csharp_doc::lookback_docs(func, &name, enums);
    format!(
        "{docs}   public int {name}Lookback({param_str})\n\
         \x20  {{\n\
         {body}\n\
         \x20  }}\n"
    )
}

/// Render a simple init expression for private_param_init VarDecls.
/// Only needs to handle arithmetic on optIn params (e.g., 2.0 / (period + 1)).
fn render_init_expr(expr: &Expr) -> String {
    match expr {
        Expr::Literal(f) => {
            let s = format!("{f}");
            if f.fract() == 0.0 && !s.contains('.') { format!("{s}.0") } else { s }
        }
        Expr::IntLiteral(i) => format!("{i}"),
        Expr::Var(name) => name.clone(),
        Expr::BinOp(lhs, op, rhs) => {
            let op_str = match op {
                BinOp::Add => "+",
                BinOp::Sub => "-",
                BinOp::Mul => "*",
                BinOp::Div => "/",
                _ => panic!("Unsupported op in private_param_init"),
            };
            format!("({}{}{})", render_init_expr(lhs), op_str, render_init_expr(rhs))
        }
        Expr::Cast(_ty, inner) => {
            format!("(double)({})", render_init_expr(inner))
        }
        _ => panic!("Unsupported expr in private_param_init: {expr:?}"),
    }
}

/// Emit the public, `OutRange`-returning wrapper over one internal core.
///
/// Guarded wrappers translate the core's `RetCode` into the documented exception
/// mapping; unguarded wrappers check nothing and never throw. Both are thin: the
/// numerics live entirely in the core (which is an overload of the same name
/// carrying the two `out int` params).
///
/// **A short range is not an error.** A valid range shorter than the lookback
/// returns `Success` with `outNBElement == 0`, which becomes an `OutRange` whose
/// `Count` is 0 — exactly C's contract, never an exception.
fn gen_public_wrapper(
    func: &FuncDef,
    single_precision: bool,
    unguarded: bool,
    enums: &HashMap<String, EnumDef>,
) -> String {
    let base_name = to_csharp_method_name(&func.name, func.camel_case.as_deref());
    let core = if unguarded {
        format!("{base_name}Unguarded")
    } else {
        base_name.clone()
    };
    let public_name = core.clone();

    // Parameters: same as the core minus the two out-int params.
    let mut params: Vec<String> = vec!["int startIdx".to_string(), "int endIdx".to_string()];
    let mut args: Vec<String> = vec!["startIdx".to_string(), "endIdx".to_string()];
    for input in &func.inputs {
        let cs_type = match (&input.param_type, single_precision) {
            (ParamType::Real, true) => "float",
            (ParamType::Real, false) => "double",
            _ => "int",
        };
        params.push(format!("{}[] {}", cs_type, input.name));
        args.push(input.name.clone());
    }
    for opt in &func.optional_inputs {
        params.push(format!("{} {}", opt_param_type_str(opt), opt.name));
        args.push(opt.name.clone());
    }
    args.push("out int outBegIdx".to_string());
    args.push("out int outNBElement".to_string());
    for output in &func.outputs {
        let cs_type = match &output.param_type {
            ParamType::Real => "double",
            _ => "int",
        };
        params.push(format!("{}[] {}", cs_type, output.name));
        args.push(output.name.clone());
    }

    let mut out = String::new();
    if unguarded {
        out.push_str(&super::csharp_doc::unguarded_docs(func, &base_name, single_precision));
    } else {
        out.push_str(&super::csharp_doc::guarded_docs(func, &base_name, single_precision, enums));
    }
    let sig_prefix = format!("   public OutRange {public_name}( ");
    let indent = " ".repeat(sig_prefix.len());
    out.push_str(&sig_prefix);
    for (i, param) in params.iter().enumerate() {
        if i > 0 {
            out.push_str(&format!(",\n{indent}"));
        }
        out.push_str(param);
    }
    out.push_str(" )\n   {\n");
    if unguarded {
        // Checks nothing by contract, so there is no failure to report and the
        // core's RetCode is discarded.
        let _ = write!(out, "      {core}(");
        out.push_str(&args.join(", "));
        out.push_str(");\n");
    } else {
        let _ = write!(out, "      RetCode retCode = {core}(");
        out.push_str(&args.join(", "));
        out.push_str(");\n");
        out.push_str("      if( retCode != RetCode.Success ) {\n");
        let _ = writeln!(out, "         throw Failure(\"{}\", retCode);", func.name);
        out.push_str("      }\n");
    }
    out.push_str("      return new OutRange(outBegIdx, outNBElement);\n");
    out.push_str("   }\n");
    out
}

/// Generate the Private-variant core (double or the float-input overload).
fn gen_private(
    func: &FuncDef,
    single_precision: bool,
    enums: &HashMap<String, EnumDef>,
    registry: &Registry,
    helpers: &HelperRegistry,
) -> String {
    let base_name = to_csharp_method_name(&func.name, func.camel_case.as_deref());
    let name_override = format!("{base_name}Private");
    gen_func_inner(func, single_precision, true, Some(&name_override), enums, registry, helpers)
}

fn gen_func(
    func: &FuncDef,
    single_precision: bool,
    logic: bool,
    enums: &HashMap<String, EnumDef>,
    registry: &Registry,
    helpers: &HelperRegistry,
) -> String {
    gen_func_inner(func, single_precision, logic, None, enums, registry, helpers)
}

#[allow(clippy::too_many_lines, clippy::cognitive_complexity)]
fn gen_func_inner(
    func: &FuncDef,
    single_precision: bool,
    logic: bool,
    name_override: Option<&str>,
    enums: &HashMap<String, EnumDef>,
    registry: &Registry,
    helpers: &HelperRegistry,
) -> String {
    let mut out = String::new();
    let base_name = to_csharp_method_name(&func.name, func.camel_case.as_deref());
    let name = if let Some(n) = name_override {
        n.to_string()
    } else if logic {
        format!("{base_name}Unguarded")
    } else {
        base_name.clone()
    };

    // Build parameter list
    let mut params: Vec<String> = Vec::new();
    params.push("int startIdx".to_string());
    params.push("int endIdx".to_string());

    for input in &func.inputs {
        let cs_type = match (&input.param_type, single_precision) {
            (ParamType::Real, true) => "float",
            (ParamType::Real, false) => "double",
            _ => "int",
        };
        params.push(format!("{}[] {}", cs_type, input.name));
    }

    for opt in &func.optional_inputs {
        params.push(format!("{} {}", opt_param_type_str(opt), opt.name));
    }

    // Extra params only on Private variant (via name_override)
    if name_override.is_some() {
        for (param_name, c_type) in &func.private_extra_params {
            let cs_type = match c_type.as_str() {
                "int" => "int",
                _ => "double",
            };
            params.push(format!("{cs_type} {param_name}"));
        }
    }

    params.push("out int outBegIdx".to_string());
    params.push("out int outNBElement".to_string());

    for output in &func.outputs {
        let cs_type = match &output.param_type {
            ParamType::Real => "double",
            _ => "int",
        };
        params.push(format!("{}[] {}", cs_type, output.name));
    }

    // Format signature. Internal: these are the cores the public OutRange
    // wrappers (and the JSON-RPC server) delegate to, not the API.
    let sig_prefix = format!("   internal RetCode {name}( ");
    let indent = " ".repeat(sig_prefix.len());
    out.push_str(&sig_prefix);
    for (i, param) in params.iter().enumerate() {
        if i > 0 {
            out.push_str(&format!(",\n{indent}"));
        }
        out.push_str(param);
    }
    out.push_str(" )\n");

    // Body
    out.push_str("   {\n");

    // Seeding prologue: the out params must be definitely assigned on every
    // path (CS0177), including the guard returns that precede any assignment
    // in the C body. Zero matches the C server's zero-initialized locals and
    // Java's fresh MIntegers, so no observable value changes.
    out.push_str("      outBegIdx = 0;\n");
    out.push_str("      outNBElement = 0;\n");

    // Body selection (same pattern as the C/Java backends).
    let body = if name_override.is_some() || (single_precision && func.has_explicit_private) {
        &func.private_body
    } else if func.has_explicit_private {
        &func.body
    } else if logic {
        &func.private_body
    } else {
        &func.body
    };

    // Carry source comments only in the double-precision guarded implementation
    // (or the double Private for explicit-private functions).
    let keep_comments = !single_precision && (name_override.is_some() || !logic);
    let body_stripped;
    let body: &[Statement] = if keep_comments {
        body
    } else {
        body_stripped = super::stmt_walk::strip_comments(body);
        &body_stripped
    };

    // Pre-scan for variables used in AddressOf contexts. Integer ones stay
    // plain `int` locals (their call sites render `out name`); Real ones need
    // the `double[1]` wrapping because they bind to callee output arrays.
    let address_of_vars = collect_address_of_vars(body);
    let double_address_of_vars = collect_double_address_of_vars(body, &address_of_vars);

    // In single-precision variants, input params are float[] while outputs are
    // double[]; the comparison fold and per-read widening need the input names.
    let float_input_params: HashSet<String> = if single_precision {
        func.inputs.iter().map(|p| p.name.clone()).collect()
    } else {
        HashSet::new()
    };

    // Local int variables assigned from MAType enum params must be declared
    // `MAType` (C is loose about enum/int; C# is not).
    let matype_params: HashSet<String> = func
        .optional_inputs
        .iter()
        .filter(|o| matches!(&o.param_type, ParamType::Enum(n) if n == "MAType"))
        .map(|o| o.name.clone())
        .collect();
    let matype_vars = collect_matype_vars(body, &matype_params);

    // Declare local variables
    for stmt in body {
        // A CIRCBUF prolog declares heap arrays (+ index/bound) at the function
        // top. maxIdx is seeded here (static_size-1) so INIT_LOCAL_ONLY (HT)
        // has a valid bound and definite assignment is satisfied.
        if let Statement::CircBuf(CircBuf::Prolog {
            id,
            layout,
            static_size,
        }) = stmt
        {
            for (arr, t) in circbuf_arrays(id, layout) {
                out.push_str(&format!("      {}[] {arr};\n", cs_circbuf_elem(&t)));
            }
            out.push_str(&format!("      int {id}_Idx = 0;\n"));
            out.push_str(&format!("      int maxIdx_{id} = ({static_size})-1;\n"));
            continue;
        }
        if let Statement::VarDecl { var_type, name, .. } = stmt {
            let cs_decl = if matype_vars.contains(name) {
                format!("MAType {name}")
            } else if double_address_of_vars.contains(name) {
                format!("double[] {name} = new double[1]")
            } else {
                match var_type {
                    VarType::Real => format!("double {name} = 0"),
                    VarType::Integer | VarType::Index => format!("int {name} = 0"),
                    VarType::RealArray(size) => {
                        format!("double[] {name} = new double[{size}]")
                    }
                    VarType::IntArray(size) => format!("int[] {name} = new int[{size}]"),
                    _ => format!("{} {name}", cs_type_str(var_type)),
                }
            };
            out.push_str(&format!("      {cs_decl};\n"));
        }
    }

    // For S_ variants with _private: emit private_param_init as local VarDecls
    // Both guarded and logic S_ variants need this (both use private_body).
    if single_precision && func.has_explicit_private && name_override.is_none() {
        for (param_name, init_expr) in &func.private_param_init {
            let init_cs = render_init_expr(init_expr);
            out.push_str(&format!("      double {param_name} = {init_cs};\n"));
        }
    }

    // Emit candle settings unpacking (only for referenced settings)
    let candle_used = detect_candle_settings(body);
    if !candle_used.is_empty() {
        out.push_str(&emit_csharp_unpacking(&candle_used, 6));
    }

    // Validation (omitted for Logic/unguarded variant)
    if !logic {
        out.push_str("      if( startIdx < 0 ) {\n");
        out.push_str("         return RetCode.OutOfRangeStartIndex ;\n");
        out.push_str("      }\n");
        out.push_str("      if( (endIdx < 0) || (endIdx < startIdx)) {\n");
        out.push_str("         return RetCode.OutOfRangeEndIndex ;\n");
        out.push_str("      }\n");
        // Optional parameter validation (default + range)
        out.push_str(&emit_opt_param_validation(func, "RetCode.BadParam"));
        // Output-distinctness (issue #108): aliasing two different output
        // arrays has no correct result, so reject it. Input == output stays
        // allowed. Array `==` is reference equality in C# as in Java.
        if func.outputs.len() >= 2 {
            let mut pairs: Vec<String> = Vec::new();
            for i in 0..func.outputs.len() {
                for j in (i + 1)..func.outputs.len() {
                    pairs.push(format!(
                        "{} == {}",
                        func.outputs[i].name, func.outputs[j].name
                    ));
                }
            }
            out.push_str(&format!("      if( {} ) {{\n", pairs.join(" || ")));
            out.push_str("         return RetCode.BadParam ;\n");
            out.push_str("      }\n");
        }
    }

    let inline_counter = Cell::new(0);
    // FMA fusion sites for this body — same detector C/Rust/Java use, so the
    // four backends fuse identical sites.
    let fma_sets = fma::build_fma_var_sets(body, &func.outputs, &fma::UNGUARDED_INDEX_SEEDS);
    let ctx = CsRenderCtx {
        single_precision,
        double_address_of_vars: &double_address_of_vars,
        float_input_params: &float_input_params,
        inline_counter: &inline_counter,
        fma: Some(&fma_sets),
        matype_map: build_matype_map(enums),
    };

    // Emit VarDecl initializations
    for stmt in body {
        if let Statement::VarDecl {
            name,
            init: Some(init),
            ..
        } = stmt
        {
            let mut hoisted_vec = Vec::new();
            let mut cnt = ctx.inline_counter.get();
            let new_init =
                hoist_block_helpers(init, helpers, &mut hoisted_vec, &mut cnt, CANDLE_FNS);
            ctx.inline_counter.set(cnt);
            out.push_str(&render_hoisted_blocks(
                &hoisted_vec, 6, &ctx, enums, registry, helpers,
            ));
            let init_str = render_expr(&new_init, &ctx, registry, helpers);
            if double_address_of_vars.contains(name) {
                out.push_str(&format!("      {name}[0] = {init_str};\n"));
            } else {
                out.push_str(&format!("      {name} = {init_str};\n"));
            }
        }
    }

    // Render body statements (skip VarDecls)
    for stmt in body {
        if matches!(stmt, Statement::VarDecl { .. }) {
            continue;
        }
        out.push_str(&render_statement_ctx(stmt, 6, &ctx, enums, registry, helpers));
    }

    // Closing brace — return statement comes from IR body
    out.push_str("   }\n");

    out
}

/// Render a ForC init or update clause. If it's a Block with multiple
/// statements, comma-separate them instead of using semicolons.
fn render_forc_part(
    stmt: &Statement,
    ctx: &CsRenderCtx,
    enums: &HashMap<String, EnumDef>,
    registry: &Registry,
    helpers: &HelperRegistry,
) -> String {
    match stmt {
        Statement::Block { body } => body
            .iter()
            .map(|s| {
                render_statement_ctx(s, 0, ctx, enums, registry, helpers)
                    .trim()
                    .trim_end_matches(';')
                    .to_string()
            })
            .collect::<Vec<_>>()
            .join(", "),
        _ => render_statement_ctx(stmt, 0, ctx, enums, registry, helpers)
            .trim()
            .trim_end_matches(';')
            .to_string(),
    }
}

/// Render hoisted block-inline helpers as C# code (temp var decl + body).
fn render_hoisted_blocks(
    hoisted: &[(String, VarType, Vec<Statement>)],
    indent: usize,
    ctx: &CsRenderCtx,
    enums: &HashMap<String, EnumDef>,
    registry: &Registry,
    helpers: &HelperRegistry,
) -> String {
    let pad = " ".repeat(indent);
    let mut out = String::new();
    for (temp_name, var_type, body) in hoisted {
        let cs_decl = match var_type {
            VarType::RealArray(size) => format!("double[] {temp_name} = new double[{size}]"),
            VarType::IntArray(size) => format!("int[] {temp_name} = new int[{size}]"),
            VarType::Real => format!("double {temp_name} = 0"),
            VarType::Integer | VarType::Index => format!("int {temp_name} = 0"),
            _ => format!("{} {temp_name}", cs_type_str(var_type)),
        };
        out.push_str(&format!("{pad}{cs_decl};\n"));
        // Declare local variables from the hoisted helper body.
        for stmt in body {
            if let Statement::VarDecl { var_type: vt, name, init } = stmt {
                let type_part = match vt {
                    VarType::RealArray(size) => {
                        out.push_str(&format!("{pad}double[] {name} = new double[{size}];\n"));
                        continue;
                    }
                    VarType::IntArray(size) => {
                        out.push_str(&format!("{pad}int[] {name} = new int[{size}];\n"));
                        continue;
                    }
                    _ => cs_type_str(vt).to_string(),
                };
                if let Some(init_expr) = init {
                    let mut inner_hoisted = Vec::new();
                    let mut cnt = ctx.inline_counter.get();
                    let hoisted_init = hoist_block_helpers(
                        init_expr, helpers, &mut inner_hoisted, &mut cnt, CANDLE_FNS,
                    );
                    ctx.inline_counter.set(cnt);
                    out.push_str(&render_hoisted_blocks(
                        &inner_hoisted, indent, ctx, enums, registry, helpers,
                    ));
                    let init_str = render_expr(&hoisted_init, ctx, registry, helpers);
                    out.push_str(&format!("{pad}{type_part} {name} = {init_str};\n"));
                } else {
                    out.push_str(&format!("{pad}{type_part} {name};\n"));
                }
            }
        }
        for stmt in body {
            if matches!(stmt, Statement::VarDecl { .. }) {
                continue;
            }
            out.push_str(&render_statement_ctx(stmt, indent, ctx, enums, registry, helpers));
        }
    }
    out
}

/// C#-backend leaf formatting for the shared [`StatementEmitter`] tree-walk.
struct CsStmt<'a> {
    ctx: &'a CsRenderCtx<'a>,
    enums: &'a HashMap<String, EnumDef>,
    registry: &'a Registry,
    helpers: &'a HelperRegistry,
}

impl CsStmt<'_> {
    /// Shared `if` tail (then-body + else branch with `} else if` collapse).
    fn render_if_tail(
        &self,
        then_body: &[Statement],
        else_body: &[Statement],
        indent: usize,
    ) -> String {
        let pad = " ".repeat(indent);
        let mut out = String::new();
        for s in then_body {
            out.push_str(&self.walk_stmt(s, indent + 3));
        }
        if else_body.is_empty() {
            out.push_str(&format!("{pad}}}\n"));
        } else {
            let code_start = else_body
                .iter()
                .position(|s| !matches!(s, Statement::Comment(_)))
                .unwrap_or(else_body.len());
            let is_else_if = else_body.len() - code_start == 1
                && matches!(else_body.get(code_start), Some(Statement::If { .. }));
            if is_else_if {
                // The `} else if` collapse pastes the walked inner `if`
                // unbraced after `else` — but a render-time fold (compat or
                // the float-overload comparison) can dissolve that inner `if`
                // into bare statements or nothing, and `} else <bare>` either
                // lets statements escape the else or dangles onto the next
                // sibling. Collapse only when the walk still starts with an
                // `if(`; otherwise fall through to the braced form.
                let inner = self.walk_stmt(&else_body[code_start], indent);
                if inner.trim_start().starts_with("if(") {
                    for c in &else_body[..code_start] {
                        out.push_str(&self.walk_stmt(c, indent));
                    }
                    out.push_str(&format!("{pad}}} else "));
                    out.push_str(inner.trim_start());
                    return out;
                }
            }
            out.push_str(&format!("{pad}}} else {{\n"));
            for s in else_body {
                out.push_str(&self.walk_stmt(s, indent + 3));
            }
            out.push_str(&format!("{pad}}}\n"));
        }
        out
    }
}

impl StatementEmitter for CsStmt<'_> {
    fn comment(&self, lines: &[String], indent: usize) -> String {
        super::stmt_walk::block_comment(lines, indent)
    }

    fn circ_buf(&self, op: &CircBuf, indent: usize) -> String {
        let pad = " ".repeat(indent);
        match op {
            // Prolog: arrays + index/bound declared at the function top by the
            // decl pass. Destroy: GC-managed — no explicit free.
            CircBuf::Prolog { .. } | CircBuf::Destroy { .. } => String::new(),
            // Advance with conditional reset (not modulo) — matches the reference macro.
            CircBuf::Next { id } => {
                format!("{pad}{id}_Idx++;\n{pad}if( {id}_Idx > maxIdx_{id} ) {{ {id}_Idx = 0; }}\n")
            }
            // Runtime-sized: allocate each array to `size` (new arrays zero-fill).
            CircBuf::Init { id, layout, size } => {
                let sz = render_expr(size, self.ctx, self.registry, self.helpers);
                let mut s = String::new();
                // Parity with the pre-cutover reference's CIRCBUF_INIT guard.
                s.push_str(&format!("{pad}if( {sz} < 1 ) return RetCode.AllocErr;\n"));
                for (arr, t) in circbuf_arrays(id, layout) {
                    s.push_str(&format!("{pad}{arr} = new {}[{sz}];\n", cs_circbuf_elem(&t)));
                }
                s.push_str(&format!("{pad}maxIdx_{id} = ({sz})-1;\n"));
                s.push_str(&format!("{pad}{id}_Idx = 0;\n"));
                s
            }
            // Always the static capacity; bound was seeded in the prolog (maxIdx + 1).
            CircBuf::InitLocalOnly { id, layout } => {
                let mut s = String::new();
                for (arr, t) in circbuf_arrays(id, layout) {
                    s.push_str(&format!(
                        "{pad}{arr} = new {}[maxIdx_{id}+1];\n",
                        cs_circbuf_elem(&t)
                    ));
                }
                s
            }
        }
    }

    fn var_decl(&self, var_type: &VarType, name: &str, init: &Option<Expr>, indent: usize) -> String {
        let pad = " ".repeat(indent);
        // Top-level VarDecls are emitted by the function renderer and skipped
        // before calling render_statement. This arm handles block-scoped
        // VarDecls (inside while/for/if bodies).
        let type_str = match var_type {
            VarType::RealArray(size) => {
                return format!("{pad}double[] {name} = new double[{size}];\n");
            }
            VarType::IntArray(size) => {
                return format!("{pad}int[] {name} = new int[{size}];\n");
            }
            _ => cs_type_str(var_type),
        };
        if let Some(init_expr) = init {
            let mut hoisted_vec = Vec::new();
            let mut cnt = self.ctx.inline_counter.get();
            let new_init = hoist_block_helpers(
                init_expr, self.helpers, &mut hoisted_vec, &mut cnt, CANDLE_FNS,
            );
            self.ctx.inline_counter.set(cnt);
            let mut out = render_hoisted_blocks(
                &hoisted_vec, indent, self.ctx, self.enums, self.registry, self.helpers,
            );
            let init_str = render_expr(&new_init, self.ctx, self.registry, self.helpers);
            out.push_str(&format!("{pad}{type_str} {name} = {init_str};\n"));
            out
        } else {
            format!("{pad}{type_str} {name};\n")
        }
    }

    fn assign(&self, target: &Expr, value: &Expr, compound: bool, indent: usize) -> String {
        let pad = " ".repeat(indent);
        // Hoist multi-statement helpers from the value expression
        let mut hoisted = Vec::new();
        let mut cnt = self.ctx.inline_counter.get();
        let new_value = hoist_block_helpers(value, self.helpers, &mut hoisted, &mut cnt, CANDLE_FNS);
        // Canonicalize accumulator recurrences so all backends fuse the same
        // product regardless of operand order.
        let new_value = if fma::EMIT_FMA {
            fma::canonicalize_accumulator_add(target, &new_value)
        } else {
            new_value
        };
        self.ctx.inline_counter.set(cnt);
        let mut out = render_hoisted_blocks(
            &hoisted, indent, self.ctx, self.enums, self.registry, self.helpers,
        );

        // Only fold compound assignments if the original source used +=/-=/etc.
        if compound {
            if let (Expr::Var(tname), Expr::BinOp(left, op, right)) = (target, &new_value) {
                if let Expr::Var(lname) = left.as_ref() {
                    if lname == tname {
                        let op_str = match op {
                            BinOp::Add => "+=",
                            BinOp::Sub => "-=",
                            BinOp::Mul => "*=",
                            BinOp::Div => "/=",
                            _ => "",
                        };
                        if !op_str.is_empty() {
                            let target_str =
                                render_assign_target(target, self.ctx, self.registry, self.helpers);
                            out.push_str(&format!(
                                "{}{} {} {};\n",
                                pad,
                                target_str,
                                op_str,
                                render_expr(right, self.ctx, self.registry, self.helpers)
                            ));
                            return out;
                        }
                    }
                }
            }
        }

        let target_str = render_assign_target(target, self.ctx, self.registry, self.helpers);
        let value_str = render_expr(&new_value, self.ctx, self.registry, self.helpers);
        out.push_str(&format!("{pad}{target_str} = {value_str};\n"));
        out
    }

    fn expr_stmt(&self, e: &Expr, indent: usize) -> String {
        let pad = " ".repeat(indent);
        // Skip bare variable statements (no side effects).
        if matches!(e, Expr::Var(_)) {
            return String::new();
        }
        if let Expr::FuncCall(fname, args) = e {
            // Check if helper inlines to a bare variable (identity helper)
            if let Some(helper) = self.helpers.get(fname) {
                if let Some(inlined) = try_inline_expr(helper, args) {
                    if matches!(inlined, Expr::Var(_)) {
                        return String::new();
                    }
                }
            }
            let rendered = render_func_call(fname, args, self.ctx, self.registry, self.helpers);
            // Skip empty renders (e.g. free() returns "")
            if rendered.is_empty() {
                return String::new();
            }
            return format!("{pad}{rendered};\n");
        }
        String::new()
    }

    fn while_loop(&self, condition: &Expr, body: &[Statement], indent: usize) -> String {
        let pad = " ".repeat(indent);
        let mut hoisted = Vec::new();
        let mut cnt = self.ctx.inline_counter.get();
        let new_condition =
            hoist_block_helpers(condition, self.helpers, &mut hoisted, &mut cnt, CANDLE_FNS);
        self.ctx.inline_counter.set(cnt);
        let mut out = render_hoisted_blocks(
            &hoisted, indent, self.ctx, self.enums, self.registry, self.helpers,
        );
        let cond_str = render_expr(&new_condition, self.ctx, self.registry, self.helpers);
        let cond_cs = if is_boolean_expr(&new_condition, self.helpers) {
            cond_str
        } else {
            format!("({cond_str}) != 0")
        };
        out.push_str(&format!("{pad}while( {cond_cs} ) {{\n"));
        for s in body {
            out.push_str(&self.walk_stmt(s, indent + 3));
        }
        out.push_str(&format!("{pad}}}\n"));
        out
    }

    fn do_while(&self, condition: &Expr, body: &[Statement], indent: usize) -> String {
        let pad = " ".repeat(indent);
        // Hoisted blocks go INSIDE the loop body (before the closing
        // `} while(cond)`) so they execute each iteration.
        let mut hoisted = Vec::new();
        let mut cnt = self.ctx.inline_counter.get();
        let new_condition =
            hoist_block_helpers(condition, self.helpers, &mut hoisted, &mut cnt, CANDLE_FNS);
        self.ctx.inline_counter.set(cnt);
        let mut out = format!("{pad}do {{\n");
        for s in body {
            out.push_str(&self.walk_stmt(s, indent + 3));
        }
        out.push_str(&render_hoisted_blocks(
            &hoisted, indent + 3, self.ctx, self.enums, self.registry, self.helpers,
        ));
        let cond_str = render_expr(&new_condition, self.ctx, self.registry, self.helpers);
        let cond_cs = if is_boolean_expr(&new_condition, self.helpers) {
            cond_str
        } else {
            format!("({cond_str}) != 0")
        };
        out.push_str(&format!("{pad}}} while( {cond_cs} );\n"));
        out
    }

    #[allow(clippy::too_many_lines, clippy::cognitive_complexity)]
    fn if_stmt(
        &self,
        condition: &Expr,
        then_body: &[Statement],
        else_body: &[Statement],
        cond_comments: &[Option<Vec<String>>],
        indent: usize,
    ) -> String {
        let pad = " ".repeat(indent);
        // Skip post-allocation null-check blocks (dead code — `new` never returns null)
        if contains_alloc_err_return(then_body) {
            return String::new();
        }
        // Compatibility is pinned to Default: fold the branch away and splice
        // the surviving arm in place (see `compat_fold`).
        match fold_compat_cond(condition) {
            CondFold::Known(taken) => {
                let kept = if taken { then_body } else { else_body };
                return kept.iter().map(|s| self.walk_stmt(s, indent)).collect();
            }
            CondFold::Open { expr, changed: true } => {
                let rebuilt = Statement::If {
                    condition: expr,
                    then_body: then_body.to_vec(),
                    else_body: else_body.to_vec(),
                    cond_comments: Vec::new(),
                };
                return self.walk_stmt(&rebuilt, indent);
            }
            CondFold::Open { changed: false, .. } => {}
        }
        // Single-precision comparison fold: a float[]-input vs double[] identity
        // test cannot hold and does not even compile in C#; fold the statement
        // (Java renders `false` instead — same dead code, different language
        // tolerance for it: C# raises CS0162 under -warnaserror).
        if self.ctx.single_precision {
            let inputs = self.ctx.float_input_params;
            let leaf = |e: &Expr| float_cmp_leaf(e, inputs);
            match fold_cond(condition, &leaf) {
                CondFold::Known(taken) => {
                    let kept = if taken { then_body } else { else_body };
                    return kept.iter().map(|s| self.walk_stmt(s, indent)).collect();
                }
                CondFold::Open { expr, changed: true } => {
                    let rebuilt = Statement::If {
                        condition: expr,
                        then_body: then_body.to_vec(),
                        else_body: else_body.to_vec(),
                        cond_comments: Vec::new(),
                    };
                    return self.walk_stmt(&rebuilt, indent);
                }
                CondFold::Open { changed: false, .. } => {}
            }
        }
        // Split `if(A && B)` into nested `if(A) { if(B)` when both sides
        // contain a candle helper call — preserves short-circuit evaluation.
        if let Expr::BinOp(left, BinOp::And, right) = condition {
            if expr_directly_contains_candle_call(left)
                && expr_directly_contains_candle_call(right)
            {
                let inner_if = Statement::If {
                    condition: *right.clone(),
                    then_body: then_body.to_vec(),
                    else_body: else_body.to_vec(),
                    cond_comments: Vec::new(),
                };
                let outer_if = Statement::If {
                    condition: *left.clone(),
                    then_body: vec![inner_if],
                    else_body: else_body.to_vec(),
                    cond_comments: Vec::new(),
                };
                return self.walk_stmt(&outer_if, indent);
            }
        }
        // Inline per-operand comments: render the `&&`-chain multi-line.
        if !cond_comments.is_empty()
            && super::stmt_walk::flatten_and(condition).len() == cond_comments.len()
        {
            let mut hoisted = Vec::new();
            let mut cnt = self.ctx.inline_counter.get();
            let new_condition =
                hoist_block_helpers(condition, self.helpers, &mut hoisted, &mut cnt, CANDLE_FNS);
            self.ctx.inline_counter.set(cnt);
            let op_strs: Vec<String> = super::stmt_walk::flatten_and(&new_condition)
                .iter()
                .map(|o| {
                    let s = render_expr(o, self.ctx, self.registry, self.helpers);
                    if is_boolean_expr(o, self.helpers) {
                        if expr_prec(o) < binop_prec(&BinOp::And) {
                            format!("({s})")
                        } else {
                            s
                        }
                    } else {
                        format!("({s}) != 0")
                    }
                })
                .collect();
            let mut out = render_hoisted_blocks(
                &hoisted, indent, self.ctx, self.enums, self.registry, self.helpers,
            );
            out.push_str(&format!("{pad}if( "));
            out.push_str(&super::stmt_walk::render_and_operands(
                &op_strs, cond_comments, &" ".repeat(indent + 4), " )", true,
            ));
            out.push_str(&format!("{pad}{{\n"));
            out.push_str(&self.render_if_tail(then_body, else_body, indent));
            return out;
        }
        // Hoist multi-statement helpers from the condition expression
        let mut hoisted = Vec::new();
        let mut cnt = self.ctx.inline_counter.get();
        let new_condition =
            hoist_block_helpers(condition, self.helpers, &mut hoisted, &mut cnt, CANDLE_FNS);
        self.ctx.inline_counter.set(cnt);
        let mut out = render_hoisted_blocks(
            &hoisted, indent, self.ctx, self.enums, self.registry, self.helpers,
        );
        let cond_str = render_expr(&new_condition, self.ctx, self.registry, self.helpers);
        let cond_cs = if is_boolean_expr(&new_condition, self.helpers) {
            cond_str
        } else {
            format!("({cond_str}) != 0")
        };
        out.push_str(&format!("{pad}if( {cond_cs} ) {{\n"));
        out.push_str(&self.render_if_tail(then_body, else_body, indent));
        out
    }

    fn return_stmt(&self, value: &Option<Expr>, indent: usize) -> String {
        let pad = " ".repeat(indent);
        match value {
            Some(expr) => {
                let rendered = render_return_expr(expr, self.ctx, self.registry, self.helpers);
                format!("{pad}return {rendered} ;\n")
            }
            None => format!("{pad}return ;\n"),
        }
    }

    fn for_loop(&self, var: &str, count: &Expr, body: &[Statement], indent: usize) -> String {
        let pad = " ".repeat(indent);
        let mut out = format!(
            "{}for( {} = {}; {} > 0; {}-- ) {{\n",
            pad,
            var,
            render_expr(count, self.ctx, self.registry, self.helpers),
            var,
            var,
        );
        for s in body {
            out.push_str(&self.walk_stmt(s, indent + 3));
        }
        out.push_str(&format!("{pad}}}\n"));
        out
    }

    fn for_c(
        &self,
        init: &Statement,
        condition: &Expr,
        update: &Statement,
        body: &[Statement],
        indent: usize,
    ) -> String {
        let pad = " ".repeat(indent);
        let init_str = render_forc_part(init, self.ctx, self.enums, self.registry, self.helpers);
        let update_str = render_forc_part(update, self.ctx, self.enums, self.registry, self.helpers);
        let mut hoisted = Vec::new();
        let mut cnt = self.ctx.inline_counter.get();
        let new_condition =
            hoist_block_helpers(condition, self.helpers, &mut hoisted, &mut cnt, CANDLE_FNS);
        self.ctx.inline_counter.set(cnt);
        let mut out = render_hoisted_blocks(
            &hoisted, indent, self.ctx, self.enums, self.registry, self.helpers,
        );
        out.push_str(&format!(
            "{}for( {}; {}; {} ) {{\n",
            pad,
            init_str.trim(),
            render_expr(&new_condition, self.ctx, self.registry, self.helpers),
            update_str.trim()
        ));
        for s in body {
            out.push_str(&self.walk_stmt(s, indent + 3));
        }
        out.push_str(&format!("{pad}}}\n"));
        out
    }

    fn switch(
        &self,
        expr: &Expr,
        cases: &[(String, Vec<Statement>)],
        default: &[Statement],
        indent: usize,
    ) -> String {
        let pad = " ".repeat(indent);
        let mut hoisted = Vec::new();
        let mut cnt = self.ctx.inline_counter.get();
        let new_expr = hoist_block_helpers(expr, self.helpers, &mut hoisted, &mut cnt, CANDLE_FNS);
        self.ctx.inline_counter.set(cnt);
        let mut out = render_hoisted_blocks(
            &hoisted, indent, self.ctx, self.enums, self.registry, self.helpers,
        );
        out.push_str(&format!(
            "{}switch( {} )\n{}{{\n",
            pad,
            render_expr(&new_expr, self.ctx, self.registry, self.helpers),
            pad
        ));
        for (label, case_body) in cases {
            let cs_label = render_csharp_switch_label(label, self.enums);
            out.push_str(&format!("{pad}case {cs_label}:\n"));
            for s in case_body {
                out.push_str(&self.walk_stmt(s, indent + 3));
            }
            out.push_str(&format!("{pad}   break;\n"));
        }
        if !default.is_empty() {
            out.push_str(&format!("{pad}default:\n"));
            for s in default {
                out.push_str(&self.walk_stmt(s, indent + 3));
            }
            out.push_str(&format!("{pad}   break;\n"));
        }
        out.push_str(&format!("{pad}}}\n"));
        out
    }
}

pub(crate) fn render_statement_ctx(
    stmt: &Statement,
    indent: usize,
    ctx: &CsRenderCtx,
    enums: &HashMap<String, EnumDef>,
    registry: &Registry,
    helpers: &HelperRegistry,
) -> String {
    CsStmt { ctx, enums, registry, helpers }.walk_stmt(stmt, indent)
}

/// Enum switch case labels are QUALIFIED (`case MAType.Sma:`) — the exact
/// inverse of `render_java_switch_label`, which must strip the qualifier for
/// pre-21 JDKs. C# requires the qualified form.
fn render_csharp_switch_label(label: &str, enums: &HashMap<String, EnumDef>) -> String {
    if let Some((enum_name, variant)) = lookup_variant(label, enums) {
        format!("{enum_name}.{}", variant.pascal_name)
    } else {
        label.to_string()
    }
}

fn render_assign_target(
    expr: &Expr,
    ctx: &CsRenderCtx,
    registry: &Registry,
    helpers: &HelperRegistry,
) -> String {
    match expr {
        Expr::Var(name) => {
            if ctx.double_address_of_vars.contains(name) {
                format!("{name}[0]")
            } else {
                name.clone()
            }
        }
        Expr::ArrayAccess(name, idx) => {
            format!("{}[{}]", name, render_expr(idx, ctx, registry, helpers))
        }
        _ => render_expr(expr, ctx, registry, helpers),
    }
}

/// Render a return expression, mapping known enum values to C# constants.
fn render_return_expr(
    expr: &Expr,
    ctx: &CsRenderCtx,
    registry: &Registry,
    helpers: &HelperRegistry,
) -> String {
    if let Expr::Var(name) = expr {
        return match name.as_str() {
            "SUCCESS" => "RetCode.Success".to_string(),
            "BadParam" => "RetCode.BadParam".to_string(),
            "OutOfRangeEndIndex" => "RetCode.OutOfRangeEndIndex".to_string(),
            "OutOfRangeStartIndex" => "RetCode.OutOfRangeStartIndex".to_string(),
            _ => render_expr(expr, ctx, registry, helpers),
        };
    }
    render_expr(expr, ctx, registry, helpers)
}

/// C#-backend leaf formatting for the shared [`ExprEmitter`] tree-walk.
struct CsExpr<'a> {
    ctx: &'a CsRenderCtx<'a>,
    registry: &'a Registry,
    helpers: &'a HelperRegistry,
}

impl ExprEmitter for CsExpr<'_> {
    fn var(&self, name: &str) -> String {
        let mapped = match name {
            // C# has no compatibility field: every read is folded away by
            // `fold_compat_cond` before rendering. Reaching here means a new
            // construct escaped the fold — fail loudly rather than emit a
            // reference to a field that does not exist.
            "COMPATIBILITY" | "METASTOCK" | "DEFAULT" => panic!(
                "csharp: compatibility reference `{name}` survived the render-time fold \
                 (C# pins the mode to Default — extend compat_fold to cover this \
                 construct)"
            ),
            "BAD_PARAM" => "RetCode.BadParam".to_string(),
            "SUCCESS" => "RetCode.Success".to_string(),
            "ALLOC_ERR" => "RetCode.AllocErr".to_string(),
            "INTERNAL_ERROR" => "RetCode.InternalError".to_string(),
            _ => self.ctx.matype_map.get(name).cloned().unwrap_or_else(|| name.to_string()),
        };
        if self.ctx.double_address_of_vars.contains(name) {
            format!("{mapped}[0]")
        } else {
            mapped
        }
    }

    fn array_access(&self, name: &str, idx: &Expr) -> String {
        let access = format!("{}[{}]", name, self.walk(idx));
        // Single-precision variants take float[] inputs but compute and store
        // in double. Widen each float input element as it is read (PR #33) —
        // C# evaluates float*float in float exactly as Java does.
        if self.ctx.single_precision && self.ctx.float_input_params.contains(name) {
            format!("(double){access}")
        } else {
            access
        }
    }

    fn binop(&self, left: &Expr, op: &BinOp, right: &Expr) -> String {
        // In single-precision variants a float[]-input vs double[] identity
        // comparison can never hold (and does not compile in C#). Statement
        // positions fold whole (see `if_stmt`); a remaining expression position
        // renders the constant.
        if self.ctx.single_precision && matches!(op, BinOp::Eq | BinOp::NotEq) {
            if let (Expr::Var(lname), Expr::Var(rname)) = (left, right) {
                let left_is_input = self.ctx.float_input_params.contains(lname.as_str());
                let right_is_input = self.ctx.float_input_params.contains(rname.as_str());
                if left_is_input != right_is_input {
                    return if matches!(op, BinOp::Eq) {
                        "false".to_string()
                    } else {
                        "true".to_string()
                    };
                }
            }
        }
        // Fused multiply-add: (a * b) + c → Math.FusedMultiplyAdd(a, b, c).
        // IEEE-754 correctly-rounded, so it matches C `fma()`, Rust `mul_add`
        // and Java `Math.fma` at the same sites (site selection is the shared
        // `fma::fuse_operands`).
        if fma::EMIT_FMA {
            if let Some(fs) = self.ctx.fma {
                if let Some((a, b, c)) = fma::fuse_operands(left, op, right, &fs.view()) {
                    return format!(
                        "Math.FusedMultiplyAdd({}, {}, {})",
                        self.walk(a),
                        self.walk(b),
                        self.walk(c)
                    );
                }
            }
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
            BinOp::Eq => "==",
            BinOp::NotEq => "!=",
            BinOp::And => "&&",
            BinOp::Or => "||",
            BinOp::BitwiseOr => "|",
            BinOp::Shr => ">>",
            BinOp::Shl => "<<",
        };
        // Minimal parenthesization: C# shares C's operator precedence.
        let pp = binop_prec(op);
        let l = wrap_child(self.walk(left), left, pp, false);
        let r = wrap_child(self.walk(right), right, pp, true);
        format!("{l} {op_str} {r}")
    }

    fn cast(&self, var_type: &VarType, inner: &Expr) -> String {
        // `array_access` already widens a float input element read to double;
        // drop a redundant source-level `(double)` around such a read.
        if self.ctx.single_precision && matches!(var_type, VarType::Real) {
            if let Expr::ArrayAccess(name, _) = inner {
                if self.ctx.float_input_params.contains(name) {
                    return self.walk(inner);
                }
            }
        }
        let cs_type = cs_type_str(var_type);
        let s = self.walk(inner);
        let s = if expr_prec(inner) < 12 { format!("({s})") } else { s };
        format!("({cs_type}){s}")
    }

    fn not(&self, inner: &Expr) -> String {
        let s = self.walk(inner);
        if expr_prec(inner) < 12 {
            format!("!({s})")
        } else {
            format!("!{s}")
        }
    }

    fn func_call(&self, name: &str, args: &[Expr]) -> String {
        render_func_call(name, args, self.ctx, self.registry, self.helpers)
    }

    fn pointer_deref(&self, name: &str) -> String {
        // `*outBegIdx` in C is a plain read/write of the `out int` param here;
        // a double taken by address is a `double[1]`, so `[0]`.
        if self.ctx.double_address_of_vars.contains(name) {
            format!("{name}[0]")
        } else {
            name.to_string()
        }
    }

    fn address_of(&self, inner: &Expr) -> String {
        match inner {
            // `&realLocal` binds to a callee's output-array parameter — the
            // local is a `double[1]`, passed bare.
            Expr::Var(name) if self.ctx.double_address_of_vars.contains(name) => name.clone(),
            // `&intLocal` binds to a callee's `out int` parameter.
            Expr::Var(name) => format!("out {name}"),
            // Anything else (e.g. `&arr[i]` in a memcpy arg) is decomposed by
            // its call-site handler before rendering; render the inner
            // expression bare as a fallback, exactly as Java does.
            _ => {
                let empty = HashSet::new();
                let inner_ctx = CsRenderCtx {
                    single_precision: self.ctx.single_precision,
                    double_address_of_vars: &empty,
                    float_input_params: self.ctx.float_input_params,
                    inline_counter: self.ctx.inline_counter,
                    fma: self.ctx.fma,
                    matype_map: self.ctx.matype_map.clone(),
                };
                render_expr(inner, &inner_ctx, self.registry, self.helpers)
            }
        }
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
        // Collapse `cond ? 1 : 0` / `cond ? 0 : 1` to a bare boolean expression
        // (is_boolean_expr consults the same rule).
        match bool_ternary_collapse(then_expr, else_expr) {
            Some(BoolTernaryCollapse::Cond) => return self.walk(cond),
            Some(BoolTernaryCollapse::Negated) => return format!("!({})", self.walk(cond)),
            None => {}
        }
        let c = self.walk(cond);
        let t = self.walk(then_expr);
        let e = self.walk(else_expr);
        let c = if matches!(cond, Expr::BinOp(..) | Expr::Ternary(..)) {
            format!("({c})")
        } else {
            c
        };
        let t = if matches!(then_expr, Expr::Ternary(..)) { format!("({t})") } else { t };
        let e = if matches!(else_expr, Expr::Ternary(..)) { format!("({e})") } else { e };
        format!("{c} ? {t} : {e}")
    }
}

pub(crate) fn render_expr(
    expr: &Expr,
    ctx: &CsRenderCtx,
    registry: &Registry,
    helpers: &HelperRegistry,
) -> String {
    CsExpr { ctx, registry, helpers }.walk(expr)
}

/// Render one of the boolean near-zero builtins (IS_ZERO / IS_ZERO_SCALED /
/// IS_ZERO_OR_NEG) in C# from already-rendered argument strings. Single source
/// of the C# form for these predicates — used by both the indicator render path
/// and the `eval_predicate` server handler. The epsilon literals are
/// character-for-character the Java/C forms (they feed the bitwise gate).
pub fn csharp_predicate_expr(which: SpecialBuiltin, args: &[String]) -> String {
    match which {
        SpecialBuiltin::IsZero => args.first().map_or_else(
            || "false".to_string(),
            |x| format!("((-0.00000000000001 < {x}) && ({x} < 0.00000000000001))"),
        ),
        SpecialBuiltin::IsZeroScaled => {
            if args.len() == 2 {
                format!("(Math.Abs({}) <= 0.00000000000001 * ({}))", args[0], args[1])
            } else {
                "false".to_string()
            }
        }
        SpecialBuiltin::IsZeroOrNeg => args
            .first()
            .map_or_else(|| "false".to_string(), |x| format!("({x} < 0.00000000000001)")),
        _ => "false".to_string(),
    }
}

/// Try to render a candle helper function call as an inline C# ternary chain —
/// same shapes as the Java renderer, with `Math.Abs`.
fn try_render_candle_ternary(
    fname: &str,
    args: &[Expr],
    ctx: &CsRenderCtx,
    registry: &Registry,
    helpers: &HelperRegistry,
) -> Option<String> {
    let r = |e: &Expr| render_expr(e, ctx, registry, helpers);
    match fname {
        "ta_candlerange" if args.len() == 5 => {
            let rt = r(&args[0]);
            let open = r(&args[1]);
            let high = r(&args[2]);
            let low = r(&args[3]);
            let close = r(&args[4]);
            Some(format!(
                "(({rt} == 0) ? (Math.Abs({close} - {open})) \
                 : (({rt} == 1) ? ({high} - {low}) \
                 : (({rt} == 2) ? (({high} - {low}) - Math.Abs({close} - {open})) \
                 : 0.0)))"
            ))
        }
        "ta_candleaverage" if args.len() == 8 => {
            let rt = r(&args[0]);
            let avg_period = r(&args[1]);
            let factor = r(&args[2]);
            let sum = r(&args[3]);
            let cr_args: Vec<Expr> = std::iter::once(args[0].clone())
                .chain(args[4..8].iter().cloned())
                .collect();
            let candlerange =
                try_render_candle_ternary("ta_candlerange", &cr_args, ctx, registry, helpers)?;
            Some(format!(
                "(({factor} * ((({avg_period} != 0) \
                 ? ({sum} / {avg_period}) : {candlerange}) \
                 / (({rt} == 2) ? 2.0 : 1.0))))"
            ))
        }
        _ => None,
    }
}

/// Render a `FuncCall` expression to C# code.
#[allow(clippy::too_many_lines)]
fn render_func_call(
    fname: &str,
    args: &[Expr],
    ctx: &CsRenderCtx,
    registry: &Registry,
    helpers: &HelperRegistry,
) -> String {
    // Check if this is a call to a helper function that can be inlined
    if let Some(helper) = helpers.get(fname) {
        if let Some(inlined_expr) = try_inline_expr(helper, args) {
            let s = render_expr(&inlined_expr, ctx, registry, helpers);
            return wrap_inlined(s, &inlined_expr);
        }
    }

    // Candle helpers: render inline as ternary chains so the && split can
    // preserve short-circuit evaluation.
    if let Some(ternary) = try_render_candle_ternary(fname, args, ctx, registry, helpers) {
        return ternary;
    }

    if let Some(b) = SpecialBuiltin::from_name(fname) {
        match b {
            SpecialBuiltin::UnstablePeriod => {
                // UNSTABLE_PERIOD(RSI) -> this.unstablePeriod[(int)FuncUnstId.Rsi]
                // (C# enums cast to int; there is no ordinal()).
                if let Some(Expr::Var(func_name)) = args.first() {
                    let pascal = unst_pascal_name(func_name);
                    return format!("this.unstablePeriod[(int)FuncUnstId.{pascal}]");
                }
                "this.unstablePeriod[0]".to_string()
            }
            SpecialBuiltin::Compatibility => {
                panic!(
                    "csharp: COMPATIBILITY() survived the render-time fold (C# pins \
                     the mode to Default — extend compat_fold to cover this construct)"
                )
            }
            pred @ (SpecialBuiltin::IsZero
            | SpecialBuiltin::IsZeroScaled
            | SpecialBuiltin::IsZeroOrNeg) => {
                let rendered: Vec<String> = args
                    .iter()
                    .map(|a| render_expr(a, ctx, registry, helpers))
                    .collect();
                csharp_predicate_expr(pred, &rendered)
            }
            SpecialBuiltin::ArrayCopy => {
                // ARRAY_COPY(dst, dstOff, src, srcOff, count)
                // -> Array.Copy(src, srcOff, dst, dstOff, count) (arg reordering,
                //    matching System.arraycopy's parameter order)
                if args.len() == 5 {
                    let dst = render_expr(&args[0], ctx, registry, helpers);
                    let dst_off = render_expr(&args[1], ctx, registry, helpers);
                    let src = render_expr(&args[2], ctx, registry, helpers);
                    let src_off = render_expr(&args[3], ctx, registry, helpers);
                    let count = render_expr(&args[4], ctx, registry, helpers);
                    return format!("Array.Copy({src}, {src_off}, {dst}, {dst_off}, {count})");
                }
                "/* ARRAY_COPY: bad args */".to_string()
            }
            SpecialBuiltin::PerToK => {
                if let Some(arg) = args.first() {
                    let x = render_expr(arg, ctx, registry, helpers);
                    return format!("(2.0 / ((double)({x}) + 1.0))");
                }
                "0.0".to_string()
            }
        }
    } else if let Some(mf) = MathFn::from_name(fname) {
        // System.Math — a *name* remap, not a casing pass: `ceil` is
        // `Math.Ceiling`, and every method is PascalCase.
        let cs = match mf {
            MathFn::Atan => "Atan",
            MathFn::Sqrt => "Sqrt",
            MathFn::Floor => "Floor",
            MathFn::Ceil => "Ceiling",
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
            MathFn::Max => "Max",
            MathFn::Min => "Min",
        };
        let rendered: Vec<String> = args
            .iter()
            .map(|a| render_expr(a, ctx, registry, helpers))
            .collect();
        format!("Math.{}({})", cs, rendered.join(", "))
    } else if let Some(s) = StdlibFn::from_name(fname) {
        match s {
            StdlibFn::Sizeof => {
                // sizeof(TYPE) → 1: byte counts normalize to element counts.
                "1".to_string()
            }
            StdlibFn::Malloc => {
                // malloc(N * sizeof(TYPE)) → new TYPE[(int)(N)]
                if let Some(arg) = args.first() {
                    let cs_type = match find_sizeof_type(arg).as_deref() {
                        Some("int") => "int",
                        Some("float") => "float",
                        _ => "double",
                    };
                    let size = render_expr(arg, ctx, registry, helpers);
                    format!("new {cs_type}[(int)({size})]")
                } else {
                    "new double[0]".to_string()
                }
            }
            StdlibFn::Free => {
                // No-op (garbage collector handles deallocation)
                String::new()
            }
            StdlibFn::Memcpy | StdlibFn::Memmove => {
                // memcpy/memmove(dst, src, count) → Array.Copy(src, srcOff, dst, dstOff, count).
                // Array.Copy handles overlapping ranges like memmove.
                if args.len() >= 3 {
                    let (dst_arr, dst_off) = decompose_array_ref(&args[0], ctx, registry, helpers);
                    let (src_arr, src_off) = decompose_array_ref(&args[1], ctx, registry, helpers);
                    let count = render_expr(&args[2], ctx, registry, helpers);
                    format!("Array.Copy({src_arr}, {src_off}, {dst_arr}, {dst_off}, {count})")
                } else {
                    format!("/* {fname}: bad args */")
                }
            }
            StdlibFn::Memset => {
                // memset(buf, 0, count) → Array.Fill(buf, fillVal, off, count).
                // NOTE the third Java argument is an END INDEX (Arrays.fill's
                // `to`); Array.Fill takes a COUNT — the classic silent trap.
                if args.len() >= 3 {
                    let (arr, off) = decompose_array_ref(&args[0], ctx, registry, helpers);
                    let count = render_expr(&args[2], ctx, registry, helpers);
                    let fill_val = match find_sizeof_type(&args[2]).as_deref() {
                        Some("int") => "0",
                        _ => "0.0",
                    };
                    format!("Array.Fill({arr}, {fill_val}, (int)({off}), (int)({count}))")
                } else {
                    "/* memset: bad args */".to_string()
                }
            }
        }
    } else {
        // Use registry for cross-call resolution
        let cs_name = registry.resolve_call(fname, Lang::CSharp);
        let rendered: Vec<String> = args
            .iter()
            .map(|a| match a {
                // NULL for a nullable output the caller discards (MA passing
                // NULL for MAMA's FAMA — issue #125). The callee writes into it
                // unconditionally, so materialize a throwaway spanning the
                // output range. NULL appears only here.
                Expr::Var(n) if n == "NULL" => {
                    "new double[(int)(endIdx - startIdx + 1)]".to_string()
                }
                // The caller's own out-params pass through to the callee's
                // `out int` pair. In C both are `int*` passed bare; C# needs
                // the `out` keyword at the argument site. These two names are
                // generator-controlled on every core signature.
                Expr::Var(n) if n == "outBegIdx" || n == "outNBElement" => {
                    format!("out {n}")
                }
                _ => render_expr(a, ctx, registry, helpers),
            })
            .collect();
        format!("{}({})", cs_name, rendered.join(", "))
    }
}

/// Decompose an expression into (array_name, offset) for array copy operations.
/// `Var("arr")` → `("arr", "0")`; `AddressOf(ArrayAccess("arr", idx))` → `("arr", rendered_idx)`
fn decompose_array_ref(
    expr: &Expr,
    ctx: &CsRenderCtx,
    registry: &Registry,
    helpers: &HelperRegistry,
) -> (String, String) {
    match expr {
        Expr::AddressOf(inner) => {
            if let Expr::ArrayAccess(name, offset) = inner.as_ref() {
                let off = render_expr(offset, ctx, registry, helpers);
                (name.clone(), off)
            } else {
                let s = render_expr(expr, ctx, registry, helpers);
                (s, "0".to_string())
            }
        }
        Expr::Var(name) => (name.clone(), "0".to_string()),
        _ => {
            let s = render_expr(expr, ctx, registry, helpers);
            (s, "0".to_string())
        }
    }
}

/// Render a complex lookback body (`LookbackExpr::Code`) into C# code.
fn render_lookback_code(
    stmts: &[Statement],
    enums: &HashMap<String, EnumDef>,
    registry: &Registry,
    helpers: &HelperRegistry,
) -> String {
    let mut out = String::new();
    let inline_counter = Cell::new(0);
    let double_address_of_vars = HashSet::new();
    let float_input_params: HashSet<String> = HashSet::new();
    let ctx = CsRenderCtx {
        single_precision: false,
        double_address_of_vars: &double_address_of_vars,
        float_input_params: &float_input_params,
        inline_counter: &inline_counter,
        // Lookback bodies are pure integer index arithmetic — no float multiply-add.
        fma: None,
        matype_map: build_matype_map(enums),
    };

    // Declare local variables (initialized: locals assigned only inside a
    // switch arm would otherwise trip C# definite assignment).
    for stmt in stmts {
        if let Statement::VarDecl { var_type, name, .. } = stmt {
            let cs_decl = match var_type {
                VarType::Real => format!("double {name} = 0"),
                VarType::Integer | VarType::Index => format!("int {name} = 0"),
                VarType::RealArray(size) => format!("double[] {name} = new double[{size}]"),
                VarType::IntArray(size) => format!("int[] {name} = new int[{size}]"),
                _ => format!("{} {name}", cs_type_str(var_type)),
            };
            out.push_str(&format!("      {cs_decl};\n"));
        }
    }

    // Emit candle settings unpacking for lookback body
    let candle_used = detect_candle_settings(stmts);
    if !candle_used.is_empty() {
        out.push_str(&emit_csharp_unpacking(&candle_used, 6));
    }

    // Emit VarDecl initializations
    for stmt in stmts {
        if let Statement::VarDecl {
            name,
            init: Some(init),
            ..
        } = stmt
        {
            out.push_str(&format!(
                "      {} = {};\n",
                name,
                render_expr(init, &ctx, registry, helpers)
            ));
        }
    }

    // Render non-VarDecl statements
    for stmt in stmts {
        if matches!(stmt, Statement::VarDecl { .. }) {
            continue;
        }
        out.push_str(&render_statement_ctx(stmt, 6, &ctx, enums, registry, helpers));
    }

    out
}

#[cfg(test)]
mod tests {
    use super::*;
    use crate::parser;
    use crate::registry::Registry;
    use std::path::Path;

    fn input_base() -> std::path::PathBuf {
        Path::new(env!("CARGO_MANIFEST_DIR")).join("../../ta_codegen/input")
    }

    fn make_registry() -> Registry {
        Registry::from_dir(&input_base())
    }

    fn load(name: &str) -> FuncDef {
        let dir = input_base().join(name);
        let mut func_def = parser::yaml::parse_yaml(&dir.join(format!("{name}.yaml")));
        let parsed = parser::c_source::parse_c_source(&dir.join(format!("{name}.c")));
        parser::c_source::wire_parsed_source(&mut func_def, &parsed);
        func_def
    }

    fn load_enums() -> HashMap<String, EnumDef> {
        parser::enums::load_enums(&input_base().join("enums.yaml"))
    }

    /// The emitter's method names and the registry's cross-call resolution are
    /// derived independently; if they disagree, every cross-indicator call
    /// targets a method that does not exist (~500 compile errors, but only at
    /// `dotnet build` time). Pin the agreement for all functions here, at
    /// `cargo test` time.
    #[test]
    fn csharp_method_names_agree_with_registry() {
        let registry = make_registry();
        let mut checked = 0usize;
        for entry in std::fs::read_dir(input_base()).unwrap().filter_map(Result::ok) {
            let dir = entry.path();
            if !dir.is_dir() {
                continue;
            }
            let key = entry.file_name().to_string_lossy().to_string();
            let yaml = dir.join(format!("{key}.yaml"));
            if !yaml.exists() {
                continue;
            }
            let fd = parser::yaml::parse_yaml(&yaml);
            let base = to_csharp_method_name(&fd.name, fd.camel_case.as_deref());
            assert_eq!(
                registry.resolve_call(&key, Lang::CSharp),
                format!("{base}Unguarded"),
                "cross-call target for `{key}` disagrees with the emitted method name"
            );
            assert_eq!(
                registry.resolve_call(&format!("{key}_lookback"), Lang::CSharp),
                format!("{base}Lookback"),
                "lookback cross-call target for `{key}` disagrees with the emitted name"
            );
            checked += 1;
        }
        assert!(checked > 150, "expected to check every indicator, got {checked}");
    }

    #[test]
    fn csharp_generates_managed_core_shape() {
        let func = load("sma");
        let enums = load_enums();
        let registry = make_registry();
        let output = generate(&func, &enums, &registry, &HelperRegistry::empty());

        // Whole-file shape: namespace + partial class.
        assert!(output.contains("namespace TALib;"), "missing namespace");
        assert!(output.contains("public partial class Core"), "missing partial class");

        // Internal cores with the out-int pair and the CS0177 seeding prologue.
        assert!(output.contains("   internal RetCode Sma( "), "missing guarded core");
        assert!(output.contains("   internal RetCode SmaUnguarded( "), "missing unguarded core");
        assert!(output.contains("out int outBegIdx"), "missing out param");
        assert!(
            output.contains("      outBegIdx = 0;\n      outNBElement = 0;\n"),
            "missing seeding prologue"
        );

        // The unguarded core skips validation; the guarded one performs it.
        let logic_pos = output.find("internal RetCode SmaUnguarded( ").unwrap();
        let logic_section = &output[logic_pos..];
        let next_fn_pos = logic_section[1..]
            .find("   internal RetCode ")
            .map_or(logic_section.len(), |i| i + 1);
        assert!(
            !logic_section[..next_fn_pos].contains("OutOfRangeStartIndex"),
            "unguarded core should not contain validation"
        );
        let guarded_pos = output.find("internal RetCode Sma( ").unwrap();
        let guarded_end = output[guarded_pos..]
            .find("internal RetCode SmaUnguarded(")
            .unwrap_or(output.len() - guarded_pos);
        assert!(
            output[guarded_pos..guarded_pos + guarded_end].contains("OutOfRangeStartIndex"),
            "guarded core should contain validation"
        );

        // The public surface is OutRange-returning wrappers over those cores,
        // and nothing Java-shaped leaks through.
        assert!(output.contains("   public OutRange Sma( "), "missing public wrapper");
        assert!(output.contains("   public OutRange SmaUnguarded( "), "missing unguarded wrapper");
        assert!(
            output.contains("throw Failure(\"SMA\", retCode);"),
            "guarded wrapper must map RetCode onto the documented exception"
        );
        assert!(!output.contains("MInteger"), "MInteger must not appear in C#");
        assert!(!output.contains(".value"), "Java's .value suffix must not appear in C#");
        assert!(!output.contains("Integer.MIN_VALUE"), "sentinel must be int.MinValue");
        assert!(output.contains("int.MinValue"), "missing int.MinValue sentinel");
    }

    /// MAVP exercises the `&localBegIdx` → `out localBegIdx` lowering and the
    /// float-overload statement fold (`if (outReal == inReal)` with a float[]
    /// inReal must fold away entirely — C# raises CS0162 on `if (false)`).
    #[test]
    fn csharp_mavp_out_args_and_float_fold() {
        let func = load("mavp");
        let enums = load_enums();
        let registry = make_registry();
        let output = generate(&func, &enums, &registry, &HelperRegistry::empty());

        assert!(
            output.contains("out localBegIdx, out localNbElement"),
            "&localBegIdx must lower to an `out` argument"
        );
        assert!(
            output.contains("MovingAverageUnguarded(startIdx, endIdx, inReal, minUsed"),
            "cross-call must resolve through the registry's C# naming"
        );
        assert!(
            !output.contains("if( false )"),
            "float-overload comparison must fold at statement level (CS0162)"
        );
        // The double-precision variants keep the real aliasing test.
        assert!(
            output.contains("if( outReal == inReal ) {"),
            "double variants must keep the in-place aliasing defence"
        );
    }

    /// MA exercises qualified enum switch labels and the NULL-output
    /// materialization for the MAMA arm.
    #[test]
    fn csharp_ma_switch_is_qualified() {
        let func = load("ma");
        let enums = load_enums();
        let registry = make_registry();
        let output = generate(&func, &enums, &registry, &HelperRegistry::empty());

        assert!(output.contains("case MAType.Sma:"), "switch labels must be qualified");
        assert!(!output.contains("case Sma:"), "unqualified labels are Java-only");
        assert!(
            output.contains("new double[(int)(endIdx - startIdx + 1)]"),
            "MAMA's discarded FAMA output must materialize a throwaway array"
        );
    }
}
