use super::ride_arg_list;
use crate::backends::ts::ts_names;
use crate::ir::{FuncDef, ParamType};
use crate::server_gen::expand_input_names;
use std::fmt::Write as _;

/// The TypeScript ride-along, the Java one recased. Each leg keeps its own
/// try, for the reason the Java emitter gives.
pub(crate) fn generate_ts_ridealong(funcs: &[FuncDef]) -> String {
    let mut s = String::new();
    s.push_str(RIDE_TS_SUPPORT);
    for func in funcs {
        if !func.streaming {
            continue;
        }
        s.push_str(&emit_ts_ridealong_fn(func));
    }
    s
}

// The key is two 32-bit FNV-1a lanes with different primes: 64 bits without
// BigInt arithmetic on every input byte.
const RIDE_TS_SUPPORT: &str = r#"
// ---- ride-along: batch-vs-stream on caller-supplied data ----
const RIDE_MAX_BARS = 4096;
const RIDE_SEEN_N = 2048;
const rideSeenA = new Int32Array(RIDE_SEEN_N);
const rideSeenB = new Int32Array(RIDE_SEEN_N);
const rideSeenUsed = new Uint8Array(RIDE_SEEN_N);
const rideSeenOpen = new Int32Array(RIDE_SEEN_N);
const rideSeenFill = new Int32Array(RIDE_SEEN_N);
let rideGen = 0;

function rideGate(p: Params): boolean {
  if (int(p, 'timed', 0) !== 0) return false;
  if (int(p, 'no_output', 0) !== 0) return false;
  if (int(p, 'use_float', 0) !== 0) return false;
  if (int(p, 'bench_mode', 0) !== 0) return false;
  return int(p, 'iters', 0) <= 1;
}

// 0 means the call returned; -1 is a throw the library does not own, which is
// itself a divergence.
function rideCode(e: unknown): number {
  return e instanceof TALibError ? e.retCode : -1;
}

function rideCodeName(e: unknown): string {
  if (e instanceof TALibError) return e.code;
  return e instanceof Error ? e.name : typeof e;
}

function rideFinite(a: Float64Array, n: number): boolean {
  for (let i = 0; i < n; i++) if (!Number.isFinite(a[i])) return false;
  return true;
}

interface RideKey { a: number; b: number }

const RIDE_F64 = new Float64Array(1);
const RIDE_U8 = new Uint8Array(RIDE_F64.buffer);

function rideByte(h: RideKey, x: number): void {
  h.a = Math.imul(h.a ^ x, 0x01000193);
  h.b = Math.imul(h.b ^ x, 0x5bd1e995);
}

function rideKey(): RideKey {
  return { a: 0x811c9dc5 | 0, b: 0x050c5d1f };
}

function rideMix(h: RideKey, v: number): RideKey {
  RIDE_F64[0] = v;
  for (let i = 0; i < 8; i++) rideByte(h, RIDE_U8[i]);
  return h;
}

function rideMixStr(h: RideKey, s: string): RideKey {
  for (let i = 0; i < s.length; i++) rideByte(h, s.charCodeAt(i) & 0xff);
  return h;
}

function rideMixArr(h: RideKey, a: Float64Array, n: number): RideKey {
  const u8 = new Uint8Array(a.buffer, a.byteOffset, n * 8);
  for (let i = 0; i < u8.length; i++) rideByte(h, u8[i]);
  return h;
}

class RideResult {
  ok = true;
  skip = 0;
  dedup = 0;
  openBars = 0;
  fillBars = 0;
  leg = 0;
  bar = -1;
  out = -1;
  m = 0;
  lb = -1;
  rej = 0;
  rcBatch = 0;
  rcOpen = 0;
  rcFill = 0;
  batch = 0n;
  stream = 0n;
  benign: number[] = [0];

  emit(): string {
    let s = `,"ride_ok":${this.ok ? 1 : 0},"ride_skip":${this.skip},"ride_dedup":${this.dedup}`
      + `,"ride_open_bars":${this.openBars},"ride_fill_bars":${this.fillBars},"ride_benign":${this.benign[0]}`
      + `,"ride_m":${this.m},"ride_lb":${this.lb},"ride_rej":${this.rej}`
      + `,"ride_rc_batch":${this.rcBatch},"ride_rc_open":${this.rcOpen},"ride_rc_fill":${this.rcFill}`;
    if (!this.ok) {
      s += `,"ride_leg":${this.leg},"ride_bar":${this.bar},"ride_out":${this.out}`
        + `,"ride_batch":"${hex64(this.batch)}","ride_stream":"${hex64(this.stream)}"`;
    }
    return s;
  }
}

"#;

#[allow(clippy::too_many_lines)]
fn emit_ts_ridealong_fn(func: &FuncDef) -> String {
    let names = ts_names(&func.name);
    let m_ns = &func.name;
    let pas = crate::backends::common::pascal_words(&func.name);
    let input_names = expand_input_names(&func.inputs);
    let outs = &func.outputs;
    let multi = outs.len() > 1;
    let holder = format!("{m_ns}.{}", names.out_class);
    let mut s = String::new();

    let mut sig_ins = String::new();
    let mut upd_args = String::new();
    for name in &input_names {
        let _ = write!(sig_ins, "{name}: Float64Array, ");
        let _ = write!(upd_args, "{name}[t], ");
    }
    let slices = |n: &str| -> Vec<String> { input_names.iter().map(|i| format!("{i}.slice(0, {n})")).collect() };
    let args_m = slices("m");
    // The reject leg hands every entry point the SAME range.
    let args_rej = slices("m");
    let args_open = slices("lb + 1");
    let api = crate::server_gen::TsPublic::new(func);
    let opt_names: Vec<&str> = func.optional_inputs.iter().map(|o| o.name.as_str()).collect();
    let mut sig_opts = String::new();
    let mut opt_args = String::new();
    for opt in &func.optional_inputs {
        let ty = match &opt.param_type {
            ParamType::Enum(e) => e.as_str(),
            _ => "number",
        };
        let _ = write!(sig_opts, "{}: {ty}, ", opt.name);
        let _ = write!(opt_args, "{}, ", opt.name);
    }

    let mut ref_decl = String::new();
    let mut fill_decl = String::new();
    let mut ref_args: Vec<String> = Vec::new();
    let mut fill_args: Vec<String> = Vec::new();
    let mut slot_of: Vec<(bool, usize)> = Vec::new();
    {
        let (mut r, mut i) = (0usize, 0usize);
        for out in outs {
            if out.param_type == ParamType::Integer {
                let _ = writeln!(ref_decl, "  const rib{i} = new Int32Array(m);");
                let _ = writeln!(fill_decl, "    const fib{i} = new Int32Array(m);");
                ref_args.push(format!("rib{i}"));
                fill_args.push(format!("fib{i}"));
                slot_of.push((true, i));
                i += 1;
            } else {
                let _ = writeln!(ref_decl, "  const rb{r} = new Float64Array(m);");
                let _ = writeln!(fill_decl, "    const fb{r} = new Float64Array(m);");
                ref_args.push(format!("rb{r}"));
                fill_args.push(format!("fb{r}"));
                slot_of.push((false, r));
                r += 1;
            }
        }
    }

    let val = |k: usize| -> String {
        if multi {
            format!("uo.{}", crate::backends::ts::output_key(&outs[k].name))
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
                    "{indent}if (cmp && {v} !== rib{slot}[{idx}]) {{ cmp = false; r.out = {k}; r.batch = float64Bits(rib{slot}[{idx}]); r.stream = float64Bits({v}); }}"
                );
            } else {
                let _ = writeln!(
                    c,
                    "{indent}if (cmp && svXtierNe(rb{slot}[{idx}], {v}, r.benign)) {{ cmp = false; r.out = {k}; r.batch = float64Bits(rb{slot}[{idx}]); r.stream = float64Bits({v}); }}"
                );
            }
        }
        let _ = writeln!(c, "{indent}if (cmp) r.openBars++;");
        let _ = writeln!(c, "{indent}if (!cmp) {{ r.ok = false; r.leg = 1; r.bar = {barexpr}; }}");
        c
    };

    let _ = writeln!(
        s,
        "function ride{pas}(core: Core, p: Params, endIdx: number, {}): string {{",
        format!("{sig_ins}{sig_opts}").trim_end().trim_end_matches(',')
    );
    s.push_str("  if (!rideGate(p)) return '';\n");
    s.push_str("  const r = new RideResult();\n");
    let _ = writeln!(
        s,
        "  rideBody{pas}(core, p, endIdx, {}{}r);",
        ride_arg_list(&input_names),
        opt_args
    );
    s.push_str("  return r.emit();\n}\n\n");

    let _ = writeln!(
        s,
        "function rideBody{pas}(core: Core, p: Params, endIdx: number, {sig_ins}{sig_opts}r: RideResult): void {{"
    );
    // A negative lookback is a REJECTED parameter and travels on to the reject
    // leg; everything between here and there must tolerate it.
    let _ = writeln!(s, "  r.lb = {};", api.lookback(&opt_names, "core"));
    s.push_str("  const lb = r.lb;\n");
    s.push_str("  let navail = endIdx + 1;\n");
    for name in &input_names {
        let _ = writeln!(s, "  if ({name}.length < navail) navail = {name}.length;");
    }
    s.push_str("  let m = lb >= 0 ? 2 * lb + 10 : navail;\n");
    s.push_str("  if (m > navail) m = navail;\n");
    s.push_str("  r.m = m;\n");
    s.push_str("  if (m > RIDE_MAX_BARS) { r.skip = 1; return; }\n");
    s.push_str("  if (m < 1) { r.skip = 2; return; }\n");
    s.push_str("  if (lb >= 0 && m < lb + 2) { r.skip = 3; return; }\n");
    s.push_str("  if (");
    for name in &input_names {
        let _ = write!(s, "!rideFinite({name}, m) || ");
    }
    s.push_str("false) { r.skip = 4; return; }\n\n");

    s.push_str("  let hash = rideKey();\n");
    let _ = writeln!(s, "  hash = rideMixStr(hash, 'TA_{}');", func.name.to_uppercase());
    s.push_str("  hash = rideMix(hash, m);\n");
    s.push_str("  hash = rideMix(hash, rideGen);\n");
    s.push_str("  hash = rideMix(hash, int(p, 'unstablePeriod', 0));\n");
    for opt in &func.optional_inputs {
        let _ = writeln!(s, "  hash = rideMix(hash, {});", opt.name);
    }
    for name in &input_names {
        let _ = writeln!(s, "  hash = rideMixArr(hash, {name}, m);");
    }
    s.push_str("  const slot = hash.a & (RIDE_SEEN_N - 1);\n");
    s.push_str("  if (rideSeenUsed[slot] !== 0 && rideSeenA[slot] === hash.a && rideSeenB[slot] === hash.b) {\n");
    s.push_str("    r.dedup = 1; r.openBars = rideSeenOpen[slot]; r.fillBars = rideSeenFill[slot]; return;\n  }\n\n");

    s.push_str(&ref_decl);
    s.push_str("  let beg = 0;\n  let nb = 0;\n");
    s.push_str("  let codeB = '';\n");
    s.push_str("  let rejected = false;\n");
    let _ = writeln!(
        s,
        "  try {{ const _rr = {}; beg = _rr.begIdx; nb = _rr.{}.length; }}",
        api.batch(&args_m, &opt_names, ("0", "m - 1"), "core", &ref_args),
        api.result_keys()[0]
    );
    s.push_str("  catch (_e) { r.rcBatch = rideCode(_e); codeB = rideCodeName(_e); rejected = true; }\n");
    // A rejection is a property of the CALL: the three entry points owe the
    // same code, and the same `code` name, over the same range.
    s.push_str("  if (rejected) {\n");
    s.push_str("    let codeO = '';\n    let codeF = '';\n");
    let _ = writeln!(
        s,
        "    try {{ {}; }} catch (_e) {{ r.rcOpen = rideCode(_e); codeO = rideCodeName(_e); }}",
        api.open(&args_rej, &opt_names, "core")
    );
    s.push_str(&fill_decl);
    let _ = writeln!(
        s,
        "    try {{ {}; }} catch (_e) {{ r.rcFill = rideCode(_e); codeF = rideCodeName(_e); }}",
        api.open_and_fill(&args_rej, &opt_names, "core", &fill_args)
    );
    s.push_str("    const cmpO = r.rcOpen === r.rcBatch && codeO === codeB;\n");
    s.push_str("    if (cmpO) r.rej++;\n");
    s.push_str("    if (!cmpO) { r.ok = false; r.leg = r.rcOpen === r.rcBatch ? 4 : 3; }\n");
    s.push_str("    const cmpF = r.rcFill === r.rcBatch && codeF === codeB;\n");
    s.push_str("    if (cmpF) r.rej++;\n");
    s.push_str("    if (!cmpF) { r.ok = false; r.leg = r.rcFill === r.rcBatch ? 4 : 3; }\n");
    s.push_str("    return;\n  }\n");
    s.push_str("  if (lb < 0) { r.skip = 7; return; }\n");
    s.push_str("  if (nb === 0) { r.skip = 5; return; }\n");
    s.push_str("  if (beg !== lb) { r.skip = 6; return; }\n\n");

    // Leg 1
    s.push_str("  try {\n");
    s.push_str("    let cmp: boolean;\n");
    let _ = writeln!(s, "    const st = {};", api.open(&args_open, &opt_names, "core"));
    if multi {
        let _ = writeln!(s, "    const uo = new {holder}(); st.value(uo);");
    } else {
        s.push_str("    let uv = st.value();\n");
    }
    s.push_str(&cmp_bar("    ", "lb - beg", "lb"));
    s.push_str("    for (let t = lb + 1; r.ok && t < m; t++) {\n");
    if multi {
        let _ = writeln!(s, "      st.update({upd_args}uo);");
    } else {
        let _ = writeln!(s, "      uv = st.update({});", upd_args.trim_end().trim_end_matches(','));
    }
    s.push_str(&cmp_bar("      ", "t - beg", "t"));
    s.push_str("    }\n");
    s.push_str("  } catch (_e) { r.ok = false; r.leg = 1; }\n\n");

    // Leg 2
    s.push_str("  if (r.ok) {\n");
    s.push_str(&fill_decl);
    s.push_str("    try {\n");
    let _ = writeln!(s, "      const f2 = {};", api.open_and_fill(&args_m, &opt_names, "core", &fill_args));
    s.push_str("      const st2 = f2.stream;\n");
    let result_lens = api.result_keys().iter().map(|k| format!(" || f2.{k}.length !== nb")).collect::<Vec<_>>().concat();
    let _ = writeln!(s, "      if (f2.begIdx !== beg{result_lens}) {{ r.ok = false; r.leg = 2; }}");
    s.push_str("      if (st2.outRange().begIdx !== beg || st2.outRange().count !== nb) { r.ok = false; r.leg = 2; }\n");
    s.push_str("      if (r.ok) {\n");
    s.push_str("        for (let k = 0; k < nb; k++) {\n");
    s.push_str("          let cmp = true;\n");
    for (k, (is_int, slot)) in slot_of.iter().enumerate() {
        if *is_int {
            let _ = writeln!(
                s,
                "          if (cmp && fib{slot}[k] !== rib{slot}[k]) {{ cmp = false; r.out = {k}; r.batch = float64Bits(rib{slot}[k]); r.stream = float64Bits(fib{slot}[k]); }}"
            );
        } else {
            let _ = writeln!(
                s,
                "          if (cmp && svXtierNe(rb{slot}[k], fb{slot}[k], r.benign)) {{ cmp = false; r.out = {k}; r.batch = float64Bits(rb{slot}[k]); r.stream = float64Bits(fb{slot}[k]); }}"
            );
        }
    }
    s.push_str("          if (cmp) r.fillBars++;\n");
    s.push_str("          if (!cmp) { r.ok = false; r.leg = 2; r.bar = beg + k; break; }\n");
    s.push_str("        }\n      }\n");
    s.push_str("    } catch (_e) { r.ok = false; r.leg = 2; }\n");
    s.push_str("  }\n\n");

    s.push_str("  if (r.ok) {\n");
    s.push_str("    rideSeenUsed[slot] = 1; rideSeenA[slot] = hash.a; rideSeenB[slot] = hash.b;\n");
    s.push_str("    rideSeenOpen[slot] = r.openBars; rideSeenFill[slot] = r.fillBars;\n");
    s.push_str("  }\n");
    s.push_str("}\n\n");
    s
}
