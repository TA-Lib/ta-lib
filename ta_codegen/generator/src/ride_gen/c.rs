use crate::ir::{FuncDef, ParamType};
use crate::server_gen::expand_input_names;
use std::fmt::Write as _;

/// The ride-along: after every eligible batch request, replay that request's own
/// inputs and parameters through this server's streaming tiers and bit-compare
/// against a fresh internal `batch(0, m-1)` over the same prefix.
///
/// Two structural rules here are load-bearing, not stylistic:
///
/// - **Outlined per function.** The server is one `-O3 -flto` translation unit,
///   so text added inside `handle_request` moves the inlining budget and
///   register allocation of the TIMED call site even though it can never
///   execute inside the measured region.
/// - **Every byte inside the `TA_REF_SERVE` guard.** The support block names no
///   stream symbol, so leaving it outside links cleanly into the frozen oracle
///   and silently shifts the reference column every benchmark ratio divides by.
pub(crate) fn generate_c_ridealong(funcs: &[FuncDef]) -> String {
    let (n_out_real, n_out_int) = crate::backends::common::max_output_arity(funcs);
    let mut s = String::new();
    s.push_str("#ifndef TA_REF_SERVE\n");
    s.push_str("/* ---- ride-along: batch-vs-stream on caller-supplied data ---- */\n");
    s.push_str("#define SR_MAX_BARS 4096\n");
    s.push_str("#define SR_SEEN_N 2048\n");
    for k in 0..n_out_real {
        let _ = writeln!(s, "static double sr_b{k}[SR_MAX_BARS], sr_f{k}[SR_MAX_BARS];");
    }
    for k in 0..n_out_int {
        let _ = writeln!(s, "static int sr_ib{k}[SR_MAX_BARS], sr_if{k}[SR_MAX_BARS];");
    }
    s.push_str(RIDE_C_SUPPORT);
    for func in funcs {
        if !func.streaming {
            continue;
        }
        s.push_str(&emit_c_ridealong_fn(func));
    }
    s.push_str("#endif /* TA_REF_SERVE */\n\n");
    s
}

const RIDE_C_SUPPORT: &str = r#"static unsigned long long g_srKey[SR_SEEN_N];
static int g_srKeyUsed[SR_SEEN_N];
static int g_srOpenBars[SR_SEEN_N];
static int g_srFillBars[SR_SEEN_N];

static int sr_gate(const char *json)
{
    if( json_find_int(json, "timed") ) return 0;
    if( json_find_int(json, "no_output") ) return 0;
    if( json_find_int(json, "use_float") ) return 0;
    if( json_find_int(json, "bench_mode") ) return 0;
    if( json_find_int(json, "iters") > 1 ) return 0;
    return 1;
}

/* The ambient state a stream honors at Open is hashed into the dedup key rather
 * than tracked by a counter bumped at each mutation site. A second sweep at a
 * new unstable period over identical bars and parameters would otherwise
 * collide with the first and be skipped, and a counter is a thing to forget. */
static unsigned long long sr_ambient(unsigned long long h)
{
    int i;
    for( i = 0; i < TA_FUNC_UNST_COUNT; i++ )
    {
        unsigned int k = TA_GetUnstablePeriod( (TA_FuncUnstId)i );
        h = fuzz_hash_bytes(h, &k, sizeof(k));
    }
    for( i = 0; i < (int)TA_AllCandleSettings; i++ )
        h = fuzz_hash_bytes(h, &TA_Globals->candleSettings[i], sizeof(TA_CandleSetting));
    return h;
}

/* Update rejects a non-finite bar and Open does not, and an array carrying one
 * is documented undefined -- so the whole replay is skipped rather than read as
 * a divergence. */
static int sr_finite(const double *a, int n)
{
    int i;
    for( i = 0; i < n; i++ ) if( !TA_IS_FINITE(a[i]) ) return 0;
    return 1;
}

static unsigned long long sr_bits(double v)
{
    unsigned long long u;
    memcpy(&u, &v, sizeof(u));
    return u;
}

/* How many elements the request actually carries for `name`. The handler throws
 * this away, and an endIdx past the end of a short array is deliberately sent
 * (the TA_MAX_INDEX probe) -- sizing the replay off endIdx reads whatever the
 * previous request left in the global buffer, with no error. Counted here
 * rather than captured at the parse site so nothing lands outside the guard.
 * -1 means the field is absent. */
static int sr_count_array(const char *json, const char *name)
{
    char key[64];
    const char *p;
    int n = 0;
    snprintf(key, sizeof(key), "\"%s\":", name);
    p = strstr(json, key);
    if( !p ) return -1;
    p += strlen(key);
    while( *p == ' ' ) p++;
    if( *p == '"' )
    {
        /* hex-of-IEEE-bits form: 16 characters per double */
        const char *q = ++p;
        while( *q && *q != '"' ) q++;
        return (int)((q - p) / 16);
    }
    if( *p != '[' ) return -1;
    p++;
    while( *p && *p != ']' )
    {
        while( *p == ' ' || *p == ',' ) p++;
        if( *p == ']' || !*p ) break;
        n++;
        while( *p && *p != ',' && *p != ']' ) p++;
    }
    return n;
}

"#;

/// One outlined `sr_<N>` per streaming function. Comparison counters are
/// incremented BY the comparison (a bar counts only once every output on it
/// matched), never beside it: a counter bumped next to a compare keeps counting
/// after the compare is disabled, which is how an OpenAndFill leg read green
/// across 178 functions for four releases.
#[allow(clippy::too_many_lines)]
fn emit_c_ridealong_fn(func: &FuncDef) -> String {
    let n = func.name.clone();
    let input_names = expand_input_names(&func.inputs);
    let mut s = String::new();

    let mut opt_decls = String::new();
    let mut opt_args = String::new();
    for opt in &func.optional_inputs {
        let ty = match &opt.param_type {
            ParamType::Real => "double",
            ParamType::Enum(_) => "TA_MAType",
            _ => "int",
        };
        let _ = write!(opt_decls, ", {ty} {}", opt.name);
        let _ = write!(opt_args, ", {}", opt.name);
    }
    let opt_call = opt_args.trim_start_matches(", ").to_string();
    let opt_list = if opt_call.is_empty() { String::new() } else { format!("{opt_call}, ") };

    let mut in_args = String::new();
    for j in 0..input_names.len() {
        let _ = write!(in_args, "g_inBuf{j}, ");
    }

    // Batch reference buffers, and the per-bar scalar the stream tiers fill.
    let mut ref_bufs = String::new();
    let mut fill_bufs = String::new();
    let mut scalar_decls = String::new();
    let mut scalar_addrs = String::new();
    let mut slot_of: Vec<(bool, usize)> = Vec::new();
    {
        let (mut r, mut i) = (0usize, 0usize);
        for (k, out) in func.outputs.iter().enumerate() {
            let _ = write!(scalar_addrs, ", &srO{k}");
            if out.param_type == ParamType::Integer {
                let _ = write!(ref_bufs, ", sr_ib{i}");
                let _ = write!(fill_bufs, ", sr_if{i}");
                let _ = writeln!(scalar_decls, "    int srO{k} = 0;");
                slot_of.push((true, i));
                i += 1;
            } else {
                let _ = write!(ref_bufs, ", sr_b{r}");
                let _ = write!(fill_bufs, ", sr_f{r}");
                let _ = writeln!(scalar_decls, "    double srO{k} = 0.0;");
                slot_of.push((false, r));
                r += 1;
            }
        }
    }

    // One bar of the Open/Update leg: every output on the bar must match before
    // the bar counts. `srCmp` is the comparison's own verdict, so deleting the
    // compare cannot leave the counter climbing.
    let cmp_bar = |idx: &str, indent: &str| -> String {
        let mut c = String::new();
        let _ = writeln!(c, "{indent}srCmp = 1;");
        for (k, (is_int, slot)) in slot_of.iter().enumerate() {
            if *is_int {
                let _ = writeln!(
                    c,
                    "{indent}if( srCmp && srO{k} != sr_ib{slot}[{idx}] ) {{ srCmp = 0; srOut = {k}; srA = (double)sr_ib{slot}[{idx}]; srB = (double)srO{k}; }}"
                );
            } else {
                let _ = writeln!(
                    c,
                    "{indent}if( srCmp && sv_xtier_ne(sr_b{slot}[{idx}], srO{k}, &srBenign) ) {{ srCmp = 0; srOut = {k}; srA = sr_b{slot}[{idx}]; srB = srO{k}; }}"
                );
            }
        }
        c
    };

    let _ = writeln!(
        s,
        "static void sr_{n}( const char *json, int endIdx{opt_decls}, char *resp, int resp_size, int *pos )\n{{"
    );
    s.push_str("    int srLb = -1, srM = 0, srAvail, srT, srK, srCmp;\n");
    s.push_str("    int srSkip = 0, srDedup = 0, srOk = 1, srBenign = 0;\n");
    s.push_str("    int srOpenBars = 0, srFillBars = 0;\n");
    s.push_str("    int srLeg = 0, srBar = -1, srOut = -1;\n");
    s.push_str("    double srA = 0.0, srB = 0.0;\n");
    s.push_str("    int srBeg = 0, srNb = 0, srSlot = 0, srRej = 0;\n");
    s.push_str("    unsigned long long srKey;\n");
    s.push_str("    TA_RetCode srRc, srRcB = TA_SUCCESS, srRcO = TA_SUCCESS, srRcF = TA_SUCCESS;\n\n");
    s.push_str("    if( !sr_gate(json) ) return;\n\n");
    // A negative lookback is a REJECTED parameter and travels on: it is the
    // only rejection the ride can reach, and the reject leg below is what
    // reads it. Everything between here and there must tolerate it.
    let _ = writeln!(s, "    srLb = TA_{n}_Lookback( {opt_call} );");
    s.push_str("    srAvail = json_find_int(json, \"use_preloaded\") && g_refN > 0 ? g_refN : endIdx + 1;\n");
    for name in &input_names {
        let _ = writeln!(s, "    {{ int _c = sr_count_array(json, \"{name}\"); if( _c >= 0 && _c < srAvail ) srAvail = _c; }}");
    }
    // The success path shortens the replay to keep the ride cheap. A rejection
    // fails before it reads a bar, so there is nothing to shorten and every
    // entry point is handed the caller's own range.
    s.push_str("    srM = srLb >= 0 ? 2 * srLb + 10 : srAvail;\n");
    s.push_str("    if( srM > srAvail ) srM = srAvail;\n");
    s.push_str("    if( srM > SR_MAX_BARS ) { srSkip = 1; goto sr_out; }\n");
    s.push_str("    if( srM < 1 ) { srSkip = 2; goto sr_out; }\n");
    s.push_str("    if( srLb >= 0 && srM < srLb + 2 ) { srSkip = 3; goto sr_out; }\n");
    s.push_str("    if( ");
    for j in 0..input_names.len() {
        let _ = write!(s, "!sr_finite(g_inBuf{j}, srM) || ");
    }
    s.push_str("0 ) { srSkip = 4; goto sr_out; }\n\n");

    s.push_str("    srKey = sr_ambient(fuzz_hash_init());\n");
    let _ = writeln!(
        s,
        "    srKey = fuzz_hash_bytes(srKey, \"TA_{n}\", {});",
        n.len() + 3
    );
    s.push_str("    srKey = fuzz_hash_bytes(srKey, &srM, sizeof(srM));\n");
    for opt in &func.optional_inputs {
        let _ = writeln!(
            s,
            "    srKey = fuzz_hash_bytes(srKey, &{0}, sizeof({0}));",
            opt.name
        );
    }
    for j in 0..input_names.len() {
        let _ = writeln!(
            s,
            "    srKey = fuzz_hash_bytes(srKey, g_inBuf{j}, (unsigned long)srM * sizeof(double));"
        );
    }
    s.push_str("    srKey = fuzz_hash_fin(srKey);\n");
    s.push_str("    srSlot = (int)(srKey % (unsigned long long)SR_SEEN_N);\n");
    // Only a replay that PASSED is cached, so a divergence re-runs and re-reports
    // at every site instead of being answered once and then hidden.
    s.push_str("    if( g_srKeyUsed[srSlot] && g_srKey[srSlot] == srKey )\n    {\n");
    s.push_str("        srDedup = 1;\n");
    s.push_str("        srOpenBars = g_srOpenBars[srSlot];\n");
    s.push_str("        srFillBars = g_srFillBars[srSlot];\n");
    s.push_str("        goto sr_out;\n    }\n\n");

    let _ = writeln!(
        s,
        "    srRc = TA_{n}( 0, srM - 1, {in_args}{opt_list}&srBeg, &srNb{ref_bufs} );"
    );
    // The reject leg. A rejection is a property of the CALL, so the three entry
    // points owe the same answer on it -- and the same range, because none of
    // them reads a bar before saying no.
    s.push_str("    if( srRc != TA_SUCCESS )\n    {\n");
    let _ = writeln!(s, "        TA_{n}_Stream *srHR = NULL;");
    for l in scalar_decls.lines() {
        let _ = writeln!(s, "    {l}");
    }
    s.push_str("        int srRBeg = 0, srRNb = 0;\n");
    s.push_str("        srRcB = srRc;\n");
    let _ = writeln!(
        s,
        "        srRcO = TA_{n}_Open( &srHR, {in_args}srM, {opt_list}{} );",
        scalar_addrs.trim_start_matches(", ")
    );
    let _ = writeln!(s, "        if( srHR ) TA_{n}_Close( srHR );");
    s.push_str("        srHR = NULL;\n");
    let _ = writeln!(
        s,
        "        srRcF = TA_{n}_OpenAndFill( &srHR, {in_args}srM, {opt_list}&srRBeg, &srRNb{fill_bufs} );"
    );
    let _ = writeln!(s, "        if( srHR ) TA_{n}_Close( srHR );");
    // `srRej` is the comparison's own verdict, exactly like the bar counters:
    // a count bumped beside a compare keeps climbing once the compare is gone.
    s.push_str("        srCmp = srRcO == srRcB;\n");
    s.push_str("        if( srCmp ) srRej++;\n");
    s.push_str("        if( !srCmp ) { srOk = 0; srLeg = 3; }\n");
    s.push_str("        srCmp = srRcF == srRcB;\n");
    s.push_str("        if( srCmp ) srRej++;\n");
    s.push_str("        if( !srCmp ) { srOk = 0; srLeg = 3; }\n");
    s.push_str("        goto sr_out;\n    }\n");
    // Lookback said no and the batch tier said yes: the two read the same
    // parameters, so the replay below has no anchor to stand on.
    s.push_str("    if( srLb < 0 ) { srSkip = 7; goto sr_out; }\n");
    s.push_str("    if( srNb <= 0 ) { srSkip = 5; goto sr_out; }\n");
    // outBegIdx has several assignment forms in src/ta_func and nothing asserts it
    // equals the published lookback; index off the reported beg rather than assume.
    s.push_str("    if( srBeg != srLb ) { srSkip = 6; goto sr_out; }\n\n");

    s.push_str("    {\n");
    let _ = writeln!(s, "        TA_{n}_Stream *srH = NULL;");
    for l in scalar_decls.lines() {
        let _ = writeln!(s, "    {l}");
    }
    let _ = writeln!(
        s,
        "        srRc = TA_{n}_Open( &srH, {in_args}srLb + 1, {opt_list}{} );",
        scalar_addrs.trim_start_matches(", ")
    );
    s.push_str("        if( srRc != TA_SUCCESS || !srH ) { srOk = 0; srLeg = 1; srBar = srLb; }\n");
    // No `else` anywhere in this function, deliberately: a statement inserted
    // between an `if` and its `else` re-binds the `else` to it and kills the
    // block silently. That shape left the real OpenAndFill compare dead across
    // 178 functions for four releases.
    s.push_str("        if( srOk )\n        {\n");
    s.push_str(&cmp_bar("srLb - srBeg", "            "));
    s.push_str("            if( srCmp ) srOpenBars++;\n");
    s.push_str("            if( !srCmp ) { srOk = 0; srLeg = 1; srBar = srLb; }\n");
    s.push_str("            for( srT = srLb + 1; srOk && srT < srM; srT++ )\n            {\n");
    let mut upd_args = String::new();
    for j in 0..input_names.len() {
        let _ = write!(upd_args, "g_inBuf{j}[srT], ");
    }
    let _ = writeln!(
        s,
        "                srRc = TA_{n}_Update( srH, {upd_args}{} );",
        scalar_addrs.trim_start_matches(", ")
    );
    s.push_str("                if( srRc != TA_SUCCESS ) { srOk = 0; srLeg = 1; srBar = srT; break; }\n");
    s.push_str(&cmp_bar("srT - srBeg", "                "));
    s.push_str("                if( srCmp ) srOpenBars++;\n");
    s.push_str("                if( !srCmp ) { srOk = 0; srLeg = 1; srBar = srT; }\n");
    s.push_str("            }\n        }\n");
    let _ = writeln!(s, "        if( srH ) TA_{n}_Close( srH );");
    s.push_str("    }\n\n");

    s.push_str("    if( srOk )\n    {\n");
    let _ = writeln!(s, "        TA_{n}_Stream *srH2 = NULL;");
    s.push_str("        int srFBeg = 0, srFNb = 0;\n");
    let _ = writeln!(
        s,
        "        srRc = TA_{n}_OpenAndFill( &srH2, {in_args}srM, {opt_list}&srFBeg, &srFNb{fill_bufs} );"
    );
    s.push_str("        if( srRc != TA_SUCCESS || !srH2 || srFBeg != srBeg || srFNb != srNb ) { srOk = 0; srLeg = 2; srBar = -1; }\n");
    s.push_str("        if( srOk )\n        {\n");
    s.push_str("            for( srK = 0; srOk && srK < srNb; srK++ )\n            {\n");
    {
        let mut c = String::new();
        let _ = writeln!(c, "                srCmp = 1;");
        for (k, (is_int, slot)) in slot_of.iter().enumerate() {
            if *is_int {
                let _ = writeln!(
                    c,
                    "                if( srCmp && sr_if{slot}[srK] != sr_ib{slot}[srK] ) {{ srCmp = 0; srOut = {k}; srA = (double)sr_ib{slot}[srK]; srB = (double)sr_if{slot}[srK]; }}"
                );
            } else {
                let _ = writeln!(
                    c,
                    "                if( srCmp && sv_xtier_ne(sr_b{slot}[srK], sr_f{slot}[srK], &srBenign) ) {{ srCmp = 0; srOut = {k}; srA = sr_b{slot}[srK]; srB = sr_f{slot}[srK]; }}"
                );
            }
        }
        s.push_str(&c);
    }
    s.push_str("                if( srCmp ) srFillBars++;\n");
    s.push_str("                if( !srCmp ) { srOk = 0; srLeg = 2; srBar = srBeg + srK; }\n");
    s.push_str("            }\n        }\n");
    let _ = writeln!(s, "        if( srH2 ) TA_{n}_Close( srH2 );");
    s.push_str("    }\n\n");

    s.push_str("    if( srOk )\n    {\n");
    s.push_str("        g_srKeyUsed[srSlot] = 1;\n");
    s.push_str("        g_srKey[srSlot] = srKey;\n");
    s.push_str("        g_srOpenBars[srSlot] = srOpenBars;\n");
    s.push_str("        g_srFillBars[srSlot] = srFillBars;\n");
    s.push_str("    }\n\n");

    s.push_str("sr_out:\n");
    s.push_str("    *pos = json_appendf(resp, resp_size, *pos,\n");
    s.push_str("        \",\\\"ride_ok\\\":%d,\\\"ride_skip\\\":%d,\\\"ride_dedup\\\":%d,\\\"ride_open_bars\\\":%d,\\\"ride_fill_bars\\\":%d,\\\"ride_benign\\\":%d,\\\"ride_m\\\":%d,\\\"ride_lb\\\":%d\"\n");
    s.push_str("        \",\\\"ride_rej\\\":%d,\\\"ride_rc_batch\\\":%d,\\\"ride_rc_open\\\":%d,\\\"ride_rc_fill\\\":%d\",\n");
    s.push_str("        srOk, srSkip, srDedup, srOpenBars, srFillBars, srBenign, srM, srLb,\n");
    s.push_str("        srRej, (int)srRcB, (int)srRcO, (int)srRcF);\n");
    s.push_str("    if( !srOk )\n");
    s.push_str("        *pos = json_appendf(resp, resp_size, *pos,\n");
    s.push_str("            \",\\\"ride_leg\\\":%d,\\\"ride_bar\\\":%d,\\\"ride_out\\\":%d,\\\"ride_batch\\\":\\\"%016llx\\\",\\\"ride_stream\\\":\\\"%016llx\\\"\",\n");
    s.push_str("            srLeg, srBar, srOut, sr_bits(srA), sr_bits(srB));\n");
    s.push_str("}\n\n");
    s
}
