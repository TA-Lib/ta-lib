//! The C sample on each function page, and `ta_examples`, the program that compiles
//! and runs every one on the synthetic series (`src/tools/ta_examples/`).

use super::examples::{self, hidden, shown, Snippet, LEN};
use crate::ir::{EnumDef, FuncDef, ParamType};
use std::collections::{BTreeSet, HashMap};
use std::fmt::Write as _;
use std::path::Path;

const WRAP: usize = 100;

const SYNTAX: examples::Syntax<'static> = examples::Syntax {
    real: super::doc_meta::fmt_real,
    bar: "i",
    sin: |x| format!("sin({x})"),
    modulo: |m| format!("(i % {m})"),
};

/// The batch call, `TA_<N>`, after `TA_Initialize`.
#[allow(clippy::implicit_hasher)]
#[must_use]
pub fn example(func: &FuncDef, enums: &HashMap<String, EnumDef>) -> Option<Snippet> {
    let series = examples::all_series(func)?;
    series.first()?;
    let name = &func.name;

    let imports = vec!["#include <stdio.h>".to_string(), "#include <ta_libc.h>".to_string()];
    let mut body = vec![shown(format!("int n = {LEN};"))];
    for s in &series {
        body.push(shown(format!("double {}[{LEN}]; /* {} */", s.var, s.comment)));
    }
    let out_vars = examples::output_vars(func);
    for ty in ["double", "int"] {
        let decls: Vec<String> = func
            .outputs
            .iter()
            .zip(&out_vars)
            .filter(|(o, _)| (o.param_type == ParamType::Integer) == (ty == "int"))
            .map(|(_, v)| format!("{v}[{LEN}]"))
            .collect();
        if !decls.is_empty() {
            body.push(shown(format!("{ty} {};", decls.join(", "))));
        }
    }
    body.push(shown("int outBegIdx, outNBElement;"));
    for s in &series {
        body.push(hidden(format!(
            "{{ int i; for( i = 0; i < n; i++ ) {}[i] = {}; }}",
            s.var,
            examples::render(s, &SYNTAX)
        )));
    }
    body.push(hidden("outBegIdx = -1; outNBElement = 0;"));
    body.push(shown(""));
    body.push(shown("TA_Initialize();"));
    let mut args = vec!["0".to_string(), "n - 1".to_string()];
    args.extend(series.iter().map(|s| s.var.to_string()));
    args.extend(func.optional_inputs.iter().map(|opt| {
        examples::opt_literal(opt, enums, super::rust_doc::fmt_real_literal, |e, v| format!("{}{v}", e.c_prefix))
    }));
    let mut outs = vec!["&outBegIdx".to_string(), "&outNBElement".to_string()];
    outs.extend(out_vars.iter().cloned());
    body.extend(examples::wrap_call(&format!("TA_{name}( "), &args, &outs, " );", WRAP).into_iter().map(shown));
    body.push(hidden(format!("if( !check( \"{name}\", outNBElement > 0 ) ) {{ TA_Shutdown(); return 1; }}")));
    let printed = &out_vars[0];
    let spec = if func.outputs[0].param_type == ParamType::Integer { "%d" } else { "%f" };
    body.push(shown(format!(
        "printf( \"%d values from bar %d; last {printed} = {spec}\\n\", outNBElement, outBegIdx, \
         {printed}[outNBElement - 1] );"
    )));
    if let Some((_, var)) = func.outputs.iter().zip(&out_vars).find(|(o, _)| o.param_type != ParamType::Integer) {
        let helper = if examples::allows_infinity(func) { "no_nan" } else { "all_finite" };
        body.push(hidden(format!(
            "if( !check( \"{name}\", {helper}( {var}, outNBElement ) ) ) {{ TA_Shutdown(); return 1; }}"
        )));
    }
    body.push(hidden("TA_Shutdown();"));
    body.push(hidden("return 0;"));
    Some(Snippet { imports, body })
}

/// The program: one function per sample holding it verbatim, all run by `main`,
/// which exits non-zero when any check failed.
#[allow(clippy::implicit_hasher)]
#[must_use]
pub fn render_program(funcs: &[FuncDef], enums: &HashMap<String, EnumDef>) -> String {
    let mut funcs: Vec<&FuncDef> = funcs.iter().collect();
    funcs.sort_by(|a, b| a.name.cmp(&b.name));
    let samples: Vec<(&FuncDef, Snippet)> =
        funcs.iter().filter_map(|f| example(f, enums).map(|s| (*f, s))).collect();
    let mut includes: BTreeSet<&str> = samples.iter().flat_map(|(_, s)| s.imports.iter().map(String::as_str)).collect();
    includes.insert("#include <math.h>");

    let mut out = String::from(super::ta_abstract_c::LICENSE);
    out.push_str(
        "\n/* GENERATED FILE -- do not edit. Produced by ta_codegen\n\
         \x20* (generator/src/backends/c_examples.rs) from ta_codegen/input/.\n\
         \x20* MF,CC\n\
         \x20*\n\
         \x20* Every function page's C sample, run on the synthetic series: the call\n\
         \x20* answers a non-empty range whose values are finite.\n\
         \x20*/\n\n",
    );
    for include in includes {
        let _ = writeln!(out, "{include}");
    }
    out.push_str(
        "\nstatic int failures = 0;\n\n\
         static int check( const char *name, int ok )\n\
         {\n\
         \x20  if( !ok )\n\
         \x20  {\n\
         \x20     failures++;\n\
         \x20     printf( \"  FAIL: %s example\\n\", name );\n\
         \x20  }\n\
         \x20  return ok;\n\
         }\n\n\
         static int all_finite( const double *a, int n )\n\
         {\n\
         \x20  int i;\n\
         \x20  for( i = 0; i < n; i++ )\n\
         \x20     if( !isfinite( a[i] ) )\n\
         \x20        return 0;\n\
         \x20  return 1;\n\
         }\n\n\
         static int no_nan( const double *a, int n )\n\
         {\n\
         \x20  int i;\n\
         \x20  for( i = 0; i < n; i++ )\n\
         \x20     if( isnan( a[i] ) )\n\
         \x20        return 0;\n\
         \x20  return 1;\n\
         }\n",
    );
    for (func, snippet) in &samples {
        let _ = writeln!(out, "\nstatic int example_{}( void )\n{{", func.name);
        for line in snippet.body_text() {
            if line.is_empty() {
                out.push('\n');
            } else {
                let _ = writeln!(out, "   {line}");
            }
        }
        out.push_str("}\n");
    }
    out.push_str("\nint main( void )\n{\n");
    for (func, _) in &samples {
        let _ = writeln!(out, "   example_{}();", func.name);
    }
    let _ = writeln!(
        out,
        "   printf( \"ta_examples: %d examples, %d failures\\n\", {}, failures );\n\
         \x20  return failures == 0 ? 0 : 1;\n\
         }}",
        samples.len()
    );
    out
}

#[allow(clippy::implicit_hasher)]
pub fn write(funcs: &[FuncDef], enums: &HashMap<String, EnumDef>, path: &Path) {
    if let Some(dir) = path.parent() {
        std::fs::create_dir_all(dir).unwrap_or_else(|e| panic!("creating {}: {e}", dir.display()));
    }
    super::write_if_changed_silent(path, &render_program(funcs, enums));
    println!("  C examples program -> {}", path.display());
}
