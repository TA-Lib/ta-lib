//! Rust `stream_verify`: per streamable function, run the Rust batch and the
//! Rust stream trajectory in-process on identical seeded inputs, compare
//! BITWISE per bar (to_bits), spot-assert peek == update, verify the
//! OpenAndFill fill against the batch arrays, and answer the same flat JSON
//! contract the ta_regtest driver reads (ok / peek_ok / legs / fill_checked /
//! fill_ok / benign / unsupportedArm / "not_streamable"). Differences from the
//! C gate, by design:
//!
//! - No startIdx-anchored OpenInternal leg: `<f>_open_internal` is pub(crate)
//!   (the anchor seam is a bit-exactness footgun, not a public API); anchored
//!   opens are exercised transitively through composed functions' own legs.
//! - No aliasing probes: `&mut` exclusivity makes #108 unexpressible in the
//!   safe public API (the driver never reads the probes directly).
//! - Settings sweeps rebuild an immutable Core via the builder instead of
//!   mutating globals.

use super::{
    collect_pin_ids, sv_guard_enum_ints, sv_range_bit, sv_reject_condition, SvRangeSite,
    SV_RANGE_MASK_RUST,
};
use crate::ir::{EnumDef, FuncDef};
use std::collections::HashMap;
use std::fmt::Write as _;

/// The per-input expanded fuzz array variable in the generated Rust handler.
fn sv_rust_input_array(name: &str, generic_idx: &mut usize) -> &'static str {
    match name {
        "inOpen" => "fz_o",
        "inHigh" => "fz_h",
        "inLow" => "fz_l",
        "inClose" => "fz_c",
        "inVolume" => "fz_v",
        "inOpenInterest" => "fz_oi",
        _ => {
            let arr = if *generic_idx == 0 { "fz_c" } else { "fz_v" };
            *generic_idx += 1;
            arr
        }
    }
}

/// Assert the slack above the produced range still holds the canary. The Rust
/// fill buffers are allocated at exactly `svN`, so the whole allocation beyond
/// `nb` is covered by this window (unlike C's fixed-width `static` buffers,
/// which must be walked to `SV_MAXN`).
fn rust_canary_check(out_is_int: &[bool]) -> String {
    let mut s = String::new();
    for (i, is_int) in out_is_int.iter().enumerate() {
        let canary = if *is_int { "-987654321i32" } else { "-1.2345678901234e300f64" };
        let _ = writeln!(s, "                    for i in nb..svN {{ if f{i}[i] != {canary} {{ fill_ok = false; }} }}");
    }
    s
}

/// One `sv_<name>` verify function for a function with an emitted Rust stream.
#[allow(clippy::too_many_lines)]
fn emit_rust_sv_func(func: &FuncDef, funcs: &[FuncDef], enums: &HashMap<String, EnumDef>) -> String {
    use std::fmt::Write as _;
    // The server-local verify fn stays snake_case (`sv_sma`); library calls use
    // the verbatim function name.
    let sn = func.name.to_lowercase();
    let fname = &func.name;
    let fname_snake = crate::backends::common::snake_words(fname);
    let candle = func.name.starts_with("CDL");
    let inputs = crate::streaming::input_array_names(func);
    let mut gi = 0usize;
    let arrays: Vec<&'static str> = inputs
        .iter()
        .map(|i| sv_rust_input_array(i, &mut gi))
        .collect();
    let out_is_int: Vec<bool> = func
        .outputs
        .iter()
        .map(|o| o.param_type == crate::ir::ParamType::Integer)
        .collect();

    let mut s = String::new();
    let _ = writeln!(s, "fn sv_{sn}(core: &Core, params: &Value) -> String {{");
    s.push_str("    let svShape = params[\"gen_shape\"].as_i64().unwrap_or(0) as i32;\n");
    s.push_str("    let svSeed = params[\"gen_seed\"].as_i64().unwrap_or(0) as i32;\n");
    s.push_str("    let mut svN = params[\"gen_n\"].as_i64().unwrap_or(0) as usize;\n");
    s.push_str("    if svN < 2 { svN = 2; }\n    if svN > 256 { svN = 256; }\n");
    // u32 to match the builder's setter. Refused rather than clamped: silently
    // reading a negative warm-up as 0 would make this arm pass while measuring
    // something the C side never asked for.
    s.push_str("    let svK = match u32::try_from(params[\"unstablePeriod\"].as_i64().unwrap_or(0)) {\n");
    s.push_str("        Ok(v) => v,\n");
    s.push_str("        Err(_) => return \"{\\\"error\\\":\\\"negative unstablePeriod\\\"}\".to_string(),\n");
    s.push_str("    };\n");
    if candle {
        s.push_str("    let candleLegs = params[\"candleLegs\"].as_i64().unwrap_or(0);\n");
    }
    // Optional params with YAML defaults (the driver always sends them, but the
    // defaults keep hand-driven requests working).
    for p in &func.optional_inputs {
        let name = &p.name;
        if p.param_type == crate::ir::ParamType::Real {
            let d = p.default.unwrap_or(0.0);
            let _ = writeln!(
                s,
                "    let {name} = params[\"{name}\"].as_f64().unwrap_or({d:?});"
            );
        } else {
            #[allow(clippy::cast_possible_truncation)]
            let d = p.default.unwrap_or(0.0) as i64;
            if let crate::ir::ParamType::Enum(enum_name) = &p.param_type {
                // The driver DOES send an out-of-list value here on purpose
                // (`maxList + 91`, test_codegen.c): batch rejects it at the
                // dispatch default arm, so the stream must reject too. A typed
                // enum cannot carry it, so reject at the type level and report
                // it — the same shape, and the same reason, as the Java server.
                let _ = writeln!(
                    s,
                    "    let {name}_raw = params[\"{name}\"].as_i64().unwrap_or({d}) as i32;"
                );
                let _ = writeln!(s, "    let {name} = match {enum_name}::try_from({name}_raw) {{");
                s.push_str("        Ok(v) => v,
");
                s.push_str("        Err(_) => return \"{\\\"retCode\\\":2,\\\"legs\\\":0,\\\"nb\\\":0,\\\"openRejects\\\":1,\\\"ok\\\":1,\\\"peek_ok\\\":1}\".to_string(),
");
                s.push_str("    };
");
            } else {
                let _ = writeln!(
                    s,
                    "    let {name} = params[\"{name}\"].as_i64().unwrap_or({d}) as i32;"
                );
            }
        }
    }
    // Seeded inputs.
    s.push_str("    let mut fz_o = vec![0.0f64; svN];\n    let mut fz_h = vec![0.0f64; svN];\n    let mut fz_l = vec![0.0f64; svN];\n    let mut fz_c = vec![0.0f64; svN];\n    let mut fz_v = vec![0.0f64; svN];\n    let mut fz_oi = vec![0.0f64; svN];\n");
    s.push_str("    fuzz_gen(svShape, svSeed, svN as i32, &mut fz_o, &mut fz_h, &mut fz_l, &mut fz_c, &mut fz_v, &mut fz_oi);\n");

    // Period-bank ramp: the fuzz period-selector series would clamp to
    // maxPeriod at every bar (vacuous slots) — overwrite with a ramp spanning
    // [min-1, max+1] fed identically to batch and stream.
    {
        let lookup = crate::streaming::FuncsLookup(funcs);
        if let Ok(crate::streaming::StreamPlan::PeriodBank(pb)) =
            crate::streaming::validate_streamable(func, &lookup)
        {
            if let Some(idx) = inputs.iter().position(|i| *i == pb.period_input) {
                let arr = arrays[idx];
                let _ = writeln!(
                    s,
                    "    for _pi in 0..svN {{ {arr}[_pi] = ({min} + ((_pi as i32) % ({max} - {min} + 3)) - 1) as f64; }}",
                    min = pb.min_param,
                    max = pb.max_param
                );
            }
        }
    }

    // Convenience strings for calls.
    let full_ins = arrays
        .iter()
        .map(|a| format!("&{a}"))
        .collect::<Vec<_>>()
        .join(", ");
    let pfx_ins = arrays
        .iter()
        .map(|a| format!("&{a}[..p]"))
        .collect::<Vec<_>>()
        .join(", ");
    let opts = func
        .optional_inputs
        .iter()
        .map(|p| p.name.clone())
        .collect::<Vec<_>>()
        .join(", ");
    let opts_lead = if opts.is_empty() { String::new() } else { format!("{opts}, ") };
    let opts_tail = if opts.is_empty() { String::new() } else { format!(", {opts}") };

    // Batch output buffers.
    let mut bdecls = String::new();
    let mut bargs = String::new();
    let mut fdecls = String::new();
    let mut fargs = String::new();
    for (i, is_int) in out_is_int.iter().enumerate() {
        let (ty, z) = if *is_int { ("i32", "0i32") } else { ("f64", "0.0f64") };
        // A nullable output takes `Option<&mut [T]>` (rule B6a); this harness
        // compares values, so it always supplies one.
        let some = func.outputs.get(i).is_some_and(crate::ir::Output::is_nullable);
        let (op, cl) = if some { ("Some(", ")") } else { ("", "") };
        let _ = writeln!(bdecls, "    let mut b{i}: Vec<{ty}> = vec![{z}; svN];");
        let _ = write!(bargs, ", {op}&mut b{i}{cl}");
        // Canary-filled, not zero-filled: the slack above the produced range is
        // asserted untouched after the call (#205's write bound), so a write
        // past `nb` fails instead of landing in unread space.
        let canary = if *is_int { "-987654321i32" } else { "-1.2345678901234e300f64" };
        let _ = writeln!(fdecls, "        let mut f{i}: Vec<{ty}> = vec![{canary}; svN];");
        let _ = write!(fargs, ", {op}&mut f{i}{cl}");
    }
    s.push_str(&bdecls);

    s.push_str("    let mut legs = 0i64;\n    let mut all_ok = true;\n    let mut peek_all = true;\n    let mut peek_reps = 0i64;\n    let mut peek_rejects = 0i64;\n    let mut peek_rep_all = true;\n    let mut fill_checked = 0i32;\n    let mut fill_ok = true;\n    let mut beg = 0usize;\n    let mut nb = 0usize;\n    let mut diag = String::new();\n");
    // The range leg (#241): a handle's OutRange against what batch reported for
    // the same bars. Public API in every backend, so unlike the state leg this
    // one is not C-only.
    s.push_str("    let mut range_checked = 0i32;\n    let mut range_ok = true;\n    let mut range_legs = 0i64;\n    let mut range_sites = 0i32;\n");
    s.push_str("    let mut value_checked = 0i32;\n    let mut value_ok = true;\n    let mut value_legs = 0i64;\n");
    // Benign +/-0 cases across every cross-tier compare in this request. `mut`
    // only when an output can reach sv_xtier_ne: an all-integer function (every
    // CDL*, MIN/MAX/MINMAXINDEX, HT_TRENDMODE) compares with `!=` and only ever
    // reads this, and rustc's unused_mut is not in the generated crate's allow
    // list — same reason cb_mut below is conditional.
    let z_mut = if out_is_int.iter().any(|b| !*b) { "mut " } else { "" };
    let _ = writeln!(s, "    let {z_mut}zsign = 0i64;");
    let rounds = if candle {
        "    let rounds = if candleLegs != 0 { 4 } else { 1 };\n"
    } else {
        "    let rounds = 1;\n"
    };
    s.push_str(rounds);
    s.push_str("    for rd in 0..rounds {\n        let _ = rd;\n");

    // Pinned + configured core for this round. `mut` only when something below
    // actually reassigns it, otherwise rustc warns on every such function.
    let pin_ids = collect_pin_ids(func, funcs, enums);
    let cb_mut = if pin_ids.is_empty() && !candle { "" } else { "mut " };
    let _ = writeln!(s, "        let {cb_mut}cb = core.to_builder();");
    for id in pin_ids {
        let _ = writeln!(
            s,
            "        if let Some(id) = func_unst_id_from_int({id}usize) {{ cb = cb.unstable_period(id, svK); }}"
        );
    }
    if candle {
        s.push_str("        cb = sv_apply_candles(cb, &sv_candle_settings(rd));\n");
    }
    // Reported, never unwrapped: this runs in a subprocess ta_regtest drives over
    // a pipe, so a panic here would surface as a dead pipe rather than a
    // diagnosable BadParam.
    s.push_str("        let c2 = match cb.build() {\n");
    s.push_str("            Ok(c) => c,\n");
    s.push_str(
        "            Err(_) => return \"{\\\"error\\\":\\\"unstablePeriod out of range\\\"}\".to_string(),\n",
    );
    s.push_str("        };\n");

    // Expected-reject precheck (dispatch / period-bank arms without a stream).
    if let Some(guard) = sv_reject_condition(func, funcs, None) {
        let guard = sv_guard_enum_ints(&guard, enums);
        let _ = writeln!(s, "        if {guard} {{");
        let _ = writeln!(
            s,
            "            let r1 = c2.{fname_snake}_open({full_ins}{opts_tail}).is_err();"
        );
        s.push_str(&fdecls.replace("        ", "            "));
        let _ = writeln!(
            s,
            "            let r2 = c2.{fname_snake}_open_and_fill({full_ins}{opts_tail}{fargs}).is_err();"
        );
        s.push_str("            let okr = r1 && r2;\n");
        s.push_str("            return format!(\"{{\\\"retCode\\\":0,\\\"legs\\\":0,\\\"unsupportedArm\\\":1,\\\"ok\\\":{},\\\"peek_ok\\\":1}}\", i32::from(okr));\n");
        s.push_str("        }\n");
    }

    // Batch leg.
    // The public tier returns the range; `beg`/`nb` stay as locals because the legs
    // below (OpenAndFill, Peek) compare against them.
    let bargs_head = bargs.trim_start_matches(", ");
    let _ = writeln!(
        s,
        "        let rc = match c2.{fname}(0, svN - 1, {full_ins}, {opts_lead}{bargs_head}) {{ Ok(r) => {{ beg = r.beg_idx; nb = r.count; RetCode::Success }} Err(e) => {{ beg = 0; nb = 0; e }} }};"
    );
    let _ = writeln!(s, "        let lb = c2.{fname}_Lookback({opts}).unwrap_or(usize::MAX);");
    s.push_str("        if rc != RetCode::Success || nb == 0 {\n");
    let _ = writeln!(
        s,
        "            let open_rejects = c2.{fname_snake}_open({full_ins}{opts_tail}).is_err();"
    );
    if candle {
        s.push_str("            if !open_rejects { all_ok = false; }\n");
        s.push_str("            if rd + 1 < rounds { continue; }\n");
        s.push_str("            return format!(\"{{\\\"retCode\\\":{},\\\"legs\\\":{},\\\"nb\\\":{},\\\"openRejects\\\":{},\\\"ok\\\":{},\\\"peek_ok\\\":{},\\\"peek_reps\\\":{},\\\"peek_rep_ok\\\":{},\\\"peek_rejects\\\":{},\\\"benign\\\":{}}}\", retcode_to_int(rc), legs, nb, i32::from(open_rejects), i32::from(all_ok), i32::from(peek_all), peek_reps, i32::from(peek_rep_all), peek_rejects, zsign);\n");
    } else {
        s.push_str("            return format!(\"{{\\\"retCode\\\":{},\\\"legs\\\":0,\\\"nb\\\":{},\\\"openRejects\\\":{},\\\"ok\\\":{},\\\"peek_ok\\\":1}}\", retcode_to_int(rc), nb, i32::from(open_rejects), i32::from(open_rejects));\n");
    }
    s.push_str("        }\n");

    // OpenAndFill leg (fill == batch arrays, bitwise).
    s.push_str("        fill_checked = 1;\n        {\n");
    s.push_str(&fdecls);
    let _ = writeln!(
        s,
        "        match c2.{fname_snake}_open_and_fill({full_ins}{opts_tail}{fargs}) {{"
    );
    s.push_str("            Err(_) => { fill_ok = false; }\n");
    let fill_bit = sv_range_bit(SvRangeSite::Fill, SV_RANGE_MASK_RUST);
    s.push_str("            Ok((_h, fr)) => {\n                range_checked = 1; range_legs += 1; range_sites |= ");
    s.push_str(&fill_bit.to_string());
    s.push_str(";\n                if _h.out_range().beg_idx != beg || _h.out_range().count != nb { range_ok = false; }\n                if fr.beg_idx != beg || fr.count != nb { fill_ok = false; }\n                else {\n");
    for (i, is_int) in out_is_int.iter().enumerate() {
        if *is_int {
            let _ = writeln!(s, "                    for i in 0..nb {{ if f{i}[i] != b{i}[i] {{ fill_ok = false; }} }}");
        } else {
            let _ = writeln!(s, "                    for i in 0..nb {{ if sv_xtier_ne(f{i}[i], b{i}[i], &mut zsign) {{ fill_ok = false; }} }}");
        }
    }
    s.push_str(&rust_canary_check(&out_is_int));
    s.push_str("                }\n            }\n        }\n        }\n");

    emit_rust_sv_prefix_sweep(&mut s, fname, &arrays, &pfx_ins, &opts_tail, &out_is_int);
    emit_rust_sv_clone_leg(&mut s, fname, &arrays, &pfx_ins, &opts_tail, &out_is_int);

    // Short-history reject leg: at `lb` bars no output is defined for ANY
    // configuration, so open must reject.
    s.push_str("        if lb >= 1 && lb < svN {\n");
    let short_ins = arrays
        .iter()
        .map(|a| format!("&{a}[..lb]"))
        .collect::<Vec<_>>()
        .join(", ");
    let _ = writeln!(
        s,
        "            if c2.{fname_snake}_open({short_ins}{opts_tail}).is_ok() {{ all_ok = false; if diag.is_empty() {{ diag = \",\\\"shortHistoryAccepted\\\":1\".to_string(); }} }}"
    );
    s.push_str("        }\n");

    s.push_str("    }\n");
    // fill_ok folds into ok as a safety net (mirrors the C gate), so a driver
    // reading only `ok` — e.g. the debug sweep — still fails on a fill regression.
    s.push_str("    format!(\"{{\\\"retCode\\\":0,\\\"beg\\\":{},\\\"nb\\\":{},\\\"legs\\\":{},\\\"fill_checked\\\":{},\\\"fill_ok\\\":{},\\\"range_checked\\\":{},\\\"range_legs\\\":{},\\\"range_sites\\\":{},\\\"range_sites_all\\\":"); s.push_str(&SV_RANGE_MASK_RUST.to_string()); s.push_str(",\\\"range_ok\\\":{},\\\"value_checked\\\":{},\\\"value_legs\\\":{},\\\"value_ok\\\":{},\\\"step_ok\\\":{},\\\"ok\\\":{},\\\"peek_ok\\\":{},\\\"peek_reps\\\":{},\\\"peek_rep_ok\\\":{},\\\"peek_rejects\\\":{},\\\"benign\\\":{}{}}}\", beg, nb, legs, fill_checked, i32::from(fill_ok), range_checked, range_legs, range_sites, i32::from(range_ok), value_checked, value_legs, i32::from(value_ok), i32::from(all_ok), i32::from(all_ok && fill_ok && range_ok && value_ok), i32::from(peek_all), peek_reps, i32::from(peek_rep_all), peek_rejects, zsign, diag)\n");
    s.push_str("}\n\n");
    s
}

/// The sweep over `pcs` (the earliest / mid-corpus / tail prefixes): open at
/// each, compare the open value, then walk `update`/`peek` to the end of the
/// corpus and compare every bar against the batch arrays.
fn emit_rust_sv_prefix_sweep(
    s: &mut String,
    fname: &str,
    arrays: &[&'static str],
    pfx_ins: &str,
    opts_tail: &str,
    out_is_int: &[bool],
) {
    let n_out = out_is_int.len();
    s.push_str("        let mut pcs = vec![lb + 1, lb + 13, svN / 2, svN - 1];\n");
    s.push_str("        pcs.retain(|p| *p >= lb + 1 && *p <= svN - 1);\n");
    s.push_str("        pcs.sort_unstable();\n        pcs.dedup();\n");
    s.push_str("        for &p in &pcs {\n");
    let fname_snake = crate::backends::common::snake_words(fname);
    let _ = writeln!(s, "            match c2.{fname_snake}_open({pfx_ins}{opts_tail}) {{");
    s.push_str("                Err(_) => { all_ok = false; if diag.is_empty() { diag = format!(\",\\\"openRejectP\\\":{}\", p); } }\n");
    s.push_str("                Ok((mut st, v0)) => {\n                    legs += 1;\n");
    // open-value compare
    let destructure = |var: &str| -> Vec<String> {
        if n_out == 1 {
            vec![var.to_string()]
        } else {
            (0..n_out).map(|i| format!("{var}.{i}")).collect()
        }
    };
    for (i, part) in destructure("v0").iter().enumerate() {
        if out_is_int[i] {
            let _ = writeln!(s, "                    if {part} != b{i}[p - 1 - beg] {{ all_ok = false; if diag.is_empty() {{ diag = format!(\",\\\"badBar\\\":{{}},\\\"badOut\\\":{i},\\\"where\\\":\\\"open\\\"\", p - 1); }} }}");
        } else {
            let _ = writeln!(s, "                    if sv_xtier_ne({part}, b{i}[p - 1 - beg], &mut zsign) {{ all_ok = false; if diag.is_empty() {{ diag = format!(\",\\\"badBar\\\":{{}},\\\"badOut\\\":{i},\\\"where\\\":\\\"open\\\"\", p - 1); }} }}");
        }
    }
    // update loop
    s.push_str("                    for t in p..svN {\n");
    let t_args = arrays.iter().map(|a| format!("{a}[t]")).collect::<Vec<_>>().join(", ");
    let d_args = arrays.iter().map(|a| format!("{a}[t - 1]")).collect::<Vec<_>>().join(", ");
    // `update` and `peek` are fallible: the streaming tier refuses a non-finite
    // bar. `update` is only ever handed a real bar of a finite corpus, so a
    // refusal there IS a defect and fails the leg. A `peek` refusal is not --
    // see `emit_sv_peek_repeat_probe` -- so it is counted and its comparisons
    // are skipped, leaving the bar's update and batch compare to run as usual.
    let _ = writeln!(s, "                        let pk_res = st.peek({t_args});");
    s.push_str("                        if pk_res.is_err() { peek_rejects += 1; }\n");
    let pk_parts = destructure("pk");
    let up_parts = destructure("up");
    // The repeat probe — see `emit_sv_peek_repeat_probe` for why the decoy in
    // the middle is what makes it see anything.
    s.push_str("                        if t % 7 == 0 {\n");
    let _ = writeln!(s, "                            if st.peek({d_args}).is_err() {{ peek_rejects += 1; }}");
    let _ = writeln!(s, "                            match (pk_res, st.peek({t_args})) {{");
    s.push_str("                                (Ok(pk), Ok(rp)) => {\n");
    s.push_str("                                    peek_reps += 1;\n");
    for (i, (pk, rp)) in pk_parts.iter().zip(destructure("rp").iter()).enumerate() {
        if out_is_int[i] {
            let _ = writeln!(s, "                                    if {rp} != {pk} {{ peek_rep_all = false; }}");
        } else {
            let _ = writeln!(s, "                                    if {rp}.to_bits() != {pk}.to_bits() {{ peek_rep_all = false; }}");
        }
    }
    s.push_str("                                }\n");
    s.push_str("                                _ => { peek_rejects += 1; }\n");
    s.push_str("                            }\n");
    s.push_str("                        }\n");
    let _ = writeln!(s, "                        let Ok(up) = st.update({t_args}) else {{ all_ok = false; if diag.is_empty() {{ diag = format!(\",\\\"updateRejected\\\":{{}}\", t); }} break; }};");
    s.push_str("                        if let Ok(pk) = pk_res {\n");
    for (i, (pk, up)) in pk_parts.iter().zip(up_parts.iter()).enumerate() {
        if out_is_int[i] {
            let _ = writeln!(s, "                            if {pk} != {up} {{ peek_all = false; }}");
        } else {
            let _ = writeln!(s, "                            if {pk}.to_bits() != {up}.to_bits() {{ peek_all = false; }}");
        }
    }
    s.push_str("                        }\n");
    for (i, up) in up_parts.iter().enumerate() {
        if out_is_int[i] {
            let _ = writeln!(s, "                        if {up} != b{i}[t - beg] {{ all_ok = false; if diag.is_empty() {{ diag = format!(\",\\\"badBar\\\":{{}},\\\"badOut\\\":{i},\\\"batchv\\\":\\\"{{}}\\\",\\\"streamv\\\":\\\"{{}}\\\"\", t, b{i}[t - beg], {up}); }} }}");
        } else {
            let _ = writeln!(s, "                        if sv_xtier_ne({up}, b{i}[t - beg], &mut zsign) {{ all_ok = false; if diag.is_empty() {{ diag = format!(\",\\\"badBar\\\":{{}},\\\"badOut\\\":{i},\\\"batchv\\\":\\\"{{:016x}}\\\",\\\"streamv\\\":\\\"{{:016x}}\\\"\", t, b{i}[t - beg].to_bits(), {up}.to_bits()); }} }}");
        }
    }
    s.push_str("                    }\n");
    // Open(p) + (svN - p) updates: whatever p was, the handle has consumed svN
    // bars and must report exactly what batch(0, svN-1) did.
    // Only when the value leg passed: its update loop breaks on a rejected bar,
    // which leaves the handle short of the bars it was supposed to consume.
    s.push_str("                    if all_ok {\n");
    s.push_str("                        range_checked = 1; range_legs += 1; range_sites |= ");
    s.push_str(&sv_range_bit(SvRangeSite::Prefix, SV_RANGE_MASK_RUST).to_string());
    s.push_str(";\n");
    s.push_str("                        if st.out_range().beg_idx != beg || st.out_range().count != nb { range_ok = false; }\n");
    // One `advance`, last on this handle -- see the C server for why.
    s.push_str("                        range_legs += 1; range_sites |= ");
    s.push_str(&sv_range_bit(SvRangeSite::Advance, SV_RANGE_MASK_RUST).to_string());
    s.push_str(";\n");
    s.push_str("                        if st.advance().is_err() { range_ok = false; }\n");
    s.push_str("                        if st.out_range().beg_idx != beg || st.out_range().count != nb + 1 { range_ok = false; }\n");
    s.push_str("                    }\n");
    s.push_str("                }\n            }\n        }\n");
}

/// Clone-independence leg (#287), the counterpart of Java's `copy()` leg and
/// C#'s `Clone()` one: open at the earliest prefix, advance to mid, fork, drive
/// both handles to the end.
///
/// Rust's fork is `#[derive(Clone)]` on the handle rather than a hand-emitted
/// method, and nothing in the harness exercised it — so a refactor that put a
/// buffer behind an `Rc`/`Arc` would turn `.clone()` into an aliased shallow
/// copy with no gate to see it. That is what the value half asserts: the two
/// handles agree with each other bitwise and each agrees with batch.
///
/// The range half is honest about what it is worth TODAY: `out` is a plain
/// `Copy` field of the derived struct, so it comes across by construction and
/// the compare cannot currently fail on its own. It is here because the site
/// has to exist in Rust for the ratchet to demand it — and because the day the
/// handle grows a hand-written `Clone`, or `out` stops being a stored pair,
/// this is the leg that already asks the question.
fn emit_rust_sv_clone_leg(
    s: &mut String,
    fname: &str,
    arrays: &[&'static str],
    pfx_ins: &str,
    opts_tail: &str,
    out_is_int: &[bool],
) {
    let n_out = out_is_int.len();
    let destructure = |var: &str| -> Vec<String> {
        if n_out == 1 {
            vec![var.to_string()]
        } else {
            (0..n_out).map(|i| format!("{var}.{i}")).collect()
        }
    };
    let t_args = arrays.iter().map(|a| format!("{a}[t]")).collect::<Vec<_>>().join(", ");
    let fname_snake = crate::backends::common::snake_words(fname);
    s.push_str("        if let Some(&p) = pcs.first() {\n");
    let _ = writeln!(s, "            match c2.{fname_snake}_open({pfx_ins}{opts_tail}) {{");
    s.push_str("                Err(_) => { all_ok = false; if diag.is_empty() { diag = \",\\\"copyOpenReject\\\":1\".to_string(); } }\n");
    s.push_str("                Ok((mut sa, _v0)) => {\n");
    // The opener seeds `value()`; nothing else in the Rust harness reads it.
    {
        let v_open = destructure("_v0");
        let a_open = destructure("va");
        s.push_str("                    { let va = sa.value(); value_checked = 1; value_legs += 1;\n");
        for (i, is_int) in out_is_int.iter().enumerate() {
            let cmp = if *is_int {
                format!("{} != {}", a_open[i], v_open[i])
            } else {
                format!("{}.to_bits() != {}.to_bits()", a_open[i], v_open[i])
            };
            let _ = writeln!(s, "                      if {cmp} {{ value_ok = false; if diag.is_empty() {{ diag = \",\\\"valueAfterOpen\\\":1\".to_string(); }} }}");
        }
        s.push_str("                    }\n");
    }
    s.push_str("                    let mid = (p + svN) / 2;\n");
    s.push_str("                    let mut forked = true;\n");
    let _ = writeln!(s, "                    for t in p..mid {{ if sa.update({t_args}).is_err() {{ all_ok = false; forked = false; if diag.is_empty() {{ diag = format!(\",\\\"copyPreRejected\\\":{{}}\", t); }} break; }} }}");
    s.push_str("                    let mut sb = sa.clone();\n");
    s.push_str("                    if forked {\n");
    s.push_str("                    for t in mid..svN {\n");
    let _ = writeln!(s, "                        let Ok(u_src) = sa.update({t_args}) else {{ all_ok = false; forked = false; if diag.is_empty() {{ diag = format!(\",\\\"copyRejected\\\":{{}}\", t); }} break; }};");
    let _ = writeln!(s, "                        let Ok(u_fork) = sb.update({t_args}) else {{ all_ok = false; forked = false; if diag.is_empty() {{ diag = format!(\",\\\"copyRejected\\\":{{}}\", t); }} break; }};");
    let src_parts = destructure("u_src");
    let fork_parts = destructure("u_fork");
    for (i, (u_src, u_fork)) in src_parts.iter().zip(fork_parts.iter()).enumerate() {
        // Same-tier: the fork is the SAME computation, so it is a strict bit
        // compare — the +/-0 tolerance the cross-tier compare carries has no
        // business between two handles that ran the same arithmetic.
        if out_is_int[i] {
            let _ = writeln!(s, "                        if {u_src} != {u_fork} {{ all_ok = false; if diag.is_empty() {{ diag = format!(\",\\\"copyDiverged\\\":{{}}\", t); }} }}");
            let _ = writeln!(s, "                        if {u_src} != b{i}[t - beg] {{ all_ok = false; if diag.is_empty() {{ diag = format!(\",\\\"copyDiverged\\\":{{}}\", t); }} }}");
        } else {
            let _ = writeln!(s, "                        if {u_src}.to_bits() != {u_fork}.to_bits() {{ all_ok = false; if diag.is_empty() {{ diag = format!(\",\\\"copyDiverged\\\":{{}}\", t); }} }}");
            let _ = writeln!(s, "                        if sv_xtier_ne({u_src}, b{i}[t - beg], &mut zsign) {{ all_ok = false; if diag.is_empty() {{ diag = format!(\",\\\"copyDiverged\\\":{{}}\", t); }} }}");
        }
    }
    // Every accepted update refreshes `value()`, on the fork as much as on the
    // original — the fork is the caller with no earlier call to have handed it one.
    {
        let src = destructure("u_src");
        let fork = destructure("u_fork");
        let va = destructure("va");
        let vb = destructure("vb");
        s.push_str("                        { let va = sa.value(); let vb = sb.value(); value_checked = 1; value_legs += 1;\n");
        for (i, is_int) in out_is_int.iter().enumerate() {
            let (ca, cb) = if *is_int {
                (format!("{} != {}", va[i], src[i]), format!("{} != {}", vb[i], fork[i]))
            } else {
                (format!("{}.to_bits() != {}.to_bits()", va[i], src[i]),
                 format!("{}.to_bits() != {}.to_bits()", vb[i], fork[i]))
            };
            let _ = writeln!(s, "                          if {ca} || {cb} {{ value_ok = false; if diag.is_empty() {{ diag = \",\\\"valueAfterUpdate\\\":1\".to_string(); }} }}");
        }
        s.push_str("                        }\n");
    }
    s.push_str("                    }\n");
    s.push_str("                    }\n");
    // Both handles have consumed bars [p-1, svN-1] — the fork's own updates
    // carried it over exactly the bars the original took — so each must report
    // what batch(0, svN-1) did. The original is the control: it is the prefix
    // leg's shape, so a failure on it alone says the leg's bookkeeping broke
    // rather than the fork. Only when the value leg passed, and only when the
    // fork actually ran every bar: a handle short of its bars has a range that
    // is legitimately not the batch one.
    s.push_str("                    if all_ok && forked {\n");
    s.push_str("                        range_checked = 1; range_legs += 1; range_sites |= ");
    s.push_str(&sv_range_bit(SvRangeSite::Copy, SV_RANGE_MASK_RUST).to_string());
    s.push_str(";\n");
    s.push_str("                        if sa.out_range().beg_idx != beg || sa.out_range().count != nb { range_ok = false; if diag.is_empty() { diag = \",\\\"copyRangeSrc\\\":1\".to_string(); } }\n");
    s.push_str("                        if sb.out_range().beg_idx != beg || sb.out_range().count != nb { range_ok = false; if diag.is_empty() { diag = \",\\\"copyRange\\\":1\".to_string(); } }\n");
    s.push_str("                    }\n");
    s.push_str("                }\n            }\n        }\n");
}

/// The whole Rust `stream_verify` section: candle-settings helpers, one
/// `sv_<name>` per function with an emitted Rust stream, and the dispatcher.
pub(crate) fn generate_rust_stream_verify(
    funcs: &[FuncDef],
    enums: &HashMap<String, EnumDef>,
) -> String {
    use std::fmt::Write as _;
    let mut s = String::new();
    s.push_str("// ---- stream_verify: Rust stream vs Rust batch, bitwise ----\n\n");
    // Cross-tier compare — see the C emitter for the rule. Differing bits that
    // compare equal are +0.0 vs -0.0 (issue #147): counted, never a mismatch.
    // The peek-vs-update spot-assert stays a strict `to_bits()` compare.
    s.push_str("fn sv_xtier_ne(a: f64, b: f64, zsign: &mut i64) -> bool {\n");
    s.push_str("    if a.to_bits() == b.to_bits() { return false; }\n");
    s.push_str("    if a == b { *zsign += 1; return false; }\n");
    s.push_str("    true\n");
    s.push_str("}\n\n");
    // Candle-settings rounds (mirror the C sweep): defaults / avgPeriod+3 /
    // avgPeriod=0 (instant candle) / rangeType=Shadows.
    s.push_str("fn sv_candle_settings(rd: i32) -> CandleSettings {\n    let mut s = CandleSettings::default_settings();\n    let all = |s: &mut CandleSettings, f: &dyn Fn(&mut CandleSetting)| {\n        for cs in [&mut s.body_long, &mut s.body_very_long, &mut s.body_short, &mut s.body_doji,\n                   &mut s.shadow_long, &mut s.shadow_very_long, &mut s.shadow_short,\n                   &mut s.shadow_very_short, &mut s.near, &mut s.far, &mut s.equal] {\n            f(cs);\n        }\n    };\n    match rd {\n        1 => all(&mut s, &|c| c.avg_period += 3),\n        2 => all(&mut s, &|c| c.avg_period = 0),\n        3 => all(&mut s, &|c| c.range_type = RangeType::Shadows),\n        _ => {}\n    }\n    s\n}\n\n");
    s.push_str("fn sv_apply_candles(b: CoreBuilder, s: &CandleSettings) -> CoreBuilder {\n    b.candle_setting(CandleSettingType::BodyLong, s.body_long)\n     .candle_setting(CandleSettingType::BodyVeryLong, s.body_very_long)\n     .candle_setting(CandleSettingType::BodyShort, s.body_short)\n     .candle_setting(CandleSettingType::BodyDoji, s.body_doji)\n     .candle_setting(CandleSettingType::ShadowLong, s.shadow_long)\n     .candle_setting(CandleSettingType::ShadowVeryLong, s.shadow_very_long)\n     .candle_setting(CandleSettingType::ShadowShort, s.shadow_short)\n     .candle_setting(CandleSettingType::ShadowVeryShort, s.shadow_very_short)\n     .candle_setting(CandleSettingType::Near, s.near)\n     .candle_setting(CandleSettingType::Far, s.far)\n     .candle_setting(CandleSettingType::Equal, s.equal)\n}\n\n");

    let lookup = crate::streaming::FuncsLookup(funcs);
    let emitted: Vec<&FuncDef> = funcs
        .iter()
        .filter(|f| crate::backends::rust_stream::emits_stream(f, &lookup))
        .collect();
    for f in &emitted {
        s.push_str(&emit_rust_sv_func(f, funcs, enums));
    }

    s.push_str("fn handle_stream_verify(core: &Core, params: &Value) -> String {\n");
    s.push_str("    let func_name = params[\"funcName\"].as_str().unwrap_or(\"\");\n");
    s.push_str("    match func_name {\n");
    for f in &emitted {
        let _ = writeln!(
            s,
            "        \"TA_{}\" => sv_{}(core, params),",
            f.name.to_uppercase(),
            f.name.to_lowercase()
        );
    }
    s.push_str("        _ => \"{\\\"error\\\":\\\"not_streamable\\\"}\".to_string(),\n    }\n}\n\n");
    s
}
