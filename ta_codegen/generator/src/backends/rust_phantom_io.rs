//! Generate `tests/no_phantom_io.rs` — the Rust half of the phantom-I/O probe.
//!
//! Negative-space coverage: array lengths chosen so that any access the contract
//! forbids is a panic rather than a comment. The Java suite of the same name
//! (`NoPhantomIoTest`) is the original; this ports the two of its four sweeps
//! that Rust can host.
//!
//! **What this reaches that the Java suite does not.** Since #236 step 3 routed
//! cross-calls through the public callee, ten composed cores are out of the
//! Java sweep's reach and are withheld by name there. Rust's cross-calls still
//! target `<N>_Impl`, so it probes all 174. It also covers the *Rust*
//! emitter: a bug that changes what one backend touches without changing what
//! it produces is invisible to `--xlang-hash` by construction, so each
//! backend's phantom-I/O coverage is only ever its own.
//!
//! **Why Rust can host it.** Java and C# have a guarded public tier and an
//! unguarded body, and the probe has to pick the second or it measures the
//! guard. Rust has no such split: `pub fn SMA` is a thin `Result` mapper over
//! `SMA_Impl`, and the bounds check lives *in the body* as the `assert!`
//! preamble plus the indexing itself, under `#![forbid(unsafe_code)]`. The
//! public API reaches it directly.
//!
//! **Generated rather than hand-written**, unlike the Java suite. Java discovers
//! its corpus by reflection, so a new indicator is probed the day it lands.
//! Rust has no reflection, and the one dynamic binder it does have —
//! `abstract_api::ParamHolder` — validates output capacity before dispatch
//! (`if o0.len() < need { return Err(BadParam) }`), which is the very guard the
//! probe must get behind. A hand-written table of 174 call sites would rot
//! silently; generating it makes `regen-check` the thing that keeps the corpus
//! complete.

// An integer parameter's `range:` and `default:` are integers that the IR
// carries as `f64`, so narrowing them back is exact by construction -- the same
// reason `rust_abstract` allows it.
#![allow(clippy::cast_possible_truncation)]

use crate::ir::{EnumDef, FuncDef, OptInput, ParamType};
use std::collections::HashMap;
use std::fmt::Write as _;
use std::path::Path;

/// The fixture value for one bar of one leg, mirroring `NoPhantomIoTest.bar`.
/// The two suites must agree on the data or "this leg is read" can differ
/// between them for reasons that have nothing to do with the generated code.
fn leg_kind(input_name: &str) -> &'static str {
    match input_name {
        "inOpen" => "open",
        "inHigh" => "high",
        "inLow" => "low",
        "inClose" => "close",
        "inVolume" => "volume",
        "inPeriods" => "inPeriods",
        _ => "real",
    }
}

/// `&[f64]` / `&[i32]`, matching [`rust_lang`](super::rust_lang)'s signature emission.
fn slice_ty(t: &ParamType) -> &'static str {
    match t {
        ParamType::Real => "f64",
        ParamType::Integer | ParamType::Enum(_) | ParamType::Price(_) => "i32",
    }
}

/// The all-defaults literal for one optional parameter: the sentinel the
/// library substitutes from, exactly as `NoPhantomIoTest.defaultFor` does.
fn default_literal(opt: &OptInput) -> String {
    match &opt.param_type {
        ParamType::Real => "Core::REAL_DEFAULT".to_string(),
        ParamType::Enum(name) => format!("{name}::DEFAULT"),
        ParamType::Integer | ParamType::Price(_) => "i32::MIN".to_string(),
    }
}

/// The documented minimum for one optional parameter; enums stay at their default.
fn minimum_literal(opt: &OptInput) -> String {
    match (&opt.param_type, opt.range) {
        (ParamType::Real, Some((lo, _))) => format!("{lo:?}f64"),
        (ParamType::Integer, Some((lo, _))) => format!("{}i32", lo as i64),
        _ => default_literal(opt),
    }
}

/// `2 * default`, capped at the documented maximum — the "periods doubled" axis.
/// `None` when doubling does not move the value (so the vector would duplicate
/// the one it was cloned from).
fn doubled_literal(opt: &OptInput) -> Option<String> {
    let (ParamType::Integer, Some((_, hi)), Some(def)) = (&opt.param_type, opt.range, opt.default)
    else {
        return None;
    };
    let doubled = (2.0 * def).min(hi) as i64;
    (doubled > def as i64).then(|| format!("{doubled}i32"))
}

/// One candidate parameter vector: a label and one literal per optional input.
struct Vector {
    label: String,
    values: Vec<String>,
}

/// The star around the defaults, ported from `NoPhantomIoTest.vectors`.
///
/// One axis moves per vector, deliberately: the product of every enum against
/// every other is where the cost stops being worth it. Validity is not judged
/// here — the emitted code keeps a vector only if `NAME_Lookback` accepts it,
/// which it signals by returning `usize::MAX`.
fn vectors(func: &FuncDef, enums: &HashMap<String, EnumDef>) -> Vec<Vector> {
    let defaults: Vec<String> = func.optional_inputs.iter().map(default_literal).collect();
    let mut out = vec![Vector { label: "defaults".to_string(), values: defaults.clone() }];

    if !func.optional_inputs.is_empty() {
        out.push(Vector {
            label: "minimums".to_string(),
            values: func.optional_inputs.iter().map(minimum_literal).collect(),
        });
    }

    // Raised, because for a composed function the two axes interact: BBANDS
    // bails early only when the moving average's lookback is BELOW the
    // deviation's, which at MAType::MAMA (a constant lookback of 32) needs a
    // period above 33 -- a bar the default of 20 never reaches.
    let mut raised = defaults.clone();
    let mut any_raised = false;
    for (k, opt) in func.optional_inputs.iter().enumerate() {
        if let Some(lit) = doubled_literal(opt) {
            raised[k] = lit;
            any_raised = true;
        }
    }

    for (k, opt) in func.optional_inputs.iter().enumerate() {
        let ParamType::Enum(enum_name) = &opt.param_type else {
            continue;
        };
        let Some(def) = enums.get(enum_name) else {
            continue;
        };
        for variant in &def.variants {
            let member = format!("{enum_name}::{}", variant.name);
            let mut v = defaults.clone();
            v[k].clone_from(&member);
            out.push(Vector { label: format!("{}={}", opt.name, variant.name), values: v });
            if any_raised {
                let mut w = raised.clone();
                w[k] = member;
                out.push(Vector {
                    label: format!("{}={}, periods doubled", opt.name, variant.name),
                    values: w,
                });
            }
        }
    }
    out
}

/// Emit the two probe functions for one indicator.
fn emit_func(o: &mut String, func: &FuncDef, enums: &HashMap<String, EnumDef>) {
    let name = &func.name;
    let vecs = vectors(func, enums);
    let opt_args: Vec<&str> = func.optional_inputs.iter().map(|p| p.name.as_str()).collect();
    let opt_tys: Vec<String> = func
        .optional_inputs
        .iter()
        .map(|p| super::rust_lang::opt_param_type(&p.param_type))
        .collect();

    // The candidate vectors, as data. `NAME_Lookback` filters them at run time,
    // so an impossible combination drops out by the library's own rule.
    let tuple_ty = if opt_tys.is_empty() {
        "&str".to_string()
    } else {
        format!("(&str, {})", opt_tys.join(", "))
    };
    let _ = writeln!(o, "const V_{name}: &[{tuple_ty}] = &[");
    for v in &vecs {
        if v.values.is_empty() {
            let _ = writeln!(o, "    {:?},", v.label);
        } else {
            let _ = writeln!(o, "    ({:?}, {}),", v.label, v.values.join(", "));
        }
    }
    let _ = writeln!(o, "];\n");

    let destructure = if opt_tys.is_empty() {
        "label".to_string()
    } else {
        format!("(label, {})", opt_args.join(", "))
    };
    let lookback_args = opt_args.join(", ");
    let call_opts = if opt_args.is_empty() { String::new() } else { format!("{lookback_args}, ") };

    // ---- sweep 1: sub-lookback ----
    let _ = writeln!(o, "fn sub_{name}(r: &mut Report) {{");
    let _ = writeln!(o, "    let core = Core::new();");
    let _ = writeln!(o, "    for &{destructure} in V_{name} {{");
    let _ = writeln!(o, "        let lb = core.{name}_Lookback({lookback_args});");
    let _ = writeln!(o, "        if lb == usize::MAX {{ continue; }}");
    // Control arm: one bar longer than the quiet range produces exactly one
    // value, so it must index an array; with zero-length arrays that is a panic.
    let _ = writeln!(o, "        r.control(\"{name}\", label, run(|| {{");
    let _ = writeln!(o, "{}", empty_call(func, &call_opts, "lb", 12));
    let _ = writeln!(o, "        }}));");
    let _ = writeln!(o, "        if lb < 1 {{ r.no_quiet_range(\"{name}\", label); continue; }}");
    let _ = writeln!(o, "        r.quiet(\"{name}\", label, lb, run(|| {{");
    let _ = writeln!(o, "{}", empty_call(func, &call_opts, "lb - 1", 12));
    let _ = writeln!(o, "        }}));");
    let _ = writeln!(o, "    }}");
    let _ = writeln!(o, "}}\n");

    // ---- sweep 2: unread legs ----
    let _ = writeln!(o, "fn legs_{name}(r: &mut Report) {{");
    let _ = writeln!(o, "    let core = Core::new();");
    if func.inputs.is_empty() {
        let _ = writeln!(o, "    r.no_legs(\"{name}\");");
        let _ = writeln!(o, "}}\n");
        return;
    }
    // The defaults vector alone; the leg question is about the body's shape,
    // not its parameters, and 174 x legs x vectors buys nothing over it.
    let first = &vecs[0];
    for (k, opt) in func.optional_inputs.iter().enumerate() {
        let _ = writeln!(o, "    let {} = {};", opt.name, first.values[k]);
    }
    let _ = writeln!(o, "    let lb = core.{name}_Lookback({lookback_args});");
    let _ = writeln!(o, "    if lb == usize::MAX {{ r.no_legs(\"{name}\"); return; }}");
    let _ = writeln!(o, "    let (startIdx, endIdx) = (lb, lb + 4);");
    for (leg, input) in func.inputs.iter().enumerate() {
        let _ = writeln!(o, "    {{");
        for other in &func.inputs {
            let ty = slice_ty(&other.param_type);
            if other.name == input.name {
                let _ = writeln!(o, "        let {}: Vec<{ty}> = Vec::with_capacity(1);", other.name);
            } else {
                let _ = writeln!(
                    o,
                    "        let {}: Vec<{ty}> = series(\"{}\", endIdx + 1);",
                    other.name,
                    leg_kind(&other.name)
                );
            }
        }
        for out in &func.outputs {
            let ty = slice_ty(&out.param_type);
            let _ = writeln!(o, "        let mut {}: Vec<{ty}> = vec![Default::default(); 5];", out.name);
        }
        let args = call_args(func, &call_opts);
        let _ = writeln!(
            o,
            "        r.leg(\"{name}\", \"{}\", {leg}, run(|| core.{name}(startIdx, endIdx, {args})));",
            input.name
        );
        let _ = writeln!(o, "    }}");
    }
    let _ = writeln!(o, "    r.legs_done(\"{name}\", {});", func.inputs.len());
    let _ = writeln!(o, "}}\n");
}

/// The argument list of a `Core::NAME(...)` call, after `startIdx, endIdx`.
fn call_args(func: &FuncDef, call_opts: &str) -> String {
    let mut parts: Vec<String> = func.inputs.iter().map(|i| format!("&{}", i.name)).collect();
    if !call_opts.is_empty() {
        parts.push(call_opts.trim_end_matches(", ").to_string());
    }
    for o in &func.outputs {
        parts.push(format!("&mut {}", o.name));
    }
    parts.join(", ")
}

/// A call with a zero-length array for every input and every output, over
/// `0..=end_expr`.
fn empty_call(func: &FuncDef, call_opts: &str, end_expr: &str, indent: usize) -> String {
    let pad = " ".repeat(indent);
    let mut s = String::new();
    // `with_capacity(1)`, not `Vec::new()`: the slices must be zero-LENGTH but
    // must not share an ADDRESS. Every unallocated Vec hands out the same
    // dangling aligned pointer, and a multi-output function's overlap guard
    // (`outUpper.as_ptr() == outMiddle.as_ptr()`) reads that as aliased buffers
    // and answers BadParam before the body runs -- which silently cost this
    // sweep all 14 multi-output indicators until the control arm said so.
    for i in &func.inputs {
        let _ = writeln!(s, "{pad}let {}: Vec<{}> = Vec::with_capacity(1);", i.name, slice_ty(&i.param_type));
    }
    for o in &func.outputs {
        let _ = writeln!(s, "{pad}let mut {}: Vec<{}> = Vec::with_capacity(1);", o.name, slice_ty(&o.param_type));
    }
    let args = call_args(func, call_opts);
    let _ = write!(s, "{pad}core.{}(0, {end_expr}, {args})", func.name);
    s
}

/// Generate `ta_codegen/output/rust/library/tests/no_phantom_io.rs`.
#[allow(clippy::implicit_hasher)]
pub fn generate(funcs: &[FuncDef], enums: &HashMap<String, EnumDef>, out_base: &Path) {
    let mut sorted: Vec<&FuncDef> = funcs.iter().collect();
    sorted.sort_by(|a, b| a.name.cmp(&b.name));

    let mut o = String::new();
    o.push_str(PREAMBLE);

    // The enum types the vectors name.
    let mut used: Vec<&String> = funcs
        .iter()
        .flat_map(|f| f.optional_inputs.iter())
        .filter_map(|p| match &p.param_type {
            ParamType::Enum(n) => Some(n),
            _ => None,
        })
        .collect();
    used.sort_unstable();
    used.dedup();
    for ty in used {
        let _ = writeln!(o, "use ta_lib::{ty};");
    }
    o.push('\n');
    o.push_str(HARNESS);

    for f in &sorted {
        emit_func(&mut o, f, enums);
    }

    let _ = writeln!(o, "type Probe = fn(&mut Report);\n");
    let _ = writeln!(o, "/// Every indicator in the corpus, name-sorted, with its two probes.");
    let _ = writeln!(o, "const PROBES: &[(&str, Probe, Probe)] = &[");
    for f in &sorted {
        let _ = writeln!(o, "    (\"{0}\", sub_{0}, legs_{0}),", f.name);
    }
    let _ = writeln!(o, "];\n");
    let _ = write!(o, "{}", driver(sorted.len()));

    let dir = out_base.join("rust/library/tests");
    std::fs::create_dir_all(&dir).unwrap();
    super::write_if_changed(&dir.join("no_phantom_io.rs"), &o, "no_phantom_io.rs", sorted.len());
}

/// The `#[test]` entry point, with the corpus size baked in so a generation that
/// silently emitted fewer probes is a failure rather than a smaller green run.
fn driver(n: usize) -> String {
    format!(
        r#"#[test]
fn no_phantom_io() {{
    // One test, both sweeps, sequential: the panic hook is process-global, and
    // two tests racing to install and restore it is a flake with no bug behind it.
    let prior = std::panic::take_hook();
    std::panic::set_hook(Box::new(|_| {{}}));

    let mut r = Report::new();
    for (name, sub, _) in PROBES {{
        r.name = name;
        sub(&mut r);
    }}
    r.finish_sub_lookback();
    for (name, _, legs) in PROBES {{
        r.name = name;
        legs(&mut r);
    }}
    r.finish_legs();

    std::panic::set_hook(prior);

    // The corpus is the generator's, not a list kept by hand: a probe that
    // stopped being emitted is a shrinking sweep, which is the one way this
    // file can fail open.
    assert_eq!(PROBES.len(), {n}, "probe count");
    assert_eq!(
        PROBES.len(),
        ta_lib::abstract_api::funcs().count(),
        "every registered function has a probe"
    );

    r.print();
    assert!(r.failures.is_empty(), "{{}} phantom-I/O violation(s)", r.failures.len());
}}
"#
    )
}

const PREAMBLE: &str = r#"//! GENERATED by ta_codegen -- DO NOT EDIT.
//!
//! Negative-space coverage for the Rust batch API: array lengths chosen so that
//! any access the contract forbids is a panic rather than a comment.
//!
//! The value gates (`ta_regtest`, `--xlang-hash`, `--fuzz-064`) can only see
//! work that reaches an output. Work a function does and then discards -- a read
//! past `endIdx`, a write past the count it reported, a leg it touches on a
//! range where it promised to touch nothing -- leaves no trace in any output, so
//! nothing there can see it.
//!
//! # What each sweep can and cannot see
//!
//! **`sub_*` (sub-lookback).** A range strictly shorter than the lookback, with
//! a zero-length slice for every input and every output. That call is a
//! documented success with no values and must touch nothing; with no elements,
//! every index is out of bounds. This sweep is fully non-circular: on such a
//! range the body's `assert!` preamble short-circuits (`_assertStart > endIdx`
//! is true), so the zero-length slices reach the body itself and the only thing
//! that can fire is a real `inReal[i]` or `outReal[i]` access. It is the sweep
//! that caught APO, PPO and PVO computing their fast MA over the whole requested
//! range before discovering the range was too short for their own.
//!
//! Each vector also gets a control arm first: the same call one bar longer
//! produces exactly one value, so it must index an array, so with zero-length
//! slices it must panic. That turns "this core was silent" into "this core was
//! silent *and* the detector was working on it".
//!
//! **`legs_*` (unread legs).** One declared input at a time given a zero-length
//! slice while the rest stay correctly sized, over a range that does produce
//! values. Here the panic can come from two places, and the difference is the
//! whole point:
//!
//! * an **assertion failure** is the body's `assert!` preamble, whose leg set is
//!   `backends::common::indexed_input_names` -- the same computation that feeds
//!   Java's and C#'s argument checks. Asserting *that* against itself would be
//!   circular, so it is reported, not asserted on.
//! * an **index-out-of-bounds** is the body indexing a leg the preamble does not
//!   bound. Under `#![forbid(unsafe_code)]` that access is checked whatever the
//!   preamble says, which is what makes this reading the body rather than the
//!   declaration. It is a violation: in Rust it is a panic in shipped code, and
//!   in the C the same input generates, an unbounded read.
//!
//! So this sweep pins one direction of `indexed_input_names` -- that it covers
//! every leg the body actually indexes. The other direction (a leg listed but
//! never read) is not reachable from outside the crate: the preamble runs first
//! and pre-empts the observation. That is a false rejection, not a memory
//! error, and Java's suite does not distinguish it either.
//!
//! # Measured against the Java suite
//!
//! Run together on the same corpus, the two agree where they can:
//!
//! * **sub-lookback: zero delta.** 144 cores probed at the defaults vector, 30
//!   skipped for having no sub-lookback range (lookback 0), 174 detector
//!   controls fired -- the same three numbers in both, so Rust reaches every
//!   core Java reaches.
//! * **unread legs: Rust sees 7, Java sees 40, and the 7 are a subset.** The 33
//!   extra are legs Rust's preamble bounds but the body does not read at the
//!   default candle settings; the preamble pre-empts the observation, which is
//!   the direction described above as unreachable from outside the crate. Zero
//!   legs are unread here and read there, which is the direction that would be
//!   a real disagreement.
//!
//! # What is deliberately not here
//!
//! `exactExtentSweep` and `openAndFillSweep` from the Java suite are not ported.
//! The first passes the public guard by construction; the second is streaming,
//! which #236 does not touch.
//!
//! And this covers the shared C under `ta_codegen/input/` as the Rust emitter
//! renders it. It is not a substitute for the Java and C# suites: an emitter bug
//! that changes what ONE backend touches without changing what it produces is
//! invisible to `--xlang-hash` by construction, so each backend needs its own.

#![allow(non_snake_case)]
// SAREXT takes eight optional parameters, so its vector tuple is eight wide.
// Naming a type per arity would be eight aliases used once each.
#![allow(clippy::type_complexity)]

use ta_lib::Core;
"#;

const HARNESS: &str = r#"/// What one call did to the arrays it was given.
#[derive(Debug, PartialEq, Eq)]
enum Touch {
    /// Returned without touching an out-of-bounds element.
    Quiet(bool, usize),
    /// The body's `assert!` preamble fired: bounded by the declaration.
    Bounded,
    /// The body indexed out of bounds: it read or wrote the array itself.
    Indexed,
    /// A panic that is neither -- reported rather than classified away.
    Other(String),
}

/// Run one call, classifying a panic by what produced it.
fn run(f: impl FnOnce() -> Result<ta_lib::OutRange, ta_lib::RetCode>) -> Touch {
    match std::panic::catch_unwind(std::panic::AssertUnwindSafe(f)) {
        Ok(Ok(range)) => Touch::Quiet(true, range.count),
        Ok(Err(_)) => Touch::Quiet(false, 0),
        Err(e) => {
            let msg = e
                .downcast_ref::<String>()
                .map(String::as_str)
                .or_else(|| e.downcast_ref::<&str>().copied())
                .unwrap_or("<non-string panic>")
                .to_string();
            if msg.contains("index out of bounds") || msg.contains("out of range for slice") {
                Touch::Indexed
            } else if msg.contains("assertion failed") {
                Touch::Bounded
            } else {
                Touch::Other(msg)
            }
        }
    }
}

/// `len` bars of `leg`, mirroring `NoPhantomIoTest.bar` so the two suites agree
/// on the fixture and a leg cannot read as unread here but read there.
fn series<T: FromF64>(leg: &str, len: usize) -> Vec<T> {
    (0..len).map(|i| T::from_f64(bar(leg, i))).collect()
}

trait FromF64 {
    fn from_f64(v: f64) -> Self;
}
impl FromF64 for f64 {
    fn from_f64(v: f64) -> Self { v }
}
impl FromF64 for i32 {
    fn from_f64(v: f64) -> Self { v as i32 }
}

fn bar(leg: &str, i: usize) -> f64 {
    let x = i as f64;
    let base = 100.0 + 10.0 * (x / 7.0).sin() + 3.0 * (x / 3.0).cos();
    match leg {
        "open" => base - 0.5,
        "high" => base + 2.0,
        "low" => base - 2.0,
        "close" => base + 0.5,
        "volume" => 1000.0 + x,
        // MAVP's per-bar period: the one leg whose value sets how far back the
        // function reads.
        "inPeriods" => if i % 3 == 0 { 1.0e5 } else { 5.0 + (i % 7) as f64 },
        _ => base,
    }
}

/// Counters and failures for both sweeps.
struct Report {
    name: &'static str,
    failures: Vec<String>,
    probed: usize,
    no_quiet: usize,
    controls_live: std::collections::BTreeSet<&'static str>,
    quiet_cores: std::collections::BTreeSet<&'static str>,
    quiet_calls: usize,
    /// Per function: the legs whose empty slice stopped the call, and how.
    bounded: std::collections::BTreeMap<&'static str, Vec<String>>,
    unread: Vec<String>,
    legs_seen: usize,
    funcs_with_legs: usize,
}

impl Report {
    fn new() -> Self {
        Report {
            name: "",
            failures: Vec::new(),
            probed: 0,
            no_quiet: 0,
            controls_live: std::collections::BTreeSet::new(),
            quiet_cores: std::collections::BTreeSet::new(),
            quiet_calls: 0,
            bounded: std::collections::BTreeMap::new(),
            unread: Vec::new(),
            legs_seen: 0,
            funcs_with_legs: 0,
        }
    }

    fn fail(&mut self, what: String) {
        self.failures.push(what);
    }

    /// The per-vector control arm: a call that produces a value must index an
    /// array, so with zero-length slices it must panic.
    fn control(&mut self, name: &'static str, label: &str, t: Touch) {
        match t {
            Touch::Quiet(ok, n) => self.fail(format!(
                "{name}[{label}] at endIdx == lookback returned {} ({n} value(s)) without \
                 touching an array; a call that produces a value must index one, so this \
                 sweep could not detect I/O for it",
                if ok { "Ok" } else { "Err" }
            )),
            Touch::Other(msg) => {
                self.fail(format!("{name}[{label}] at endIdx == lookback panicked with {msg}"));
            }
            _ => {
                self.controls_live.insert(name);
            }
        }
    }

    fn no_quiet_range(&mut self, name: &'static str, label: &str) {
        if label == "defaults" {
            self.no_quiet += 1;
        }
        let _ = name;
    }

    /// The sweep proper: a sub-lookback range must be a success with no values
    /// and must touch neither array.
    fn quiet(&mut self, name: &'static str, label: &str, lb: usize, t: Touch) {
        if label == "defaults" {
            self.probed += 1;
        }
        self.quiet_calls += 1;
        match t {
            Touch::Quiet(true, 0) => {
                self.quiet_cores.insert(name);
            }
            Touch::Quiet(true, n) => self.fail(format!(
                "{name}[{label}] (lookback {lb}, endIdx {}) reported {n} value(s) on a \
                 sub-lookback range",
                lb - 1
            )),
            Touch::Quiet(false, _) => self.fail(format!(
                "{name}[{label}] (lookback {lb}, endIdx {}) returned Err, expected a \
                 success with no values",
                lb - 1
            )),
            Touch::Bounded => self.fail(format!(
                "{name}[{label}] (lookback {lb}, endIdx {}) tripped its own bounds assert on a \
                 range it promised to touch nothing on",
                lb - 1
            )),
            Touch::Indexed => self.fail(format!(
                "{name}[{label}] (lookback {lb}, endIdx {}) INDEXED an array on a sub-lookback \
                 range: it read an input or wrote an output the contract says it must not touch",
                lb - 1
            )),
            Touch::Other(msg) => self.fail(format!("{name}[{label}] panicked with {msg}")),
        }
    }

    fn no_legs(&mut self, name: &'static str) {
        let _ = name;
    }

    /// One leg given a zero-length slice while the rest stay sized.
    fn leg(&mut self, name: &'static str, leg: &str, _index: usize, t: Touch) {
        self.legs_seen += 1;
        match t {
            Touch::Bounded => self.bounded.entry(name).or_default().push(leg.to_string()),
            Touch::Indexed => self.fail(format!(
                "{name}.{leg} was indexed with a zero-length slice, but the body's bounds-assert \
                 preamble does not cover it: the body reads a leg the declaration says it does \
                 not, so nothing bounds that read"
            )),
            Touch::Quiet(_, _) => self.unread.push(format!("{name}.{leg}")),
            Touch::Other(msg) => self.fail(format!("{name}.{leg} panicked with {msg}")),
        }
    }

    fn legs_done(&mut self, name: &'static str, declared: usize) {
        self.funcs_with_legs += 1;
        if !self.bounded.contains_key(name) {
            self.fail(format!(
                "{name} reads none of its {declared} declared leg(s): a function that reads no \
                 input has stopped being an indicator"
            ));
        }
    }

    fn finish_sub_lookback(&mut self) {
        // Every core is accounted for at the defaults vector: probed, or
        // explicitly counted as having no sub-lookback range.
        if self.probed + self.no_quiet != PROBES.len() {
            self.fail(format!(
                "sub-lookback: every core is probed or counted ({} + {} vs {})",
                self.probed,
                self.no_quiet,
                PROBES.len()
            ));
        }
        if self.probed == 0 || self.no_quiet == 0 {
            self.fail(format!(
                "sub-lookback: both outcomes must occur, so neither branch is dead ({} probed, \
                 {} with no sub-lookback range)",
                self.probed, self.no_quiet
            ));
        }
        // The detector is proved live per core, not once on SMA.
        if self.controls_live.len() != PROBES.len() {
            let missing: Vec<&str> = PROBES
                .iter()
                .map(|p| p.0)
                .filter(|n| !self.controls_live.contains(n))
                .collect();
            self.fail(format!(
                "sub-lookback: the detector is proved live for every core ({} of {}; not proved \
                 {missing:?})",
                self.controls_live.len(),
                PROBES.len()
            ));
        }
    }

    fn finish_legs(&mut self) {
        if self.funcs_with_legs == 0 || self.legs_seen == 0 {
            self.fail("unread legs: the sweep ran no leg at all".to_string());
        }
    }

    fn print(&self) {
        println!(
            "  sub-lookback: {} core(s) probed at defaults, {} skipped (lookback 0, no \
             sub-lookback range exists), {} quiet call(s) across all vectors; {} detector \
             control(s) fired",
            self.probed,
            self.no_quiet,
            self.quiet_calls,
            self.controls_live.len()
        );
        let bounded_legs: usize = self.bounded.values().map(Vec::len).sum();
        println!(
            "  unread legs: {} leg(s) across {} function(s); {} bounded by the assert preamble, \
             {} declared but never indexed{}",
            self.legs_seen,
            self.funcs_with_legs,
            bounded_legs,
            self.unread.len(),
            if self.unread.is_empty() { String::new() } else { format!(" -> {:?}", self.unread) }
        );
        for f in &self.failures {
            println!("  VIOLATION: {f}");
        }
    }
}

"#;
