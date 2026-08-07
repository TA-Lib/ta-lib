//! Identifier validation for the `ta_codegen/input/` source of truth.
//!
//! Every name authored under `ta_codegen/input/` — the indicator name, its
//! signature arguments, and every local declared in its C body — is rendered
//! into four languages. It therefore has to be a legal identifier in *all four*,
//! and the intersection is smaller than any one of them: `base` is unremarkable
//! C but a C# keyword, `loop` is fine in C and Java but a Rust keyword, `new` is
//! fine in C and Rust but a Java keyword. Whichever backend you happen to build
//! first, the others fail — usually as a compiler error inside generated code,
//! a long way from the input file that caused it.
//!
//! So the check runs once, up front, over the union of what the backends
//! forbid. A name is rejected if **any** backend rejects it, and the message
//! names the backend and the spelling it objected to.
//!
//! Two layers:
//!
//! 1. [`check_syntax`] — the language-independent shape of an identifier: ASCII
//!    letters, digits and `_`, no leading digit, no leading `__` or `_X`.
//! 2. [`LanguageBackend::check_name`] — one call per backend, each free to
//!    define its own word list ([`LanguageBackend::reserved_words`]), its own
//!    name mangling ([`LanguageBackend::rendered_name`]), or to override the
//!    whole check.
//!
//! [`LanguageBackend::check_name`]: crate::backends::LanguageBackend::check_name
//! [`LanguageBackend::reserved_words`]: crate::backends::LanguageBackend::reserved_words
//! [`LanguageBackend::rendered_name`]: crate::backends::LanguageBackend::rendered_name

use std::collections::BTreeSet;

use crate::backends;
use crate::helper_registry::HelperRegistry;
use crate::ir::{CircBuf, CircBufLayout, FuncDef, LookbackExpr, Statement};

/// Where a name appears, which is what decides how each backend spells it.
#[derive(Debug, Clone, Copy)]
pub enum NameKind<'a> {
    /// An indicator function name (`FuncDef.name`), carrying the YAML
    /// `camel_case` override when the function declares one — that field, not
    /// the directory name, is what Java and C# render.
    Function { camel_case: Option<&'a str> },
    /// A generated-signature argument: an input, an optional input or an output.
    /// Emitted verbatim by every backend.
    Argument,
    /// A local declared in a function body — a `VarDecl`, a `CIRCBUF` buffer id
    /// or one of its fields, a private-variant extra parameter. Emitted verbatim
    /// by every backend.
    Variable,
    /// An `input/helpers/*.c` function name or one of its parameters. Helpers
    /// are inlined at their call sites and so reach no output on the normal
    /// path, but they are authored in the same C and read by the same parser.
    Helper,
}

impl NameKind<'_> {
    /// How the name reads in an error message ("local variable `base`").
    fn label(self) -> &'static str {
        match self {
            NameKind::Function { .. } => "function name",
            NameKind::Argument => "argument",
            NameKind::Variable => "local variable",
            NameKind::Helper => "helper name",
        }
    }
}

/// Is `name` usable, as `kind`, by every backend?
///
/// The boolean form of [`check_name`], for callers that only need the verdict.
#[must_use]
pub fn is_valid_name(name: &str, kind: NameKind<'_>) -> bool {
    check_name(name, kind).is_ok()
}

/// Validate `name` for use as `kind`, returning the reason it cannot be used.
///
/// Runs [`check_syntax`] first, then asks every registered backend. The first
/// objection wins — a name is no more usable for being rejected twice, and one
/// clear reason reads better than four.
///
/// [`check_syntax`] runs on each backend's *rendered* spelling too, not only on
/// the authored one: a function's Java and C# names come from the YAML
/// `camel_case` field, which nothing else checks, so `camel_case: My Func` would
/// otherwise reach `javac` before anything noticed.
///
/// The returned string is a reason clause, phrased to follow the name:
/// `` `base` is a reserved word in the csharp backend ``.
pub fn check_name(name: &str, kind: NameKind<'_>) -> Result<(), String> {
    check_syntax(name)?;
    for backend in backends::all() {
        let rendered = backend.rendered_name(name, kind);
        if rendered != name {
            check_syntax(&rendered).map_err(|reason| {
                format!(
                    "it renders as `{rendered}` in the {} backend, and {reason}",
                    backend.name()
                )
            })?;
        }
        backend.check_name(name, kind)?;
    }
    Ok(())
}

/// The language-independent half: what every target agrees an identifier looks
/// like.
///
/// ASCII only — C, Java, C# and Rust all accept some wider set, but they do not
/// accept the *same* wider set, and generated source that is not pure ASCII has
/// an encoding problem waiting in it. The leading-underscore rules are C's
/// (C11 7.1.3): `__*` and `_<uppercase>*` are reserved to the implementation at
/// every scope, so they are never the input's to spend.
pub fn check_syntax(name: &str) -> Result<(), String> {
    if name.is_empty() {
        return Err("a name may not be empty".to_string());
    }
    let mut chars = name.chars();
    let first = chars.next().expect("non-empty");
    if !(first.is_ascii_alphabetic() || first == '_') {
        let what = if first.is_ascii_digit() {
            "a name may not start with a digit".to_string()
        } else {
            format!("a name must start with an ASCII letter or `_`, not `{first}`")
        };
        return Err(what);
    }
    for c in chars {
        if !(c.is_ascii_alphanumeric() || c == '_') {
            let what = match c {
                ' ' => "a space".to_string(),
                '\t' => "a tab".to_string(),
                c => format!("`{c}`"),
            };
            return Err(format!(
                "a name may only contain ASCII letters, digits and `_`; found {what}"
            ));
        }
    }
    if name.starts_with("__") {
        return Err("a leading `__` is reserved to the C implementation (C11 7.1.3)".to_string());
    }
    if let Some(rest) = name.strip_prefix('_') {
        if rest.chars().next().is_some_and(|c| c.is_ascii_uppercase()) {
            return Err(
                "`_` followed by an upper-case letter is reserved to the C implementation (C11 7.1.3)"
                    .to_string(),
            );
        }
    }
    Ok(())
}

/// Validate every name the loaded input tree defines.
///
/// Returns one message per rejected name, already prefixed with the function (or
/// helper file) it came from. `Ok(())` means every backend can render every name
/// in `funcs` and `helpers`.
pub fn validate_all(funcs: &[FuncDef], helpers: &HelperRegistry) -> Result<(), Vec<String>> {
    let mut errors = Vec::new();
    for func in funcs {
        validate_func(func, &mut errors);
    }
    for helper in helpers.iter() {
        let scope = format!("helpers/{}", helper.name);
        check(&helper.name, NameKind::Helper, &scope, &mut errors);
        for param in &helper.params {
            check(&param.name, NameKind::Variable, &scope, &mut errors);
        }
        let mut declared = BTreeSet::new();
        collect_declared(&helper.body, &mut declared);
        for name in &declared {
            check(name, NameKind::Variable, &scope, &mut errors);
        }
    }
    if errors.is_empty() {
        Ok(())
    } else {
        Err(errors)
    }
}

/// Validate one function's name, signature and every local it declares.
fn validate_func(func: &FuncDef, errors: &mut Vec<String>) {
    let scope = &func.name;
    check(
        &func.name,
        NameKind::Function {
            camel_case: func.camel_case.as_deref(),
        },
        scope,
        errors,
    );

    for input in &func.inputs {
        check(&input.name, NameKind::Argument, scope, errors);
    }
    for opt in &func.optional_inputs {
        check(&opt.name, NameKind::Argument, scope, errors);
    }
    for out in &func.outputs {
        check(&out.name, NameKind::Argument, scope, errors);
    }
    for (name, _ty) in &func.private_extra_params {
        check(name, NameKind::Argument, scope, errors);
    }

    // Locals from all three bodies. A name declared in more than one of them
    // (the usual case — the guarded and private variants share most of the
    // body) is reported once.
    let mut declared = BTreeSet::new();
    collect_declared(&func.body, &mut declared);
    collect_declared(&func.private_body, &mut declared);
    if let Some(LookbackExpr::Code(body)) = &func.lookback {
        collect_declared(body, &mut declared);
    }
    for name in &declared {
        check(name, NameKind::Variable, scope, errors);
    }
}

/// Run one check, pushing a scoped message on failure.
fn check(name: &str, kind: NameKind<'_>, scope: &str, errors: &mut Vec<String>) {
    if let Err(reason) = check_name(name, kind) {
        errors.push(format!("{scope}: {} `{name}`: {reason}", kind.label()));
    }
}

/// Collect every name a statement tree *declares*, recursing into nested blocks.
///
/// Only declarations: a `Expr::Var` reference must resolve to a declaration or a
/// signature argument, both of which are checked already, so walking expressions
/// would only re-report the same names.
fn collect_declared<'a>(stmts: &'a [Statement], out: &mut BTreeSet<&'a str>) {
    for stmt in stmts {
        match stmt {
            Statement::VarDecl { name, .. } => {
                out.insert(name.as_str());
            }
            Statement::For { var, body, .. } => {
                out.insert(var.as_str());
                collect_declared(body, out);
            }
            Statement::While { body, .. }
            | Statement::DoWhile { body, .. }
            | Statement::Block { body } => collect_declared(body, out),
            Statement::If {
                then_body,
                else_body,
                ..
            } => {
                collect_declared(then_body, out);
                collect_declared(else_body, out);
            }
            Statement::ForC {
                init, update, body, ..
            } => {
                collect_declared(std::slice::from_ref(init), out);
                collect_declared(std::slice::from_ref(update), out);
                collect_declared(body, out);
            }
            Statement::Switch { cases, default, .. } => {
                for (_label, body) in cases {
                    collect_declared(body, out);
                }
                collect_declared(default, out);
            }
            Statement::CircBuf(cb) => collect_circbuf(cb, out),
            Statement::Assign { .. }
            | Statement::UnrollHint { .. }
            | Statement::Return { .. }
            | Statement::Break
            | Statement::Continue
            | Statement::Expr(_)
            | Statement::Comment(_) => {}
        }
    }
}

/// A `CIRCBUF` declares its buffer id, and one buffer per field for the `_CLASS`
/// layout (stored as `<id>_<field>`). Both halves are authored names, so both are
/// checked; the derived spellings (`<id>_Idx`, `maxIdx_<id>`) follow from them.
fn collect_circbuf<'a>(cb: &'a CircBuf, out: &mut BTreeSet<&'a str>) {
    let (id, layout) = match cb {
        CircBuf::Prolog { id, layout, .. }
        | CircBuf::Init { id, layout, .. }
        | CircBuf::InitLocalOnly { id, layout }
        | CircBuf::Destroy { id, layout } => (id, Some(layout)),
        CircBuf::Next { id } => (id, None),
    };
    out.insert(id.as_str());
    if let Some(CircBufLayout::Class(fields)) = layout {
        for (field, _ty) in fields {
            out.insert(field.as_str());
        }
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    use std::path::Path;

    const VAR: NameKind<'static> = NameKind::Variable;
    const FUNC: NameKind<'static> = NameKind::Function { camel_case: None };

    #[test]
    fn accepts_ordinary_names() {
        for name in [
            "i",
            "today",
            "tempReal",
            "outIdx",
            "inReal",
            "optInTimePeriod",
            "outNBElement",
            "prevMA",
            "x2",
            "_scratch",
        ] {
            assert!(is_valid_name(name, VAR), "{name} should be valid");
        }
    }

    #[test]
    fn rejects_malformed_names() {
        // (name, the word the message must mention)
        for (name, needle) in [
            ("", "empty"),
            ("2fast", "digit"),
            ("my var", "space"),
            ("my-var", "`-`"),
            ("kö", "`ö`"),
            ("__hidden", "`__`"),
            ("_Reserved", "upper-case"),
        ] {
            let err = check_name(name, VAR).expect_err("{name} should be rejected");
            assert!(
                err.contains(needle),
                "message for `{name}` should mention {needle}, got: {err}"
            );
        }
    }

    /// The defect this module exists for: `base` is a C# keyword and nothing
    /// else's, so three backends compile and one does not.
    #[test]
    fn rejects_base_as_a_variable() {
        let err = check_name("base", VAR).expect_err("`base` is a C# keyword");
        assert!(err.contains("csharp"), "got: {err}");
        assert!(err.contains("reserved word"), "got: {err}");
    }

    /// One backend objecting is enough — each of these is a keyword in exactly
    /// one of the four.
    #[test]
    fn one_backend_is_enough() {
        for (name, backend) in [
            ("base", "csharp"),
            ("loop", "rust"),
            ("synchronized", "java"),
            ("restrict", "c"),
        ] {
            let err = check_name(name, VAR)
                .expect_err("a single backend's keyword must be enough to reject");
            assert!(
                err.contains(backend),
                "`{name}` should be rejected by {backend}, got: {err}"
            );
        }
    }

    /// A function name is mangled before it is checked, so the verdict can
    /// differ from that of the same word used as a variable.
    #[test]
    fn function_names_are_checked_after_mangling() {
        // `BASE` -> TA_BASE / base / base / Base. Rust's `base` is not a
        // keyword, so unlike the local variable this is fine.
        assert!(is_valid_name("BASE", FUNC));
        // `LOOP` -> TA_LOOP / loop / loop / Loop. Rust's is.
        let err = check_name("LOOP", FUNC).expect_err("`loop` is a Rust keyword");
        assert!(err.contains("rust") && err.contains("`loop`"), "got: {err}");
        // `NEW` -> TA_NEW / new / new / New. Java's is (and C's C++ half).
        assert!(check_name("NEW", FUNC).is_err());
    }

    /// The YAML `camel_case` override is what Java and C# actually emit, so it
    /// is the spelling that has to be legal.
    #[test]
    fn camel_case_override_is_checked() {
        let kind = NameKind::Function {
            camel_case: Some("Base"),
        };
        // `Base` is a fine C# method name...
        assert!(is_valid_name("SOMETHING", kind));
        let kind = NameKind::Function {
            camel_case: Some("Switch"),
        };
        // ...but Java lower-cases the first letter, giving the keyword `switch`.
        let err = check_name("SOMETHING", kind).expect_err("`switch` is a Java keyword");
        assert!(
            err.contains("java") && err.contains("`switch`"),
            "got: {err}"
        );
    }

    /// `camel_case` is authored by hand and is what Java and C# emit verbatim,
    /// so the rendered spelling gets the syntax check too — the authored name
    /// alone would have been fine here.
    #[test]
    fn a_malformed_camel_case_override_is_caught() {
        let kind = NameKind::Function {
            camel_case: Some("My Func"),
        };
        let err = check_name("MYFUNC", kind).expect_err("`My Func` is not an identifier");
        assert!(err.contains("space"), "got: {err}");
    }

    /// Contextual keywords are legal identifiers and must not be rejected —
    /// `value` and `from` in C#, `var` and `record` in Java.
    #[test]
    fn contextual_keywords_are_allowed() {
        for name in ["value", "from", "on", "record", "var", "yield_"] {
            assert!(is_valid_name(name, VAR), "{name} should be valid");
        }
    }

    /// A word only one backend treats as weak is still rejected: `union` is a
    /// legal Rust identifier and a C keyword, and the verdict is the union of
    /// what the backends forbid, not the intersection.
    #[test]
    fn a_weak_keyword_elsewhere_is_still_rejected() {
        let err = check_name("union", VAR).expect_err("`union` is a C keyword");
        assert!(err.contains("the c backend"), "got: {err}");
    }

    /// Every backend must actually carry a list; an empty one would make this
    /// gate silently vacuous for that language.
    #[test]
    fn every_backend_declares_reserved_words() {
        for backend in backends::all() {
            let words = backend.reserved_words();
            assert!(
                !words.is_empty(),
                "backend `{}` declares no reserved words",
                backend.name()
            );
            let mut sorted: Vec<&str> = words.to_vec();
            sorted.sort_unstable();
            sorted.dedup();
            assert_eq!(
                sorted.len(),
                words.len(),
                "backend `{}` has a duplicate reserved word",
                backend.name()
            );
        }
    }

    /// The shipped tree must pass its own gate. Reads `ta_codegen/input/`
    /// directly, so a name added by hand in a later commit fails here rather
    /// than in a downstream compiler.
    #[test]
    fn shipped_input_tree_passes() {
        let base = Path::new(env!("CARGO_MANIFEST_DIR")).join("../../ta_codegen/input");
        if !base.exists() {
            return;
        }
        let helpers = HelperRegistry::from_dir(&base);
        let mut funcs = Vec::new();
        let mut dirs: Vec<_> = std::fs::read_dir(&base)
            .expect("input dir")
            .filter_map(Result::ok)
            .filter(|e| e.path().is_dir())
            .collect();
        dirs.sort_by_key(std::fs::DirEntry::file_name);
        for entry in &dirs {
            let dir = entry.path();
            let name = entry.file_name().to_string_lossy().to_string();
            let yaml = dir.join(format!("{name}.yaml"));
            let c_src = dir.join(format!("{name}.c"));
            if !yaml.exists() || !c_src.exists() {
                continue;
            }
            let mut func = crate::parser::yaml::parse_yaml(&yaml);
            let parsed = crate::parser::c_source::parse_c_source(&c_src);
            crate::parser::c_source::wire_parsed_source(&mut func, &parsed);
            funcs.push(func);
        }
        assert!(!funcs.is_empty(), "no functions loaded from {base:?}");
        if let Err(errors) = validate_all(&funcs, &helpers) {
            panic!("input tree has invalid names:\n  {}", errors.join("\n  "));
        }
    }
}
