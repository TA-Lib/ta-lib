//! Generates a standalone direct-call benchmark binary for the C backend.
//!
//! Unlike the JSON-RPC server, this binary generates its own price data and
//! calls indicator functions directly. Output: one `FUNCNAME timing_ns` line
//! per function on stdout.
//!
//! The price data comes from `src/tools/ta_bench/bench_corpus.h`, the same
//! corpus header ta_bench and ta_bench_direct use, so all four benchmark
//! binaries measure the same series for a given (shape, seed, n) — see
//! `--shape` / `--list-shapes`. The default shape reproduces the seed-42 walk
//! these binaries generated inline before the corpus header existed.

use crate::ir::{FuncDef, ParamType};
use crate::server_gen::expand_input_names;
use std::path::Path;
use std::fmt::Write as _;

pub fn generate_c_bench(funcs: &[FuncDef]) -> String {
    // Resolve `PRAGMA TA_ALT` for this language before anything reads a body.
    let resolved = crate::ir::resolve_all(funcs, crate::ir::Lang::C);
    let funcs: &[FuncDef] = &resolved;
    let mut s = String::new();

    s.push_str("/* Auto-generated direct-call benchmark for ta_codegen C output.\n");
    s.push_str(" * No JSON-RPC -- generates its own data, calls functions directly.\n");
    s.push_str(" * Output: FUNCNAME timing_ns (one per line)\n");
    s.push_str(" */\n");
    s.push_str("#include <stdio.h>\n#include <stdlib.h>\n#include <string.h>\n");
    s.push_str("#include <math.h>\n#include <time.h>\n#include <ctype.h>\n");
    s.push_str("#ifdef _WIN32\n#include <windows.h>\n#endif\n#ifdef __APPLE__\n#include <mach/mach_time.h>\n#endif\n\n");
    // The shared benchmark input corpus (src/tools/ta_bench is on the include
    // path — see the ta_bench_cg / ta_bench_stream gcc invocations in main.rs).
    s.push_str("#include \"bench_corpus.h\"\n\n");
    // Fail-loud allocation checking (src/ is on the include path -- same
    // gcc invocations).
    s.push_str("#include \"tools/ta_alloc_check.h\"\n\n");

    // Internal stream declarations (TA_<N>_OpenInternal)
    s.push_str("#include \"ta_func/ta_func_stream_private.h\"\n\n");
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
    // One buffer per output slot the widest function in the corpus uses —
    // counted rather than written down; see `common::max_output_arity` (#262).
    let (n_out_real, n_out_int) = crate::backends::common::max_output_arity(funcs);
    for k in 0..n_out_real {
        let _ = writeln!(s, "static double g_outBuf{k}[MAX_POINTS];");
    }
    for k in 0..n_out_int {
        let _ = writeln!(s, "static int g_outIntBuf{k}[MAX_POINTS];");
    }
    s.push('\n');

    s.push_str(FUNC_MATCHES);
    generate_bench_func(&mut s, funcs);
    s.push_str(&MAIN_FUNC.replace("__CORPUS_ARGS__", CORPUS_ARGS));

    s
}

/// Per expanded array slot: true when that slot is MAVP's per-element period
/// series (`inPeriods`).
///
/// `expand_input_names` renames generic reals to `inReal0`/`inReal1` whenever a
/// function has more than one of them, so by the time a caller sees the expanded
/// name MAVP's `inPeriods` is indistinguishable from any other second real. This
/// walks the ORIGINAL IR inputs in the same slot order to recover it — the same
/// thing `variant_frame.rs` does for `TA_VIN_PERIODS`.
fn periods_slots(func: &FuncDef) -> Vec<bool> {
    let mut slots = Vec::new();
    for inp in &func.inputs {
        match &inp.param_type {
            ParamType::Real => slots.push(inp.name == "inPeriods"),
            ParamType::Price(components) => slots.extend(components.iter().map(|_| false)),
            _ => {} // Integer / Enum inputs are not array parameters
        }
    }
    slots
}

/// Map an expanded input name to the global benchmark array variable.
/// `is_periods` comes from [`periods_slots`] and wins over the name.
fn input_slot_to_global(name: &str, real_idx: usize, is_periods: bool) -> String {
    if is_periods {
        return "g_periods".to_string();
    }
    input_name_to_global(name, real_idx)
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
        // MAVP's per-element period series (same special case as the variant
        // frame's TA_VIN_PERIODS): a price series here clamps every bar to
        // maxPeriod, so the bench would only ever measure the one-distinct-
        // period fast path. Only reachable when it is the sole generic real —
        // otherwise the slot is flagged by `periods_slots`, which wins.
        "inPeriods" => "g_periods".to_string(),
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
        let periods = periods_slots(func);
        let mut real_idx = 0;
        for (k, inp_name) in input_names.iter().enumerate() {
            args.push(input_slot_to_global(
                inp_name,
                real_idx,
                periods.get(k).copied().unwrap_or(false),
            ));
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
    crate::emit::write_if_changed(&path, &content).unwrap_or_else(|e| {
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
// - peek_ns       : one `TA_XXX_Peek` (the same transition, rewritten to commit
//   nothing) — the same rotating feed and index cost as update, so the
//   peek-minus-update delta is what running it non-committing costs.
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

/// Row printer + the tally behind the summary and `--min-ratio`.
///
/// `speedup = batch_last_ns / update_ns` is the number this binary exists to
/// produce: streaming is only worth choosing over a batch recompute when it is
/// above 1. Both halves are measured in one TU on one input, so unlike every
/// other ratio in the tree it is not comparing two build configurations.
const STREAM_ROW_HELPER: &str = r##"static double g_min_ratio = 0.0;   /* 0 = report only, no gate */
static double g_worst_ratio = -1.0;
static char   g_worst_name[64] = "";
static int    g_rows = 0, g_slow = 0, g_below = 0, g_reject = 0;

static void bench_stream_row(const char *name, double b, double u, double p,
                             int lb, size_t hb)
{
    if( u <= 0.0 ) {   /* Open rejected the default params */
        printf("%s %.3f -1 -1 %d 0 -1\n", name, b, lb);
        g_reject++;
        return;
    }
    double r = b / u;
    printf("%s %.3f %.3f %.3f %d %zu %.3f\n", name, b, u, p, lb, hb, r);
    g_rows++;
    if( r < 1.0 ) g_slow++;
    if( g_min_ratio > 0.0 && r < g_min_ratio ) g_below++;
    if( g_worst_ratio < 0.0 || r < g_worst_ratio ) {
        g_worst_ratio = r;
        snprintf(g_worst_name, sizeof(g_worst_name), "%s", name);
    }
}

/* Returns the process exit code: non-zero only when --min-ratio is set and
   something came in under it, so this can gate a nightly. */
static int bench_stream_summary(void)
{
    printf("# %d timed, %d rejected; %d slower than batch@last; worst %s %.2fx\n",
           g_rows, g_reject, g_slow,
           g_worst_name[0] ? g_worst_name : "-", g_worst_ratio);
    if( g_min_ratio > 0.0 ) {
        printf("# --min-ratio=%.2f: %d below -> %s\n",
               g_min_ratio, g_below, g_below ? "FAIL" : "PASS");
        return g_below ? 1 : 0;
    }
    return 0;
}

"##;

#[allow(clippy::too_many_lines)]
fn generate_stream_bench_func(s: &mut String, funcs: &[FuncDef]) {
    s.push_str("static volatile int g_sink = 0;\n\n");
    s.push_str("#define BENCH_MASK 4095\n\n");
    s.push_str(STREAM_ROW_HELPER);
    s.push_str("static void bench_stream_all(const char *filter, int iters) {\n");
    s.push_str("    printf(\"# func batch_last_ns update_ns peek_ns lookback handle_bytes speedup\\n\");\n");
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
        let periods = periods_slots(func);
        let mut in_arrays = Vec::new();
        let mut bar_scalars = Vec::new();
        let mut real_idx = 0usize;
        for (k, inp) in input_names.iter().enumerate() {
            let g = input_slot_to_global(inp, real_idx, periods.get(k).copied().unwrap_or(false));
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
            "            bench_stream_row(\"{name}\", best_b/(double)iters, best_u/(double)iters, best_p/(double)npk, lb, handle_bytes);\n"
        ));
        s.push_str("        } else {\n");
        s.push_str("            g_sink += (int)acc + nb;\n");
        s.push_str("            if( st ) { g_ta_track = 0; ");
        s.push_str(&format!("{ta}_Close(st); }}\n"));
        s.push_str(&format!(
            "            bench_stream_row(\"{name}\", best_b/(double)iters, -1.0, -1.0, lb, 0);\n"
        ));
        s.push_str("        }\n");
        s.push_str("        fflush(stdout);\n");
        s.push_str("    }\n");
    }
    s.push_str("}\n\n");
}

/// Generate the standalone streaming benchmark TU.
pub fn generate_c_stream_bench(funcs: &[FuncDef]) -> String {
    // Resolve `PRAGMA TA_ALT` for this language before anything reads a body.
    let resolved = crate::ir::resolve_all(funcs, crate::ir::Lang::C);
    let funcs: &[FuncDef] = &resolved;
    let mut s = String::new();
    s.push_str("/* Auto-generated streaming benchmark for ta_codegen C output.\n");
    s.push_str(" * Per streamable function: batch@last vs Update vs Peek (ns) + handle bytes.\n");
    s.push_str(" * Output: `NAME batch_last update peek lookback handle_bytes` per line.\n");
    s.push_str(" */\n");
    s.push_str("#include <stdio.h>\n#include <stdlib.h>\n#include <string.h>\n");
    s.push_str("#include <math.h>\n#include <time.h>\n#include <ctype.h>\n");
    s.push_str("#ifdef _WIN32\n#include <windows.h>\n#endif\n#ifdef __APPLE__\n#include <mach/mach_time.h>\n#endif\n\n");
    // The shared benchmark input corpus (src/tools/ta_bench is on the include
    // path — see the ta_bench_cg / ta_bench_stream gcc invocations in main.rs).
    s.push_str("#include \"bench_corpus.h\"\n\n");
    // Fail-loud allocation checking (src/ is on the include path -- same
    // gcc invocations).
    s.push_str("#include \"tools/ta_alloc_check.h\"\n\n");

    s.push_str("#include \"ta_func/ta_func_stream_private.h\"\n\n");
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
    // One buffer per output slot the widest function in the corpus uses —
    // counted rather than written down; see `common::max_output_arity` (#262).
    let (n_out_real, n_out_int) = crate::backends::common::max_output_arity(funcs);
    for k in 0..n_out_real {
        let _ = writeln!(s, "static double g_outBuf{k}[MAX_POINTS];");
    }
    for k in 0..n_out_int {
        let _ = writeln!(s, "static int g_outIntBuf{k}[MAX_POINTS];");
    }
    s.push('\n');

    // Growing history batch@last appends into (distinct from the static price
    // arrays the update feed rotates over).
    s.push_str("static double *g_rt_open, *g_rt_high, *g_rt_low, *g_rt_close, *g_rt_volume, *g_rt_oi, *g_rt_periods;\n");
    s.push_str("static int g_rtCap;\n\n");

    s.push_str(FUNC_MATCHES);
    generate_stream_bench_func(&mut s, funcs);
    s.push_str(&STREAM_MAIN_FUNC.replace("__CORPUS_ARGS__", CORPUS_ARGS));
    s
}

pub fn write_c_stream_bench(funcs: &[FuncDef], output_dir: &Path) {
    let content = generate_c_stream_bench(funcs);
    let path = output_dir.join("ta_bench_stream.c");
    crate::emit::write_if_changed(&path, &content).unwrap_or_else(|e| {
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
    int verify_corpus = 0;
    const char *func_filter = NULL;
    for( int i = 1; i < argc; i++ ) {
        if( strncmp(argv[i], "--points=", 9) == 0 )    n_points = atoi(argv[i]+9);
        else if( strncmp(argv[i], "--iters=", 8) == 0 ) n_iters = atoi(argv[i]+8);
        else if( strncmp(argv[i], "--function=", 11) == 0 ) func_filter = argv[i]+11;
        /* Gate: exit non-zero if any function streams slower than this multiple
           of its batch@last cost. Stream-bench only, hence not in CORPUS_ARGS. */
        else if( strncmp(argv[i], "--min-ratio=", 12) == 0 ) g_min_ratio = atof(argv[i]+12);
__CORPUS_ARGS__    }
    if( n_points > MAX_POINTS ) n_points = MAX_POINTS;
    if( n_points < BENCH_MASK + 1 ) n_points = BENCH_MASK + 1; /* the bar feed indexes it & BENCH_MASK */
    if( n_iters < 1 ) n_iters = 1;
    /* After the loop, so the check runs at the n actually benchmarked
       regardless of where --points sits in argv. */
    if( verify_corpus ) return bench_corpus_selfcheck(n_points, &g_corpus) ? 1 : 0;
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
    g_rt_periods = malloc(sizeof(double) * (size_t)g_rtCap);
    if( g_rtCap <= 0 || !g_rt_open || !g_rt_high || !g_rt_low || !g_rt_close || !g_rt_volume || !g_rt_oi || !g_rt_periods ) {
        fprintf( stderr, "ta_bench_stream: allocation failed (try a smaller --iters)\n" );
        return 1;
    }
    for( int i = 0; i < g_rtCap; i++ ) {
        int j = i % g_nPoints;
        g_rt_open[i]=g_open[j]; g_rt_high[i]=g_high[j]; g_rt_low[i]=g_low[j];
        g_rt_close[i]=g_close[j]; g_rt_volume[i]=g_volume[j]; g_rt_oi[i]=g_oi[j];
        g_rt_periods[i]=g_periods[j];
    }
    bench_stream_all(func_filter, n_iters);
    int rc = bench_stream_summary();
    free(g_rt_open); free(g_rt_high); free(g_rt_low); free(g_rt_close); free(g_rt_volume); free(g_rt_oi); free(g_rt_periods);
    free(g_open); free(g_high); free(g_low); free(g_close); free(g_volume); free(g_oi); free(g_periods);
    return rc;
}
"#;

const TIMING_HELPER: &str = r"
static long long get_nanotime(void) {
#if defined(_WIN32)
    /* No clock_gettime in the MSVC CRT. QueryPerformanceCounter is the
     * monotonic counter here; its frequency is fixed for the lifetime of the
     * process, so one query is enough. Scaling ticks->ns as (t/f)*1e9 would
     * truncate to whole seconds, and t*1e9 overflows a signed 64-bit at
     * ~9.2e9 ticks (roughly an hour at a 10 MHz QPC), so split the tick count
     * into whole seconds plus a remainder before scaling.
     */
    static LARGE_INTEGER freq = {0};
    LARGE_INTEGER now;
    if( freq.QuadPart == 0 ) QueryPerformanceFrequency(&freq);
    QueryPerformanceCounter(&now);
    return (long long)((now.QuadPart / freq.QuadPart) * 1000000000LL
         + ((now.QuadPart % freq.QuadPart) * 1000000000LL) / freq.QuadPart);
#elif defined(__APPLE__)
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

const PRICE_DATA_GEN: &str = r#"
static double *g_open, *g_high, *g_low, *g_close, *g_volume, *g_oi, *g_periods;
static int g_nPoints;

/* Corpus selection (--shape / --seed / --regime-period / --trend-strength).
 * The defaults reproduce the seed-42 walk this binary generated inline before
 * bench_corpus.h existed. */
static BenchCorpusCfg g_corpus = { BENCH_RANDWALK, BENCH_CORPUS_SEED,
                                   BENCH_CORPUS_PERIOD, BENCH_CORPUS_TREND };

static void generate_price_data(int n) {
    g_nPoints = n;
    g_open   = calloc(n, sizeof(double));
    g_high   = calloc(n, sizeof(double));
    g_low    = calloc(n, sizeof(double));
    g_close  = calloc(n, sizeof(double));
    g_volume = calloc(n, sizeof(double));
    g_oi     = calloc(n, sizeof(double));
    g_periods = calloc(n, sizeof(double));
    if( !g_open || !g_high || !g_low || !g_close || !g_volume || !g_oi || !g_periods )
        TA_TOOL_OOM("the price data arrays");
    bench_corpus_gen(&g_corpus, n,
                     g_open, g_high, g_low, g_close, g_volume, g_oi, g_periods);
}

"#;

/* Shared --shape / --seed / --regime-period / --list-shapes handling, spliced
 * into both generated mains. Unknown shape names fail loudly rather than
 * silently benchmarking the default. */
const CORPUS_ARGS: &str = r#"        else if( strncmp(argv[i], "--shape=", 8) == 0 ) {
            g_corpus.shape = bench_shape_id(argv[i]+8);
            if( g_corpus.shape < 0 ) {
                printf("unknown --shape=%s\n\n", argv[i]+8);
                bench_shape_list();
                return 1;
            }
        }
        else if( strncmp(argv[i], "--seed=", 7) == 0 )   g_corpus.seed = atoi(argv[i]+7);
        else if( strncmp(argv[i], "--regime-period=", 16) == 0 ) g_corpus.refPeriod = atoi(argv[i]+16);
        else if( strncmp(argv[i], "--trend-strength=", 17) == 0 ) g_corpus.trendStrength = atof(argv[i]+17);
        else if( strcmp(argv[i], "--list-shapes") == 0 ) { bench_shape_list(); return 0; }
        else if( strcmp(argv[i], "--verify-corpus") == 0 ) verify_corpus = 1;
        else {
            /* Reject rather than ignore. ta_bench_direct forwards the corpus
             * flags to this binary unconditionally, so a silently-dropped flag
             * makes it time two DIFFERENT input classes and print the ratio as
             * if they matched — a wrong answer with no diagnostic. */
            fprintf(stderr, "%s: unknown option '%s'\n", argv[0], argv[i]);
            return 2;
        }
"#;

const FUNC_MATCHES: &str = r#"
/* strtok_r and strcasestr are POSIX; the MSVC CRT has neither. strtok_s is the
 * same call with the same reentrancy contract, and the case-insensitive search
 * is short enough to spell out rather than reach for a platform extension.
 */
#if defined(_WIN32)
#define bench_strtok_r(str, delim, save) strtok_s((str), (delim), (save))
#else
#define bench_strtok_r(str, delim, save) strtok_r((str), (delim), (save))
#endif

static const char *bench_strcasestr(const char *hay, const char *needle) {
    if( !*needle ) return hay;
    for( ; *hay; hay++ )
    {
        const char *h = hay, *n = needle;
        while( *h && *n && tolower((unsigned char)*h) == tolower((unsigned char)*n) )
        {
            h++;
            n++;
        }
        if( !*n ) return hay;
    }
    return NULL;
}

static int func_matches(const char *filter, const char *name) {
    if( !filter || !*filter ) return 1;
    char buf[512]; strncpy(buf, filter, sizeof(buf)-1); buf[sizeof(buf)-1]='\0';
    char *saveptr = NULL;
    for( char *tok = bench_strtok_r(buf, ",", &saveptr); tok; tok = bench_strtok_r(NULL, ",", &saveptr) )
        if( bench_strcasestr(name, tok) ) return 1;
    return 0;
}

"#;

const MAIN_FUNC: &str = r#"
int main(int argc, char *argv[]) {
    TA_Initialize();
    int n_points = 100000;
    int n_iters = 200;
    int verify_corpus = 0;
    const char *func_filter = NULL;
    for( int i = 1; i < argc; i++ ) {
        if( strncmp(argv[i], "--points=", 9) == 0 )    n_points = atoi(argv[i]+9);
        else if( strncmp(argv[i], "--iters=", 8) == 0 ) n_iters = atoi(argv[i]+8);
        else if( strncmp(argv[i], "--function=", 11) == 0 ) func_filter = argv[i]+11;
__CORPUS_ARGS__    }
    if( n_points > MAX_POINTS ) n_points = MAX_POINTS;
    /* After the loop, so the check runs at the n actually benchmarked
       regardless of where --points sits in argv. */
    if( verify_corpus ) return bench_corpus_selfcheck(n_points, &g_corpus) ? 1 : 0;
    generate_price_data(n_points);
    bench_all(func_filter, n_iters);
    free(g_open); free(g_high); free(g_low); free(g_close); free(g_volume); free(g_oi); free(g_periods);
    return 0;
}
"#;
