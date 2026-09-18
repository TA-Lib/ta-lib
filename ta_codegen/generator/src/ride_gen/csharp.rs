use super::ride_arg_list;
use crate::ir::{FuncDef, ParamType};
use crate::server_gen::expand_input_names;
use std::fmt::Write as _;

/// The C# ride-along, mirroring Java's split. Outlined for the same reason the
/// others are: `Handle_<N>` runs in Tier0 during a bench measurement, so its
/// body must be unchanged except one call.
pub(crate) fn generate_csharp_ridealong(funcs: &[FuncDef]) -> String {
    let mut s = String::new();
    s.push_str(RIDE_CSHARP_SUPPORT);
    for func in funcs {
        if !func.streaming {
            continue;
        }
        s.push_str(&emit_csharp_ridealong_fn(func));
    }
    s
}

const RIDE_CSHARP_SUPPORT: &str = r#"
    // ---- ride-along: batch-vs-stream on caller-supplied data ----
    const int RIDE_MAX_BARS = 4096;
    const int RIDE_SEEN_N = 2048;
    static readonly ulong[] rideSeenHash = new ulong[RIDE_SEEN_N];
    static readonly bool[] rideSeenUsed = new bool[RIDE_SEEN_N];
    static readonly int[] rideSeenOpen = new int[RIDE_SEEN_N];
    static readonly int[] rideSeenFill = new int[RIDE_SEEN_N];
    static ulong rideGen = 0;

    static bool RideGate(JsonElement p)
    {
        if (GetInt(p, "timed", 0) != 0) return false;
        if (GetInt(p, "no_output", 0) != 0) return false;
        if (GetInt(p, "use_float", 0) != 0) return false;
        if (GetInt(p, "bench_mode", 0) != 0) return false;
        return GetInt(p, "iters", 1) <= 1;
    }

    /* 0 means the call returned. Anything else is the code C would have
     * returned for the same condition, so the three entry points are comparable
     * without narrowing the catch types; -1 is an exception the library does
     * not own, which is itself a divergence. */
    static int RideCode(Exception e) => e is ITALibFailure f ? (int) f.RetCode : -1;

    static bool RideFinite(double[] a, int n)
    {
        for (int i = 0; i < n; i++) if (!double.IsFinite(a[i])) return false;
        return true;
    }

    static ulong RideMix(ulong h, ulong v)
    {
        ulong acc = h;
        for (int i = 0; i < 8; i++)
        {
            acc ^= (v >> (i * 8)) & 0xffUL;
            acc *= 1099511628211UL;
        }
        return acc;
    }

    static ulong RideMixStr(ulong h, string s)
    {
        ulong acc = h;
        for (int i = 0; i < s.Length; i++)
        {
            acc ^= (ulong)(s[i] & 0xff);
            acc *= 1099511628211UL;
        }
        return acc;
    }

    static ulong RideMixArr(ulong h, double[] a, int n)
    {
        ulong acc = h;
        for (int i = 0; i < n; i++) acc = RideMix(acc, (ulong)BitConverter.DoubleToInt64Bits(a[i]));
        return acc;
    }

    sealed class RideResult
    {
        public bool Ok = true;
        public int Skip, Dedup, OpenBars, FillBars, Leg, Bar = -1, Out = -1, M, Lb = -1;
        public int Rej, RcBatch, RcOpen, RcFill;
        public long Batch, Stream;
        public long[] Benign = new long[1];
        public void Emit(System.Text.StringBuilder sb)
        {
            sb.Append($",\"ride_ok\":{(Ok ? 1 : 0)},\"ride_skip\":{Skip},\"ride_dedup\":{Dedup}");
            sb.Append($",\"ride_open_bars\":{OpenBars},\"ride_fill_bars\":{FillBars}");
            sb.Append($",\"ride_benign\":{Benign[0]},\"ride_m\":{M},\"ride_lb\":{Lb}");
            sb.Append($",\"ride_rej\":{Rej},\"ride_rc_batch\":{RcBatch}");
            sb.Append($",\"ride_rc_open\":{RcOpen},\"ride_rc_fill\":{RcFill}");
            if (!Ok)
            {
                sb.Append($",\"ride_leg\":{Leg},\"ride_bar\":{Bar},\"ride_out\":{Out}");
                sb.Append($",\"ride_batch\":\"{Batch:x16}\",\"ride_stream\":\"{Stream:x16}\"");
            }
        }
    }

"#;

#[allow(clippy::too_many_lines)]
fn emit_csharp_ridealong_fn(func: &FuncDef) -> String {
    let n = crate::backends::common::pascal_words(&func.name);
    let pas = crate::backends::common::pascal_words(&func.name);
    let input_names = expand_input_names(&func.inputs);
    let outs = &func.outputs;
    let multi = outs.len() > 1;
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
        let _ = write!(args_m, "{name}.AsSpan(0, m), ");
        let _ = write!(args_open, "{name}.AsSpan(0, lb + 1), ");
        let _ = write!(args_rej, "{name}.AsSpan(0, m), ");
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

    let val = |k: usize| -> String {
        if multi {
            format!("uv.{}", crate::backends::csharp_stream::value_member_name(&outs[k].name))
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
                    "{indent}if (cmp && (int) {v} != rib{slot}[{idx}]) {{ cmp = false; r.Out = {k}; r.Batch = BitConverter.DoubleToInt64Bits(rib{slot}[{idx}]); r.Stream = BitConverter.DoubleToInt64Bits({v}); }}"
                );
            } else {
                let _ = writeln!(
                    c,
                    "{indent}if (cmp && SvXtierNe(rb{slot}[{idx}], {v}, ref r.Benign[0])) {{ cmp = false; r.Out = {k}; r.Batch = BitConverter.DoubleToInt64Bits(rb{slot}[{idx}]); r.Stream = BitConverter.DoubleToInt64Bits({v}); }}"
                );
            }
        }
        let _ = writeln!(c, "{indent}if (cmp) r.OpenBars++;");
        let _ = writeln!(c, "{indent}if (!cmp) {{ r.Ok = false; r.Leg = 1; r.Bar = {barexpr}; }}");
        c
    };

    let _ = writeln!(
        s,
        "    static void Ride{n}(Core core, JsonElement p, int endIdx, {sig_ins}{sig_opts}System.Text.StringBuilder sb)\n    {{"
    );
    s.push_str("        if (!RideGate(p)) return;\n");
    s.push_str("        RideResult r = new RideResult();\n");
    let _ = writeln!(
        s,
        "        RideBody{n}(core, p, endIdx, {}{}r);",
        ride_arg_list(&input_names),
        opt_args
    );
    s.push_str("        r.Emit(sb);\n    }\n\n");

    let _ = writeln!(
        s,
        "    static void RideBody{n}(Core core, JsonElement p, int endIdx, {sig_ins}{sig_opts}RideResult r)\n    {{"
    );
    // A negative lookback is a REJECTED parameter and travels on: it is the
    // only rejection the ride can reach, and the reject leg below is what
    // reads it. Everything between here and there must tolerate it.
    let _ = writeln!(
        s,
        "        try {{ r.Lb = core.{n}Lookback({opt_bare}); }} catch (Exception) {{ r.Lb = -1; }}"
    );
    s.push_str("        int lb = r.Lb;\n");
    s.push_str("        int navail = endIdx + 1;\n");
    for name in &input_names {
        let _ = writeln!(s, "        if ({name}.Length < navail) navail = {name}.Length;");
    }
    // The success path shortens the replay to keep the ride cheap; the reject
    // path has nothing to shorten and takes the caller's own range.
    s.push_str("        int m = lb >= 0 ? 2 * lb + 10 : navail;\n");
    s.push_str("        if (m > navail) m = navail;\n");
    s.push_str("        r.M = m;\n");
    s.push_str("        if (m > RIDE_MAX_BARS) { r.Skip = 1; return; }\n");
    s.push_str("        if (m < 1) { r.Skip = 2; return; }\n");
    s.push_str("        if (lb >= 0 && m < lb + 2) { r.Skip = 3; return; }\n");
    s.push_str("        if (");
    for name in &input_names {
        let _ = write!(s, "!RideFinite({name}, m) || ");
    }
    s.push_str("false) { r.Skip = 4; return; }\n\n");

    s.push_str("        ulong hash = 0xcbf29ce484222325UL;\n");
    let _ = writeln!(s, "        hash = RideMixStr(hash, \"TA_{}\");", func.name.to_uppercase());
    s.push_str("        hash = RideMix(hash, (ulong) m);\n");
    s.push_str("        hash = RideMix(hash, rideGen);\n");
    s.push_str("        hash = RideMix(hash, (ulong)(long) GetInt(p, \"unstablePeriod\", 0));\n");
    for opt in &func.optional_inputs {
        match &opt.param_type {
            ParamType::Real => {
                let _ = writeln!(s, "        hash = RideMix(hash, (ulong) BitConverter.DoubleToInt64Bits({}));", opt.name);
            }
            // An enum is an int in C#; both fold the same way.
            _ => {
                let _ = writeln!(s, "        hash = RideMix(hash, (ulong)(long) {});", opt.name);
            }
        }
    }
    for name in &input_names {
        let _ = writeln!(s, "        hash = RideMixArr(hash, {name}, m);");
    }
    s.push_str("        int slot = (int)(hash % (ulong) RIDE_SEEN_N);\n");
    s.push_str("        if (rideSeenUsed[slot] && rideSeenHash[slot] == hash)\n        {\n");
    s.push_str("            r.Dedup = 1; r.OpenBars = rideSeenOpen[slot]; r.FillBars = rideSeenFill[slot]; return;\n        }\n\n");

    s.push_str(&ref_decl);
    s.push_str("        int beg = 0;\n        int nb = 0;\n");
    s.push_str("        string clsB = \"\";\n");
    s.push_str("        bool rejected = false;\n");
    let _ = writeln!(
        s,
        "        try {{ OutRange _rr = core.{n}(0, m - 1, {args_m}{opt_args}{}); beg = _rr.BegIdx; nb = _rr.Count; }}",
        ref_args.trim_start_matches(", ")
    );
    s.push_str("        catch (Exception _e) { r.RcBatch = RideCode(_e); clsB = _e.GetType().FullName ?? \"\"; rejected = true; }\n");
    // The reject leg. A rejection is a property of the CALL, so the three entry
    // points owe the same answer on it, over the same range -- and in a backend
    // that throws, the same exception class too: the code alone cannot tell a
    // library rejection from a guard that happens to report the same condition.
    s.push_str("        if (rejected)\n        {\n");
    s.push_str("            string clsO = \"\", clsF = \"\";\n");
    let rej_open = format!("{args_rej}{opt_bare}");
    let rej_open = rej_open.trim_end().trim_end_matches(',').to_string();
    let _ = writeln!(
        s,
        "            try {{ core.{pas}Open({rej_open}); }} catch (Exception _e) {{ r.RcOpen = RideCode(_e); clsO = _e.GetType().FullName ?? \"\"; }}"
    );
    s.push_str(&fill_decl);
    let _ = writeln!(
        s,
        "            try {{ core.{pas}OpenAndFill({args_rej}{opt_args}{}); }} catch (Exception _e) {{ r.RcFill = RideCode(_e); clsF = _e.GetType().FullName ?? \"\"; }}",
        fill_args.trim_start_matches(", ")
    );
    // `r.Rej` is the comparison's own verdict, exactly like the bar counters:
    // a count bumped beside a compare keeps climbing once the compare is gone.
    // The class is folded into that verdict, and the leg says which half spoke.
    s.push_str("            bool cmpO = r.RcOpen == r.RcBatch && clsO == clsB;\n");
    s.push_str("            if (cmpO) r.Rej++;\n");
    s.push_str("            if (!cmpO) { r.Ok = false; r.Leg = r.RcOpen == r.RcBatch ? 4 : 3; }\n");
    s.push_str("            bool cmpF = r.RcFill == r.RcBatch && clsF == clsB;\n");
    s.push_str("            if (cmpF) r.Rej++;\n");
    s.push_str("            if (!cmpF) { r.Ok = false; r.Leg = r.RcFill == r.RcBatch ? 4 : 3; }\n");
    s.push_str("            return;\n        }\n");
    // Lookback said no and the batch tier said yes: the two read the same
    // parameters, so the replay below has no anchor to stand on.
    s.push_str("        if (lb < 0) { r.Skip = 7; return; }\n");
    s.push_str("        if (nb == 0) { r.Skip = 5; return; }\n");
    s.push_str("        if (beg != lb) { r.Skip = 6; return; }\n\n");

    s.push_str("        try\n        {\n");
    s.push_str("            bool cmp;\n");
    let open_call = format!("{args_open}{opt_bare}");
    let open_call = open_call.trim_end().trim_end_matches(',').to_string();
    let _ = writeln!(s, "            var st = core.{pas}Open({open_call});");
    s.push_str("            var uv = st.Value;\n");
    s.push_str(&cmp_bar("            ", "lb - beg", "lb"));
    s.push_str("            for (int t = lb + 1; r.Ok && t < m; t++)\n            {\n");
    let _ = writeln!(s, "                uv = st.Update({});", upd_args.trim_end().trim_end_matches(','));
    s.push_str(&cmp_bar("                ", "t - beg", "t"));
    s.push_str("            }\n");
    s.push_str("        }\n        catch (Exception) { r.Ok = false; r.Leg = 1; }\n\n");

    s.push_str("        if (r.Ok)\n        {\n");
    s.push_str(&fill_decl);
    s.push_str("            try\n            {\n");
    let _ = writeln!(
        s,
        "                var st2 = core.{pas}OpenAndFill({args_m}{opt_args}{});",
        fill_args.trim_start_matches(", ")
    );
    s.push_str("                if (st2.OutRange.BegIdx != beg || st2.OutRange.Count != nb) { r.Ok = false; r.Leg = 2; }\n");
    s.push_str("                if (r.Ok)\n                {\n");
    s.push_str("                    for (int k = 0; k < nb; k++)\n                    {\n");
    s.push_str("                        bool cmp = true;\n");
    for (k, (is_int, slot)) in slot_of.iter().enumerate() {
        if *is_int {
            let _ = writeln!(
                s,
                "                        if (cmp && fib{slot}[k] != rib{slot}[k]) {{ cmp = false; r.Out = {k}; r.Batch = BitConverter.DoubleToInt64Bits(rib{slot}[k]); r.Stream = BitConverter.DoubleToInt64Bits(fib{slot}[k]); }}"
            );
        } else {
            let _ = writeln!(
                s,
                "                        if (cmp && SvXtierNe(rb{slot}[k], fb{slot}[k], ref r.Benign[0])) {{ cmp = false; r.Out = {k}; r.Batch = BitConverter.DoubleToInt64Bits(rb{slot}[k]); r.Stream = BitConverter.DoubleToInt64Bits(fb{slot}[k]); }}"
            );
        }
    }
    s.push_str("                        if (cmp) r.FillBars++;\n");
    s.push_str("                        if (!cmp) { r.Ok = false; r.Leg = 2; r.Bar = beg + k; break; }\n");
    s.push_str("                    }\n                }\n");
    s.push_str("            }\n            catch (Exception) { r.Ok = false; r.Leg = 2; }\n");
    s.push_str("        }\n\n");

    s.push_str("        if (r.Ok)\n        {\n");
    s.push_str("            rideSeenUsed[slot] = true; rideSeenHash[slot] = hash;\n");
    s.push_str("            rideSeenOpen[slot] = r.OpenBars; rideSeenFill[slot] = r.FillBars;\n");
    s.push_str("        }\n");
    s.push_str("    }\n\n");
    s
}
