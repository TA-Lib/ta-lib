//! The **shipped** Java introspection registry — `io.github.talib.metadata`.
//!
//! Java's analog of Rust's generated `abstract_api.rs`: the same metadata the
//! JSON-RPC server's inline table carries, emitted into the library so an
//! application can enumerate functions and their parameters without a test
//! server. It replaces the hand-written 2007 `meta/` reflection island, which
//! reflected over `@FuncInfo` annotations, shipped empty function hints, and
//! could only describe the guarded double-precision batch API.
//!
//! Both surfaces render from [`abstract_rows::rows`](super::abstract_rows::rows),
//! so they cannot disagree: the values come from the same backend-neutral rows
//! the Rust and C# registries render, and `test_abstract.c` already diffs the
//! server's copy against C.
//!
//! Scope is the double-precision batch API — the same as the C and Rust abstract
//! layers. Describing streaming handles and `float[]` overloads is new schema
//! design no backend does yet, and is a deliberate follow-up.

use std::collections::HashMap;
use std::fmt::Write as _;
use std::path::Path;

use super::abstract_rows::{rows, FuncRow, InputKind, OptDomain, OutputKind};
use crate::ir::{EnumDef, FuncDef};

/// Java package (and directory) the registry is emitted into.
const PACKAGE: &str = "io.github.talib.metadata";

/// The license header every shipped Java file carries.
fn header(contributors: &str) -> String {
    format!(
        "/* TA-LIB Copyright (c) 1999-2026, Mario Fortier\n\
         \x20* All rights reserved.\n\
         \x20*\n\
         \x20* Redistribution and use in source and binary forms, with or\n\
         \x20* without modification, are permitted provided that the following\n\
         \x20* conditions are met:\n\
         \x20*\n\
         \x20* - Redistributions of source code must retain the above copyright\n\
         \x20*   notice, this list of conditions and the following disclaimer.\n\
         \x20*\n\
         \x20* - Redistributions in binary form must reproduce the above copyright\n\
         \x20*   notice, this list of conditions and the following disclaimer in\n\
         \x20*   the documentation and/or other materials provided with the\n\
         \x20*   distribution.\n\
         \x20*\n\
         \x20* - Neither name of author nor the names of its contributors\n\
         \x20*   may be used to endorse or promote products derived from this\n\
         \x20*   software without specific prior written permission.\n\
         \x20*\n\
         \x20* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS\n\
         \x20* ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT\n\
         \x20* LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS\n\
         \x20* FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE\n\
         \x20* REGENTS OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,\n\
         \x20* INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES\n\
         \x20* (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS\n\
         \x20* OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS\n\
         \x20* INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,\n\
         \x20* WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE\n\
         \x20* OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,\n\
         \x20* EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.\n\
         \x20*/\n\
         \n\
         /* GENERATED FILE — do not edit. Produced by ta_codegen\n\
         \x20* (generator/src/backends/java_metadata.rs) from ta_codegen/input/.\n\
         \x20* {contributors}\n\
         \x20*/\n\
         \n\
         package {PACKAGE};\n\n"
    )
}

/// The package's javadoc. Without a `package-info.java` the published javadoc
/// renders an empty description for the package page, and a first release is
/// the worst moment for that.
///
/// Built from `header` rather than beside it because a package javadoc must sit
/// immediately before the `package` declaration, where `header` already ends.
fn package_info() -> String {
    let h = header("MF,CC");
    let pkg = format!("package {PACKAGE};\n\n");
    let license = h
        .strip_suffix(&pkg)
        .expect("header() ends with the package declaration");
    format!(
        "{license}/**\n\
         \x20* Runtime introspection: what the library's functions are, what they take,\n\
         \x20* and how to call one whose name is not known until run time.\n\
         \x20*\n\
         \x20* <p>The Java face of the C library's {{@code ta_abstract}} layer, and\n\
         \x20* generated from the same definitions as the indicators themselves, so a\n\
         \x20* row here cannot describe a method that does not exist.\n\
         \x20*\n\
         \x20* <p>{{@link {PACKAGE}.Functions#all()}} enumerates every function;\n\
         \x20* {{@link {PACKAGE}.Functions#byName(java.lang.String)}} looks one up.\n\
         \x20* Each {{@link {PACKAGE}.FunctionInfo}} carries its group, its flags and its\n\
         \x20* input/optional-input/output descriptors, and mints a\n\
         \x20* {{@link {PACKAGE}.ParamHolder}} to bind arguments into:\n\
         \x20*\n\
         \x20* <pre>{{@code\n\
         \x20* FunctionInfo rsi = Functions.byName(\"RSI\");\n\
         \x20* double[] out = new double[close.length];\n\
         \x20* OutRange r = rsi.newCall()\n\
         \x20*     .setInput(0, close)\n\
         \x20*     .setOptInput(0, 14)\n\
         \x20*     .setOutput(0, out)\n\
         \x20*     .call(0, close.length - 1);\n\
         \x20* }}</pre>\n\
         \x20*\n\
         \x20* <p>Binding is checked, not reflective: the dispatch onto the typed method\n\
         \x20* is a generated {{@code switch}}, so the library stays AOT- and\n\
         \x20* jlink-friendly and a signature change is a compile error rather than a\n\
         \x20* run-time one. The typed API on {{@link io.github.talib.Core}} remains the\n\
         \x20* faster and safer choice whenever the function is known at compile time.\n\
         \x20*/\n\
         {pkg}"
    )
}

/// Emit `s` as a Java string literal (escaping `\"` and `\\`).
fn js(s: &str) -> String {
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

/// Format an `f64` as a valid Java `double` literal (shortest round-tripping form).
fn jd(v: f64) -> String {
    format!("{v:?}")
}

/// Generate the whole `io.github.talib.metadata` package into `lib_src`
/// (`.../java/library/src`).
#[allow(clippy::implicit_hasher)]
pub fn generate(funcs: &[FuncDef], enums: &HashMap<String, EnumDef>, lib_src: &Path) {
    let dir = lib_src.join(PACKAGE.replace('.', "/"));
    std::fs::create_dir_all(&dir).unwrap_or_else(|e| panic!("creating {}: {e}", dir.display()));

    let rows = rows(funcs, enums);

    // ParamHolder binds a choice list through `MAType.values()[value]`, i.e. it
    // treats the declared VALUE as an enum ordinal, and range-checks against
    // `MAType.values().length`. So the assumption is not merely "dense from zero"
    // — a dense SUBSET of MAType would still let setOptInput(idx, 7) bind a member
    // the list never declared. Assert the whole invariant (same length, same
    // members, same order, and a default among its own choices) and fail the build
    // where the fix is obvious. C# guards the equivalent assumption; Java did not,
    // so a list with a gap silently bound the wrong member (issue #164).
    let matype: Vec<&str> = enums
        .get("MAType")
        .map(|e| e.variants.iter().map(|v| v.name.as_str()).collect())
        .unwrap_or_default();
    for f in &rows {
        for opt in &f.opt_inputs {
            if let OptDomain::IntegerList { values, default } = &opt.domain {
                assert_eq!(
                    values.len(),
                    matype.len(),
                    "{}.{}: choice list has {} entries but MAType has {}. ParamHolder \
                     resolves these through MAType.values()[value]; teach it the declared \
                     values before shipping a list that is not MAType.",
                    f.name, opt.param_name, values.len(), matype.len()
                );
                for (i, (v, name)) in values.iter().enumerate() {
                    assert_eq!(
                        *v, i64::try_from(i).unwrap(),
                        "{}.{}: choice list is not dense from zero ({name} = {v} at position {i}).",
                        f.name, opt.param_name
                    );
                    assert_eq!(
                        name.as_str(), matype[i],
                        "{}.{}: choice list entry {i} is {name}, MAType's is {}. The binder \
                         maps value -> MAType ordinal, so the two must agree member for member.",
                        f.name, opt.param_name, matype[i]
                    );
                }
                assert!(
                    values.iter().any(|(v, _)| v == default),
                    "{}.{}: declared default {default} is not one of its own choices — \
                     MAType.values()[{default}] would be an out-of-bounds bind.",
                    f.name, opt.param_name
                );
            }
        }
    }

    write(&dir, "package-info.java", &package_info());
    write(&dir, "InputType.java", &input_type_enum());
    write(&dir, "OptInputType.java", &opt_input_type_enum());
    write(&dir, "OutputType.java", &output_type_enum());
    write(&dir, "FuncFlags.java", &func_flags_class());
    write(&dir, "InputFlags.java", &input_flags_class());
    write(&dir, "OptInputFlags.java", &opt_input_flags_class());
    write(&dir, "OutputFlags.java", &output_flags_class());
    write(&dir, "InputInfo.java", &input_info_record());
    write(&dir, "OptInputInfo.java", &opt_input_info_record());
    write(&dir, "OutputInfo.java", &output_info_record());
    write(&dir, "FunctionInfo.java", &function_info_record());
    write(&dir, "Functions.java", &functions_registry(&rows));
    write(&dir, "ParamHolder.java", &param_holder_class());
    write(&dir, "Dispatch.java", &dispatch_class(&rows));
    write(&dir, "FunctionDescription.java", &function_description_class(funcs));

    println!("  java metadata registry -> {} ({} functions)", dir.display(), rows.len());
}

/// `TA_FunctionDescriptionXML`'s analog, carrying the real XML.
///
/// Ships the real XML, never a `(length, checksum)` pair baked at generation
/// time: `test_abstract.c` compares it against C's actual bytes, and a constant
/// the generator computed from the same string C's table is built from is the
/// generator agreeing with itself — a gate that cannot fail (#164).
///
/// Split across parts because a `CONSTANT_Utf8` entry is length-prefixed with a
/// `u2`: one 195 KB literal does not fit in a class file. They are joined at
/// runtime rather than with `+`, which javac would fold straight back into the
/// single oversized constant.
fn function_description_class(funcs: &[FuncDef]) -> String {
    const CHUNK: usize = 50_000;
    let xml = super::func_api_xml::generate_string(funcs);
    let bytes: Vec<u8> = xml.bytes().collect();
    assert!(bytes.is_ascii(), "ta_func_api.xml is no longer ASCII; the chunking below splits on bytes");

    let mut s = header("MF,CC");
    s.push_str(
        "/**\n\
         \x20* The machine-readable description of every function, as XML.\n\
         \x20*\n\
         \x20* <p>The Java analog of C's {@code TA_FunctionDescriptionXML()}. Same bytes:\n\
         \x20* both are emitted by one generator from one set of definitions.\n\
         \x20*/\n\
         public final class FunctionDescription {\n\
         \x20   private FunctionDescription() {\n\
         \x20   }\n\n",
    );

    let parts: Vec<&[u8]> = bytes.chunks(CHUNK).collect();
    for (i, part) in parts.iter().enumerate() {
        let _ = write!(s, "    private static final String P{i} = \"");
        for &b in *part {
            match b {
                b'"' => s.push_str("\\\""),
                b'\\' => s.push_str("\\\\"),
                b'\n' => s.push_str("\\n"),
                b'\r' => s.push_str("\\r"),
                b'\t' => s.push_str("\\t"),
                _ => s.push(b as char),
            }
        }
        s.push_str("\";\n\n");
    }

    let _ = writeln!(s, "    private static final String[] PARTS = {{");
    for i in 0..parts.len() {
        let _ = writeln!(s, "        P{i},");
    }
    s.push_str("    };\n\n");
    s.push_str(
        "    /**\n\
         \x20    * The full XML description of every function.\n\
         \x20    *\n\
         \x20    * @return the XML document, identical to what C's\n\
         \x20    *         {@code TA_FunctionDescriptionXML()} returns\n\
         \x20    */\n\
         \x20   public static String xml() {\n\
         \x20       StringBuilder sb = new StringBuilder();\n\
         \x20       for (String p : PARTS) {\n\
         \x20           sb.append(p);\n\
         \x20       }\n\
         \x20       return sb.toString();\n\
         \x20   }\n\
         }\n",
    );
    s
}

fn write(dir: &Path, name: &str, body: &str) {
    super::write_if_changed_silent(&dir.join(name), body);
}

// ---------------------------------------------------------------------------
// Enums / flag vocabularies
// ---------------------------------------------------------------------------

fn input_type_enum() -> String {
    let mut s = header("MF,CC");
    s.push_str(
        "/**\n\
         \x20* What an indicator consumes. Mirrors C's {@code TA_InputParameterType}.\n\
         \x20*/\n\
         public enum InputType {\n\
         \x20   /** A bundle of OHLCV components; see {@link InputInfo#flags()}. */\n\
         \x20   PRICE,\n\
         \x20   /** A single {@code double[]} series. */\n\
         \x20   REAL,\n\
         \x20   /** A single {@code int[]} series. */\n\
         \x20   INTEGER;\n\
         \n\
         \x20   /** The C type code (Price=0, Real=1, Integer=2). */\n\
         \x20   public int code() { return ordinal(); }\n\
         }\n",
    );
    s
}

fn opt_input_type_enum() -> String {
    let mut s = header("MF,CC");
    s.push_str(
        "/**\n\
         \x20* The domain of an optional parameter. Mirrors C's\n\
         \x20* {@code TA_OptInputParameterType} and selects which {@link OptInputInfo}\n\
         \x20* accessors carry meaning.\n\
         \x20*/\n\
         public enum OptInputType {\n\
         \x20   /** A {@code double} within {@code [min, max]}. */\n\
         \x20   REAL_RANGE,\n\
         \x20   /** A {@code double} from a fixed set (unused by the shipped functions). */\n\
         \x20   REAL_LIST,\n\
         \x20   /** An {@code int} within {@code [min, max]}. */\n\
         \x20   INTEGER_RANGE,\n\
         \x20   /** An {@code int} from the enumerated {@link OptInputInfo#valueList()}. */\n\
         \x20   INTEGER_LIST;\n\
         \n\
         \x20   /** The C type code (RealRange=0, RealList=1, IntegerRange=2, IntegerList=3). */\n\
         \x20   public int code() { return ordinal(); }\n\
         }\n",
    );
    s
}

fn output_type_enum() -> String {
    let mut s = header("MF,CC");
    s.push_str(
        "/**\n\
         \x20* What an indicator produces. Mirrors C's {@code TA_OutputParameterType}.\n\
         \x20*/\n\
         public enum OutputType {\n\
         \x20   /** A {@code double[]} series. */\n\
         \x20   REAL,\n\
         \x20   /** An {@code int[]} series (candlestick patterns, index outputs). */\n\
         \x20   INTEGER;\n\
         \n\
         \x20   /** The C type code (Real=0, Integer=1). */\n\
         \x20   public int code() { return ordinal(); }\n\
         }\n",
    );
    s
}

/// A `public static final int` constant block with a doc comment each.
fn flag_class(name: &str, doc: &str, consts: &[(&str, u32, &str)]) -> String {
    let mut s = header("MF,CC");
    let _ = write!(s, "/**\n * {doc}\n */\npublic final class {name} {{\n\n");
    let _ = write!(s, "   private {name}() {{ }}\n\n");
    for (cname, bits, cdoc) in consts {
        let _ = write!(s, "   /** {cdoc} */\n   public static final int {cname} = 0x{bits:08X};\n\n");
    }
    s.push_str("}\n");
    s
}

fn func_flags_class() -> String {
    flag_class(
        "FuncFlags",
        "Bit flags on a {@link FunctionInfo}. Values match C's {@code TA_FUNC_FLG_*}.",
        &[
            ("OVERLAP_STUDY", 0x0100_0000, "Output overlays the price chart."),
            ("STREAMING", 0x0200_0000, "A streaming (one-bar-at-a-time) API exists."),
            ("VOLUME_USED", 0x0400_0000, "Consumes volume."),
            ("UNSTABLE_PERIOD", 0x0800_0000, "Recursive: honours the unstable-period setting."),
            ("CANDLESTICK", 0x1000_0000, "A candlestick pattern."),
            (
                "PATH_DEPENDENT",
                0x2000_0000,
                "Output depends on where the caller started, so it never converges across ranges.",
            ),
            (
                "NAN_INF_OUTPUT",
                0x4000_0000,
                "Some inputs have no finite result, so a successful call can return NaN or \
                 +/-Infinity.",
            ),
            (
                "PERIOD1_IDENTITY",
                0x0000_0001,
                "A period of 1 performs no smoothing: the lookback is 0 and the output is a \
                 bit-exact copy of the input.",
            ),
        ],
    )
}

fn input_flags_class() -> String {
    flag_class(
        "InputFlags",
        "Which OHLCV components a {@link InputType#PRICE} input consumes. Values match C's \
         {@code TA_IN_PRICE_*}.",
        &[
            ("PRICE_OPEN", 0x0000_0001, "Open."),
            ("PRICE_HIGH", 0x0000_0002, "High."),
            ("PRICE_LOW", 0x0000_0004, "Low."),
            ("PRICE_CLOSE", 0x0000_0008, "Close."),
            ("PRICE_VOLUME", 0x0000_0010, "Volume."),
            ("PRICE_OPENINTEREST", 0x0000_0020, "Open interest."),
        ],
    )
}

fn opt_input_flags_class() -> String {
    flag_class(
        "OptInputFlags",
        "Display hints for an optional parameter. Values match C's {@code TA_OPTIN_*}.",
        &[
            ("IS_PERCENT", 0x0010_0000, "Expressed as a percentage."),
            ("IS_DEGREE", 0x0020_0000, "Expressed in degrees."),
            ("IS_CURRENCY", 0x0040_0000, "Expressed in currency."),
            ("ADVANCED", 0x0100_0000, "Advanced: hide from a basic UI."),
        ],
    )
}

fn output_flags_class() -> String {
    flag_class(
        "OutputFlags",
        "How an output is meant to be drawn, and whether it may be omitted. Values match C's \
         {@code TA_OUT_*}. The old hand-written island stopped at {@code ZERO} and left \
         consumers hardcoding the rest.",
        &[
            ("LINE", 0x0000_0001, "Draw as a continuous line."),
            ("DOT_LINE", 0x0000_0002, "Draw as a dotted line."),
            ("DASH_LINE", 0x0000_0004, "Draw as a dashed line."),
            ("DOT", 0x0000_0008, "Draw as unconnected dots."),
            ("HISTOGRAM", 0x0000_0010, "Draw as a histogram."),
            ("PATTERN_BOOL", 0x0000_0020, "0 = no pattern, 100 = pattern."),
            ("PATTERN_BULL_BEAR", 0x0000_0040, "-100 = bearish, 0 = none, 100 = bullish."),
            ("PATTERN_STRENGTH", 0x0000_0080, "-200..-100 = bearish, 100..200 = bullish."),
            ("POSITIVE", 0x0000_0100, "Always &gt;= 0."),
            ("NEGATIVE", 0x0000_0200, "Always &lt;= 0."),
            ("ZERO", 0x0000_0400, "Zero is a meaningful reference level."),
            ("UPPER_LIMIT", 0x0000_0800, "An upper band/limit line."),
            ("LOWER_LIMIT", 0x0000_1000, "A lower band/limit line."),
            (
                "NULLABLE",
                0x0000_2000,
                "Discardable: C accepts NULL for it. Java still requires an array.",
            ),
        ],
    )
}

// ---------------------------------------------------------------------------
// Row records
// ---------------------------------------------------------------------------

fn input_info_record() -> String {
    let mut s = header("MF,CC");
    s.push_str(
        "/**\n\
         \x20* One input of an indicator.\n\
         \x20*\n\
         \x20* @param type      what kind of series this is\n\
         \x20* @param paramName the parameter's name, e.g. {@code inReal} or {@code inPriceHLC}\n\
         \x20* @param flags     for {@link InputType#PRICE}, the OHLCV components consumed\n\
         \x20*                  (see {@link InputFlags}); otherwise 0\n\
         \x20*/\n\
         public record InputInfo(InputType type, String paramName, int flags) {\n\
         }\n",
    );
    s
}

fn output_info_record() -> String {
    let mut s = header("MF,CC");
    s.push_str(
        "/**\n\
         \x20* One output of an indicator.\n\
         \x20*\n\
         \x20* @param type      whether the series is real or integer\n\
         \x20* @param paramName the parameter's name, e.g. {@code outReal}\n\
         \x20* @param flags     drawing/semantic hints (see {@link OutputFlags})\n\
         \x20*/\n\
         public record OutputInfo(OutputType type, String paramName, int flags) {\n\
         }\n",
    );
    s
}

fn opt_input_info_record() -> String {
    let mut s = header("MF,CC");
    s.push_str(
        "/**\n\
         \x20* One optional parameter of an indicator, with its domain.\n\
         \x20*\n\
         \x20* <p>Which fields carry meaning depends on {@link #type()}:\n\
         \x20* {@link OptInputType#REAL_RANGE} uses {@code min}/{@code max}/{@code precision}\n\
         \x20* and the {@code suggested*} triple; {@link OptInputType#INTEGER_RANGE} uses\n\
         \x20* {@code intMin}/{@code intMax} and {@code intSuggested*};\n\
         \x20* {@link OptInputType#INTEGER_LIST} uses {@link #valueList()}.\n\
         \x20*\n\
         \x20* @param type         the parameter's domain\n\
         \x20* @param paramName    the parameter's name, e.g. {@code optInTimePeriod}\n\
         \x20* @param flags        display hints (see {@link OptInputFlags})\n\
         \x20* @param displayName  a human-readable label, e.g. {@code Time Period}\n\
         \x20* @param hint         per-parameter help text ({@code \"\"} when none)\n\
         \x20* @param defaultValue the value {@code Integer.MIN_VALUE} (or the real-default\n\
         \x20*                     sentinel) selects\n\
         \x20*/\n\
         public record OptInputInfo(\n\
         \x20      OptInputType type,\n\
         \x20      String paramName,\n\
         \x20      int flags,\n\
         \x20      String displayName,\n\
         \x20      String hint,\n\
         \x20      double defaultValue,\n\
         \x20      double min,\n\
         \x20      double max,\n\
         \x20      int precision,\n\
         \x20      double suggestedStart,\n\
         \x20      double suggestedEnd,\n\
         \x20      double suggestedIncrement,\n\
         \x20      int intMin,\n\
         \x20      int intMax,\n\
         \x20      int intSuggestedStart,\n\
         \x20      int intSuggestedEnd,\n\
         \x20      int intSuggestedIncrement,\n\
         \x20      String valueList) {\n\
         }\n",
    );
    s
}

fn function_info_record() -> String {
    let mut s = header("MF,CC");
    s.push_str(
        "import io.github.talib.Core;\n\
         import java.util.List;\n\n\
         /**\n\
         \x20* Everything the library knows about one indicator's guarded,\n\
         \x20* double-precision batch form.\n\
         \x20*\n\
         \x20* @param name          the canonical upper-case name, e.g. {@code SMA}\n\
         \x20* @param group         the functional group, e.g. {@code Overlap Studies}\n\
         \x20* @param hint          a one-line description ({@code \"\"} when none)\n\
         \x20* @param flags         see {@link FuncFlags}\n\
         \x20* @param inputs        inputs in call order\n\
         \x20* @param optInputs     optional parameters in call order\n\
         \x20* @param outputs       outputs in call order\n\
         \x20*/\n\
         public record FunctionInfo(\n\
         \x20      String name,\n\
         \x20      String group,\n\
         \x20      String hint,\n\
         \x20      int flags,\n\
         \x20      List<InputInfo> inputs,\n\
         \x20      List<OptInputInfo> optInputs,\n\
         \x20      List<OutputInfo> outputs) {\n\
         \n\
         \x20   /** Whether every bit in {@code mask} is set (see {@link FuncFlags}). */\n\
         \x20   public boolean hasFlags(int mask) {\n\
         \x20      return (flags & mask) == mask;\n\
         \x20   }\n\
         \n\
         \x20   /**\n\
         \x20    * Begins a call to this function with arguments bound at run time,\n\
         \x20    * against {@link Core#DEFAULT}. See {@link ParamHolder}.\n\
         \x20    */\n\
         \x20   public ParamHolder newCall() {\n\
         \x20      return new ParamHolder(this, Core.DEFAULT);\n\
         \x20   }\n\
         \n\
         \x20   /**\n\
         \x20    * Begins a call to this function with arguments bound at run time,\n\
         \x20    * against a specific {@link Core}. See {@link ParamHolder}.\n\
         \x20    */\n\
         \x20   public ParamHolder newCall(Core core) {\n\
         \x20      return new ParamHolder(this, core);\n\
         \x20   }\n\
         }\n",
    );
    s
}

// ---------------------------------------------------------------------------
// The registry itself
// ---------------------------------------------------------------------------

fn functions_registry(rows: &[FuncRow]) -> String {
    let mut s = header("MF,CC");
    s.push_str(
        "import java.util.Collections;\n\
         import java.util.LinkedHashMap;\n\
         import java.util.List;\n\
         import java.util.Locale;\n\
         import java.util.Map;\n\n\
         /**\n\
         \x20* The catalogue of every TA-Lib indicator, for applications that pick a\n\
         \x20* function at run time — a charting UI enumerating indicators, a backtester\n\
         \x20* with user-selectable studies, a parameter sweep.\n\
         \x20*\n\
         \x20* <p>Generated from the same definitions as the indicators themselves, so it\n\
         \x20* cannot drift from them. Immutable and safe to use from any thread.\n\
         \x20*\n\
         \x20* <pre>{@code\n\
         \x20* for (FunctionInfo f : Functions.all()) {\n\
         \x20*     System.out.println(f.name() + \" — \" + f.hint());\n\
         \x20* }\n\
         \x20* }</pre>\n\
         \x20*\n\
         \x20* <p>Scope is the double-precision batch API — the same surface C's\n\
         \x20* {@code ta_abstract} and Rust's {@code abstract_api} describe. Streaming\n\
         \x20* handles and {@code float[]} overloads are not catalogued.\n\
         \x20*/\n\
         public final class Functions {\n\n\
         \x20   private Functions() { }\n\n",
    );

    s.push_str("   private static final Map<String, FunctionInfo> BY_NAME = build();\n\n");
    s.push_str(
        "   /** Every function, in canonical name order. */\n\
         \x20  public static List<FunctionInfo> all() {\n\
         \x20     return List.copyOf(BY_NAME.values());\n\
         \x20  }\n\n\
         \x20  /**\n\
         \x20   * One function by name, matched case-insensitively, e.g. {@code \"SMA\"},\n\
         \x20   * {@code \"sma\"} or {@code \"Sma\"}. The metadata itself always reports the\n\
         \x20   * canonical upper-case spelling regardless of the case looked up.\n\
         \x20   *\n\
         \x20   * @param name the function's name, in any ASCII casing\n\
         \x20   * @return the metadata, or {@code null} if no such function exists\n\
         \x20   */\n\
         \x20  public static FunctionInfo byName(String name) {\n\
         \x20     return name == null ? null : BY_NAME.get(asciiUpper(name));\n\
         \x20  }\n\n\
         \x20  /**\n\
         \x20   * Upper-cases the ASCII letters of {@code s} and nothing else.\n\
         \x20   *\n\
         \x20   * <p>Not {@code String.toUpperCase}, with or without a {@code Locale}. The\n\
         \x20   * platform default has the Turkish-{@code i} bug in the narrowing\n\
         \x20   * direction; {@link Locale#ROOT} has it in the widening one, mapping the\n\
         \x20   * dotless {@code '\\u0131'} onto {@code 'I'} and the long {@code 's'}\n\
         \x20   * ({@code '\\u017F'}) onto {@code 'S'}, so {@code \"s\\u0131n\"} would resolve\n\
         \x20   * to SIN and {@code \"\\u017Fma\"} to SMA. Function names are invariant ASCII,\n\
         \x20   * so the fold that matches them is invariant ASCII too.\n\
         \x20   *\n\
         \x20   * <p>Returns {@code s} itself when it holds no lower-case ASCII letter,\n\
         \x20   * which is every call that already passes a canonical name.\n\
         \x20   */\n\
         \x20  private static String asciiUpper(String s) {\n\
         \x20     char[] out = null;\n\
         \x20     for (int i = 0; i < s.length(); i++) {\n\
         \x20        char c = s.charAt(i);\n\
         \x20        if (c >= 'a' && c <= 'z') {\n\
         \x20           if (out == null) {\n\
         \x20              out = s.toCharArray();\n\
         \x20           }\n\
         \x20           out[i] = (char) (c - ('a' - 'A'));\n\
         \x20        }\n\
         \x20     }\n\
         \x20     return out == null ? s : new String(out);\n\
         \x20  }\n\n\
         \x20  /** The distinct group names, in first-appearance order. */\n\
         \x20  public static List<String> groups() {\n\
         \x20     return BY_NAME.values().stream().map(FunctionInfo::group).distinct().toList();\n\
         \x20  }\n\n",
    );

    // The table. One private static method per function keeps each initializer
    // well under the 64 KB bytecode limit a single <clinit> would blow past.
    s.push_str("   private static Map<String, FunctionInfo> build() {\n");
    s.push_str("      Map<String, FunctionInfo> m = new LinkedHashMap<>();\n");
    for row in rows {
        let _ = writeln!(s, "      put(m, {}());", java_ident(&row.name));
    }
    s.push_str("      return Collections.unmodifiableMap(m);\n");
    s.push_str("   }\n\n");
    s.push_str("   private static void put(Map<String, FunctionInfo> m, FunctionInfo f) {\n");
    s.push_str("      m.put(f.name(), f);\n");
    s.push_str("   }\n\n");

    for row in rows {
        emit_function_factory(&mut s, row);
    }

    s.push_str("}\n");
    s
}

/// A Java identifier for a function's factory method (`TA` names are already
/// valid identifiers, but keep the mapping explicit and total).
fn java_ident(name: &str) -> String {
    let mut o = String::from("f_");
    for c in name.chars() {
        if c.is_ascii_alphanumeric() {
            o.push(c);
        } else {
            o.push('_');
        }
    }
    o
}

fn emit_function_factory(s: &mut String, f: &FuncRow) {
    let _ = writeln!(s, "   private static FunctionInfo {}() {{", java_ident(&f.name));
    let _ = writeln!(s, "      return new FunctionInfo(");
    let _ = writeln!(
        s,
        "         {}, {}, {}, 0x{:08X},",
        js(&f.name),
        js(f.group.as_str()),
        js(&f.hint),
        f.flags
    );

    if f.inputs.is_empty() {
        s.push_str("         List.of(),\n");
    } else {
        s.push_str("         List.of(\n");
        for (i, inp) in f.inputs.iter().enumerate() {
            let sep = if i + 1 == f.inputs.len() { "" } else { "," };
            let _ = writeln!(
                s,
                "            new InputInfo(InputType.{}, {}, 0x{:08X}){sep}",
                input_type_name(inp.kind),
                js(&inp.param_name),
                inp.flags
            );
        }
        s.push_str("         ),\n");
    }

    if f.opt_inputs.is_empty() {
        s.push_str("         List.of(),\n");
    } else {
        s.push_str("         List.of(\n");
        for (i, o) in f.opt_inputs.iter().enumerate() {
            let sep = if i + 1 == f.opt_inputs.len() { "" } else { "," };
            // Every domain's slots are carried positionally; the ones this
            // domain does not use stay literal 0 / null, and `type()` is what
            // tells a consumer which to read. The arms are spelled out rather
            // than caught by `_` so a new OptDomain variant fails to compile
            // here too: a catch-all would hand it zeros and a null value list,
            // which is a plausible-looking row rather than a build error.
            let (rmin, rmax, prec, rs, re, ri) = match &o.domain {
                OptDomain::RealRange { min, max, precision, suggested, .. } => {
                    (*min, *max, *precision, suggested.0, suggested.1, suggested.2)
                }
                OptDomain::IntegerRange { .. }
                | OptDomain::IntegerList { .. }
                | OptDomain::RealList { .. } => (0.0, 0.0, 0, 0.0, 0.0, 0.0),
            };
            let (imin, imax, is, ie, ii) = match &o.domain {
                OptDomain::IntegerRange { min, max, suggested, .. } => {
                    (*min, *max, suggested.0, suggested.1, suggested.2)
                }
                OptDomain::RealRange { .. }
                | OptDomain::IntegerList { .. }
                | OptDomain::RealList { .. } => (0, 0, 0, 0, 0),
            };
            let vl = match &o.domain {
                OptDomain::IntegerList { .. } | OptDomain::RealList { .. } => {
                    js(&o.domain.value_list_string())
                }
                OptDomain::RealRange { .. } | OptDomain::IntegerRange { .. } => "null".to_string(),
            };
            let _ = writeln!(s, "            new OptInputInfo(");
            let _ = writeln!(s, "               OptInputType.{}, {}, 0x{:08X},", opt_type_name(&o.domain), js(&o.param_name), o.flags);
            let _ = writeln!(s, "               {}, {}, {},", js(&o.display_name), js(&o.hint), jd(o.domain.default_as_f64()));
            let _ = writeln!(s, "               {}, {}, {prec}, {}, {}, {},", jd(rmin), jd(rmax), jd(rs), jd(re), jd(ri));
            let _ = writeln!(s, "               {imin}, {imax}, {is}, {ie}, {ii}, {vl}){sep}");
        }
        s.push_str("         ),\n");
    }

    if f.outputs.is_empty() {
        s.push_str("         List.of());\n");
    } else {
        s.push_str("         List.of(\n");
        for (i, out) in f.outputs.iter().enumerate() {
            let sep = if i + 1 == f.outputs.len() { "" } else { "," };
            let _ = writeln!(
                s,
                "            new OutputInfo(OutputType.{}, {}, 0x{:08X}){sep}",
                output_type_name(out.kind),
                js(&out.param_name),
                out.flags
            );
        }
        s.push_str("         ));\n");
    }
    s.push_str("   }\n\n");
}

fn input_type_name(kind: InputKind) -> &'static str {
    match kind {
        InputKind::Price => "PRICE",
        InputKind::Real => "REAL",
        InputKind::Integer => "INTEGER",
    }
}

fn opt_type_name(domain: &OptDomain) -> &'static str {
    match domain {
        OptDomain::RealRange { .. } => "REAL_RANGE",
        OptDomain::RealList { .. } => "REAL_LIST",
        OptDomain::IntegerRange { .. } => "INTEGER_RANGE",
        OptDomain::IntegerList { .. } => "INTEGER_LIST",
    }
}

fn output_type_name(kind: OutputKind) -> &'static str {
    match kind {
        OutputKind::Real => "REAL",
        OutputKind::Integer => "INTEGER",
    }
}

// ---------------------------------------------------------------------------
// Call-by-name: ParamHolder + a generated static switch
// ---------------------------------------------------------------------------
//
// C ships this as TA_ParamHolderAlloc -> TA_SetInputParam* -> TA_CallFunc, and
// the deleted `meta/` island served it with Method.invoke. Here the dispatch is a
// generated switch over the typed public wrappers: no reflection, so it survives
// AOT/jlink and cannot desync from the real signatures.

/// `ParamHolder.java`'s text, without writing it — so a test can assert on the
/// emitted binder.
pub fn render_param_holder() -> String {
    param_holder_class()
}

#[allow(clippy::too_many_lines)]
fn param_holder_class() -> String {
    let mut s = header("MF,CC");
    s.push_str(
        r#"import io.github.talib.Core;
import io.github.talib.MAType;
import io.github.talib.OutRange;

/**
 * Binds arguments to a function chosen at run time, then calls it.
 *
 * <p>The counterpart of C's {@code TA_ParamHolder}, for an application that does
 * not know at compile time which indicator it will run — a charting UI listing
 * every study, a parameter sweep. Obtain one from
 * {@link FunctionInfo#newCall()}:
 *
 * <pre>{@code
 * FunctionInfo f = Functions.byName("SMA");
 * OutRange r = f.newCall()
 *     .setInput(0, close)
 *     .setOptInput(0, 30)
 *     .setOutput(0, out)
 *     .call(0, close.length - 1);
 * }</pre>
 *
 * <p>Everything is validated against the {@link FunctionInfo} row: an index out
 * of bounds, a type that does not match the declared parameter, or an unset
 * parameter at {@link #call} time throws {@link IllegalArgumentException}. The
 * call itself then behaves exactly like the typed method — including throwing
 * on misuse and returning an empty {@link OutRange} when the range is shorter
 * than the lookback.
 *
 * <p>Not thread-safe: confine one holder to one thread, or build one per call.
 */
public final class ParamHolder {

   private final FunctionInfo info;
   private final Core core;

   /** Per input slot: a real series, an int series, or the six price components. */
   private final double[][] realInputs;
   private final int[][] intInputs;
   private final double[][][] priceInputs;   // [slot][component] in OHLCV order

   private final double[] realOpts;
   private final int[] intOpts;
   private final MAType[] maTypeOpts;
   private final boolean[] optSet;

   private final double[][] realOutputs;
   private final int[][] intOutputs;

   ParamHolder(FunctionInfo info, Core core) {
      this.info = info;
      this.core = core;
      int ni = info.inputs().size();
      int no = info.optInputs().size();
      int nout = info.outputs().size();
      this.realInputs = new double[ni][];
      this.intInputs = new int[ni][];
      this.priceInputs = new double[ni][][];
      this.realOpts = new double[no];
      this.intOpts = new int[no];
      this.maTypeOpts = new MAType[no];
      this.optSet = new boolean[no];
      this.realOutputs = new double[nout][];
      this.intOutputs = new int[nout][];
   }

   /** The function this holder calls. */
   public FunctionInfo info() {
      return info;
   }

   private void checkInput(int idx, InputType expected) {
      if (idx < 0 || idx >= info.inputs().size()) {
         throw new IllegalArgumentException(
            info.name() + ": input index " + idx + " out of range [0, " + info.inputs().size() + ")");
      }
      InputType actual = info.inputs().get(idx).type();
      if (actual != expected) {
         throw new IllegalArgumentException(
            info.name() + " input " + idx + " (" + info.inputs().get(idx).paramName()
            + ") is " + actual + ", not " + expected);
      }
   }

   /** Binds a {@link InputType#REAL} input. */
   public ParamHolder setInput(int idx, double[] series) {
      checkInput(idx, InputType.REAL);
      realInputs[idx] = require(series, "input " + idx);
      return this;
   }

   /** Binds an {@link InputType#INTEGER} input. */
   public ParamHolder setInput(int idx, int[] series) {
      checkInput(idx, InputType.INTEGER);
      intInputs[idx] = require(series, "input " + idx);
      return this;
   }

   /**
    * Binds an {@link InputType#PRICE} input. Pass {@code null} for any component
    * the function does not consume — {@link InputInfo#flags()} says which it does
    * (see {@link InputFlags}). Mirrors C's {@code TA_SetInputParamPricePtr}.
    */
   public ParamHolder setPriceInput(int idx, double[] open, double[] high, double[] low,
                                    double[] close, double[] volume, double[] openInterest) {
      checkInput(idx, InputType.PRICE);
      double[][] c = { open, high, low, close, volume, openInterest };
      int flags = info.inputs().get(idx).flags();
      int[] bits = { InputFlags.PRICE_OPEN, InputFlags.PRICE_HIGH, InputFlags.PRICE_LOW,
                     InputFlags.PRICE_CLOSE, InputFlags.PRICE_VOLUME, InputFlags.PRICE_OPENINTEREST };
      String[] names = { "open", "high", "low", "close", "volume", "openInterest" };
      for (int k = 0; k < c.length; k++) {
         if ((flags & bits[k]) != 0 && c[k] == null) {
            throw new IllegalArgumentException(
               info.name() + " input " + idx + " requires " + names[k]);
         }
      }
      priceInputs[idx] = c;
      return this;
   }

   private void checkOpt(int idx, OptInputType... expected) {
      if (idx < 0 || idx >= info.optInputs().size()) {
         throw new IllegalArgumentException(
            info.name() + ": optInput index " + idx + " out of range [0, "
            + info.optInputs().size() + ")");
      }
      OptInputType actual = info.optInputs().get(idx).type();
      for (OptInputType e : expected) {
         if (actual == e) {
            return;
         }
      }
      throw new IllegalArgumentException(
         info.name() + " optInput " + idx + " (" + info.optInputs().get(idx).paramName()
         + ") is " + actual + ", not " + java.util.Arrays.toString(expected));
   }

   /** Binds an {@link OptInputType#INTEGER_RANGE} or {@link OptInputType#INTEGER_LIST} parameter. */
   public ParamHolder setOptInput(int idx, int value) {
      checkOpt(idx, OptInputType.INTEGER_RANGE, OptInputType.INTEGER_LIST);
      if (info.optInputs().get(idx).type() == OptInputType.INTEGER_LIST) {
         MAType[] all = MAType.values();
         /* Setting a parameter to its documented default THROUGH the abstract
            interface is part of the ABI: C's TA_SetOptInputParamInteger accepts
            TA_INTEGER_DEFAULT and the function substitutes the declared default.
            Java cannot carry the sentinel any further than here -- Core takes a
            real MAType -- so it resolves at this boundary, which is exactly where
            an UNSET choice list already resolves (see the constructor). Leaving
            the two to disagree was issue #164's first finding. */
         if (value == Core.INTEGER_DEFAULT) {
            int declared = (int) info.optInputs().get(idx).defaultValue();
            maTypeOpts[idx] = all[declared];
            intOpts[idx] = declared;
         } else if (value < 0 || value >= all.length) {
            /* Ahead of the write below, not after it. Reversed, a rejected ordinal
               left `value` in intOpts -- unobservable, since resolveUnsetOptInputs
               rewrites an unset slot and Dispatch reads a choice list through
               maTypeOpt(), never intOpt() -- but it broke the same rule
               setPriceInput breaks visibly: a rejected setter must leave the
               holder as it found it (issue #266). */
            throw new IllegalArgumentException(
               info.name() + " optInput " + idx + ": " + value + " is not a valid MAType ordinal");
         } else {
            maTypeOpts[idx] = all[value];
            intOpts[idx] = value;
         }
      } else {
         intOpts[idx] = value;
      }
      optSet[idx] = true;
      return this;
   }

   /** Binds an {@link OptInputType#REAL_RANGE} parameter. */
   public ParamHolder setOptInput(int idx, double value) {
      checkOpt(idx, OptInputType.REAL_RANGE, OptInputType.REAL_LIST);
      realOpts[idx] = value;
      optSet[idx] = true;
      return this;
   }

   /** Binds an {@link OptInputType#INTEGER_LIST} (moving-average type) parameter. */
   public ParamHolder setOptInput(int idx, MAType value) {
      checkOpt(idx, OptInputType.INTEGER_LIST);
      maTypeOpts[idx] = require(value, "optInput " + idx);
      intOpts[idx] = value.ordinal();
      optSet[idx] = true;
      return this;
   }

   private void checkOutput(int idx, OutputType expected) {
      if (idx < 0 || idx >= info.outputs().size()) {
         throw new IllegalArgumentException(
            info.name() + ": output index " + idx + " out of range [0, "
            + info.outputs().size() + ")");
      }
      OutputType actual = info.outputs().get(idx).type();
      if (actual != expected) {
         throw new IllegalArgumentException(
            info.name() + " output " + idx + " (" + info.outputs().get(idx).paramName()
            + ") is " + actual + ", not " + expected);
      }
   }

   /** Binds an {@link OutputType#REAL} output array. */
   public ParamHolder setOutput(int idx, double[] out) {
      checkOutput(idx, OutputType.REAL);
      realOutputs[idx] = require(out, "output " + idx);
      return this;
   }

   /** Binds an {@link OutputType#INTEGER} output array. */
   public ParamHolder setOutput(int idx, int[] out) {
      checkOutput(idx, OutputType.INTEGER);
      intOutputs[idx] = require(out, "output " + idx);
      return this;
   }

   /**
    * The first index at which this function produces output, for the optional
    * parameters bound so far.
    *
    * <p>The counterpart of C's {@code TA_GetLookback} and C#'s
    * {@code FunctionCall.Lookback()}. Inputs and outputs need not be bound --
    * a lookback depends only on the optional parameters, which is what makes it
    * useful for sizing the output arrays before binding them.
    *
    * @return the lookback, or {@code -1} if a parameter is out of range
    */
   public int lookback() {
      resolveUnsetOptInputs();
      return Dispatch.lookback(this);
   }

   /**
    * Calls the function over {@code [startIdx, endIdx]}.
    *
    * <p>Unbound parameters that carry a documented default are filled in with it;
    * unbound inputs or outputs are an error.
    *
    * @throws IllegalArgumentException if a required parameter was never bound
    */
   public OutRange call(int startIdx, int endIdx) {
      for (int i = 0; i < info.inputs().size(); i++) {
         boolean bound = switch (info.inputs().get(i).type()) {
            case REAL -> realInputs[i] != null;
            case INTEGER -> intInputs[i] != null;
            case PRICE -> priceInputs[i] != null;
         };
         if (!bound) {
            throw new IllegalArgumentException(
               info.name() + ": input " + i + " (" + info.inputs().get(i).paramName() + ") not set");
         }
      }
      for (int i = 0; i < info.outputs().size(); i++) {
         boolean bound = info.outputs().get(i).type() == OutputType.REAL
            ? realOutputs[i] != null : intOutputs[i] != null;
         if (!bound) {
            throw new IllegalArgumentException(
               info.name() + ": output " + i + " (" + info.outputs().get(i).paramName() + ") not set");
         }
      }
      resolveUnsetOptInputs();
      return Dispatch.call(this, startIdx, endIdx);
   }

   /* Unset optional parameters take the cross-language default sentinel, which
      every generated function maps to its documented default. Shared by call()
      and lookback(): it used to live inside call(), so a lookback taken before
      the first call read zero-initialised slots and came back -1 for every
      function with an optional parameter. */
   private void resolveUnsetOptInputs() {
      for (int i = 0; i < info.optInputs().size(); i++) {
         if (optSet[i]) {
            continue;
         }
         switch (info.optInputs().get(i).type()) {
            case REAL_RANGE, REAL_LIST -> realOpts[i] = -4e37;
            case INTEGER_RANGE -> intOpts[i] = Integer.MIN_VALUE;
            /* Both slots, not just maTypeOpts: setOptInput's sentinel branch keeps
               them in step, and leaving unset to record intOpts=0 while recording
               maTypeOpts=Ema (APO/PPO/PVO default to 1) would reintroduce the very
               unset-vs-sentinel divergence this pair of methods exists to remove. */
            case INTEGER_LIST -> {
               int declared = (int) info.optInputs().get(i).defaultValue();
               maTypeOpts[i] = MAType.values()[declared];
               intOpts[i] = declared;
            }
         }
      }
   }

   private static <T> T require(T v, String what) {
      if (v == null) {
         throw new IllegalArgumentException(info(what));
      }
      return v;
   }

   private static String info(String what) {
      return what + " must not be null";
   }

   /* Accessors used by the generated Dispatch switch. */

   Core core() { return core; }
   double[] realInput(int i) { return realInputs[i]; }
   int[] intInput(int i) { return intInputs[i]; }
   double[] price(int slot, int component) { return priceInputs[slot][component]; }
   double realOpt(int i) { return realOpts[i]; }
   int intOpt(int i) { return intOpts[i]; }
   MAType maTypeOpt(int i) { return maTypeOpts[i]; }
   double[] realOutput(int i) { return realOutputs[i]; }
   int[] intOutput(int i) { return intOutputs[i]; }
}
"#,
    );
    s
}

/// The generated `switch` from a function name onto its typed public wrapper.
fn dispatch_class(rows: &[FuncRow]) -> String {
    let mut s = header("MF,CC");
    // Column 0, like the two tails below and this file's other emitters: in a
    // non-raw literal the Rust source's own indentation is string content, and
    // it shipped in the sources jar as a 9-space leak on every header line.
    s.push_str(
        r"import io.github.talib.Core;
import io.github.talib.OutRange;

/**
 * Routes a {@link ParamHolder} onto the typed method it names.
 *
 * <p>A generated {@code switch}, not reflection: the argument lists below are
 * emitted from the same definitions as the methods they call, so a signature
 * change breaks this file at compile time instead of at run time. It also
 * leaves the library AOT- and jlink-friendly.
 */
final class Dispatch {

   private Dispatch() { }

   static OutRange call(ParamHolder h, int startIdx, int endIdx) {
      Core core = h.core();
      switch (h.info().name()) {
",
    );

    for f in rows {
        let camel = f.name.clone();

        // Argument order comes STRAIGHT from the row the registry publishes:
        // a price bundle is one slot, and `signature_components` is the order
        // the typed method takes its arrays in. Do not re-fold `FuncDef` with a
        // second `price_bundle::group` pass here — nothing would check its
        // agreement with the row's own slot numbering, and keying that fold by
        // input NAME collides for two bundles sharing a component. Reading the
        // row cannot: the slot IS the row's index.
        let mut args: Vec<String> = vec!["startIdx".into(), "endIdx".into()];
        for (slot, inp) in f.inputs.iter().enumerate() {
            match inp.kind {
                InputKind::Price => {
                    for c in &inp.signature_components {
                        args.push(format!("h.price({slot}, {})", *c as usize));
                    }
                }
                InputKind::Real => args.push(format!("h.realInput({slot})")),
                InputKind::Integer => args.push(format!("h.intInput({slot})")),
            }
        }
        for (k, opt) in f.opt_inputs.iter().enumerate() {
            match &opt.domain {
                OptDomain::RealRange { .. } | OptDomain::RealList { .. } => {
                    args.push(format!("h.realOpt({k})"));
                }
                OptDomain::IntegerList { .. } => args.push(format!("h.maTypeOpt({k})")),
                OptDomain::IntegerRange { .. } => args.push(format!("h.intOpt({k})")),
            }
        }
        for (k, out) in f.outputs.iter().enumerate() {
            match out.kind {
                OutputKind::Integer => args.push(format!("h.intOutput({k})")),
                OutputKind::Real => args.push(format!("h.realOutput({k})")),
            }
        }

        let _ = writeln!(s, "         case {}:", js(&f.name));
        let _ = writeln!(s, "            return core.{camel}(");
        let _ = writeln!(s, "               {});", args.join(", "));
    }

    s.push_str(
        r#"         default:
            throw new IllegalArgumentException("no such function: " + h.info().name());
      }
   }

   /* The lookback tier. Separate from call() because it takes only the optional
      parameters -- no inputs, no outputs, no range -- so a caller can size its
      output arrays before binding them, exactly as TA_GetLookback allows. */
   static int lookback(ParamHolder h) {
      Core core = h.core();
      switch (h.info().name()) {
"#,
    );

    for f in rows {
        let camel = f.name.clone();
        let mut args: Vec<String> = Vec::new();
        for (k, opt) in f.opt_inputs.iter().enumerate() {
            match &opt.domain {
                OptDomain::RealRange { .. } | OptDomain::RealList { .. } => {
                    args.push(format!("h.realOpt({k})"));
                }
                OptDomain::IntegerList { .. } => args.push(format!("h.maTypeOpt({k})")),
                OptDomain::IntegerRange { .. } => args.push(format!("h.intOpt({k})")),
            }
        }
        let _ = writeln!(s, "         case {}:", js(&f.name));
        let _ = writeln!(s, "            return core.{camel}_Lookback({});", args.join(", "));
    }

    s.push_str(
        r#"         default:
            throw new IllegalArgumentException("no such function: " + h.info().name());
      }
   }
}
"#,
    );
    s
}
