//! TypeScript `stream_verify`: the Java emitter's legs, recased. Where Java
//! catches `IllegalArgumentException`, TS reads the one `TALibError` class's
//! `retCode`: `BAD_PARAM` or `INSUFFICIENT_HISTORY` is a reject, and any other
//! throw escapes and fails the request loudly instead of reading as reject
//! parity. An out-of-list enum is representable here, as in C#, so it reaches
//! the batch tier's own rejection.

use super::{
    collect_pin_ids, sv_guard_enum_ints, sv_input_suffix, sv_range_bit, sv_reject_condition,
    SvRangeSite, SV_RANGE_MASK_TS,
};
use crate::backends::ts::ts_names;
use crate::ir::{EnumDef, FuncDef};
use std::collections::HashMap;

fn sv_ts_input_array(name: &str, generic_idx: &mut usize) -> &'static str {
    match sv_input_suffix(name, generic_idx) {
        "o" => "fz_o",
        "h" => "fz_h",
        "l" => "fz_l",
        "c" => "fz_c",
        "v" => "fz_v",
        _ => "fz_oi",
    }
}

#[allow(clippy::too_many_lines, clippy::cast_possible_truncation, clippy::cognitive_complexity)]
fn emit_ts_sv_func(func: &FuncDef, funcs: &[FuncDef], enums: &HashMap<String, EnumDef>) -> String {
    use std::fmt::Write as _;
    let names = ts_names(&func.name);
    let ns = &func.name;
    let impl_fn = format!("{ns}.{}", names.body);
    let api = crate::server_gen::TsPublic::new(func);
    let open_internal = format!("{ns}.{}", names.open_internal);
    let ocls = format!("{ns}.{}", names.out_class);
    let candle = func.is_candlestick();
    let inputs = crate::streaming::input_array_names(func);
    let mut gi = 0usize;
    let arrays: Vec<&'static str> = inputs.iter().map(|i| sv_ts_input_array(i, &mut gi)).collect();
    let n_out = func.outputs.len();
    let multi = n_out > 1;
    let out_is_int: Vec<bool> = func
        .outputs
        .iter()
        .map(|o| o.param_type == crate::ir::ParamType::Integer)
        .collect();
    let vfield: Vec<String> = func
        .outputs
        .iter()
        .map(|o| crate::backends::ts::output_key(&o.name))
        .collect();

    let mut s = String::new();
    let _ = writeln!(s, "function sv_{}(p: Params): string {{", func.name);
    s.push_str("  const svShape = int(p, 'gen_shape', 0);\n");
    s.push_str("  const svSeed = int(p, 'gen_seed', 0);\n");
    s.push_str("  let svN = int(p, 'gen_n', 0);\n");
    s.push_str("  if (svN < 2) svN = 2;\n  if (svN > 256) svN = 256;\n");
    let pin_ids = collect_pin_ids(func, funcs, enums);
    if !pin_ids.is_empty() {
        s.push_str("  const svK = int(p, 'unstablePeriod', 0);\n");
    }
    if candle {
        s.push_str("  const candleLegs = int(p, 'candleLegs', 0);\n");
    }
    let mut has_int_default = false;
    for prm in &func.optional_inputs {
        let name = &prm.name;
        match &prm.param_type {
            crate::ir::ParamType::Real => {
                let d = prm.default.unwrap_or(0.0);
                let _ = writeln!(s, "  const {name} = real(p, '{name}', {d:e});");
            }
            crate::ir::ParamType::Enum(en) => {
                let _ = writeln!(s, "  const _raw_{name} = int(p, '{name}', {en}.DEFAULT);");
                let _ = writeln!(s, "  const {name} = _raw_{name} as {en};");
            }
            _ => {
                let d = prm.default.unwrap_or(0.0) as i64;
                if prm.default.is_some() {
                    has_int_default = true;
                }
                let _ = writeln!(s, "  const {name} = int(p, '{name}', {d});");
            }
        }
    }
    s.push_str("  const fz_o = new Float64Array(svN);\n  const fz_h = new Float64Array(svN);\n  const fz_l = new Float64Array(svN);\n  const fz_c = new Float64Array(svN);\n  const fz_v = new Float64Array(svN);\n  const fz_oi = new Float64Array(svN);\n");
    s.push_str("  fuzzGen(svShape, svSeed, svN, fz_o, fz_h, fz_l, fz_c, fz_v, fz_oi);\n");

    {
        let lookup = crate::streaming::FuncsLookup(funcs);
        if let Ok(crate::streaming::StreamPlan::PeriodBank(pb)) =
            crate::streaming::validate_streamable(func, &lookup)
        {
            if let Some(idx) = inputs.iter().position(|i| *i == pb.period_input) {
                let arr = arrays[idx];
                let _ = writeln!(
                    s,
                    "  for (let _pi = 0; _pi < svN; _pi++) {{ {arr}[_pi] = {min} + (_pi % ({max} - {min} + 3)) - 1; }}",
                    min = pb.min_param,
                    max = pb.max_param
                );
            }
        }
    }

    let full_ins = arrays.join(", ");
    let opt_names: Vec<String> = func.optional_inputs.iter().map(|o| o.name.clone()).collect();
    let pfx_ins = |n: &str| -> Vec<String> { arrays.iter().map(|a| format!("{a}.slice(0, {n})")).collect() };
    let open = |ins: &[String]| api.open(ins, &opt_names, "c2");
    let open_fill = |ins: &[String], outs: &[String]| api.open_and_fill(ins, &opt_names, "c2", outs);
    let full: Vec<String> = arrays.iter().map(|a| (*a).to_string()).collect();
    let opts = opt_names.join(", ");
    let opts_lead = if opts.is_empty() { String::new() } else { format!("{opts}, ") };
    let opts_tail = if opts.is_empty() { String::new() } else { format!(", {opts}") };
    let bar_args = |t: &str| -> String {
        arrays.iter().map(|a| format!("{a}[{t}]")).collect::<Vec<_>>().join(", ")
    };
    let bars_t = bar_args("t");

    let mut bdecls = String::new();
    let mut bargs = String::new();
    let mut fdecls = String::new();
    let mut fouts: Vec<String> = Vec::new();
    for (i, is_int) in out_is_int.iter().enumerate() {
        let ty = if *is_int { "Int32Array" } else { "Float64Array" };
        let _ = writeln!(bdecls, "  const b{i} = new {ty}(svN);");
        let _ = write!(bargs, ", b{i}");
        // Canary-filled: the slack above the produced range is asserted untouched.
        let canary = if *is_int { "-987654321" } else { "-1.2345678901234e300" };
        let _ = writeln!(fdecls, "      const f{i} = new {ty}(svN).fill({canary});");
        fouts.push(format!("f{i}"));
    }
    s.push_str(&bdecls);

    s.push_str("  let legs = 0;\n  let roundMask = 0;\n  let allOk = true;\n  let peekAll = true;\n  let peekReps = 0;\n  let peekRejects = 0;\n  let peekRepAll = true;\n  let fillChecked = 0;\n  let fillOk = true;\n  const beg = new MInteger();\n  const nb = new MInteger();\n  let diag = '';\n");
    s.push_str("  let rangeChecked = 0;\n  let rangeOk = true;\n  let rangeLegs = 0;\n  let rangeSites = 0;\n");
    s.push_str("  const zsign = [0];\n");
    if candle {
        s.push_str("  const rounds = candleLegs !== 0 ? 5 : 1;\n");
    } else {
        s.push_str("  const rounds = 1;\n");
    }
    s.push_str("  for (let rd = 0; rd < rounds; rd++) {\n");

    // A fresh, pinned, configured Core per round: streams snapshot settings at open.
    let mut pins = String::new();
    for id in pin_ids {
        let _ = write!(pins, ".unstablePeriod({id}, svK)");
    }
    let base = if candle { "svCandleRound(rd)" } else { "Core.builder()" };
    if pins.is_empty() {
        let _ = writeln!(s, "    const c2 = {};", if candle { "svCandleRound(rd).build()" } else { "new Core()" });
    } else {
        // Reported, never thrown: an escaping exception reaches ta_regtest as a dead pipe.
        s.push_str("    let c2: Core;\n");
        let _ = writeln!(s, "    try {{ c2 = {base}{pins}.build(); }}");
        s.push_str("    catch (e) { if (!(e instanceof TALibError)) throw e; return '{\"error\":\"unstablePeriod out of range\"}'; }\n");
    }

    if let Some(guard) = sv_reject_condition(func, funcs, None) {
        let mut guard_ts = sv_guard_enum_ints(&guard, enums);
        for prm in &func.optional_inputs {
            if matches!(prm.param_type, crate::ir::ParamType::Enum(_)) {
                guard_ts = guard_ts.replace(&prm.name, &format!("_raw_{}", prm.name));
            }
        }
        let _ = writeln!(s, "    if ({guard_ts}) {{");
        s.push_str("      let r1: boolean;\n");
        let _ = writeln!(
            s,
            "      try {{ {}; r1 = false; }} catch (e) {{ r1 = svRejected(e); }}",
            open(&full)
        );
        s.push_str(&fdecls);
        s.push_str("      let r2: boolean;\n");
        let _ = writeln!(
            s,
            "      try {{ {}; r2 = false; }} catch (e) {{ r2 = svRejected(e); }}",
            open_fill(&full, &fouts)
        );
        s.push_str("      const okr = r1 && r2;\n");
        s.push_str("      return `{\"retCode\":0,\"legs\":0,\"unsupportedArm\":1,\"ok\":${okr ? 1 : 0},\"peek_ok\":1}`;\n");
        s.push_str("    }\n");
    }

    // Batch leg: the body, so the gate sees the code the public tier throws.
    let _ = writeln!(s, "    let rc: number;");
    let _ = writeln!(
        s,
        "    try {{ rc = {impl_fn}(c2, 0, svN - 1, {full_ins}, {opts_lead}beg, nb{bargs}); }}\n    catch (e) {{ if (!(e instanceof TALibError)) throw e; rc = e.retCode; beg.value = 0; nb.value = 0; }}"
    );
    let _ = writeln!(s, "    const lb = {};", api.lookback(&opt_names, "c2"));
    s.push_str("    if (rc !== RetCode.SUCCESS || nb.value === 0) {\n");
    s.push_str("      let openRejects: boolean;\n");
    let _ = writeln!(
        s,
        "      try {{ {}; openRejects = false; }} catch (e) {{ openRejects = svRejected(e); }}",
        open(&full)
    );
    if candle {
        s.push_str("      if (!openRejects) allOk = false;\n");
        s.push_str("      if (rd + 1 < rounds) continue;\n");
        s.push_str("      return `{\"retCode\":${rc},\"legs\":${legs},\"nb\":${nb.value},\"openRejects\":${openRejects ? 1 : 0},\"ok\":${allOk ? 1 : 0},\"peek_ok\":${peekAll ? 1 : 0},\"peek_reps\":${peekReps},\"peek_rep_ok\":${peekRepAll ? 1 : 0},\"peek_rejects\":${peekRejects},\"benign\":${zsign[0]},\"round_mask\":${roundMask}}`;\n");
    } else {
        s.push_str("      return `{\"retCode\":${rc},\"legs\":0,\"nb\":${nb.value},\"openRejects\":${openRejects ? 1 : 0},\"ok\":${openRejects ? 1 : 0},\"peek_ok\":1}`;\n");
    }
    s.push_str("    }\n");

    // OpenAndFill leg + aliasing probes.
    s.push_str("    fillChecked = 1;\n    try {\n");
    s.push_str(&fdecls);
    let _ = writeln!(s, "      const _fh = {};", open_fill(&full, &fouts));
    s.push_str("      const _fr = _fh.stream.outRange();\n");
    let _ = writeln!(s, "      rangeChecked = 1; rangeLegs++; rangeSites |= {};", sv_range_bit(SvRangeSite::Fill, SV_RANGE_MASK_TS));
    s.push_str("      if (_fr.begIdx !== beg.value || _fr.count !== nb.value) rangeOk = false;\n");
    let result_lens = api.result_keys().iter().map(|k| format!(" || _fh.{k}.length !== _fr.count")).collect::<Vec<_>>().concat();
    let _ = writeln!(s, "      if (_fh.begIdx !== _fr.begIdx{result_lens}) fillOk = false;");
    s.push_str("      if (_fr.begIdx !== beg.value || _fr.count !== nb.value) fillOk = false;\n      else {\n");
    for (i, is_int) in out_is_int.iter().enumerate() {
        if *is_int {
            let _ = writeln!(s, "        for (let i = 0; i < nb.value; i++) if (f{i}[i] !== b{i}[i]) fillOk = false;");
        } else {
            let _ = writeln!(s, "        for (let i = 0; i < nb.value; i++) if (svXtierNe(f{i}[i], b{i}[i], zsign)) fillOk = false;");
        }
    }
    for (i, is_int) in out_is_int.iter().enumerate() {
        let canary = if *is_int { "-987654321" } else { "-1.2345678901234e300" };
        let _ = writeln!(s, "        for (let i = nb.value; i < svN; i++) if (f{i}[i] !== {canary}) fillOk = false;");
    }
    s.push_str("      }\n");
    if !out_is_int[0] {
        let mut alias = fouts.clone();
        alias[0] = arrays[0].to_string();
        let _ = writeln!(s, "      try {{ {}; fillOk = false; }} catch (e) {{ svRejected(e); }}", open_fill(&full, &alias));
        if multi && !out_is_int[1] {
            let mut alias2 = fouts.clone();
            alias2[1] = "f0".to_string();
            let _ = writeln!(s, "      try {{ {}; fillOk = false; }} catch (e) {{ svRejected(e); }}", open_fill(&full, &alias2));
        }
    }
    s.push_str("    } catch (e) { svRejected(e); fillOk = false; }\n");

    // Prefix sweep.
    s.push_str("    const pcs = [lb + 1, lb + 13, Math.trunc(svN / 2), svN - 1].sort((x, y) => x - y);\n");
    s.push_str("    let prevP = -1;\n");
    s.push_str("    for (let pi = 0; pi < pcs.length; pi++) {\n");
    s.push_str("      const P = pcs[pi];\n");
    s.push_str("      if (P < lb + 1 || P > svN - 1 || P === prevP) continue;\n");
    s.push_str("      prevP = P;\n");
    let _ = writeln!(s, "      let st: {ns}.{};", names.stream_class);
    let _ = writeln!(s, "      try {{ st = {}; }}", open(&pfx_ins("P")));
    s.push_str("      catch (e) { svRejected(e); allOk = false; if (diag === '') diag = `,\"openRejectP\":${P}`; continue; }\n");
    s.push_str("      legs++; roundMask |= 1 << rd;\n");
    let open_diag = |i: usize| {
        format!("{{ allOk = false; if (diag === '') diag = `,\"badBar\":${{P - 1}},\"badOut\":{i},\"where\":\"open\"`; }}")
    };
    if multi {
        let _ = writeln!(s, "      const v0 = new {ocls}(); st.value(v0);");
        for (i, f) in vfield.iter().enumerate() {
            if out_is_int[i] {
                let _ = writeln!(s, "      if (v0.{f} !== b{i}[P - 1 - beg.value]) {}", open_diag(i));
            } else {
                let _ = writeln!(s, "      if (svXtierNe(v0.{f}, b{i}[P - 1 - beg.value], zsign)) {}", open_diag(i));
            }
        }
    } else if out_is_int[0] {
        let _ = writeln!(s, "      if (st.value() !== b0[P - 1 - beg.value]) {}", open_diag(0));
    } else {
        let _ = writeln!(s, "      if (svXtierNe(st.value(), b0[P - 1 - beg.value], zsign)) {}", open_diag(0));
    }
    // `pk`, `up`, `vc` and `rp` stay four DISTINCT sinks, or the compares below
    // read one buffer against itself.
    if multi {
        let _ = writeln!(s, "      const pk = new {ocls}();");
        let _ = writeln!(s, "      const up = new {ocls}();");
        let _ = writeln!(s, "      const vc = new {ocls}();");
        let _ = writeln!(s, "      const rp = new {ocls}();");
    }
    s.push_str("      for (let t = P; t < svN; t++) {\n");
    s.push_str("        let pkTook = true;\n");
    if multi {
        let _ = writeln!(s, "        try {{ st.peek({bars_t}, pk); }} catch (e) {{ svRejected(e); pkTook = false; peekRejects++; }}");
    } else {
        s.push_str("        let pk = 0;\n");
        let _ = writeln!(s, "        try {{ pk = st.peek({bars_t}); }} catch (e) {{ svRejected(e); pkTook = false; peekRejects++; }}");
    }
    s.push_str("        if (t % 7 === 0) {\n");
    s.push_str("          let rpTook = pkTook;\n");
    if multi {
        let _ = writeln!(s, "          try {{ st.peek({}, rp); }} catch (e) {{ svRejected(e); peekRejects++; }}", bar_args("t - 1"));
        let _ = writeln!(s, "          try {{ st.peek({bars_t}, rp); }} catch (e) {{ svRejected(e); rpTook = false; }}");
    } else {
        let _ = writeln!(s, "          try {{ st.peek({}); }} catch (e) {{ svRejected(e); peekRejects++; }}", bar_args("t - 1"));
        s.push_str("          let rp = 0;\n");
        let _ = writeln!(s, "          try {{ rp = st.peek({bars_t}); }} catch (e) {{ svRejected(e); rpTook = false; }}");
    }
    s.push_str("          if (rpTook) {\n");
    s.push_str("            peekReps++;\n");
    if multi {
        for (i, f) in vfield.iter().enumerate() {
            if out_is_int[i] {
                let _ = writeln!(s, "            if (rp.{f} !== pk.{f}) peekRepAll = false;");
            } else {
                let _ = writeln!(s, "            if (svBne(rp.{f}, pk.{f})) peekRepAll = false;");
            }
        }
    } else if out_is_int[0] {
        s.push_str("            if (rp !== pk) peekRepAll = false;\n");
    } else {
        s.push_str("            if (svBne(rp, pk)) peekRepAll = false;\n");
    }
    s.push_str("          } else { peekRejects++; }\n");
    s.push_str("        }\n");
    if multi {
        let _ = writeln!(s, "        st.update({bars_t}, up);");
        for (i, f) in vfield.iter().enumerate() {
            if out_is_int[i] {
                let _ = writeln!(s, "        if (pkTook && pk.{f} !== up.{f}) peekAll = false;");
            } else {
                let _ = writeln!(s, "        if (pkTook && svBne(pk.{f}, up.{f})) peekAll = false;");
            }
        }
        // The intervening peek of another bar is what lets `value` == `update`
        // fail: without it both render from the same fields with nothing between.
        let _ = writeln!(s, "        try {{ st.peek({}, pk); }} catch (e) {{ svRejected(e); peekRejects++; }}", bar_args("t - 1"));
        s.push_str("        st.value(vc);\n");
        for (i, f) in vfield.iter().enumerate() {
            if out_is_int[i] {
                let _ = writeln!(s, "        if (vc.{f} !== up.{f}) allOk = false;");
            } else {
                let _ = writeln!(s, "        if (svBne(vc.{f}, up.{f})) allOk = false;");
            }
        }
    } else {
        let _ = writeln!(s, "        const up = st.update({bars_t});");
        let _ = writeln!(
            s,
            "        if (pkTook && {}) peekAll = false;",
            if out_is_int[0] { "pk !== up" } else { "svBne(pk, up)" }
        );
        let _ = writeln!(s, "        try {{ st.peek({}); }} catch (e) {{ svRejected(e); peekRejects++; }}", bar_args("t - 1"));
        let _ = writeln!(
            s,
            "        if ({}) allOk = false;",
            if out_is_int[0] { "st.value() !== up" } else { "svBne(st.value(), up)" }
        );
    }
    let up_val = |i: usize| if multi { format!("up.{}", vfield[i]) } else { "up".to_string() };
    for (i, is_int) in out_is_int.iter().enumerate() {
        let v = up_val(i);
        if *is_int {
            let _ = writeln!(s, "        if ({v} !== b{i}[t - beg.value]) {{ allOk = false; if (diag === '') diag = `,\"badBar\":${{t}},\"badOut\":{i},\"batchv\":\"${{b{i}[t - beg.value]}}\",\"streamv\":\"${{{v}}}\"`; }}");
        } else {
            let _ = writeln!(s, "        if (svXtierNe({v}, b{i}[t - beg.value], zsign)) {{ allOk = false; if (diag === '') diag = `,\"badBar\":${{t}},\"badOut\":{i},\"batchv\":\"${{hex64(float64Bits(b{i}[t - beg.value]))}}\",\"streamv\":\"${{hex64(float64Bits({v}))}}\"`; }}");
        }
    }
    s.push_str("      }\n");
    s.push_str("      if (allOk) {\n");
    let _ = writeln!(s, "        rangeChecked = 1; rangeLegs++; rangeSites |= {};", sv_range_bit(SvRangeSite::Prefix, SV_RANGE_MASK_TS));
    s.push_str("        if (st.outRange().begIdx !== beg.value || st.outRange().count !== nb.value) rangeOk = false;\n");
    let _ = writeln!(s, "        rangeLegs++; rangeSites |= {};", sv_range_bit(SvRangeSite::Advance, SV_RANGE_MASK_TS));
    s.push_str("        st.advance();\n");
    s.push_str("        if (st.outRange().begIdx !== beg.value || st.outRange().count !== nb.value + 1) rangeOk = false;\n");
    s.push_str("      }\n");
    s.push_str("    }\n");

    // clone() independence leg, as Java's copy() leg.
    s.push_str("    {\n");
    s.push_str("      const p0 = lb + 1;\n");
    s.push_str("      if (p0 <= svN - 1) {\n");
    s.push_str("        try {\n");
    s.push_str(&fdecls.replace("      const", "          const"));
    let _ = writeln!(s, "          const sA = {}.stream;", open_fill(&pfx_ins("p0"), &fouts));
    s.push_str("          const mid = Math.trunc((p0 + svN) / 2);\n");
    let val = |u: &str, i: usize| if multi { format!("{u}.{}", vfield[i]) } else { u.to_string() };
    let diverged = |cond: String| {
        format!("            if ({cond}) {{ allOk = false; if (diag === '') diag = `,\"copyDiverged\":${{t}}`; }}\n")
    };
    let ne = |i: usize, a: String, b: String| {
        if out_is_int[i] { format!("{a} !== {b}") } else { format!("svBne({a}, {b})") }
    };
    let off_batch = |i: usize, a: String| {
        if out_is_int[i] {
            format!("{a} !== b{i}[t - beg.value]")
        } else {
            format!("svXtierNe({a}, b{i}[t - beg.value], zsign)")
        }
    };
    if multi {
        let _ = writeln!(s, "          const uA = new {ocls}();");
        let _ = writeln!(s, "          const uB = new {ocls}();");
    }
    s.push_str("          for (let t = p0; t < mid; t++) {\n");
    if multi {
        let _ = writeln!(s, "            sA.update({bars_t}, uA);");
    } else {
        let _ = writeln!(s, "            const uA = sA.update({bars_t});");
    }
    for i in 0..n_out {
        s.push_str(&diverged(off_batch(i, val("uA", i))));
    }
    s.push_str("          }\n");
    s.push_str("          const sB = sA.clone();\n");
    s.push_str("          sB.advance();\n          sB.advance();\n");
    for (i, is_int) in out_is_int.iter().enumerate() {
        let ty = if *is_int { "Int32Array" } else { "Float64Array" };
        let _ = writeln!(s, "          const fk{i} = new {ty}(svN);");
    }
    s.push_str("          for (let t = mid; t < svN; t++) {\n");
    if multi {
        let _ = writeln!(s, "            sB.update({bars_t}, uB);");
    } else {
        let _ = writeln!(s, "            const uB = sB.update({bars_t});");
    }
    for i in 0..n_out {
        let _ = writeln!(s, "            fk{i}[t] = {};", val("uB", i));
        s.push_str(&diverged(off_batch(i, val("uB", i))));
    }
    s.push_str("          }\n");
    s.push_str("          for (let t = mid; t < svN; t++) {\n");
    if multi {
        let _ = writeln!(s, "            sA.update({bars_t}, uA);");
    } else {
        let _ = writeln!(s, "            const uA = sA.update({bars_t});");
    }
    for i in 0..n_out {
        let cond = format!("{} || {}", ne(i, val("uA", i), format!("fk{i}[t]")), off_batch(i, val("uA", i)));
        s.push_str(&diverged(cond));
    }
    s.push_str("          }\n");
    s.push_str("          if (allOk) {\n");
    let _ = writeln!(s, "            rangeChecked = 1; rangeLegs++; rangeSites |= {};", sv_range_bit(SvRangeSite::Copy, SV_RANGE_MASK_TS));
    s.push_str("            if (sA.outRange().begIdx !== beg.value || sA.outRange().count !== nb.value) { rangeOk = false; if (diag === '') diag = ',\"copyRangeSrc\":1'; }\n");
    s.push_str("            if (sB.outRange().begIdx !== beg.value || sB.outRange().count !== nb.value + 2) { rangeOk = false; if (diag === '') diag = ',\"copyRange\":1'; }\n");
    s.push_str("          }\n");
    s.push_str("        } catch (e) { svRejected(e); allOk = false; if (diag === '') diag = ',\"copyOpenReject\":1'; }\n");
    s.push_str("      }\n");
    s.push_str("    }\n");

    // Short-history reject leg: at `lb` bars Open owes INSUFFICIENT_HISTORY.
    s.push_str("    if (lb >= 1 && lb < svN) {\n");
    let _ = writeln!(
        s,
        "      try {{ {}; allOk = false; if (diag === '') diag = ',\"shortHistoryAccepted\":1'; }}",
        open(&pfx_ins("lb"))
    );
    s.push_str("      catch (e) { if (!svShortHistory(e)) { allOk = false; if (diag === '') diag = ',\"shortHistoryWrongType\":1'; } }\n");
    s.push_str("      {\n");
    s.push_str(&fdecls.replace("      const", "        const"));
    let _ = writeln!(
        s,
        "        try {{ {}; allOk = false; if (diag === '') diag = ',\"shortHistoryFillAccepted\":1'; }}",
        open_fill(&pfx_ins("lb"), &fouts)
    );
    s.push_str("        catch (e) { if (!svShortHistory(e)) { allOk = false; if (diag === '') diag = ',\"shortHistoryFillWrongType\":1'; } }\n");
    s.push_str("      }\n");
    s.push_str("    }\n");

    // Default-sentinel leg: open(INTEGER_DEFAULT) == open(explicit default), bitwise.
    if has_int_default {
        let mut sent_args: Vec<String> = Vec::new();
        let mut expl_args: Vec<String> = Vec::new();
        for prm in &func.optional_inputs {
            match &prm.param_type {
                crate::ir::ParamType::Integer if prm.default.is_some() => {
                    let d = prm.default.unwrap_or(0.0) as i64;
                    sent_args.push("Core.INTEGER_DEFAULT".to_string());
                    expl_args.push(format!("{d}"));
                }
                _ => {
                    sent_args.push(prm.name.clone());
                    expl_args.push(prm.name.clone());
                }
            }
        }
        s.push_str("    try {\n");
        let _ = writeln!(s, "      const sD = {};", api.open(&full, &sent_args, "c2"));
        let _ = writeln!(s, "      const sE = {};", api.open(&full, &expl_args, "c2"));
        let dflt = "{ allOk = false; if (diag === '') diag = ',\"minValueDefault\":1'; }";
        if multi {
            let _ = writeln!(s, "      const vD = new {ocls}(); sD.value(vD);");
            let _ = writeln!(s, "      const vE = new {ocls}(); sE.value(vE);");
            for (i, f) in vfield.iter().enumerate() {
                if out_is_int[i] {
                    let _ = writeln!(s, "      if (vD.{f} !== vE.{f}) {dflt}");
                } else {
                    let _ = writeln!(s, "      if (svBne(vD.{f}, vE.{f})) {dflt}");
                }
            }
        } else if out_is_int[0] {
            let _ = writeln!(s, "      if (sD.value() !== sE.value()) {dflt}");
        } else {
            let _ = writeln!(s, "      if (svBne(sD.value(), sE.value())) {dflt}");
        }
        s.push_str("    } catch (e) { svRejected(e); }\n");
    }

    // The startIdx-anchored range site, through `_OpenInternal`.
    s.push_str("    {\n");
    let anchored_bit = sv_range_bit(SvRangeSite::Anchored, SV_RANGE_MASK_TS);
    s.push_str("      const Sidx = lb + Math.trunc((svN - lb) / 3);\n");
    s.push_str("      if (Sidx > lb && Sidx < svN - 1) {\n");
    s.push_str("        const begS = new MInteger();\n");
    s.push_str("        const nbS = new MInteger();\n");
    s.push_str("        let rcS: number;\n");
    let _ = writeln!(
        s,
        "        try {{ rcS = {impl_fn}(c2, Sidx, svN - 1, {full_ins}, {opts_lead}begS, nbS{bargs}); }}\n        catch (e) {{ if (!(e instanceof TALibError)) throw e; rcS = e.retCode; }}"
    );
    s.push_str("        if (rcS === RetCode.SUCCESS && nbS.value > 0) {\n");
    let _ = writeln!(
        s,
        "          try {{\n\
         \x20           const stA = {open_internal}(c2, {}, Sidx{opts_tail});\n\
         \x20           rangeChecked = 1; rangeLegs++; rangeSites |= {anchored_bit};\n\
         \x20           if (stA.outRange().begIdx !== begS.value || stA.outRange().count !== nbS.value) rangeOk = false;\n\
         \x20         }} catch (e) {{ svRejected(e); rangeOk = false; if (diag === '') diag = ',\"anchoredOpenRejected\":1'; }}",
        pfx_ins("svN").join(", ")
    );
    s.push_str("        }\n");
    s.push_str("      }\n");
    s.push_str("    }\n");

    s.push_str("  }\n");
    let _ = writeln!(
        s,
        "  return `{{\"retCode\":0,\"beg\":${{beg.value}},\"nb\":${{nb.value}},\"legs\":${{legs}},\"fill_checked\":${{fillChecked}},\"fill_ok\":${{fillOk ? 1 : 0}},\"range_checked\":${{rangeChecked}},\"range_legs\":${{rangeLegs}},\"range_sites\":${{rangeSites}},\"range_sites_all\":{SV_RANGE_MASK_TS},\"range_ok\":${{rangeOk ? 1 : 0}},\"step_ok\":${{allOk ? 1 : 0}},\"ok\":${{allOk && fillOk && rangeOk ? 1 : 0}},\"peek_ok\":${{peekAll ? 1 : 0}},\"peek_reps\":${{peekReps}},\"peek_rep_ok\":${{peekRepAll ? 1 : 0}},\"peek_rejects\":${{peekRejects}},\"benign\":${{zsign[0]}},\"round_mask\":${{roundMask}}${{diag}}}}`;"
    );
    s.push_str("}\n\n");
    s
}

/// The whole TS `stream_verify` section: compare helpers, the candle rounds,
/// one `sv_<NAME>` per function with a TS stream, and the dispatcher, whose
/// unknown names (TA_STREAM_PROBE included) answer `not_streamable`.
pub(crate) fn generate_ts_stream_verify(funcs: &[FuncDef], enums: &HashMap<String, EnumDef>) -> String {
    use std::fmt::Write as _;
    let mut s = String::new();
    s.push_str(SV_TS_SUPPORT);
    let emitted: Vec<&FuncDef> = funcs
        .iter()
        .filter(|f| crate::backends::ts_stream::emits_stream(f, &crate::streaming::FuncsLookup(funcs)))
        .collect();
    for f in &emitted {
        s.push_str(&emit_ts_sv_func(f, funcs, enums));
    }
    s.push_str("function handleStreamVerify(p: Params): string {\n");
    s.push_str("  switch (p['funcName']) {\n");
    for f in &emitted {
        let _ = writeln!(s, "    case 'TA_{}': return sv_{}(p);", f.name.to_uppercase(), f.name);
    }
    s.push_str("    default: return '{\"error\":\"not_streamable\"}';\n");
    s.push_str("  }\n");
    s.push_str("}\n\n");
    s
}

const SV_TS_SUPPORT: &str = r"// ---- stream_verify: TS stream vs TS batch, bitwise ----

const SV_F64 = new Float64Array(2);
const SV_U32 = new Uint32Array(SV_F64.buffer);

function svBne(a: number, b: number): boolean {
  SV_F64[0] = a;
  SV_F64[1] = b;
  return SV_U32[0] !== SV_U32[2] || SV_U32[1] !== SV_U32[3];
}

// Differing bits that compare equal are +0.0 vs -0.0: counted, never a mismatch.
function svXtierNe(a: number, b: number, zsign: number[]): boolean {
  if (!svBne(a, b)) return false;
  if (a === b) { zsign[0]++; return false; }
  return true;
}

function svRejected(e: unknown): true {
  if (e instanceof TALibError && (e.retCode === RetCode.BAD_PARAM || e.retCode === RetCode.INSUFFICIENT_HISTORY)) return true;
  throw e;
}

// True for INSUFFICIENT_HISTORY, false for BAD_PARAM; anything else escapes.
function svShortHistory(e: unknown): boolean {
  svRejected(e);
  return (e as TALibError).retCode === RetCode.INSUFFICIENT_HISTORY;
}

// Defaults / avgPeriod+3 / avgPeriod=0 / rangeType=Shadows / the other range type, as every server sweeps.
function svCandleRound(rd: number): CoreBuilder {
  const b = Core.builder();
  if (rd === 0) return b;
  for (const cs of Core.DEFAULT.candleSettings) {
    if (rd === 1) b.candleSetting(cs.settingType as Parameters<CoreBuilder['candleSetting']>[0], cs.rangeType, cs.avgPeriod + 3, cs.factor);
    else if (rd === 2) b.candleSetting(cs.settingType as Parameters<CoreBuilder['candleSetting']>[0], cs.rangeType, 0, cs.factor);
    else if (rd === 3) b.candleSetting(cs.settingType as Parameters<CoreBuilder['candleSetting']>[0], RangeType.SHADOWS, cs.avgPeriod, cs.factor);
    else b.candleSetting(cs.settingType as Parameters<CoreBuilder['candleSetting']>[0], cs.rangeType === RangeType.REAL_BODY ? RangeType.HIGH_LOW : RangeType.REAL_BODY, cs.avgPeriod, cs.factor);
  }
  return b;
}

";
