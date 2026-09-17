use crate::ir::{FuncDef, ParamType};
use crate::server_gen::expand_input_names;
use std::fmt::Write as _;

/// The Rust ride-along. Same contract as C's, in Rust's idiom: outlined per
/// function so the handler body is unchanged except one guarded call, and
/// reaching only the PUBLIC streaming tier, which is all a separate `tools`
/// crate can call.
///
/// The dedup key folds the request's own `unstablePeriod` and a generation
/// counter bumped by every non-indicator method, because `Core`'s ambient state
/// is `pub(crate)` and unreadable from here: a second sweep at a new unstable
/// period over identical bars would otherwise collide with the first.
pub(crate) fn generate_rust_ridealong(funcs: &[FuncDef]) -> String {
    let mut s = String::new();
    s.push_str(RIDE_RUST_SUPPORT);
    for func in funcs {
        if !func.streaming {
            continue;
        }
        s.push_str(&emit_rust_ridealong_fn(func));
    }
    s
}

const RIDE_RUST_SUPPORT: &str = r#"// ---- ride-along: batch-vs-stream on caller-supplied data ----
const RIDE_MAX_BARS: usize = 4096;
const RIDE_SEEN_N: usize = 2048;

thread_local! {
    static RIDE_SEEN: std::cell::RefCell<Vec<(u64, i32, i32)>> =
        std::cell::RefCell::new(vec![(0, 0, 0); RIDE_SEEN_N]);
    static RIDE_GEN: std::cell::Cell<u64> = const { std::cell::Cell::new(0) };
}

fn ride_gate(params: &Value) -> bool {
    params["timed"].as_i64().unwrap_or(0) == 0
        && params["iters"].as_i64().unwrap_or(1) <= 1
        && params["no_output"].as_i64().unwrap_or(0) == 0
        && params["bench_mode"].as_i64().unwrap_or(0) == 0
        && params["use_float"].as_i64().unwrap_or(0) == 0
}

fn ride_finite(a: &[f64]) -> bool {
    a.iter().all(|v| v.is_finite())
}

fn ride_mix_u64(h: u64, v: u64) -> u64 {
    let mut acc = h;
    for b in v.to_le_bytes() {
        acc ^= u64::from(b);
        acc = acc.wrapping_mul(1_099_511_628_211);
    }
    acc
}

fn ride_mix_str(h: u64, s: &str) -> u64 {
    let mut acc = h;
    for b in s.as_bytes() {
        acc ^= u64::from(*b);
        acc = acc.wrapping_mul(1_099_511_628_211);
    }
    acc
}

fn ride_mix_f64s(h: u64, a: &[f64]) -> u64 {
    let mut acc = h;
    for v in a {
        acc = ride_mix_u64(acc, v.to_bits());
    }
    acc
}

struct RideResult {
    ok: bool,
    skip: i32,
    dedup: i32,
    open_bars: i32,
    fill_bars: i32,
    benign: i64,
    m: i32,
    lb: i32,
    leg: i32,
    bar: i32,
    out: i32,
    batch: u64,
    stream: u64,
    rej: i32,
    rc_batch: i32,
    rc_open: i32,
    rc_fill: i32,
}

impl RideResult {
    fn new() -> Self {
        RideResult { ok: true, skip: 0, dedup: 0, open_bars: 0, fill_bars: 0, benign: 0,
                     m: 0, lb: -1, leg: 0, bar: -1, out: -1, batch: 0, stream: 0,
                     rej: 0, rc_batch: 0, rc_open: 0, rc_fill: 0 }
    }
    fn emit(&self, resp: &mut String) {
        resp.push_str(&format!(
            ",\"ride_ok\":{},\"ride_skip\":{},\"ride_dedup\":{},\"ride_open_bars\":{},\"ride_fill_bars\":{},\"ride_benign\":{},\"ride_m\":{},\"ride_lb\":{},\"ride_rej\":{},\"ride_rc_batch\":{},\"ride_rc_open\":{},\"ride_rc_fill\":{}",
            i32::from(self.ok), self.skip, self.dedup, self.open_bars, self.fill_bars,
            self.benign, self.m, self.lb, self.rej, self.rc_batch, self.rc_open, self.rc_fill));
        if !self.ok {
            resp.push_str(&format!(
                ",\"ride_leg\":{},\"ride_bar\":{},\"ride_out\":{},\"ride_batch\":\"{:016x}\",\"ride_stream\":\"{:016x}\"",
                self.leg, self.bar, self.out, self.batch, self.stream));
        }
    }
}

"#;

#[allow(clippy::too_many_lines)]
fn emit_rust_ridealong_fn(func: &FuncDef) -> String {
    let n = func.name.clone();
    let base = crate::backends::common::snake_words(&func.name);
    let input_names = expand_input_names(&func.inputs);
    let outs = &func.outputs;
    let nouts = outs.len();
    let mut s = String::new();

    let mut sig_ins = String::new();
    let mut slice_m = String::new();
    let mut slice_open = String::new();
    let mut upd_args = String::new();
    // The reject leg hands every entry point the SAME range: nothing reads a
    // bar before it says no, so there is nothing to shorten.
    let mut slice_rej = String::new();
    for name in &input_names {
        let _ = write!(sig_ins, "{name}: &[f64], ");
        let _ = write!(slice_m, "&{name}[..m], ");
        let _ = write!(slice_open, "&{name}[..=lb], ");
        let _ = write!(slice_rej, "&{name}[..m], ");
        let _ = write!(upd_args, "{name}[t], ");
    }
    let mut sig_opts = String::new();
    let mut opt_args = String::new();
    for opt in &func.optional_inputs {
        let ty = match &opt.param_type {
            ParamType::Real => "f64",
            ParamType::Enum(_) => "MAType",
            _ => "i32",
        };
        let _ = write!(sig_opts, "{}: {ty}, ", opt.name);
        let _ = write!(opt_args, "{}, ", opt.name);
    }

    // Batch reference buffers, and the same buffers again for the fill leg.
    let mut ref_decl = String::new();
    let mut ref_args = String::new();
    let mut fill_decl = String::new();
    let mut fill_args = String::new();
    let mut slot_of: Vec<(bool, usize)> = Vec::new();
    {
        let (mut r, mut i) = (0usize, 0usize);
        for out in outs {
            let (op, cl) = if out.is_nullable() { ("Some(", ")") } else { ("", "") };
            if out.param_type == ParamType::Integer {
                let _ = writeln!(ref_decl, "    let mut rib{i} = vec![0i32; m];");
                let _ = writeln!(fill_decl, "        let mut fib{i} = vec![0i32; m];");
                let _ = write!(ref_args, ", {op}&mut rib{i}{cl}");
                let _ = write!(fill_args, ", {op}&mut fib{i}{cl}");
                slot_of.push((true, i));
                i += 1;
            } else {
                let _ = writeln!(ref_decl, "    let mut rb{r} = vec![0.0f64; m];");
                let _ = writeln!(fill_decl, "        let mut fb{r} = vec![0.0f64; m];");
                let _ = write!(ref_args, ", {op}&mut rb{r}{cl}");
                let _ = write!(fill_args, ", {op}&mut fb{r}{cl}");
                slot_of.push((false, r));
                r += 1;
            }
        }
    }

    // Single-output tiers hand back a bare value; multi-output ones a tuple.
    let field = |k: usize| -> String {
        if nouts == 1 { "u".to_string() } else { format!("u.{k}") }
    };

    let cmp_bar = |idx: &str, indent: &str, leg: i32, barexpr: &str| -> String {
        let mut c = String::new();
        let _ = writeln!(c, "{indent}let mut cmp = true;");
        for (k, (is_int, slot)) in slot_of.iter().enumerate() {
            let v = field(k);
            if *is_int {
                let _ = writeln!(
                    c,
                    "{indent}if cmp && {v} != rib{slot}[{idx}] {{ cmp = false; r.out = {k}; r.batch = f64::from(rib{slot}[{idx}]).to_bits(); r.stream = f64::from({v}).to_bits(); }}"
                );
            } else {
                let _ = writeln!(
                    c,
                    "{indent}if cmp && sv_xtier_ne(rb{slot}[{idx}], {v}, &mut r.benign) {{ cmp = false; r.out = {k}; r.batch = rb{slot}[{idx}].to_bits(); r.stream = {v}.to_bits(); }}"
                );
            }
        }
        let _ = writeln!(c, "{indent}if cmp {{ r.open_bars += 1; }}");
        let _ = writeln!(
            c,
            "{indent}if !cmp {{ r.ok = false; r.leg = {leg}; r.bar = {barexpr}; }}"
        );
        c
    };

    let _ = writeln!(
        s,
        "#[allow(clippy::too_many_arguments, clippy::needless_range_loop)]\nfn ride_{base}(core: &Core, params: &Value, endIdx: usize, {sig_ins}{sig_opts}resp: &mut String) {{"
    );
    s.push_str("    if !ride_gate(params) { return; }\n");
    s.push_str("    let mut r = RideResult::new();\n");
    // A negative lookback is a REJECTED parameter and travels on: it is the
    // only rejection the ride can reach, and the reject leg below is what
    // reads it. Everything between here and there must tolerate it.
    let _ = writeln!(
        s,
        "    let lb_opt = core.{n}_Lookback({}).ok();",
        opt_args.trim_end().trim_end_matches(',')
    );
    s.push_str("    r.lb = match lb_opt { Some(v) => v as i32, None => -1 };\n");
    s.push_str("    let mut navail = endIdx + 1;\n");
    for name in &input_names {
        let _ = writeln!(s, "    if {name}.len() < navail {{ navail = {name}.len(); }}");
    }
    // The success path shortens the replay to keep the ride cheap; the reject
    // path has nothing to shorten and takes the caller's own range.
    s.push_str("    let mut m = match lb_opt { Some(lb) => 2 * lb + 10, None => navail };\n");
    s.push_str("    if m > navail { m = navail; }\n");
    s.push_str("    r.m = m as i32;\n");
    s.push_str("    if m > RIDE_MAX_BARS { r.skip = 1; r.emit(resp); return; }\n");
    s.push_str("    if m < 1 { r.skip = 2; r.emit(resp); return; }\n");
    s.push_str("    if matches!(lb_opt, Some(lb) if m < lb + 2) { r.skip = 3; r.emit(resp); return; }\n");
    s.push_str("    if ");
    for name in &input_names {
        let _ = write!(s, "!ride_finite(&{name}[..m]) || ");
    }
    s.push_str("false { r.skip = 4; r.emit(resp); return; }\n\n");

    s.push_str("    let mut key = fuzz_hash_init();\n");
    let _ = writeln!(s, "    key = ride_mix_str(key, \"TA_{}\");", n.to_uppercase());
    s.push_str("    key = ride_mix_u64(key, m as u64);\n");
    s.push_str("    key = ride_mix_u64(key, RIDE_GEN.with(std::cell::Cell::get));\n");
    s.push_str("    key = ride_mix_u64(key, params[\"unstablePeriod\"].as_i64().unwrap_or(0) as u64);\n");
    for opt in &func.optional_inputs {
        match &opt.param_type {
            ParamType::Real => {
                let _ = writeln!(s, "    key = ride_mix_u64(key, {}.to_bits());", opt.name);
            }
            ParamType::Enum(_) => {
                let _ = writeln!(s, "    key = ride_mix_u64(key, {} as i32 as u64);", opt.name);
            }
            _ => {
                let _ = writeln!(s, "    key = ride_mix_u64(key, {} as u64);", opt.name);
            }
        }
    }
    for name in &input_names {
        let _ = writeln!(s, "    key = ride_mix_f64s(key, &{name}[..m]);");
    }
    s.push_str("    key = fuzz_hash_fin(key);\n");
    s.push_str("    let slot = (key as usize) % RIDE_SEEN_N;\n");
    s.push_str("    if let Some((ob, fb)) = RIDE_SEEN.with(|t| { let t = t.borrow(); let e = t[slot]; if e.0 == key { Some((e.1, e.2)) } else { None } }) {\n");
    s.push_str("        r.dedup = 1; r.open_bars = ob; r.fill_bars = fb; r.emit(resp); return;\n    }\n\n");

    s.push_str(&ref_decl);
    // The reject leg. A rejection is a property of the CALL, so the three entry
    // points owe the same answer on it, over the same range.
    let reject_buffers = fill_decl.replace("        let mut f", "            let mut f");
    let _ = writeln!(
        s,
        "    let (beg, nb) = match core.{n}(0, m - 1, {slice_m}{opt_args}{}) {{",
        ref_args.trim_start_matches(", ")
    );
    s.push_str("        Ok(rr) => (rr.beg_idx, rr.count),\n");
    s.push_str("        Err(rc) => {\n");
    s.push_str("            r.rc_batch = retcode_to_int(rc);\n");
    let _ = writeln!(
        s,
        "            r.rc_open = match core.{base}_open({slice_rej}{}) {{ Ok(_) => 0, Err(e) => retcode_to_int(e) }};",
        opt_args.trim_end().trim_end_matches(',')
    );
    s.push_str(&reject_buffers);
    let _ = writeln!(
        s,
        "            r.rc_fill = match core.{base}_open_and_fill({slice_rej}{opt_args}{}) {{ Ok(_) => 0, Err(e) => retcode_to_int(e) }};",
        fill_args.trim_start_matches(", ")
    );
    // `r.rej` is the comparison's own verdict, exactly like the bar counters:
    // a count bumped beside a compare keeps climbing once the compare is gone.
    s.push_str("            let mut cmp = r.rc_open == r.rc_batch;\n");
    s.push_str("            if cmp { r.rej += 1; }\n");
    s.push_str("            if !cmp { r.ok = false; r.leg = 3; }\n");
    s.push_str("            cmp = r.rc_fill == r.rc_batch;\n");
    s.push_str("            if cmp { r.rej += 1; }\n");
    s.push_str("            if !cmp { r.ok = false; r.leg = 3; }\n");
    s.push_str("            r.emit(resp);\n            return;\n");
    s.push_str("        }\n    };\n");
    // Lookback said no and the batch tier said yes: the two read the same
    // parameters, so the replay below has no anchor to stand on.
    s.push_str("    let lb = match lb_opt { Some(v) => v, None => { r.skip = 7; r.emit(resp); return; } };\n");
    s.push_str("    if nb == 0 { r.skip = 5; r.emit(resp); return; }\n");
    s.push_str("    if beg != lb { r.skip = 6; r.emit(resp); return; }\n\n");

    let _ = writeln!(
        s,
        "    match core.{base}_open({slice_open}{}) {{",
        opt_args.trim_end().trim_end_matches(',')
    );
    s.push_str("        Err(_) => { r.ok = false; r.leg = 1; r.bar = lb as i32; }\n");
    s.push_str("        Ok((mut st, u)) => {\n");
    s.push_str(&cmp_bar("lb - beg", "            ", 1, "lb as i32"));
    s.push_str("            for t in (lb + 1)..m {\n");
    let _ = writeln!(
        s,
        "                match st.update({}) {{",
        upd_args.trim_end().trim_end_matches(',')
    );
    s.push_str("                    Err(_) => { r.ok = false; r.leg = 1; r.bar = t as i32; break; }\n");
    s.push_str("                    Ok(u) => {\n");
    s.push_str(&cmp_bar("t - beg", "                        ", 1, "t as i32"));
    s.push_str("                    }\n                }\n");
    s.push_str("                if !r.ok { break; }\n");
    s.push_str("            }\n        }\n    }\n\n");

    s.push_str("    if r.ok {\n");
    s.push_str(&fill_decl);
    let _ = writeln!(
        s,
        "        match core.{base}_open_and_fill({slice_m}{opt_args}{}) {{",
        fill_args.trim_start_matches(", ")
    );
    s.push_str("            Err(_) => { r.ok = false; r.leg = 2; }\n");
    s.push_str("            Ok((_st, rng)) => {\n");
    s.push_str("                if rng.beg_idx != beg || rng.count != nb { r.ok = false; r.leg = 2; }\n");
    s.push_str("                if r.ok {\n");
    s.push_str("                    for k in 0..nb {\n");
    s.push_str("                        let mut cmp = true;\n");
    for (k, (is_int, slot)) in slot_of.iter().enumerate() {
        if *is_int {
            let _ = writeln!(
                s,
                "                        if cmp && fib{slot}[k] != rib{slot}[k] {{ cmp = false; r.out = {k}; r.batch = f64::from(rib{slot}[k]).to_bits(); r.stream = f64::from(fib{slot}[k]).to_bits(); }}"
            );
        } else {
            let _ = writeln!(
                s,
                "                        if cmp && sv_xtier_ne(rb{slot}[k], fb{slot}[k], &mut r.benign) {{ cmp = false; r.out = {k}; r.batch = rb{slot}[k].to_bits(); r.stream = fb{slot}[k].to_bits(); }}"
            );
        }
    }
    s.push_str("                        if cmp { r.fill_bars += 1; }\n");
    s.push_str("                        if !cmp { r.ok = false; r.leg = 2; r.bar = (beg + k) as i32; break; }\n");
    s.push_str("                    }\n                }\n            }\n        }\n    }\n\n");

    s.push_str("    if r.ok {\n");
    s.push_str("        RIDE_SEEN.with(|t| { t.borrow_mut()[slot] = (key, r.open_bars, r.fill_bars); });\n");
    s.push_str("    }\n");
    s.push_str("    r.emit(resp);\n");
    s.push_str("}\n\n");
    s
}
