//! The Go sample on each function page, and `example_test.go`, which holds every
//! one as a runnable `Example` with its `// Output:`, for `go test` and
//! pkg.go.dev.
//!
//! `generate` cannot run Go, so what each example prints is read from
//! [`LEDGER`], which `ta_codegen bless-go-examples` records from the package.
//! The printed value takes the most decimals, up to four, that leave it further
//! than [`MARGIN`] from a rounding boundary, so a last-bit difference between
//! machines or Go versions cannot change the text; a value with no such count
//! prints the range alone. A function the ledger lacks gets an example with no
//! `// Output:`, which `go test` compiles but does not run, and which
//! `build-libraries --backend=go` refuses.

use super::examples::{self, hidden, shown, Snippet, Term, LEN};
use super::go_fmt::{GExpr, GOp};
use crate::ir::{EnumDef, FuncDef, ParamType};
use std::collections::{BTreeMap, HashMap};
use std::fmt::Write as _;
use std::path::Path;
use std::sync::RwLock;

/// The recorded output of every example, relative to the repository root.
pub const LEDGER: &str = "ta_codegen/input/go_example_values.yaml";

/// The module path the examples import.
pub const MODULE: &str = "ta-lib.github.io/go";

/// Relative distance a printed value keeps from a rounding boundary: the
/// largest difference between GOARCH, GOAMD64 and Go versions measured on these
/// samples is under 1e-11.
const MARGIN: f64 = 1e-9;

const MAX_DECIMALS: usize = 4;

const WRAP: usize = 100;

/// One example's output as the package printed it.
#[derive(Debug, Clone, PartialEq, serde::Deserialize, serde::Serialize)]
pub struct Recorded {
    pub beg: usize,
    pub count: usize,
    /// The last value of the first output, as Go's shortest round-trip text.
    pub last: String,
}

pub type Ledger = BTreeMap<String, Recorded>;

static SEEDED: RwLock<Option<Ledger>> = RwLock::new(None);

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
    *SEEDED.write().expect("ledger") = Some(load(root));
}

fn recorded(name: &str) -> Option<Recorded> {
    let mut guard = SEEDED.write().expect("ledger");
    let ledger = guard.get_or_insert_with(|| load(&Path::new(env!("CARGO_MANIFEST_DIR")).join("../..")));
    ledger.get(name).cloned()
}

/// Decimals that print `v` the same on every machine, or `None`.
#[must_use]
pub fn stable_decimals(v: f64) -> Option<usize> {
    if !v.is_finite() {
        return None;
    }
    let margin = MARGIN * v.abs().max(1.0);
    if v.abs() <= margin {
        return None; // the sign of zero is noise
    }
    (0..=MAX_DECIMALS).rev().find(|&d| {
        let scale = 10f64.powi(i32::try_from(d).expect("small"));
        let x = v.abs() * scale;
        ((x - x.floor()) - 0.5).abs() / scale > margin
    })
}

/// How the sample prints its result.
#[derive(Debug, Clone, Copy, PartialEq)]
enum Print {
    /// The last value with this many decimals.
    Decimals(usize),
    /// The last value of an integer output.
    Integer,
    /// The range alone.
    Range,
    /// A line `bless-go-examples` parses.
    Bless,
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

fn bar() -> GExpr {
    GExpr::call("float64", vec![GExpr::name("i")])
}

#[allow(clippy::float_cmp)] // the amplitudes are literals, not measurements
fn term(t: &Term) -> GExpr {
    match *t {
        Term::Const(c) => lit(c),
        Term::Linear(c) => GExpr::bin(lit(c), GOp::Mul, bar()),
        Term::Sin { amp, freq, phase } => {
            let mut arg = GExpr::bin(lit(freq), GOp::Mul, bar());
            if phase != 0.0 {
                arg = GExpr::bin(arg, if phase < 0.0 { GOp::Sub } else { GOp::Add }, lit(phase.abs()));
            }
            let s = GExpr::call("math.Sin", vec![arg]);
            if amp == 1.0 { s } else { GExpr::bin(lit(amp), GOp::Mul, s) }
        }
        Term::Mod(m) => GExpr::call(
            "float64",
            vec![GExpr::bin(GExpr::name("i"), GOp::Rem, GExpr::name(m.to_string()))],
        ),
        Term::Ramp { lo, hi } => {
            let ramp = GExpr::bin(
                GExpr::bin(lit(hi - lo), GOp::Mul, bar()),
                GOp::Quo,
                GExpr::name(format!("{}.0", LEN - 1)),
            );
            if lo == 0.0 { ramp } else { GExpr::bin(lit(lo), GOp::Add, ramp) }
        }
    }
}

fn series_expr(s: &examples::Series) -> String {
    let mut terms = s.terms.iter().map(term);
    let first = terms.next().expect("a series has a term");
    terms.fold(first, |acc, t| GExpr::bin(acc, GOp::Add, t)).render()
}

fn print_mode(func: &FuncDef) -> (Print, Option<Recorded>) {
    let rec = recorded(&func.name);
    if func.outputs[0].param_type == ParamType::Integer {
        return (Print::Integer, rec);
    }
    let mode = match &rec {
        Some(r) => r.last.parse::<f64>().ok().and_then(stable_decimals).map_or(Print::Range, Print::Decimals),
        None => Print::Decimals(MAX_DECIMALS),
    };
    (mode, rec)
}

fn sample(func: &FuncDef, enums: &HashMap<String, EnumDef>, print: Print) -> Option<Snippet> {
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
    let printed = &out_vars[0];
    let last = format!("{printed}[r.NBElement-1]");
    body.push(match print {
        Print::Decimals(d) => shown(format!(
            "fmt.Printf(\"%d values from bar %d; last {printed} = %.{d}f\\n\", r.NBElement, r.BegIdx, {last})"
        )),
        Print::Integer => shown(format!(
            "fmt.Printf(\"%d values from bar %d; last {printed} = %d\\n\", r.NBElement, r.BegIdx, {last})"
        )),
        Print::Range => shown("fmt.Printf(\"%d values from bar %d\\n\", r.NBElement, r.BegIdx)".to_string()),
        Print::Bless if func.outputs[0].param_type == ParamType::Integer => {
            hidden(format!("fmt.Println(\"{}\", r.BegIdx, r.NBElement, {last})", func.name))
        }
        Print::Bless => hidden(format!(
            "fmt.Println(\"{}\", r.BegIdx, r.NBElement, strconv.FormatFloat({last}, 'g', -1, 64))",
            func.name
        )),
    });
    Some(Snippet { imports, body })
}

/// The sample a function page shows and its example runs.
#[allow(clippy::implicit_hasher)]
#[must_use]
pub fn example(func: &FuncDef, enums: &HashMap<String, EnumDef>) -> Option<Snippet> {
    sample(func, enums, print_mode(func).0)
}

/// The `// Output:` of `func`'s example, when the ledger has its row.
#[must_use]
pub fn expected_output(func: &FuncDef) -> Option<String> {
    let (print, rec) = print_mode(func);
    let rec = rec?;
    let printed = &output_vars(func)[0];
    let head = format!("{} values from bar {}", rec.count, rec.beg);
    Some(match print {
        Print::Decimals(d) => format!("{head}; last {printed} = {:.d$}", rec.last.parse::<f64>().ok()?),
        Print::Integer => format!("{head}; last {printed} = {}", rec.last),
        Print::Range | Print::Bless => head,
    })
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
    let _ = write!(out, "\nimport (\n\t\"fmt\"\n\t\"log\"\n\t\"math\"\n\n\ttalib \"{MODULE}\"\n)\n");
    for func in sorted(funcs) {
        let Some(snippet) = example(func, enums) else { continue };
        let _ = write!(out, "\nfunc {}() {{\n", example_name(&func.name));
        push_body(&mut out, &snippet);
        if let Some(expected) = expected_output(func) {
            let _ = writeln!(out, "\t// Output: {expected}");
        }
        out.push_str("}\n");
    }
    out
}

/// The program `bless-go-examples` runs: every sample, each printing its name,
/// range and last value in full.
#[allow(clippy::implicit_hasher)]
#[must_use]
pub fn render_bless_program(funcs: &[FuncDef], enums: &HashMap<String, EnumDef>) -> String {
    let mut out = format!(
        "package main\n\nimport (\n\t\"fmt\"\n\t\"log\"\n\t\"math\"\n\t\"strconv\"\n\n\ttalib \"{MODULE}\"\n)\n\n\
         var _ = math.Sin\nvar _ = strconv.Itoa\n"
    );
    let mut calls = Vec::new();
    for (k, func) in sorted(funcs).into_iter().enumerate() {
        let Some(snippet) = sample(func, enums, Print::Bless) else { continue };
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
/// On a line that is not `NAME BEG COUNT LAST`.
pub fn parse_bless_output(text: &str) -> Result<Ledger, String> {
    let mut ledger = Ledger::new();
    for line in text.lines().filter(|l| !l.trim().is_empty()) {
        let parts: Vec<&str> = line.split_whitespace().collect();
        let [name, beg, count, last] = parts.as_slice() else {
            return Err(format!("unexpected line `{line}`"));
        };
        let num = |s: &str| s.parse::<usize>().map_err(|e| format!("`{line}`: {e}"));
        ledger.insert((*name).to_string(), Recorded { beg: num(beg)?, count: num(count)?, last: (*last).to_string() });
    }
    Ok(ledger)
}

/// The ledger file's text.
#[must_use]
pub fn render_ledger(ledger: &Ledger) -> String {
    let mut s = String::from(
        "# What each Go example prints, recorded from the package by\n\
         # `ta_codegen bless-go-examples`. generate writes each example's\n\
         # `// Output:` and the decimals it prints from this file.\n",
    );
    for (name, r) in ledger {
        let _ = writeln!(s, "{name}: {{beg: {}, count: {}, last: \"{}\"}}", r.beg, r.count, r.last);
    }
    s
}

#[allow(clippy::implicit_hasher)]
pub fn write(funcs: &[FuncDef], enums: &HashMap<String, EnumDef>, path: &Path) {
    let missing: Vec<&str> =
        sorted(funcs).iter().filter(|f| recorded(&f.name).is_none()).map(|f| f.name.as_str()).collect();
    if !missing.is_empty() {
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
    fn decimals_back_off_from_a_rounding_boundary() {
        assert_eq!(stable_decimals(99.123_44), Some(4));
        assert_eq!(stable_decimals(0.123_45), Some(3), "4 decimals sit on the boundary");
        assert_eq!(stable_decimals(0.123_450_000_000_1), Some(3), "inside the margin");
        assert_eq!(stable_decimals(1e-12), None, "the sign of zero is noise");
        assert_eq!(stable_decimals(-0.000_03), Some(4), "a sign far from zero is stable");
        assert_eq!(stable_decimals(f64::NAN), None);
        assert_eq!(stable_decimals(f64::INFINITY), None);
    }

    #[test]
    fn a_name_with_an_underscore_attaches_to_core() {
        assert_eq!(example_name("SMA"), "ExampleCore_SMA");
        assert_eq!(example_name("HT_DCPERIOD"), "ExampleCore_ht_dcperiod");
        assert_eq!(example_name("SWAK_2PHP"), "ExampleCore_swak2php");
    }

    #[test]
    fn the_bless_output_round_trips_through_the_ledger() {
        let ledger = parse_bless_output("SMA 29 223 99.12345678901234\nCDLDOJI 10 242 -100\n").unwrap();
        let back: Ledger = serde_yaml::from_str(&render_ledger(&ledger)).unwrap();
        assert_eq!(back, ledger);
        assert!(parse_bless_output("SMA 29\n").is_err());
    }
}
