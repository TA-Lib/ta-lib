//! The Go sample on each function page, and `example_test.go`, which holds every
//! one as a runnable `Example` with its `// Output:`, for `go test` and
//! pkg.go.dev.
//!
//! `generate` cannot run Go, so what each example prints is read from
//! [`LEDGER`], which `ta_codegen bless-go-examples` records from the package:
//! the first and last value of every output, which `// Output:` prints at four
//! decimals. A function the ledger lacks gets an example with no `// Output:`,
//! which `go test` compiles but does not run, and which
//! `build-libraries --backend=go` refuses.

use super::examples::{self, hidden, shown, Ends, Int, Shape, Snippet, LEN};
use super::go_fmt::{GExpr, GOp};
use crate::ir::{EnumDef, FuncDef, ParamType};
use std::collections::{BTreeMap, HashMap};
use std::fmt::Write as _;
use std::path::Path;
use std::sync::atomic::{AtomicBool, Ordering};
use std::sync::RwLock;

/// The recorded output of every example, relative to the repository root.
pub const LEDGER: &str = "ta_codegen/input/go_example_values.yaml";

/// The module path the examples import.
pub const MODULE: &str = "ta-lib.github.io/go";

const WRAP: usize = 100;

/// One example's output as the package printed it.
#[derive(Debug, Clone, PartialEq, serde::Deserialize, serde::Serialize)]
pub struct Recorded {
    pub beg: usize,
    pub count: usize,
    /// Element 0 of each output, in declaration order, as Go's shortest
    /// round-trip text.
    pub first: Vec<String>,
    /// Element `count - 1` of each output.
    pub last: Vec<String>,
}

pub type Ledger = BTreeMap<String, Recorded>;

static SEEDED: RwLock<Option<Ledger>> = RwLock::new(None);

static IGNORED: AtomicBool = AtomicBool::new(false);

/// Reads the ledger under `root`; a missing file is an empty ledger.
///
/// # Panics
/// On a ledger that does not parse.
#[must_use]
pub fn load(root: &Path) -> Ledger {
    let path = root.join(LEDGER);
    match std::fs::read_to_string(&path) {
        Ok(text) => serde_yaml::from_str(&text).unwrap_or_else(|e| panic!("{}: {e}", path.display())),
        Err(_) => Ledger::new(),
    }
}

/// Makes `root`'s ledger the one every sample reads. Unseeded, a sample reads
/// the generator crate's own tree.
pub fn seed(root: &Path) {
    let ledger = if IGNORED.load(Ordering::Relaxed) { Ledger::new() } else { load(root) };
    *SEEDED.write().expect("ledger") = Some(ledger);
}

/// While on, [`seed`] neither reads nor checks the ledger, and every function
/// reads as unrecorded: `bless-go-examples` replaces a ledger whatever it holds.
pub fn ignore_ledger(on: bool) {
    IGNORED.store(on, Ordering::Relaxed);
}

fn recorded(name: &str) -> Option<Recorded> {
    let mut guard = SEEDED.write().expect("ledger");
    let ledger = guard.get_or_insert_with(|| load(&Path::new(env!("CARGO_MANIFEST_DIR")).join("../..")));
    ledger.get(name).cloned()
}

/// How many values `name`'s example computes, when the ledger has its row.
#[must_use]
pub fn recorded_count(name: &str) -> Option<usize> {
    recorded(name).map(|r| r.count)
}

/// Names a sample's output may not take: Go's keywords and what the sample
/// itself calls or binds. A predeclared name it does not use (`min`) is fine.
const TAKEN: &[&str] = &["core", "err", "float64", "fmt", "i", "int32", "len", "log", "make", "math", "r", "talib"];

/// The variable of each output: `examples::output_vars`, with a trailing `_`
/// where the sample cannot bind it.
fn output_vars(func: &FuncDef) -> Vec<String> {
    examples::output_vars(func)
        .into_iter()
        .map(|v| if super::go::RESERVED_WORDS.contains(&v.as_str()) || TAKEN.contains(&v.as_str()) { format!("{v}_") } else { v })
        .collect()
}

fn lit(v: f64) -> GExpr {
    GExpr::name(super::doc_meta::fmt_real(v))
}

fn int(v: i64) -> GExpr {
    GExpr::name(v.to_string())
}

fn int_term(t: Int) -> GExpr {
    let i = || GExpr::name("i");
    match t {
        Int::Const(c) => int(c.abs()),
        Int::Linear(1) => i(),
        Int::Linear(c) => GExpr::bin(int(c), GOp::Mul, i()),
        Int::Mod(m) => GExpr::bin(i(), GOp::Rem, int(m)),
        Int::Wave { coef, period, shift } => {
            let at = if shift == 0 { i() } else { GExpr::bin(i(), GOp::Add, int(shift)).paren() };
            let k = || GExpr::bin(GExpr::bin(at.clone(), GOp::Rem, int(period)), GOp::Sub, int(period / 2));
            let square = GExpr::bin(k(), GOp::Mul, k());
            if coef.abs() == 1 { square } else { GExpr::bin(GExpr::bin(int(coef.abs()), GOp::Mul, k()), GOp::Mul, k()) }
        }
    }
}

#[allow(clippy::float_cmp)] // the scales are literals, not measurements
fn series_expr(s: &examples::Series) -> String {
    match &s.shape {
        Shape::Ramp { lo, hi } => {
            let bar = GExpr::call("float64", vec![GExpr::name("i")]);
            let ramp = GExpr::bin(
                GExpr::bin(lit(hi - lo), GOp::Mul, bar),
                GOp::Quo,
                GExpr::name(format!("{}.0", LEN - 1)),
            );
            if *lo == 0.0 { ramp } else { GExpr::bin(lit(*lo), GOp::Add, ramp) }
        }
        Shape::Waves { offset, scale, terms, div } => {
            let mut sum: Option<GExpr> = None;
            for t in terms {
                let negative = matches!(t, Int::Const(c) if *c < 0) || matches!(t, Int::Wave { coef, .. } if *coef < 0);
                sum = Some(match sum {
                    None if negative => GExpr::unary("-", int_term(*t)),
                    None => int_term(*t),
                    Some(acc) => GExpr::bin(acc, if negative { GOp::Sub } else { GOp::Add }, int_term(*t)),
                });
            }
            let mut value = GExpr::call("float64", vec![sum.expect("a series has a term")]);
            if *scale != 1.0 {
                value = GExpr::call("float64", vec![GExpr::bin(lit(*scale), GOp::Mul, value)]);
            }
            if *div != 1 {
                value = GExpr::bin(value, GOp::Quo, int(*div));
            }
            if *offset != 0.0 {
                value = GExpr::bin(lit(*offset), GOp::Add, value);
            }
            value
        }
    }
    .render()
}

fn sample(func: &FuncDef, enums: &HashMap<String, EnumDef>, bless: bool) -> Option<Snippet> {
    let series = examples::all_series(func)?;
    let first = series.first()?.var;
    let imports = ["import (", "\t\"fmt\"", "\t\"log\"", "", &format!("\ttalib \"{MODULE}\""), ")"]
        .iter()
        .map(|s| (*s).to_string())
        .collect();
    let mut body = Vec::new();
    for s in &series {
        body.push(shown(format!("{} := make([]float64, {LEN}) // {}", s.var, s.comment)));
        body.push(hidden(format!("for i := range {} {{", s.var)));
        body.push(hidden(format!("\t{}[i] = {}", s.var, series_expr(s))));
        body.push(hidden("}"));
    }
    let out_vars = output_vars(func);
    for (output, var) in func.outputs.iter().zip(&out_vars) {
        let ty = if output.param_type == ParamType::Integer { "int32" } else { "float64" };
        body.push(shown(format!("{var} := make([]{ty}, {LEN})")));
    }
    body.push(shown(""));
    body.push(shown("core := talib.NewCore()"));
    let mut args = vec!["0".to_string(), format!("len({first})-1")];
    args.extend(series.iter().map(|s| s.var.to_string()));
    args.extend(func.optional_inputs.iter().map(|opt| {
        examples::opt_literal(opt, enums, super::doc_meta::fmt_real, |e, v| format!("talib.{}{v}", e.name))
    }));
    let lead = format!("r, err := core.{}(", func.name);
    let one = format!("{lead}{}, {})", args.join(", "), out_vars.join(", "));
    if one.len() <= WRAP {
        body.push(shown(one));
    } else {
        body.push(shown(format!("{lead}{},", args.join(", "))));
        body.push(shown(format!("\t{})", out_vars.join(", "))));
    }
    body.extend(["if err != nil {", "\tlog.Fatal(err)", "}"].into_iter().map(shown));
    let at = |var: &str| [format!("{var}[0]"), format!("{var}[r.NBElement-1]")];
    if bless {
        let mut values = Vec::new();
        for (output, var) in func.outputs.iter().zip(&out_vars) {
            for v in at(var) {
                values.push(if output.param_type == ParamType::Integer {
                    v
                } else {
                    format!("strconv.FormatFloat({v}, 'g', -1, 64)")
                });
            }
        }
        body.push(hidden(format!("fmt.Println(\"{}\", r.BegIdx, r.NBElement, {})", func.name, values.join(", "))));
        return Some(Snippet { imports, body });
    }
    let ends = Ends::of(func);
    let list = |output: &crate::ir::Output, var: &str| {
        let spec = if output.param_type == ParamType::Integer { "%d" } else { "%.4f" };
        let [first, last] = at(var);
        (ends.list(spec, spec), ends.args(first, last).join(", "))
    };
    if let [output] = func.outputs.as_slice() {
        let (fmt, args) = list(output, &out_vars[0]);
        body.push(shown(format!("fmt.Printf(\"%d values from bar %d: {fmt}\\n\", r.NBElement, r.BegIdx, {args})")));
    } else {
        body.push(shown("fmt.Printf(\"%d values from bar %d:\\n\", r.NBElement, r.BegIdx)"));
        for (output, var) in func.outputs.iter().zip(&out_vars) {
            let (fmt, args) = list(output, var);
            body.push(shown(format!("fmt.Printf(\"{var} = {fmt}\\n\", {args})")));
        }
    }
    Some(Snippet { imports, body })
}

/// The sample a function page shows and its example runs.
#[allow(clippy::implicit_hasher)]
#[must_use]
pub fn example(func: &FuncDef, enums: &HashMap<String, EnumDef>) -> Option<Snippet> {
    sample(func, enums, false)
}

/// One recorded value as `// Output:` prints it.
///
/// # Errors
/// On a value that is not a finite number of the output's type.
fn printed(func: &FuncDef, output: usize, value: &str) -> Result<String, String> {
    let bad = |why: &str| format!("{}: output {} recorded `{value}`, {why}", func.name, func.outputs[output].name);
    if func.outputs[output].param_type == ParamType::Integer {
        return value.parse::<i64>().map(|v| v.to_string()).map_err(|_| bad("not an integer"));
    }
    match value.parse::<f64>() {
        Ok(v) if v.is_finite() => Ok(format!("{v:.4}")),
        Ok(_) => Err(bad("not finite: fix the example's input series")),
        Err(_) => Err(bad("not a number")),
    }
}

/// The lines of `func`'s `// Output:`, when the ledger has its row.
///
/// # Panics
/// On a row that does not hold one finite first and last value per output.
#[must_use]
pub fn expected_output(func: &FuncDef) -> Option<Vec<String>> {
    let rec = recorded(&func.name)?;
    let n = func.outputs.len();
    assert!(
        rec.first.len() == n && rec.last.len() == n,
        "{}: {LEDGER} records {} first and {} last values for {n} outputs; run `ta_codegen bless-go-examples`",
        func.name,
        rec.first.len(),
        rec.last.len()
    );
    let ends = Ends::of(func);
    let list = |k: usize| {
        let value = |v: &str| printed(func, k, v).unwrap_or_else(|e| panic!("{LEDGER}: {e}"));
        ends.list(&value(&rec.first[k]), &value(&rec.last[k]))
    };
    let head = format!("{} values from bar {}:", rec.count, rec.beg);
    if n == 1 {
        return Some(vec![format!("{head} {}", list(0))]);
    }
    let mut lines = vec![head];
    lines.extend(output_vars(func).iter().enumerate().map(|(k, var)| format!("{var} = {}", list(k))));
    Some(lines)
}

/// The example's name: `ExampleCore_SMA` attaches it to the method. `go vet`
/// reads each `_`-separated part after the method as a suffix that must start
/// lower-case, so a name holding `_` attaches to `Core` instead, under a
/// lower-case suffix that keeps only the `_` a letter follows.
#[must_use]
pub fn example_name(func_name: &str) -> String {
    if !func_name.contains('_') {
        return format!("ExampleCore_{func_name}");
    }
    let lower = func_name.to_lowercase();
    let mut suffix = String::new();
    let mut chars = lower.chars().peekable();
    while let Some(c) = chars.next() {
        if c != '_' || chars.peek().is_some_and(char::is_ascii_lowercase) {
            suffix.push(c);
        }
    }
    format!("ExampleCore_{suffix}")
}

fn file_head(package: &str) -> String {
    format!("{}\n\n{}package {package}\n", super::go::GENERATED_HEADER, super::ta_abstract_c::LICENSE)
}

fn sorted(funcs: &[FuncDef]) -> Vec<&FuncDef> {
    let mut funcs: Vec<&FuncDef> = funcs.iter().collect();
    funcs.sort_by(|a, b| a.name.cmp(&b.name));
    funcs
}

fn push_body(out: &mut String, snippet: &Snippet) {
    for line in snippet.body_text() {
        if line.is_empty() {
            out.push('\n');
        } else {
            let _ = writeln!(out, "\t{line}");
        }
    }
}

/// `example_test.go`: one `Example` per function, in the external test package.
#[allow(clippy::implicit_hasher)]
#[must_use]
pub fn render_test(funcs: &[FuncDef], enums: &HashMap<String, EnumDef>) -> String {
    let mut out = file_head("talib_test");
    let _ = write!(out, "\nimport (\n\t\"fmt\"\n\t\"log\"\n\n\ttalib \"{MODULE}\"\n)\n");
    for func in sorted(funcs) {
        let Some(snippet) = example(func, enums) else { continue };
        let _ = write!(out, "\nfunc {}() {{\n", example_name(&func.name));
        push_body(&mut out, &snippet);
        match expected_output(func).as_deref() {
            Some([line]) => {
                let _ = writeln!(out, "\t// Output: {line}");
            }
            Some(lines) => {
                out.push_str("\t// Output:\n");
                for line in lines {
                    let _ = writeln!(out, "\t// {line}");
                }
            }
            None => {}
        }
        out.push_str("}\n");
    }
    out
}

/// The program `bless-go-examples` runs: every sample, each printing its name,
/// range and the first and last value of each output in full.
#[allow(clippy::implicit_hasher)]
#[must_use]
pub fn render_bless_program(funcs: &[FuncDef], enums: &HashMap<String, EnumDef>) -> String {
    let mut out = format!(
        "package main\n\nimport (\n\t\"fmt\"\n\t\"log\"\n\t\"strconv\"\n\n\ttalib \"{MODULE}\"\n)\n\n\
         var _ = strconv.Itoa\n"
    );
    let mut calls = Vec::new();
    for (k, func) in sorted(funcs).into_iter().enumerate() {
        let Some(snippet) = sample(func, enums, true) else { continue };
        let _ = write!(out, "\nfunc sample{k}() {{\n");
        push_body(&mut out, &snippet);
        out.push_str("}\n");
        calls.push(format!("\tsample{k}()\n"));
    }
    out.push_str("\nfunc main() {\n");
    out.extend(calls);
    out.push_str("}\n");
    out
}

/// Parses the bless program's output into a ledger.
///
/// # Errors
/// On a line that is not `NAME BEG COUNT` then a first and last value per
/// output, or on a value that is not a finite number.
pub fn parse_bless_output(text: &str) -> Result<Ledger, String> {
    let mut ledger = Ledger::new();
    for line in text.lines().filter(|l| !l.trim().is_empty()) {
        let parts: Vec<&str> = line.split_whitespace().collect();
        let [name, beg, count, values @ ..] = parts.as_slice() else {
            return Err(format!("unexpected line `{line}`"));
        };
        if values.is_empty() || values.len() % 2 != 0 {
            return Err(format!("unexpected line `{line}`"));
        }
        if let Some(bad) = values.iter().find(|v| !v.parse::<f64>().is_ok_and(f64::is_finite)) {
            return Err(format!("{name}: the example printed `{bad}`, not a finite value: fix its input series"));
        }
        let num = |s: &str| s.parse::<usize>().map_err(|e| format!("`{line}`: {e}"));
        let pick = |k: usize| values.iter().skip(k).step_by(2).map(|v| (*v).to_string()).collect();
        ledger.insert((*name).to_string(), Recorded { beg: num(beg)?, count: num(count)?, first: pick(0), last: pick(1) });
    }
    Ok(ledger)
}

/// The ledger file's text.
#[must_use]
pub fn render_ledger(ledger: &Ledger) -> String {
    let mut s = String::from(
        "# The first and last value of each output of each Go example, recorded\n\
         # from the package by `ta_codegen bless-go-examples`. generate writes\n\
         # each example's `// Output:` from this file, at four decimals.\n",
    );
    let quoted = |v: &[String]| v.iter().map(|x| format!("\"{x}\"")).collect::<Vec<_>>().join(", ");
    for (name, r) in ledger {
        let _ = writeln!(
            s,
            "{name}: {{beg: {}, count: {}, first: [{}], last: [{}]}}",
            r.beg,
            r.count,
            quoted(&r.first),
            quoted(&r.last)
        );
    }
    s
}

#[allow(clippy::implicit_hasher)]
pub fn write(funcs: &[FuncDef], enums: &HashMap<String, EnumDef>, path: &Path) {
    let missing: Vec<&str> =
        sorted(funcs).iter().filter(|f| recorded(&f.name).is_none()).map(|f| f.name.as_str()).collect();
    if !missing.is_empty() && !IGNORED.load(Ordering::Relaxed) {
        println!(
            "  WARNING: no recorded Go example output for {}; run `ta_codegen bless-go-examples`",
            missing.join(", ")
        );
    }
    super::write_if_changed_silent(path, &render_test(funcs, enums));
    println!("  Go examples -> {}", path.display());
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn a_name_with_an_underscore_attaches_to_core() {
        assert_eq!(example_name("SMA"), "ExampleCore_SMA");
        assert_eq!(example_name("HT_DCPERIOD"), "ExampleCore_ht_dcperiod");
        assert_eq!(example_name("SWAK_2PHP"), "ExampleCore_swak2php");
    }

    #[test]
    fn the_bless_output_round_trips_through_the_ledger() {
        let ledger =
            parse_bless_output("SMA 29 223 98.5 99.12345678901234\nCDLDOJI 10 242 0 -100\nMACD 33 219 1 2 3 4 5 6\n").unwrap();
        assert_eq!(ledger["MACD"].first, ["1", "3", "5"]);
        assert_eq!(ledger["MACD"].last, ["2", "4", "6"]);
        let back: Ledger = serde_yaml::from_str(&render_ledger(&ledger)).unwrap();
        assert_eq!(back, ledger);
        assert!(parse_bless_output("SMA 29\n").is_err());
        assert!(parse_bless_output("SMA 29 223 1\n").is_err());
        let nan = parse_bless_output("SMA 29 223 NaN 1\n").unwrap_err();
        assert!(nan.starts_with("SMA: ") && nan.contains("`NaN`"), "{nan}");
        assert!(parse_bless_output("SMA 29 223 1 +Inf\n").is_err());
    }
}
