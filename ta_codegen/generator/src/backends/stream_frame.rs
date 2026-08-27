//! Generate `src/tools/ta_regtest/ta_stream_frame.h` — a generic in-process
//! dispatcher over `TA_<N>_Open` / `TA_<N>_OpenAndFill` / `TA_<N>_Close`,
//! for every YAML-declared streaming function (issue #256's L2/L3).
//!
//! `ta_abstract`'s dynamic dispatch (`TA_CallFunc` via `TA_ParamHolder`) only
//! ever reaches the BATCH tier — it predates the streaming API and never grew
//! a streaming form — so there was no generic, in-process way to open a
//! stream for an arbitrary function name. Before this, the only place that
//! existed was the generated JSON-RPC servers' own per-function dispatch,
//! and using one of those as "the golden implementation" reintroduces the
//! exact trap `bin/ta_codegen_serve_c` already has elsewhere: it is a
//! SEPARATE build target from `ta_regtest`, so it can go stale relative to
//! the in-process library `ta_regtest` is always freshly linked against.
//!
//! This mirrors `variant_frame.rs` (issue #137) exactly for that reason: a
//! header emitted straight into `src/tools/ta_regtest/`, compiled directly
//! into `ta_regtest`, so it is rebuilt fresh every time and carries none of
//! that risk. Every value that crosses it is a native C type (`double`,
//! `TA_Integer`, `TA_RetCode`) — never JSON, never any wire-shaped
//! intermediate — so no wire-transport bug (issue #257) can ever reach it
//! either.
//!
//! Emitted as a header, like `ta_variant_frame.h`, so neither source list
//! needs an entry, and only when the `c` backend runs.

use std::fmt::Write as _;
use std::path::Path;

use super::variant_frame::flat_inputs;
use super::write_if_changed;
use crate::ir::{FuncDef, ParamType};

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

/// `optIn[j]`, cast to the slot's real C type (int / `TA_MAType` / double).
fn opt_arg(j: usize, opt: &crate::ir::OptInput) -> String {
    match &opt.param_type {
        ParamType::Real => format!("optIn[{j}]"),
        ParamType::Enum(_) => format!("(TA_MAType)(int)optIn[{j}]"),
        _ => format!("(int)optIn[{j}]"),
    }
}

/// The `stream**, inputs..., historyLen, optParams...` prefix shared by
/// `_Open` and `_OpenAndFill` — everything before their one point of
/// difference (a scalar out-param each vs. `outBegIdx/outNBElement` + arrays).
fn lead_in_args(func: &FuncDef, inputs: &[(&'static str, String)]) -> Vec<String> {
    let mut args = vec![format!("(TA_{}_Stream **)stream", func.name)];
    for (idx, (_, iname)) in inputs.iter().enumerate() {
        args.push(format!("in[{idx}] /* {iname} */"));
    }
    args.push("historyLen".into());
    for (j, opt) in func.optional_inputs.iter().enumerate() {
        args.push(format!("{} /* {} */", opt_arg(j, opt), opt.name));
    }
    args
}

/// One `outReal[i]` / `outInteger[i]` per output, in declaration order.
///
/// The subscript is the declaration position in BOTH arrays, so whichever one
/// the slot does not belong to has a hole there. See the same loop in
/// `variant_frame::emit_thunks` for why that is what a mixed-type function
/// (SYNTH12) needs.
fn output_args(func: &FuncDef) -> Vec<String> {
    let mut args = Vec::with_capacity(func.outputs.len());
    for (i, out) in func.outputs.iter().enumerate() {
        if out.param_type == ParamType::Integer {
            args.push(format!("outInteger[{i}] /* {} */", out.name));
        } else {
            args.push(format!("outReal[{i}] /* {} */", out.name));
        }
    }
    args
}

fn emit_call(o: &mut String, callee: &str, args: &[String]) {
    let _ = writeln!(o, "   return {callee}(");
    for (k, a) in args.iter().enumerate() {
        let comma = if k + 1 == args.len() { "" } else { "," };
        let _ = writeln!(o, "               {a}{comma}");
    }
    let _ = writeln!(o, "               );\n}}");
}

/// Emit the Open/OpenAndFill/Close thunks for one streaming function.
///
/// The opaque `void*`/`void**` handle means the thunk signature needs no
/// per-function struct type; only the body, which casts back to the real
/// `TA_<N>_Stream*`, does.
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

    // ---- Open: stream**, inputs..., historyLen, optParams..., one scalar
    // out-param per output (double* or int*, per TA_<N>_Open's own shape). ----
    let _ = writeln!(
        o,
        "static TA_RetCode TA_{name}_SFrameOpen( void **stream,\n\
         \x20                 const double *const in[], int historyLen,\n\
         \x20                 const double optIn[],\n\
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
    let mut args = lead_in_args(func, &inputs);
    args.extend(output_args(func));
    emit_call(o, &format!("TA_{name}_Open"), &args);

    // ---- OpenAndFill: same lead-in, then outBegIdx/outNBElement + array
    // outputs, matching batch's own out-shape (variant_frame.rs's thunk). ----
    let _ = writeln!(
        o,
        "static TA_RetCode TA_{name}_SFrameFill( void **stream,\n\
         \x20                 const double *const in[], int historyLen,\n\
         \x20                 const double optIn[],\n\
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
    let mut args = lead_in_args(func, &inputs);
    args.push("outBegIdx".into());
    args.push("outNBElement".into());
    args.extend(output_args(func));
    emit_call(o, &format!("TA_{name}_OpenAndFill"), &args);

    // ---- Close: the one thunk with nothing to unpack. ----
    let _ = writeln!(
        o,
        "static TA_RetCode TA_{name}_SFrameClose( void *stream )\n\
         {{\n\
         \x20  return TA_{name}_Close( (TA_{name}_Stream *)stream );\n\
         }}\n"
    );
}

/// Build the whole `ta_stream_frame.h` for the streaming subset of `funcs`.
pub fn render(funcs: &[FuncDef]) -> String {
    let streaming: Vec<&FuncDef> = funcs.iter().filter(|f| f.streaming).collect();

    let mut o = String::new();
    o.push_str(LICENSE);
    o.push_str(
        "/* AUTO-GENERATED by ta_codegen — DO NOT EDIT.\n\
         * Source of truth: ta_codegen/generator/src/backends/stream_frame.rs\n\
         * (regenerate: cd ta_codegen/generator && cargo run -- generate)\n\
         *\n\
         * A uniform in-process dispatcher over TA_<N>_Open / TA_<N>_OpenAndFill /\n\
         * TA_<N>_Close for every YAML-declared streaming function. See the module\n\
         * doc comment in stream_frame.rs for why this exists in-process rather than\n\
         * routing through a generated server (issue #256's L2/L3).\n\
         *\n\
         * The stream handle crosses every thunk as an opaque void* / void** — the\n\
         * uniform signature cannot name a per-function TA_<N>_Stream type, only the\n\
         * thunk body (which casts back before calling the real function) needs it.\n\
         * historyLen is the caller's array length, exactly as TA_<N>_Open/\n\
         * OpenAndFill already require it (no length is recoverable from a bare\n\
         * pointer). Reuses TA_VInputKind/TA_VOptSpec from ta_variant_frame.h\n\
         * (issue #137) rather than redefining them.\n\
         *\n\
         * Only regenerated when the `c` backend runs.\n\
         */\n\n",
    );
    o.push_str("#ifndef TA_STREAM_FRAME_H\n#define TA_STREAM_FRAME_H\n\n");
    o.push_str("#ifndef TA_FUNC_H\n   #include \"ta_func.h\"\n#endif\n");
    o.push_str("#include \"ta_variant_frame.h\"\n\n");

    o.push_str(
        "typedef TA_RetCode (*TA_SFrameOpen)( void **stream,\n\
         \x20                 const double *const in[], int historyLen,\n\
         \x20                 const double optIn[],\n\
         \x20                 double *const outReal[], int *const outInteger[] );\n\
         typedef TA_RetCode (*TA_SFrameFill)( void **stream,\n\
         \x20                 const double *const in[], int historyLen,\n\
         \x20                 const double optIn[],\n\
         \x20                 int *outBegIdx, int *outNBElement,\n\
         \x20                 double *const outReal[], int *const outInteger[] );\n\
         typedef TA_RetCode (*TA_SFrameClose)( void *stream );\n\n",
    );
    o.push_str(
        "typedef struct {\n\
         \x20  const char          *name;         /* without the TA_ prefix */\n\
         \x20  TA_SFrameOpen        open;\n\
         \x20  TA_SFrameFill        openAndFill;\n\
         \x20  TA_SFrameClose       close;\n\
         \x20  int                  nbInput;      /* flattened array parameters */\n\
         \x20  const TA_VInputKind *inputKind;     /* from ta_variant_frame.h */\n\
         \x20  int                  nbOptInput;\n\
         \x20  const TA_VOptSpec   *optInput;      /* from ta_variant_frame.h; NULL when nbOptInput == 0 */\n\
         \x20  int                  nbOutput;\n\
         \x20  const int           *outIsInt;      /* from ta_variant_frame.h; per output, 1 = TA_Integer */\n\
         } TA_StreamEntry;\n\n",
    );

    for func in &streaming {
        emit_thunks(&mut o, func);
    }

    o.push_str("static const TA_StreamEntry TA_StreamTable[] = {\n");
    for func in &streaming {
        let n = &func.name;
        let nb_in = flat_inputs(func).len();
        let nb_opt = func.optional_inputs.len();
        let opt_ptr = if nb_opt == 0 {
            "NULL".to_string()
        } else {
            format!("TA_VOpt_{n}")
        };
        let _ = writeln!(
            o,
            "   {{ \"{n}\", TA_{n}_SFrameOpen, TA_{n}_SFrameFill, TA_{n}_SFrameClose,\n\
             \x20    {nb_in}, TA_VIn_{n}, {nb_opt}, {opt_ptr}, {}, TA_VOutIsInt_{n} }},",
            func.outputs.len()
        );
    }
    o.push_str("};\n\n");
    let _ = writeln!(
        o,
        "#define TA_STREAM_TABLE_SIZE {}\n\n#endif /* TA_STREAM_FRAME_H */",
        streaming.len()
    );
    o
}

/// Write `src/tools/ta_regtest/ta_stream_frame.h`.
pub fn generate(funcs: &[FuncDef], root: &Path) {
    let path = root.join("src/tools/ta_regtest/ta_stream_frame.h");
    let streaming_count = funcs.iter().filter(|f| f.streaming).count();
    write_if_changed(&path, &render(funcs), "ta_stream_frame.h", streaming_count);
}
