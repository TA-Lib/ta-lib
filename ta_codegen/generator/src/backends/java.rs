use std::cell::Cell;
use std::collections::{HashMap, HashSet};
use std::fmt::Write as _;

use crate::candle_settings::{detect_candle_settings, emit_java_unpacking};
use crate::helper_registry::{hoist_block_helpers, try_inline_expr, HelperRegistry};
use crate::ir::{
    BinOp, CircBuf, CircBufLayout, EnumDef, Expr, FuncDef, LookbackExpr, ParamType, Statement,
    VarType,
};
use crate::parser::enums::lookup_variant;
use crate::registry::{Lang, Registry};

/// Words this backend cannot render as an identifier (see [`crate::naming`]):
/// the Java keywords (JLS 3.9), the three literals, `_` (a keyword since Java 9),
/// and `yield` — a restricted identifier that a `switch` body cannot open a
/// statement with, and this backend emits `switch`.
///
/// The other restricted identifiers (`var`, `record`, `sealed`, `permits`, and
/// the module-declaration words) are deliberately absent: they are legal
/// variable names everywhere this backend puts one.
pub(crate) const RESERVED_WORDS: &[&str] = &[
    "abstract",
    "assert",
    "boolean",
    "break",
    "byte",
    "case",
    "catch",
    "char",
    "class",
    "const",
    "continue",
    "default",
    "do",
    "double",
    "else",
    "enum",
    "extends",
    "final",
    "finally",
    "float",
    "for",
    "goto",
    "if",
    "implements",
    "import",
    "instanceof",
    "int",
    "interface",
    "long",
    "native",
    "new",
    "package",
    "private",
    "protected",
    "public",
    "return",
    "short",
    "static",
    "strictfp",
    "super",
    "switch",
    "synchronized",
    "this",
    "throw",
    "throws",
    "transient",
    "try",
    "void",
    "volatile",
    "while",
    // literals + the lone underscore + the one hazardous restricted identifier
    "false",
    "null",
    "true",
    "yield",
    "_",
];
use super::common::{contains_alloc_err_return, expr_directly_contains_candle_call, find_sizeof_type};
use super::builtins::{MathFn, SpecialBuiltin, StdlibFn};
use super::expr_walk::{binop_prec, expr_prec, is_int_bitwise, wrap_child, wrap_inlined, ExprEmitter};
use super::fma::{self, FmaVarSets};
use super::stmt_walk::StatementEmitter;

/// Candle helper function names that should be rendered inline (as ternary
/// expressions) rather than hoisted into switch-block temporaries.  Keeping
/// them as `FuncCall` nodes lets the `&&`-split optimisation preserve
/// short-circuit evaluation — hoisted switch blocks would be evaluated
/// unconditionally before the `if`.
pub(crate) const JAVA_CANDLE_FNS: &[&str] = &["ta_candlerange", "ta_candleaverage"];

// The compatibility fold (Java pins the mode to Default) lives in the shared
// [`compat_fold`](super::compat_fold) module — C# folds the identical way. It
// hangs off [`StatementEmitter::if_stmt`], which every Java render path funnels
// through — batch bodies, `LookbackExpr::Code` (CMO/RSI test the mode inside
// their lookback), and the streaming warm-up opens. Anything that reaches
// [`ExprEmitter::var`] or the `Compatibility` builtin afterwards is a construct
// the fold does not understand, and panics rather than emitting a reference to
// a field that no longer exists.
use super::compat_fold::{fold_compat_cond, CondFold};

/// Per-render state for the Java backend, mirroring `RustRenderCtx`/`CRenderCtx`.
/// Bundles the loose per-render state (precision flag, address-of variable sets,
/// float input params, and the inline-helper counter) threaded through the
/// recursive renderer. Services (enums/registry/helpers) stay as separate params.
pub(crate) struct JavaRenderCtx<'a> {
    pub(crate) single_precision: bool,
    pub(crate) address_of_vars: &'a HashSet<String>,
    pub(crate) double_address_of_vars: &'a HashSet<String>,
    pub(crate) float_input_params: &'a HashSet<String>,
    pub(crate) inline_counter: &'a Cell<usize>,
    /// FMA fusion name-sets for the body being rendered (`None` disables fusion).
    /// Populated by [`build_fma_var_sets`] so Java's `binop` emits `Math.fma(a,b,c)`
    /// at exactly the sites C/Rust fuse (cross-language bit-parity).
    pub(crate) fma: Option<&'a FmaVarSets>,
    /// Fully-qualified MAType constant (`TA_MAType_SMA`) → its Java rendering
    /// (`MAType.Sma`), derived from `enums.yaml` by [`build_matype_map`].
    /// Populated for batch/lookback bodies — the only place
    /// `optInMAType == TA_MAType_*` comparisons render; stream bodies dispatch
    /// MA-type structurally and leave this empty.
    pub(crate) matype_map: HashMap<String, String>,
}

/// Build the `TA_MAType_*` → `MAType.<Pascal>` map the [`ExprEmitter::var`] hook
/// uses for `optInMAType == TA_MAType_SMA` comparisons. Derived from the `MAType`
/// enum in `enums.yaml`, so a new `TA_MAType_X` row needs no generator edit.
#[allow(clippy::implicit_hasher)]
pub(crate) fn build_matype_map(enums: &HashMap<String, EnumDef>) -> HashMap<String, String> {
    enums
        .get("MAType")
        .map(|e| {
            e.variants
                .iter()
                .map(|v| (v.c_name.clone(), format!("MAType.{}", v.name)))
                .collect()
        })
        .unwrap_or_default()
}

/// Check if an expression renders to a boolean result in Java.
/// Used to avoid wrapping comparisons with `!= 0` (which would be a type error).
/// Must mirror what the expression renderer actually emits: `cond ? 1 : 0`
/// prettifies to the bare (boolean) condition, and a single-return helper call
/// renders as its inlined return expression.
pub(crate) fn is_boolean_expr(expr: &Expr, helpers: &HelperRegistry) -> bool {
    match expr {
        Expr::BinOp(_, op, _) => matches!(
            op,
            BinOp::Eq
                | BinOp::NotEq
                | BinOp::Less
                | BinOp::LessEq
                | BinOp::Greater
                | BinOp::GreaterEq
                | BinOp::And
                | BinOp::Or
        ),
        Expr::Not(_) => true,
        Expr::FuncCall(name, args) => {
            if matches!(
                name.as_str(),
                "IS_ZERO" | "IS_ZERO_SCALED" | "IS_ZERO_OR_NEG" | "IS_FINITE"
            ) {
                return true;
            }
            if let Some(helper) = helpers.get(name) {
                if let Some(inlined) = try_inline_expr(helper, args) {
                    return is_boolean_expr(&inlined, helpers);
                }
            }
            false
        }
        // A collapsed `? 1 : 0` / `? 0 : 1` ternary (see bool_ternary_collapse)
        // renders to the bare cond or its negation, so the result is boolean
        // only when cond itself renders boolean — an int-typed cond still needs
        // the caller's `!= 0` wrap around the collapsed expression.
        Expr::Ternary(cond, then_expr, else_expr) => {
            bool_ternary_collapse(then_expr, else_expr).is_some() && is_boolean_expr(cond, helpers)
        }
        _ => false,
    }
}

/// Check if an expression is an integer literal with a specific value.
fn is_int_literal(expr: &Expr, value: i64) -> bool {
    matches!(expr, Expr::IntLiteral(v) if *v == value)
}

/// How the managed renderers (Java, C#) collapse a boolean-shaped ternary.
pub(crate) enum BoolTernaryCollapse {
    /// `cond ? 1 : 0` renders as the bare condition.
    Cond,
    /// `cond ? 0 : 1` renders as `!(condition)`.
    Negated,
}

/// The single definition of when a `? 1 : 0` / `? 0 : 1` ternary collapses to a
/// bare boolean expression. Consulted by BOTH the `ternary()` render hook (to
/// perform the collapse) and `is_boolean_expr` (to know the collapsed result is
/// boolean-typed), so the two can never disagree about the rule. Returns `None`
/// when the ternary keeps its normal `c ? t : e` form.
pub(crate) fn bool_ternary_collapse(
    then_expr: &Expr,
    else_expr: &Expr,
) -> Option<BoolTernaryCollapse> {
    if is_int_literal(then_expr, 1) && is_int_literal(else_expr, 0) {
        Some(BoolTernaryCollapse::Cond)
    } else if is_int_literal(then_expr, 0) && is_int_literal(else_expr, 1) {
        Some(BoolTernaryCollapse::Negated)
    } else {
        None
    }
}

/// The heap array storage names for a CIRCBUF. `Plain` is a single array named `<id>`;
/// `Class` is one array per struct field named `<id>_<field>` (matching the `CIRCBUF_REF`
/// access flatten). Returns `(array_name, element_type)` pairs. Java arrays are always
/// heap-allocated via `new[]` (no stack form).
pub(crate) fn circbuf_arrays(id: &str, layout: &CircBufLayout) -> Vec<(String, VarType)> {
    match layout {
        CircBufLayout::Plain(t) => vec![(id.to_string(), t.clone())],
        CircBufLayout::Class(fields) => fields
            .iter()
            .map(|(f, t)| (format!("{id}_{f}"), t.clone()))
            .collect(),
    }
}

/// Java scalar element type for a CIRCBUF buffer (`double` / `int`).
fn java_circbuf_elem(t: &VarType) -> &'static str {
    if matches!(t, VarType::Integer) {
        "int"
    } else {
        "double"
    }
}

/// Collect all variable names used in `AddressOf(Var(name))` contexts.
/// These variables need to be declared as `MInteger` instead of `int` in Java.
pub(crate) fn collect_address_of_vars(stmts: &[Statement]) -> HashSet<String> {
    let mut vars = HashSet::new();
    collect_address_of_vars_stmts(stmts, &mut vars);
    vars
}

fn collect_address_of_vars_stmts(stmts: &[Statement], vars: &mut HashSet<String>) {
    for stmt in stmts {
        collect_address_of_vars_stmt(stmt, vars);
    }
}

fn collect_address_of_vars_stmt(stmt: &Statement, vars: &mut HashSet<String>) {
    match stmt {
        Statement::Assign { target, value, .. } => {
            scan_expr_for_address_of(target, vars);
            scan_expr_for_address_of(value, vars);
        }
        Statement::If {
            condition,
            then_body,
            else_body,
            ..
        } => {
            scan_expr_for_address_of(condition, vars);
            collect_address_of_vars_stmts(then_body, vars);
            collect_address_of_vars_stmts(else_body, vars);
        }
        Statement::While { condition, body } | Statement::DoWhile { condition, body } => {
            scan_expr_for_address_of(condition, vars);
            collect_address_of_vars_stmts(body, vars);
        }
        Statement::ForC {
            init,
            condition,
            update,
            body,
        } => {
            collect_address_of_vars_stmt(init, vars);
            scan_expr_for_address_of(condition, vars);
            collect_address_of_vars_stmt(update, vars);
            collect_address_of_vars_stmts(body, vars);
        }
        Statement::For { count, body, .. } => {
            scan_expr_for_address_of(count, vars);
            collect_address_of_vars_stmts(body, vars);
        }
        Statement::Return { value: Some(expr) } => {
            scan_expr_for_address_of(expr, vars);
        }
        Statement::Block { body } => {
            collect_address_of_vars_stmts(body, vars);
        }
        Statement::Switch {
            expr,
            cases,
            default,
        } => {
            scan_expr_for_address_of(expr, vars);
            for (_, case_body) in cases {
                collect_address_of_vars_stmts(case_body, vars);
            }
            collect_address_of_vars_stmts(default, vars);
        }
        Statement::VarDecl { init: Some(e), .. } | Statement::Expr(e) => {
            scan_expr_for_address_of(e, vars);
        }
        Statement::VarDecl { init: None, .. }
        | Statement::Return { value: None }
        | Statement::UnrollHint { .. }
        | Statement::Break
        | Statement::Continue
        | Statement::CircBuf(_)
        | Statement::Comment(_) => {}
    }
}

fn scan_expr_for_address_of(expr: &Expr, vars: &mut HashSet<String>) {
    match expr {
        Expr::AddressOf(inner) => {
            if let Expr::Var(name) = inner.as_ref() {
                vars.insert(name.clone());
            }
            scan_expr_for_address_of(inner, vars);
        }
        Expr::FuncCall(_, args) => {
            for arg in args {
                scan_expr_for_address_of(arg, vars);
            }
        }
        Expr::BinOp(l, _, r) => {
            scan_expr_for_address_of(l, vars);
            scan_expr_for_address_of(r, vars);
        }
        Expr::Not(inner)
        | Expr::BitwiseNot(inner)
        | Expr::Cast(_, inner)
        | Expr::PostIncrement(inner)
        | Expr::PostDecrement(inner)
        | Expr::PreIncrement(inner)
        | Expr::PreDecrement(inner) => {
            scan_expr_for_address_of(inner, vars);
        }
        Expr::ArrayAccess(_, idx) => {
            scan_expr_for_address_of(idx, vars);
        }
        Expr::Ternary(cond, then_expr, else_expr) => {
            scan_expr_for_address_of(cond, vars);
            scan_expr_for_address_of(then_expr, vars);
            scan_expr_for_address_of(else_expr, vars);
        }
        Expr::Literal(_)
        | Expr::IntLiteral(_)
        | Expr::Var(_)
        | Expr::PointerDeref(_) => {}
    }
}

/// Collect local int variables that are assigned from MAType enum parameters.
/// These variables must be declared as `MAType` instead of `int` in Java.
///
/// Scans the function body for `Assign { target: Var(local), value: Var(param) }`
/// where `param` is a known MAType parameter name.
pub(crate) fn collect_matype_vars(stmts: &[Statement], matype_params: &HashSet<String>) -> HashSet<String> {
    let mut vars = HashSet::new();
    if matype_params.is_empty() {
        return vars;
    }
    collect_matype_vars_stmts(stmts, matype_params, &mut vars);
    vars
}

fn collect_matype_vars_stmts(
    stmts: &[Statement],
    matype_params: &HashSet<String>,
    vars: &mut HashSet<String>,
) {
    for stmt in stmts {
        match stmt {
            Statement::Assign {
                target: Expr::Var(tname),
                value: Expr::Var(vname),
                ..
            } => {
                // If value is a known MAType param, target must be MAType
                if matype_params.contains(vname) {
                    vars.insert(tname.clone());
                }
                // If value is a known MAType local var, target must be too
                if vars.contains(vname) {
                    vars.insert(tname.clone());
                }
            }
            Statement::If {
                then_body,
                else_body,
                ..
            } => {
                collect_matype_vars_stmts(then_body, matype_params, vars);
                collect_matype_vars_stmts(else_body, matype_params, vars);
            }
            Statement::While { body, .. }
            | Statement::DoWhile { body, .. }
            | Statement::For { body, .. }
            | Statement::Block { body } => {
                collect_matype_vars_stmts(body, matype_params, vars);
            }
            Statement::ForC { init, body, .. } => {
                collect_matype_vars_stmts(&[*init.clone()], matype_params, vars);
                collect_matype_vars_stmts(body, matype_params, vars);
            }
            Statement::Switch {
                cases, default, ..
            } => {
                for (_, case_body) in cases {
                    collect_matype_vars_stmts(case_body, matype_params, vars);
                }
                collect_matype_vars_stmts(default, matype_params, vars);
            }
            _ => {}
        }
    }
}

/// Collect Real-typed variables that appear in AddressOf contexts.
/// These need `double[]` wrapping instead of MInteger wrapping in Java.
pub(crate) fn collect_double_address_of_vars(
    stmts: &[Statement],
    address_of_vars: &HashSet<String>,
) -> HashSet<String> {
    let mut double_vars = HashSet::new();
    for stmt in stmts {
        if let Statement::VarDecl {
            var_type: VarType::Real,
            name,
            ..
        } = stmt
        {
            if address_of_vars.contains(name) {
                double_vars.insert(name.clone());
            }
        }
    }
    double_vars
}

#[allow(clippy::implicit_hasher)]
pub fn generate(
    func: &FuncDef,
    enums: &HashMap<String, EnumDef>,
    registry: &Registry,
    helpers: &HelperRegistry,
) -> String {
    // Resolve `PRAGMA TA_ALT` for this language (ir::FuncDef::resolved_for).
    let resolved = func.resolved_for(crate::ir::Lang::Java);
    let func: &FuncDef = &resolved;
    let mut out = String::new();
    // File-level comments carried from the input .c (e.g. contributors/history).
    for block in &func.header_comments {
        out.push_str(&super::stmt_walk::block_comment(block, 0));
        out.push('\n');
    }
    // Name the alternate that won the batch cell, if one did.
    if let Some(m) = func.alt_marker(crate::ir::Tier::Batch, crate::ir::Lang::Java) {
        out.push_str(&format!("/* {m} */\n\n"));
    }
    out.push_str(&gen_lookback(func, enums, registry, helpers));
    if func.has_explicit_private {
        out.push_str(&gen_private(func, enums, registry, helpers)); // Private method (double)
        out.push_str(&gen_private_sp(func, enums, registry, helpers)); // Private method (float overload)
    }
    // Internal cores keep the RetCode + MInteger shape: this text is spliced
    // verbatim into BOTH the shipped Core and the JSON-RPC server's inline Core,
    // and the server calls these directly — so the harness's retCode ints and
    // output hashes are untouched by the public surface below.
    out.push_str(&gen_func(func, false, enums, registry, helpers)); // double-precision guarded
    out.push_str(&gen_func(func, true, enums, registry, helpers)); // single-precision guarded
    // Public surface: OutRange-returning wrappers over the cores above.
    out.push_str(&gen_public_wrapper(func, false, enums, registry));
    out.push_str(&gen_public_wrapper(func, true, enums, registry));
    // Streaming API section (only for YAML-declared streamable functions).
    if func.streaming {
        out.push_str(&super::java_stream::generate(func, enums, registry, helpers));
    }
    out
}

/// Java type name for a scalar or pointer `VarType`.
///
/// Array types map to their array type name here, but call sites that need a
/// size-dependent initializer (`new double[N]`) match `RealArray`/`IntArray`
/// explicitly before falling through to this helper.
pub(crate) fn java_type_str(var_type: &VarType) -> &'static str {
    match var_type {
        VarType::Real => "double",
        VarType::Integer | VarType::Index => "int",
        VarType::RetCodeType => "RetCode",
        VarType::RealPointer | VarType::RealArray(_) => "double[]",
        VarType::IntPointer | VarType::IntArray(_) => "int[]",
    }
}

/// Optional-parameter validation prologue (Java): map the Integer.MIN_VALUE /
/// `Core.REAL_DEFAULT` sentinels to the documented default value, then reject
/// out-of-range values. One source of truth for both variants: guarded
/// functions fail with `RetCode.BadParam`, lookback functions fail with `-1`
/// (the classic lookback bad-param contract).
///
/// An `enum:` param substitutes its type's `DEFAULT` member (#182) and nothing
/// else: a Java enum reference cannot hold an arbitrary int, so there is no
/// out-of-range value to reject, and `Integer.MIN_VALUE` — the spelling C, Rust
/// and C# also accept because all three surface the parameter as an integer — is
/// not a `MAType` and cannot be made into one (issue #162).
// Integer optional-param defaults/ranges are `f64` in the IR; the integer-valued
// casts to `i32` for literal emission are exact, not truncating.
#[allow(clippy::cast_possible_truncation)]
pub(crate) fn emit_opt_param_validation(
    func: &FuncDef,
    fail: &str,
    enums: &HashMap<String, EnumDef>,
) -> String {
    let mut out = String::new();
    for opt in &func.optional_inputs {
        match &opt.param_type {
            ParamType::Enum(enum_name) => {
                if let (Some(default_val), Some(def_variant)) = (
                    opt.default,
                    super::common::enum_default_variant(enums, enum_name),
                ) {
                    // A declared default always names a member, so the literal
                    // resolves; Java has no int->enum cast to fall back on.
                    let Some(val) =
                        super::common::enum_member_literal(enums, enum_name, default_val as i32)
                    else {
                        continue;
                    };
                    out.push_str(&format!(
                        "      if( {name} == {enum_name}.{member} ) {{\n         {name} = {val};\n      }}\n",
                        name = opt.name,
                        member = def_variant.name
                    ));
                }
            }
            ParamType::Integer => {
                if let Some(default_val) = opt.default {
                    out.push_str(&format!(
                        "      if( {name} == Integer.MIN_VALUE ) {{\n         {name} = {val};\n      }}",
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
                        "      if( {name} == REAL_DEFAULT ) {{\n         {name} = {val:e};\n      }}",
                        name = opt.name,
                        val = default_val
                    ));
                    // Every declared bound is checked (see backends::c).
                    if let Some((min, max)) = opt.range {
                        out.push_str(&format!(
                            " else if( {cond} ) {{\n         return {fail};\n      }}",
                            cond = super::common::real_range_reject(
                                &opt.name,
                                &super::common::real_bound_literal(min, ""),
                                &super::common::real_bound_literal(max, ""),
                                false
                            )
                        ));
                    }
                    out.push('\n');
                }
            }
            ParamType::Price(_) => {}
        }
    }
    out
}

fn gen_lookback(
    func: &FuncDef,
    enums: &HashMap<String, EnumDef>,
    registry: &Registry,
    helpers: &HelperRegistry,
) -> String {
    let name = func.name.clone();

    // Build parameter list for signature
    let param_str = if func.optional_inputs.is_empty() {
        " ".to_string()
    } else {
        let params: Vec<String> = func
            .optional_inputs
            .iter()
            .map(|opt| {
                let java_type = match &opt.param_type {
                    ParamType::Real => "double",
                    ParamType::Integer => "int",
                    ParamType::Enum(ref name) => name.as_str(),
                    ParamType::Price(_) => unreachable!("Price expanded during parsing"),
                };
                format!("{} {}", java_type, opt.name)
            })
            .collect();
        format!(" {} ", params.join(", "))
    };

    // Same param validation as the guarded function, with the lookback
    // bad-param contract: out-of-range returns -1.
    let validation = emit_opt_param_validation(func, "-1", enums);

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

    let docs = super::java_doc::lookback_docs(func, &name, enums);
    format!(
        "{docs}   public int {name}_Lookback({param_str})\n\
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

/// Name of the implementation tier: the transcribed numerics, and nothing else.
///
/// Suffixed `_Impl`, matching the streaming tiers (`_OpenImpl`,
/// `_OpenAndFillImpl`). `Internal` is deliberately NOT reused: in these two
/// backends it names a *variant* (`_OpenAndFillInternal` is the composed-open
/// fusion seam), and until #236 step 5 it named the deleted C-shaped tier, so
/// one word would carry three meanings across the history.
///
/// Not public API in any backend. It keeps the C-shaped signature because the
/// body is a literal transcription of C, which writes its indices through
/// out-parameters; what changed in #236 step 3 is only that a cross-call inside
/// it now calls the public callee and does not test a return code.
fn body_name(base: &str) -> String {
    format!("{base}_Impl")
}

/// Emit the wrapper's array-argument checks (issue #172 C2).
///
/// C cannot do this — it is handed bare pointers and has no sizes. Java arrays
/// carry their length, so an undersized output, an `endIdx` past the end of the
/// input, or a null array is detectable here, before the core writes a single
/// element. Without it each of those is an `ArrayIndexOutOfBoundsException`
/// raised from deep inside the algorithm, after the output buffer is already
/// half written and with no `OutRange` to say how far the call got.
///
/// The bound is the one the Rust backend already asserts and the cross-language
/// harness already verifies (`rust_lang::emit_bounds_asserts`): every input the
/// body indexes must reach `endIdx`, and every output must hold the values
/// actually produced — `endIdx - max(startIdx, lookback) + 1`, the produced
/// count, not the width of the requested range.
///
/// `clampedStart` is `max(startIdx, lookback)`, or `-1` when the core will reject
/// the call itself — the one case that must not be pre-empted, because the core
/// owns that diagnosis.
///
/// The `_assertStart > endIdx ||` escape in front of the Rust asserts is applied
/// to the OUTPUT bound only. A range shorter than the lookback produces no values,
/// so any output length will do — including none. The input bound does NOT take
/// the escape: `endIdx` past the end of the series the caller supplied is a caller
/// bug in every range, and the only reason C answers it with `TA_SUCCESS` is that
/// it has no size to check against. Reporting it beats an empty `OutRange` that
/// reads as "no data yet".
///
/// This is the one bound where Java checks more than C and Rust do. It is not
/// load-bearing for memory safety — `NoPhantomIoTest` pins that no core reads
/// anything on a sub-lookback range — it is a diagnostic.
///
/// A null array is rejected either way — the length check is conditional, the
/// contract that an argument exists is not.
///
/// **Order.** `requireIndexRange` comes first, then the presence of any non-buffer
/// argument, then the buffer checks: the specification evaluates B-1/B-2 before
/// B-3, and this wrapper used to run the presence check ahead of both, so an
/// absent buffer pre-empted an out-of-range index (Part 3 item 3). The null enum
/// check (item 4) has to sit ahead of the `_Lookback` call below, because that is
/// where a null one is first dereferenced.
fn gen_argument_checks(func: &FuncDef, base_name: &str) -> String {
    let indexed = super::common::indexed_input_names(func);
    let inputs: Vec<&str> = func
        .inputs
        .iter()
        .filter(|i| indexed.contains(&i.name))
        .map(|i| i.name.as_str())
        .collect();
    let mut out = String::new();
    let _ = writeln!(
        out,
        "      requireIndexRange(\"{base_name}\", startIdx, endIdx);"
    );
    for opt in &func.optional_inputs {
        if matches!(opt.param_type, ParamType::Enum(_)) {
            let _ = writeln!(
                out,
                "      requireArgument(\"{base_name}\", \"{0}\", {0});",
                opt.name
            );
        }
    }
    if inputs.is_empty() && func.outputs.is_empty() {
        return out;
    }
    let lb_args: Vec<String> = func.optional_inputs.iter().map(|o| o.name.clone()).collect();
    let _ = writeln!(
        out,
        "      int guardStart = clampedStart(startIdx, endIdx, {base_name}_Lookback({}));",
        lb_args.join(", ")
    );
    if !inputs.is_empty() {
        out.push_str("      int guardInLen = guardStart < 0 ? 0 : endIdx + 1;\n");
    }
    if !func.outputs.is_empty() {
        out.push_str(
            "      int guardOutLen = guardStart < 0 || guardStart > endIdx ? 0 : endIdx - guardStart + 1;\n",
        );
    }
    for name in inputs {
        let _ = writeln!(
            out,
            "      requireLength(\"{base_name}\", \"{name}\", {name}, guardInLen);"
        );
    }
    for output in &func.outputs {
        let name = &output.name;
        let _ = writeln!(
            out,
            "      requireLength(\"{base_name}\", \"{name}\", {name}, guardOutLen);"
        );
    }
    out
}

/// Emit the public, `OutRange`-returning wrapper over one internal core.
///
/// The wrapper translates the core's `RetCode` into the documented exception
/// mapping. It is thin: the numerics live entirely in the core.
///
/// **A short range is not an error.** A valid range shorter than the lookback
/// returns `Success` with `outNBElement == 0`, which becomes an `OutRange` whose
/// `count` is 0 — exactly C's contract, never an exception.
fn gen_public_wrapper(
    func: &FuncDef,
    single_precision: bool,
    enums: &HashMap<String, EnumDef>,
    registry: &Registry,
) -> String {
    let base_name = func.name.clone();
    let core = body_name(&base_name);
    let public_name = base_name.clone();

    // Parameters: same as the core minus the two MInteger out-params.
    let mut params: Vec<String> = vec!["int startIdx".to_string(), "int endIdx".to_string()];
    let mut args: Vec<String> = vec!["startIdx".to_string(), "endIdx".to_string()];
    for input in &func.inputs {
        let java_type = match (&input.param_type, single_precision) {
            (ParamType::Real, true) => "float",
            (ParamType::Real, false) => "double",
            _ => "int",
        };
        params.push(format!("{} {}[]", java_type, input.name));
        args.push(input.name.clone());
    }
    for opt in &func.optional_inputs {
        let java_type = match &opt.param_type {
            ParamType::Real => "double",
            ParamType::Integer => "int",
            ParamType::Enum(ref name) => name.as_str(),
            ParamType::Price(_) => unreachable!("Price expanded during parsing"),
        };
        params.push(format!("{} {}", java_type, opt.name));
        args.push(opt.name.clone());
    }
    args.push("outBegIdx".to_string());
    args.push("outNBElement".to_string());
    for output in &func.outputs {
        let java_type = match &output.param_type {
            ParamType::Real => "double",
            _ => "int",
        };
        params.push(format!("{} {}[]", java_type, output.name));
        args.push(output.name.clone());
    }

    let mut out = String::new();
    out.push_str(&super::java_doc::guarded_docs(
        func, &base_name, single_precision, enums, registry,
    ));
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
    out.push_str(&gen_argument_checks(func, &base_name));
    out.push_str("      MInteger outBegIdx = new MInteger();\n");
    out.push_str("      MInteger outNBElement = new MInteger();\n");
    {
        let _ = write!(out, "      RetCode retCode = {core}(");
        out.push_str(&args.join(", "));
        out.push_str(");\n");
        out.push_str("      if( retCode != RetCode.Success ) {\n");
        let _ = writeln!(out, "         throw failure(\"{}\", retCode);", func.name);
        out.push_str("      }\n");
    }
    out.push_str("      return new OutRange(outBegIdx.value, outNBElement.value);\n");
    out.push_str("   }\n");

    out
}

/// Generate the Private method (double, extra params).
fn gen_private(
    func: &FuncDef,
    enums: &HashMap<String, EnumDef>,
    registry: &Registry,
    helpers: &HelperRegistry,
) -> String {
    let base_name = func.name.clone();
    let name_override = format!("{base_name}_Private");
    gen_func_inner(func, false, Some(&name_override), enums, registry, helpers)
}

/// Generate the Private method float overload (for Java method overloading).
/// Java needs this because float[] is not assignable to double[] — an S_ caller
/// of `<N>_Private(float_input, k)` needs a float overload. No shipped indicator
/// declares a `_private`; the construct is carried by the SYNTH4 gate fixture
/// (`input_synth/README.md`).
fn gen_private_sp(
    func: &FuncDef,
    enums: &HashMap<String, EnumDef>,
    registry: &Registry,
    helpers: &HelperRegistry,
) -> String {
    let base_name = func.name.clone();
    let name_override = format!("{base_name}_Private");
    gen_func_inner(func, true, Some(&name_override), enums, registry, helpers)
}

fn gen_func(
    func: &FuncDef,
    single_precision: bool,
    enums: &HashMap<String, EnumDef>,
    registry: &Registry,
    helpers: &HelperRegistry,
) -> String {
    gen_func_inner(func, single_precision, None, enums, registry, helpers)
}

/// `name_override` distinguishes the two things this emits: `Some(..)` is the
/// `Private` variant (no validation prologue, extra private params), `None` is the
/// guarded internal core.
#[allow(clippy::too_many_lines, clippy::cognitive_complexity)]
fn gen_func_inner(
    func: &FuncDef,
    single_precision: bool,
    name_override: Option<&str>,
    enums: &HashMap<String, EnumDef>,
    registry: &Registry,
    helpers: &HelperRegistry,
) -> String {
    let mut out = String::new();
    let base_name = func.name.clone();
    let name = if let Some(n) = name_override {
        n.to_string()
    } else {
        body_name(&base_name)
    };

    // Build parameter list
    let mut params: Vec<String> = Vec::new();
    params.push("int startIdx".to_string());
    params.push("int endIdx".to_string());

    for input in &func.inputs {
        let java_type = if single_precision {
            match &input.param_type {
                ParamType::Real => "float",
                ParamType::Integer | ParamType::Enum(_) | ParamType::Price(_) => "int",
            }
        } else {
            match &input.param_type {
                ParamType::Real => "double",
                ParamType::Integer | ParamType::Enum(_) | ParamType::Price(_) => "int",
            }
        };
        params.push(format!("{} {}[]", java_type, input.name));
    }

    for opt in &func.optional_inputs {
        let java_type = match &opt.param_type {
            ParamType::Real => "double",
            ParamType::Integer => "int",
            ParamType::Enum(ref name) => name.as_str(),
            ParamType::Price(_) => unreachable!("Price expanded during parsing"),
        };
        params.push(format!("{} {}", java_type, opt.name));
    }

    // Extra params only on Private variant (via name_override)
    if name_override.is_some() {
        for (param_name, c_type) in &func.private_extra_params {
            let java_type = match c_type.as_str() {
                "int" => "int",
                _ => "double",
            };
            params.push(format!("{java_type} {param_name}"));
        }
    }

    params.push("MInteger outBegIdx".to_string());
    params.push("MInteger outNBElement".to_string());

    for output in &func.outputs {
        let java_type = match &output.param_type {
            ParamType::Real => "double",
            ParamType::Integer | ParamType::Enum(_) | ParamType::Price(_) => "int",
        };
        params.push(format!("{} {}[]", java_type, output.name));
    }

    // Format signature. Package-private: these are the internal cores the public
    // OutRange wrappers (and the JSON-RPC server) delegate to, not the API.
    let sig_prefix = format!("   RetCode {name}( ");
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

    // Body selection (same pattern as C backend):
    // - Private variant (name_override): always private_body
    // - S_ variants with _private: inline private_body
    // - Double variants with _private: body (delegates to Private)
    // - Logic without _private: private_body (same content as body)
    // - Guarded without _private: body
    let body = if name_override.is_some() || (single_precision && func.has_explicit_private) {
        // Private variant, or S_ variant inlining the private body
        &func.private_body
    } else {
        &func.body
    };

    // Carry source comments only in the double-precision implementation (guarded
    // `xxx` and, for explicit-private functions, `xxxPrivate`). Strip them from the
    // single-precision copy.
    let keep_comments = !single_precision;
    let body_stripped;
    let body: &[Statement] = if keep_comments {
        body
    } else {
        body_stripped = super::stmt_walk::strip_comments(body);
        &body_stripped
    };

    // Pre-scan for variables used in AddressOf contexts (need MInteger wrapping)
    let mut address_of_vars = collect_address_of_vars(body);

    // In single-precision variants, input params are float[] while outputs are double[].
    // Collect input param names so render_expr can replace float[]==double[] with false.
    let float_input_params: HashSet<String> = if single_precision {
        func.inputs.iter().map(|p| p.name.clone()).collect()
    } else {
        HashSet::new()
    };

    // Pre-scan for local int variables that are assigned from MAType enum params.
    // In C, MAType temporaries are plain ints (parsed as VarType::Integer),
    // but in Java the variable must be declared as `MAType` to allow enum assignment.
    let matype_params: HashSet<String> = func
        .optional_inputs
        .iter()
        .filter(|o| matches!(&o.param_type, ParamType::Enum(n) if n == "MAType"))
        .map(|o| o.name.clone())
        .collect();
    let matype_vars = collect_matype_vars(body, &matype_params);

    // Collect Real-typed variables used in AddressOf contexts.
    // These need `double[]` wrapping (not MInteger) — e.g. `double prevATR`
    // becomes `double[] prevATR = new double[1]` and uses `[0]` instead of `.value`.
    let double_address_of_vars = collect_double_address_of_vars(body, &address_of_vars);

    // Remove double address-of vars from the integer set so they don't get `.value`
    for name in &double_address_of_vars {
        address_of_vars.remove(name);
    }

    // Declare local variables
    for stmt in body {
        // A CIRCBUF prolog declares heap arrays (+ index/bound) at the function top.
        // maxIdx is seeded here (static_size-1) so INIT_LOCAL_ONLY (HT) has a valid
        // bound and Java definite-assignment is satisfied.
        if let Statement::CircBuf(CircBuf::Prolog {
            id,
            layout,
            static_size,
        }) = stmt
        {
            for (arr, t) in circbuf_arrays(id, layout) {
                out.push_str(&format!("      {}[] {arr};\n", java_circbuf_elem(&t)));
            }
            out.push_str(&format!("      int {id}_Idx = 0;\n"));
            out.push_str(&format!("      int maxIdx_{id} = ({static_size})-1;\n"));
            continue;
        }
        if let Statement::VarDecl { var_type, name, .. } = stmt {
            let java_decl = if matype_vars.contains(name) {
                format!("MAType {name}")
            } else if address_of_vars.contains(name)
                && matches!(var_type, VarType::Integer | VarType::Index)
            {
                format!("MInteger {name} = new MInteger()")
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
                    _ => format!("{} {name}", java_type_str(var_type)),
                }
            };
            out.push_str(&format!("      {java_decl};\n"));
        }
    }

    // For S_ variants with _private: the extra params (e.g. EMA's k factor) the
    // inlined private body needs. DECLARED here, ASSIGNED after the validation
    // prologue — the initialiser reads an optional parameter, and the prologue is
    // what substitutes a sentinel for the declared default.
    let sp_private_init = single_precision && func.has_explicit_private && name_override.is_none();
    if sp_private_init {
        for (param_name, _) in &func.private_param_init {
            out.push_str(&format!("      double {param_name} = 0.0;\n"));
        }
    }

    // Emit candle settings unpacking (only for referenced settings)
    let candle_used = detect_candle_settings(body);
    if !candle_used.is_empty() {
        out.push_str(&emit_java_unpacking(&candle_used, 6));
    }

    // Validation prologue. Omitted for the `Private` variant, whose callers are the
    // guarded cores that have already validated.
    if name_override.is_none() {
        out.push_str("      if( (startIdx < 0) || (startIdx > MAX_INDEX) ) {\n");
        out.push_str("         return RetCode.OutOfRangeStartIndex ;\n");
        out.push_str("      }\n");
        out.push_str("      if( (endIdx < 0) || (endIdx > MAX_INDEX) || (endIdx < startIdx)) {\n");
        out.push_str("         return RetCode.OutOfRangeEndIndex ;\n");
        out.push_str("      }\n");
        // Optional parameter validation (default + range)
        out.push_str(&emit_opt_param_validation(func, "RetCode.BadParam", enums));
        // Output-distinctness (issue #108): aliasing two different output arrays
        // has no correct result, so reject it. Input == output stays allowed.
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

    // Any sentinel is substituted by now — derive the private extra params.
    if sp_private_init {
        for (param_name, init_expr) in &func.private_param_init {
            let init_java = render_init_expr(init_expr);
            out.push_str(&format!("      {param_name} = {init_java};\n"));
        }
    }

    let inline_counter = Cell::new(0);
    // FMA fusion sites for this body — same detector Rust/C use, so the three
    // backends fuse identical sites. The index-param seeds never affect a fusion
    // decision (never float operands), so one seed set is used uniformly.
    let fma_sets = fma::build_fma_var_sets(body, &func.outputs, &fma::INDEX_PARAM_SEEDS);
    let ctx = JavaRenderCtx {
        single_precision,
        address_of_vars: &address_of_vars,
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
            // Hoist multi-statement helpers from init expressions
            let mut hoisted_vec = Vec::new();
            let mut cnt = ctx.inline_counter.get();
            let new_init = hoist_block_helpers(
                init, helpers, &mut hoisted_vec, &mut cnt, JAVA_CANDLE_FNS,
            );
            ctx.inline_counter.set(cnt);
            out.push_str(&render_hoisted_blocks(
                &hoisted_vec, 6, &ctx, enums, registry, helpers,
            ));
            let init_str = render_expr(&new_init, &ctx, registry, helpers);
            if address_of_vars.contains(name) {
                out.push_str(&format!("      {name}.value = {init_str};\n"));
            } else if double_address_of_vars.contains(name) {
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
    ctx: &JavaRenderCtx,
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

/// Render hoisted block-inline helpers as Java code (temp var decl + body).
pub(crate) fn render_hoisted_blocks(
    hoisted: &[(String, VarType, Vec<Statement>)],
    indent: usize,
    ctx: &JavaRenderCtx,
    enums: &HashMap<String, EnumDef>,
    registry: &Registry,
    helpers: &HelperRegistry,
) -> String {
    let pad = " ".repeat(indent);
    let mut out = String::new();
    for (temp_name, var_type, body) in hoisted {
        let java_decl = match var_type {
            VarType::RealArray(size) => format!("double[] {temp_name} = new double[{size}]"),
            VarType::IntArray(size) => format!("int[] {temp_name} = new int[{size}]"),
            _ => format!("{} {temp_name}", java_type_str(var_type)),
        };
        out.push_str(&format!("{pad}{java_decl};\n"));
        // Declare local variables from the hoisted helper body.
        // render_statement skips VarDecl, so we emit them explicitly here.
        // For VarDecls with an initializer, emit `type name = <init>;` directly.
        for stmt in body {
            if let Statement::VarDecl { var_type: vt, name, init } = stmt {
                let type_part = match vt {
                    VarType::RealArray(size) => {
                        // Arrays with size are initialized inline; emit and continue
                        out.push_str(&format!("{pad}double[] {name} = new double[{size}];\n"));
                        continue;
                    }
                    VarType::IntArray(size) => {
                        out.push_str(&format!("{pad}int[] {name} = new int[{size}];\n"));
                        continue;
                    }
                    _ => java_type_str(vt).to_string(),
                };
                if let Some(init_expr) = init {
                    // Hoist any multi-statement helpers in the init expression
                    // (e.g. ta_candlerange inside ta_candleaverage's VarDecl init)
                    let mut inner_hoisted = Vec::new();
                    let mut cnt = ctx.inline_counter.get();
                    let hoisted_init = hoist_block_helpers(
                        init_expr, helpers, &mut inner_hoisted, &mut cnt, JAVA_CANDLE_FNS,
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
            // Skip VarDecls — already emitted in the declaration loop above
            if matches!(stmt, Statement::VarDecl { .. }) {
                continue;
            }
            out.push_str(&render_statement_ctx(stmt, indent, ctx, enums, registry, helpers));
        }
    }
    out
}

#[allow(clippy::implicit_hasher)]
pub fn render_statement(
    stmt: &Statement,
    indent: usize,
    single_precision: bool,
    enums: &HashMap<String, EnumDef>,
    registry: &Registry,
    helpers: &HelperRegistry,
    inline_counter: &Cell<usize>,
    address_of_vars: &HashSet<String>,
    double_address_of_vars: &HashSet<String>,
    float_input_params: &HashSet<String>,
) -> String {
    let ctx = JavaRenderCtx {
        single_precision,
        address_of_vars,
        double_address_of_vars,
        float_input_params,
        inline_counter,
        // Auxiliary entry (no body available to derive fusion sets); fusion for
        // the shipped indicator bodies flows through gen_func_inner's context.
        fma: None,
        matype_map: build_matype_map(enums),
    };
    render_statement_ctx(stmt, indent, &ctx, enums, registry, helpers)
}

/// Java-backend leaf formatting for the shared [`StatementEmitter`] tree-walk.
/// Bundles the render context with the enum/registry/helper services the hooks
/// need; the recursion and variant dispatch live in [`StatementEmitter::walk_stmt`].
struct JavaStmt<'a> {
    ctx: &'a JavaRenderCtx<'a>,
    enums: &'a HashMap<String, EnumDef>,
    registry: &'a Registry,
    helpers: &'a HelperRegistry,
}

impl JavaStmt<'_> {
    /// Shared `if` tail (then-body + else branch with `} else if` collapse) used
    /// by both the flat and multi-line-condition rendering paths.
    fn render_if_tail(&self, then_body: &[Statement], else_body: &[Statement], indent: usize) -> String {
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
                // unbraced after `else` — but the compat fold can dissolve
                // that inner `if` into bare statements or nothing, and
                // `} else <bare>` either lets statements escape the else or
                // dangles onto the next sibling. Collapse only when the walk
                // still starts with an `if(`; otherwise fall through to the
                // braced form. (Latent today — every compat site is a
                // top-level if — but the C# float fold proved the mechanism.)
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

impl StatementEmitter for JavaStmt<'_> {
    fn comment(&self, lines: &[String], indent: usize) -> String {
        super::stmt_walk::block_comment(lines, indent)
    }

    fn circ_buf(&self, op: &CircBuf, indent: usize) -> String {
        let pad = " ".repeat(indent);
        match op {
            // Prolog: arrays + index/bound declared at the function top by the decl pass.
            // Destroy: Java arrays are GC-managed — no explicit free.
            CircBuf::Prolog { .. } | CircBuf::Destroy { .. } => String::new(),
            // Advance with conditional reset (not modulo) — matches the reference macro.
            CircBuf::Next { id } => {
                format!("{pad}{id}_Idx++;\n{pad}if( {id}_Idx > maxIdx_{id} ) {{ {id}_Idx = 0; }}\n")
            }
            // Runtime-sized: allocate each array to `size` (Java zero-fills new arrays).
            CircBuf::Init { id, layout, size } => {
                let sz = render_expr(size, self.ctx, self.registry, self.helpers);
                let mut s = String::new();
                // The size is derived, so < 1 is a logic defect rather than an allocation
                // failure: same code as C's TA_INTERNAL_ERROR(137) (#178).
                s.push_str(&format!("{pad}if( {sz} < 1 ) return RetCode.InternalError;\n"));
                for (arr, t) in circbuf_arrays(id, layout) {
                    s.push_str(&format!(
                        "{pad}{arr} = new {}[{sz}];\n",
                        java_circbuf_elem(&t)
                    ));
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
                        java_circbuf_elem(&t)
                    ));
                }
                s
            }
        }
    }

    #[allow(clippy::too_many_lines, clippy::cognitive_complexity)]
    fn var_decl(&self, var_type: &VarType, name: &str, init: &Option<Expr>, indent: usize) -> String {
        let pad = " ".repeat(indent);
        // Top-level VarDecls are emitted by the function renderer and skipped
        // before calling render_statement. This arm handles block-scoped VarDecls
        // (inside while/for/if bodies) that need local declarations.
        let type_str = match var_type {
            VarType::RealArray(size) => {
                return format!(
                    "{pad}double[] {name} = new double[{size}];\n"
                );
            }
            VarType::IntArray(size) => {
                return format!("{pad}int[] {name} = new int[{size}];\n");
            }
            _ => java_type_str(var_type),
        };
        if let Some(init_expr) = init {
            let mut hoisted_vec = Vec::new();
            let mut cnt = self.ctx.inline_counter.get();
            let new_init = hoist_block_helpers(
                init_expr, self.helpers, &mut hoisted_vec, &mut cnt, JAVA_CANDLE_FNS,
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

    #[allow(clippy::too_many_lines, clippy::cognitive_complexity)]
    fn assign(&self, target: &Expr, value: &Expr, compound: bool, indent: usize) -> String {
        let pad = " ".repeat(indent);
        // A cross-indicator call answers an `OutRange` and throws (#236 step 3),
        // so the assigned code is Success by construction.
        if let Expr::FuncCall(fname, cargs) = value {
            if self.registry.contains(fname) {
                if let Some(block) =
                    render_cross_indicator_call(fname, cargs, indent, self.ctx, self.registry, self.helpers)
                {
                    let t = render_assign_target(target, self.ctx, self.registry, self.helpers);
                    return format!("{block}{pad}{t} = RetCode.Success;\n");
                }
            }
        }
        // Handle output scalar assignments via .value
        if let Expr::Var(name) = target {
            if name == "outBegIdx" || name == "outNBElement" {
                return format!(
                    "{}{}.value = {};\n",
                    pad,
                    name,
                    render_expr(value, self.ctx, self.registry, self.helpers)
                );
            }
        }

        // Hoist multi-statement helpers from the value expression
        let mut hoisted = Vec::new();
        let mut cnt = self.ctx.inline_counter.get();
        let new_value = hoist_block_helpers(
            value, self.helpers, &mut hoisted, &mut cnt, JAVA_CANDLE_FNS,
        );
        // Canonicalize accumulator recurrences so all backends fuse the same
        // product regardless of operand order (cross-language / batch-vs-stream).
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
                            BinOp::BitwiseAnd => "&=",
                            BinOp::BitwiseOr => "|=",
                            BinOp::BitwiseXor => "^=",
                            BinOp::Shl => "<<=",
                            BinOp::Shr => ">>=",
                            BinOp::Mod
                            | BinOp::LessEq
                            | BinOp::Less
                            | BinOp::Greater
                            | BinOp::GreaterEq
                            | BinOp::Eq
                            | BinOp::NotEq
                            | BinOp::And
                            | BinOp::Or => "",
                        };
                        if !op_str.is_empty() {
                            let target_str = render_assign_target(target, self.ctx, self.registry, self.helpers);
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
        // Statement-level expression: render a bare call/macro for its side effects.
        // Skip bare variable statements (no side effects — e.g. inlined identity helpers)
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
        // Hoist multi-statement helpers from the condition expression
        let mut hoisted = Vec::new();
        let mut cnt = self.ctx.inline_counter.get();
        let new_condition = hoist_block_helpers(
            condition, self.helpers, &mut hoisted, &mut cnt, JAVA_CANDLE_FNS,
        );
        self.ctx.inline_counter.set(cnt);
        let mut out = render_hoisted_blocks(
            &hoisted, indent, self.ctx, self.enums, self.registry, self.helpers,
        );
        let cond_str = render_expr(&new_condition, self.ctx, self.registry, self.helpers);
        let cond_java = if is_boolean_expr(&new_condition, self.helpers) {
            cond_str
        } else {
            format!("({cond_str}) != 0")
        };
        out.push_str(&format!("{pad}while( {cond_java} ) {{\n"));
        for s in body {
            out.push_str(&self.walk_stmt(s, indent + 3));
        }
        out.push_str(&format!("{pad}}}\n"));
        out
    }

    fn do_while(&self, condition: &Expr, body: &[Statement], indent: usize) -> String {
        let pad = " ".repeat(indent);
        // Hoist multi-statement helpers from the condition expression.
        // For do-while, hoisted blocks go INSIDE the loop body (before the
        // closing `} while(cond)`) so they execute each iteration.
        let mut hoisted = Vec::new();
        let mut cnt = self.ctx.inline_counter.get();
        let new_condition = hoist_block_helpers(
            condition, self.helpers, &mut hoisted, &mut cnt, JAVA_CANDLE_FNS,
        );
        self.ctx.inline_counter.set(cnt);
        let mut out = format!("{pad}do {{\n");
        for s in body {
            out.push_str(&self.walk_stmt(s, indent + 3));
        }
        out.push_str(&render_hoisted_blocks(
            &hoisted, indent + 3, self.ctx, self.enums, self.registry, self.helpers,
        ));
        let cond_str = render_expr(&new_condition, self.ctx, self.registry, self.helpers);
        let cond_java = if is_boolean_expr(&new_condition, self.helpers) {
            cond_str
        } else {
            format!("({cond_str}) != 0")
        };
        out.push_str(&format!("{pad}}} while( {cond_java} );\n"));
        out
    }

    #[allow(clippy::too_many_lines, clippy::cognitive_complexity)]
    fn if_stmt(&self, condition: &Expr, then_body: &[Statement], else_body: &[Statement], cond_comments: &[Option<Vec<String>>], indent: usize) -> String {
        let pad = " ".repeat(indent);
        // Skip post-allocation null-check blocks (dead code in Java — `new` never returns null)
        if contains_alloc_err_return(then_body) {
            return String::new();
        }
        // Compatibility is pinned to Default in Java: fold the branch away and
        // splice the surviving arm in place (see `fold_compat_cond`). The dropped
        // arm's statements are the only thing removed — the survivor renders at
        // this `if`'s own indent, since its block is dissolved.
        match fold_compat_cond(condition) {
            CondFold::Known(taken) => {
                let kept = if taken { then_body } else { else_body };
                return kept.iter().map(|s| self.walk_stmt(s, indent)).collect();
            }
            CondFold::Open { expr, changed: true } => {
                // A compound condition that lost a compatibility operand (e.g.
                // `unstablePeriod == 0 && COMPATIBILITY() == METASTOCK`) re-renders
                // through the normal path with the survivor alone. The per-operand
                // comments no longer line up with the shortened `&&`-chain, so they
                // are dropped rather than mis-attached.
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
        // Split `if(A && B)` into nested `if(A) { if(B)` when both sides
        // contain a candle helper call (ta_candlerange/ta_candleaverage).
        // This preserves short-circuit evaluation so the expensive ternary
        // on the right side is only computed when the left side is true.
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
        // Inline per-operand comments: render the `&&`-chain multi-line (same
        // tokens as the flat form, plus the comments).
        if !cond_comments.is_empty()
            && super::stmt_walk::flatten_and(condition).len() == cond_comments.len()
        {
            let mut hoisted = Vec::new();
            let mut cnt = self.ctx.inline_counter.get();
            let new_condition = hoist_block_helpers(
                condition, self.helpers, &mut hoisted, &mut cnt, JAVA_CANDLE_FNS,
            );
            self.ctx.inline_counter.set(cnt);
            let op_strs: Vec<String> = super::stmt_walk::flatten_and(&new_condition)
                .iter()
                .map(|o| {
                    let s = render_expr(o, self.ctx, self.registry, self.helpers);
                    if is_boolean_expr(o, self.helpers) {
                        // Re-joined with `&&`, so wrap an operand that binds
                        // looser than `&&` (an `||` chain or ternary).
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
        let new_condition = hoist_block_helpers(
            condition, self.helpers, &mut hoisted, &mut cnt, JAVA_CANDLE_FNS,
        );
        self.ctx.inline_counter.set(cnt);
        let mut out = render_hoisted_blocks(
            &hoisted, indent, self.ctx, self.enums, self.registry, self.helpers,
        );
        let cond_str = render_expr(&new_condition, self.ctx, self.registry, self.helpers);
        let cond_java = if is_boolean_expr(&new_condition, self.helpers) {
            cond_str
        } else {
            format!("({cond_str}) != 0")
        };
        out.push_str(&format!("{pad}if( {cond_java} ) {{\n"));
        out.push_str(&self.render_if_tail(then_body, else_body, indent));
        out
    }

    fn return_stmt(&self, value: &Option<Expr>, indent: usize) -> String {
        let pad = " ".repeat(indent);
        // `return macd(...)` -- the tail-call form of a cross-indicator call.
        if let Some(Expr::FuncCall(fname, cargs)) = value {
            if self.registry.contains(fname) {
                if let Some(block) =
                    render_cross_indicator_call(fname, cargs, indent, self.ctx, self.registry, self.helpers)
                {
                    return format!("{block}{pad}return RetCode.Success ;\n");
                }
            }
        }
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

    fn for_c(&self, init: &Statement, condition: &Expr, update: &Statement, body: &[Statement], indent: usize) -> String {
        let pad = " ".repeat(indent);
        let init_str = render_forc_part(init, self.ctx, self.enums, self.registry, self.helpers);
        let update_str = render_forc_part(update, self.ctx, self.enums, self.registry, self.helpers);
        // Hoist multi-statement helpers from the condition expression
        let mut hoisted = Vec::new();
        let mut cnt = self.ctx.inline_counter.get();
        let new_condition = hoist_block_helpers(
            condition, self.helpers, &mut hoisted, &mut cnt, JAVA_CANDLE_FNS,
        );
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

    #[allow(clippy::too_many_lines, clippy::cognitive_complexity)]
    fn switch(&self, expr: &Expr, cases: &[(String, Vec<Statement>)], default: &[Statement], indent: usize) -> String {
        let pad = " ".repeat(indent);
        // Hoist multi-statement helpers from the switch expression
        let mut hoisted = Vec::new();
        let mut cnt = self.ctx.inline_counter.get();
        let new_expr = hoist_block_helpers(
            expr, self.helpers, &mut hoisted, &mut cnt, JAVA_CANDLE_FNS,
        );
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
            let java_label = render_java_switch_label(label, self.enums);
            out.push_str(&format!("{pad}case {java_label}:\n"));
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
    ctx: &JavaRenderCtx,
    enums: &HashMap<String, EnumDef>,
    registry: &Registry,
    helpers: &HelperRegistry,
) -> String {
    JavaStmt { ctx, enums, registry, helpers }.walk_stmt(stmt, indent)
}

pub(crate) fn render_java_switch_label(label: &str, enums: &HashMap<String, EnumDef>) -> String {
    if let Some((_enum_name, variant)) = lookup_variant(label, enums) {
        // Enum switch case labels must be UNQUALIFIED ("case Sma:", not
        // "case MAType.Sma:"): qualified enum case labels are Java 21+
        // syntax, and the shipped Core.java must keep compiling on older
        // JDKs (the reference Java also emitted unqualified labels).
        variant.name.clone()
    } else {
        label.to_string()
    }
}

fn render_assign_target(
    expr: &Expr,
    ctx: &JavaRenderCtx,
    registry: &Registry,
    helpers: &HelperRegistry,
) -> String {
    match expr {
        Expr::Var(name) => {
            if ctx.address_of_vars.contains(name) {
                format!("{name}.value")
            } else if ctx.double_address_of_vars.contains(name) {
                format!("{name}[0]")
            } else {
                name.clone()
            }
        }
        Expr::ArrayAccess(name, idx) => {
            format!(
                "{}[{}]",
                name,
                render_expr(idx, ctx, registry, helpers)
            )
        }
        Expr::Literal(_)
        | Expr::IntLiteral(_)
        | Expr::BinOp(_, _, _)
        | Expr::Cast(_, _)
        | Expr::Not(_)
        | Expr::BitwiseNot(_)
        | Expr::FuncCall(_, _)
        | Expr::PointerDeref(_)
        | Expr::AddressOf(_)
        | Expr::PostIncrement(_)
        | Expr::PostDecrement(_)
        | Expr::PreIncrement(_)
        | Expr::PreDecrement(_)
        | Expr::Ternary(_, _, _) => {
            render_expr(expr, ctx, registry, helpers)
        }
    }
}

/// Render a return expression, mapping known enum values to Java constants.
fn render_return_expr(
    expr: &Expr,
    ctx: &JavaRenderCtx,
    registry: &Registry,
    helpers: &HelperRegistry,
) -> String {
    if let Expr::Var(name) = expr {
        return match name.as_str() {
            "SUCCESS" => "RetCode.Success".to_string(),
            "BadParam" => "RetCode.BadParam".to_string(),
            "InsufficientHistory" => "RetCode.InsufficientHistory".to_string(),
            "OutOfRangeEndIndex" => "RetCode.OutOfRangeEndIndex".to_string(),
            "OutOfRangeStartIndex" => "RetCode.OutOfRangeStartIndex".to_string(),
            _ => render_expr(expr, ctx, registry, helpers),
        };
    }
    render_expr(expr, ctx, registry, helpers)
}

#[allow(clippy::too_many_lines)]
/// Java-backend leaf formatting for the shared [`ExprEmitter`] tree-walk. Bundles
/// the render context with the registry/helper services the call-dispatch hooks
/// need; the recursion itself lives in [`ExprEmitter::walk`].
struct JavaExpr<'a> {
    ctx: &'a JavaRenderCtx<'a>,
    registry: &'a Registry,
    helpers: &'a HelperRegistry,
}

impl ExprEmitter for JavaExpr<'_> {
    fn var(&self, name: &str) -> String {
        let mapped = match name {
            // Java has no compatibility field: every read is folded away by
            // `fold_compat_cond` before rendering. Reaching here means a new
            // construct escaped the fold — fail loudly rather than emit a
            // reference to a field that does not exist.
            "COMPATIBILITY" | "METASTOCK" | "DEFAULT" => panic!(
                "java: compatibility reference `{name}` survived the render-time fold \
                 (Java pins the mode to Default — extend fold_compat_cond to cover \
                 this construct)"
            ),
            "BAD_PARAM" => "RetCode.BadParam".to_string(),
            "SUCCESS" => "RetCode.Success".to_string(),
            "ALLOC_ERR" => "RetCode.AllocErr".to_string(),
            "INTERNAL_ERROR" => "RetCode.InternalError".to_string(),
            // MAType constants (`TA_MAType_SMA` → `MAType.Sma`) resolve from the
            // enums.yaml-derived map on the ctx; unknown names pass through.
            _ => self.ctx.matype_map.get(name).cloned().unwrap_or_else(|| name.to_string()),
        };
        if self.ctx.address_of_vars.contains(name) {
            format!("{mapped}.value")
        } else if self.ctx.double_address_of_vars.contains(name) {
            format!("{mapped}[0]")
        } else {
            mapped
        }
    }

    fn array_access(&self, name: &str, idx: &Expr) -> String {
        let access = format!("{}[{}]", name, self.walk(idx));
        // Single-precision variants take float[] inputs but compute and store in
        // double. Java evaluates float*float in float, so combining two float
        // inputs directly (e.g. the mult float[] overload's inReal0[i]*inReal1[i])
        // rounds — and can overflow to Infinity — before the widening to double.
        // Widen each float input element as it is read. Reported as PR #33
        // (@iglesias). Double-input overloads are unchanged.
        if self.ctx.single_precision && self.ctx.float_input_params.contains(name) {
            format!("(double){access}")
        } else {
            access
        }
    }

    fn binop(&self, left: &Expr, op: &BinOp, right: &Expr) -> String {
        // In single-precision variants, input params are float[] and output params are
        // double[]. Java forbids == / != comparisons between incompatible array types.
        // When exactly one operand is a known float input param, the comparison can
        // never be true (they are different types and can never alias), so emit false/true.
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
        // Fused multiply-add: (a * b) + c → Math.fma(a, b, c) (Java 9+). IEEE-754
        // correctly-rounded, so it matches C `fma()` and Rust `mul_add` at the same
        // sites (site selection is the shared `fma::fuse_operands`). `Math.fma` is a
        // call expression (atomic), so no outer parens / wrap_child needed.
        if fma::EMIT_FMA {
            if let Some(fs) = self.ctx.fma {
                if let Some((a, b, c)) = fma::fuse_operands(left, op, right, &fs.view()) {
                    return format!("Math.fma({}, {}, {})", self.walk(a), self.walk(b), self.walk(c));
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
            BinOp::BitwiseXor => "^",
            BinOp::BitwiseAnd => "&",
            BinOp::Shr => ">>",
            BinOp::Shl => "<<",
        };
        // Minimal parenthesization: only wrap an operand that binds looser than
        // this operator (Java shares C's operator precedence).
        let pp = binop_prec(op);
        // Integer-bitwise operand of a logical operator carries C truthiness:
        // it needs an explicit != 0 here.
        let logical = matches!(op, BinOp::And | BinOp::Or);
        let l = if logical && is_int_bitwise(left) {
            format!("({}) != 0", self.walk(left))
        } else {
            wrap_child(self.walk(left), left, pp, false)
        };
        let r = if logical && is_int_bitwise(right) {
            format!("({}) != 0", self.walk(right))
        } else {
            wrap_child(self.walk(right), right, pp, true)
        };
        format!("{l} {op_str} {r}")
    }

    fn cast(&self, var_type: &VarType, inner: &Expr) -> String {
        // In single-precision variants `array_access` already widens a float input
        // element read to double. Drop a redundant source-level `(double)` around
        // such a read so we emit `(double)inX[i]`, not `(double)(double)inX[i]`.
        if self.ctx.single_precision && matches!(var_type, VarType::Real) {
            if let Expr::ArrayAccess(name, _) = inner {
                if self.ctx.float_input_params.contains(name) {
                    return self.walk(inner);
                }
            }
        }
        let java_type = match var_type {
            VarType::Real => "double",
            VarType::Integer | VarType::Index => "int",
            VarType::RetCodeType => "RetCode",
            VarType::RealPointer => "double[]",
            VarType::IntPointer => "int[]",
            VarType::RealArray(_) | VarType::IntArray(_) => "/* array cast */",
        };
        let s = self.walk(inner);
        let s = if expr_prec(inner) < 12 { format!("({s})") } else { s };
        format!("({java_type}){s}")
    }

    fn not(&self, inner: &Expr) -> String {
        // C's logical `!` over an integer-bitwise value: `!` needs a boolean
        // operand here, so spell out the comparison.
        if is_int_bitwise(inner) {
            return format!("(({}) == 0)", self.walk(inner));
        }
        let s = self.walk(inner);
        if expr_prec(inner) < 12 {
            format!("!({s})")
        } else {
            format!("!{s}")
        }
    }

    fn bitwise_not(&self, inner: &Expr) -> String {
        let s = self.walk(inner);
        if expr_prec(inner) < 12 {
            format!("~({s})")
        } else {
            format!("~{s}")
        }
    }

    fn func_call(&self, name: &str, args: &[Expr]) -> String {
        render_func_call(name, args, self.ctx, self.registry, self.helpers)
    }

    fn pointer_deref(&self, name: &str) -> String {
        // Java has no pointer dereference; output params are MInteger .value
        // For double address-of vars, use [0] instead
        if self.ctx.double_address_of_vars.contains(name) {
            format!("{name}[0]")
        } else {
            format!("{name}.value")
        }
    }

    fn address_of(&self, inner: &Expr) -> String {
        // Java has no address-of; render the inner expression directly.
        // Pass empty sets so MInteger vars render as object refs (no .value)
        // and double[] vars render as array refs (no [0]).
        let empty = HashSet::new();
        let inner_ctx = JavaRenderCtx {
            single_precision: self.ctx.single_precision,
            address_of_vars: &empty,
            double_address_of_vars: &empty,
            float_input_params: self.ctx.float_input_params,
            inline_counter: self.ctx.inline_counter,
            // Carry the fusion sets so any a*b+c inside the address-of expression
            // fuses consistently with the surrounding body.
            fma: self.ctx.fma,
            matype_map: self.ctx.matype_map.clone(),
        };
        render_expr(inner, &inner_ctx, self.registry, self.helpers)
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
        // Collapse `cond ? 1 : 0` / `cond ? 0 : 1` to a bare boolean expression.
        // is_boolean_expr consults the same bool_ternary_collapse rule so it
        // agrees the collapsed result is boolean-typed.
        // A bitwise condition is integer-typed: collapsing would substitute the
        // mask value for C's 0/1, and `!` cannot apply. Wrap with != 0 instead.
        if !is_int_bitwise(cond) {
            match bool_ternary_collapse(then_expr, else_expr) {
                Some(BoolTernaryCollapse::Cond) => return self.walk(cond),
                Some(BoolTernaryCollapse::Negated) => return format!("!({})", self.walk(cond)),
                None => {}
            }
        }
        // Default: render as Java ternary
        let c = self.walk(cond);
        let t = self.walk(then_expr);
        let e = self.walk(else_expr);
        let c = if is_int_bitwise(cond) {
            format!("(({c}) != 0)")
        } else if matches!(cond, Expr::BinOp(..) | Expr::Ternary(..)) {
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
    ctx: &JavaRenderCtx,
    registry: &Registry,
    helpers: &HelperRegistry,
) -> String {
    JavaExpr { ctx, registry, helpers }.walk(expr)
}

/// Render one of the boolean value builtins (the near-zero trio IS_ZERO /
/// IS_ZERO_SCALED / IS_ZERO_OR_NEG, plus the exact IS_FINITE) in Java from already-rendered argument strings. Single source
/// of the Java form for these predicates — used by both the indicator render path
/// and the `eval_predicate` server handler (see the C backend for the rationale).
pub(crate) fn java_predicate_expr(which: SpecialBuiltin, args: &[String]) -> String {
    match which {
        SpecialBuiltin::IsZero => args.first().map_or_else(
            || "false".to_string(),
            |x| format!("((-0.00000000000001 < {x}) && ({x} < 0.00000000000001))"),
        ),
        SpecialBuiltin::IsZeroScaled => {
            if args.len() == 2 {
                format!("(Math.abs({}) <= 0.00000000000001 * ({}))", args[0], args[1])
            } else {
                "false".to_string()
            }
        }
        SpecialBuiltin::IsZeroOrNeg => args
            .first()
            .map_or_else(|| "false".to_string(), |x| format!("({x} < 0.00000000000001)")),
        SpecialBuiltin::IsFinite => args
            .first()
            .map_or_else(|| "false".to_string(), |x| format!("(Double.isFinite({x}))")),
        _ => "false".to_string(),
    }
}

/// The `FuncUnstId` variant name for an `UNSTABLE_PERIOD(<name>)` argument
/// (with or without its `FUNC_UNST_` prefix). Verbatim: the enum emitted from
/// enums.yaml spells every variant exactly as the YAML names it.
pub(crate) fn unst_variant_name(func_name: &str) -> String {
    func_name.strip_prefix("FUNC_UNST_").unwrap_or(func_name).to_string()
}

/// Try to render a candle helper function call as an inline Java ternary chain.
///
/// Converts `ta_candlerange(rangeType, open, high, low, close)` into a nested
/// ternary that mirrors the original switch:
/// ```text
/// ((rt==0) ? Math.abs(close-open) : ((rt==1) ? (high-low) : ((rt==2) ? …)))
/// ```
///
/// `ta_candleaverage(rangeType, avgPeriod, factor, sum, open, high, low, close)`
/// becomes:
/// ```text
/// (factor * (((avgPeriod!=0) ? sum/avgPeriod : <candlerange>) / ((rt==2)?2.0:1.0)))
/// ```
///
/// Returns `None` if the function isn't a candle helper or the arg count is wrong.
fn try_render_candle_ternary(
    fname: &str,
    args: &[Expr],
    ctx: &JavaRenderCtx,
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
            // The Shadows arm is upper + lower, NOT the algebraically equal
            // (high - low) - |close - open|. It must match TA_CANDLERANGE in
            // ta_utility.h term for term: the two forms differ by
            // reassociation on any bar whose low sits below half its high,
            // and C is the reference (#217). This spelling is hardcoded here
            // rather than read from input/helpers/candlestick.c, so a fix to
            // the helper alone does NOT reach Java -- csharp.rs carries the
            // same duplicate.
            Some(format!(
                "(({rt} == 0) ? (Math.abs({close} - {open})) \
                 : (({rt} == 1) ? ({high} - {low}) \
                 : (({rt} == 2) ? (({high} - ((({close}) >= ({open})) ? ({close}) : ({open}))) \
                 + (((({close}) >= ({open})) ? ({open}) : ({close})) - {low})) \
                 : 0.0)))"
            ))
        }
        "ta_candleaverage" if args.len() == 8 => {
            let rt = r(&args[0]);
            let avg_period = r(&args[1]);
            let factor = r(&args[2]);
            let sum = r(&args[3]);
            // Build the 5-element arg list for the nested ta_candlerange call:
            // [rangeType, open, high, low, close]
            let cr_args: Vec<Expr> = std::iter::once(args[0].clone())
                .chain(args[4..8].iter().cloned())
                .collect();
            let candlerange = try_render_candle_ternary(
                "ta_candlerange", &cr_args, ctx, registry, helpers,
            )?;
            Some(format!(
                "(({factor} * ((({avg_period} != 0) \
                 ? ({sum} / {avg_period}) : {candlerange}) \
                 / (({rt} == 2) ? 2.0 : 1.0))))"
            ))
        }
        _ => None,
    }
}

/// Render a `FuncCall` expression to Java code.
#[allow(clippy::too_many_lines)]
fn render_func_call(
    fname: &str,
    args: &[Expr],
    ctx: &JavaRenderCtx,
    registry: &Registry,
    helpers: &HelperRegistry,
) -> String {
    // Check if this is a call to a helper function that can be inlined
    if let Some(helper) = helpers.get(fname) {
        if let Some(inlined_expr) = try_inline_expr(helper, args) {
            let s = render_expr(&inlined_expr, ctx, registry, helpers);
            // Spliced where an atomic call result is expected — wrap a
            // non-atomic inlined body to preserve the surrounding grouping.
            return wrap_inlined(s, &inlined_expr);
        }
        // Multi-statement helpers: Task 10 will handle
    }

    // Candle helpers: render inline as Java ternary chains instead of
    // hoisted switch blocks.  This keeps them inside the expression so
    // the && split can preserve short-circuit evaluation.
    if let Some(ternary) = try_render_candle_ternary(fname, args, ctx, registry, helpers) {
        return ternary;
    }

    if let Some(b) = SpecialBuiltin::from_name(fname) {
        match b {
            SpecialBuiltin::UnstablePeriod => {
                // UNSTABLE_PERIOD(RSI) -> this.unstablePeriod[FuncUnstId.Rsi.ordinal()]
                // UNSTABLE_PERIOD(FUNC_UNST_ATR) -> strip FUNC_UNST_ prefix first
                if let Some(Expr::Var(func_name)) = args.first() {
                    let variant = unst_variant_name(func_name);
                    return format!("this.unstablePeriod[FuncUnstId.{variant}.ordinal()]");
                }
                "this.unstablePeriod[0]".to_string()
            }
            SpecialBuiltin::Compatibility => {
                // See the `var` hook: Java pins the mode to Default and carries no
                // compatibility field, so a surviving read is a generator bug.
                panic!(
                    "java: COMPATIBILITY() survived the render-time fold (Java pins \
                     the mode to Default — extend fold_compat_cond to cover this \
                     construct)"
                )
            }
            pred @ (SpecialBuiltin::IsZero
                   | SpecialBuiltin::IsZeroScaled
                   | SpecialBuiltin::IsZeroOrNeg
                   | SpecialBuiltin::IsFinite) => {
                // The near-zero trio -> the Java epsilon form; IS_FINITE -> Double.isFinite.
                // java_predicate_expr is the single source of that form (also used by
                // the eval_predicate server handler).
                let rendered: Vec<String> = args
                    .iter()
                    .map(|a| render_expr(a, ctx, registry, helpers))
                    .collect();
                java_predicate_expr(pred, &rendered)
            }
            SpecialBuiltin::ArrayCopy => {
                // ARRAY_COPY(dst, dstOff, src, srcOff, count)
                // -> System.arraycopy(src, srcOff, dst, dstOff, count) (note arg reordering)
                if args.len() == 5 {
                    let dst = render_expr(&args[0], ctx, registry, helpers);
                    let dst_off = render_expr(&args[1], ctx, registry, helpers);
                    let src = render_expr(&args[2], ctx, registry, helpers);
                    let src_off = render_expr(&args[3], ctx, registry, helpers);
                    let count = render_expr(&args[4], ctx, registry, helpers);
                    return format!("System.arraycopy({src},{src_off},{dst},{dst_off},{count})");
                }
                "/* ARRAY_COPY: bad args */".to_string()
            }
            SpecialBuiltin::PerToK => {
                // PER_TO_K(period) -> (2.0 / ((double)(period) + 1.0))
                if let Some(arg) = args.first() {
                    let x = render_expr(arg, ctx, registry, helpers);
                    return format!("(2.0 / ((double)({x}) + 1.0))");
                }
                "0.0".to_string()
            }
        }
    } else if let Some(mf) = MathFn::from_name(fname) {
        // Java uses Math.func() for standard math functions. The canonical math
        // name already matches java.lang.Math: fabs/ABS → abs, max/fmax → max,
        // min/fmin → min.
        let rendered: Vec<String> = args
            .iter()
            .map(|a| render_expr(a, ctx, registry, helpers))
            .collect();
        format!("Math.{}({})", mf.canonical(), rendered.join(", "))
    } else if let Some(s) = StdlibFn::from_name(fname) {
        match s {
            StdlibFn::Sizeof => {
                // sizeof(TYPE) → 1: normalizes byte counts to element counts for Java array operations
                "1".to_string()
            }
            StdlibFn::Malloc => {
                // malloc(N * sizeof(TYPE)) → new TYPE_JAVA[(int)(N)]
                // sizeof renders as 1, so the arg is already the element count
                if let Some(arg) = args.first() {
                    let java_type = match find_sizeof_type(arg).as_deref() {
                        Some("int") => "int",
                        Some("float") => "float",
                        _ => "double",
                    };
                    let size = render_expr(arg, ctx, registry, helpers);
                    format!("new {java_type}[(int)({size})]")
                } else {
                    "new double[0]".to_string()
                }
            }
            StdlibFn::Free => {
                // No-op in Java (garbage collector handles deallocation)
                String::new()
            }
            StdlibFn::Memcpy | StdlibFn::Memmove => {
                // memcpy/memmove(dst, src, count) → System.arraycopy(src, srcOff, dst, dstOff, count)
                if args.len() >= 3 {
                    let (dst_arr, dst_off) =
                        decompose_java_array_ref(&args[0], ctx, registry, helpers);
                    let (src_arr, src_off) =
                        decompose_java_array_ref(&args[1], ctx, registry, helpers);
                    let count = render_expr(&args[2], ctx, registry, helpers);
                    format!("System.arraycopy({src_arr}, {src_off}, {dst_arr}, {dst_off}, {count})")
                } else {
                    format!("/* {fname}: bad args */")
                }
            }
            StdlibFn::Memset => {
                // memset(buf, 0, count) → java.util.Arrays.fill(buf, off, off+count, fillVal)
                if args.len() >= 3 {
                    let (arr, off) =
                        decompose_java_array_ref(&args[0], ctx, registry, helpers);
                    let count = render_expr(&args[2], ctx, registry, helpers);
                    let fill_val = match find_sizeof_type(&args[2]).as_deref() {
                        Some("int") => "0",
                        _ => "0.0",
                    };
                    if off == "0" {
                        format!("java.util.Arrays.fill({arr}, 0, (int)({count}), {fill_val})")
                    } else {
                        format!(
                            "java.util.Arrays.fill({arr}, {off}, ({off}) + (int)({count}), {fill_val})"
                        )
                    }
                } else {
                    "/* memset: bad args */".to_string()
                }
            }
        }
    } else {
        // Use registry for cross-call resolution
        let java_name = registry.resolve_call(fname, Lang::Java);
        let rendered: Vec<String> = args
            .iter()
            .map(|a| match a {
                // NULL for a nullable output the caller discards (MA passing NULL
                // for MAMA's FAMA — issue #125). Java arrays are nullable, but the
                // callee writes into it unconditionally, so materialize a throwaway
                // spanning the output range (mirrors the discard buffer the C
                // source used before nullable outputs). NULL appears only here.
                Expr::Var(n) if n == "NULL" => {
                    "new double[(int)(endIdx - startIdx + 1)]".to_string()
                }
                _ => render_expr(a, ctx, registry, helpers),
            })
            .collect();
        format!("{}({})", java_name, rendered.join(", "))
    }
}

/// Emit a cross-indicator call to the callee's PUBLIC entry point (#236 step 3).
///
/// The C source is written in C's idiom -- `retCode = ma( .., &beg, &nb, buf );
/// if( retCode != TA_SUCCESS ) return retCode;` -- and the transcription is
/// literal, so every backend needed a callee that answered a code through
/// out-parameters. C never did: `ta_APO.c` calls `TA_MA`, which IS C's public
/// API. The managed backends now do the same, which is what puts the callee's
/// argument checks on the composed path -- the one place a scratch buffer sized
/// by the CALLER meets a bound computed from the CALLEE's lookback.
///
/// The two out-parameter arguments are dropped from the call and bound from the
/// returned range instead. They are found positionally: the callee's signature
/// is `(startIdx, endIdx, inputs.., opts.., outBegIdx, outNBElement, outputs..)`,
/// and the registry knows how many outputs it declares. Returns `None` when that
/// arithmetic does not hold, so a shape this does not understand falls through
/// to the old rendering rather than being silently mis-sliced.
///
/// The enclosing `if( retCode != Success )` is left standing and becomes dead:
/// the body stays a literal transcription of its C source, and several of those
/// tests also carry a `|| count == 0` half that is still live.
fn render_cross_indicator_call(
    fname: &str,
    args: &[Expr],
    indent: usize,
    ctx: &JavaRenderCtx,
    registry: &Registry,
    helpers: &HelperRegistry,
) -> Option<String> {
    let n_out = registry.callee_outputs(fname).len();
    if n_out == 0 || args.len() < n_out + 2 {
        return None;
    }
    let split = args.len() - n_out - 2;
    let pad = " ".repeat(indent);
    let public = registry.resolve_call(fname, Lang::Java);

    let mut call_args: Vec<String> = Vec::new();
    for a in args[..split].iter().chain(args[split + 2..].iter()) {
        call_args.push(match a {
            // NULL for a nullable output the caller discards (#125): the callee
            // writes it unconditionally, so materialize a throwaway.
            Expr::Var(n) if n == "NULL" => "new double[(int)(endIdx - startIdx + 1)]".to_string(),
            _ => render_expr(a, ctx, registry, helpers),
        });
    }

    let n = ctx.inline_counter.get();
    ctx.inline_counter.set(n + 1);
    let tmp = format!("_xr{n}");
    let beg = out_meta_target(&args[split], ctx, registry, helpers);
    let nb = out_meta_target(&args[split + 1], ctx, registry, helpers);
    Some(format!(
        "{pad}OutRange {tmp} = {public}({});\n{pad}{beg}.value = {tmp}.begIdx();\n{pad}{nb}.value = {tmp}.count();\n",
        call_args.join(", ")
    ))
}

/// The `MInteger` an out-parameter argument names. `&beg` and a pointer
/// parameter passed straight through (`outNBElement`) are both spelled as the
/// object here; only an rvalue READ of one renders as `.value`, which is why
/// this cannot go through `render_expr`.
fn out_meta_target(
    arg: &Expr,
    ctx: &JavaRenderCtx,
    registry: &Registry,
    helpers: &HelperRegistry,
) -> String {
    match arg {
        Expr::AddressOf(inner) => match inner.as_ref() {
            Expr::Var(n) => n.clone(),
            other => render_expr(other, ctx, registry, helpers),
        },
        Expr::Var(n) => n.clone(),
        other => render_expr(other, ctx, registry, helpers),
    }
}

/// Decompose an expression into (array_name, offset) for array copy operations.
/// `Var("arr")` → `("arr", "0")`; `AddressOf(ArrayAccess("arr", idx))` → `("arr", rendered_idx)`
fn decompose_java_array_ref(
    expr: &Expr,
    ctx: &JavaRenderCtx,
    registry: &Registry,
    helpers: &HelperRegistry,
) -> (String, String) {
    match expr {
        Expr::AddressOf(inner) => {
            if let Expr::ArrayAccess(name, offset) = inner.as_ref() {
                let off = render_expr(
                    offset, ctx, registry, helpers,
                );
                (name.clone(), off)
            } else {
                let s = render_expr(
                    expr, ctx, registry, helpers,
                );
                (s, "0".to_string())
            }
        }
        Expr::Var(name) => (name.clone(), "0".to_string()),
        _ => {
            let s = render_expr(
                expr, ctx, registry, helpers,
            );
            (s, "0".to_string())
        }
    }
}

/// Render a complex lookback body (`LookbackExpr::Code`) into Java code.
fn render_lookback_code(
    stmts: &[Statement],
    enums: &HashMap<String, EnumDef>,
    registry: &Registry,
    helpers: &HelperRegistry,
) -> String {
    let mut out = String::new();
    let inline_counter = Cell::new(0);
    // Lookback bodies don't have cross-indicator calls, so no address-of vars
    let address_of_vars = HashSet::new();
    let double_address_of_vars = HashSet::new();
    // Lookback bodies are always double-precision; no float input params needed
    let float_input_params: HashSet<String> = HashSet::new();
    let ctx = JavaRenderCtx {
        single_precision: false,
        address_of_vars: &address_of_vars,
        double_address_of_vars: &double_address_of_vars,
        float_input_params: &float_input_params,
        inline_counter: &inline_counter,
        // Lookback bodies are pure integer index arithmetic — no float multiply-add.
        fma: None,
        matype_map: build_matype_map(enums),
    };

    // Declare local variables
    for stmt in stmts {
        if let Statement::VarDecl { var_type, name, .. } = stmt {
            let java_decl = match var_type {
                VarType::RealArray(size) => format!("double[] {name} = new double[{size}]"),
                VarType::IntArray(size) => format!("int[] {name} = new int[{size}]"),
                _ => format!("{} {name}", java_type_str(var_type)),
            };
            out.push_str(&format!("      {java_decl};\n"));
        }
    }

    // Emit candle settings unpacking for lookback body
    let candle_used = detect_candle_settings(stmts);
    if !candle_used.is_empty() {
        out.push_str(&emit_java_unpacking(&candle_used, 6));
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

    fn make_registry() -> Registry {
        let base = Path::new(env!("CARGO_MANIFEST_DIR")).join("../../ta_codegen/input");
        Registry::from_dir(&base)
    }

    fn load_sma() -> FuncDef {
        let base = Path::new(env!("CARGO_MANIFEST_DIR"));
        let yaml_path = base.join("../../ta_codegen/input/sma/sma.yaml");
        let c_path = base.join("../../ta_codegen/input/sma/sma.c");
        let mut func_def = parser::yaml::parse_yaml(&yaml_path);
        let parsed = parser::c_source::parse_c_source(&c_path);
        func_def.body = parsed.functions[0].body.clone();
        func_def.lookback = Some(LookbackExpr::Code(parsed.lookback_body));
        func_def
    }

    #[test]
    fn test_java_generates_core_and_wrapper() {
        let func = load_sma();
        let enums = HashMap::new();
        let registry = make_registry();
        let output = generate(&func, &enums, &registry, &HelperRegistry::empty());

        // #236 step 5: the C-shaped tier is GONE. Two tiers remain -- the
        // public wrapper and the body it calls -- and nothing in the shipped
        // library answers a RetCode any more.
        assert!(!output.contains("SMA_Internal"), "the C-shaped tier must not come back");
        assert!(!output.contains("Unguarded"), "no unguarded tier may exist");
        assert!(
            !output.contains("public RetCode SMA"),
            "cores must be package-private — RetCode never appears on the public surface"
        );

        // The BODY validates. Bounded to the double body's own text so a match
        // inside the float overload cannot stand in for it.
        let body_pos = output.find("RetCode SMA_Impl( ").unwrap();
        let body_section = &output[body_pos..];
        let body_end = body_section[1..]
            .find("   RetCode ")
            .map_or(body_section.len(), |i| i + 1);
        assert!(
            body_section[..body_end].contains("OutOfRangeStartIndex"),
            "the body should contain validation"
        );

        // The public surface is OutRange-returning wrappers, and they call the
        // BODY, not the shim — a sub-call's throw has to propagate rather than be
        // converted and re-thrown under the outer function's name.
        assert!(output.contains("   public OutRange SMA( "), "Missing public SMA wrapper");
        assert!(
            output.contains("RetCode retCode = SMA_Impl("),
            "the public wrapper must call the body directly"
        );
        assert!(
            output.contains("throw failure(\"SMA\", retCode);"),
            "guarded wrapper must map RetCode onto the documented exception"
        );
    }
}
