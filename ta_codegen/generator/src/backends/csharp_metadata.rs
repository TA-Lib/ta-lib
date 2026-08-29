//! The **shipped** C# introspection registry — `TALib.Metadata`.
//!
//! C#'s analog of Rust's generated `abstract_api.rs` and Java's
//! `io.github.talib.metadata`: the catalogue of every indicator, for an
//! application that picks a function at run time. Rendered from
//! [`abstract_rows`](super::abstract_rows), the same rows those two render, so
//! the three cannot disagree.
//!
//! ## Why the server has no table of its own
//!
//! `TaCodegenServe.csproj` compiles `../library/src/**/*.cs` directly, so the
//! JSON-RPC server answers the `ta_abstract` metadata RPCs **out of the shipped
//! catalogue**. `test_abstract.c` therefore proves the shipped artifact against
//! the C library, not a test-only copy of it. Java's server still carries a
//! second, inline table, and that duplicate had silently omitted the
//! per-parameter `hint` for as long as nothing compared it — the failure mode
//! this shape avoids by construction. (The hint is compared now, and Java's
//! table carries it, so the specific instance is closed; the shape that allowed
//! it is not.)
//!
//! ## Shape notes
//!
//! * Every constructor is `internal`. That is what closes the
//!   `default(ImmutableArray<T>)` hazard structurally: no caller outside the
//!   assembly can build a `FunctionInfo`, so an uninitialised collection field
//!   is unreachable rather than merely undocumented.
//! * The catalogue is an *instance* singleton (`FunctionCatalog.Default`)
//!   because a `static class` cannot have an indexer, and `catalog["SMA"]` is
//!   the shape a C# consumer expects.
//! * Dispatch is a pair of delegate thunks emitted **inside the same expression
//!   as the parameter lists they index**, not a name-keyed `switch` in a
//!   separate file. Slot agreement is then adjacency rather than a second
//!   derivation — the class of bug Java's separate `Dispatch.java` can have.
//! * Doc comments are required: the shipped csproj sets
//!   `GenerateDocumentationFile` with `TreatWarningsAsErrors`, so a missing
//!   `<summary>` on a public member is a build error. The 168 rows live in
//!   `private static` factories and so cost none.

use std::collections::HashMap;
use std::fmt::Write as _;
use std::path::Path;

use super::abstract_rows::{
    func_flag_bits, opt_flag_bits, output_flag_bits, rows, FuncRow, Group, InputKind, InputRow,
    OptDomain, OptRow, OutputKind, OutputRow,
};
use super::csharp_doc::xml_escape_raw;
use crate::ir::{EnumDef, FuncDef, ParamType, PriceComponent};

/// Namespace (and directory) the registry is emitted into.
const NAMESPACE: &str = "TALib.Metadata";

/// Every file this module emits. Anything else ending in `.g.cs` in the output
/// directory is a stale orphan and gets swept.
const EMITTED: &[&str] = &[
    "Vocabulary.g.cs",
    "Model.g.cs",
    "FunctionCall.g.cs",
    "FunctionCatalog.g.cs",
    "CatalogFacts.g.cs",
    "FunctionDescription.g.cs",
];

/// Generate the whole `TALib.Metadata` namespace into `dir`
/// (`.../csharp/library/src/metadata`), plus the phantom-I/O probe's binder
/// into the test project.
#[allow(clippy::implicit_hasher)]
pub fn generate(funcs: &[FuncDef], enums: &HashMap<String, EnumDef>, dir: &Path) {
    std::fs::create_dir_all(dir).unwrap_or_else(|e| panic!("creating {}: {e}", dir.display()));

    let rows = rows(funcs, enums);
    let by_name: HashMap<&str, &FuncDef> = funcs.iter().map(|f| (f.name.as_str(), f)).collect();

    // This directory is 100% generated, and the csproj globs `src/**/*.cs`, so an
    // orphan left by a rename would still compile into the shipped assembly.
    // Sweep anything we no longer emit. (Renaming a file that declares a type
    // would surface as CS0101 eventually, but a file whose types moved elsewhere
    // would simply keep winning, silently.)
    clean_stale(dir, EMITTED);

    write(dir, "Vocabulary.g.cs", &vocabulary(&rows));
    write(dir, "Model.g.cs", &model());
    write(dir, "FunctionCall.g.cs", &function_call());
    write(dir, "FunctionCatalog.g.cs", &catalog(&rows, &by_name));
    write(dir, "CatalogFacts.g.cs", &catalog_facts(&rows));
    write(dir, "FunctionDescription.g.cs", &function_description(funcs));

    println!("  C# metadata registry -> {} ({} functions)", dir.display(), rows.len());

    // The phantom-I/O probe's `<N>_Impl` binder, into the TEST project (#265).
    // Not shipped, so it does not shape the public API the way the catalogue's
    // own thunk did; generated rather than hand-written, because reflection
    // cannot pass a `ReadOnlySpan<double>` and a table of 176 call sites kept by
    // hand cannot track a corpus that changes -- `scripts/synth_gate.py` adds
    // eleven functions and the suite went red on all of them.
    let test_dir = dir
        .parent()
        .and_then(Path::parent)
        .expect("metadata dir sits under csharp/library/src")
        .join("test");
    std::fs::create_dir_all(&test_dir)
        .unwrap_or_else(|e| panic!("creating {}: {e}", test_dir.display()));
    super::write_if_changed_silent(
        &test_dir.join("NoPhantomIoBinder.g.cs"),
        &phantom_io_binder(&rows, &by_name),
    );
    println!("  C# phantom-I/O binder -> {} ({} functions)", test_dir.display(), rows.len());
}

/// The phantom-I/O probe's own binder: one `<N>_Impl` call site per function.
///
/// **Why the probe needs one at all.** Its subject is what a *body* touches, so
/// it must reach the numerics tier — and it must do so without borrowing the
/// shipped metadata API's call path, or a test's reach decides which tier that
/// API calls (#265).
///
/// **Why it cannot use reflection.** A generated `<N>_Impl` takes
/// `ReadOnlySpan<double>` and `Span<double>`; a ref struct cannot be boxed, so
/// `MethodInfo.Invoke` cannot pass one. Java's probe discovers its corpus by
/// reflection and Rust's is generated; C#'s has to be written out.
///
/// **Why generated rather than hand-written.** The corpus is not fixed —
/// `scripts/synth_gate.py` swaps the `SYNTH*` fixtures into `ta_codegen/input/`
/// and regenerates — so a hand-kept table covers none of them and fails the
/// probe's own completeness check. Emitting it makes `regen-check` what keeps
/// the corpus complete, the same argument `rust_phantom_io` makes for the Rust
/// sweep.
///
/// It lands in the TEST project, not `src/`, so it is not shipped and cannot
/// shape the public API.
fn phantom_io_binder(rows: &[FuncRow], by_name: &HashMap<&str, &FuncDef>) -> String {
    let mut s = header();
    s.push_str(
        r#"
using System;
using System.Collections.Generic;
using TALib;
using TALib.Metadata;

namespace TALib.Test;

/// <summary>
/// <c>NoPhantomIoTest</c>'s own binder: one call site per function, each naming
/// <c>NAME_Impl</c> — the transcribed numerics and nothing above them.
/// </summary>
/// <remarks>
/// <para>The probe's subject is what a <i>body</i> touches, so it names the
/// body — and it brings its own call site rather than borrowing
/// <see cref="FunctionCall.TryInvoke"/>, whose thunks call the public entry
/// point like C's frames and Java's Dispatch. Sharing one would make a test's
/// reach decide which tier the shipped metadata API calls (issue #265).</para>
///
/// <para>Reflection cannot substitute: a generated <c>NAME_Impl</c> takes
/// <c>ReadOnlySpan&lt;double&gt;</c>, and a ref struct cannot be boxed for
/// <c>MethodInfo.Invoke</c>. Buffers and parameters still come from
/// <see cref="FunctionCall"/> — the probe binds them through the public setters
/// and reads them back through the same internal accessors the catalogue's
/// thunks use. Only the call itself is local.</para>
/// </remarks>
internal static class NoPhantomIoBinder
{
    /// <summary>What one numerics call produced: the code and the range.</summary>
    /// <remarks>The probe's own, because the numerics tier answers a code and the
    /// shipped <see cref="InvokeThunk"/> does not — its thunks call the public
    /// overload, which throws.</remarks>
    internal readonly record struct CallOutcome(RetCode Code, int BegIdx, int Count);

    internal delegate CallOutcome Thunk(Core core, FunctionCall c, int startIdx, int endIdx);

    /// <summary>Runs one function's numerics over the bound buffers.</summary>
    /// <remarks>Reports failure as a code, like
    /// <see cref="FunctionCall.TryInvoke"/> and for the same reason: a composed
    /// body cross-calls its callee's PUBLIC tier, and that throws. Converting it
    /// here keeps the sweeps reading one thing. Anything that is not the
    /// library's own failure is left to propagate — the sweeps classify it.
    /// <para>No boundness check: the sweeps bind every input and output before
    /// calling, and an unbound slot faulting is a fixture bug the sweeps should
    /// see rather than a code they should read.</para></remarks>
    internal static RetCode Invoke(string name, Core core, FunctionCall call,
                                   int startIdx, int endIdx, out OutRange range)
    {
        try
        {
            CallOutcome outcome = Thunks[name](core, call, startIdx, endIdx);
            range = new OutRange(outcome.BegIdx, outcome.Count);
            return outcome.Code;
        }
        catch (Exception e) when (e is ITaLibFailure f)
        {
            range = new OutRange(0, 0);
            return f.RetCode;
        }
    }

    /// <summary>One thunk per catalogued function, by name.</summary>
    internal static readonly Dictionary<string, Thunk> Thunks = new(StringComparer.Ordinal)
    {
"#,
    );

    for r in rows {
        let def = by_name[r.name.as_str()];
        let mut args: Vec<String> = vec!["startIdx".into(), "endIdx".into()];
        args.extend(input_arg_exprs(r));
        args.extend(opt_arg_exprs(def));
        args.push("out int b".into());
        args.push("out int n".into());
        for (k, out) in r.outputs.iter().enumerate() {
            args.push(match out.kind {
                OutputKind::Real => format!("c.RealOut({k})"),
                OutputKind::Integer => format!("c.IntOut({k})"),
            });
        }
        let _ = writeln!(s, "        [\"{}\"] = static (core, c, startIdx, endIdx) =>", r.name);
        s.push_str("        {\n");
        let _ = writeln!(s, "            RetCode rc = core.{}_Impl(", r.name);
        let _ = writeln!(s, "                {});", args.join(", "));
        s.push_str("            return new CallOutcome(rc, b, n);\n");
        s.push_str("        },\n");
    }

    s.push_str("    };\n}\n");
    s
}

fn write(dir: &Path, name: &str, body: &str) {
    super::write_if_changed_silent(&dir.join(name), body);
}

/// Remove any `.g.cs` in `dir` that this module no longer emits.
fn clean_stale(dir: &Path, keep: &[&str]) {
    let Ok(entries) = std::fs::read_dir(dir) else {
        return;
    };
    for entry in entries.flatten() {
        let name = entry.file_name().to_string_lossy().to_string();
        if name.ends_with(".g.cs") && !keep.contains(&name.as_str()) {
            std::fs::remove_file(entry.path()).ok();
            println!("  removed stale C# metadata file {name}");
        }
    }
}

/// The license + generated-file banner every emitted file carries.
fn header() -> String {
    let mut s = String::from(super::ta_abstract_c::LICENSE);
    s.push_str("\n/* Generated by ta_codegen (backends/csharp_metadata.rs) from\n");
    s.push_str(" * ta_codegen/input/ — do not edit.\n");
    s.push_str(" * MF,CC\n */\n\n");
    // Roslyn treats a `.g.cs` file as auto-generated and then refuses nullable
    // annotations unless the context is stated in the file itself (CS8669),
    // regardless of the project-level `<Nullable>enable</Nullable>`.
    s.push_str("#nullable enable\n\n");
    s
}

/// A C# string literal (escaping `"` and `\`).
fn cs(s: &str) -> String {
    let mut o = String::from("\"");
    for c in s.chars() {
        if c == '"' || c == '\\' {
            o.push('\\');
        }
        o.push(c);
    }
    o.push('"');
    o
}

/// A `double` literal C# accepts. Rust's `Debug` gives the shortest
/// round-tripping form; the `D` suffix keeps an integral value a `double`.
fn cd(v: f64) -> String {
    let s = format!("{v:?}");
    if s.contains('.') || s.contains('e') || s.contains('E') {
        s
    } else {
        format!("{s}D")
    }
}

// ---------------------------------------------------------------------------
// Vocabularies
// ---------------------------------------------------------------------------
//
// The bit VALUES are not written here: each is computed by calling the shared
// `*_flag_bits` helper with the flag's own YAML token, so a C# constant cannot
// drift from the value the Rust and Java registries emit. `flag_sync` pins
// those helpers against `include/ta_abstract.h` in turn.

/// `(yaml_token, C# member name, doc sentence)` for every function flag.
const FUNC_FLAGS: &[(&str, &str, &str)] = &[
    ("overlap", "Overlap", "Output is on the input's scale and overlays a price chart."),
    ("stream", "Stream", "A streaming (one-bar-at-a-time) API exists for this function."),
    ("volume", "VolumeUsed", "The function consumes volume."),
    (
        "unstable_period",
        "UnstablePeriod",
        "Recursive: honours the unstable-period setting. See <see cref=\"FunctionInfo.UnstableId\"/>.",
    ),
    ("candlestick", "Candlestick", "The function recognises a candlestick pattern."),
    (
        "path_dependent",
        "PathDependent",
        "Output depends on where the caller started, so it never converges across ranges.",
    ),
    (
        "nan_inf_output",
        "NanInfOutput",
        "Some inputs have no finite result, so a successful call can return NaN or +/-Infinity.",
    ),
    (
        "period1_identity",
        "Period1Identity",
        "A period of 1 performs no smoothing: the lookback is 0 and the output is a bit-exact copy of the input.",
    ),
];

const OPT_FLAGS: &[(&str, &str, &str)] = &[
    ("percent", "IsPercent", "The value is expressed as a percentage."),
    ("degree", "IsDegree", "The value is expressed in degrees."),
    ("currency", "IsCurrency", "The value is expressed in a currency."),
    ("advanced", "Advanced", "Advanced: a basic user interface may hide it."),
];

const OUTPUT_FLAGS: &[(&str, &str, &str)] = &[
    ("line", "Line", "Draw as a continuous line."),
    ("dot_line", "DotLine", "Draw as a dotted line."),
    ("dash_line", "DashLine", "Draw as a dashed line."),
    ("dot", "Dot", "Draw as unconnected dots."),
    ("histogram", "Histogram", "Draw as a histogram."),
    ("pattern_bool", "PatternBool", "0 = no pattern, 100 = pattern."),
    ("pattern_bull_bear", "PatternBullBear", "-100 = bearish, 0 = none, 100 = bullish."),
    ("pattern_strength", "PatternStrength", "-200..-100 bearish, 100..200 bullish."),
    ("positive", "Positive", "The value is always at or above zero."),
    ("negative", "Negative", "The value is always at or below zero."),
    ("zero", "Zero", "Zero is a meaningful reference level."),
    ("upper_limit", "UpperLimit", "An upper band or limit line."),
    ("lower_limit", "LowerLimit", "A lower band or limit line."),
    (
        "nullable",
        "Nullable",
        "Discardable: C accepts <c>NULL</c> for it. C# still requires an array.",
    ),
];

/// The six OHLCV components, with the bit `price_bundle` assigns each.
const PRICE_COMPONENTS: &[(PriceComponent, &str, &str)] = &[
    (PriceComponent::Open, "Open", "The open price."),
    (PriceComponent::High, "High", "The high price."),
    (PriceComponent::Low, "Low", "The low price."),
    (PriceComponent::Close, "Close", "The close price."),
    (PriceComponent::Volume, "Volume", "The traded volume."),
    (PriceComponent::OpenInterest, "OpenInterest", "The open interest."),
];

/// The OR of every flags word actually emitted, per class — used both for the
/// "no shipped function sets this bit" remarks below and by `CatalogFacts`.
struct FlagCensus {
    func: u32,
    input: u32,
    opt: u32,
    output: u32,
}

fn census(rows: &[FuncRow]) -> FlagCensus {
    let mut c = FlagCensus { func: 0, input: 0, opt: 0, output: 0 };
    for r in rows {
        c.func |= r.flags;
        for i in &r.inputs {
            c.input |= i.flags;
        }
        for o in &r.opt_inputs {
            c.opt |= o.flags;
        }
        for o in &r.outputs {
            c.output |= o.flags;
        }
    }
    c
}

/// The remark appended to a flag member no shipped function sets. Derived from
/// the rows, so it is a self-updating documented fact rather than folklore.
fn dead_remark(bit: u32, seen: u32) -> &'static str {
    if bit & seen == 0 {
        " No shipped function sets this bit."
    } else {
        ""
    }
}

fn flags_enum(
    s: &mut String,
    name: &str,
    doc: &str,
    members: &[(&str, &str, &str)],
    bits: fn(&[String]) -> u32,
    seen: u32,
) {
    let _ = write!(s, "/// <summary>{doc}</summary>\n[Flags]\npublic enum {name} : uint\n{{\n");
    s.push_str("    /// <summary>No flags.</summary>\n    None = 0,\n");
    for (token, member, mdoc) in members {
        let bit = bits(&[(*token).to_string()]);
        assert_ne!(bit, 0, "csharp_metadata: `{token}` is not a known flag token");
        let _ = write!(
            s,
            "\n    /// <summary>{mdoc}{}</summary>\n    {member} = 0x{bit:08X},\n",
            dead_remark(bit, seen)
        );
    }
    s.push_str("}\n\n");
}

fn vocabulary(rows: &[FuncRow]) -> String {
    let c = census(rows);
    let mut s = header();
    s.push_str("using System;\n\n");
    let _ = writeln!(s, "namespace {NAMESPACE};\n");

    // --- FunctionGroup ---
    s.push_str("/// <summary>The category a function belongs to.</summary>\n");
    s.push_str("public enum FunctionGroup\n{\n");
    for (i, g) in Group::ALL.iter().enumerate() {
        if i > 0 {
            s.push('\n');
        }
        let _ = write!(
            s,
            "    /// <summary>{}.</summary>\n    {},\n",
            xml_escape_raw(g.as_str()),
            g.ident()
        );
    }
    s.push_str("}\n\n");

    s.push_str("/// <summary>Display names for <see cref=\"FunctionGroup\"/>.</summary>\n");
    s.push_str("public static class FunctionGroupExtensions\n{\n");
    s.push_str("    /// <summary>The canonical display name, matching C's <c>TA_FuncInfo.group</c>.</summary>\n");
    s.push_str("    /// <param name=\"group\">The group.</param>\n");
    s.push_str("    /// <returns>For example <c>\"Statistic Functions\"</c>.</returns>\n");
    s.push_str("    /// <exception cref=\"ArgumentOutOfRangeException\">The value is not a declared group.</exception>\n");
    s.push_str("    public static string ToDisplayName(this FunctionGroup group) => group switch\n    {\n");
    for g in Group::ALL {
        let _ = writeln!(
            s,
            "        FunctionGroup.{} => {},",
            g.ident(),
            cs(g.as_str())
        );
    }
    s.push_str("        _ => throw new ArgumentOutOfRangeException(nameof(group)),\n");
    s.push_str("    };\n}\n\n");

    // --- flag classes ---
    flags_enum(
        &mut s,
        "FunctionFlags",
        "Behavioural properties of a function. Values match C's <c>TA_FUNC_FLG_*</c>.",
        FUNC_FLAGS,
        func_flag_bits,
        c.func,
    );

    // Price components are their own vocabulary: the bits come from
    // `price_bundle`, which is also what the C abstract tables use.
    s.push_str("/// <summary>Which OHLCV components a price input consumes. Values match C's <c>TA_IN_PRICE_*</c>.</summary>\n");
    s.push_str("[Flags]\npublic enum PriceComponents : uint\n{\n");
    s.push_str("    /// <summary>Not a price input.</summary>\n    None = 0,\n");
    for (comp, member, doc) in PRICE_COMPONENTS {
        let bit = super::price_bundle::flags(&[*comp]);
        let _ = write!(
            s,
            "\n    /// <summary>{doc}{}</summary>\n    {member} = 0x{bit:08X},\n",
            dead_remark(bit, c.input)
        );
    }
    s.push_str("}\n\n");

    flags_enum(
        &mut s,
        "OptInputFlags",
        "Presentation hints for an optional parameter. Values match C's <c>TA_OPTIN_*</c>.",
        OPT_FLAGS,
        opt_flag_bits,
        c.opt,
    );
    flags_enum(
        &mut s,
        "OutputFlags",
        "How an output is meant to be drawn, and whether it may be discarded. Values match C's <c>TA_OUT_*</c>.",
        OUTPUT_FLAGS,
        output_flag_bits,
        c.output,
    );

    // --- parameter kinds ---
    s.push_str(
        "/// <summary>What a required input carries. Mirrors C's <c>TA_InputParameterType</c>.</summary>\n\
         public enum InputKind\n{\n\
         \x20   /// <summary>An OHLCV bundle; see <see cref=\"InputInfo.Components\"/>.</summary>\n\
         \x20   Price = 0,\n\n\
         \x20   /// <summary>A single <c>double[]</c> series.</summary>\n\
         \x20   Real = 1,\n\n\
         \x20   /// <summary>A single <c>int[]</c> series. Declared by no shipped function.</summary>\n\
         \x20   Integer = 2,\n}\n\n",
    );
    s.push_str(
        "/// <summary>What an output carries. Mirrors C's <c>TA_OutputParameterType</c>.</summary>\n\
         public enum OutputKind\n{\n\
         \x20   /// <summary>A <c>double[]</c> series.</summary>\n\
         \x20   Real = 0,\n\n\
         \x20   /// <summary>An <c>int[]</c> series (candlestick patterns, index outputs).</summary>\n\
         \x20   Integer = 1,\n}\n",
    );
    s
}

// ---------------------------------------------------------------------------
// The corpus-independent model types
// ---------------------------------------------------------------------------

fn model() -> String {
    let mut s = header();
    s.push_str(MODEL);
    s
}

/// `FunctionCall.g.cs`'s text, without writing it — so a test can assert on the
/// emitted binder.
pub fn render_function_call() -> String {
    function_call()
}

fn function_call() -> String {
    let mut s = header();
    s.push_str(FUNCTION_CALL);
    s
}

// ---------------------------------------------------------------------------
// The catalogue
// ---------------------------------------------------------------------------

fn catalog(rows: &[FuncRow], by_name: &HashMap<&str, &FuncDef>) -> String {
    let mut s = header();
    s.push_str("using System;\n");
    s.push_str("using System.Collections;\n");
    s.push_str("using System.Collections.Frozen;\n");
    s.push_str("using System.Collections.Generic;\n");
    s.push_str("using System.Collections.Immutable;\n");
    s.push_str("using System.Diagnostics.CodeAnalysis;\n");
    s.push_str("using System.Linq;\n\n");
    let _ = writeln!(s, "namespace {NAMESPACE};\n");

    s.push_str(CATALOG_DOC);
    s.push_str("public sealed class FunctionCatalog : IReadOnlyList<FunctionInfo>\n{\n");

    // The moving-average choice list, emitted once and shared by every parameter
    // that offers it — the C# analogue of C's single &TA_MA_TypeList.
    //
    // It MUST be declared before `Default`: C# runs static field initializers in
    // textual order, so a list declared after it is still
    // `default(ImmutableArray<T>)` when `new()` calls the factories, and the
    // first `.Length` throws. (MetadataTest caught exactly that.)
    // Every choice list in the corpus is the SAME list, which is not an
    // accident: `ta_abstract_c` routes every `ParamType::Enum(_)` to the single
    // `&TA_MA_TypeList`, so one shared array mirrors the C oracle rather than
    // diverging from it. Assert it instead of assuming it — a second enum type
    // would otherwise render every parameter with the first one's values, and
    // the only thing that would notice is `test_abstract.c`'s valueList strcmp,
    // one language at a time.
    let lists: Vec<&Vec<(i64, String)>> = rows
        .iter()
        .flat_map(|r| &r.opt_inputs)
        .filter_map(|o| match &o.domain {
            OptDomain::IntegerList { values, .. } => Some(values),
            _ => None,
        })
        .collect();
    assert!(
        lists.windows(2).all(|w| w[0] == w[1]),
        "csharp_metadata: the corpus now has more than one distinct choice list; \
         the shared MATypeValues array can no longer stand in for all of them"
    );
    if let Some(values) = lists.first().map(|v| (*v).clone()) {
        s.push_str("    private static readonly ImmutableArray<NamedValue> MATypeValues =\n    [\n");
        for (v, name) in &values {
            let _ = writeln!(s, "        new({v}, {}),", cs(name));
        }
        s.push_str("    ];\n\n");
    }

    s.push_str("    /// <summary>The process-wide catalogue.</summary>\n");
    s.push_str("    public static FunctionCatalog Default { get; } = new();\n\n");

    s.push_str("    /// <summary>Every group at least one function belongs to, in canonical order.</summary>\n");
    s.push_str("    public static ImmutableArray<FunctionGroup> Groups { get; } =\n    [\n");
    for g in Group::ALL {
        if rows.iter().any(|r| r.group == *g) {
            let _ = writeln!(s, "        FunctionGroup.{},", g.ident());
        }
    }
    s.push_str("    ];\n\n");

    s.push_str("    private readonly ImmutableArray<FunctionInfo> _all;\n");
    s.push_str("    private readonly FrozenDictionary<string, FunctionInfo> _byName;\n\n");
    s.push_str("    private FunctionCatalog()\n    {\n");
    s.push_str("        _all =\n        [\n");
    for r in rows {
        let _ = writeln!(s, "            {}(),", factory_name(&r.name));
    }
    s.push_str("        ];\n");
    // OrdinalIgnoreCase (issue #278): every name is invariant ASCII, so an
    // ordinal fold is exact and cheap, and matches C's TA_GetFuncHandle, which
    // folds ASCII case the same way. Never a culture-aware comparer — that
    // would have the classic Turkish-locale "i" bug on top of being slower.
    s.push_str("        _byName = _all.ToFrozenDictionary(f => f.Name, StringComparer.OrdinalIgnoreCase);\n");
    s.push_str("    }\n\n");

    s.push_str(CATALOG_MEMBERS);

    for r in rows {
        emit_factory(&mut s, r, by_name);
    }

    s.push_str("}\n");
    s
}

/// The `private static FunctionInfo MakeXxx()` name for a function.
fn factory_name(name: &str) -> String {
    let mut o = String::from("Make");
    let mut upper = true;
    for c in name.chars() {
        if c == '_' {
            upper = true;
        } else if upper {
            o.extend(c.to_uppercase());
            upper = false;
        } else {
            o.extend(c.to_lowercase());
        }
    }
    o
}

fn group_ident(g: Group) -> String {
    format!("FunctionGroup.{}", g.ident())
}

fn func_flags_expr(bits: u32) -> String {
    flag_expr("FunctionFlags", bits, FUNC_FLAGS, func_flag_bits)
}

fn opt_flags_expr(bits: u32) -> String {
    flag_expr("OptInputFlags", bits, OPT_FLAGS, opt_flag_bits)
}

fn output_flags_expr(bits: u32) -> String {
    flag_expr("OutputFlags", bits, OUTPUT_FLAGS, output_flag_bits)
}

/// Render a flags word as an OR of named members, so the emitted source reads
/// as the flags it means and a stray bit cannot hide behind a hex literal.
fn flag_expr(
    ty: &str,
    bits: u32,
    members: &[(&str, &str, &str)],
    bit_of: fn(&[String]) -> u32,
) -> String {
    let mut named = Vec::new();
    let mut covered = 0u32;
    for (token, member, _) in members {
        let b = bit_of(&[(*token).to_string()]);
        if bits & b != 0 {
            named.push(format!("{ty}.{member}"));
            covered |= b;
        }
    }
    assert_eq!(
        covered, bits,
        "csharp_metadata: {ty} word {bits:#010x} carries a bit no member names"
    );
    if named.is_empty() {
        format!("{ty}.None")
    } else {
        named.join(" | ")
    }
}

fn price_components_expr(bits: u32) -> String {
    let mut named = Vec::new();
    let mut covered = 0u32;
    for (comp, member, _) in PRICE_COMPONENTS {
        let b = super::price_bundle::flags(&[*comp]);
        if bits & b != 0 {
            named.push(format!("PriceComponents.{member}"));
            covered |= b;
        }
    }
    assert_eq!(covered, bits, "csharp_metadata: price bits {bits:#010x} carry an unnamed component");
    if named.is_empty() {
        "PriceComponents.None".to_string()
    } else {
        named.join(" | ")
    }
}

fn component_member(c: PriceComponent) -> &'static str {
    match c {
        PriceComponent::Open => "Open",
        PriceComponent::High => "High",
        PriceComponent::Low => "Low",
        PriceComponent::Close => "Close",
        PriceComponent::Volume => "Volume",
        PriceComponent::OpenInterest => "OpenInterest",
    }
}

fn input_expr(inp: &InputRow) -> String {
    let order = if inp.signature_components.is_empty() {
        "[]".to_string()
    } else {
        format!(
            "[{}]",
            inp.signature_components
                .iter()
                .map(|c| format!("PriceComponents.{}", component_member(*c)))
                .collect::<Vec<_>>()
                .join(", ")
        )
    };
    let kind = match inp.kind {
        InputKind::Price => "Price",
        InputKind::Real => "Real",
        InputKind::Integer => "Integer",
    };
    format!(
        "new InputInfo(InputKind.{kind}, {}, {}, {order})",
        cs(&inp.param_name),
        price_components_expr(inp.flags)
    )
}

fn output_expr(out: &OutputRow) -> String {
    let kind = match out.kind {
        OutputKind::Real => "Real",
        OutputKind::Integer => "Integer",
    };
    format!(
        "new OutputInfo(OutputKind.{kind}, {}, {})",
        cs(&out.param_name),
        output_flags_expr(out.flags)
    )
}

fn domain_expr(domain: &OptDomain) -> String {
    match domain {
        OptDomain::RealRange { min, max, precision, default, suggested } => format!(
            "new OptInputDomain.RealRange({}, {}, {precision}, {}, {}, {}, {})",
            cd(*min),
            cd(*max),
            cd(*default),
            cd(suggested.0),
            cd(suggested.1),
            cd(suggested.2)
        ),
        OptDomain::IntegerRange { min, max, default, suggested } => format!(
            "new OptInputDomain.IntegerRange({min}, {max}, {default}, {}, {}, {})",
            suggested.0, suggested.1, suggested.2
        ),
        OptDomain::IntegerList { default, .. } => {
            format!("new OptInputDomain.IntegerList(MATypeValues, {default})")
        }
        // Unreachable on the shipped corpus; emitted rather than folded into a
        // catch-all so a future real list is a compile-time decision, not a
        // silent misrepresentation.
        OptDomain::RealList { values, default } => format!(
            "new OptInputDomain.RealList([{}], {})",
            values
                .iter()
                .map(|(v, n)| format!("new({}, {})", cd(*v), cs(n)))
                .collect::<Vec<_>>()
                .join(", "),
            cd(*default)
        ),
    }
}

fn opt_expr(opt: &OptRow) -> String {
    format!(
        "new OptInputInfo({}, {}, {}, {}, {})",
        cs(&opt.param_name),
        cs(&opt.display_name),
        cs(&opt.hint),
        opt_flags_expr(opt.flags),
        domain_expr(&opt.domain)
    )
}

/// One function's factory, with its two dispatch thunks emitted in the same
/// expression as the parameter lists they index.
fn emit_factory(s: &mut String, r: &FuncRow, by_name: &HashMap<&str, &FuncDef>) {
    let def = by_name[r.name.as_str()];
    let method = r.name.clone();

    let _ = writeln!(s, "    private static FunctionInfo {}() => new(", factory_name(&r.name));
    let _ = writeln!(s, "        name: {},", cs(&r.name));
    let _ = writeln!(s, "        group: {},", group_ident(r.group));
    let _ = writeln!(s, "        hint: {},", cs(&r.hint));
    let _ = writeln!(s, "        flags: {},", func_flags_expr(r.flags));
    match &r.unst {
        Some(u) => {
            let _ = writeln!(s, "        unstableId: FuncUnstId.{},", u.name);
        }
        None => s.push_str("        unstableId: null,\n"),
    }

    emit_list(s, "inputs", &r.inputs.iter().map(input_expr).collect::<Vec<_>>());
    emit_list(s, "optInputs", &r.opt_inputs.iter().map(opt_expr).collect::<Vec<_>>());
    emit_list(s, "outputs", &r.outputs.iter().map(output_expr).collect::<Vec<_>>());

    // --- the thunks ---
    let opt_args = opt_arg_exprs(def);
    let _ = writeln!(
        s,
        "        lookback: static (core, c) => core.{method}_Lookback({}),",
        opt_args.join(", ")
    );

    let mut call_args: Vec<String> = vec!["startIdx".into(), "endIdx".into()];
    call_args.extend(input_arg_exprs(r));
    call_args.extend(opt_args);
    for (k, out) in r.outputs.iter().enumerate() {
        call_args.push(match out.kind {
            OutputKind::Real => format!("c.RealOut({k})"),
            OutputKind::Integer => format!("c.IntOut({k})"),
        });
    }
    // The PUBLIC overload (#265), which is what C's frames and Java's Dispatch
    // have always called. It bound the body until then, so this tier did no
    // argument checking at all and a leg shorter than the range reached the
    // numerics: an `IndexOutOfRangeException` escaping `TryInvoke`, whose
    // documented contract is a code and not an exception.
    //
    // It bound the body for a reason, and the reason was a test: `NoPhantomIoTest`
    // drove functions through this binder, so the public tier's length checks
    // would have rejected its undersized arrays before the body could touch
    // them. That probe brings its own `NAME_Impl` binder now, which is what
    // freed this one.
    //
    // The public overload returns `OutRange` and throws, so the code comes back
    // through `FunctionCall.TryInvoke`'s `ITaLibFailure` catch -- the same one
    // that already converted a composed body's cross-call rejection since #236
    // step 3, now on the direct path too.
    s.push_str("        invoke: static (core, c, startIdx, endIdx) =>\n");
    let _ = writeln!(s, "            core.{method}(");
    let _ = writeln!(s, "                {}));\n", call_args.join(", "));
}

/// The argument expressions for a function's required inputs. A price bundle is
/// unfolded here — by *naming* each component, never by indexing — using the
/// signature order the row carries.
fn input_arg_exprs(r: &FuncRow) -> Vec<String> {
    let mut args = Vec::new();
    for (slot, inp) in r.inputs.iter().enumerate() {
        match inp.kind {
            InputKind::Price => {
                for c in &inp.signature_components {
                    args.push(format!("c.Price({slot}, PriceComponents.{})", component_member(*c)));
                }
            }
            InputKind::Real => args.push(format!("c.Series({slot})")),
            InputKind::Integer => args.push(format!("c.IntSeries({slot})")),
        }
    }
    args
}

/// The argument expressions for a function's optional parameters. Read from the
/// `FuncDef` rather than the row because only it names the enum *type* the
/// generated signature declares.
fn opt_arg_exprs(def: &FuncDef) -> Vec<String> {
    def.optional_inputs
        .iter()
        .enumerate()
        .map(|(i, o)| match &o.param_type {
            ParamType::Real => format!("c.RealOpt({i})"),
            ParamType::Enum(name) => format!("({name})c.IntOpt({i})"),
            _ => format!("c.IntOpt({i})"),
        })
        .collect()
}

fn emit_list(s: &mut String, label: &str, items: &[String]) {
    if items.is_empty() {
        let _ = writeln!(s, "        {label}: [],");
        return;
    }
    let _ = writeln!(s, "        {label}:\n        [");
    for it in items {
        let _ = writeln!(s, "            {it},");
        }
    s.push_str("        ],\n");
}

// ---------------------------------------------------------------------------
// CatalogFacts — the numbers the shipped test suite asserts against
// ---------------------------------------------------------------------------

/// `TA_FunctionDescriptionXML`'s analog, carrying the real XML.
///
/// Ships the real XML, never a `(length, checksum)` pair baked at generation
/// time: `test_abstract.c` compares it against C's actual bytes, and a constant
/// the generator computed from the same string C's own table is built from is
/// the generator agreeing with itself — a gate that cannot fail (#164).
///
/// A verbatim literal: the XML is ASCII with no backslashes, so only `"` needs
/// doubling, and C# has no per-literal size limit to work around.
fn function_description(funcs: &[FuncDef]) -> String {
    let xml = super::func_api_xml::generate_string(funcs);
    assert!(
        xml.is_ascii(),
        "ta_func_api.xml is no longer ASCII; revisit the verbatim-literal encoding below"
    );

    let mut s = header();
    let _ = writeln!(s, "namespace {NAMESPACE};\n");
    s.push_str(
        "/// <summary>\n\
         /// The machine-readable description of every function, as XML.\n\
         /// </summary>\n\
         /// <remarks>\n\
         /// The C# analog of C's <c>TA_FunctionDescriptionXML()</c>. Same bytes: both\n\
         /// are emitted by one generator from one set of definitions.\n\
         /// </remarks>\n\
         public static class FunctionDescription\n{\n",
    );
    s.push_str("    /// <summary>The XML document, identical to C's.</summary>\n");
    s.push_str("    public static string Xml => XmlText;\n\n");
    s.push_str("    private const string XmlText = @\"");
    s.push_str(&xml.replace('"', "\"\""));
    s.push_str("\";\n}\n");
    s
}

fn catalog_facts(rows: &[FuncRow]) -> String {
    let c = census(rows);
    let mut s = header();
    let _ = writeln!(s, "namespace {NAMESPACE};\n");
    s.push_str(
        "/* Generated facts about the catalogue, for the shipped test suite to\n\
         \x20  assert against. `internal`, so they are not public API — a consumer\n\
         \x20  should read the catalogue, not a constant about it.\n\
         \n\
         \x20  The flag masks are the EXACT OR of every emitted flags word. A\n\
         \x20  presence test (`(flags & X) != 0`) structurally cannot see an EXTRA\n\
         \x20  bit; comparing the OR can, which is what makes the flag assertions\n\
         \x20  non-vacuous. */\n",
    );
    s.push_str("internal static class CatalogFacts\n{\n");
    let _ = writeln!(s, "    internal const int FunctionCount = {};", rows.len());
    let _ = writeln!(s, "    internal const uint AllFunctionFlags = 0x{:08X}U;", c.func);
    let _ = writeln!(s, "    internal const uint AllPriceComponents = 0x{:08X}U;", c.input);
    let _ = writeln!(s, "    internal const uint AllOptInputFlags = 0x{:08X}U;", c.opt);
    let _ = writeln!(s, "    internal const uint AllOutputFlags = 0x{:08X}U;", c.output);

    let mut dead: Vec<String> = Vec::new();
    for (token, member, _) in FUNC_FLAGS {
        if func_flag_bits(&[(*token).to_string()]) & c.func == 0 {
            dead.push(format!("FunctionFlags.{member}"));
        }
    }
    for (comp, member, _) in PRICE_COMPONENTS {
        if super::price_bundle::flags(&[*comp]) & c.input == 0 {
            dead.push(format!("PriceComponents.{member}"));
        }
    }
    for (token, member, _) in OPT_FLAGS {
        if opt_flag_bits(&[(*token).to_string()]) & c.opt == 0 {
            dead.push(format!("OptInputFlags.{member}"));
        }
    }
    for (token, member, _) in OUTPUT_FLAGS {
        if output_flag_bits(&[(*token).to_string()]) & c.output == 0 {
            dead.push(format!("OutputFlags.{member}"));
        }
    }
    s.push_str(
        "\n    /* Flag members no shipped function sets. Pinned rather than deleted:\n\
         \x20     these are the C vocabulary, and the day a definition starts using\n\
         \x20     one this list is what tells you, instead of the fact changing\n\
         \x20     quietly under consumers. */\n",
    );
    let _ = writeln!(s, "    internal const int DeadFlagCount = {};", dead.len());
    s.push_str("    internal static readonly string[] DeadFlags =\n    [\n");
    for d in &dead {
        let _ = writeln!(s, "        \"{d}\",");
    }
    s.push_str("    ];\n}\n");
    s
}

// ---------------------------------------------------------------------------
// Fixed source (corpus-independent), emitted verbatim
// ---------------------------------------------------------------------------

const CATALOG_DOC: &str = r#"/// <summary>
/// Every indicator the library provides, queryable at run time.
/// </summary>
/// <remarks>
/// For an application that picks a function dynamically — a charting UI listing
/// studies, a backtester with user-selectable indicators, a parameter sweep.
/// Use <see cref="Default"/>.
/// <para>Implements <see cref="IReadOnlyList{T}"/>, so it is directly
/// enumerable and LINQ-able:</para>
/// <code>
/// foreach (var f in FunctionCatalog.Default.Where(f => f.Flags.HasFlag(FunctionFlags.Candlestick)))
///     Console.WriteLine($"{f.Name}: {f.Hint}");
/// </code>
/// <para>Generated from the same definitions as the indicators themselves, so it
/// cannot drift from them. Immutable and safe to use from any thread.</para>
/// <para>Scope is the guarded, double-precision batch API — the same surface C's
/// <c>ta_abstract</c> and Rust's <c>abstract_api</c> describe. Streaming handles,
/// <c>float[]</c> overloads are not catalogued.</para>
/// </remarks>
"#;

const CATALOG_MEMBERS: &str = r#"    /// <summary>How many functions the catalogue holds.</summary>
    public int Count => _all.Length;

    /// <summary>The function at an index, in canonical name order.</summary>
    /// <param name="index">A zero-based index below <see cref="Count"/>.</param>
    /// <returns>The function's metadata.</returns>
    public FunctionInfo this[int index] => _all[index];

    /// <summary>The function with a given name.</summary>
    /// <param name="name">The function name, for example <c>"SMA"</c> or <c>"sma"</c> —
    /// matched ASCII case-insensitively (<see cref="StringComparer.OrdinalIgnoreCase"/>,
    /// matching C's <c>TA_GetFuncHandle</c>), never culture-aware. The returned metadata's
    /// own <see cref="FunctionInfo.Name"/> always stays the canonical upper-case spelling.</param>
    /// <returns>The function's metadata.</returns>
    /// <exception cref="KeyNotFoundException">No function has that name.</exception>
    public FunctionInfo this[string name] => _byName[name];

    /// <summary>Looks a function up without throwing.</summary>
    /// <param name="name">The function name, matched ASCII case-insensitively — see
    /// <see cref="this[string]"/>.</param>
    /// <param name="info">The metadata, when the name is known.</param>
    /// <returns><see langword="true"/> when the name is known.</returns>
    public bool TryGet(string name, [NotNullWhen(true)] out FunctionInfo? info)
        => _byName.TryGetValue(name, out info);

    /// <summary>The functions in one group, in canonical name order.</summary>
    /// <param name="group">The group to filter by.</param>
    /// <returns>A lazily filtered sequence.</returns>
    public IEnumerable<FunctionInfo> InGroup(FunctionGroup group)
        => _all.Where(f => f.Group == group);

    /// <summary>Enumerates every function, in canonical name order.</summary>
    /// <returns>An enumerator over the catalogue.</returns>
    public IEnumerator<FunctionInfo> GetEnumerator()
        => ((IEnumerable<FunctionInfo>)_all).GetEnumerator();

    IEnumerator IEnumerable.GetEnumerator() => GetEnumerator();

"#;

const MODEL: &str = r#"using System;
using System.Collections.Generic;
using System.Collections.Immutable;
using System.Linq;

namespace TALib.Metadata;

/// <summary>Computes a function's lookback from a bound call.</summary>
internal delegate int LookbackThunk(Core core, FunctionCall call);

/// <summary>Runs a function from a bound call.</summary>
/// <remarks>The thunk calls the function's public overload, so a rejection
/// arrives as an exception and the range is all that comes back;
/// <see cref="FunctionCall.TryInvoke"/> turns the exception into the code it
/// promises (#265).</remarks>
internal delegate OutRange InvokeThunk(Core core, FunctionCall call, int startIdx, int endIdx);

/// <summary>One entry of a named choice list.</summary>
public sealed record NamedValue
{
    internal NamedValue(long value, string name) => (Value, Name) = (value, name);

    /// <summary>The numeric value the function is given.</summary>
    public long Value { get; }

    /// <summary>The short label, for example <c>"SMA"</c>.</summary>
    public string Name { get; }
}

/// <summary>One entry of a named real choice list.</summary>
public sealed record NamedRealValue
{
    internal NamedRealValue(double value, string name) => (Value, Name) = (value, name);

    /// <summary>The numeric value the function is given.</summary>
    public double Value { get; }

    /// <summary>The short label.</summary>
    public string Name { get; }
}

/// <summary>The values an optional parameter accepts.</summary>
/// <remarks>The typed replacement for C's <c>void *dataSet</c> plus a separate
/// type tag. The hierarchy is closed — the constructor is
/// <see langword="private protected"/>, so the four nested records below are the
/// only cases that can exist — which lets a consumer <c>switch</c> over it
/// exhaustively.</remarks>
public abstract record OptInputDomain
{
    private protected OptInputDomain() { }

    /// <summary>The documented default, as C reports it in <c>defaultValue</c>.</summary>
    public abstract double DefaultValue { get; }

    /// <summary>A continuous range of real values.</summary>
    public sealed record RealRange : OptInputDomain
    {
        internal RealRange(double min, double max, int precision, double defaultValue,
                           double suggestedStart, double suggestedEnd, double suggestedIncrement)
        {
            Min = min;
            Max = max;
            Precision = precision;
            Default = defaultValue;
            SuggestedStart = suggestedStart;
            SuggestedEnd = suggestedEnd;
            SuggestedIncrement = suggestedIncrement;
        }

        /// <summary>The smallest accepted value.</summary>
        public double Min { get; }

        /// <summary>The largest accepted value.</summary>
        public double Max { get; }

        /// <summary>Decimal places a user interface should offer.</summary>
        public int Precision { get; }

        /// <summary>The documented default.</summary>
        public double Default { get; }

        /// <summary>The first value a parameter sweep should try.</summary>
        public double SuggestedStart { get; }

        /// <summary>The last value a parameter sweep should try.</summary>
        public double SuggestedEnd { get; }

        /// <summary>The step between swept values.</summary>
        public double SuggestedIncrement { get; }

        /// <inheritdoc/>
        public override double DefaultValue => Default;

        /// <summary>The suggested sweep values, low to high.</summary>
        /// <returns><see cref="Default"/> alone when <see cref="SuggestedIncrement"/>
        /// is not positive; otherwise <c>SuggestedStart + i * SuggestedIncrement</c>
        /// while that does not exceed <see cref="SuggestedEnd"/>. Computed from the
        /// index rather than accumulated, so the step cannot drift.</returns>
        public IEnumerable<double> Sweep()
        {
            if (SuggestedIncrement <= 0)
            {
                yield return Default;
                yield break;
            }

            /* The slack absorbs accumulated representation error and nothing
               more. SAREXT's 0.01..0.15 step 0.01 computes its endpoint as
               0.15000000000000002 — 2.8e-17 over — so a bare `v > SuggestedEnd`
               silently drops the last value the metadata advertises. It must
               NOT be a half-step: T3's optInVFactor is 0.01..1.0 step 0.05,
               whose grid overshoots to 1.01, and half a step would admit a
               value outside the parameter's own [0, 1] range. */
            double limit = SuggestedEnd + (Math.Abs(SuggestedIncrement) * 1e-9);
            for (int i = 0; ; i++)
            {
                double v = SuggestedStart + (i * SuggestedIncrement);
                if (v > limit)
                {
                    yield break;
                }

                yield return v;
            }
        }
    }

    /// <summary>A range of integer values.</summary>
    public sealed record IntegerRange : OptInputDomain
    {
        internal IntegerRange(int min, int max, int defaultValue,
                              int suggestedStart, int suggestedEnd, int suggestedIncrement)
        {
            Min = min;
            Max = max;
            Default = defaultValue;
            SuggestedStart = suggestedStart;
            SuggestedEnd = suggestedEnd;
            SuggestedIncrement = suggestedIncrement;
        }

        /// <summary>The smallest accepted value.</summary>
        public int Min { get; }

        /// <summary>The largest accepted value.</summary>
        public int Max { get; }

        /// <summary>The documented default.</summary>
        public int Default { get; }

        /// <summary>The first value a parameter sweep should try.</summary>
        public int SuggestedStart { get; }

        /// <summary>The last value a parameter sweep should try.</summary>
        public int SuggestedEnd { get; }

        /// <summary>The step between swept values.</summary>
        public int SuggestedIncrement { get; }

        /// <inheritdoc/>
        public override double DefaultValue => Default;

        /// <summary>The suggested sweep values, low to high.</summary>
        /// <returns><see cref="Default"/> alone when <see cref="SuggestedIncrement"/>
        /// is not positive.</returns>
        public IEnumerable<int> Sweep()
        {
            if (SuggestedIncrement <= 0)
            {
                yield return Default;
                yield break;
            }

            for (long v = SuggestedStart; v <= SuggestedEnd; v += SuggestedIncrement)
            {
                yield return (int)v;
            }
        }
    }

    /// <summary>A fixed set of named integer choices.</summary>
    public sealed record IntegerList : OptInputDomain
    {
        internal IntegerList(ImmutableArray<NamedValue> values, long defaultValue)
            => (Values, Default) = (values, defaultValue);

        /// <summary>The choices, in declaration order.</summary>
        public ImmutableArray<NamedValue> Values { get; }

        /// <summary>The documented default value (a value, not an index).</summary>
        public long Default { get; }

        /// <inheritdoc/>
        public override double DefaultValue => Default;

        /// <summary>The list in C's <c>"0=SMA;1=EMA;..."</c> form.</summary>
        /// <returns>Semicolon-separated <c>value=name</c> pairs.</returns>
        public string ToValueListString() => string.Join(";", Values.Select(v => $"{v.Value}={v.Name}"));
    }

    /// <summary>A fixed set of named real choices.</summary>
    /// <remarks>Declared by no shipped function. Modelled anyway, so that the day
    /// one appears every consumer's <c>switch</c> has to account for it rather
    /// than silently treating it as a range.</remarks>
    public sealed record RealList : OptInputDomain
    {
        internal RealList(ImmutableArray<NamedRealValue> values, double defaultValue)
            => (Values, Default) = (values, defaultValue);

        /// <summary>The choices, in declaration order.</summary>
        public ImmutableArray<NamedRealValue> Values { get; }

        /// <summary>The documented default value.</summary>
        public double Default { get; }

        /// <inheritdoc/>
        public override double DefaultValue => Default;

        /// <summary>The list in C's <c>"value=name;..."</c> form.</summary>
        /// <returns>Semicolon-separated <c>value=name</c> pairs.</returns>
        public string ToValueListString() => string.Join(";", Values.Select(v => $"{v.Value}={v.Name}"));
    }
}

/// <summary>One required input of a function.</summary>
public sealed record InputInfo
{
    internal InputInfo(InputKind kind, string paramName, PriceComponents components,
                       ImmutableArray<PriceComponents> signatureOrder)
    {
        Kind = kind;
        ParamName = paramName;
        Components = components;
        SignatureOrder = signatureOrder;
    }

    /// <summary>What the input carries.</summary>
    public InputKind Kind { get; }

    /// <summary>The parameter's name, for example <c>inReal</c> or <c>inPriceHLC</c>.</summary>
    public string ParamName { get; }

    /// <summary>For a <see cref="InputKind.Price"/> input, the components it
    /// consumes; otherwise <see cref="PriceComponents.None"/>.</summary>
    public PriceComponents Components { get; }

    /// <summary>The components in the order the typed method takes them as
    /// separate arrays. Empty for a non-price input.</summary>
    public ImmutableArray<PriceComponents> SignatureOrder { get; }

    /// <summary>Whether this input requires a given price component.</summary>
    /// <param name="component">The component to test for.</param>
    /// <returns><see langword="true"/> when the function needs it.</returns>
    public bool Requires(PriceComponents component) => (Components & component) == component;
}

/// <summary>One optional parameter of a function.</summary>
public sealed record OptInputInfo
{
    internal OptInputInfo(string paramName, string displayName, string hint,
                          OptInputFlags flags, OptInputDomain domain)
    {
        ParamName = paramName;
        DisplayName = displayName;
        Hint = hint;
        Flags = flags;
        Domain = domain;
    }

    /// <summary>The parameter's name, for example <c>optInTimePeriod</c>.</summary>
    public string ParamName { get; }

    /// <summary>A short label for a user interface.</summary>
    public string DisplayName { get; }

    /// <summary>Longer help text. Empty when the definition declares none.</summary>
    public string Hint { get; }

    /// <summary>Presentation hints.</summary>
    public OptInputFlags Flags { get; }

    /// <summary>The values this parameter accepts.</summary>
    public OptInputDomain Domain { get; }

    /// <summary>Shorthand for <see cref="OptInputDomain.DefaultValue"/>.</summary>
    public double DefaultValue => Domain.DefaultValue;
}

/// <summary>One output of a function.</summary>
public sealed record OutputInfo
{
    internal OutputInfo(OutputKind kind, string paramName, OutputFlags flags)
    {
        Kind = kind;
        ParamName = paramName;
        Flags = flags;
    }

    /// <summary>Whether the series is real or integer.</summary>
    public OutputKind Kind { get; }

    /// <summary>The parameter's name, for example <c>outRealUpperBand</c>.</summary>
    public string ParamName { get; }

    /// <summary>Drawing and semantic hints.</summary>
    public OutputFlags Flags { get; }
}

/// <summary>Everything the library knows about one indicator.</summary>
public sealed record FunctionInfo
{
    internal FunctionInfo(string name, FunctionGroup group, string hint,
                          FunctionFlags flags, FuncUnstId? unstableId,
                          ImmutableArray<InputInfo> inputs,
                          ImmutableArray<OptInputInfo> optInputs,
                          ImmutableArray<OutputInfo> outputs,
                          LookbackThunk lookback, InvokeThunk invoke)
    {
        Name = name;
        Group = group;
        Hint = hint;
        Flags = flags;
        UnstableId = unstableId;
        Inputs = inputs;
        OptInputs = optInputs;
        Outputs = outputs;
        Lookback = lookback;
        Invoke = invoke;
    }

    /// <summary>The canonical name, for example <c>"BBANDS"</c>. It is also the
    /// name of the <see cref="Core"/> method that computes the function.</summary>
    public string Name { get; }

    /// <summary>The category the function belongs to.</summary>
    public FunctionGroup Group { get; }

    /// <summary>A one-line description. Empty when the definition declares none.</summary>
    public string Hint { get; }

    /// <summary>Behavioural properties of the function.</summary>
    public FunctionFlags Flags { get; }

    /// <summary>The function's unstable-period identity, or <see langword="null"/>
    /// when it has none. Non-null exactly when
    /// <see cref="FunctionFlags.UnstablePeriod"/> is set.</summary>
    public FuncUnstId? UnstableId { get; }

    /// <summary>The required inputs, in call order. Price components are folded
    /// into one bundle, so <c>ADX</c> has <b>one</b> input, not three — see
    /// <see cref="InputInfo.SignatureOrder"/>.</summary>
    public ImmutableArray<InputInfo> Inputs { get; }

    /// <summary>The optional parameters, in call order.</summary>
    public ImmutableArray<OptInputInfo> OptInputs { get; }

    /// <summary>The outputs, in call order.</summary>
    public ImmutableArray<OutputInfo> Outputs { get; }

    internal LookbackThunk Lookback { get; }

    internal InvokeThunk Invoke { get; }

    /// <summary>Begins a call whose arguments are bound at run time.</summary>
    /// <returns>A fresh, unbound call against <see cref="Core"/>'s defaults.</returns>
    public FunctionCall CreateCall() => new(this, new Core());

    /// <summary>Begins a call against a specific <see cref="Core"/>.</summary>
    /// <param name="core">The core whose settings the call should use.</param>
    /// <returns>A fresh, unbound call.</returns>
    public FunctionCall CreateCall(Core core) => new(this, core);

    /// <summary>The function's name.</summary>
    /// <returns><see cref="Name"/>.</returns>
    public override string ToString() => Name;
}
"#;

const FUNCTION_CALL: &str = r#"using System;
using System.Collections.Immutable;

namespace TALib.Metadata;

/// <summary>
/// Binds arguments to a function chosen at run time, then calls it.
/// </summary>
/// <remarks>
/// The replacement for C's <c>TA_ParamHolder</c> allocate/set/call/free dance —
/// there is nothing to free, and every binding is checked against the
/// <see cref="FunctionInfo"/> row it belongs to. Obtain one from
/// <see cref="FunctionInfo.CreateCall()"/>:
/// <code>
/// var f = FunctionCatalog.Default["SMA"];
/// var range = f.CreateCall()
///     .SetInput(0, close)
///     .SetOption(0, 30)
///     .SetOutput(0, outReal)
///     .Invoke(0, close.Length - 1);
/// </code>
/// <para>An index out of range, a type that does not match the declared
/// parameter, or an unbound input or output at call time throws
/// <see cref="ArgumentException"/>. Optional parameters left unbound take their
/// documented defaults.</para>
/// <para>Not thread-safe: confine one call object to one thread, or build one
/// per call. The <see cref="FunctionCatalog"/> it comes from is immutable and
/// shared freely.</para>
/// </remarks>
public sealed class FunctionCall
{
    /* The cross-language "use the documented default" sentinels. Kept here
       rather than read from the row so an unbound parameter takes exactly the
       path an explicit sentinel would — the two are then the same code path,
       which is what the shipped sentinel test asserts. */
    private const int IntDefault = int.MinValue;
    private const double RealDefault = -4e37;

    private readonly FunctionInfo _info;
    private readonly Core _core;

    private readonly double[]?[] _series;
    private readonly int[]?[] _intSeries;
    private readonly double[]?[][] _price;      // [slot][component index] in OHLCV order

    private readonly double[] _realOpts;
    private readonly int[] _intOpts;

    private readonly double[]?[] _realOuts;
    private readonly int[]?[] _intOuts;

    internal FunctionCall(FunctionInfo info, Core core)
    {
        _info = info;
        _core = core;
        int ni = info.Inputs.Length;
        _series = new double[ni][];
        _intSeries = new int[ni][];
        _price = new double[ni][][];
        for (int i = 0; i < ni; i++)
        {
            _price[i] = new double[6][];
        }

        int no = info.OptInputs.Length;
        _realOpts = new double[no];
        _intOpts = new int[no];
        for (int i = 0; i < no; i++)
        {
            _realOpts[i] = RealDefault;

            /* An unbound parameter means "the documented default", and the
               sentinel is how every integer domain says that — choice lists
               included since issue #162 taught the typed API to substitute
               `(MAType)int.MinValue`. Leaving it to the sentinel keeps the
               unbound path and an explicitly-passed sentinel on one code path,
               and makes this binder exercise that substitution rather than
               route around it. */
            _intOpts[i] = IntDefault;
        }

        int nout = info.Outputs.Length;
        _realOuts = new double[nout][];
        _intOuts = new int[nout][];
    }

    /// <summary>The function this call runs.</summary>
    public FunctionInfo Info => _info;

    private static int ComponentIndex(PriceComponents c) => c switch
    {
        PriceComponents.Open => 0,
        PriceComponents.High => 1,
        PriceComponents.Low => 2,
        PriceComponents.Close => 3,
        PriceComponents.Volume => 4,
        PriceComponents.OpenInterest => 5,
        _ => throw new ArgumentException($"{c} is not a single price component", nameof(c)),
    };

    private InputInfo CheckInput(int slot, InputKind expected)
    {
        if (slot < 0 || slot >= _info.Inputs.Length)
        {
            throw new ArgumentOutOfRangeException(nameof(slot),
                $"{_info.Name}: input {slot} is outside [0, {_info.Inputs.Length})");
        }

        InputInfo info = _info.Inputs[slot];
        if (info.Kind != expected)
        {
            throw new ArgumentException(
                $"{_info.Name} input {slot} ({info.ParamName}) is {info.Kind}, not {expected}", nameof(slot));
        }

        return info;
    }

    /// <summary>Binds a <see cref="InputKind.Real"/> input.</summary>
    /// <param name="slot">The input's index.</param>
    /// <param name="series">The series. Not copied.</param>
    /// <returns>This call, for chaining.</returns>
    /// <exception cref="ArgumentException">The slot is not a real input.</exception>
    public FunctionCall SetInput(int slot, double[] series)
    {
        CheckInput(slot, InputKind.Real);
        ArgumentNullException.ThrowIfNull(series);
        _series[slot] = series;
        return this;
    }

    /// <summary>Binds an <see cref="InputKind.Integer"/> input.</summary>
    /// <param name="slot">The input's index.</param>
    /// <param name="series">The series. Not copied.</param>
    /// <returns>This call, for chaining.</returns>
    /// <exception cref="ArgumentException">The slot is not an integer input.</exception>
    public FunctionCall SetInput(int slot, int[] series)
    {
        CheckInput(slot, InputKind.Integer);
        ArgumentNullException.ThrowIfNull(series);
        _intSeries[slot] = series;
        return this;
    }

    /// <summary>Binds one component of a <see cref="InputKind.Price"/> input.</summary>
    /// <param name="slot">The input's index.</param>
    /// <param name="component">A single component, for example
    /// <see cref="PriceComponents.High"/>.</param>
    /// <param name="series">The series. Not copied.</param>
    /// <returns>This call, for chaining.</returns>
    /// <exception cref="ArgumentException">The slot is not a price input.</exception>
    /// <remarks>A component the function does not consume is accepted and ignored,
    /// matching C's <c>TA_SetInputParamPricePtr</c> (whose <c>SET_PARAM_INFO</c>
    /// stores only the flagged components) and Java's <c>ParamHolder</c>. Do not
    /// make it throw: that reads like the stricter, safer choice and is not — no
    /// function in the catalogue consumes <see cref="PriceComponents.OpenInterest"/>,
    /// so the natural generic call (hand the binder a whole OHLCV bundle and let
    /// it take what it needs) would throw for every price function here while
    /// working against C and Java. Rejecting a MISSING required component is the
    /// check that earns its keep, and all three backends do it.</remarks>
    public FunctionCall SetPriceInput(int slot, PriceComponents component, double[] series)
    {
        InputInfo info = CheckInput(slot, InputKind.Price);
        ArgumentNullException.ThrowIfNull(series);
        _price[slot][ComponentIndex(component)] = series;
        return this;
    }

    /// <summary>Binds a price input from named component series.</summary>
    /// <param name="slot">The input's index.</param>
    /// <param name="open">The open series, when consumed.</param>
    /// <param name="high">The high series, when consumed.</param>
    /// <param name="low">The low series, when consumed.</param>
    /// <param name="close">The close series, when consumed.</param>
    /// <param name="volume">The volume series, when consumed.</param>
    /// <param name="openInterest">The open-interest series, when consumed.</param>
    /// <returns>This call, for chaining.</returns>
    /// <exception cref="ArgumentException">A consumed component was not supplied.
    /// A component the function ignores is accepted — see the single-component
    /// overload's remarks.</exception>
    /// <remarks>Validates every consumed component before writing any of them, so
    /// a rejection leaves this call exactly as it found it (issue #266).
    /// Interleaved, it committed the components ahead of the offending one, and a
    /// caller re-binding an already-good bundle then got <c>Success</c> over a
    /// mixture of the two — no code, no exception, wrong numbers.</remarks>
    public FunctionCall SetPriceInput(int slot, double[]? open = null, double[]? high = null,
                                      double[]? low = null, double[]? close = null,
                                      double[]? volume = null, double[]? openInterest = null)
    {
        InputInfo info = CheckInput(slot, InputKind.Price);
        double[]?[] given = [open, high, low, close, volume, openInterest];
        PriceComponents[] all =
        [
            PriceComponents.Open, PriceComponents.High, PriceComponents.Low,
            PriceComponents.Close, PriceComponents.Volume, PriceComponents.OpenInterest,
        ];

        for (int i = 0; i < all.Length; i++)
        {
            if (info.Requires(all[i]) && given[i] is null)
            {
                throw new ArgumentException(
                    $"{_info.Name} input {slot} ({info.ParamName}) requires {all[i]}", nameof(slot));
            }
        }

        /* An unconsumed component is stored and ignored -- see the remark on the
           single-component overload. */
        for (int i = 0; i < all.Length; i++)
        {
            _price[slot][i] = given[i];
        }

        return this;
    }

    private OptInputInfo CheckOpt(int index)
    {
        if (index < 0 || index >= _info.OptInputs.Length)
        {
            throw new ArgumentOutOfRangeException(nameof(index),
                $"{_info.Name}: optional parameter {index} is outside [0, {_info.OptInputs.Length})");
        }

        return _info.OptInputs[index];
    }

    /// <summary>Binds an integer-domain optional parameter.</summary>
    /// <param name="index">The parameter's index.</param>
    /// <param name="value">The value.</param>
    /// <returns>This call, for chaining.</returns>
    /// <exception cref="ArgumentException">The parameter's domain is not integral.</exception>
    public FunctionCall SetOption(int index, int value)
    {
        OptInputInfo p = CheckOpt(index);
        if (p.Domain is not (OptInputDomain.IntegerRange or OptInputDomain.IntegerList))
        {
            throw new ArgumentException(
                $"{_info.Name} parameter {index} ({p.ParamName}) is {p.Domain.GetType().Name}, not integral",
                nameof(index));
        }

        _intOpts[index] = value;
        return this;
    }

    /// <summary>Binds a real-domain optional parameter.</summary>
    /// <param name="index">The parameter's index.</param>
    /// <param name="value">The value.</param>
    /// <returns>This call, for chaining.</returns>
    /// <exception cref="ArgumentException">The parameter's domain is not real.</exception>
    public FunctionCall SetOption(int index, double value)
    {
        OptInputInfo p = CheckOpt(index);
        if (p.Domain is not (OptInputDomain.RealRange or OptInputDomain.RealList))
        {
            throw new ArgumentException(
                $"{_info.Name} parameter {index} ({p.ParamName}) is {p.Domain.GetType().Name}, not real",
                nameof(index));
        }

        _realOpts[index] = value;
        return this;
    }

    /// <summary>Binds a moving-average-type parameter.</summary>
    /// <param name="index">The parameter's index.</param>
    /// <param name="value">The moving-average type.</param>
    /// <returns>This call, for chaining.</returns>
    /// <exception cref="ArgumentException">The parameter is not a choice list.</exception>
    public FunctionCall SetOption(int index, MAType value)
    {
        /* Not a delegation to SetOption(int, int): that accepts an
           IntegerRange too, so `SetOption(0, MAType.EMA)` on SMA would bind a
           period of 1 and return Success with silently wrong output. Java's
           ParamHolder.setOptInput(int, MAType) checks INTEGER_LIST for the same
           reason, and no moving-average parameter is an IntegerRange, so this
           cannot reject a legitimate call. */
        OptInputInfo p = CheckOpt(index);
        if (p.Domain is not OptInputDomain.IntegerList)
        {
            throw new ArgumentException(
                $"{_info.Name} parameter {index} ({p.ParamName}) is {p.Domain.GetType().Name}, not a choice list",
                nameof(index));
        }

        _intOpts[index] = (int)value;
        return this;
    }

    /// <summary>Binds a parameter from its metadata row and a numeric value.</summary>
    /// <param name="parameter">A row from <see cref="FunctionInfo.OptInputs"/>.</param>
    /// <param name="value">The value, converted to the parameter's own type.</param>
    /// <returns>This call, for chaining.</returns>
    /// <exception cref="ArgumentException">The row does not belong to this function.</exception>
    public FunctionCall SetParam(OptInputInfo parameter, double value)
    {
        ArgumentNullException.ThrowIfNull(parameter);
        int index = _info.OptInputs.IndexOf(parameter);
        if (index < 0)
        {
            throw new ArgumentException(
                $"{parameter.ParamName} is not a parameter of {_info.Name}", nameof(parameter));
        }

        return parameter.Domain switch
        {
            OptInputDomain.RealRange or OptInputDomain.RealList => SetOption(index, value),
            _ => SetOption(index, ToIntegerOperand(parameter, value)),
        };
    }

    /* `(int)value` on a double outside the int range is unspecified in ECMA-334.
       .NET saturates: a large POSITIVE value lands on int.MaxValue, a large
       NEGATIVE one on int.MinValue -- and int.MinValue IS the "use the default"
       sentinel. So SetParam(p, -1e18) silently meant "use the default", and
       SetParam(p, 1e18) silently meant a period of 2147483647, where the caller
       plainly meant an error in both cases; NaN lands on 0. Reject what the
       integer domain cannot represent. The sentinel itself stays reachable:
       asking for the default IS a legal request (issue #162). */
    private static int ToIntegerOperand(OptInputInfo parameter, double value)
    {
        if (double.IsNaN(value) || value < int.MinValue || value > int.MaxValue)
        {
            throw new ArgumentOutOfRangeException(nameof(value),
                $"{parameter.ParamName}: {value} is outside the range an integer parameter can hold");
        }

        return (int)value;
    }

    private OutputInfo CheckOutput(int index, OutputKind expected)
    {
        if (index < 0 || index >= _info.Outputs.Length)
        {
            throw new ArgumentOutOfRangeException(nameof(index),
                $"{_info.Name}: output {index} is outside [0, {_info.Outputs.Length})");
        }

        OutputInfo info = _info.Outputs[index];
        if (info.Kind != expected)
        {
            throw new ArgumentException(
                $"{_info.Name} output {index} ({info.ParamName}) is {info.Kind}, not {expected}", nameof(index));
        }

        return info;
    }

    /// <summary>Binds a real output buffer.</summary>
    /// <param name="index">The output's index.</param>
    /// <param name="buffer">The buffer to write into.</param>
    /// <returns>This call, for chaining.</returns>
    /// <exception cref="ArgumentException">The output is not real.</exception>
    public FunctionCall SetOutput(int index, double[] buffer)
    {
        CheckOutput(index, OutputKind.Real);
        ArgumentNullException.ThrowIfNull(buffer);
        _realOuts[index] = buffer;
        return this;
    }

    /// <summary>Binds an integer output buffer.</summary>
    /// <param name="index">The output's index.</param>
    /// <param name="buffer">The buffer to write into.</param>
    /// <returns>This call, for chaining.</returns>
    /// <exception cref="ArgumentException">The output is not integral.</exception>
    public FunctionCall SetOutput(int index, int[] buffer)
    {
        CheckOutput(index, OutputKind.Integer);
        ArgumentNullException.ThrowIfNull(buffer);
        _intOuts[index] = buffer;
        return this;
    }

    /// <summary>The number of leading bars this call's parameters consume.</summary>
    /// <returns>The lookback, or <c>-1</c> when a bound parameter is out of range.</returns>
    public int Lookback() => _info.Lookback(_core, this);

    /* Returns Success when every input and output is bound, or the code C's
       TA_CallFunc returns for the same condition. Split out of RequireBound so
       TryInvoke can honour its name: it advertises "failure as a code rather than
       an exception" and then threw from the binding it performs, which would take
       the C# server's process down on the first reject vector. */
    private RetCode BoundState() => BoundState(out _);

    /* `which` names the offending slot so Invoke can keep the diagnostic it used
       to throw ("SMA: input 0 (inReal) was not set"). TryInvoke discards it and
       returns the bare code. Reporting only the code from both would have made a
       mis-bound multi-input call materially harder to place -- the same class of
       problem the server hardening exists to fix. */
    private RetCode BoundState(out string which)
    {
        which = "";
        for (int i = 0; i < _info.Inputs.Length; i++)
        {
            InputInfo probe = _info.Inputs[i];
            bool ok = probe.Kind switch
            {
                InputKind.Real => _series[i] is not null,
                InputKind.Integer => _intSeries[i] is not null,
                _ => AllPriceComponentsBound(i, probe),
            };

            if (!ok)
            {
                which = $"input {i} ({probe.ParamName})";
                return RetCode.InputNotAllInitialize;
            }
        }

        for (int i = 0; i < _info.Outputs.Length; i++)
        {
            OutputInfo probe = _info.Outputs[i];
            if (probe.Kind == OutputKind.Real ? _realOuts[i] is null : _intOuts[i] is null)
            {
                which = $"output {i} ({probe.ParamName})";
                return RetCode.OutputNotAllInitialize;
            }
        }

        return RetCode.Success;
    }


    private bool AllPriceComponentsBound(int slot, InputInfo info)
    {
        PriceComponents[] all =
        [
            PriceComponents.Open, PriceComponents.High, PriceComponents.Low,
            PriceComponents.Close, PriceComponents.Volume, PriceComponents.OpenInterest,
        ];

        for (int i = 0; i < all.Length; i++)
        {
            if (info.Requires(all[i]) && _price[slot][i] is null)
            {
                return false;
            }
        }

        return true;
    }

    /// <summary>Runs the function over <c>[startIdx, endIdx]</c>.</summary>
    /// <param name="startIdx">First input bar to compute.</param>
    /// <param name="endIdx">Last input bar to compute.</param>
    /// <returns>Where the output starts and how much there is.</returns>
    /// <exception cref="ArgumentException">A required input or output was never
    /// bound, or an argument is invalid — the same failures the typed method
    /// reports.</exception>
    public OutRange Invoke(int startIdx, int endIdx)
    {
        RetCode bound = BoundState(out string which);
        if (bound != RetCode.Success)
        {
            throw new ArgumentException($"{_info.Name}: {which} was not set");
        }

        // The function's OWN exception, not a relabelled code. Since #265 the
        // thunk calls the public overload, whose message names the buffer and
        // both sizes and whose type carries the RetCode; going through
        // TryInvoke flattened that to "SMA failed: BadParam". TryInvoke exists
        // to hand back a code; this method's contract is the exception, so it
        // should be the real one -- which is what Java's ParamHolder.call does.
        return _info.Invoke(_core, this, startIdx, endIdx);
    }

    /// <summary>Runs the function, reporting failure as a code rather than an
    /// exception.</summary>
    /// <param name="startIdx">First input bar to compute.</param>
    /// <param name="endIdx">Last input bar to compute.</param>
    /// <param name="range">Where the output starts and how much there is.</param>
    /// <returns>The function's return code. An unbound input or output is reported
    /// as <see cref="RetCode.InputNotAllInitialize"/> /
    /// <see cref="RetCode.OutputNotAllInitialize"/>, the codes C returns for the
    /// same condition — this method does not throw.</returns>
    public RetCode TryInvoke(int startIdx, int endIdx, out OutRange range)
    {
        RetCode bound = BoundState();
        if (bound != RetCode.Success)
        {
            range = new OutRange(0, 0);
            return bound;
        }

        try
        {
            range = _info.Invoke(_core, this, startIdx, endIdx);
            return RetCode.Success;
        }
        catch (Exception _e) when (_e is ITaLibFailure)
        {
            // The one conversion point. Since #265 the thunk calls the function's
            // PUBLIC overload, like C's frames and Java's Dispatch, so every
            // rejection this method reports -- a bad parameter, a range out of
            // bounds, a buffer too short -- arrives here as a throw. It also
            // still catches what it was written for: a composed body cross-calls
            // its callee's public tier, `OutRange _xr0 = MA(startIdx, endIdx,
            // ...)` in APO, and that throws too. Only the library's own failure
            // is converted; anything else is not ours to relabel.
            range = new OutRange(0, 0);
            return ((ITaLibFailure)_e).RetCode;
        }
    }

    /* Accessors used by the generated thunks. Every one is reached only after
       BoundState(), so the null-forgiving operator is discharged there. */

    internal double[] Series(int slot) => _series[slot]!;

    internal int[] IntSeries(int slot) => _intSeries[slot]!;

    internal double[] Price(int slot, PriceComponents component) => _price[slot][ComponentIndex(component)]!;

    internal double RealOpt(int index) => _realOpts[index];

    internal int IntOpt(int index) => _intOpts[index];

    internal double[] RealOut(int index) => _realOuts[index]!;

    internal int[] IntOut(int index) => _intOuts[index]!;
}
"#;
