//! Generate `src/tools/ta_regtest/ta_variant_frame.h` — the generic four-variant
//! dispatcher used by `ta_regtest`'s variant-parity gate (issue #137).
//!
//! Every TA function ships in four flavours with identical arity:
//!
//! | symbol                     | inputs   | range checks |
//! |----------------------------|----------|--------------|
//! | `TA_<N>`                   | `double` | yes          |
//! | `TA_<N>_Unguarded`         | `double` | no           |
//! | `TA_S_<N>`                 | `float`  | yes          |
//! | `TA_S_<N>_Unguarded`       | `float`  | no           |
//!
//! The abstract layer (`TA_CallFunc` via `ta_frame.c`) only ever reaches the
//! first, so the other three had no generic gate and were only covered where a
//! hand-written test happened to call them. This header gives `ta_regtest` one
//! uniform signature per flavour plus a table over every function, so a single
//! `TA_ForEachFunc`-style loop can assert the whole matrix bit-for-bit with no
//! external oracle.
//!
//! Emitted as a **header** rather than a `.c` on purpose: it needs no entry in
//! `CMakeLists.txt` / `Makefile.am`, so the two source lists cannot drift and
//! `scripts/build.py check-source-lists` stays green for free.
//!
//! Like every other C-side artifact this is written only when the `c` backend
//! runs (`cargo run -- generate --backend=rust` leaves it untouched).

use std::collections::HashMap;
use std::fmt::Write as _;
use std::path::Path;

use super::write_if_changed;
use crate::ir::{EnumDef, FuncDef, OptInput, ParamType, PriceComponent};

const LICENSE: &str = "\
/* TA-LIB Copyright (c) 1999-2026, Mario Fortier
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or
 * without modification, are permitted provided that the following
 * conditions are met:
 *
 * - Redistributions of source code must retain the above copyright
 *   notice, this list of conditions and the following disclaimer.
 *
 * - Redistributions in binary form must reproduce the above copyright
 *   notice, this list of conditions and the following disclaimer in
 *   the documentation and/or other materials provided with the
 *   distribution.
 *
 * - Neither name of author nor the names of its contributors
 *   may be used to endorse or promote products derived from this
 *   software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * REGENTS OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
 * OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */\n\n";

/// One flattened input slot: what series `ta_regtest` should feed it.
///
/// A price bundle is already expanded to one input per component by
/// `parser::yaml`, which tags each with the component it carries — so the kind
/// is read off that tag rather than reconstructed from the abstract grouping.
fn input_kind(name: &str, component: Option<PriceComponent>) -> &'static str {
    if let Some(comp) = component {
        return match comp {
            PriceComponent::Open => "TA_VIN_OPEN",
            PriceComponent::High => "TA_VIN_HIGH",
            PriceComponent::Low => "TA_VIN_LOW",
            PriceComponent::Close => "TA_VIN_CLOSE",
            PriceComponent::Volume => "TA_VIN_VOLUME",
            PriceComponent::OpenInterest => "TA_VIN_OPENINTEREST",
        };
    }
    // Plain real inputs. `inPeriods` (MAVP) is a period series, not a price
    // series — feeding it prices would drive every period out of range and make
    // the gate vacuous, so it gets its own kind.
    if name == "inPeriods" {
        "TA_VIN_PERIODS"
    } else {
        "TA_VIN_REAL"
    }
}

/// Flattened input slots for `func`: `(c_kind, name)` per array parameter.
fn flat_inputs(func: &FuncDef) -> Vec<(&'static str, String)> {
    let mut out = Vec::new();
    for input in &func.inputs {
        match &input.param_type {
            // Price components arrive here already expanded, as `Real` inputs carrying a
            // `PriceRef`; a literal `ParamType::Price` never reaches the backends.
            ParamType::Price(_) => unreachable!(
                "TA_{}: parser::yaml expands price bundles before the backends see them",
                func.name
            ),
            ParamType::Real => out.push((
                input_kind(&input.name, input.price.map(|p| p.component)),
                input.name.clone(),
            )),
            ParamType::Integer | ParamType::Enum(_) => panic!(
                "TA_{}: input `{}` is not a real/price array — the variant frame \
                 assumes every input array is `const double[]` / `const float[]`",
                func.name, input.name
            ),
        }
    }
    out
}

/// `(is_real, min, max, default)` for one optional input, as concrete numbers.
///
/// The gate feeds these straight through: `TA_<N>_Unguarded` does **not**
/// substitute the `TA_INTEGER_DEFAULT` / `TA_REAL_DEFAULT` sentinels (that code
/// lives in the guarded-only validation prologue), so a sentinel forwarded to
/// the unguarded variant would compute against `0x80000000` and diverge for
/// reasons that have nothing to do with correctness. Always send resolved values.
fn opt_spec(opt: &OptInput, enums: &HashMap<String, EnumDef>) -> (bool, f64, f64, f64) {
    let is_real = matches!(opt.param_type, ParamType::Real);
    let (min, max) = match (&opt.range, &opt.param_type) {
        (Some((lo, hi)), _) => (*lo, *hi),
        (None, ParamType::Enum(enum_name)) => {
            let n = enums.get(enum_name).map_or(1, |e| e.variants.len()).max(1);
            (0.0, f64::from(u32::try_from(n - 1).unwrap_or(u32::MAX)))
        }
        (None, _) => (0.0, 0.0),
    };
    let def = opt.default.unwrap_or(min);
    (is_real, min, max, def)
}

/// Render a C double literal that round-trips exactly.
///
/// The `==` against `trunc()` is deliberate: the question is exactly "is this
/// value integral", so an epsilon would be wrong, not safer.
#[allow(clippy::float_cmp)]
fn c_double(v: f64) -> String {
    if v == v.trunc() && v.abs() < 1e15 {
        format!("{v:.1}")
    } else {
        format!("{v:.17e}")
    }
}

/// Emit the four thunks for one function.
///
/// Each thunk unpacks the uniform argument arrays into the function's real
/// signature. `single` selects the `float`-input pair (`TA_S_*`).
fn emit_thunks(o: &mut String, func: &FuncDef) {
    let name = &func.name;
    let inputs = flat_inputs(func);
    let has_int_out = func
        .outputs
        .iter()
        .any(|out| matches!(out.param_type, ParamType::Integer));
    let has_real_out = func
        .outputs
        .iter()
        .any(|out| !matches!(out.param_type, ParamType::Integer));

    for (suffix, callee, elem) in [
        ("VFrameD", format!("TA_{name}"), "double"),
        ("VFrameDU", format!("TA_{name}_Unguarded"), "double"),
        ("VFrameS", format!("TA_S_{name}"), "float"),
        ("VFrameSU", format!("TA_S_{name}_Unguarded"), "float"),
    ] {
        let _ = writeln!(
            o,
            "static TA_RetCode TA_{name}_{suffix}( int startIdx, int endIdx,\n\
             \x20                 const {elem} *const in[], const double optIn[],\n\
             \x20                 int *outBegIdx, int *outNBElement,\n\
             \x20                 double *const outReal[], int *const outInteger[] )\n\
             {{"
        );
        if func.optional_inputs.is_empty() {
            let _ = writeln!(o, "   (void)optIn;");
        }
        if !has_int_out {
            let _ = writeln!(o, "   (void)outInteger;");
        }
        if !has_real_out {
            let _ = writeln!(o, "   (void)outReal;");
        }

        let mut args: Vec<String> = vec!["startIdx".into(), "endIdx".into()];
        for (idx, (_, iname)) in inputs.iter().enumerate() {
            args.push(format!("in[{idx}] /* {iname} */"));
        }
        for (j, opt) in func.optional_inputs.iter().enumerate() {
            let expr = match &opt.param_type {
                ParamType::Real => format!("optIn[{j}]"),
                ParamType::Enum(_) => format!("(TA_MAType)(int)optIn[{j}]"),
                _ => format!("(int)optIn[{j}]"),
            };
            args.push(format!("{expr} /* {} */", opt.name));
        }
        args.push("outBegIdx".into());
        args.push("outNBElement".into());
        let mut real_i = 0;
        let mut int_i = 0;
        for out in &func.outputs {
            if out.param_type == ParamType::Integer {
                args.push(format!("outInteger[{int_i}] /* {} */", out.name));
                int_i += 1;
            } else {
                args.push(format!("outReal[{real_i}] /* {} */", out.name));
                real_i += 1;
            }
        }
        let _ = writeln!(o, "   return {callee}(");
        for (k, a) in args.iter().enumerate() {
            let comma = if k + 1 == args.len() { "" } else { "," };
            let _ = writeln!(o, "               {a}{comma}");
        }
        let _ = writeln!(o, "               );\n}}");
    }
    o.push('\n');
}

/// Build the whole `ta_variant_frame.h` for `funcs`.
#[allow(clippy::implicit_hasher, clippy::too_many_lines)]
pub fn render(funcs: &[FuncDef], enums: &HashMap<String, EnumDef>) -> String {
    let mut o = String::new();
    o.push_str(LICENSE);
    o.push_str(
        "/* AUTO-GENERATED by ta_codegen — DO NOT EDIT.\n\
         * Source of truth: ta_codegen/generator/src/backends/variant_frame.rs\n\
         * (regenerate: cd ta_codegen/generator && cargo run -- generate)\n\
         *\n\
         * A uniform dispatcher over the four flavours every TA function ships:\n\
         *   TA_<N>  TA_<N>_Unguarded  TA_S_<N>  TA_S_<N>_Unguarded\n\
         * All four have identical arity, differing only in `double` vs `float`\n\
         * input arrays and in whether the parameter-validation prologue is\n\
         * present, so one signature per flavour covers every function.\n\
         *\n\
         * Used by src/tools/ta_regtest/ta_test_func/test_variants.c (issue #137).\n\
         * Only regenerated when the `c` backend runs.\n\
         */\n\n",
    );
    o.push_str("#ifndef TA_VARIANT_FRAME_H\n#define TA_VARIANT_FRAME_H\n\n");
    o.push_str("#ifndef TA_FUNC_H\n   #include \"ta_func.h\"\n#endif\n\n");
    // ta_func.h does NOT pull in the unguarded declarations; without this the
    // *_Unguarded calls below would become implicit declarations and silently
    // mis-pass arguments.
    o.push_str("#ifndef TA_FUNC_UNGUARDED_H\n   #include \"ta_func_unguarded.h\"\n#endif\n\n");

    o.push_str(
        "/* Which test series feeds a given input slot. Price groups are already\n\
         * flattened to one slot per component. */\n\
         typedef enum {\n\
         \x20  TA_VIN_REAL = 0,\n\
         \x20  TA_VIN_OPEN,\n\
         \x20  TA_VIN_HIGH,\n\
         \x20  TA_VIN_LOW,\n\
         \x20  TA_VIN_CLOSE,\n\
         \x20  TA_VIN_VOLUME,\n\
         \x20  TA_VIN_OPENINTEREST,\n\
         \x20  TA_VIN_PERIODS      /* MAVP's per-element period series */\n\
         } TA_VInputKind;\n\n",
    );
    o.push_str(
        "/* Concrete (never-sentinel) bounds for one optional parameter. */\n\
         typedef struct {\n\
         \x20  const char *name;\n\
         \x20  int    isReal;      /* 1 = TA_Real parameter, 0 = integer/enum */\n\
         \x20  double minValue;\n\
         \x20  double maxValue;\n\
         \x20  double defValue;\n\
         } TA_VOptSpec;\n\n",
    );
    o.push_str(
        "typedef TA_RetCode (*TA_VFrameD)( int startIdx, int endIdx,\n\
         \x20                 const double *const in[], const double optIn[],\n\
         \x20                 int *outBegIdx, int *outNBElement,\n\
         \x20                 double *const outReal[], int *const outInteger[] );\n\
         typedef TA_RetCode (*TA_VFrameS)( int startIdx, int endIdx,\n\
         \x20                 const float *const in[], const double optIn[],\n\
         \x20                 int *outBegIdx, int *outNBElement,\n\
         \x20                 double *const outReal[], int *const outInteger[] );\n\n",
    );
    o.push_str(
        "typedef struct {\n\
         \x20  const char          *name;         /* without the TA_ prefix */\n\
         \x20  TA_VFrameD           guarded;      /* TA_<N>                */\n\
         \x20  TA_VFrameD           unguarded;    /* TA_<N>_Unguarded      */\n\
         \x20  TA_VFrameS           single;       /* TA_S_<N>              */\n\
         \x20  TA_VFrameS           singleUnguarded; /* TA_S_<N>_Unguarded */\n\
         \x20  int                  nbInput;      /* flattened array parameters */\n\
         \x20  const TA_VInputKind *inputKind;\n\
         \x20  int                  nbOptInput;\n\
         \x20  const TA_VOptSpec   *optInput;     /* NULL when nbOptInput == 0 */\n\
         \x20  int                  nbOutput;\n\
         \x20  int                  outIsInteger; /* 1 = outputs are TA_Integer */\n\
         \x20  /* 1 when guarded and unguarded both delegate to a shared\n\
         \x20   * TA_<N>_Private body, making unguarded parity structurally true\n\
         \x20   * rather than tested. Derived from the definition having an\n\
         \x20   * explicit <name>_private() in ta_codegen/input/, so it can never\n\
         \x20   * go stale against a hand-maintained list. */\n\
         \x20  int                  delegatesToPrivate;\n\
         } TA_VariantEntry;\n\n",
    );

    // Per-function thunks and descriptor arrays.
    for func in funcs {
        emit_thunks(&mut o, func);

        let inputs = flat_inputs(func);
        let _ = write!(o, "static const TA_VInputKind TA_VIn_{}[] = {{ ", func.name);
        for (k, (kind, _)) in inputs.iter().enumerate() {
            let comma = if k + 1 == inputs.len() { "" } else { ", " };
            let _ = write!(o, "{kind}{comma}");
        }
        o.push_str(" };\n");

        if !func.optional_inputs.is_empty() {
            let _ = writeln!(o, "static const TA_VOptSpec TA_VOpt_{}[] = {{", func.name);
            for opt in &func.optional_inputs {
                let (is_real, min, max, def) = opt_spec(opt, enums);
                let _ = writeln!(
                    o,
                    "   {{ \"{}\", {}, {}, {}, {} }},",
                    opt.name,
                    i32::from(is_real),
                    c_double(min),
                    c_double(max),
                    c_double(def)
                );
            }
            o.push_str("};\n");
        }
        o.push('\n');
    }

    // The table.
    o.push_str("static const TA_VariantEntry TA_VariantTable[] = {\n");
    for func in funcs {
        let n = &func.name;
        let nb_in = flat_inputs(func).len();
        let nb_opt = func.optional_inputs.len();
        let opt_ptr = if nb_opt == 0 {
            "NULL".to_string()
        } else {
            format!("TA_VOpt_{n}")
        };
        // No shipped function mixes real and integer outputs; the gate relies on
        // that so one flag per function is enough. Assert it here rather than
        // letting a future mixed-output function emit a silently wrong table.
        let nb_int = func
            .outputs
            .iter()
            .filter(|o| matches!(o.param_type, ParamType::Integer))
            .count();
        assert!(
            nb_int == 0 || nb_int == func.outputs.len(),
            "TA_{n} mixes real and integer outputs — ta_variant_frame's single \
             outIsInteger flag can no longer describe it"
        );
        let _ = writeln!(
            o,
            "   {{ \"{n}\", TA_{n}_VFrameD, TA_{n}_VFrameDU, TA_{n}_VFrameS, TA_{n}_VFrameSU,\n\
             \x20    {nb_in}, TA_VIn_{n}, {nb_opt}, {opt_ptr}, {}, {}, {} }},",
            func.outputs.len(),
            i32::from(nb_int > 0),
            i32::from(func.has_explicit_private)
        );
    }
    o.push_str("};\n\n");
    let _ = writeln!(
        o,
        "#define TA_VARIANT_TABLE_SIZE {}\n\n#endif /* TA_VARIANT_FRAME_H */",
        funcs.len()
    );
    o
}

/// Write `src/tools/ta_regtest/ta_variant_frame.h`.
#[allow(clippy::implicit_hasher)]
pub fn generate(funcs: &[FuncDef], enums: &HashMap<String, EnumDef>, root: &Path) {
    let path = root.join("src/tools/ta_regtest/ta_variant_frame.h");
    write_if_changed(&path, &render(funcs, enums), "ta_variant_frame.h", funcs.len());
}
