//! The Java sample on each function page, and `ExamplesTest`, the suite that compiles
//! and runs every one on the synthetic series.

use super::examples::{self, hidden, shown, Snippet, LEN};
use crate::ir::{EnumDef, FuncDef, ParamType};
use std::collections::{BTreeSet, HashMap};
use std::fmt::Write as _;
use std::path::Path;

const WRAP: usize = 100;

/// The batch call through the public tier, `Core.DEFAULT`.
#[allow(clippy::implicit_hasher)]
#[must_use]
pub fn example(func: &FuncDef, enums: &HashMap<String, EnumDef>) -> Option<Snippet> {
    let series = examples::all_series(func)?;
    let first = series.first()?.var;
    let name = &func.name;
    let method = super::common::camel_words(name);

    let mut imports = vec!["import io.github.talib.Core;".to_string(), "import io.github.talib.OutRange;".to_string()];
    imports.extend(super::rust_doc::example_enum_imports(func).iter().map(|e| format!("import io.github.talib.{e};")));
    imports.sort();

    let mut body = Vec::new();
    for s in &series {
        body.push(shown(format!("double[] {} = new double[{LEN}]; // {}", s.var, s.comment)));
        body.push(hidden(format!(
            "for (int i = 0; i < {LEN}; i++) {}[i] = {};",
            s.var,
            examples::render(s, &examples::MATH_SIN)
        )));
    }
    let out_vars = examples::output_vars(func);
    for (output, var) in func.outputs.iter().zip(&out_vars) {
        let ty = if output.param_type == ParamType::Integer { "int" } else { "double" };
        body.push(shown(format!("{ty}[] {var} = new {ty}[{LEN}];")));
    }
    body.push(shown(""));
    body.push(shown("Core core = Core.DEFAULT;"));
    let mut args = vec!["0".to_string(), format!("{first}.length - 1")];
    args.extend(series.iter().map(|s| s.var.to_string()));
    args.extend(func.optional_inputs.iter().map(|opt| {
        examples::opt_literal(opt, enums, super::rust_doc::fmt_real_literal, |e, v| format!("{}.{v}", e.name))
    }));
    body.extend(
        examples::wrap_call(&format!("OutRange r = core.{method}("), &args, &out_vars, ");", WRAP).into_iter().map(shown),
    );
    body.push(hidden(format!("if (!check(\"{name}\", r.count() > 0)) return;")));
    let printed = &out_vars[0];
    body.push(shown(format!(
        "System.out.println(r.count() + \" values from bar \" + r.begIdx() + \"; last {printed} = \" + \
         {printed}[r.count() - 1]);"
    )));
    if let Some((_, var)) = func.outputs.iter().zip(&out_vars).find(|(o, _)| o.param_type != ParamType::Integer) {
        let helper = if examples::allows_infinity(func) { "noNaN" } else { "allFinite" };
        body.push(hidden(format!("check(\"{name}\", {helper}({var}, r.count()));")));
    }
    Some(Snippet { imports, body })
}

/// The suite: one method per function holding its sample verbatim, run by `main`.
#[allow(clippy::implicit_hasher)]
#[must_use]
pub fn render_test(funcs: &[FuncDef], enums: &HashMap<String, EnumDef>) -> String {
    let mut funcs: Vec<&FuncDef> = funcs.iter().collect();
    funcs.sort_by(|a, b| a.name.cmp(&b.name));
    let samples: Vec<(&FuncDef, Snippet)> =
        funcs.iter().filter_map(|f| example(f, enums).map(|s| (*f, s))).collect();
    let imports: BTreeSet<&String> = samples.iter().flat_map(|(_, s)| s.imports.iter()).collect();

    let mut out = String::from(super::ta_abstract_c::LICENSE);
    out.push_str(
        "\n/* GENERATED FILE \u{2014} do not edit. Produced by ta_codegen\n\
         \x20* (generator/src/backends/java_examples.rs) from ta_codegen/input/.\n\
         \x20* MF,CC\n\
         \x20*/\n\n\
         package io.github.talib.test;\n\n",
    );
    for import in imports {
        let _ = writeln!(out, "{import}");
    }
    out.push_str(
        "\n/**\n\
         \x20* Every function page's Java sample, run on the synthetic series: the\n\
         \x20* call answers a non-empty range whose values are finite.\n\
         \x20*/\n\
         public class ExamplesTest {\n\n\
         \x20   private static int failures = 0;\n\
         \x20   private static int checks = 0;\n\n\
         \x20   private static boolean check(String name, boolean ok) {\n\
         \x20       checks++;\n\
         \x20       if (!ok) {\n\
         \x20           failures++;\n\
         \x20           System.out.println(\"  FAIL: \" + name + \" example\");\n\
         \x20       }\n\
         \x20       return ok;\n\
         \x20   }\n\n\
         \x20   private static boolean allFinite(double[] a, int n) {\n\
         \x20       for (int i = 0; i < n; i++) {\n\
         \x20           if (!Double.isFinite(a[i])) {\n\
         \x20               return false;\n\
         \x20           }\n\
         \x20       }\n\
         \x20       return true;\n\
         \x20   }\n\n\
         \x20   private static boolean noNaN(double[] a, int n) {\n\
         \x20       for (int i = 0; i < n; i++) {\n\
         \x20           if (Double.isNaN(a[i])) {\n\
         \x20               return false;\n\
         \x20           }\n\
         \x20       }\n\
         \x20       return true;\n\
         \x20   }\n",
    );
    for (func, snippet) in &samples {
        let _ = writeln!(out, "\n    static void {}() {{", super::common::camel_words(&func.name));
        for line in snippet.body_text() {
            if line.is_empty() {
                out.push('\n');
            } else {
                let _ = writeln!(out, "        {line}");
            }
        }
        out.push_str("    }\n");
    }
    out.push_str("\n    public static void main(String[] args) {\n");
    for (func, _) in &samples {
        let _ = writeln!(out, "        {}();", super::common::camel_words(&func.name));
    }
    let _ = writeln!(
        out,
        "        System.out.println(\"ExamplesTest: \" + checks + \" checks, \" + failures + \" failures\");\n\
         \x20       System.exit(failures == 0 ? 0 : 1);\n\
         \x20   }}\n\
         }}"
    );
    out
}

#[allow(clippy::implicit_hasher)]
pub fn write(funcs: &[FuncDef], enums: &HashMap<String, EnumDef>, path: &Path) {
    if let Some(dir) = path.parent() {
        std::fs::create_dir_all(dir).unwrap_or_else(|e| panic!("creating {}: {e}", dir.display()));
    }
    super::write_if_changed_silent(path, &render_test(funcs, enums));
    println!("  Java examples test -> {}", path.display());
}
