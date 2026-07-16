//! Generates a standalone direct-call benchmark binary for the C backend.
//!
//! Unlike the JSON-RPC server, this binary generates its own price data
//! (deterministic, seed=42) and calls indicator functions directly.
//! Output: one `FUNCNAME timing_ns` line per function on stdout.

use crate::ir::{FuncDef, ParamType};
use crate::server_gen::expand_input_names;
use std::path::Path;

pub fn generate_c_bench(funcs: &[FuncDef]) -> String {
    let mut s = String::new();

    s.push_str("/* Auto-generated direct-call benchmark for ta_codegen C output.\n");
    s.push_str(" * No JSON-RPC -- generates its own data, calls functions directly.\n");
    s.push_str(" * Output: FUNCNAME timing_ns (one per line)\n");
    s.push_str(" */\n");
    s.push_str("#include <stdio.h>\n#include <stdlib.h>\n#include <string.h>\n");
    s.push_str("#include <math.h>\n#include <time.h>\n");
    s.push_str("#ifdef __APPLE__\n#include <mach/mach_time.h>\n#endif\n\n");

    // Include headers for unguarded and private function declarations
    s.push_str("#include \"ta_func_unguarded.h\"\n");
    s.push_str("#include \"ta_func/ta_func_private.h\"\n\n");
    // Globals + indicator includes (same single-TU pattern as server)
    s.push_str("#include \"ta_common/ta_global.c\"\n");
    s.push_str("#include \"ta_func/ta_utility.c\"\n");
    s.push_str("#include \"ta_common/ta_version.c\"\n");
    s.push_str("#include \"ta_common/ta_retcode.c\"\n\n");
    let mut sorted: Vec<&str> = funcs.iter().map(|f| f.name.as_str()).collect();
    sorted.sort_unstable();
    if let Some(pos) = sorted.iter().position(|n| *n == "MA") {
        let ma = sorted.remove(pos);
        sorted.push(ma);
    }
    for name in &sorted {
        s.push_str(&format!("#include \"ta_{name}.c\"\n"));
    }
    s.push('\n');

    s.push_str(TIMING_HELPER);
    s.push_str(PRICE_DATA_GEN);

    s.push_str("#define MAX_POINTS 200000\n");
    s.push_str("static double g_outBuf0[MAX_POINTS];\n");
    s.push_str("static double g_outBuf1[MAX_POINTS];\n");
    s.push_str("static double g_outBuf2[MAX_POINTS];\n");
    s.push_str("static int g_outIntBuf0[MAX_POINTS];\n");
    s.push_str("static int g_outIntBuf1[MAX_POINTS];\n\n");

    s.push_str(FUNC_MATCHES);
    generate_bench_func(&mut s, funcs);
    s.push_str(MAIN_FUNC);

    s
}

/// Map an expanded input name to the global benchmark array variable.
fn input_name_to_global(name: &str, real_idx: usize) -> String {
    match name {
        "inOpen" => "g_open".to_string(),
        "inHigh" => "g_high".to_string(),
        "inLow" => "g_low".to_string(),
        "inClose" => "g_close".to_string(),
        "inVolume" => "g_volume".to_string(),
        "inOpenInterest" => "g_oi".to_string(),
        _ => {
            // Generic real inputs: first uses g_close, second uses g_high
            if real_idx == 0 { "g_close".to_string() } else { "g_high".to_string() }
        }
    }
}

fn generate_bench_func(s: &mut String, funcs: &[FuncDef]) {
    // Volatile sink prevents LTO from eliminating function calls
    s.push_str("static volatile int g_sink = 0;\n\n");
    s.push_str("static void bench_all(const char *filter, int iters) {\n");

    for func in funcs {
        let name = &func.name;
        let ta_name = format!("TA_{name}");
        let input_names = expand_input_names(&func.inputs);

        s.push_str(&format!("    if( func_matches(filter, \"{name}\") ) {{\n"));
        s.push_str("        long long best = 0;\n");
        s.push_str("        for( int pass = 0; pass < 3; pass++ ) {\n");
        s.push_str("            int outBegIdx, outNBElement;\n");
        s.push_str("            long long t0 = get_nanotime();\n");
        s.push_str("            for( int it = 0; it < iters; it++ ) {\n");

        // Build the function call arguments
        let mut args = Vec::new();
        args.push("0".to_string());
        args.push("g_nPoints - 1".to_string());

        // Input arrays
        let mut real_idx = 0;
        for inp_name in &input_names {
            args.push(input_name_to_global(inp_name, real_idx));
            if !matches!(
                inp_name.as_str(),
                "inOpen" | "inHigh" | "inLow" | "inClose" | "inVolume" | "inOpenInterest"
            ) {
                real_idx += 1;
            }
        }

        // Optional inputs — use defaults
        for opt in &func.optional_inputs {
            if opt.param_type == ParamType::Real {
                let default = opt.default.unwrap_or(0.0);
                args.push(format!("{default:.15}"));
            } else {
                #[allow(clippy::cast_possible_truncation)]
                let default = opt.default.unwrap_or(0.0) as i32;
                args.push(format!("{default}"));
            }
        }

        args.push("&outBegIdx".to_string());
        args.push("&outNBElement".to_string());

        // Output arrays
        let mut out_real_idx = 0;
        let mut out_int_idx = 0;
        for out in &func.outputs {
            if out.param_type == ParamType::Integer {
                args.push(format!("g_outIntBuf{out_int_idx}"));
                out_int_idx += 1;
            } else {
                args.push(format!("g_outBuf{out_real_idx}"));
                out_real_idx += 1;
            }
        }

        let args_str = args.join(", ");
        s.push_str(&format!("                {ta_name}({args_str});\n"));

        s.push_str("            }\n");
        s.push_str("            long long elapsed = get_nanotime() - t0;\n");
        s.push_str("            if( !best || elapsed < best ) best = elapsed;\n");
        // Observe outputs after timing — prevents LTO from eliminating function bodies.
        // Must read from ALL output buffers, not just outNBElement.
        s.push_str("            g_sink += outNBElement;\n");
        // Sink first element of each output array so LTO can't DCE the writes
        {
            let mut sink_real = 0;
            let mut sink_int = 0;
            for out in &func.outputs {
                if out.param_type == ParamType::Integer {
                    s.push_str(&format!("            g_sink += g_outIntBuf{sink_int}[0];\n"));
                    sink_int += 1;
                } else {
                    s.push_str(&format!("            g_sink += (int)g_outBuf{sink_real}[0];\n"));
                    sink_real += 1;
                }
            }
        }
        s.push_str("        }\n");
        s.push_str(&format!("        printf(\"{name} %lld\\n\", best / iters);\n"));
        s.push_str("        fflush(stdout);\n");
        s.push_str("    }\n");
    }

    s.push_str("}\n\n");
}

pub fn write_c_bench(funcs: &[FuncDef], output_dir: &Path) {
    let content = generate_c_bench(funcs);
    let path = output_dir.join("ta_bench_cg.c");
    std::fs::write(&path, &content).unwrap_or_else(|e| {
        panic!("Failed to write {}: {e}", path.display());
    });
    eprintln!("  C bench -> {}", path.display());
}

// =====================================================================
// Streaming benchmark (ta_bench_stream)
// =====================================================================
//
// A separate standalone binary (leaving the validated batch bench untouched)
// that, for every streamable function, measures and prints one row:
//
//   NAME  batch_last_ns  update_ns  peek_ns  lookback  handle_bytes
//
// All three costs are THROUGHPUT: many calls timed together and divided by the
// count, so per-call clock noise (a single call can be ~2ns, below timer
// resolution) averages out. Every measured call writes one bar AND computes, so
// batch@last and update are charged the same way.
//
// - batch_last_ns : the batch guarded entry for a single bar. Each call appends
//   the incoming bar to a GROWING buffer (`g_rt_*`) and computes one output over
//   the last `lookback` bars (startIdx==endIdx==t) — the append is the per-bar
//   write the update also pays, on real cache behaviour.
// - update_ns     : one `TA_XXX_Update` on an already-open steady-state stream
//   (opened over the full history once, outside timing). Input scalars rotate
//   over the first MASK+1 bars — CSE-proof; write-plus-compute per call, an
//   apples-to-apples throughput comparison with batch@last.
// - peek_ns       : one `TA_XXX_Peek` (deep-copy of the handle + one transition,
//   never commits) — the same rotating feed and index cost as update, so the
//   peek-minus-update delta is exactly the throwaway-copy overhead.
// - lookback      : `TA_XXX_Lookback(defaults)` — contextualises batch_last.
// - handle_bytes  : retained bytes of the open handle, measured by overriding
//   TA_Malloc/TA_Free with a registry and taking the net-live delta across
//   Open (scratch temporaries use raw malloc/free and are invisible, which is
//   correct — they are freed before Open returns; only retained state counts).
//
// All params are at their defaults, so every Open succeeds (default MAType is
// always SMA, a supported arm); a rejecting Open still prints its batch_last
// and lookback with REJECT in the update/peek columns.

/// The per-bar out-scalar declarations and address-argument lists shared by a
/// function's Open / Update / Peek calls (outputs appear in the same declared
/// order in all three). Returns `(decls, addr_args, acc_lines)`, where
/// `acc_lines` accumulate every output into `acc` INSIDE the timed loop so the
/// optimizer cannot dead-code-eliminate calls whose only observable effect is
/// the (overwritten) scalar output — the load-bearing anti-DCE dependency that
/// makes stateless (T1) and non-committing (peek) measurements honest.
fn stream_out_bits(func: &FuncDef) -> (String, String, String) {
    let mut decls = String::new();
    let mut addrs = Vec::new();
    let mut acc = String::new();
    let (mut r, mut i) = (0usize, 0usize);
    for out in &func.outputs {
        if out.param_type == ParamType::Integer {
            decls.push_str(&format!("            int iv{i} = 0;\n"));
            addrs.push(format!("&iv{i}"));
            acc.push_str(&format!("                    acc += (double)iv{i};\n"));
            i += 1;
        } else {
            decls.push_str(&format!("            double v{r} = 0.0;\n"));
            addrs.push(format!("&v{r}"));
            acc.push_str(&format!("                    acc += v{r};\n"));
            r += 1;
        }
    }
    (decls, addrs.join(", "), acc)
}

/// Optional-param default values as C call arguments (shared by Open, batch,
/// and Lookback). Mirrors the batch emitter exactly.
fn stream_opt_args(func: &FuncDef) -> Vec<String> {
    let mut args = Vec::new();
    for opt in &func.optional_inputs {
        if opt.param_type == ParamType::Real {
            let default = opt.default.unwrap_or(0.0);
            args.push(format!("{default:.15}"));
        } else {
            #[allow(clippy::cast_possible_truncation)]
            let default = opt.default.unwrap_or(0.0) as i32;
            args.push(format!("{default}"));
        }
    }
    args
}

#[allow(clippy::too_many_lines)]
fn generate_stream_bench_func(s: &mut String, funcs: &[FuncDef]) {
    s.push_str("static volatile int g_sink = 0;\n\n");
    s.push_str("#define BENCH_MASK 4095\n\n");
    s.push_str("static void bench_stream_all(const char *filter, int iters) {\n");
    s.push_str("    printf(\"# func batch_last_ns update_ns peek_ns lookback handle_bytes\\n\");\n");
    s.push_str("    fflush(stdout);\n");

    for func in funcs {
        if !func.streaming {
            continue;
        }
        let name = &func.name;
        let ta = format!("TA_{name}");
        let input_names = expand_input_names(&func.inputs);
        let opt_args = stream_opt_args(func);
        let (out_decls, out_addrs, out_acc) = stream_out_bits(func);

        // Input array args (Open + batch@last) and per-bar scalar args (Update/Peek).
        let mut in_arrays = Vec::new();
        let mut bar_scalars = Vec::new();
        let mut real_idx = 0usize;
        for inp in &input_names {
            let g = input_name_to_global(inp, real_idx);
            in_arrays.push(g.clone());
            bar_scalars.push(format!("{g}[it & BENCH_MASK]"));
            if !matches!(
                inp.as_str(),
                "inOpen" | "inHigh" | "inLow" | "inClose" | "inVolume" | "inOpenInterest"
            ) {
                real_idx += 1;
            }
        }

        // Batch@last output arrays (same selection as the batch bench).
        let mut batch_out = Vec::new();
        {
            let (mut r, mut i) = (0usize, 0usize);
            for out in &func.outputs {
                if out.param_type == ParamType::Integer {
                    batch_out.push(format!("g_outIntBuf{i}"));
                    i += 1;
                } else {
                    batch_out.push(format!("g_outBuf{r}"));
                    r += 1;
                }
            }
        }

        // batch@last computes over a *growing* buffer the caller appends into
        // (g_rt_*): same inputs as the update feed, distinct storage written one
        // bar per call so the per-bar cost includes the write.
        let rt_arrays: Vec<String> =
            in_arrays.iter().map(|a| a.replace("g_", "g_rt_")).collect();

        let opt_csv = if opt_args.is_empty() {
            String::new()
        } else {
            format!("{}, ", opt_args.join(", "))
        };
        let opt_only = opt_args.join(", "); // for Lookback

        s.push_str(&format!("    if( func_matches(filter, \"{name}\") ) {{\n"));
        s.push_str("        long long best_b = 0, best_u = -1, best_p = -1;\n");
        s.push_str("        int begIdx = 0, nb = 0;\n");
        s.push_str("        size_t handle_bytes = 0;\n");
        s.push_str("        double acc = 0.0;\n");
        // Lookback contextualises batch@last and sizes its compute window.
        s.push_str(&format!("        int lb = {ta}_Lookback({opt_only});\n"));
        s.push_str("        if( lb < 0 ) lb = 0;\n");
        // batch@last: append the incoming bar at `t` in the growing buffer, then
        // compute one output over the last `lb` bars (startIdx==endIdx==t).
        s.push_str("        for( int pass = 0; pass < 3; pass++ ) {\n");
        s.push_str("            int t = lb;\n");
        s.push_str("            long long t0 = get_nanotime();\n");
        s.push_str("            for( int it = 0; it < iters; it++ ) {\n");
        for (k, rt) in rt_arrays.iter().enumerate() {
            s.push_str(&format!("                {rt}[t] = {}[it & BENCH_MASK];\n", in_arrays[k]));
        }
        {
            let mut a = vec!["t".to_string(), "t".to_string()];
            a.extend(rt_arrays.iter().cloned());
            a.extend(opt_args.iter().cloned());
            a.push("&begIdx".to_string());
            a.push("&nb".to_string());
            a.extend(batch_out.iter().cloned());
            s.push_str(&format!("                {ta}({});\n", a.join(", ")));
        }
        // anti-DCE: observe an output element every iteration.
        {
            let (mut r, mut i) = (0usize, 0usize);
            for out in &func.outputs {
                if out.param_type == ParamType::Integer {
                    s.push_str(&format!("                acc += (double)g_outIntBuf{i}[0];\n"));
                    i += 1;
                } else {
                    s.push_str(&format!("                acc += g_outBuf{r}[0];\n"));
                    r += 1;
                }
            }
        }
        s.push_str("                t++;\n");
        s.push_str("            }\n");
        s.push_str("            long long el = get_nanotime() - t0;\n");
        s.push_str("            if( !best_b || el < best_b ) best_b = el;\n");
        s.push_str("        }\n");

        // Open once over the full history; measure retained handle bytes.
        s.push_str(&format!("        {ta}_Stream *st = NULL;\n"));
        s.push_str(&out_decls);
        s.push_str("        g_trk_reset(); g_ta_track = 1;\n");
        {
            let mut a = vec!["&st".to_string()];
            a.extend(in_arrays.iter().cloned());
            a.push("g_nPoints".to_string());
            if !opt_csv.is_empty() {
                a.push(opt_only.clone());
            }
            a.push(out_addrs.clone());
            let joined = a.iter().filter(|x| !x.is_empty()).cloned().collect::<Vec<_>>().join(", ");
            s.push_str(&format!("        TA_RetCode orc = {ta}_Open({joined});\n"));
        }
        s.push_str("        g_ta_track = 0; handle_bytes = g_ta_live_bytes;\n");
        s.push_str("        if( orc == TA_SUCCESS && st ) {\n");
        s.push_str("            int blk = (iters >= 64) ? 32 : 1;\n");
        s.push_str("            int nblk = iters / blk; int npk = nblk * blk; if( npk < 1 ) npk = 1;\n");
        // update: pure committing-transition loop on the steady-state handle
        // (input rotates over the first BENCH_MASK+1 bars; state evolves, so the
        // amortised cost of any data-dependent branch — extrema rescans — shows).
        // Throughput, like batch@last: both write one bar and compute per call.
        s.push_str("            for( int pass = 0; pass < 3; pass++ ) {\n");
        s.push_str("                long long t0 = get_nanotime();\n");
        s.push_str("                for( int it = 0; it < iters; it++ ) {\n");
        {
            let mut a = vec!["st".to_string()];
            a.extend(bar_scalars.iter().cloned());
            a.push(out_addrs.clone());
            s.push_str(&format!("                    {ta}_Update({});\n", a.join(", ")));
        }
        s.push_str(&out_acc);
        s.push_str("                }\n");
        s.push_str("                long long tu = get_nanotime() - t0;\n");
        s.push_str("                if( best_u < 0 || tu < best_u ) best_u = tu;\n");
        s.push_str("            }\n");
        // peek: timed ALONE in blocks (throwaway deep-copy + one transition, no
        // commit), with state advanced by UNTIMED updates between blocks. Timing
        // peek away from any adjacent update avoids both the transition-CSE and
        // the superscalar-overlap that hide peek's cost when it sits next to an
        // update; the between-block updates keep the state on the same evolving
        // trajectory so data-dependent branches are sampled representatively.
        s.push_str("            for( int pass = 0; pass < 3; pass++ ) {\n");
        s.push_str("                long long tp = 0;\n");
        s.push_str("                for( int b = 0; b < nblk; b++ ) {\n");
        s.push_str("                    long long t0 = get_nanotime();\n");
        s.push_str("                    for( int j = 0; j < blk; j++ ) {\n");
        s.push_str("                        int it = b * blk + j;\n");
        {
            let mut a = vec!["st".to_string()];
            a.extend(bar_scalars.iter().cloned());
            a.push(out_addrs.clone());
            s.push_str(&format!("                        {ta}_Peek({});\n", a.join(", ")));
        }
        s.push_str(&out_acc.replace("                    acc", "                        acc"));
        s.push_str("                    }\n");
        s.push_str("                    tp += get_nanotime() - t0;\n");
        s.push_str("                    for( int j = 0; j < blk; j++ ) {\n");
        s.push_str("                        int it = b * blk + j;\n");
        {
            let mut a = vec!["st".to_string()];
            a.extend(bar_scalars.iter().cloned());
            a.push(out_addrs.clone());
            s.push_str(&format!("                        {ta}_Update({});\n", a.join(", ")));
        }
        s.push_str(&out_acc.replace("                    acc", "                        acc"));
        s.push_str("                    }\n");
        s.push_str("                }\n");
        s.push_str("                if( best_p < 0 || tp < best_p ) best_p = tp;\n");
        s.push_str("            }\n");
        s.push_str("            g_sink += (int)acc + nb;\n");
        s.push_str(&format!("            {ta}_Close(st);\n"));
        s.push_str(&format!(
            "            printf(\"{name} %.3f %.3f %.3f %d %zu\\n\", best_b/(double)iters, best_u/(double)iters, best_p/(double)npk, lb, handle_bytes);\n"
        ));
        s.push_str("        } else {\n");
        s.push_str("            g_sink += (int)acc + nb;\n");
        s.push_str("            if( st ) { g_ta_track = 0; ");
        s.push_str(&format!("{ta}_Close(st); }}\n"));
        s.push_str(&format!(
            "            printf(\"{name} %.3f -1 -1 %d 0\\n\", best_b/(double)iters, lb);\n"
        ));
        s.push_str("        }\n");
        s.push_str("        fflush(stdout);\n");
        s.push_str("    }\n");
    }
    s.push_str("}\n\n");
}

/// Generate the standalone streaming benchmark TU.
pub fn generate_c_stream_bench(funcs: &[FuncDef]) -> String {
    let mut s = String::new();
    s.push_str("/* Auto-generated streaming benchmark for ta_codegen C output.\n");
    s.push_str(" * Per streamable function: batch@last vs Update vs Peek (ns) + handle bytes.\n");
    s.push_str(" * Output: `NAME batch_last update peek lookback handle_bytes` per line.\n");
    s.push_str(" */\n");
    s.push_str("#include <stdio.h>\n#include <stdlib.h>\n#include <string.h>\n");
    s.push_str("#include <math.h>\n#include <time.h>\n");
    s.push_str("#ifdef __APPLE__\n#include <mach/mach_time.h>\n#endif\n\n");

    s.push_str("#include \"ta_func_unguarded.h\"\n");
    s.push_str("#include \"ta_func/ta_func_private.h\"\n\n");
    s.push_str("#include \"ta_common/ta_global.c\"\n");
    s.push_str("#include \"ta_func/ta_utility.c\"\n");
    s.push_str("#include \"ta_common/ta_version.c\"\n");
    s.push_str("#include \"ta_common/ta_retcode.c\"\n\n");

    // Handle-size tracking: override TA_Malloc/TA_Free for the indicator TUs
    // ONLY (included below). A small registry records live allocations while
    // g_ta_track is set (around Open); net-live delta == retained handle bytes.
    s.push_str(HANDLE_TRACKER);

    let mut sorted: Vec<&str> = funcs.iter().map(|f| f.name.as_str()).collect();
    sorted.sort_unstable();
    if let Some(pos) = sorted.iter().position(|n| *n == "MA") {
        let ma = sorted.remove(pos);
        sorted.push(ma);
    }
    for name in &sorted {
        s.push_str(&format!("#include \"ta_{name}.c\"\n"));
    }
    s.push('\n');

    s.push_str(TIMING_HELPER);
    s.push_str(PRICE_DATA_GEN);

    s.push_str("#define MAX_POINTS 200000\n");
    s.push_str("static double g_outBuf0[MAX_POINTS];\n");
    s.push_str("static double g_outBuf1[MAX_POINTS];\n");
    s.push_str("static double g_outBuf2[MAX_POINTS];\n");
    s.push_str("static int g_outIntBuf0[MAX_POINTS];\n");
    s.push_str("static int g_outIntBuf1[MAX_POINTS];\n\n");

    // Growing history batch@last appends into (distinct from the static price
    // arrays the update feed rotates over).
    s.push_str("static double *g_rt_open, *g_rt_high, *g_rt_low, *g_rt_close, *g_rt_volume, *g_rt_oi;\n");
    s.push_str("static int g_rtCap;\n\n");

    s.push_str(FUNC_MATCHES);
    generate_stream_bench_func(&mut s, funcs);
    s.push_str(STREAM_MAIN_FUNC);
    s
}

pub fn write_c_stream_bench(funcs: &[FuncDef], output_dir: &Path) {
    let content = generate_c_stream_bench(funcs);
    let path = output_dir.join("ta_bench_stream.c");
    std::fs::write(&path, &content).unwrap_or_else(|e| {
        panic!("Failed to write {}: {e}", path.display());
    });
    eprintln!("  C stream bench -> {}", path.display());
}

const HANDLE_TRACKER: &str = r"
/* --- retained-handle allocation tracking (indicator TUs only) --- */
#define TRK_MAX 65536
static void  *g_trk_ptr[TRK_MAX];
static size_t g_trk_sz[TRK_MAX];
static int    g_trk_n = 0;
static size_t g_ta_live_bytes = 0;
static int    g_ta_track = 0;
static void g_trk_reset(void) { g_trk_n = 0; g_ta_live_bytes = 0; }
static void *bench_tracked_malloc(size_t n) {
    void *p = malloc(n);
    if( p && g_ta_track ) {
        g_ta_live_bytes += n;
        if( g_trk_n < TRK_MAX ) { g_trk_ptr[g_trk_n] = p; g_trk_sz[g_trk_n] = n; g_trk_n++; }
    }
    return p;
}
static void bench_tracked_free(void *p) {
    if( p && g_ta_track ) {
        for( int i = g_trk_n - 1; i >= 0; i-- ) {
            if( g_trk_ptr[i] == p ) {
                g_ta_live_bytes -= g_trk_sz[i];
                g_trk_ptr[i] = g_trk_ptr[--g_trk_n];
                g_trk_sz[i]  = g_trk_sz[g_trk_n];
                break;
            }
        }
    }
    free(p);
}
#undef TA_Malloc
#undef TA_Free
#define TA_Malloc(a) bench_tracked_malloc(a)
#define TA_Free(a)   bench_tracked_free(a)

";

const STREAM_MAIN_FUNC: &str = r#"
int main(int argc, char *argv[]) {
    TA_Initialize();
    int n_points = 100000;
    int n_iters = 500;
    const char *func_filter = NULL;
    for( int i = 1; i < argc; i++ ) {
        if( strncmp(argv[i], "--points=", 9) == 0 )    n_points = atoi(argv[i]+9);
        else if( strncmp(argv[i], "--iters=", 8) == 0 ) n_iters = atoi(argv[i]+8);
        else if( strncmp(argv[i], "--function=", 11) == 0 ) func_filter = argv[i]+11;
    }
    if( n_points > MAX_POINTS ) n_points = MAX_POINTS;
    if( n_points < BENCH_MASK + 1 ) n_points = BENCH_MASK + 1; /* the bar feed indexes it & BENCH_MASK */
    if( n_iters < 1 ) n_iters = 1;
    generate_price_data(n_points);
    /* Growing history for batch@last: one buffer sized to hold the whole run
       (n_iters appended bars + lookback headroom) so it never recycles within a pass. */
    g_rtCap = n_iters + 8192;
    g_rt_open   = malloc(sizeof(double) * (size_t)g_rtCap);
    g_rt_high   = malloc(sizeof(double) * (size_t)g_rtCap);
    g_rt_low    = malloc(sizeof(double) * (size_t)g_rtCap);
    g_rt_close  = malloc(sizeof(double) * (size_t)g_rtCap);
    g_rt_volume = malloc(sizeof(double) * (size_t)g_rtCap);
    g_rt_oi     = malloc(sizeof(double) * (size_t)g_rtCap);
    if( g_rtCap <= 0 || !g_rt_open || !g_rt_high || !g_rt_low || !g_rt_close || !g_rt_volume || !g_rt_oi ) {
        fprintf( stderr, "ta_bench_stream: allocation failed (try a smaller --iters)\n" );
        return 1;
    }
    for( int i = 0; i < g_rtCap; i++ ) {
        int j = i % g_nPoints;
        g_rt_open[i]=g_open[j]; g_rt_high[i]=g_high[j]; g_rt_low[i]=g_low[j];
        g_rt_close[i]=g_close[j]; g_rt_volume[i]=g_volume[j]; g_rt_oi[i]=g_oi[j];
    }
    bench_stream_all(func_filter, n_iters);
    free(g_rt_open); free(g_rt_high); free(g_rt_low); free(g_rt_close); free(g_rt_volume); free(g_rt_oi);
    free(g_open); free(g_high); free(g_low); free(g_close); free(g_volume); free(g_oi);
    return 0;
}
"#;

const TIMING_HELPER: &str = r"
static long long get_nanotime(void) {
#ifdef __APPLE__
    static mach_timebase_info_data_t info = {0, 0};
    if( info.denom == 0 ) mach_timebase_info(&info);
    uint64_t t = mach_absolute_time();
    return (long long)(t * info.numer / info.denom);
#else
    struct timespec ts;
    if( clock_gettime(CLOCK_MONOTONIC, &ts) == 0 )
        return (long long)ts.tv_sec * 1000000000LL + (long long)ts.tv_nsec;
    return 0;
#endif
}

";

const PRICE_DATA_GEN: &str = r"
static double *g_open, *g_high, *g_low, *g_close, *g_volume, *g_oi;
static int g_nPoints;

static void generate_price_data(int n) {
    g_nPoints = n;
    g_open   = calloc(n, sizeof(double));
    g_high   = calloc(n, sizeof(double));
    g_low    = calloc(n, sizeof(double));
    g_close  = calloc(n, sizeof(double));
    g_volume = calloc(n, sizeof(double));
    g_oi     = calloc(n, sizeof(double));
    unsigned int seed = 42;
    double price = 100.0;
    for( int i = 0; i < n; i++ ) {
        seed = seed * 1103515245 + 12345;
        double r = ((double)(seed >> 16) / 32768.0) - 1.0;
        double o = price, c = price + r * 2.0;
        double h = fmax(o, c) + fabs(r) * 0.5;
        double l = fmin(o, c) - fabs(r) * 0.5;
        if( l < 1.0 ) l = 1.0;
        g_open[i] = o; g_high[i] = h; g_low[i] = l; g_close[i] = c;
        g_volume[i] = 1000000.0 + r * 500000.0;
        price = c; if( price < 1.0 ) price = 1.0;
    }
}

";

const FUNC_MATCHES: &str = r#"
static int func_matches(const char *filter, const char *name) {
    if( !filter || !*filter ) return 1;
    char buf[512]; strncpy(buf, filter, sizeof(buf)-1); buf[sizeof(buf)-1]='\0';
    char *saveptr = NULL;
    for( char *tok = strtok_r(buf, ",", &saveptr); tok; tok = strtok_r(NULL, ",", &saveptr) )
        if( strcasestr(name, tok) ) return 1;
    return 0;
}

"#;

const MAIN_FUNC: &str = r#"
int main(int argc, char *argv[]) {
    TA_Initialize();
    int n_points = 100000;
    int n_iters = 200;
    const char *func_filter = NULL;
    for( int i = 1; i < argc; i++ ) {
        if( strncmp(argv[i], "--points=", 9) == 0 )    n_points = atoi(argv[i]+9);
        else if( strncmp(argv[i], "--iters=", 8) == 0 ) n_iters = atoi(argv[i]+8);
        else if( strncmp(argv[i], "--function=", 11) == 0 ) func_filter = argv[i]+11;
    }
    if( n_points > MAX_POINTS ) n_points = MAX_POINTS;
    generate_price_data(n_points);
    bench_all(func_filter, n_iters);
    free(g_open); free(g_high); free(g_low); free(g_close); free(g_volume); free(g_oi);
    return 0;
}
"#;
