//! Java `stream_verify`: per streamable function, run the Java batch and the
//! Java stream trajectory in-process on identical seeded inputs (FuzzData port
//! of fuzz_data.h), compare BITWISE per bar (doubleToRawLongBits), spot-assert
//! peek == update AND value() == update, verify OpenAndFill against the batch
//! arrays plus the aliasing rejection, drive a mid-stream copy() independence
//! leg, check the Integer.MIN_VALUE default sentinel, and answer the same flat
//! JSON contract the ta_regtest driver reads. Differences from C/Rust, by
//! design:
//!
//! - Open rejects are exceptions: ONLY IllegalArgumentException (and its
//!   InsufficientHistoryException subclass) counts as an expected reject — an
//!   NPE/AIOOBE escapes and fails the run loudly instead of masquerading as
//!   reject-parity.
//! - An out-of-list enum request value is unrepresentable in the type-safe
//!   Java surface (MAType.values()[x] would throw); the sv_ answers the
//!   batch-fail reject-parity shape directly — type safety IS the rejection.
//! - Settings sweeps configure a FRESH per-round Core instance (per-instance
//!   settings; streams snapshot candle settings at open).

use super::{
    collect_pin_ids, sv_guard_enum_ints, sv_input_suffix, sv_range_bit, sv_reject_condition,
    SvRangeSite, SV_RANGE_MASK_JAVA,
};
use crate::ir::{EnumDef, FuncDef};
use std::collections::HashMap;

/// The per-input expanded fuzz array variable in the generated Java handler
/// (same mapping as the C/Rust servers: price components by name, generic
/// real0 -> close, real1 -> volume).
fn sv_java_input_array(name: &str, generic_idx: &mut usize) -> &'static str {
    match sv_input_suffix(name, generic_idx) {
        "o" => "fz_o",
        "h" => "fz_h",
        "l" => "fz_l",
        "c" => "fz_c",
        "v" => "fz_v",
        _ => "fz_oi",
    }
}

/// One `sv_<NAME>` verify method for a function with an emitted Java stream.
// Integer optional-param defaults are integer-valued `f64` in the IR; the
// `as i64` casts for literal emission are exact, not truncating.
#[allow(clippy::too_many_lines, clippy::cast_possible_truncation, clippy::cognitive_complexity)]
fn emit_java_sv_func(func: &FuncDef, funcs: &[FuncDef], enums: &HashMap<String, EnumDef>) -> String {
    use std::fmt::Write as _;
    let base = crate::backends::common::camel_words(&func.name);
    let base_camel = crate::backends::common::camel_words(&func.name);
    let class = crate::backends::java_stream::stream_class_name(func);
    let candle = func.name.starts_with("CDL");
    let inputs = crate::streaming::input_array_names(func);
    let mut gi = 0usize;
    let arrays: Vec<&'static str> = inputs
        .iter()
        .map(|i| sv_java_input_array(i, &mut gi))
        .collect();
    let n_out = func.outputs.len();
    let multi = n_out > 1;
    let out_is_int: Vec<bool> = func
        .outputs
        .iter()
        .map(|o| o.param_type == crate::ir::ParamType::Integer)
        .collect();
    // A public FIELD read, not an accessor call: since #310 a multi-output
    // stream writes a caller-owned `<N>Out` rather than answering a record.
    // Rendered once here because all ten read sites interpolate this same list.
    let vfield: Vec<String> = func
        .outputs
        .iter()
        .map(|o| crate::backends::java_stream::value_field_name(&o.name))
        .collect();
    let ocls = crate::backends::java_stream::out_class_name(func);

    let mut s = String::new();
    let _ = writeln!(s, "    static String sv_{}(String json) {{", func.name);
    s.push_str("        int svShape = jsonInt(json, \"gen_shape\");\n");
    s.push_str("        int svSeed = jsonInt(json, \"gen_seed\");\n");
    s.push_str("        int svN = jsonInt(json, \"gen_n\");\n");
    s.push_str("        if (svN < 2) svN = 2;\n        if (svN > 256) svN = 256;\n");
    s.push_str("        int svK = jsonInt(json, \"unstablePeriod\");\n");
    if candle {
        s.push_str("        int candleLegs = jsonInt(json, \"candleLegs\");\n");
    }
    // Optional params with YAML defaults; enum params parse as raw ints first
    // (out-of-list detection), everything else straight to its Java type.
    let mut has_int_default = false;
    for p in &func.optional_inputs {
        let name = &p.name;
        match &p.param_type {
            crate::ir::ParamType::Real => {
                let d = p.default.unwrap_or(0.0);
                let _ = writeln!(
                    s,
                    "        double {name} = json.contains(\"\\\"{name}\\\"\") ? jsonDouble(json, \"{name}\") : {d:e};"
                );
            }
            crate::ir::ParamType::Enum(en) => {
                let d = p.default.unwrap_or(0.0) as i64;
                let _ = writeln!(
                    s,
                    "        int _raw_{name} = json.contains(\"\\\"{name}\\\"\") ? jsonInt(json, \"{name}\") : {d};"
                );
                let _ = writeln!(
                    s,
                    "        if (_raw_{name} < 0 || _raw_{name} >= {en}.values().length) {{"
                );
                s.push_str("            /* Out-of-list enum: unrepresentable in the type-safe Java surface —\n             * batch and stream both reject at the type level (reject parity). */\n");
                s.push_str("            return \"{\\\"retCode\\\":2,\\\"legs\\\":0,\\\"nb\\\":0,\\\"openRejects\\\":1,\\\"ok\\\":1,\\\"peek_ok\\\":1}\";\n");
                s.push_str("        }\n");
                let _ = writeln!(s, "        {en} {name} = {en}.values()[_raw_{name}];");
            }
            _ => {
                let d = p.default.unwrap_or(0.0) as i64;
                if p.default.is_some() {
                    has_int_default = true;
                }
                let _ = writeln!(
                    s,
                    "        int {name} = json.contains(\"\\\"{name}\\\"\") ? jsonInt(json, \"{name}\") : {d};"
                );
            }
        }
    }
    // Seeded inputs.
    s.push_str("        double[] fz_o = new double[svN];\n        double[] fz_h = new double[svN];\n        double[] fz_l = new double[svN];\n        double[] fz_c = new double[svN];\n        double[] fz_v = new double[svN];\n        double[] fz_oi = new double[svN];\n");
    s.push_str("        FuzzData.fuzzGen(svShape, svSeed, svN, fz_o, fz_h, fz_l, fz_c, fz_v, fz_oi);\n");

    // Period-bank ramp (see the C/Rust emitters): span [min-1, max+1] so every
    // bank slot and both clamp directions are exercised.
    {
        let lookup = crate::streaming::FuncsLookup(funcs);
        if let Ok(crate::streaming::StreamPlan::PeriodBank(pb)) =
            crate::streaming::validate_streamable(func, &lookup)
        {
            if let Some(idx) = inputs.iter().position(|i| *i == pb.period_input) {
                let arr = arrays[idx];
                let _ = writeln!(
                    s,
                    "        for (int _pi = 0; _pi < svN; _pi++) {{ {arr}[_pi] = {min} + (_pi % ({max} - {min} + 3)) - 1; }}",
                    min = pb.min_param,
                    max = pb.max_param
                );
            }
        }
    }

    let full_ins = arrays.join(", ");
    let pfx_ins = |p: &str| -> String {
        arrays
            .iter()
            .map(|a| format!("java.util.Arrays.copyOf({a}, {p})"))
            .collect::<Vec<_>>()
            .join(", ")
    };
    let opts = func
        .optional_inputs
        .iter()
        .map(|p| p.name.clone())
        .collect::<Vec<_>>()
        .join(", ");
    let opts_lead = if opts.is_empty() { String::new() } else { format!("{opts}, ") };
    let opts_tail = if opts.is_empty() { String::new() } else { format!(", {opts}") };
    let bar_args = |t: &str| -> String {
        arrays
            .iter()
            .map(|a| format!("{a}[{t}]"))
            .collect::<Vec<_>>()
            .join(", ")
    };
    let bars_t = bar_args("t");

    // Batch + fill output buffers.
    let mut bdecls = String::new();
    let mut bargs = String::new();
    let mut fdecls = String::new();
    let mut fargs = String::new();
    for (i, is_int) in out_is_int.iter().enumerate() {
        let ty = if *is_int { "int" } else { "double" };
        let _ = writeln!(bdecls, "        {ty}[] b{i} = new {ty}[svN];");
        let _ = write!(bargs, ", b{i}");
        // Canary-filled, not zero-filled: the slack above the produced range is
        // asserted untouched after the call (#205's write bound), so a write
        // past `nb` fails instead of landing in unread space.
        let canary = if *is_int { "-987654321" } else { "-1.2345678901234e300" };
        let _ = writeln!(fdecls, "            {ty}[] f{i} = new {ty}[svN];");
        let _ = writeln!(fdecls, "            java.util.Arrays.fill(f{i}, ({ty}){canary});");
        let _ = write!(fargs, ", f{i}");
    }
    s.push_str(&bdecls);

    s.push_str("        long legs = 0;\n        boolean allOk = true;\n        boolean peekAll = true;\n        long peekReps = 0;\n        long peekRejects = 0;\n        boolean peekRepAll = true;\n        int fillChecked = 0;\n        boolean fillOk = true;\n        MInteger beg = new MInteger();\n        MInteger nb = new MInteger();\n        String diag = \"\";\n");
    // The range leg (#241): a handle's outRange() against what batch reported
    // for the same bars. Public API in every backend, so unlike the state leg
    // this one is not C-only.
    s.push_str("        int rangeChecked = 0;\n        boolean rangeOk = true;\n        long rangeLegs = 0;\n        int rangeSites = 0;\n");
    // Benign +/-0 cases across every cross-tier compare in this request. A
    // one-element array, not a static: the server answers many requests per
    // process and a static would carry one function's count into the next.
    s.push_str("        long[] zsign = { 0 };\n");
    if candle {
        s.push_str("        int rounds = (candleLegs != 0) ? 4 : 1;\n");
    } else {
        s.push_str("        int rounds = 1;\n");
    }
    s.push_str("        for (int rd = 0; rd < rounds; rd++) {\n");

    // Fresh, pinned, configured Core for this round (per-instance settings).
    s.push_str("            Core c2 = new Core();\n");
    for id in collect_pin_ids(func, funcs, enums) {
        let _ = writeln!(s, "            c2.unstablePeriod[{id}] = svK;");
    }
    if candle {
        s.push_str("            svApplyCandleRound(c2, rd);\n");
    }

    // Expected-reject precheck (dispatch/period-bank arms without a stream) —
    // live again since #139: hma has no stream yet, so every MAType-dispatching
    // function (MA, BBANDS, APO/PPO/PVO, STOCH*, MACDEXT, MAVP) generates a
    // guard for the HMA arm. The guard compares raw enum ints, so substitute
    // C constants with values.
    if let Some(guard) = sv_reject_condition(func, funcs, None) {
        // Rewrite enum param names to their raw-int locals for the guard, and
        // C enum constants to their integer values (Java has no TA_MAType_*).
        let mut guard_java = sv_guard_enum_ints(&guard, enums);
        for p in &func.optional_inputs {
            if matches!(p.param_type, crate::ir::ParamType::Enum(_)) {
                guard_java = guard_java.replace(&p.name, &format!("_raw_{}", p.name));
            }
        }
        let _ = writeln!(s, "            if ({guard_java}) {{");
        s.push_str("                boolean r1;\n");
        let _ = writeln!(
            s,
            "                try {{ c2.{base_camel}Open({full_ins}{opts_tail}); r1 = false; }} catch (IllegalArgumentException _e) {{ r1 = true; }}"
        );
        s.push_str(&fdecls.replace("            ", "                "));
        s.push_str("                boolean r2;\n");
        let _ = writeln!(
            s,
            "                try {{ c2.{base_camel}OpenAndFill({full_ins}{opts_tail}{fargs}); r2 = false; }} catch (IllegalArgumentException _e) {{ r2 = true; }}"
        );
        s.push_str("                boolean okr = r1 && r2;\n");
        s.push_str("                return \"{\\\"retCode\\\":0,\\\"legs\\\":0,\\\"unsupportedArm\\\":1,\\\"ok\\\":\" + (okr ? 1 : 0) + \",\\\"peek_ok\\\":1}\";\n");
        s.push_str("            }\n");
    }

    // Batch leg.
    let _ = writeln!(
        s,
        "            RetCode rc;\n            try {{ rc = c2.{base}Impl(0, svN - 1, {full_ins}, {opts_lead}beg, nb{bargs}); }}\n            catch (RuntimeException _sve) {{ if (!(_sve instanceof TALibFailure)) throw _sve; rc = ((TALibFailure) _sve).retCode(); beg.value = 0; nb.value = 0; }}"
    );
    let _ = writeln!(s, "            int lb = c2.{base}Lookback({opts});");
    s.push_str("            if (rc != RetCode.SUCCESS || nb.value == 0) {\n");
    s.push_str("                boolean openRejects;\n");
    let _ = writeln!(
        s,
        "                try {{ c2.{base_camel}Open({full_ins}{opts_tail}); openRejects = false; }} catch (IllegalArgumentException _e) {{ openRejects = true; }}"
    );
    if candle {
        s.push_str("                if (!openRejects) allOk = false;\n");
        s.push_str("                if (rd + 1 < rounds) continue;\n");
        s.push_str("                return \"{\\\"retCode\\\":\" + rc.toInt() + \",\\\"legs\\\":\" + legs + \",\\\"nb\\\":\" + nb.value + \",\\\"openRejects\\\":\" + (openRejects ? 1 : 0) + \",\\\"ok\\\":\" + (allOk ? 1 : 0) + \",\\\"peek_ok\\\":\" + (peekAll ? 1 : 0) + \",\\\"peek_reps\\\":\" + peekReps + \",\\\"peek_rep_ok\\\":\" + (peekRepAll ? 1 : 0) + \",\\\"peek_rejects\\\":\" + peekRejects + \",\\\"benign\\\":\" + zsign[0] + \"}\";\n");
    } else {
        s.push_str("                return \"{\\\"retCode\\\":\" + rc.toInt() + \",\\\"legs\\\":0,\\\"nb\\\":\" + nb.value + \",\\\"openRejects\\\":\" + (openRejects ? 1 : 0) + \",\\\"ok\\\":\" + (openRejects ? 1 : 0) + \",\\\"peek_ok\\\":1}\";\n");
    }
    s.push_str("            }\n");

    // OpenAndFill leg (fill == batch arrays, bitwise) + aliasing probes.
    s.push_str("            fillChecked = 1;\n            try {\n");
    s.push_str(&fdecls.replace("            ", "                "));
    let _ = writeln!(
        s,
        "                Core.{class} _fh = c2.{base_camel}OpenAndFill({full_ins}{opts_tail}{fargs});"
    );
    s.push_str("                OutRange _fr = _fh.outRange();\n");
    let _ = writeln!(s, "                rangeChecked = 1; rangeLegs++; rangeSites |= {};", sv_range_bit(SvRangeSite::Fill, SV_RANGE_MASK_JAVA));
    s.push_str("                if (_fr.begIdx() != beg.value || _fr.count() != nb.value) rangeOk = false;\n");
    s.push_str("                if (_fr.begIdx() != beg.value || _fr.count() != nb.value) fillOk = false;\n                else {\n");
    for (i, is_int) in out_is_int.iter().enumerate() {
        if *is_int {
            let _ = writeln!(s, "                    for (int i = 0; i < nb.value; i++) if (f{i}[i] != b{i}[i]) fillOk = false;");
        } else {
            let _ = writeln!(s, "                    for (int i = 0; i < nb.value; i++) if (svXtierNe(f{i}[i], b{i}[i], zsign)) fillOk = false;");
        }
    }
    // Slack canary: everything above the produced range must be untouched.
    for (i, is_int) in out_is_int.iter().enumerate() {
        let ty = if *is_int { "int" } else { "double" };
        let canary = if *is_int { "-987654321" } else { "-1.2345678901234e300" };
        let _ = writeln!(s, "                    for (int i = nb.value; i < svN; i++) if (f{i}[i] != ({ty}){canary}) fillOk = false;");
    }
    s.push_str("                }\n");
    // Aliasing probes (Java arrays make out==in expressible; the guards must
    // reject with IAE and mint no handle).
    if !out_is_int[0] {
        let alias_args = {
            let mut fa = String::new();
            for (i, is_int) in out_is_int.iter().enumerate() {
                if i == 0 {
                    let _ = write!(fa, ", {}", arrays[0]);
                } else {
                    let _ = write!(fa, ", f{i}");
                }
                let _ = is_int;
            }
            fa
        };
        let _ = writeln!(
            s,
            "                try {{ c2.{base_camel}OpenAndFill({full_ins}{opts_tail}{alias_args}); fillOk = false; }} catch (IllegalArgumentException _e) {{ /* expected: output aliases input */ }}"
        );
        if multi && !out_is_int[1] {
            let alias_args2 = {
                let mut fa = String::new();
                for (i, _) in out_is_int.iter().enumerate() {
                    if i == 1 {
                        let _ = write!(fa, ", f0");
                    } else {
                        let _ = write!(fa, ", f{i}");
                    }
                }
                fa
            };
            let _ = writeln!(
                s,
                "                try {{ c2.{base_camel}OpenAndFill({full_ins}{opts_tail}{alias_args2}); fillOk = false; }} catch (IllegalArgumentException _e) {{ /* expected: output aliases output */ }}"
            );
        }
    }
    s.push_str("            } catch (IllegalArgumentException _e) { fillOk = false; }\n");

    // Prefix sweep.
    s.push_str("            int[] pcs = { lb + 1, lb + 13, svN / 2, svN - 1 };\n");
    s.push_str("            java.util.Arrays.sort(pcs);\n");
    s.push_str("            int prevP = -1;\n");
    s.push_str("            for (int pi = 0; pi < pcs.length; pi++) {\n");
    s.push_str("                int p = pcs[pi];\n");
    s.push_str("                if (p < lb + 1 || p > svN - 1 || p == prevP) continue;\n");
    s.push_str("                prevP = p;\n");
    let _ = writeln!(s, "                Core.{class} st;");
    let _ = writeln!(
        s,
        "                try {{ st = c2.{base_camel}Open({}{opts_tail}); }}",
        pfx_ins("p")
    );
    s.push_str("                catch (IllegalArgumentException _e) { allOk = false; if (diag.isEmpty()) diag = \",\\\"openRejectP\\\":\" + p; continue; }\n");
    s.push_str("                legs++;\n");
    // Open-value compare through value() (load-bearing: Java open returns only
    // the handle, so the anchor compare IS the value() verification).
    if multi {
        let _ = writeln!(s, "                Core.{ocls} v0 = new Core.{ocls}(); st.value(v0);");
        for (i, f) in vfield.iter().enumerate() {
            if out_is_int[i] {
                let _ = writeln!(s, "                if (v0.{f} != b{i}[p - 1 - beg.value]) {{ allOk = false; if (diag.isEmpty()) diag = \",\\\"badBar\\\":\" + (p - 1) + \",\\\"badOut\\\":{i},\\\"where\\\":\\\"open\\\"\"; }}");
            } else {
                let _ = writeln!(s, "                if (svXtierNe(v0.{f}, b{i}[p - 1 - beg.value], zsign)) {{ allOk = false; if (diag.isEmpty()) diag = \",\\\"badBar\\\":\" + (p - 1) + \",\\\"badOut\\\":{i},\\\"where\\\":\\\"open\\\"\"; }}");
            }
        }
    } else if out_is_int[0] {
        s.push_str("                if (st.value() != b0[p - 1 - beg.value]) { allOk = false; if (diag.isEmpty()) diag = \",\\\"badBar\\\":\" + (p - 1) + \",\\\"badOut\\\":0,\\\"where\\\":\\\"open\\\"\"; }\n");
    } else {
        s.push_str("                if (svXtierNe(st.value(), b0[p - 1 - beg.value], zsign)) { allOk = false; if (diag.isEmpty()) diag = \",\\\"badBar\\\":\" + (p - 1) + \",\\\"badOut\\\":0,\\\"where\\\":\\\"open\\\"\"; }\n");
    }
    // Update loop: peek every bar, `value()`==update, and the repeat probe every
    // seventh. The multi-output sinks are allocated ONCE and reused, which is
    // the usage the `<N>Out` javadoc tells callers to write; `pk`, `up`, `vc`
    // and `rp` stay four DISTINCT objects, or the compares below would read one
    // buffer against itself.
    if multi {
        let _ = writeln!(s, "                Core.{ocls} pk = new Core.{ocls}();");
        let _ = writeln!(s, "                Core.{ocls} up = new Core.{ocls}();");
        let _ = writeln!(s, "                Core.{ocls} vc = new Core.{ocls}();");
        let _ = writeln!(s, "                Core.{ocls} rp = new Core.{ocls}();");
    }
    s.push_str("                for (int t = p; t < svN; t++) {\n");
    let up_ty = if multi {
        String::new()
    } else if out_is_int[0] {
        "int ".to_string()
    } else {
        "double ".to_string()
    };
    // A peek may refuse the bar -- see `emit_sv_peek_repeat_probe`. Refusals are
    // counted, and every comparison that would read the untouched sink is
    // guarded, because a refused peek writes nothing.
    s.push_str("                    boolean pkTook = true;\n");
    if multi {
        let _ = writeln!(s, "                    try {{ st.peek({bars_t}, pk); }} catch (IllegalArgumentException _e) {{ pkTook = false; peekRejects++; }}");
    } else {
        let _ = writeln!(s, "                    {up_ty}pk = 0;");
        let _ = writeln!(s, "                    try {{ pk = st.peek({bars_t}); }} catch (IllegalArgumentException _e) {{ pkTook = false; peekRejects++; }}");
    }
    // The repeat probe — see `emit_sv_peek_repeat_probe` for why the decoy in
    // the middle is what makes it see anything.
    s.push_str("                    if (t % 7 == 0) {\n");
    s.push_str("                        boolean rpTook = pkTook;\n");
    if multi {
        let _ = writeln!(s, "                        try {{ st.peek({}, rp); }} catch (IllegalArgumentException _e) {{ peekRejects++; }}", bar_args("t - 1"));
        let _ = writeln!(s, "                        try {{ st.peek({bars_t}, rp); }} catch (IllegalArgumentException _e) {{ rpTook = false; }}");
    } else {
        let _ = writeln!(s, "                        try {{ st.peek({}); }} catch (IllegalArgumentException _e) {{ peekRejects++; }}", bar_args("t - 1"));
        let _ = writeln!(s, "                        {up_ty}rp = 0;");
        let _ = writeln!(s, "                        try {{ rp = st.peek({bars_t}); }} catch (IllegalArgumentException _e) {{ rpTook = false; }}");
    }
    s.push_str("                        if (rpTook) {\n");
    s.push_str("                            peekReps++;\n");
    if multi {
        for (i, f) in vfield.iter().enumerate() {
            if out_is_int[i] {
                let _ = writeln!(s, "                            if (rp.{f} != pk.{f}) peekRepAll = false;");
            } else {
                let _ = writeln!(s, "                            if (svBne(rp.{f}, pk.{f})) peekRepAll = false;");
            }
        }
    } else if out_is_int[0] {
        s.push_str("                            if (rp != pk) peekRepAll = false;\n");
    } else {
        s.push_str("                            if (svBne(rp, pk)) peekRepAll = false;\n");
    }
    s.push_str("                        } else { peekRejects++; }\n");
    s.push_str("                    }\n");
    if multi {
        let _ = writeln!(s, "                    st.update({bars_t}, up);");
    } else {
        let _ = writeln!(s, "                    {up_ty}up = st.update({bars_t});");
    }
    if multi {
        for (i, f) in vfield.iter().enumerate() {
            if out_is_int[i] {
                let _ = writeln!(s, "                    if (pkTook && pk.{f} != up.{f}) peekAll = false;");
            } else {
                let _ = writeln!(s, "                    if (pkTook && svBne(pk.{f}, up.{f})) peekAll = false;");
            }
        }
        // `value()` == what `update` just wrote, read AFTER an intervening
        // `peek`. The peek is the whole leg: without it, `update`'s sink write
        // and `value`'s are the SAME generated statements over the same
        // `cur_*` fields with nothing in between, so the compare cannot fail.
        // Peeking a DIFFERENT bar (t-1, always in range since t >= lb+1 >= 1)
        // makes it the documented contract instead: `value` is a pure read that
        // `peek` does not disturb, and a peek that commits moves the handle so
        // `value` reports the peeked bar. Same shape as the C# leg, which found
        // the tautology first. `pk` is reused here -- its own compare is done.
        let _ = writeln!(s, "                    try {{ st.peek({}, pk); }} catch (IllegalArgumentException _e) {{ peekRejects++; }}", bar_args("t - 1"));
        s.push_str("                    st.value(vc);\n");
        for (i, f) in vfield.iter().enumerate() {
            if out_is_int[i] {
                let _ = writeln!(s, "                    if (vc.{f} != up.{f}) allOk = false;");
            } else {
                let _ = writeln!(s, "                    if (svBne(vc.{f}, up.{f})) allOk = false;");
            }
        }
    } else {
        // The same intervening peek the multi arm gets, and for the same
        // reason: without it `update`'s return and `value`'s read render from
        // one expression over one field with nothing between, and the compare
        // cannot fail. C# emits it for every arity; this arm did not.
        let _ = writeln!(s, "                    if (pkTook && {}) peekAll = false;",
            if out_is_int[0] { "pk != up" } else { "svBne(pk, up)" });
        let _ = writeln!(s, "                    try {{ st.peek({}); }} catch (IllegalArgumentException _e) {{ peekRejects++; }}", bar_args("t - 1"));
        let _ = writeln!(s, "                    if ({}) allOk = false;",
            if out_is_int[0] { "st.value() != up" } else { "svBne(st.value(), up)" });
    }
    let emit_up_compares = |s: &mut String, pad: &str| {
        if multi {
            for (i, f) in vfield.iter().enumerate() {
                if out_is_int[i] {
                    let _ = writeln!(s, "{pad}if (up.{f} != b{i}[t - beg.value]) {{ allOk = false; if (diag.isEmpty()) diag = \",\\\"badBar\\\":\" + t + \",\\\"badOut\\\":{i},\\\"batchv\\\":\\\"\" + b{i}[t - beg.value] + \"\\\",\\\"streamv\\\":\\\"\" + up.{f} + \"\\\"\"; }}");
                } else {
                    let _ = writeln!(s, "{pad}if (svXtierNe(up.{f}, b{i}[t - beg.value], zsign)) {{ allOk = false; if (diag.isEmpty()) diag = \",\\\"badBar\\\":\" + t + \",\\\"badOut\\\":{i},\\\"batchv\\\":\\\"\" + String.format(\"%016x\", Double.doubleToRawLongBits(b{i}[t - beg.value])) + \"\\\",\\\"streamv\\\":\\\"\" + String.format(\"%016x\", Double.doubleToRawLongBits(up.{f})) + \"\\\"\"; }}");
                }
            }
        } else if out_is_int[0] {
            let _ = writeln!(s, "{pad}if (up != b0[t - beg.value]) {{ allOk = false; if (diag.isEmpty()) diag = \",\\\"badBar\\\":\" + t + \",\\\"badOut\\\":0,\\\"batchv\\\":\\\"\" + b0[t - beg.value] + \"\\\",\\\"streamv\\\":\\\"\" + up + \"\\\"\"; }}");
        } else {
            let _ = writeln!(s, "{pad}if (svXtierNe(up, b0[t - beg.value], zsign)) {{ allOk = false; if (diag.isEmpty()) diag = \",\\\"badBar\\\":\" + t + \",\\\"badOut\\\":0,\\\"batchv\\\":\\\"\" + String.format(\"%016x\", Double.doubleToRawLongBits(b0[t - beg.value])) + \"\\\",\\\"streamv\\\":\\\"\" + String.format(\"%016x\", Double.doubleToRawLongBits(up)) + \"\\\"\"; }}");
        }
    };
    emit_up_compares(&mut s, "                    ");
    s.push_str("                }\n");
    // Open(p) + (svN - p) updates: whatever p was, the handle has consumed svN
    // bars and must report exactly what batch(0, svN-1) did. Only when the value
    // leg passed — otherwise the handle is short of the bars it was to consume.
    s.push_str("                if (allOk) {\n");
    let _ = writeln!(s, "                    rangeChecked = 1; rangeLegs++; rangeSites |= {};", sv_range_bit(SvRangeSite::Prefix, SV_RANGE_MASK_JAVA));
    s.push_str("                    if (st.outRange().begIdx() != beg.value || st.outRange().count() != nb.value) rangeOk = false;\n");
    // One advance(), last on this handle -- see the C server for why.
    let _ = writeln!(s, "                    rangeLegs++; rangeSites |= {};", sv_range_bit(SvRangeSite::Advance, SV_RANGE_MASK_JAVA));
    s.push_str("                    st.advance();\n");
    s.push_str("                    if (st.outRange().begIdx() != beg.value || st.outRange().count() != nb.value + 1) rangeOk = false;\n");
    s.push_str("                }\n");
    s.push_str("            }\n");

    // copy() independence leg: open at the earliest prefix, advance to mid,
    // copy, drive both to the end — both must match batch bitwise (a shallow
    // sub-handle/bank/ring copy diverges here), and both must report the batch
    // range (#287: a copy that carries every numeric field but drops the range
    // pair produced identical values and was invisible here).
    s.push_str("            {\n");
    s.push_str("                int p0 = lb + 1;\n");
    s.push_str("                if (p0 <= svN - 1) {\n");
    s.push_str("                    try {\n");
    let _ = writeln!(
        s,
        "                        Core.{class} sA = c2.{base_camel}Open({}{opts_tail});",
        pfx_ins("p0")
    );
    s.push_str("                        int mid = (p0 + svN) / 2;\n");
    if multi {
        let _ = writeln!(s, "                        Core.{ocls} uA = new Core.{ocls}();");
        let _ = writeln!(s, "                        Core.{ocls} uB = new Core.{ocls}();");
        let _ = writeln!(s, "                        for (int t = p0; t < mid; t++) sA.update({bars_t}, uA);");
    } else {
        let _ = writeln!(s, "                        for (int t = p0; t < mid; t++) sA.update({bars_t});");
    }
    let _ = writeln!(s, "                        Core.{class} sB = sA.clone();");
    s.push_str("                        for (int t = mid; t < svN; t++) {\n");
    if multi {
        let _ = writeln!(s, "                            sA.update({bars_t}, uA);");
        let _ = writeln!(s, "                            sB.update({bars_t}, uB);");
    } else {
        let _ = writeln!(s, "                            {up_ty}uA = sA.update({bars_t});");
        let _ = writeln!(s, "                            {up_ty}uB = sB.update({bars_t});");
    }
    if multi {
        for (i, f) in vfield.iter().enumerate() {
            if out_is_int[i] {
                let _ = writeln!(s, "                            if (uA.{f} != uB.{f} || uA.{f} != b{i}[t - beg.value]) {{ allOk = false; if (diag.isEmpty()) diag = \",\\\"copyDiverged\\\":\" + t; }}");
            } else {
                let _ = writeln!(s, "                            if (svBne(uA.{f}, uB.{f}) || svXtierNe(uA.{f}, b{i}[t - beg.value], zsign)) {{ allOk = false; if (diag.isEmpty()) diag = \",\\\"copyDiverged\\\":\" + t; }}");
            }
        }
    } else if out_is_int[0] {
        s.push_str("                            if (uA != uB || uA != b0[t - beg.value]) { allOk = false; if (diag.isEmpty()) diag = \",\\\"copyDiverged\\\":\" + t; }\n");
    } else {
        s.push_str("                            if (svBne(uA, uB) || svXtierNe(uA, b0[t - beg.value], zsign)) { allOk = false; if (diag.isEmpty()) diag = \",\\\"copyDiverged\\\":\" + t; }\n");
    }
    s.push_str("                        }\n");
    // Both handles have now consumed bars [p0-1, svN-1] — the fork's own
    // updates carried it over exactly the bars the original took — so each
    // must report what batch(0, svN-1) did, the same claim the prefix leg
    // makes about the handle it never copied. The original is the control: it
    // is the prefix leg's shape, so a failure on sA alone says the leg's own
    // bookkeeping broke rather than copy().
    // Only when the value leg passed: a diverged handle is not one whose range
    // is worth reading.
    s.push_str("                        if (allOk) {\n");
    let _ = writeln!(
        s,
        "                            rangeChecked = 1; rangeLegs++; rangeSites |= {};",
        sv_range_bit(SvRangeSite::Copy, SV_RANGE_MASK_JAVA)
    );
    s.push_str("                            if (sA.outRange().begIdx() != beg.value || sA.outRange().count() != nb.value) { rangeOk = false; if (diag.isEmpty()) diag = \",\\\"copyRangeSrc\\\":1\"; }\n");
    s.push_str("                            if (sB.outRange().begIdx() != beg.value || sB.outRange().count() != nb.value) { rangeOk = false; if (diag.isEmpty()) diag = \",\\\"copyRange\\\":1\"; }\n");
    s.push_str("                        }\n");
    s.push_str("                    } catch (IllegalArgumentException _e) { allOk = false; if (diag.isEmpty()) diag = \",\\\"copyOpenReject\\\":1\"; }\n");
    s.push_str("                }\n");
    s.push_str("            }\n");

    // Short-history reject leg: at `lb` bars no output is defined for ANY
    // configuration, so open must reject (with the typed exception).
    s.push_str("            if (lb >= 1 && lb < svN) {\n");
    let _ = writeln!(
        s,
        "                try {{ c2.{base_camel}Open({}{opts_tail}); allOk = false; if (diag.isEmpty()) diag = \",\\\"shortHistoryAccepted\\\":1\"; }}",
        pfx_ins("lb")
    );
    s.push_str("                catch (InsufficientHistoryException _e) { /* expected, typed */ }\n");
    s.push_str("                catch (IllegalArgumentException _e) { allOk = false; if (diag.isEmpty()) diag = \",\\\"shortHistoryWrongType\\\":1\"; }\n");
    s.push_str("            }\n");

    // Integer.MIN_VALUE default-sentinel leg: open(MIN_VALUE) must equal
    // open(explicit YAML default) bitwise (the batch guard transcribes into
    // the stream open, so defaulting can never silently diverge).
    if has_int_default {
        let mut sent_args: Vec<String> = Vec::new();
        let mut expl_args: Vec<String> = Vec::new();
        for p in &func.optional_inputs {
            match &p.param_type {
                crate::ir::ParamType::Integer if p.default.is_some() => {
                    let d = p.default.unwrap_or(0.0) as i64;
                    sent_args.push("Integer.MIN_VALUE".to_string());
                    expl_args.push(format!("{d}"));
                }
                _ => {
                    sent_args.push(p.name.clone());
                    expl_args.push(p.name.clone());
                }
            }
        }
        s.push_str("            try {\n");
        let _ = writeln!(
            s,
            "                Core.{class} sD = c2.{base_camel}Open({full_ins}, {});",
            sent_args.join(", ")
        );
        let _ = writeln!(
            s,
            "                Core.{class} sE = c2.{base_camel}Open({full_ins}, {});",
            expl_args.join(", ")
        );
        if multi {
            let _ = writeln!(s, "                Core.{ocls} vD = new Core.{ocls}(); sD.value(vD);");
            let _ = writeln!(s, "                Core.{ocls} vE = new Core.{ocls}(); sE.value(vE);");
            for (i, f) in vfield.iter().enumerate() {
                if out_is_int[i] {
                    let _ = writeln!(s, "                if (vD.{f} != vE.{f}) {{ allOk = false; if (diag.isEmpty()) diag = \",\\\"minValueDefault\\\":1\"; }}");
                } else {
                    let _ = writeln!(s, "                if (svBne(vD.{f}, vE.{f})) {{ allOk = false; if (diag.isEmpty()) diag = \",\\\"minValueDefault\\\":1\"; }}");
                }
            }
        } else if out_is_int[0] {
            s.push_str("                if (sD.value() != sE.value()) { allOk = false; if (diag.isEmpty()) diag = \",\\\"minValueDefault\\\":1\"; }\n");
        } else {
            s.push_str("                if (svBne(sD.value(), sE.value())) { allOk = false; if (diag.isEmpty()) diag = \",\\\"minValueDefault\\\":1\"; }\n");
        }
        s.push_str("            } catch (IllegalArgumentException _e) { /* defaults need more history than svN — skip */ }\n");
    }

    // startIdx-anchored range site (#241). `_OpenInternal` is the composition
    // seam, and its range is max(startIdx, lookback) — a DIFFERENT expression
    // from the two sites above, resolved by a different emitter branch. It was
    // gated in C alone, which is how a clamp landed in the three managed
    // backends without its history re-check and shipped: nothing here executed
    // it. Same shape as the C leg, against a reference recomputed for the
    // anchored range under this request's own settings.
    s.push_str("            {\n");
    let anchored_bit = sv_range_bit(SvRangeSite::Anchored, SV_RANGE_MASK_JAVA);
    s.push_str("                int Sidx = lb + (svN - lb) / 3;\n");
    s.push_str("                if (Sidx > lb && Sidx < svN - 1) {\n");
    s.push_str("                    MInteger begS = new MInteger();\n");
    s.push_str("                    MInteger nbS = new MInteger();\n");
    s.push_str("                    RetCode rcS;\n");
    let _ = writeln!(
        s,
        "                    try {{ rcS = c2.{base}Impl(Sidx, svN - 1, {full_ins}, {opts_lead}begS, nbS{bargs}); }}\n\
         \x20                   catch (RuntimeException _sve) {{ if (!(_sve instanceof TALibFailure)) throw _sve; rcS = ((TALibFailure) _sve).retCode(); }}"
    );
    s.push_str("                    if (rcS == RetCode.SUCCESS && nbS.value > 0) {\n");
    let _ = writeln!(
        s,
        "                        try {{\n\
         \x20                           Core.{class} stA = c2.{base_camel}OpenInternal({}, Sidx{opts_tail});\n\
         \x20                           rangeChecked = 1; rangeLegs++; rangeSites |= {anchored_bit};\n\
         \x20                           if (stA.outRange().begIdx() != begS.value || stA.outRange().count() != nbS.value) rangeOk = false;\n\
         \x20                       }} catch (IllegalArgumentException _e) {{ rangeOk = false; if (diag.isEmpty()) diag = \",\\\"anchoredOpenRejected\\\":1\"; }}",
        pfx_ins("svN")
    );
    s.push_str("                    }\n");
    s.push_str("                }\n");

    s.push_str("            }\n");

    s.push_str("        }\n");
    // fill_ok folds into ok as a safety net (mirrors the C/Rust gates).

    s.push_str("        return \"{\\\"retCode\\\":0,\\\"beg\\\":\" + beg.value + \",\\\"nb\\\":\" + nb.value + \",\\\"legs\\\":\" + legs + \",\\\"fill_checked\\\":\" + fillChecked + \",\\\"fill_ok\\\":\" + (fillOk ? 1 : 0) + \",\\\"range_checked\\\":\" + rangeChecked + \",\\\"range_legs\\\":\" + rangeLegs + \",\\\"range_sites\\\":\" + rangeSites + \",\\\"range_sites_all\\\":"); s.push_str(&SV_RANGE_MASK_JAVA.to_string()); s.push_str(",\\\"range_ok\\\":\" + (rangeOk ? 1 : 0) + \",\\\"step_ok\\\":\" + (allOk ? 1 : 0) + \",\\\"ok\\\":\" + ((allOk && fillOk && rangeOk) ? 1 : 0) + \",\\\"peek_ok\\\":\" + (peekAll ? 1 : 0) + \",\\\"peek_reps\\\":\" + peekReps + \",\\\"peek_rep_ok\\\":\" + (peekRepAll ? 1 : 0) + \",\\\"peek_rejects\\\":\" + peekRejects + \",\\\"benign\\\":\" + zsign[0] + diag + \"}\";\n");
    s.push_str("    }\n\n");
    s
}

/// The whole Java `stream_verify` section: bit-compare + candle-round helpers,
/// one `sv_<NAME>` per function with an emitted Java stream, and the
/// dispatcher (unknown names — including TA_STREAM_PROBE — answer
/// "not_streamable", the driver's capability probe contract).
pub(crate) fn generate_java_stream_verify(
    funcs: &[FuncDef],
    enums: &HashMap<String, EnumDef>,
) -> String {
    use std::fmt::Write as _;
    let mut s = String::new();
    s.push_str("    // ---- stream_verify: Java stream vs Java batch, bitwise ----\n\n");
    s.push_str("    static boolean svBne(double a, double b) {\n        return Double.doubleToRawLongBits(a) != Double.doubleToRawLongBits(b);\n    }\n\n");
    // Cross-tier compare — see the C emitter for the rule. Differing bits that
    // compare equal are +0.0 vs -0.0 (issue #147): counted, never a mismatch.
    // peek/value()/copy-vs-copy stay on svBne — one code path, no licence to differ.
    s.push_str("    static boolean svXtierNe(double a, double b, long[] zsign) {\n");
    s.push_str("        if (!svBne(a, b)) return false;\n");
    s.push_str("        if (a == b) { zsign[0]++; return false; }\n");
    s.push_str("        return true;\n");
    s.push_str("    }\n\n");
    // Candle-settings rounds (mirror the C/Rust sweep): defaults / avgPeriod+3
    // / avgPeriod=0 (instant candle) / rangeType=Shadows.
    // REPLACES each slot rather than mutating the CandleSetting in it. `new Core()`
    // shallow-clones DEFAULT_CANDLE_SETTINGS, so every Core's slot i is the SAME
    // object as the default's slot i; writing `cs.avgPeriod += 3` there would edit
    // the defaults themselves, and round 1 of the first candlestick would leave
    // every later Core -- and every later round, and restore_candle_default_settings
    // -- reading +3. CandleSetting's fields are final so that spelling does not
    // compile (#215).
    s.push_str("    static void svApplyCandleRound(Core c, int rd) {\n");
    s.push_str("        if (rd == 0) return;\n");
    s.push_str("        for (int i = 0; i < c.candleSettings.length; i++) {\n");
    s.push_str("            CandleSetting cs = c.candleSettings[i];\n");
    s.push_str("            if (rd == 1) {\n");
    s.push_str("                c.candleSettings[i] =\n");
    s.push_str("                    new CandleSetting(cs.rangeType, cs.avgPeriod + 3, cs.factor);\n");
    s.push_str("            } else if (rd == 2) {\n");
    s.push_str("                c.candleSettings[i] = new CandleSetting(cs.rangeType, 0, cs.factor);\n");
    s.push_str("            } else if (rd == 3) {\n");
    s.push_str("                c.candleSettings[i] =\n");
    s.push_str("                    new CandleSetting(RangeType.SHADOWS, cs.avgPeriod, cs.factor);\n");
    s.push_str("            }\n");
    s.push_str("        }\n");
    s.push_str("    }\n\n");

    let lookup = crate::streaming::FuncsLookup(funcs);
    let emitted: Vec<&FuncDef> = funcs
        .iter()
        .filter(|f| crate::backends::java_stream::emits_stream(f, &lookup))
        .collect();
    for f in &emitted {
        s.push_str(&emit_java_sv_func(f, funcs, enums));
    }

    // fuzz_in_hash — the same input-port self-check the Rust server answers
    // (issue #113): proves the FuzzData port reproduces C's fuzz_gen bytes.
    // The stream pass probes it so the port can never silently rot.
    s.push_str("    static String handle_fuzz_in_hash(String json) {\n");
    s.push_str("        int shape = jsonInt(json, \"gen_shape\");\n");
    s.push_str("        int seed = jsonInt(json, \"gen_seed\");\n");
    s.push_str("        int n = jsonInt(json, \"gen_n\");\n");
    s.push_str("        if (n < 1) n = 1;\n");
    s.push_str("        if (n > MAX_ARRAY_SIZE) n = MAX_ARRAY_SIZE;\n");
    s.push_str("        double[] fo = new double[n]; double[] fh = new double[n]; double[] fl = new double[n];\n");
    s.push_str("        double[] fc = new double[n]; double[] fv = new double[n]; double[] foi = new double[n];\n");
    s.push_str("        FuzzData.fuzzGen(shape, seed, n, fo, fh, fl, fc, fv, foi);\n");
    s.push_str("        long hh = svHashInit();\n");
    s.push_str("        hh = svHashF64(hh, fo, n);\n");
    s.push_str("        hh = svHashF64(hh, fh, n);\n");
    s.push_str("        hh = svHashF64(hh, fl, n);\n");
    s.push_str("        hh = svHashF64(hh, fc, n);\n");
    s.push_str("        hh = svHashF64(hh, fv, n);\n");
    s.push_str("        hh = svHashF64(hh, foi, n);\n");
    s.push_str("        hh = svHashFin(hh);\n");
    s.push_str("        return \"{\\\"in_hash\\\":\\\"\" + String.format(\"%016x\", hh) + \"\\\"}\";\n");
    s.push_str("    }\n\n");

    s.push_str("    static String handle_stream_verify(String json) {\n");
    s.push_str("        String fn = jsonString(json, \"funcName\");\n");
    s.push_str("        switch (fn) {\n");
    for f in &emitted {
        let _ = writeln!(
            s,
            "        case \"TA_{}\": return sv_{}(json);",
            f.name.to_uppercase(),
            f.name
        );
    }
    s.push_str("        default: return \"{\\\"error\\\":\\\"not_streamable\\\"}\";\n");
    s.push_str("        }\n");
    s.push_str("    }\n\n");
    s
}
