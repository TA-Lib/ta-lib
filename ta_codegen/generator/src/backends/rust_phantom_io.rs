//! Generate `src/ta_func/no_phantom_io.rs` — the Rust half of the phantom-I/O
//! probe.
//!
//! Negative-space coverage: array lengths chosen so that any access the contract
//! forbids is a panic rather than a comment. The Java suite of the same name
//! (`NoPhantomIoTest`) is the original; this ports the two of its four sweeps
//! that Rust can host.
//!
//! **What this reaches that the Java suite does not.** All three suites probe
//! `<N>_Impl` and all three reach every one of the 176 cores. That takes a
//! dispatch body carrying the "nothing to produce" guard every other composed
//! core has (#267); without it, routing a cross-call through the public callee
//! puts a composed core out of a sweep's reach.
//! What is left that is Rust's alone is the *Rust* emitter: a bug that changes
//! what one backend touches without changing what it produces is invisible to
//! `--xlang-hash` by construction, so each backend's phantom-I/O coverage is
//! only ever its own.
//!
//! **An in-crate `#[cfg(test)]` module calling `<N>_Impl` directly**, which is
//! what the Java suite does and what #265 made every probe do. It was an
//! integration test under `library/tests/` reaching the public tier, which works
//! only while that tier is a pure forwarder — and that reach is precisely what
//! kept Rust's argument contract stated as an `assert!` in the numerics instead
//! of a code from the public tier. The probe's subject is what the *body*
//! touches, so it names the body; the public tier's own contract is
//! `tests/nullable_outputs.rs`'s and `BatchApiTest`'s business.
//!
//! **Generated rather than hand-written**, unlike the Java suite. Java discovers
//! its corpus by reflection, so a new indicator is probed the day it lands.
//! Rust has no reflection, and the one dynamic binder it does have —
//! `abstract_api::ParamHolder` — validates its own arguments before dispatch,
//! which is the very guard the probe must get behind. A hand-written table of
//! 176 call sites would rot silently; generating it makes `regen-check` the
//! thing that keeps the corpus complete.

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
    let _ = writeln!(o, "        let Ok(lb) = core.{name}_Lookback({lookback_args}) else {{ continue; }};");
    // Control arm: one bar longer than the quiet range produces exactly one
    // value, so it must index an array; with zero-length arrays that is a panic.
    let _ = writeln!(o, "        r.control(\"{name}\", label, run(|| {{");
    let _ = write!(o, "{}", empty_call(func, &call_opts, "lb", 12));
    let _ = writeln!(o, "        }}));");
    let _ = writeln!(o, "        if lb < 1 {{ r.no_quiet_range(\"{name}\", label); continue; }}");
    let _ = writeln!(o, "        r.quiet(\"{name}\", label, lb, run(|| {{");
    let _ = write!(o, "{}", empty_call(func, &call_opts, "lb - 1", 12));
    let _ = writeln!(o, "        }}));");
    let _ = writeln!(o, "    }}");
    let _ = writeln!(o, "}}\n");

    // ---- sweep 2: declared legs ----
    let _ = writeln!(o, "fn legs_{name}(r: &mut Report) {{");
    let _ = writeln!(o, "    let core = Core::new();");
    if func.inputs.is_empty() {
        let _ = writeln!(o, "    r.no_legs(\"{name}\");");
        let _ = writeln!(o, "}}\n");
        return;
    }
    // The defaults vector alone; the leg question is about the body's shape,
    // not its parameters, and 176 x legs x vectors buys nothing over it.
    let first = &vecs[0];
    for (k, opt) in func.optional_inputs.iter().enumerate() {
        let _ = writeln!(o, "    let {} = {};", opt.name, first.values[k]);
    }
    let _ = writeln!(o, "    let Ok(lb) = core.{name}_Lookback({lookback_args}) else {{ r.no_legs(\"{name}\"); return; }};");
    let _ = writeln!(o, "    let (startIdx, endIdx) = (lb, lb + 4);");
    // Control arm first: the same call with EVERY leg correctly sized must succeed
    // and produce values. Without it "every leg tripped the preamble" is satisfiable
    // by a broken fixture -- a mis-sized `series`, a vector the lookback rejects --
    // and the sweep would read green while probing nothing.
    let _ = write!(o, "{}", sized_call_block(func, None));
    let _ = writeln!(o, "        r.legs_control(\"{name}\", run(|| {{");
    let _ = write!(o, "{}", impl_call(func, &call_opts, "startIdx", "endIdx", 12));
    let _ = writeln!(o, "        }}));\n    }}");
    for (leg, input) in func.inputs.iter().enumerate() {
        let _ = write!(o, "{}", sized_call_block(func, Some(&input.name)));
        let _ = writeln!(
            o,
            "        r.leg(\"{name}\", \"{}\", {leg}, run(|| {{",
            input.name
        );
        let _ = write!(o, "{}", impl_call(func, &call_opts, "startIdx", "endIdx", 12));
        let _ = writeln!(o, "        }}));\n    }}");
    }
    let _ = writeln!(o, "    r.legs_done(\"{name}\", {});", func.inputs.len());
    let _ = writeln!(o, "}}\n");
}

/// One `{ ... }` block binding every input to a correctly sized series and every
/// output to a five-slot buffer, less `hole`, which gets a zero-length one. The
/// caller closes the block with the `Report` call that judges it.
fn sized_call_block(func: &FuncDef, hole: Option<&str>) -> String {
    let mut s = String::from("    {\n");
    for input in &func.inputs {
        let ty = slice_ty(&input.param_type);
        if hole == Some(input.name.as_str()) {
            let _ = writeln!(s, "        let {}: Vec<{ty}> = Vec::with_capacity(1);", input.name);
        } else {
            let _ = writeln!(
                s,
                "        let {}: Vec<{ty}> = series(\"{}\", endIdx + 1);",
                input.name,
                leg_kind(&input.name)
            );
        }
    }
    for out in &func.outputs {
        let ty = slice_ty(&out.param_type);
        let _ = writeln!(s, "        let mut {}: Vec<{ty}> = vec![Default::default(); 5];", out.name);
    }
    s
}

/// The argument list of a `Core::NAME_Impl(...)` call, after `startIdx, endIdx`.
///
/// The numerics tier keeps C's shape, so the two out-params sit between the
/// optional parameters and the output slices.
fn call_args(func: &FuncDef, call_opts: &str) -> String {
    let mut parts: Vec<String> = func.inputs.iter().map(|i| format!("&{}", i.name)).collect();
    if !call_opts.is_empty() {
        parts.push(call_opts.trim_end_matches(", ").to_string());
    }
    parts.push("&mut _b".to_string());
    parts.push("&mut _n".to_string());
    for o in &func.outputs {
        // A nullable output takes `Option<&mut [T]>` (rule B6a). The sweep hands
        // it `Some(..)`: declining it would hide exactly the writes it is here
        // to catch.
        if o.is_nullable() {
            parts.push(format!("Some(&mut {})", o.name));
        } else {
            parts.push(format!("&mut {}", o.name));
        }
    }
    parts.join(", ")
}

/// One `<N>_Impl` call over `start_expr..=end_expr`, as the tail of a `run(||)`
/// closure: the two out-params, the call, and `(rc, count)` for the classifier.
fn impl_call(
    func: &FuncDef,
    call_opts: &str,
    start_expr: &str,
    end_expr: &str,
    indent: usize,
) -> String {
    let pad = " ".repeat(indent);
    let mut s = String::new();
    let _ = writeln!(s, "{pad}let mut _b: usize = 0;");
    let _ = writeln!(s, "{pad}let mut _n: usize = 0;");
    let _ = writeln!(
        s,
        "{pad}let rc = core.{}_Impl({start_expr}, {end_expr}, {});",
        func.name,
        call_args(func, call_opts)
    );
    let _ = writeln!(s, "{pad}(rc, _n)");
    s
}

/// A call with a zero-length array for every input and every output, over
/// `0..=end_expr`.
fn empty_call(func: &FuncDef, call_opts: &str, end_expr: &str, indent: usize) -> String {
    let pad = " ".repeat(indent);
    let mut s = String::new();
    // `with_capacity(1)`, not `Vec::new()`: the slices must be zero-LENGTH but
    // must not share an ADDRESS. Every unallocated Vec hands out the same
    // dangling aligned pointer, which a multi-output function's overlap guard
    // once read as aliased buffers -- silently costing this sweep all 14
    // multi-output indicators until the control arm said so. The guard now
    // excludes empty operands (rule B6, #262), so this is belt and braces: it
    // keeps the sweep independent of that guard's shape.
    for i in &func.inputs {
        let _ = writeln!(s, "{pad}let {}: Vec<{}> = Vec::with_capacity(1);", i.name, slice_ty(&i.param_type));
    }
    for o in &func.outputs {
        let _ = writeln!(s, "{pad}let mut {}: Vec<{}> = Vec::with_capacity(1);", o.name, slice_ty(&o.param_type));
    }
    s.push_str(&impl_call(func, call_opts, "0", end_expr, indent));
    s
}

/// Generate `ta_codegen/output/rust/library/src/ta_func/no_phantom_io.rs`.
///
/// In `src/`, not `tests/`: the sweep probes `<N>_Impl`, which is
/// `pub(crate)`. The generated `mod.rs` declares it `#[cfg(test)]`
/// (`RUST_GENERATED_TEST_MODULES` in `main.rs`) and the Rust backend's
/// `clean_keep` spares it from the stale-file sweep, which would otherwise see
/// a `.rs` in `src/ta_func/` that names no indicator.
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
        let _ = writeln!(o, "use crate::{ty};");
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

    let dir = out_base.join("rust/library/src/ta_func");
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
    // two tests racing to install it is a flake with no bug behind it.
    //
    // It silences EXACTLY the calls `run` wraps, and delegates everything else to
    // the hook installed before it. A bare `|_| {{}}` was enough while this lived
    // in its own integration-test binary; in-crate, libtest runs the other unit
    // tests concurrently in this same process, so it would have swallowed a
    // sibling's assertion message -- and any panic a probe raised OUTSIDE `run`,
    // which is a bug rather than a verdict. Not restored afterwards, because with
    // `IN_PROBE` false the delegating hook is exactly the one it replaced.
    let _ = PRIOR_HOOK.set(std::panic::take_hook());
    std::panic::set_hook(Box::new(|info| {{
        if !IN_PROBE.with(std::cell::Cell::get) {{
            if let Some(prior) = PRIOR_HOOK.get() {{
                prior(info);
            }}
        }}
    }}));

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

    // The corpus is the generator's, not a list kept by hand: a probe that
    // stopped being emitted is a shrinking sweep, which is the one way this
    // file can fail open.
    assert_eq!(PROBES.len(), {n}, "probe count");
    assert_eq!(
        PROBES.len(),
        crate::abstract_api::funcs().count(),
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
//! Negative-space coverage for the Rust numerics tier: array lengths chosen so
//! that any access the contract forbids is a panic rather than a comment. The
//! value gates can only see work that reaches an output, and work a function
//! does and then discards -- a read past `endIdx`, a write past the count it
//! reported, a leg it touches on a range where it promised to touch nothing --
//! leaves no trace for them to compare.
//!
//! It probes `<N>_Impl`, the crate-private body, which is why this is an
//! in-crate `#[cfg(test)]` module and not an integration test under `tests/`.
//!
//! Two sweeps, each preceded by a control arm on the same core, because
//! "nothing was touched" is also what a fixture that probes nothing reports:
//!
//! * **`sub_*`** -- a range shorter than the lookback, every input and output a
//!   zero-length slice. That call is a documented success with no values and
//!   must touch nothing; with no elements, every index is out of bounds. The
//!   body's `assert!` preamble short-circuits on such a range, so the slices
//!   reach the body itself and the only thing that can fire is a real access.
//!   Control: the same call one bar longer produces a value, so it must index
//!   an array, so on zero-length slices it must panic.
//! * **`legs_*`** -- one declared input zero-length, the rest correctly sized,
//!   over a range that produces values (below the lookback the preamble is off
//!   entirely, per the sweep above). Every leg must hit the preamble: it bounds
//!   every DECLARED input, not only the ones the body indexes, so "a declared
//!   input must be supplied" holds over the whole declaration here as it does
//!   over C's NULL checks. A leg that SUCCEEDS instead is a divergence -- the
//!   identical call is `TA_BAD_PARAM` in C. An index-out-of-bounds is the body
//!   reading past the bound it just stated, checked whatever the preamble says
//!   under `#![forbid(unsafe_code)]`, and is the same unbounded read in the C
//!   this input generates. Control: every leg correctly sized must succeed and
//!   produce values.
//!
//! This reads the shared C as the RUST emitter renders it. An emitter bug that
//! changes what one backend touches without changing what it produces is
//! invisible to `--xlang-hash` by construction, so each backend keeps its own.

use crate::{Core, RetCode};
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

/// The hook installed before this sweep's, so everything that is not one of its
/// own expected panics still reports normally.
static PRIOR_HOOK: std::sync::OnceLock<
    Box<dyn Fn(&std::panic::PanicHookInfo<'_>) + Sync + Send>,
> = std::sync::OnceLock::new();

thread_local! {
    /// True only inside [`run`]'s `catch_unwind`, where a panic is a verdict.
    static IN_PROBE: std::cell::Cell<bool> = const { std::cell::Cell::new(false) };
}

/// Run one call, classifying a panic by what produced it.
///
/// The closure hands back the numerics tier's own pair -- a `RetCode` and the
/// count it wrote into `outNBElement` -- rather than a `Result`, because that
/// is the shape `<N>_Impl` has.
fn run(f: impl FnOnce() -> (RetCode, usize)) -> Touch {
    IN_PROBE.with(|p| p.set(true));
    let outcome = std::panic::catch_unwind(std::panic::AssertUnwindSafe(f));
    IN_PROBE.with(|p| p.set(false));
    match outcome {
        Ok((RetCode::Success, count)) => Touch::Quiet(true, count),
        Ok((_, _)) => Touch::Quiet(false, 0),
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
/// on the fixture and a leg cannot read as untouched here but read there.
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
    unbounded: Vec<String>,
    legs_control: std::collections::BTreeSet<&'static str>,
    legs_swept: std::collections::BTreeSet<&'static str>,
    legs_none: usize,
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
            unbounded: Vec::new(),
            legs_control: std::collections::BTreeSet::new(),
            legs_swept: std::collections::BTreeSet::new(),
            legs_none: 0,
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
                if ok { "Success" } else { "a failure code" }
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
                "{name}[{label}] (lookback {lb}, endIdx {}) returned a failure code, expected a \
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

    /// A function the sweep cannot judge: no declared input, or a `_Lookback`
    /// that rejects its own defaults. Counted, not discarded -- `finish_legs`
    /// accounts for every probe, so a sweep that quietly stopped judging the
    /// corpus is a failure rather than a smaller run.
    fn no_legs(&mut self, name: &'static str) {
        let _ = name;
        self.legs_none += 1;
    }

    /// The per-function control arm: every leg correctly sized must succeed and
    /// produce values, so a `Bounded` verdict below is attributable to the one
    /// zero-length leg and not to the fixture.
    fn legs_control(&mut self, name: &'static str, t: Touch) {
        match t {
            Touch::Quiet(true, n) if n > 0 => {
                self.legs_control.insert(name);
            }
            Touch::Quiet(ok, n) => self.fail(format!(
                "{name} with every leg correctly sized returned {} ({n} value(s)); the leg sweep \
                 is probing a call that does nothing, so every rejection below proves nothing",
                if ok { "Success" } else { "a failure code" }
            )),
            Touch::Bounded => self.fail(format!(
                "{name} tripped its own bounds assert with every leg correctly sized: the \
                 preamble states a bound the fixture does not meet"
            )),
            Touch::Indexed => self.fail(format!(
                "{name} indexed out of bounds with every leg correctly sized"
            )),
            Touch::Other(msg) => {
                self.fail(format!("{name} control panicked with {msg}"));
            }
        }
    }

    /// One leg given a zero-length slice while the rest stay sized. Since #260
    /// the preamble bounds EVERY declared leg, so every one of these must be
    /// `Bounded`: a leg that sails through is the exemption coming back.
    fn leg(&mut self, name: &'static str, leg: &str, _index: usize, t: Touch) {
        self.legs_seen += 1;
        match t {
            Touch::Bounded => self.bounded.entry(name).or_default().push(leg.to_string()),
            Touch::Indexed => self.fail(format!(
                "{name}.{leg} was indexed with a zero-length slice, but the body's bounds-assert \
                 preamble does not cover it: the body reads past the bound the preamble states, \
                 so nothing bounds that read"
            )),
            Touch::Quiet(_, _) => {
                self.unbounded.push(format!("{name}.{leg}"));
                self.fail(format!(
                    "{name}.{leg} is a DECLARED input that a zero-length slice sails through: \
                     the bounds-assert preamble does not cover it, so the same call is \
                     TA_BAD_PARAM in C and a success here (#260)"
                ));
            }
            Touch::Other(msg) => self.fail(format!("{name}.{leg} panicked with {msg}")),
        }
    }

    fn legs_done(&mut self, name: &'static str, declared: usize) {
        self.funcs_with_legs += 1;
        self.legs_swept.insert(name);
        let bounded = self.bounded.get(name).map_or(0, Vec::len);
        if bounded != declared {
            self.fail(format!(
                "{name}: {bounded} of {declared} declared leg(s) are bounded by the preamble; \
                 every declared input must be, or a caller may omit one in Rust and not in C"
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
            self.fail("declared legs: the sweep ran no leg at all".to_string());
        }
        // Every core is accounted for: judged, or explicitly counted as having
        // nothing to judge.
        if self.legs_swept.len() + self.legs_none != PROBES.len() {
            self.fail(format!(
                "declared legs: every core is swept or counted ({} + {} vs {})",
                self.legs_swept.len(),
                self.legs_none,
                PROBES.len()
            ));
        }
        // Every function the sweep judged must have had its control arm fire, or
        // its verdicts are unattributable. Missing from the SWEPT set, not from
        // `bounded`: a function whose every leg sailed through has no `bounded`
        // entry, and filtering that would drop it from the list naming it.
        if self.legs_control.len() != self.funcs_with_legs {
            let missing: Vec<&str> = self
                .legs_swept
                .iter()
                .copied()
                .filter(|n| !self.legs_control.contains(n))
                .collect();
            self.fail(format!(
                "declared legs: the control arm fired for {} of {} function(s) swept; not \
                 proved {missing:?}",
                self.legs_control.len(),
                self.funcs_with_legs
            ));
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
            "  declared legs: {} leg(s) across {} function(s); {} bounded by the assert preamble, \
             {} unbounded{}; {} control(s) fired",
            self.legs_seen,
            self.funcs_with_legs,
            bounded_legs,
            self.unbounded.len(),
            if self.unbounded.is_empty() { String::new() } else { format!(" -> {:?}", self.unbounded) },
            self.legs_control.len()
        );
        for f in &self.failures {
            println!("  VIOLATION: {f}");
        }
    }
}

"#;
