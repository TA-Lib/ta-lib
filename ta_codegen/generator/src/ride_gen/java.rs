use super::ride_arg_list;
use crate::ir::{FuncDef, ParamType};
use crate::server_gen::expand_input_names;
use std::fmt::Write as _;

/// The Java ride-along. Split into a body that sets the result and a wrapper
/// that emits it once, because Java has no `goto` and the alternative is the
/// emit repeated at nine returns.
///
/// Each leg gets its OWN try: a throw out of OpenAndFill must not skip
/// Open+Update, or a real library rejection is misfiled as a dead harness leg,
/// which is the one distinction the two error ids exist to preserve.
pub(crate) fn generate_java_ridealong(funcs: &[FuncDef]) -> String {
    let mut s = String::new();
    s.push_str(RIDE_JAVA_SUPPORT);
    for func in funcs {
        if !func.streaming {
            continue;
        }
        s.push_str(&emit_java_ridealong_fn(func));
    }
    s
}

const RIDE_JAVA_SUPPORT: &str = r#"
    // ---- ride-along: batch-vs-stream on caller-supplied data ----
    static final int RIDE_MAX_BARS = 4096;
    static final int RIDE_SEEN_N = 2048;
    static final long[] rideSeenHash = new long[RIDE_SEEN_N];
    static final boolean[] rideSeenUsed = new boolean[RIDE_SEEN_N];
    static final int[] rideSeenOpen = new int[RIDE_SEEN_N];
    static final int[] rideSeenFill = new int[RIDE_SEEN_N];
    static long rideGen = 0;

    static boolean rideGate(String json) {
        if (jsonInt(json, "timed") != 0) return false;
        if (jsonInt(json, "no_output") != 0) return false;
        if (jsonInt(json, "use_float") != 0) return false;
        if (jsonInt(json, "bench_mode") != 0) return false;
        return jsonInt(json, "iters") <= 1;
    }

    /* 0 means the call returned. Anything else is the code C would have
     * returned for the same condition, so the three entry points are comparable
     * without narrowing the catch types; -1 is an exception the library does
     * not own, which is itself a divergence. */
    static int rideCode(RuntimeException e) {
        return (e instanceof TaLibFailure) ? ((TaLibFailure) e).retCode().toInt() : -1;
    }

    static boolean rideFinite(double[] a, int n) {
        for (int i = 0; i < n; i++) if (!Double.isFinite(a[i])) return false;
        return true;
    }

    static long rideMix(long h, long v) {
        long acc = h;
        for (int i = 0; i < 8; i++) {
            acc ^= (v >>> (i * 8)) & 0xffL;
            acc *= 1099511628211L;
        }
        return acc;
    }

    static long rideMixStr(long h, String s) {
        long acc = h;
        for (int i = 0; i < s.length(); i++) {
            acc ^= s.charAt(i) & 0xffL;
            acc *= 1099511628211L;
        }
        return acc;
    }

    static long rideMixArr(long h, double[] a, int n) {
        long acc = h;
        for (int i = 0; i < n; i++) acc = rideMix(acc, Double.doubleToRawLongBits(a[i]));
        return acc;
    }

    static final class RideResult {
        boolean ok = true;
        int skip = 0, dedup = 0, openBars = 0, fillBars = 0;
        int leg = 0, bar = -1, out = -1, m = 0, lb = -1;
        int rej = 0, rcBatch = 0, rcOpen = 0, rcFill = 0;
        long batch = 0, stream = 0;
        long[] benign = new long[1];
        void emit(StringBuilder sb) {
            sb.append(",\"ride_ok\":").append(ok ? 1 : 0)
              .append(",\"ride_skip\":").append(skip)
              .append(",\"ride_dedup\":").append(dedup)
              .append(",\"ride_open_bars\":").append(openBars)
              .append(",\"ride_fill_bars\":").append(fillBars)
              .append(",\"ride_benign\":").append(benign[0])
              .append(",\"ride_m\":").append(m)
              .append(",\"ride_lb\":").append(lb)
              .append(",\"ride_rej\":").append(rej)
              .append(",\"ride_rc_batch\":").append(rcBatch)
              .append(",\"ride_rc_open\":").append(rcOpen)
              .append(",\"ride_rc_fill\":").append(rcFill);
            if (!ok) {
                sb.append(",\"ride_leg\":").append(leg)
                  .append(",\"ride_bar\":").append(bar)
                  .append(",\"ride_out\":").append(out)
                  .append(",\"ride_batch\":\"").append(String.format("%016x", batch)).append('"')
                  .append(",\"ride_stream\":\"").append(String.format("%016x", stream)).append('"');
            }
        }
    }

"#;

#[allow(clippy::too_many_lines)]
fn emit_java_ridealong_fn(func: &FuncDef) -> String {
    let n = func.name.clone();
    let base = crate::backends::common::camel_words(&func.name);
    let input_names = expand_input_names(&func.inputs);
    let outs = &func.outputs;
    let multi = outs.len() > 1;
    let holder = format!("Core.{}Out", crate::backends::common::pascal_words(&func.name));
    let mut s = String::new();

    let mut sig_ins = String::new();
    let mut args_m = String::new();
    let mut args_open = String::new();
    let mut upd_args = String::new();
    // The reject leg hands every entry point the SAME range: nothing reads a
    // bar before it says no, so there is nothing to shorten.
    let mut args_rej = String::new();
    for name in &input_names {
        let _ = write!(sig_ins, "double[] {name}, ");
        let _ = write!(args_m, "java.util.Arrays.copyOf({name}, m), ");
        let _ = write!(args_open, "java.util.Arrays.copyOf({name}, lb + 1), ");
        let _ = write!(args_rej, "java.util.Arrays.copyOf({name}, m), ");
        let _ = write!(upd_args, "{name}[t], ");
    }
    let mut sig_opts = String::new();
    let mut opt_args = String::new();
    for opt in &func.optional_inputs {
        let ty = match &opt.param_type {
            ParamType::Real => "double",
            ParamType::Enum(_) => "MAType",
            _ => "int",
        };
        let _ = write!(sig_opts, "{ty} {}, ", opt.name);
        let _ = write!(opt_args, "{}, ", opt.name);
    }
    let opt_bare = opt_args.trim_end().trim_end_matches(',').to_string();

    let mut ref_decl = String::new();
    let mut fill_decl = String::new();
    let mut ref_args = String::new();
    let mut fill_args = String::new();
    let mut slot_of: Vec<(bool, usize)> = Vec::new();
    {
        let (mut r, mut i) = (0usize, 0usize);
        for out in outs {
            if out.param_type == ParamType::Integer {
                let _ = writeln!(ref_decl, "        int[] rib{i} = new int[m];");
                let _ = writeln!(fill_decl, "            int[] fib{i} = new int[m];");
                let _ = write!(ref_args, ", rib{i}");
                let _ = write!(fill_args, ", fib{i}");
                slot_of.push((true, i));
                i += 1;
            } else {
                let _ = writeln!(ref_decl, "        double[] rb{r} = new double[m];");
                let _ = writeln!(fill_decl, "            double[] fb{r} = new double[m];");
                let _ = write!(ref_args, ", rb{r}");
                let _ = write!(fill_args, ", fb{r}");
                slot_of.push((false, r));
                r += 1;
            }
        }
    }

    // Single-output tiers return the value; multi-output ones fill a holder.
    let val = |k: usize| -> String {
        if multi {
            format!("uo.{}", crate::backends::java_stream::value_field_name(&outs[k].name))
        } else {
            "uv".to_string()
        }
    };

    let cmp_bar = |indent: &str, idx: &str, barexpr: &str| -> String {
        let mut c = String::new();
        let _ = writeln!(c, "{indent}cmp = true;");
        for (k, (is_int, slot)) in slot_of.iter().enumerate() {
            let v = val(k);
            if *is_int {
                let _ = writeln!(
                    c,
                    "{indent}if (cmp && (int) {v} != rib{slot}[{idx}]) {{ cmp = false; r.out = {k}; r.batch = Double.doubleToRawLongBits(rib{slot}[{idx}]); r.stream = Double.doubleToRawLongBits({v}); }}"
                );
            } else {
                let _ = writeln!(
                    c,
                    "{indent}if (cmp && svXtierNe(rb{slot}[{idx}], {v}, r.benign)) {{ cmp = false; r.out = {k}; r.batch = Double.doubleToRawLongBits(rb{slot}[{idx}]); r.stream = Double.doubleToRawLongBits({v}); }}"
                );
            }
        }
        let _ = writeln!(c, "{indent}if (cmp) r.openBars++;");
        let _ = writeln!(c, "{indent}if (!cmp) {{ r.ok = false; r.leg = 1; r.bar = {barexpr}; }}");
        c
    };

    let _ = writeln!(
        s,
        "    static void ride{n}(Core core, String json, int endIdx, {sig_ins}{sig_opts}StringBuilder sb) {{"
    );
    s.push_str("        if (!rideGate(json)) return;\n");
    s.push_str("        RideResult r = new RideResult();\n");
    let _ = writeln!(
        s,
        "        rideBody{n}(core, json, endIdx, {}{}r);",
        ride_arg_list(&input_names),
        opt_args
    );
    s.push_str("        r.emit(sb);\n    }\n\n");

    let _ = writeln!(
        s,
        "    @SuppressWarnings(\"unused\")\n    static void rideBody{n}(Core core, String json, int endIdx, {sig_ins}{sig_opts}RideResult r) {{"
    );
    // A negative lookback is a REJECTED parameter and travels on: it is the
    // only rejection the ride can reach, and the reject leg below is what
    // reads it. Everything between here and there must tolerate it.
    let _ = writeln!(
        s,
        "        try {{ r.lb = core.{n}_Lookback({opt_bare}); }} catch (RuntimeException _e) {{ r.lb = -1; }}"
    );
    s.push_str("        int lb = r.lb;\n");
    s.push_str("        int navail = endIdx + 1;\n");
    for name in &input_names {
        let _ = writeln!(s, "        if ({name}.length < navail) navail = {name}.length;");
    }
    // The success path shortens the replay to keep the ride cheap; the reject
    // path has nothing to shorten and takes the caller's own range.
    s.push_str("        int m = lb >= 0 ? 2 * lb + 10 : navail;\n");
    s.push_str("        if (m > navail) m = navail;\n");
    s.push_str("        r.m = m;\n");
    s.push_str("        if (m > RIDE_MAX_BARS) { r.skip = 1; return; }\n");
    s.push_str("        if (m < 1) { r.skip = 2; return; }\n");
    s.push_str("        if (lb >= 0 && m < lb + 2) { r.skip = 3; return; }\n");
    s.push_str("        if (");
    for name in &input_names {
        let _ = write!(s, "!rideFinite({name}, m) || ");
    }
    s.push_str("false) { r.skip = 4; return; }\n\n");

    s.push_str("        long hash = 0xcbf29ce484222325L;\n");
    let _ = writeln!(s, "        hash = rideMixStr(hash, \"TA_{}\");", n.to_uppercase());
    s.push_str("        hash = rideMix(hash, m);\n");
    s.push_str("        hash = rideMix(hash, rideGen);\n");
    s.push_str("        hash = rideMix(hash, jsonInt(json, \"unstablePeriod\"));\n");
    for opt in &func.optional_inputs {
        match &opt.param_type {
            ParamType::Real => {
                let _ = writeln!(s, "        hash = rideMix(hash, Double.doubleToRawLongBits({}));", opt.name);
            }
            ParamType::Enum(_) => {
                let _ = writeln!(s, "        hash = rideMix(hash, {}.ordinal());", opt.name);
            }
            _ => {
                let _ = writeln!(s, "        hash = rideMix(hash, {});", opt.name);
            }
        }
    }
    for name in &input_names {
        let _ = writeln!(s, "        hash = rideMixArr(hash, {name}, m);");
    }
    s.push_str("        int slot = (int) Math.floorMod(hash, (long) RIDE_SEEN_N);\n");
    s.push_str("        if (rideSeenUsed[slot] && rideSeenHash[slot] == hash) {\n");
    s.push_str("            r.dedup = 1; r.openBars = rideSeenOpen[slot]; r.fillBars = rideSeenFill[slot]; return;\n        }\n\n");

    s.push_str(&ref_decl);
    s.push_str("        int beg = 0;\n        int nb = 0;\n");
    s.push_str("        String clsB = \"\";\n");
    s.push_str("        boolean rejected = false;\n");
    let _ = writeln!(
        s,
        "        try {{ OutRange _rr = core.{n}(0, m - 1, {args_m}{opt_args}{}); beg = _rr.begIdx(); nb = _rr.count(); }}",
        ref_args.trim_start_matches(", ")
    );
    s.push_str("        catch (RuntimeException _e) { r.rcBatch = rideCode(_e); clsB = _e.getClass().getName(); rejected = true; }\n");
    // The reject leg. A rejection is a property of the CALL, so the three entry
    // points owe the same answer on it, over the same range -- and in a backend
    // that throws, the same exception class too: the code alone cannot tell a
    // library rejection from a guard that happens to report the same condition.
    s.push_str("        if (rejected) {\n");
    s.push_str("            String clsO = \"\", clsF = \"\";\n");
    let rej_open = format!("{args_rej}{opt_bare}");
    let rej_open = rej_open.trim_end().trim_end_matches(',').to_string();
    let _ = writeln!(
        s,
        "            try {{ core.{base}Open({rej_open}); }} catch (RuntimeException _e) {{ r.rcOpen = rideCode(_e); clsO = _e.getClass().getName(); }}"
    );
    s.push_str(&fill_decl);
    let _ = writeln!(
        s,
        "            try {{ core.{base}OpenAndFill({args_rej}{opt_args}{}); }} catch (RuntimeException _e) {{ r.rcFill = rideCode(_e); clsF = _e.getClass().getName(); }}",
        fill_args.trim_start_matches(", ")
    );
    // `r.rej` is the comparison's own verdict, exactly like the bar counters:
    // a count bumped beside a compare keeps climbing once the compare is gone.
    // The class is folded into that verdict, and the leg says which half spoke.
    s.push_str("            boolean cmpO = r.rcOpen == r.rcBatch && clsO.equals(clsB);\n");
    s.push_str("            if (cmpO) r.rej++;\n");
    s.push_str("            if (!cmpO) { r.ok = false; r.leg = r.rcOpen == r.rcBatch ? 4 : 3; }\n");
    s.push_str("            boolean cmpF = r.rcFill == r.rcBatch && clsF.equals(clsB);\n");
    s.push_str("            if (cmpF) r.rej++;\n");
    s.push_str("            if (!cmpF) { r.ok = false; r.leg = r.rcFill == r.rcBatch ? 4 : 3; }\n");
    s.push_str("            return;\n        }\n");
    // Lookback said no and the batch tier said yes: the two read the same
    // parameters, so the replay below has no anchor to stand on.
    s.push_str("        if (lb < 0) { r.skip = 7; return; }\n");
    s.push_str("        if (nb == 0) { r.skip = 5; return; }\n");
    s.push_str("        if (beg != lb) { r.skip = 6; return; }\n\n");

    // Leg 1
    s.push_str("        try {\n");
    s.push_str("            boolean cmp;\n");
    let open_call = format!("{args_open}{opt_bare}");
    let open_call = open_call.trim_end().trim_end_matches(',').to_string();
    let _ = writeln!(
        s,
        "            Core.{}Stream st = core.{base}Open({open_call});",
        crate::backends::common::pascal_words(&func.name)
    );
    if multi {
        let _ = writeln!(s, "            {holder} uo = new {holder}(); st.value(uo);");
    } else {
        s.push_str("            double uv = st.value();\n");
    }
    s.push_str(&cmp_bar("            ", "lb - beg", "lb"));
    s.push_str("            for (int t = lb + 1; r.ok && t < m; t++) {\n");
    if multi {
        let _ = writeln!(s, "                st.update({upd_args}uo);");
    } else {
        let _ = writeln!(s, "                double uv2 = st.update({});", upd_args.trim_end().trim_end_matches(','));
        s.push_str("                uv = uv2;\n");
    }
    s.push_str(&cmp_bar("                ", "t - beg", "t"));
    s.push_str("            }\n");
    s.push_str("        } catch (RuntimeException _e) { r.ok = false; r.leg = 1; }\n\n");

    // Leg 2
    s.push_str("        if (r.ok) {\n");
    s.push_str(&fill_decl);
    s.push_str("            try {\n");
    let _ = writeln!(
        s,
        "                Core.{}Stream st2 = core.{base}OpenAndFill({args_m}{opt_args}{});",
        crate::backends::common::pascal_words(&func.name),
        fill_args.trim_start_matches(", ")
    );
    s.push_str("                if (st2.outRange().begIdx() != beg || st2.outRange().count() != nb) { r.ok = false; r.leg = 2; }\n");
    s.push_str("                if (r.ok) {\n");
    s.push_str("                    for (int k = 0; k < nb; k++) {\n");
    s.push_str("                        boolean cmp = true;\n");
    for (k, (is_int, slot)) in slot_of.iter().enumerate() {
        if *is_int {
            let _ = writeln!(
                s,
                "                        if (cmp && fib{slot}[k] != rib{slot}[k]) {{ cmp = false; r.out = {k}; r.batch = Double.doubleToRawLongBits(rib{slot}[k]); r.stream = Double.doubleToRawLongBits(fib{slot}[k]); }}"
            );
        } else {
            let _ = writeln!(
                s,
                "                        if (cmp && svXtierNe(rb{slot}[k], fb{slot}[k], r.benign)) {{ cmp = false; r.out = {k}; r.batch = Double.doubleToRawLongBits(rb{slot}[k]); r.stream = Double.doubleToRawLongBits(fb{slot}[k]); }}"
            );
        }
    }
    s.push_str("                        if (cmp) r.fillBars++;\n");
    s.push_str("                        if (!cmp) { r.ok = false; r.leg = 2; r.bar = beg + k; break; }\n");
    s.push_str("                    }\n                }\n");
    s.push_str("            } catch (RuntimeException _e) { r.ok = false; r.leg = 2; }\n");
    s.push_str("        }\n\n");

    s.push_str("        if (r.ok) {\n");
    s.push_str("            rideSeenUsed[slot] = true; rideSeenHash[slot] = hash;\n");
    s.push_str("            rideSeenOpen[slot] = r.openBars; rideSeenFill[slot] = r.fillBars;\n");
    s.push_str("        }\n");
    s.push_str("    }\n\n");
    s
}
